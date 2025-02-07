#include "bootloader.hxx"
#include "misc.hxx"
#include "cmetawriter.hxx"
#include "cenvironmentwriter.hxx"
#include "constants.hxx"
#include "bootassembler.hxx"
using namespace beastie;

#include <cassert>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <format>
#include <tuple>
#include <span>

#include <elf.h>
#include <linux/kexec.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <endian.h>

beastie::Bootloader::Bootloader()
    : m_debug(false)
    , m_efi(isEFI())
    , m_howto(0)
    , m_btext(0)
    , m_kernblock()
    , m_bootblock()
    , m_metaphys(0)
    , m_kernend(0)
    , m_fb(fetchFB())
    , m_rsdp(0)
    , m_rsdt(0)
    , m_segments()
    , m_nr_segments(0)
    , m_env()
    , m_meta()
    , m_kernphys(0)
    , m_symphys(0)
    , m_envphys(0)
    , m_bootphys(0)
    , m_sym()
    , m_smap(fetchSMAP())
    , m_efimap(fetchEFIMAP())
    , m_force(false)
{
    std::tie(m_rsdp, m_rsdt) = fetchACPI20(m_efi);
    writeDefaultEnv();
    setDefaultResolution();
}

beastie::Bootloader::~Bootloader()
{
    unload();
}

void beastie::Bootloader::setDebug(bool debug)
{
    m_debug = debug;
}

void beastie::Bootloader::setHowto(uint32_t howto)
{
    m_howto = howto;
}

void beastie::Bootloader::setForce(bool force)
{
    m_force = force;
}

void beastie::Bootloader::setDefaultResolution()
{
    m_fb.width = 1024;
    m_fb.height = 768;
}

void beastie::Bootloader::fileLoad(std::filesystem::path path)
{
    auto kernel = slurp<std::vector<char>>(path);
    return elfLoad(std::move(kernel));
}

/*
 * Documentation for the font format (.fnt files):
 *
 * header
 *   . char[8] magic
 *   . u8      width
 *   . u8      height
 *   . u16     pad
 *   . u32     glyph count
 *   . u32[]   map count
 *
 * glyph data
 *   . char[]  data
 *
 * font mappings (4 in total, repeats map count times)
 *   . u32     src
 *   . u16     dst
 *   . u16     len
 *
 * NOTES:
 *   - All the fields are stored in big endian format.
 *   - The glyph data has a size of (glyph_count * width * height) bytes,
 *       where width is rounded to the next multiple of 8.
 *   - There are normally 4 mapping tables:
 *       normal, normal right, bold, bold right.
 *
 ****/
/*
 * Documentation for in-kernel memory format (metadata type 0x800e)
 *
 * header
 *   . u32     checksum
 *   . u32     width
 *   . u32     height
 *   . u32     bitmap size
 *   . u32[]   map count
 *
 * font mappings (4 in total, repeats map count times)
 *   . u32     src
 *   . u16     dst
 *   . u16     len
 *
 * glyph data
 *   . char[]  data
 *
 * NOTES:
 *   - The glyph data size is in the header.
 *   - See the file format .fnt for mappings.
 *
 ****/
void beastie::Bootloader::fontLoad(std::filesystem::path path)
{
    unsigned index = 0;
    auto buffer = zslurp(path);
    font_header hdr;
    std::memcpy(&hdr, buffer.data(), sizeof(hdr));

    if (std::string((char*)&hdr.fh_magic[0], 8) != "VFNT0002")
        throw std::runtime_error(std::format("{}: format error", path.string()));

    // The header is stored big endian (!!)
    hdr.fh_glyph_count = be32toh(hdr.fh_glyph_count);
    for (int i = 0; i < VFNT_MAPS; ++i)
        hdr.fh_map_count[i] = be32toh(hdr.fh_map_count[i]);
    index += sizeof(hdr);

    unsigned gbytes = howmany(hdr.fh_width, 8) * hdr.fh_height;
    unsigned glyphCount = hdr.fh_glyph_count;
    unsigned total = glyphCount * gbytes;

    font_info fi;
    fi.fi_width = hdr.fh_width;
    fi.fi_height = hdr.fh_height;
    fi.fi_bitmap_size = total;
    for (int i = 0; i < VFNT_MAPS; ++i)
        fi.fi_map_count[i] = hdr.fh_map_count[i];

    uint32_t checksum;
    checksum = fi.fi_width;
    checksum += fi.fi_height;
    checksum += fi.fi_bitmap_size;
    for (int i = 0; i < VFNT_MAPS; ++i)
        checksum += fi.fi_map_count[i];

    fi.fi_checksum = -checksum;

    uint8_t fontBytes[total];
    std::memcpy(fontBytes, buffer.data() + index, sizeof(fontBytes));
    index += sizeof(fontBytes);

    std::vector<vfnt_map> maps[VFNT_MAPS];
    for (int i = 0; i < VFNT_MAPS; ++i) {
        for (unsigned j = 0; j < hdr.fh_map_count[i]; ++j) {
            vfnt_map map;
            std::memcpy(&map, buffer.data() + index, sizeof(map));
            map.vfm_src = be32toh(map.vfm_src);
            map.vfm_dst = be16toh(map.vfm_dst);
            map.vfm_len = be16toh(map.vfm_len);
            maps[i].push_back(map);
            index += sizeof(map);
        }
    }

    m_fontblock.clear();

    // 1. insert the header
    std::span<char> spanFontHdr((char*)&fi, sizeof(fi));
    m_fontblock.insert(m_fontblock.end(), spanFontHdr.begin(), spanFontHdr.end());

    // 2. insert the maps
    for (int i = 0; i < VFNT_MAPS; ++i) {
        for (vfnt_map map : maps[i]) {
            std::span<char> spanFontMap((char*)&map, sizeof(map));
            m_fontblock.insert(m_fontblock.end(), spanFontMap.begin(), spanFontMap.end());
        }
    }

    // 3. insert glyph data
    std::span<char> spanFontGlyph((char*)fontBytes, total);
    m_fontblock.insert(m_fontblock.end(), spanFontGlyph.begin(), spanFontGlyph.end());
    assert(index == buffer.size());
}

void beastie::Bootloader::elfLoad(std::vector<char>&& buffer)
{
    Elf64_Ehdr hdr;
    bool isKernel;
    bool isModule;

    std::memcpy(&hdr, buffer.data(), sizeof(hdr));
    assert(hdr.e_ident[EI_MAG0] == 0x7f);
    assert(hdr.e_ident[EI_MAG1] == 0x45);
    assert(hdr.e_ident[EI_MAG2] == 0x4c);
    assert(hdr.e_ident[EI_MAG3] == 0x46);
    assert(hdr.e_ident[EI_CLASS] == 0x02);
    assert(hdr.e_ident[EI_DATA] == 0x01);
    assert(hdr.e_ident[EI_VERSION] == 0x01);
    assert(hdr.e_ident[EI_OSABI] == 0x09);
    assert(hdr.e_machine == 0x3e); // amd64
    assert(hdr.e_version == 0x01);

    isKernel = (hdr.e_type == ET_EXEC);
    isModule = (hdr.e_type == ET_REL);
    assert(isKernel || isModule);

    if (isKernel) {
        elfLoadExec(hdr, std::move(buffer));
    }

    if (isModule) {
        elfLoadRel(hdr, std::move(buffer));
    }
}

void beastie::Bootloader::elfLoadExec(Elf64_Ehdr hdr, std::vector<char>&& buffer)
{
    Elf64_Phdr phdr[hdr.e_phnum];
    Elf64_Shdr shdr[hdr.e_shnum];

    std::memcpy(phdr, buffer.data() + hdr.e_phoff, sizeof(phdr));
    std::memcpy(shdr, buffer.data() + hdr.e_shoff, sizeof(shdr));

    this->m_btext = hdr.e_entry;
    assert(this->m_btext);

    for (int i = 0; i < hdr.e_phnum; ++i) {
        if (phdr[i].p_type != PT_LOAD)
            continue;

        // the entries are sorted by vaddr, and there's always a 2 MiB hole
        // in front of the kernel that must be removed
        uintptr_t paddr = phdr[i].p_vaddr - KERNBASE - 0x200000;
        Elf64_Off offset = phdr[i].p_offset;
        Elf64_Xword memsz = phdr[i].p_memsz;

        m_kernblock.resize(paddr + memsz);

        if (m_debug)
            std::cout << std::format("[PT_LOAD]  phys=0x{:x} size=0x{:x} off=0x{:x}\n",
                                     paddr,
                                     phdr[i].p_memsz,
                                     offset);

        char* dst = &m_kernblock.data()[paddr];
        char* src = &buffer.data()[offset];
        std::memcpy(dst, src, phdr[i].p_filesz);
    }

    for (int i = 0; i < hdr.e_shnum; ++i) {
        if (shdr[i].sh_type != SHT_SYMTAB)
            continue;

        std::vector<char> symtab(shdr[i].sh_size);
        Elf64_Off offset = shdr[i].sh_offset;

        char* dst = symtab.data();
        char* src = &buffer.data()[offset];
        std::memcpy(dst, src, shdr[i].sh_size);

        m_sym.addSymTab(symtab);
        break;
    }

    for (int i = 0; i < hdr.e_shnum; ++i) {
        if (shdr[i].sh_type != SHT_STRTAB)
            continue;

        std::vector<char> strtab(shdr[i].sh_size);
        Elf64_Off offset = shdr[i].sh_offset;

        char* dst = strtab.data();
        char* src = &buffer.data()[offset];
        std::memcpy(dst, src, shdr[i].sh_size);

        m_sym.addStrTab(strtab);
        break;
    }

    // XXX move to boot()
    m_kernphys = 0x20'0000;
    m_symphys = m_kernphys + roundup(m_kernblock.size(), 4096);
    m_envphys = m_symphys + roundup(m_sym.size(), 4096);
    m_fontphys = m_envphys + roundup(m_env.size(), 4096);
    m_metaphys = m_fontphys + roundup(m_fontblock.size(), 4096);

    writeMetadata();
    m_kernend = m_metaphys + roundup(m_meta.size(), 4096);


    BootAssembler ba(m_btext, m_metaphys, m_kernend, m_fb);
    ba.assemble();
    if (m_debug)
        ba.debug();
    m_bootblock = ba.data();
    m_bootphys = 0x10'0000;
}

void beastie::Bootloader::elfLoadRel(Elf64_Ehdr hdr, std::vector<char>&& buffer)
{
    assert(hdr.e_phnum == 0);
    Elf64_Shdr shdr[hdr.e_shnum];

    // XXX TODO

    std::memcpy(shdr, buffer.data() + hdr.e_shoff, sizeof(shdr));
    assert(hdr.e_entry == 0);
}

void beastie::Bootloader::unload()
{
    if (syscall(SYS_kexec_load, 0, 0, nullptr, KEXEC_FILE_UNLOAD))
    {
        throw std::runtime_error(std::strerror(errno));
    }
}

void beastie::Bootloader::load()
{
    prepareSegments();

    if (m_debug) {
        for (unsigned int i = 0; i < m_nr_segments; ++i) {
            std::cout << std::format("kexec segment: mem={:p} memsz={:08x}\n",
                                     m_segments[i].mem,
                                     m_segments[i].memsz);
        }
    }

    if (syscall(SYS_kexec_load, getEntry(), m_nr_segments, m_segments, KEXEC_ARCH_X86_64))
    {
        throw std::runtime_error(std::strerror(errno));
    }
}

uintptr_t beastie::Bootloader::getEntry()
{
    return 0x10'0000;
}

void beastie::Bootloader::boot()
{
    load();
    if (m_force)
        forcedshutdown();
    else
        shutdown();
}

void beastie::Bootloader::writeDefaultEnv()
{
    m_env += std::format("acpi.rsdp=0x{:x}", m_rsdp);
    m_env += std::format("acpi.rsdt=0x{:x}", m_rsdt);
    m_env += "hint.uart.0.at=acpi";
    m_env += "hint.uart.0.port=0x3f8";
    m_env += "hint.uart.0.flags=0x10";
}

void beastie::Bootloader::prepareSegments()
{
    m_nr_segments = 0;

    if (m_kernblock.size() > 0) {
        m_segments[0].buf = m_kernblock.data();
        m_segments[0].bufsz = m_kernblock.size();
        m_segments[0].mem = reinterpret_cast<const void*>(m_kernphys);
        m_segments[0].memsz = roundup(m_kernblock.size(), 4096);
        m_nr_segments++;
    }

    if (m_sym.size() > 0) {
        m_segments[1].buf = m_sym.data();
        m_segments[1].bufsz = m_sym.size();
        m_segments[1].mem = reinterpret_cast<const void*>(m_symphys);
        m_segments[1].memsz = roundup(m_sym.size(), 4096);
        m_nr_segments++;
    }

    if (m_env.size() > 0) {
        m_segments[2].buf = m_env.data();
        m_segments[2].bufsz = m_env.size();
        m_segments[2].mem = reinterpret_cast<const void*>(m_envphys);
        m_segments[2].memsz = roundup(m_env.size(), 4096);
        m_nr_segments++;
    }

    if (m_meta.size() > 0) {
        m_segments[3].buf = m_meta.data();
        m_segments[3].bufsz = m_meta.size();
        m_segments[3].mem = reinterpret_cast<const void*>(m_metaphys);
        m_segments[3].memsz = roundup(m_meta.size(), 4096);
        m_nr_segments++;
    }

    if (m_bootblock.size() > 0) {
        m_segments[4].buf = m_bootblock.data();
        m_segments[4].bufsz = m_bootblock.size();
        m_segments[4].mem = reinterpret_cast<const void*>(m_bootphys);
        m_segments[4].memsz = roundup(m_bootblock.size(), 4096);
        m_nr_segments++;
    }

    if (m_fontblock.size() > 0) {
        m_segments[5].buf = m_fontblock.data();
        m_segments[5].bufsz = m_fontblock.size();
        m_segments[5].mem = reinterpret_cast<const void*>(m_fontphys);
        m_segments[5].memsz = roundup(m_fontblock.size(), 4096);
        m_nr_segments++;
    }
}

void beastie::Bootloader::writeMetadata()
{
    m_meta.clear();

    m_meta.addName("/boot/kernel/kernel");
    m_meta.addType("elf kernel");
    m_meta.addAddr(m_kernphys);
    m_meta.addSize(m_kernblock.size());  // XXX may not be accurate

    /* extended types */
    assert(m_symphys);
    assert(m_sym.size());
    m_meta.addMetadata(MODINFO_METADATA | MODINFOMD_SSYM, uintptr_t(m_symphys));
    m_meta.addMetadata(MODINFO_METADATA | MODINFOMD_ESYM, uintptr_t(m_symphys + m_sym.size()));
    assert(m_envphys);
    m_meta.addMetadata(MODINFO_METADATA | MODINFOMD_ENVP, uintptr_t(m_envphys));
    m_meta.addMetadata(MODINFO_METADATA | MODINFOMD_HOWTO, m_howto);
    m_meta.addMetadata(MODINFO_METADATA | MODINFOMD_FW_HANDLE, uintptr_t(m_rsdp));

    if (m_efi == false) {
        std::span<char> smapSpan((char*)m_smap.e820_table,
                                 m_smap.e820_entries * sizeof(boot_e820_entry));
        m_meta.addMetadata(MODINFO_METADATA | MODINFOMD_SMAP, smapSpan);
    } else {
        std::span<char> efimapSpan((char*)&m_efimap, sizeof(efimapinfo));
        m_meta.addMetadata(MODINFO_METADATA | MODINFOMD_EFI_MAP, efimapSpan);
    }

    efifbinfo efifb;
    efifb.addr = m_fb.phys;
    efifb.size = m_fb.width * m_fb.height * 4;
    efifb.height = m_fb.height;
    efifb.width = m_fb.width;
    efifb.stride = m_fb.width;
    efifb.maskRed = m_fb.mask_red;
    efifb.maskGreen = m_fb.mask_green;
    efifb.maskBlue = m_fb.mask_blue;
    efifb.maskReserved = 0xff000000;
    std::span<char> efifbSpan((char*)&efifb, sizeof(efifb));
    m_meta.addMetadata(MODINFO_METADATA | MODINFOMD_EFI_FB, efifbSpan);

    m_meta.addMetadata(MODINFO_METADATA | MODINFOMD_FONT, uintptr_t(m_fontphys));

    m_meta.addEnd();
}

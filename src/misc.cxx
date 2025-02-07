#include "misc.hxx"
#include "constants.hxx"
using namespace beastie;

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <utility>

#include <asm/bootparam.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/reboot.h>
#include <sys/io.h>
#include <sys/ioctl.h>
#include <syscall.h>
#include <unistd.h>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/copy.hpp>

template<class T>
T beastie::slurp(std::filesystem::path path)
{
    T buffer;

    std::ifstream file(path, std::ios::binary | std::ios::in);
    typedef std::istreambuf_iterator<char> Iter;
    if (file) {
        buffer.insert(buffer.end(), Iter(file), Iter());
        file.close();
    } else {
        throw std::runtime_error(std::strerror(errno));
    }

    return buffer;
}
template
std::string beastie::slurp<std::string>(std::filesystem::path path);
template
std::vector<char> beastie::slurp<std::vector<char>>(std::filesystem::path path);

std::vector<std::string> beastie::slurpLines(std::filesystem::path path)
{
    std::vector<std::string> lines;
    std::string line;

    std::ifstream file(path, std::ios::in);
    if (file) {
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        file.close();
    } else {
        throw std::runtime_error(std::strerror(errno));
    }

    return lines;
}

unsigned long long beastie::slurpULL(std::filesystem::path path)
{
    unsigned long long result;
    auto contents = slurp(path);
    result = std::stoull(contents, 0, 0);
    return result;
}

std::vector<char> beastie::zslurp(std::filesystem::path path)
{
    std::vector<char> buffer;
    typedef std::istreambuf_iterator<char> Iter;

    std::ifstream file(path, std::ios::binary | std::ios::binary);
    unsigned char magic0, magic1;
    file >> magic0 >> magic1;
    file.seekg(0);

    if (magic0 == 0x1f && magic1 == 0x8b) {
        boost::iostreams::filtering_istreambuf in;
        in.push(boost::iostreams::gzip_decompressor());
        in.push(file);
        buffer.assign(Iter(&in), {});
        return buffer;
    } else {
        return slurp<std::vector<char>>(path);
    }
}

void beastie::printBuffer(std::span<char> vs, std::string_view name)
{
    int address = 0;

    std::cout << std::format("[{}]\n", name);
    while (address < vs.size()) {
        std::cout << std::format("[{:016x}]  {:02x} {:02x} {:02x} {:02x}  {:02x} {:02x} {:02x} {:02x}\n",
                                 address,
                                 vs[address + 0],
                                 vs[address + 1],
                                 vs[address + 2],
                                 vs[address + 3],
                                 vs[address + 4],
                                 vs[address + 5],
                                 vs[address + 6],
                                 vs[address + 7]);
        address += 8;
    }
}

bool beastie::isEFI()
{
    std::string FIRMWARE_EFI_DIR = "/sys/firmware/efi";
    std::filesystem::path p(FIRMWARE_EFI_DIR);
    return std::filesystem::exists(p) &&
           std::filesystem::is_directory(p) &&
           ! std::filesystem::is_empty(p);
}

void beastie::shutdown()
{
    const char* args[] = {
        "shutdown",
        "-r",
        "now",
        NULL
    };

    execv("/sbin/shutdown", const_cast<char**>(args));
    execv("/etc/shutdown", const_cast<char**>(args));
    execv("/bin/shutdown", const_cast<char**>(args));

    throw std::runtime_error("shutdown");
}

void beastie::forcedshutdown()
{
    syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_KEXEC, nullptr);
}

std::pair<uintptr_t,uintptr_t> beastie::fetchACPI20(bool efi)
{
    /*
     * Advanced Configuration and Power Interface (ACPI) Specification
     * 5.2.5.2 Finding the RSDP on UEFI Enabled Systems
     */
    intptr_t acpi20 = 0;
    intptr_t rsdt = 0;

    if (efi) {
        std::string systab, line;
        unsigned char hdr[20];
        systab = slurp<std::string>("/sys/firmware/efi/systab");

        std::istringstream stream(systab);
        while(std::getline(stream, line) && line.starts_with("ACPI20=")) {
            size_t pos = line.find('=');
            acpi20 = std::stoull(line.substr(pos + 1), 0, 16);
        }
        assert(acpi20);

        /* look up rsdt in /dev/mem */
        std::FILE* fd = std::fopen("/dev/mem", "rb");
        assert(fd);
        std::fseek(fd, acpi20, SEEK_SET);
        std::fread(hdr, 20, 1, fd);
        std::fclose(fd);

        rsdt = *reinterpret_cast<uint32_t*>(hdr + 16);
        //assert(rsdt);
    } else {
        /* try old boot time params */
        struct boot_params bp;

        std::filesystem::path bpd("/sys/kernel/boot_params/data");
        std::ifstream file(bpd);
        if (file.is_open() == false) {
            throw std::runtime_error(std::format("{}: {}", bpd.string(), strerror(errno)));
        }

        file.read(reinterpret_cast<char*>(&bp), sizeof(bp));
        acpi20 = bp.acpi_rsdp_addr;
        assert(acpi20);

        /* look up rsdt in /dev/mem */
        unsigned char hdr[20];
        std::FILE* fd = std::fopen("/dev/mem", "rb");
        assert(fd);
        std::fseek(fd, acpi20, SEEK_SET);
        std::fread(hdr, 20, 1, fd);
        std::fclose(fd);

        rsdt = *reinterpret_cast<uint32_t*>(hdr + 16);
        assert(rsdt);
    }
    return {acpi20, rsdt};
}

static void vmware_write(uint32_t iostart, uint32_t reg, uint32_t value)
{
    outl(reg, iostart);
    outl(value, iostart + 1);
}

fbinfo beastie::fetchFB()
{
    std::filesystem::path fp("/dev/fb0");
    std::ifstream file("/dev/fb0", std::ios::binary | std::ios::in);
    if (file.is_open() == false) {
        throw std::runtime_error(std::format("{}: {}", fp.string(), strerror(errno)));
    }

    struct fb_fix_screeninfo fix;
    struct fb_var_screeninfo var;

#if defined(__cpp_lib_fstream_native_handle)
    int fd = file.native_handle();
#else
    struct Hack : std::basic_filebuf<char> {
        Hack(std::basic_filebuf<char> rhs)
            :std::basic_filebuf<char>(std::move(rhs))
        {}
        int fd() {
            return this->_M_file.fd();
        }
    } hack = std::move(*file.rdbuf());
    int fd = hack.fd();
    *file.rdbuf() = std::move(hack);
#endif

    if (-1 == ioctl(fd, FBIOGET_FSCREENINFO, &fix) ||
        -1 == ioctl(fd, FBIOGET_VSCREENINFO, &var)) {
        throw std::runtime_error(std::format("{}: {}", fp.string(), strerror(errno)));
    }

    fbinfo fb0;
    std::string_view id(fix.id);
    if (id == "EFI VGA") {
        fb0.id = id;
        fb0.phys = fix.smem_start;
        fb0.size = fix.smem_len;
        fb0.width = var.xres;
        fb0.height = var.yres;
        fb0.mask_red = 0xff << var.red.offset;
        fb0.mask_green = 0xff << var.green.offset;
        fb0.mask_blue = 0xff << var.blue.offset;
        fb0.mask_reserved = 0xff000000;
        return fb0;

    } else {
        /* try old boot time video */
        struct screen_info si;

        std::filesystem::path path("/sys/kernel/boot_params/data");
        std::ifstream file(path);
        if (file.is_open() == false) {
            throw std::runtime_error(std::format("{}: {}", path.string(), strerror(errno)));
        }
        file.read(reinterpret_cast<char*>(&si), sizeof(si));

        fb0.id = id;
        fb0.phys = si.lfb_base + (static_cast<uint64_t>(si.ext_lfb_base) << 32);
        fb0.size = fix.smem_len;
        fb0.width = var.xres;
        fb0.height = var.yres;
        fb0.mask_red = 0xff << var.red.offset;
        fb0.mask_green = 0xff << var.green.offset;
        fb0.mask_blue = 0xff << var.blue.offset;
        fb0.mask_reserved = 0xff000000;
        return fb0;
    }
    throw std::runtime_error("fb0 not found");
}

smapinfo beastie::fetchSMAP(bool debug)
{
    /* try old boot time params */
    struct boot_params bp;

    std::filesystem::path path("/sys/kernel/boot_params/data");
    std::ifstream file(path);
    if (file.is_open() == false) {
        throw std::runtime_error(std::format("{}: {}", path.string(), strerror(errno)));
    }

    file.seekg(0);
    file.read(reinterpret_cast<char*>(&bp), sizeof(bp));
    assert(bp.e820_entries);
    file.close();

    smapinfo si;
    std::memset(&si, 0, sizeof(si));
    si.e820_entries = bp.e820_entries;
    for (int i = 0; i < 128; ++i) {
        si.e820_table[i].addr = bp.e820_table[i].addr;
        si.e820_table[i].size = bp.e820_table[i].size;
        si.e820_table[i].type = bp.e820_table[i].type;

        if (debug && (si.e820_table[i].size > 0)) {
            std::cout << std::format("SMAP  phys={:x} size={:x} type={:d}\n",
                                     uint64_t(si.e820_table[i].addr),
                                     uint64_t(si.e820_table[i].size),
                                     uint32_t(si.e820_table[i].type));
        }
    }
    return (si);
}

efimapinfo beastie::fetchEFIMAP()
{

    /*
     * Linux doesn't include the memory descriptors for system memory in
     * the efi/runtime-map, so take the descriptors from the e820 map
     **/

    auto map = fetchSMAP(false);

    efimapinfo ei;
    std::memset(&ei, 0, sizeof(ei));
    ei.memory_size = map.e820_entries * sizeof(efimapentry);
    ei.descriptor_size = sizeof(efimapentry);
    ei.descriptor_version = 1;

    for (int i = 0; i < 128; ++i) {
        if (map.e820_table[i].type != SMAP_TYPE_MEMORY)
            continue;

        ei.efi_table[i].type = EFI_MD_TYPE_FREE;
        ei.efi_table[i].phys = map.e820_table[i].addr;
        ei.efi_table[i].virt = 0;
        ei.efi_table[i].pages = map.e820_table[i].size / 4096;
        ei.efi_table[i].attr = 0x0f;  // seems to be standard attr
    }

    return (ei);
}

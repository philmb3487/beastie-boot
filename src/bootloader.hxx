#pragma once

#include "types.hxx"
#include "cenvironmentwriter.hxx"
#include "cmetawriter.hxx"
#include "csymbolswriter.hxx"
using namespace beastie;

#include <filesystem>
#include <vector>
#include <cstdint>

#include <elf.h>
#include <linux/kexec.h>

namespace beastie {
class Bootloader
{
public:
    Bootloader();
    ~Bootloader();

    // Set debug mode
    void setDebug(bool debug);

    // Set boot-time howto params
    void setHowto(uint32_t howto);

    // Set forceful kexec boot (!!)
    void setForce(bool force);

    // Load an ELF kernel/module
    void fileLoad(std::filesystem::path path);

    // Load a font file
    void fontLoad(std::filesystem::path path);

    // Boot into the new system
    void boot();

    // Set the framebuffer to default resolution
    void setDefaultResolution();

private:
    void elfLoad(std::vector<char>&& buffer);
    void elfLoadExec(Elf64_Ehdr hdr, std::vector<char>&& buffer);
    void elfLoadRel(Elf64_Ehdr hdr, std::vector<char>&& buffer);
    uintptr_t getEntry();
    void writeDefaultEnv();
    void prepareSegments();
    void writeMetadata();

    // Unload the kexec segments associated with this instance
    void unload();

    // Load the kexec segments associated with this instance
    void load();

private:
    constexpr static uintptr_t KERNBASE = 0xffff'ffff'8000'0000;
    bool m_debug = false;
    bool m_efi = false;
    uint32_t m_howto;
    uintptr_t m_btext;
    std::vector<char> m_kernblock;
    std::vector<char> m_bootblock;
    uintptr_t m_metaphys;
    uintptr_t m_kernend;
    fbinfo m_fb;
    uintptr_t m_rsdp;
    uintptr_t m_rsdt;
    kexec_segment m_segments[KEXEC_SEGMENT_MAX];
    unsigned long m_nr_segments;
    CEnvironmentWriter m_env;
    CMetaWriter m_meta;
    uintptr_t m_kernphys;
    uintptr_t m_symphys;
    uintptr_t m_envphys;
    uintptr_t m_bootphys;
    CSymbolsWriter m_sym;
    smapinfo m_smap;
    efimapinfo m_efimap;
    bool m_force;
    std::vector<char> m_fontblock;
    uintptr_t m_fontphys;

};
} // namespace beastie

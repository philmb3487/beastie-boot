#pragma once

#include "cgfx.hxx"
using namespace beastie;

#include <cstdint>
#include <vector>

#include <asmjit/asmjit.h>

namespace beastie {
class CVmwaregfx : public CGfx
{
public:
    CVmwaregfx();
    ~CVmwaregfx();

    // Is there a vmware graphics card in this system?
    bool isPresent() override;

    // What's the framebuffer base address?
    uintptr_t base() override;

    void debug() override;

    std::vector<char> assembleReset(int, int) override;

private:
    constexpr static int VENDOR_VMWARE = 0x15AD;
    constexpr static int DEVICE_SVGAII = 0x0405;
    uint16_t m_iostart;
    bool m_present;
    uintptr_t m_fbbase;
    size_t m_fbsize;


    constexpr static int SVGA_REG_ID             = 0;
    constexpr static int SVGA_REG_ENABLE         = 1;
    constexpr static int SVGA_REG_WIDTH          = 2;
    constexpr static int SVGA_REG_HEIGHT         = 3;
    constexpr static int SVGA_REG_MAX_WIDTH      = 4;
    constexpr static int SVGA_REG_MAX_HEIGHT     = 5;
    constexpr static int SVGA_REG_DEPTH          = 6;
    constexpr static int SVGA_REG_BITS_PER_PIXEL = 7;
    constexpr static int SVGA_REG_PSEUDOCOLOR    = 8;
    constexpr static int SVGA_REG_RED_MASK       = 9;
    constexpr static int SVGA_REG_GREEN_MASK     = 10;
    constexpr static int SVGA_REG_BLUE_MASK      = 11;
    constexpr static int SVGA_REG_BYTES_PER_LINE = 12;
    constexpr static int SVGA_REG_FB_START       = 13;
    constexpr static int SVGA_REG_FB_OFFSET      = 14;
    constexpr static int SVGA_REG_VRAM_SIZE      = 15;
    constexpr static int SVGA_REG_FB_SIZE        = 16;


private:
    void write(uint32_t reg, uint32_t value);
    uint32_t read(uint32_t reg);
    void setMode(int w, int h);

    asmjit::x86::Assembler m_asm;
    asmjit::CodeHolder m_code;
    asmjit::Environment m_environment;

    void initAsmJit();

};
} // namespace beastie

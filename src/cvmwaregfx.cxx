#include "cvmwaregfx.hxx"
#include "misc.hxx"
using namespace beastie;

#include <iostream>

#include <sys/io.h>
#include <asmjit/asmjit.h>

const static std::filesystem::path devices = "/sys/bus/pci/devices";

beastie::CVmwaregfx::CVmwaregfx()
    : m_iostart(0)
    , m_present(false)
    , m_fbbase(0)
    , m_fbsize(0)
    , m_asm()
    , m_code()
    , m_environment()
{
    initAsmJit();

    for (auto const& dir_entry : std::filesystem::directory_iterator{devices}) {
        uint16_t device = slurpULL(dir_entry.path()/"device") & 0xffff;
        uint16_t vendor = slurpULL(dir_entry.path()/"vendor") & 0xffff;

        if (vendor == VENDOR_VMWARE &&
            device == DEVICE_SVGAII) {
            m_present = true;

            // BAR0: I/O ports
            m_iostart = slurpULL(dir_entry.path()/"resource") & 0xffff;
        }
    }

    if (m_present) {
        ioperm(m_iostart, 16, 1);
        m_fbbase = read(SVGA_REG_FB_START);
        m_fbsize = read(SVGA_REG_FB_SIZE);
    }
}

beastie::CVmwaregfx::~CVmwaregfx()
{
    ioperm(m_iostart, 16, 0);
}

bool beastie::CVmwaregfx::isPresent()
{
    return m_present;
}

uintptr_t beastie::CVmwaregfx::base()
{
    return (m_fbbase);
}

void beastie::CVmwaregfx::write(uint32_t reg, uint32_t value)
{
    outl(reg, m_iostart);
    outl(value, m_iostart + 1);
}

uint32_t beastie::CVmwaregfx::read(uint32_t reg)
{
    outl(reg, m_iostart);
    return (inl(m_iostart + 1));
}

void beastie::CVmwaregfx::debug()
{
    std::cout << std::format("i915   base=0x{:x} port=0x{:04x}\n", this->base(), this->m_iostart);
}

void beastie::CVmwaregfx::setMode(int w, int h)
{
    write(SVGA_REG_ENABLE, 0);
    write(SVGA_REG_ID, 0);
    write(SVGA_REG_WIDTH, 640);
    write(SVGA_REG_HEIGHT, 480);
    write(SVGA_REG_BITS_PER_PIXEL, 32);
    write(SVGA_REG_ENABLE, 1);
}

std::vector<char> beastie::CVmwaregfx::assembleReset(int width,
                                                     int height)
{
    using namespace asmjit::x86::regs;

    auto outl = [this](unsigned int value, unsigned short int port)
    {
        m_asm.mov(eax, value);
        m_asm.mov(edx, port);
        m_asm.out(dx, eax);
    };
    auto write = [this, outl](uint32_t reg, uint32_t value) {
        outl(reg, this->m_iostart);
        outl(value, this->m_iostart + 1);
    };

    write(SVGA_REG_ENABLE, 0);
    //write(SVGA_REG_ID, 0);
    write(SVGA_REG_WIDTH, width);
    write(SVGA_REG_HEIGHT, height);
    write(SVGA_REG_BITS_PER_PIXEL, 32);
    write(SVGA_REG_BYTES_PER_LINE, height * 4);
    write(SVGA_REG_ENABLE, 1);

    m_asm.finalize();
    std::vector<char> buffer(m_code.codeSize());
    m_code.copySectionData(buffer.data(), buffer.size(), 0);
    return buffer;
}

void beastie::CVmwaregfx::initAsmJit()
{
    m_environment.setArch(asmjit::Arch::kX64);
    m_code.init(m_environment);
    m_code.attach(&m_asm);
    // Enable strict validation.
    m_asm.addDiagnosticOptions(asmjit::DiagnosticOptions::kValidateAssembler);
}

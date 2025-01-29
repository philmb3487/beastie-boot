#pragma once

#include "types.hxx"
using namespace beastie;

#include <cstdint>
#include <vector>

#include <asmjit/asmjit.h>

namespace beastie {
class BootAssembler
{
public:
    BootAssembler(uintptr_t btext,
                  uintptr_t modulep,
                  uintptr_t kernend,
                  fbinfo fb);
    void assemble();
    void debug();

    std::vector<char> data();

private:
    asmjit::x86::Assembler m_asm;
    asmjit::CodeHolder m_code;
    asmjit::Environment m_environment;
    asmjit::Section *m_text, *m_data;
    uintptr_t m_btext;
    uintptr_t m_modulep;
    uintptr_t m_kernend;
    fbinfo m_fb;
    std::vector<char> m_gfxcode;

    struct {
        asmjit::Label entry;
        asmjit::Label GDT;
        asmjit::Label GDTP;
        asmjit::Label stackTop;
        asmjit::Label PML4T;
        asmjit::Label PDPT[2];
        asmjit::Label PDT[2];
    } m_labels;

    void initAsmJit();
    void createLayout();
    void createGlobalLabels();

    void assembleText();
    void assembleData();

    // XXX  https://github.com/asmjit/asmjit/discussions/464
    void align(int i)
    {
        while (m_asm.offset() % i) {
            m_asm.db(0x00);
        }
    }
};
} // namespace beastie

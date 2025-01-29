#include "bootassembler.hxx"
#include "types.hxx"
#include "cgfx.hxx"
#include "cvmwaregfx.hxx"
using namespace beastie;

#include <cassert>
#include <cstdint>
#include <format>
#include <iostream>
#include <stdexcept>

#include <asmjit/asmjit.h>

namespace beastie {
#ifndef PAGE_SIZE
constexpr int PAGE_SIZE = 4096;
#endif
constexpr int STACK_SIZE = 1 * PAGE_SIZE;
constexpr int LOWBASE = 0x10'0000;

class MyErrorHandler : public asmjit::ErrorHandler {
public:
    void handleError(asmjit::Error err, const char* message, asmjit::BaseEmitter* origin) override {
        throw std::runtime_error(std::format("{}", message));
    };
} hMyErrorHandler;
} // namespace beastie

BootAssembler::BootAssembler(uintptr_t btext,
                             uintptr_t modulep,
                             uintptr_t kernend,
                             fbinfo fb)
    : m_asm()
    , m_code()
    , m_environment()
    , m_text(nullptr)
    , m_data(nullptr)
    , m_btext(btext)
    , m_modulep(modulep)
    , m_kernend(kernend)
    , m_fb(fb)
    , m_gfxcode()
{
    assert(modulep < kernend);
    initAsmJit();
    createLayout();
    createGlobalLabels();

    // generate VGA reset code if needed
    if (m_fb.id == "vmwgfxdrmfb") {
        CGfx* gfx = new CVmwaregfx();
        m_gfxcode = gfx->assembleReset(m_fb.width,
                                       m_fb.height);
        delete gfx;
    }
}

void BootAssembler::initAsmJit()
{
    m_environment.setArch(asmjit::Arch::kX64);
    m_code.init(m_environment, LOWBASE);
    m_code.setErrorHandler(&hMyErrorHandler);
    m_code.attach(&m_asm);
    // Enable strict validation.
    m_asm.addDiagnosticOptions(asmjit::DiagnosticOptions::kValidateAssembler);
}

void BootAssembler::createLayout()
{
    m_text = m_code.textSection();
    m_code.newSection(&m_data, ".data", SIZE_MAX, asmjit::SectionFlags::kReadOnly, 0, 1);

    m_text->setOffset(0x0'000);
    m_data->setOffset(0x1'000);
    m_text->setVirtualSize(0x1'000);
    m_data->setVirtualSize(0xf'000);
}

void BootAssembler::createGlobalLabels()
{
    m_labels.entry = m_asm.newNamedLabel("entry", SIZE_MAX, asmjit::LabelType::kGlobal);
    m_labels.GDT = m_asm.newNamedLabel("GDT", SIZE_MAX, asmjit::LabelType::kGlobal);
    m_labels.GDTP = m_asm.newNamedLabel("GDTP", SIZE_MAX, asmjit::LabelType::kGlobal);
    m_labels.stackTop = m_asm.newNamedLabel("stackTop", SIZE_MAX, asmjit::LabelType::kGlobal);
    m_labels.PML4T = m_asm.newNamedLabel("PML4T", SIZE_MAX, asmjit::LabelType::kGlobal);
    m_labels.PDPT[0] = m_asm.newNamedLabel("PDPT[0]", SIZE_MAX, asmjit::LabelType::kGlobal);
    m_labels.PDPT[1] = m_asm.newNamedLabel("PDPT[1]", SIZE_MAX, asmjit::LabelType::kGlobal);
    m_labels.PDT[0] = m_asm.newNamedLabel("PDT[0]", SIZE_MAX, asmjit::LabelType::kGlobal);
    m_labels.PDT[1] = m_asm.newNamedLabel("PDT[1]", SIZE_MAX, asmjit::LabelType::kGlobal);
}

void BootAssembler::assemble()
{
    assembleText();
    assembleData();
    m_code.resolveUnresolvedLinks();
    m_code.relocEntries();
}

//
// Reset the VGA hardware
// I think this is from XEN code
//
static void resetVGA(asmjit::x86::Assembler& a) {
    using namespace asmjit;
    using namespace asmjit::x86;
    using namespace asmjit::x86::regs;

    auto inb = [&a](uint16_t port) {
        a.mov(dx, port);
        a.in(al, dx);
    };
    auto outb = [&a](uint8_t val, uint16_t port) {
        a.mov(dx, port);
        a.mov(al, val);
        a.out(dx, al);
    };
    auto outw = [&a](uint16_t val, uint16_t port) {
        a.mov(dx, port);
        a.mov(ax, val);
        a.out(dx, al);
    };

    // This is from XEN code I believe
    inb(0x3da);
    outb(0, 0x3c0);
    outw(0x0300, 0x3c4);
    outw(0x0001, 0x3c4);
    outw(0x0302, 0x3c4);
    outw(0x0003, 0x3c4);
    outw(0x0204, 0x3c4);
    outw(0x0e11, 0x3d4);
    outw(0x5f00, 0x3d4);
    outw(0x4f01, 0x3d4);
    outw(0x5002, 0x3d4);
    outw(0x8203, 0x3d4);
    outw(0x5504, 0x3d4);
    outw(0x8105, 0x3d4);
    outw(0xbf06, 0x3d4);
    outw(0x1f07, 0x3d4);
    outw(0x0008, 0x3d4);
    outw(0x4f09, 0x3d4);
    outw(0x200a, 0x3d4);
    outw(0x0e0b, 0x3d4);
    outw(0x000c, 0x3d4);
    outw(0x000d, 0x3d4);
    outw(0x010e, 0x3d4);
    outw(0xe00f, 0x3d4);
    outw(0x9c10, 0x3d4);
    outw(0x8e11, 0x3d4);
    outw(0x8f12, 0x3d4);
    outw(0x2813, 0x3d4);
    outw(0x1f14, 0x3d4);
    outw(0x9615, 0x3d4);
    outw(0xb916, 0x3d4);
    outw(0xa317, 0x3d4);
    outw(0xff18, 0x3d4);
    outw(0x0000, 0x3ce);
    outw(0x0001, 0x3ce);
    outw(0x0002, 0x3ce);
    outw(0x0003, 0x3ce);
    outw(0x0004, 0x3ce);
    outw(0x1005, 0x3ce);
    outw(0x0e06, 0x3ce);
    outw(0x0007, 0x3ce);
    outw(0xff08, 0x3ce);
    inb(0x3da);
    outb(0x00, 0x3c0);
    outb(0x00, 0x3c0);
    inb(0x3da);
    outb(0x01, 0x3c0);
    outb(0x01, 0x3c0);
    inb(0x3da);
    outb(0x02, 0x3c0);
    outb(0x02, 0x3c0);
    inb(0x3da);
    outb(0x03, 0x3c0);
    outb(0x03, 0x3c0);
    inb(0x3da);
    outb(0x04, 0x3c0);
    outb(0x04, 0x3c0);
    inb(0x3da);
    outb(0x05, 0x3c0);
    outb(0x05, 0x3c0);
    inb(0x3da);
    outb(0x06, 0x3c0);
    outb(0x14, 0x3c0);
    inb(0x3da);
    outb(0x07, 0x3c0);
    outb(0x07, 0x3c0);
    inb(0x3da);
    outb(0x08, 0x3c0);
    outb(0x38, 0x3c0);
    inb(0x3da);
    outb(0x09, 0x3c0);
    outb(0x39, 0x3c0);
    inb(0x3da);
    outb(0x0a, 0x3c0);
    outb(0x3a, 0x3c0);
    inb(0x3da);
    outb(0x0b, 0x3c0);
    outb(0x3b, 0x3c0);
    inb(0x3da);
    outb(0x0c, 0x3c0);
    outb(0x3c, 0x3c0);
    inb(0x3da);
    outb(0x0d, 0x3c0);
    outb(0x3d, 0x3c0);
    inb(0x3da);
    outb(0x0e, 0x3c0);
    outb(0x3e, 0x3c0);
    inb(0x3da);
    outb(0x0f, 0x3c0);
    outb(0x3f, 0x3c0);
    inb(0x3da);
    outb(0x10, 0x3c0);
    outb(0x0c, 0x3c0);
    inb(0x3da);
    outb(0x11, 0x3c0);
    outb(0x00, 0x3c0);
    inb(0x3da);
    outb(0x12, 0x3c0);
    outb(0x0f, 0x3c0);
    inb(0x3da);
    outb(0x13, 0x3c0);
    outb(0x08, 0x3c0);
    inb(0x3da);
    outb(0x14, 0x3c0);
    outb(0x00, 0x3c0);
    inb(0x3da);
    outb(0x20, 0x3c0);
}

void BootAssembler::assembleText()
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using namespace asmjit::x86::regs;

    Label L1, L2, lp_pd0, lp_pd1;
    Label lp_hlt;
    L1 = m_asm.newLabel();
    L2 = m_asm.newLabel();
    lp_pd0 = m_asm.newLabel();
    lp_pd1 = m_asm.newLabel();
    lp_hlt = m_asm.newLabel();

    m_asm.section(m_text);
    m_asm.bind(m_labels.entry);
    m_asm.cli();

    // Set up GDT
    m_asm.mov(rdi, 4*8-1);                     // limit: 4 entries in the GDT
    m_asm.mov(dword_ptr(m_labels.GDTP), edi);  // update limit
    m_asm.lea(rsi, ptr(m_labels.GDT));         // base : GDT
    m_asm.mov(ptr(m_labels.GDTP, 2), rsi);     // update base
    m_asm.lgdt(ptr(m_labels.GDTP));

    // Update CS
    m_asm.mov(rax, 0x10);
    m_asm.push(rax);
    m_asm.lea(rax, ptr(L1));
    m_asm.push(rax);
    m_asm.rex_w().retf();                      // retfq/lretq
    m_asm.bind(L1);

    // Load data segments
    m_asm.mov(eax, 0x18);
    m_asm.mov(ss, eax);
    m_asm.mov(ds, eax);
    m_asm.mov(es, eax);
    m_asm.mov(fs, eax);
    m_asm.mov(gs, eax);

    // Set up a stack
    m_asm.lea(rsp, ptr(m_labels.stackTop));

    /*
     * PAGING
     */

    // Level 4 (low mapping)
    m_asm.lea(rax, qword_ptr(m_labels.PDPT[0]));      // rax = &PDPT[0][0]
    m_asm.or_(rax, 3);                                // flags = RW|P
    m_asm.mov(qword_ptr(m_labels.PML4T), rax);        // PML4T[0] = rax

    // Level 3 (low mapping)
    m_asm.lea(rax, qword_ptr(m_labels.PDT[0]));       // rax = &PDT[0][0]
    m_asm.or_(rax, 3);                                // flags = RW|P
    m_asm.mov(qword_ptr(m_labels.PDPT[0], 8*0), rax); // PDPT[0][0] = rax
    m_asm.add(rax, 0x1000);                           // rax += 4096
    m_asm.mov(qword_ptr(m_labels.PDPT[0], 8*1), rax); // PDPT[0][1] = rax
    m_asm.add(rax, 0x1000);                           // rax += 4096
    m_asm.mov(qword_ptr(m_labels.PDPT[0], 8*2), rax); // PDPT[0][2] = rax
    m_asm.add(rax, 0x1000);                           // rax += 4096
    m_asm.mov(qword_ptr(m_labels.PDPT[0], 8*3), rax); // PDPT[0][3] = rax

    // Level 2 (low mapping)
    m_asm.xor_(rax, rax);                    // rax = 0
    m_asm.xor_(rcx, rcx);                    // loop counter
    m_asm.or_(rax, 0x83);                    // flags = PS|RW|P
    m_asm.bind(lp_pd0);                      // lp_pd:
    m_asm.lea(r11, ptr(m_labels.PDT[0]));    // r11 = &PDT[0][0]
    m_asm.mov(qword_ptr(r11, rcx, 3), rax);  // PDT[0][rcx * 8] = rax
    m_asm.add(rax, 0x200000);                // rax += 2 MiB
    m_asm.inc(ecx);                          // loop counter ++
    m_asm.cmp(ecx, 512 * 4);
    m_asm.jl(lp_pd0);                        // loop to lp_pd

    // Level 4 (high mapping)
    m_asm.lea(rax, qword_ptr(m_labels.PDPT[1]));      // rax = &PDPT[1][0]
    m_asm.or_(rax, 3);                                // flags = RW|P
    m_asm.mov(ptr(m_labels.PML4T, 8*511), rax);       // PML4T[511] = rax

    // Level 3 (high mapping)
    m_asm.lea(rax, qword_ptr(m_labels.PDT[1]));         // rax = &PDT[1][0]
    m_asm.or_(rax, 3);                                  // flags = RW|P
    m_asm.mov(qword_ptr(m_labels.PDPT[1], 8*510), rax); // PDPT[1][510] = rax
    m_asm.add(rax, 0x1000);                             // rax = &PDT[1][512]
    m_asm.mov(qword_ptr(m_labels.PDPT[1], 8*511), rax); // PDPT[1][511] = rax

    // Level 2 (high mapping) -- "zero page" compatibility ---> physical 0
    m_asm.mov(rax, 0);                                  // rax = 0
    m_asm.or_(rax, 0x83);                               // flags = PS|RW|P
    m_asm.mov(qword_ptr(m_labels.PDT[1]), rax);         // PDT[1][0] = rax

    // Level 2 (high mapping)
    m_asm.mov(rax, 0x200000);                // rax = 2 MiB
    m_asm.or_(rax, 0x83);                    // flags = PS|RW|P
    m_asm.mov(rcx, 1);                       // loop counter, start at 1
    m_asm.or_(rax, 0x83);                    // flags = PS|RW|P
    m_asm.bind(lp_pd1);                      // lp_pd:
    m_asm.lea(r12, ptr(m_labels.PDT[1]));    // r12 = &PDT[1][0]
    m_asm.mov(qword_ptr(r12, rcx, 3), rax);  // PDT[1][rcx * 8] = rax
    m_asm.add(rax, 0x200000);                // rax += 2 MiB
    m_asm.inc(ecx);                          // loop counter ++
    m_asm.cmp(ecx, 512 * 2);
    m_asm.jl(lp_pd1);                        // loop to lp_pd

    // CR3
    m_asm.lea(rax, qword_ptr(m_labels.PML4T));        // rax = &PML4T[0]
    m_asm.mov(cr3, rax);                              // cr3 = rax

    /*** BOOT ***/

    // Reset VGA Card
    m_asm.embed(m_gfxcode.data(), m_gfxcode.size());

    // Long-mode boot code:
    //      (*btext)(void)
    //
    // The stack looks like this upon entry:
    //     [rsp+0] = 32 bit return address (cannot be used)
    //     [rsp+4] = 32 bit modulep
    //     [rsp+8] = 32 bit kernend
    //
    m_asm.mov(eax, 0);
    m_asm.mov(ebx, m_modulep);
    m_asm.mov(ecx, m_kernend);
    m_asm.sub(rsp, 4);
    m_asm.mov(dword_ptr(rsp), ecx);
    m_asm.sub(rsp, 4);
    m_asm.mov(dword_ptr(rsp), ebx);
    m_asm.sub(rsp, 4);
    m_asm.mov(dword_ptr(rsp), eax);

    m_asm.lea(rax, qword_ptr(m_btext));
    m_asm.jmp(rax);

    // Halt the machine.
    m_asm.bind(lp_hlt);
    m_asm.hlt();
    m_asm.jmp(lp_hlt);
    m_asm.int3();
}

void BootAssembler::assembleData()
{
    using namespace asmjit;
    using namespace asmjit::x86;
    using namespace asmjit::x86::regs;

    m_asm.section(m_data);

    // GDT
    m_asm.align(AlignMode::kZero, 16);
    m_asm.bind(m_labels.GDT);
    m_asm.dq(0x0000000000000000ULL);    // NULL entry
    m_asm.dq(0x0000000000000000ULL);    // NULL entry
    m_asm.dq(0x00af9a000000ffffULL);    // Kernel code
    m_asm.dq(0x00cf92000000ffffULL);    // Kernel data

    // GDTPTR
    m_asm.align(AlignMode::kZero, 16);
    m_asm.bind(m_labels.GDTP);
    m_asm.dw(0);                    // For limit storage
    m_asm.dq(0);                    // For base storage

    /*
     * Memory for paging. Each table must be page aligned,
     * and is one page in size.
     */
    align(PAGE_SIZE);
    m_asm.bind(m_labels.PML4T);
    m_asm.db(0x00, PAGE_SIZE);
    m_asm.bind(m_labels.PDPT[0]); // low
    m_asm.db(0x00, PAGE_SIZE);
    m_asm.bind(m_labels.PDPT[1]); // high
    m_asm.db(0x00, PAGE_SIZE);
    m_asm.bind(m_labels.PDT[0]); // low
    m_asm.db(0x00, PAGE_SIZE);
    m_asm.db(0x00, PAGE_SIZE);
    m_asm.db(0x00, PAGE_SIZE);
    m_asm.db(0x00, PAGE_SIZE);
    m_asm.bind(m_labels.PDT[1]); // high
    m_asm.db(0x00, PAGE_SIZE);
    m_asm.db(0x00, PAGE_SIZE);
    /*
     * Because i'm using 2MiB pages, this is where paging
     * ends. There is no need for PTE's.
     */

    /* insert space for a stack */
    align(PAGE_SIZE);
    m_asm.db(0x00, STACK_SIZE);
    m_asm.bind(m_labels.stackTop);
}

void BootAssembler::debug()
{
    std::cout << std::format("=========================================\n");

    for (auto& s : m_code.sectionsByOrder()) {
        if (m_code.isSectionValid(s->id()) == false || s->realSize() == 0)
            continue;
        std::cout << std::format("segment {0}: [mem {1:08x}-{2:08x}] {3}\n",
                                 s->id(),
                                 s->offset(),
                                 s->offset() + s->realSize() - 1,
                                 s->name());
    }

    std::cout << std::format("=========================================\n");
    std::cout << std::format("btext       | {:016x}\n", m_btext);

    /* let's list all the named labels, and their section */
    for (auto& l : m_code.labelEntries()) {

        if (l->type() == asmjit::LabelType::kAnonymous || l->hasName() == false)
            continue;

        uintptr_t addr = l->isBound()? l->offset() + l->section()->offset() + m_code.baseAddress() : -1;
        std::string section = "<unbound>";
        auto name = l->hasName()? l->name() : "<anonymous>";
        auto sect = l->isBound()? l->section()->name() : "<unbound>";

        std::cout << std::format("{:12s}| {:016x} | {}\n", name, addr, sect);
    }
    std::cout << std::format("=========================================\n");
    std::cout << std::format("total = {:d} kbytes\n", (m_text->realSize() + m_data->realSize()) / 1024);
    std::cout << std::format("=========================================\n");
    std::flush(std::cout);
}

std::vector<char> BootAssembler::data()
{
    m_code.flatten();
    std::vector<char> buffer(m_code.codeSize());
    m_code.copyFlattenedData(buffer.data(), buffer.size(), asmjit::CopySectionFlags::kPadTargetBuffer);
    return buffer;
}

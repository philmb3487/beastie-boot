#include "ci915gfx.hxx"
#include "misc.hxx"
using namespace beastie;

#include <cassert>
#include <iostream>

beastie::CI915gfx::CI915gfx()
    : m_fb()
    , m_present(false)
{
    m_fb = beastie::fetchFB();
    m_present = (m_fb.id == "i915drmfb");
    assert((m_fb.width * m_fb.height * 4) == m_fb.size);
}

bool beastie::CI915gfx::isPresent()
{
    return m_present;
}

uintptr_t beastie::CI915gfx::base()
{
    return (m_fb.phys);
}

void beastie::CI915gfx::debug()
{
    std::cout << std::format("i915   base=0x{:x}\n", this->base());
}

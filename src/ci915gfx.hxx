#pragma once

#include "cgfx.hxx"
#include "misc.hxx"
using namespace beastie;

#include <vector>

namespace beastie {
class CI915gfx : public CGfx
{
public:
    CI915gfx();

    // Is there an i915 graphics card in this system?
    bool isPresent() override;

    // What's the framebuffer base address?
    uintptr_t base() override;

    void debug() override;

    std::vector<char> assembleReset(int, int) override {
        return {};
    }

private:
    fbinfo m_fb;

private:
    constexpr static int VENDOR_INTEL = 0x8086;
    constexpr static int DEVICE_RAPTOR = 0xA788;
    bool m_present;

};
} // namespace beastie

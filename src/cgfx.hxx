#pragma once

#include <cstdint>
#include <vector>

namespace beastie {
class CGfx
{
public:
    virtual ~CGfx() = default;

    // Is there a vmware graphics card in this system?
    virtual bool isPresent() = 0;

    // What's the framebuffer base address?
    virtual uintptr_t base() = 0;

    // Debug print information about this card
    virtual void debug() = 0;

    // Assemble reset code for this card
    virtual std::vector<char> assembleReset(int, int) = 0;

};
} // namespace beastie

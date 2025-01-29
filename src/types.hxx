#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include <asm/bootparam.h>

namespace beastie {

struct efifb {
    uint64_t    fb_addr;
    uint64_t    fb_size;
    uint32_t    fb_height;
    uint32_t    fb_width;
    uint32_t    fb_stride;
    uint32_t    fb_mask_red;
    uint32_t    fb_mask_green;
    uint32_t    fb_mask_blue;
    uint32_t    fb_mask_reserved;
};

struct fbinfo {
    std::string id;
    uintptr_t phys;
    size_t    size;
    uint32_t  width;
    uint32_t  height;
    uint32_t  mask_red;
    uint32_t  mask_green;
    uint32_t  mask_blue;
    uint32_t  mask_reserved;
    uint64_t  extra1;
};

struct smapentry {
    uint64_t addr;
    uint64_t size;
    uint32_t type;
} __attribute__((packed));

struct smapinfo {
    smapentry e820_table[128];
    uint8_t e820_entries;
};

struct efimapentry {
    uint32_t type;
    uint32_t pad;
    uint64_t phys;
    uint64_t virt;
    uint64_t pages;
    uint64_t attr;
} __attribute__((packed));

struct efimapinfo {
    uint64_t memory_size;
    uint64_t descriptor_size;
    uint32_t descriptor_version;
    uint32_t pad1;
    uint64_t pad2;
    efimapentry efi_table[128];
} __attribute__((packed));

struct efifbinfo {
    uint64_t addr;
    uint64_t size;
    uint32_t height;
    uint32_t width;
    uint32_t stride;
    uint32_t maskRed;
    uint32_t maskGreen;
    uint32_t maskBlue;
    uint32_t maskReserved;
} __attribute__((packed));

} // namespace beastie

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

enum vfnt_map_type {
    VFNT_MAP_NORMAL = 0,    /* Normal font. */
    VFNT_MAP_NORMAL_RIGHT,  /* Normal font right hand. */
    VFNT_MAP_BOLD,      /* Bold font. */
    VFNT_MAP_BOLD_RIGHT,    /* Bold font right hand. */
    VFNT_MAPS       /* Number of maps. */
};

struct font_header {
    uint8_t     fh_magic[8];
    uint8_t     fh_width;
    uint8_t     fh_height;
    uint16_t    fh_pad;
    uint32_t    fh_glyph_count;
    uint32_t    fh_map_count[VFNT_MAPS];
} __attribute__((packed));

struct font_info {
    int32_t fi_checksum;
    uint32_t fi_width;
    uint32_t fi_height;
    uint32_t fi_bitmap_size;
    uint32_t fi_map_count[VFNT_MAPS];
};

struct vfnt_map {
    uint32_t     vfm_src;
    uint16_t     vfm_dst;
    uint16_t     vfm_len;
} __attribute__((packed));
typedef struct vfnt_map vfnt_map_t;

} // namespace beastie

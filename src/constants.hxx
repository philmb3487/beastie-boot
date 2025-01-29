#pragma once

#include <string_view>

namespace beastie {
constexpr std::string_view progname = "beastie";
constexpr std::string_view progvers = "0.1";

constexpr int RB_AUTOBOOT = 0;       /* flags for system auto-booting itself */
constexpr int RB_ASKNAME  = 0x001;   /* force prompt of device of root filesystem */
constexpr int RB_SINGLE   = 0x002;   /* reboot to single user only */
constexpr int RB_NOSYNC   = 0x004;   /* dont sync before reboot */
constexpr int RB_HALT     = 0x008;   /* don't reboot, just halt */
constexpr int RB_INITNAME = 0x010;   /* Unused placeholder to specify init path */
constexpr int RB_DFLTROOT = 0x020;   /* use compiled-in rootdev */
constexpr int RB_KDB      = 0x040;   /* give control to kernel debugger */
constexpr int RB_RDONLY   = 0x080;   /* mount root fs read-only */
constexpr int RB_DUMP     = 0x100;   /* dump kernel memory before reboot */
constexpr int RB_MINIROOT = 0x200;   /* Unused placeholder */
constexpr int RB_VERBOSE  = 0x800;   /* print all potentially useful info */
constexpr int RB_SERIAL   = 0x1000;  /* use serial port as console */
constexpr int RB_CDROM    = 0x2000;  /* use cdrom as root */
constexpr int RB_POWEROFF = 0x4000;  /* turn the power off if possible */
constexpr int RB_GDB      = 0x8000;  /* use GDB remote debugger instead of DDB */
constexpr int RB_MUTE     = 0x10000; /* start up with the console muted */
constexpr int RB_SELFTEST = 0x20000; /* unused placeholder */
constexpr int RB_RESERVED1= 0x40000; /* reserved for internal use of boot blocks */
constexpr int RB_RESERVED2= 0x80000; /* reserved for internal use of boot blocks */
constexpr int RB_PAUSE    = 0x100000;    /* pause after each output line during probe */
constexpr int RB_REROOT   = 0x200000;    /* unmount the rootfs and mount it again */
constexpr int RB_POWERCYCLE= 0x400000;   /* Power cycle if possible */
constexpr int RB_MUTEMSGS = 0x800000;    /* start up with console muted after banner */
constexpr int RB_PROBE    = 0x10000000;  /* Probe multiple consoles */
constexpr int RB_MULTIPLE = 0x20000000;  /* use multiple consoles */
constexpr int RB_BOOTINFO = 0x80000000;  /* have `struct bootinfo *' arg */

constexpr int MODINFO_END  = 0x0000;
constexpr int MODINFO_NAME = 0x0001;
constexpr int MODINFO_TYPE = 0x0002;
constexpr int MODINFO_ADDR = 0x0003;
constexpr int MODINFO_SIZE = 0x0004;
constexpr int MODINFO_ARGS = 0x0006;
constexpr int MODINFO_METADATA = 0x8000;

constexpr int MODINFOMD_AOUTEXEC   = 0x0001;
constexpr int MODINFOMD_ELFHDR     = 0x0002;
constexpr int MODINFOMD_SSYM       = 0x0003;
constexpr int MODINFOMD_ESYM       = 0x0004;
constexpr int MODINFOMD_ENVP       = 0x0006;
constexpr int MODINFOMD_HOWTO      = 0x0007;
constexpr int MODINFOMD_KERNEND    = 0x0008;
constexpr int MODINFOMD_SHDR       = 0x0009;
constexpr int MODINFOMD_CTORS_ADDR = 0x000a;
constexpr int MODINFOMD_CTORS_SIZE = 0x000b;
constexpr int MODINFOMD_FW_HANDLE  = 0x000c;
constexpr int MODINFOMD_KEYBUF     = 0x000d;
constexpr int MODINFOMD_FONT       = 0x000e;
constexpr int MODINFOMD_SPLASH     = 0x000f;

constexpr int MODINFOMD_SMAP       = 0x1001;
constexpr int MODINFOMD_SMAP_XATTR = 0x1002;
constexpr int MODINFOMD_DTBP       = 0x1003;
constexpr int MODINFOMD_EFI_MAP    = 0x1004;
constexpr int MODINFOMD_EFI_FB     = 0x1005;
constexpr int MODINFOMD_MODULEP    = 0x1006;
constexpr int MODINFOMD_VBE_FB     = 0x1007;
constexpr int MODINFOMD_EFI_ARCH   = 0x1008;

constexpr int SMAP_TYPE_MEMORY       = 0x0001;
constexpr int SMAP_TYPE_RESERVED     = 0x0002;
constexpr int SMAP_TYPE_ACPI_RECLAIM = 0x0003;
constexpr int SMAP_TYPE_ACPI_NVS     = 0x0004;
constexpr int SMAP_TYPE_ACPI_ERROR   = 0x0005;
constexpr int SMAP_TYPE_DISABLED     = 0x0006;
constexpr int SMAP_TYPE_PMEM         = 0x0007;
constexpr int SMAP_TYPE_PRAM         = 0x000c;

constexpr int EFI_MD_TYPE_NULL       = 0;
constexpr int EFI_MD_TYPE_CODE       = 1;   /* Loader text. */
constexpr int EFI_MD_TYPE_DATA       = 2;   /* Loader data. */
constexpr int EFI_MD_TYPE_BS_CODE    = 3;   /* Boot services text. */
constexpr int EFI_MD_TYPE_BS_DATA    = 4;   /* Boot services data. */
constexpr int EFI_MD_TYPE_RT_CODE    = 5;   /* Runtime services text. */
constexpr int EFI_MD_TYPE_RT_DATA    = 6;   /* Runtime services data. */
constexpr int EFI_MD_TYPE_FREE       = 7;   /* Unused/free memory. */
constexpr int EFI_MD_TYPE_BAD        = 8;   /* Bad memory */
constexpr int EFI_MD_TYPE_RECLAIM    = 9;   /* ACPI reclaimable memory. */
constexpr int EFI_MD_TYPE_FIRMWARE   = 10;  /* ACPI NV memory */
constexpr int EFI_MD_TYPE_IOMEM      = 11;  /* Memory-mapped I/O. */
constexpr int EFI_MD_TYPE_IOPORT     = 12;  /* I/O port space. */
constexpr int EFI_MD_TYPE_PALCODE    = 13;  /* PAL */
constexpr int EFI_MD_TYPE_PERSISTENT = 14;  /* Persistent memory. */

} // namespace beastie

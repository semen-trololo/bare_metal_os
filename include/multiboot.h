#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

/* Multiboot magic number */
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

/* Multiboot info flags */
#define MULTIBOOT_INFO_MEMORY      0x00000001  // mem_lower, mem_upper
#define MULTIBOOT_INFO_BOOTDEV     0x00000002
#define MULTIBOOT_INFO_CMDLINE     0x00000004
#define MULTIBOOT_INFO_MODS        0x00000008
#define MULTIBOOT_INFO_AOUT_SYMS   0x00000010
#define MULTIBOOT_INFO_ELF_SHDR    0x00000020
#define MULTIBOOT_INFO_MEM_MAP     0x00000040  // mmap_* (E820)
#define MULTIBOOT_INFO_DRIVE_INFO  0x00000080
#define MULTIBOOT_INFO_CONFIG_TABLE 0x00000100
#define MULTIBOOT_INFO_BOOT_LOADER_NAME 0x00000200
#define MULTIBOOT_INFO_APM_TABLE   0x00000400
#define MULTIBOOT_INFO_VBE_INFO    0x00000800
#define MULTIBOOT_INFO_FRAMEBUFFER_INFO 0x00001000

/* Memory map entry types */
#define MULTIBOOT_MEMORY_AVAILABLE        1
#define MULTIBOOT_MEMORY_RESERVED         2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE 3
#define MULTIBOOT_MEMORY_NVS              4
#define MULTIBOOT_MEMORY_BADRAM           5

/* Multiboot info structure */
typedef struct {
    uint32_t flags;

    /* Valid if flags[0] set */
    uint32_t mem_lower;    // KB
    uint32_t mem_upper;    // KB

    /* Valid if flags[1] set */
    uint32_t boot_device;

    /* Valid if flags[2] set */
    uint32_t cmdline;

    /* Valid if flags[3] set */
    uint32_t mods_count;
    uint32_t mods_addr;

    /* Valid if flags[4] or flags[5] set (ELF section headers) */
    uint32_t shdr_num;
    uint32_t shdr_size;
    uint32_t shdr_addr;
    uint32_t shdr_strndx;

    /* Valid if flags[6] set - THIS IS WHAT WE NEED! */
    uint32_t mmap_length;  // Размер memory map в байтах
    uint32_t mmap_addr;    // Физический адрес массива mmap entries

    /* Valid if flags[7] set */
    uint32_t drives_length;
    uint32_t drives_addr;

    /* Valid if flags[8] set */
    uint32_t config_table;

    /* Valid if flags[9] set */
    uint32_t boot_loader_name;

    /* Valid if flags[10] set */
    uint32_t apm_table;

    /* Valid if flags[11] set (VBE info) */
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;

    /* Valid if flags[12] set (framebuffer) */
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
} __attribute__((packed)) multiboot_info_t;

/* Memory map entry (E820 record) */
typedef struct {
    uint32_t size;      // Размер оставшейся части структуры (НЕ включая это поле!)
    uint64_t addr;      // Базовый физический адрес региона
    uint64_t len;       // Длина региона в байтах
    uint32_t type;      // Тип региона (1=available, 2=reserved, etc.)
} __attribute__((packed)) multiboot_memory_map_t;

#endif // MULTIBOOT_H

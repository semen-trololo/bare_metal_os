#include "pmm.h"
#include "klib.h"
#include "multiboot.h"

// Битмап: 1 бит = 1 страница. 
static uint8_t pmm_bitmap[PMM_PAGES_COUNT / 8] __attribute__((aligned(4096)));

// Статистика
static uint32_t total_available_pages = 0;
static uint32_t used_pages = 0;

// Импорт символов из linker.ld
extern uint8_t _kernel_start[];
extern uint8_t _kernel_end[];

#define MAX_E820_ENTRIES 64
static e820_entry_t e820_map[MAX_E820_ENTRIES];
static uint32_t e820_count = 0;

static inline void bitmap_set(uint32_t bit) {
    pmm_bitmap[bit / 8] |= (1 << (bit % 8));
}

static inline void bitmap_clear(uint32_t bit) {
    pmm_bitmap[bit / 8] &= ~(1 << (bit % 8));
}

static inline uint8_t bitmap_test(uint32_t bit) {
    return pmm_bitmap[bit / 8] & (1 << (bit % 8));
}

// Освобождает диапазон физических адресов в битмапе
static void pmm_free_region(uint64_t base, uint64_t length) {
    uint64_t align_base = (base + PMM_PAGE_SIZE - 1) & ~(uint64_t)(PMM_PAGE_SIZE - 1);
    uint64_t align_end = (base + length) & ~(uint64_t)(PMM_PAGE_SIZE - 1);

    if (align_base >= align_end) return;

    if (align_base >= PMM_MAX_MEMORY_SIZE) return;
    if (align_end > PMM_MAX_MEMORY_SIZE) {
        align_end = PMM_MAX_MEMORY_SIZE;
    }

    uint32_t start_page = (uint32_t)(align_base / PMM_PAGE_SIZE);
    uint32_t end_page = (uint32_t)(align_end / PMM_PAGE_SIZE);

    for (uint32_t i = start_page; i < end_page; i++) {
        if (bitmap_test(i)) { 
            bitmap_clear(i);
            total_available_pages++;
        }
    }
}

// Резервирует (заносит в битмап) диапазон страниц
static void pmm_reserve_region(uintptr_t base, uintptr_t end) {
    uint32_t start_page = base / PMM_PAGE_SIZE;
    uint32_t end_page = (end + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;

    for (uint32_t i = start_page; i < end_page; i++) {
        if (i < PMM_PAGES_COUNT && !bitmap_test(i)) {
            bitmap_set(i);
            total_available_pages--;
        }
    }
}

void pmm_init(multiboot_info_t* info) {
    e820_count = 0;
    // 1. SAFE BY DEFAULT: Помечаем ВСЕ страницы как ЗАНЯТЫЕ.
    for (uint32_t i = 0; i < sizeof(pmm_bitmap); i++) {
        pmm_bitmap[i] = 0xFF;
    }
    total_available_pages = 0;
    used_pages = 0;

    // 2. Парсим Multiboot Memory Map (E820) и ОСВОБОЖДАЕМ доступные регионы
    if (info && (info->flags & MULTIBOOT_INFO_MEM_MAP)) {
        multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)(uintptr_t)info->mmap_addr;
        uint32_t mmap_end = info->mmap_addr + info->mmap_length;
        
        while ((uintptr_t)mmap < mmap_end) {
            if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
                pmm_free_region(mmap->addr, mmap->len);
            }
            // Копируем запись в безопасный массив ядра
            if (e820_count < MAX_E820_ENTRIES) {
            e820_map[e820_count].addr = mmap->addr;
            e820_map[e820_count].len = mmap->len;
            e820_map[e820_count].type = mmap->type;
            e820_count++;
}
            mmap = (multiboot_memory_map_t*)((uintptr_t)mmap + mmap->size + sizeof(uint32_t));
        }
    } else {
        k_printf("[PMM] WARNING: E820 not available. Using mem_upper fallback.\n");
        if (info && info->mem_upper > 0) {
            uint64_t upper_mem_bytes = (uint64_t)info->mem_upper * 1024;
            pmm_free_region(0x100000, upper_mem_bytes); 
        }
    }

    // 3. PUNCHING HOLES: Резервируем служебные области ПОСЛЕ парсинга E820!
    
    // А) Резервируем нижний 1 МБ (Real Mode IVT, BIOS, VGA)
    pmm_reserve_region(0x00000000, 0x00100000);

    // Б) Резервируем образ ядра (включая стек и статический битмап PMM)
    uintptr_t k_start = (uintptr_t)_kernel_start;
    uintptr_t k_end = (uintptr_t)_kernel_end;
    pmm_reserve_region(k_start, k_end);

    k_printf("[PMM] Kernel image reserved (0x%x - 0x%x)\n", k_start, k_end);
    k_printf("[PMM] Initialized. Available: %u pages (%u MB).\n", 
             total_available_pages, (total_available_pages * PMM_PAGE_SIZE) / (1024 * 1024));
}

uint32_t pmm_alloc_page(void) {
    for (uint32_t i = 0; i < PMM_PAGES_COUNT; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            used_pages++;
            return i * PMM_PAGE_SIZE;
        }
    }
    return 0; 
}

void pmm_free_page(uint32_t phys_addr) {
    if (phys_addr % PMM_PAGE_SIZE != 0) return;
    uint32_t page_index = phys_addr / PMM_PAGE_SIZE;
    if (page_index >= PMM_PAGES_COUNT) return;
    if (!bitmap_test(page_index)) return; 
    
    bitmap_clear(page_index);
    used_pages--;
}

uint32_t pmm_get_used_pages(void) { return used_pages; }
uint32_t pmm_get_free_pages(void) { return total_available_pages - used_pages; }
uint32_t pmm_get_total_pages(void) { return total_available_pages; }
const e820_entry_t* pmm_get_memory_map(uint32_t* count) {
    if (count) *count = e820_count;
    return e820_map;
}
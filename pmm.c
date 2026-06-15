#include "pmm.h"
#include "klib.h" // Для k_printf

// Битмап: 1 бит = 1 страница. 
// 32768 страниц / 8 бит в байте = 4096 байт (ровно одна физическая страница!)
static uint8_t pmm_bitmap[PMM_PAGES_COUNT / 8] __attribute__((aligned(4096)));

// Вспомогательные функции для работы с битами
static inline void bitmap_set(uint32_t bit) {
    pmm_bitmap[bit / 8] |= (1 << (bit % 8));
}

static inline void bitmap_clear(uint32_t bit) {
    pmm_bitmap[bit / 8] &= ~(1 << (bit % 8));
}

static inline uint8_t bitmap_test(uint32_t bit) {
    return pmm_bitmap[bit / 8] & (1 << (bit % 8));
}

void pmm_init(void) {
    // 1. Очищаем битмап (все страницы свободны)
    for (uint32_t i = 0; i < sizeof(pmm_bitmap); i++) {
        pmm_bitmap[i] = 0;
    }

    // 2. Резервируем первые 2 МБ памяти (заняты BIOS, VGA, ядром, стеком, битмапом)
    // Это критически важно, чтобы pmm_alloc_page() не выдал нам память, 
    // где уже лежит код ядра!
    uint32_t reserved_pages = (2 * 1024 * 1024) / PMM_PAGE_SIZE; // 512 страниц
    for (uint32_t i = 0; i < reserved_pages; i++) {
        bitmap_set(i);
    }

    k_printf("[PMM] Initialized. Tracking %d MB of RAM.\n", PMM_MEMORY_SIZE / (1024 * 1024));
}

uint32_t pmm_alloc_page(void) {
    for (uint32_t i = 0; i < PMM_PAGES_COUNT; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            return i * PMM_PAGE_SIZE;
        }
    }
    // Убрали k_printf отсюда! PMM должен быть независим от UI.
    return 0; 
}

void pmm_free_page(uint32_t phys_addr) {
    if (phys_addr % PMM_PAGE_SIZE != 0) {
        k_printf("[PMM] ERROR: Unaligned address 0x%x\n", phys_addr);
        return;
    }
    uint32_t page_index = phys_addr / PMM_PAGE_SIZE;
    if (page_index >= PMM_PAGES_COUNT) {
        k_printf("[PMM] ERROR: Address 0x%x out of range\n", phys_addr);
        return;
    }
    if (!bitmap_test(page_index)) {
        k_printf("[PMM] WARNING: Freeing already free page 0x%x\n", phys_addr);
        return;
    }
    bitmap_clear(page_index);
}

uint32_t pmm_get_used_pages(void) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < PMM_PAGES_COUNT; i++) {
        if (bitmap_test(i)) count++;
    }
    return count;
}

uint32_t pmm_get_free_pages(void) {
    return PMM_PAGES_COUNT - pmm_get_used_pages();
}
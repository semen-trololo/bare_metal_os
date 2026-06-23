#ifndef PMM_H
#define PMM_H

#include <stdint.h>

#define PMM_MEMORY_SIZE (512 * 1024 * 1024) // 512 MB
#define PMM_PAGE_SIZE   4096
#define PMM_PAGES_COUNT (PMM_MEMORY_SIZE / PMM_PAGE_SIZE) // 131072 страницы

void pmm_init(void);
uint32_t pmm_alloc_page(void);
void pmm_free_page(uint32_t phys_addr);

// Новые функции для статистики
uint32_t pmm_get_used_pages(void);
uint32_t pmm_get_free_pages(void);

#endif // PMM_H
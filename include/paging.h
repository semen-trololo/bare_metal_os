#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

// Флаги для записей Page Directory и Page Table
#define PAGE_PRESENT  (1 << 0)
#define PAGE_WRITE    (1 << 1)
#define PAGE_USER     (1 << 2)

// Инициализация таблиц страниц (Direct Map 512 MB) и включение MMU
void paging_init(void);

// Динамическое маппинг виртуального адреса на физический
// virt: виртуальный адрес (должен быть кратен 4096)
// phys: физический адрес (должен быть кратен 4096)
// flags: комбинация флагов (PAGE_PRESENT | PAGE_WRITE и т.д.)
void vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags);

#endif // PAGING_H
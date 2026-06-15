#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

// Флаги для записей Page Directory и Page Table
#define PAGE_PRESENT  (1 << 0)
#define PAGE_WRITE    (1 << 1)
#define PAGE_USER     (1 << 2)

// Инициализация таблиц страниц и включение MMU
void paging_init(void);

#endif // PAGING_H

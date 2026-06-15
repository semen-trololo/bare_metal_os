#include "paging.h"
#include "pmm.h"
#include "klib.h"

// Статические таблицы для Identity Mapping (Bootstrap)
// Выровнены по границе 4 КБ, как требует архитектура x86.
static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t page_tables[4][1024] __attribute__((aligned(4096))); // 4 таблицы * 4 МБ = 16 МБ

// ASM-функция для загрузки CR3 и включения пейджинга
static inline void load_page_directory(uint32_t pd_phys_addr) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(pd_phys_addr));
}

static inline void enable_paging(void) {
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1 << 31); // Устанавливаем бит PG (Paging)
    cr0 |= (1 << 16); // Устанавливаем бит WP (Write Protect) - защита от записи в RO страницы
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}

void paging_init(void) {
    // 1. Очищаем Page Directory
    for (int i = 0; i < 1024; i++) {
        page_directory[i] = 0; // Present = 0
    }

    // 2. Настраиваем Identity Mapping для первых 16 МБ
    // Нам нужно 4 Page Tables, так как одна таблица покрывает 4 МБ (1024 * 4 КБ)
    for (int i = 0; i < 4; i++) {
        // Физический адрес i-й Page Table
        // Так как массивы статические, мы берем их виртуальный адрес.
        // В режиме Flat Memory (до включения Paging) виртуальный = физическому.
        uint32_t pt_phys_addr = (uint32_t)&page_tables[i];
        
        // Записываем адрес таблицы в Page Directory + флаги
        page_directory[i] = pt_phys_addr | PAGE_PRESENT | PAGE_WRITE;

        // 3. Заполняем саму Page Table
        for (int j = 0; j < 1024; j++) {
            // Физический адрес страницы: (i * 4 МБ) + (j * 4 КБ)
            uint32_t page_phys_addr = (i * 1024 * 4096) + (j * 4096);
            page_tables[i][j] = page_phys_addr | PAGE_PRESENT | PAGE_WRITE;
        }
    }

    k_printf("[Paging] Identity mapping configured for first 16 MB.\n");

    // 4. Загружаем адрес Page Directory в регистр CR3
    // В Flat Memory виртуальный адрес page_directory равен его физическому адресу.
    load_page_directory((uint32_t)&page_directory);

    // 5. Включаем MMU (Прыжок веры!)
    // Если таблицы заполнены неверно, здесь произойдет Triple Fault и ребут.
    enable_paging();

    k_printf("[Paging] MMU enabled successfully. Welcome to Virtual Memory!\n");
}

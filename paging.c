#include "paging.h"
#include "pmm.h"
#include "klib.h"

// Статические таблицы для Direct Map (Bootstrap)
// 128 таблиц * 4 МБ = 512 МБ покрытого пространства.
// Занимают 512 КБ в секции .bss. Выровнены по границе 4 КБ.
static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t page_tables[128][1024] __attribute__((aligned(4096))); 

// ASM-функция для загрузки CR3
static inline void load_page_directory(uint32_t pd_phys_addr) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(pd_phys_addr));
}

// ASM-функция для включения пейджинга
static inline void enable_paging(void) {
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1 << 31); // Устанавливаем бит PG (Paging)
    cr0 |= (1 << 16); // Устанавливаем бит WP (Write Protect)
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}

void paging_init(void) {
    // 1. Очищаем Page Directory
    for (int i = 0; i < 1024; i++) {
        page_directory[i] = 0; 
    }

    // 2. Настраиваем Direct Map для первых 512 МБ
    for (int i = 0; i < 128; i++) {
        // Физический адрес i-й Page Table
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

    k_printf("[Paging] Direct mapping configured for first 512 MB.\n");

    // 4. Загружаем адрес Page Directory в регистр CR3
    load_page_directory((uint32_t)&page_directory);

    // 5. Включаем MMU (Прыжок веры!)
    enable_paging();

    k_printf("[Paging] MMU enabled successfully. Welcome to Virtual Memory!\n");
}

void vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    // Извлекаем индексы из виртуального адреса
    uint32_t dir_index = virt >> 22;
    uint32_t table_index = (virt >> 12) & 0x3FF;

    // 1. Если Page Table для этого куска памяти еще не создана
    if (!(page_directory[dir_index] & PAGE_PRESENT)) {
        // Выделяем физическую страницу под саму Page Table
        uint32_t new_pt_phys = pmm_alloc_page();
        if (new_pt_phys == 0) {
            k_printf("[VMM] FATAL: Out of memory for Page Table!\n");
            return; // В реальной ОС здесь был бы kernel panic
        }
        
        // МАГИЯ DIRECT MAP: 
        // new_pt_phys - это физический адрес (например, 0x00200000).
        // Так как первые 512 МБ замаплены как Virt == Phys, мы можем 
        // просто скастовать его к указателю и очистить через k_memset!
        k_memset((void*)new_pt_phys, 0, 4096); 
        
        // Прописываем адрес новой таблицы в Directory
        page_directory[dir_index] = new_pt_phys | PAGE_PRESENT | PAGE_WRITE;
    }

    // 2. Достаем физический адрес нужной Page Table
    // Маска 0xFFFFF000 убирает младшие биты (флаги), оставляя только адрес
    uint32_t pt_phys = page_directory[dir_index] & 0xFFFFF000; 
    
    // 3. Записываем связь (PTE - Page Table Entry)
    // Снова спасает Direct Map: мы обращаемся к таблице по её физ. адресу
    uint32_t* pt = (uint32_t*)pt_phys; 
    pt[table_index] = phys | flags;

    // 4. Говорим MMU сбросить кэш (TLB) для этого виртуального адреса
    // Без этого процессор может использовать старую, неактуальную запись из кэша
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}
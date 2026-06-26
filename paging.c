#include "paging.h"
#include "pmm.h"
#include "klib.h"
#include "framebuffer.h" // <-- ДОБАВЛЕНО для доступа к fb_params

// Статические таблицы для Direct Map (Bootstrap)
// 128 таблиц * 4 МБ = 512 МБ покрытого пространства.
// Занимают 512 КБ в секции .bss. Выровнены по границе 4 КБ.
static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t page_tables[128][1024] __attribute__((aligned(4096))); 

// Внешняя структура из boot.asm
extern framebuffer_info_t fb_params;

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

    // ========================================================================
    // 4. КРИТИЧЕСКИ ВАЖНО: Маппинг Framebuffer ДО включения Paging!
    // ========================================================================
    if (fb_params.address != 0) {
        uint32_t fb_phys = fb_params.address;
        uint32_t fb_size = fb_params.width * fb_params.height * 4;
        uint32_t pages = (fb_size + 4095) / 4096;
        
        k_printf("[Paging] Mapping framebuffer @ 0x%x (%u pages)...\n", fb_phys, pages);
        
        for (uint32_t i = 0; i < pages; i++) {
            uint32_t addr = fb_phys + i * 4096;
            uint32_t dir_index = addr >> 22;
            uint32_t table_index = (addr >> 12) & 0x3FF;
            
            // Если Page Table для этого 4 МБ блока еще не создана
            if (!(page_directory[dir_index] & PAGE_PRESENT)) {
                uint32_t new_pt_phys = pmm_alloc_page();
                if (new_pt_phys == 0) {
                    k_printf("[Paging] FATAL: OOM for Framebuffer PT!\n");
                    while(1) __asm__ volatile("hlt");
                }
                
                // Очищаем новую Page Table
                uint32_t* new_pt = (uint32_t*)new_pt_phys;
                for (int j = 0; j < 1024; j++) {
                    new_pt[j] = 0;
                }
                
                // Записываем PDE
                page_directory[dir_index] = new_pt_phys | PAGE_PRESENT | PAGE_WRITE;
            }
            
            // Записываем PTE для фреймбуфера
            uint32_t pt_phys = page_directory[dir_index] & 0xFFFFF000;
            uint32_t* pt = (uint32_t*)pt_phys;
            pt[table_index] = addr | PAGE_PRESENT | PAGE_WRITE;
        }
        
        k_printf("[Paging] Framebuffer mapped successfully.\n");
    }

    // 5. Загружаем адрес Page Directory в регистр CR3
    load_page_directory((uint32_t)&page_directory);

    // 6. Включаем MMU (Прыжок веры!)
    enable_paging();

    k_printf("[Paging] MMU enabled successfully. Welcome to Virtual Memory!\n");
}

void vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t dir_index = virt >> 22;
    uint32_t table_index = (virt >> 12) & 0x3FF;

    // 1. Если Page Table для этого куска памяти еще не создана
    if (!(page_directory[dir_index] & PAGE_PRESENT)) {
        uint32_t new_pt_phys = pmm_alloc_page();
        if (new_pt_phys == 0) {
            k_printf("[VMM] FATAL: Out of memory for Page Table!\n");
            return;
        }
        
        k_memset((void*)new_pt_phys, 0, 4096); 
        page_directory[dir_index] = new_pt_phys | PAGE_PRESENT | PAGE_WRITE;
        
        // КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ:
        // После изменения PDE нужно инвалидировать TLB для всего 4 MB диапазона.
        // Проще всего - перезагрузить CR3 (сбросит весь TLB, но это безопасно).
        uint32_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
        __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));
    }

    uint32_t pt_phys = page_directory[dir_index] & 0xFFFFF000; 
    uint32_t* pt = (uint32_t*)pt_phys; 
    pt[table_index] = phys | flags;

    // Инвалидируем TLB для этого конкретного виртуального адреса
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}
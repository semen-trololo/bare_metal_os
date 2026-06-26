#include "gdt.h"
#include <stddef.h>

// Количество дескрипторов в нашей GDT
#define GDT_ENTRIES 6

// Массив дескрипторов (статический, живёт в секции .data)
static struct gdt_entry gdt[GDT_ENTRIES];

// Указатель на GDT (передаётся процессору через lgdt)
static struct gdt_ptr gp;

// Внешняя ассемблерная функция (реализована ниже)
// Загружает GDT и обновляет сегментные регистры
extern void gdt_flush(uint32_t gdt_ptr);

// Вспомогательная функция для заполнения одного дескриптора
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    // Заполняем базу (32 бита разбиты на 3 части)
    gdt[num].base_low    = (base & 0xFFFF);          // Биты 0-15
    gdt[num].base_middle = (base >> 16) & 0xFF;      // Биты 16-23
    gdt[num].base_high   = (base >> 24) & 0xFF;      // Биты 24-31

    // Заполняем лимит (20 бит разбиты на 2 части)
    gdt[num].limit_low   = (limit & 0xFFFF);         // Биты 0-15
    gdt[num].granularity = (limit >> 16) & 0x0F;     // Биты 16-19 (младшие 4 бита)
    
    // Заполняем флаги и байт доступа
    gdt[num].granularity |= (gran & 0xF0);           // Флаги в старшие 4 бита
    gdt[num].access      = access;
}

void gdt_install(void) {
    // Настраиваем указатель на GDT
    gp.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gp.base  = (uint32_t)&gdt;

    // ========================================
    // Дескриптор 0: NULL (обязателен!)
    // ========================================
    // Процессор требует, чтобы первый дескриптор был нулевым.
    // При попытке загрузить селектор 0 в сегментный регистр произойдёт #GP.
    gdt_set_gate(0, 0, 0, 0, 0);

    // ========================================
    // Дескриптор 1: Kernel Code Segment (селектор 0x08)
    // ========================================
    // Base: 0 (плоская модель)
    // Limit: 0xFFFFFFFF (4 ГБ)
    // Access: 0x9A
    //   - Present = 1
    //   - DPL = 0 (только kernel)
    //   - S = 1 (Code/Data)
    //   - Executable = 1 (Code segment)
    //   - Conforming = 0
    //   - Readable = 1 (можно читать код как данные)
    //   - Accessed = 0
    // Granularity: 0xCF
    //   - G = 1 (Limit в 4KB страницах)
    //   - D = 1 (32-bit protected mode)
    //   - L = 0 (не long mode)
    //   - Reserved = 0
    //   - Limit high = 0xF (старшие 4 бита лимита)
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    // ========================================
    // Дескриптор 2: Kernel Data Segment (селектор 0x10)
    // ========================================
    // Base: 0 (плоская модель)
    // Limit: 0xFFFFFFFF (4 ГБ)
    // Access: 0x92
    //   - Present = 1
    //   - DPL = 0 (только kernel)
    //   - S = 1 (Code/Data)
    //   - Executable = 0 (Data segment)
    //   - Direction = 0 (растёт вверх)
    //   - Writable = 1 (можно писать)
    //   - Accessed = 0
    // Granularity: 0xCF (аналогично Code segment)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    // ========================================
    // Дескриптор 3: User Code Segment (селектор 0x18)
    // ========================================
    // Access: 0xFA (Present=1, DPL=3, S=1, Type=1010b - Exec/Read)
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    // ========================================
    // Дескриптор 4: User Data Segment (селектор 0x20)
    // ========================================
    // Access: 0xF2 (Present=1, DPL=3, S=1, Type=0010b - Data/Write)
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    // Загружаем GDT в процессор и обновляем сегментные регистры
    gdt_flush((uint32_t)&gp);
}

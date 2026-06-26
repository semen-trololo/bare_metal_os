#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// Структура одного дескриптора GDT (ровно 8 байт)
// __attribute__((packed)) запрещает компилятору добавлять выравнивание
struct gdt_entry {
    uint16_t limit_low;    // Младшие 16 бит лимита
    uint16_t base_low;     // Младшие 16 бит базы
    uint8_t  base_middle;  // Следующие 8 бит базы
    uint8_t  access;       // Байт доступа (тип, DPL, Present)
    uint8_t  granularity;  // Granularity + старшие 4 бита лимита
    uint8_t  base_high;    // Старшие 8 бит базы
} __attribute__((packed));

// Структура указателя на GDT (для инструкции lgdt)
// Процессор ожидает именно такой формат: 2 байта размер + 4 байта адрес
struct gdt_ptr {
    uint16_t limit;        // Размер таблицы в байтах минус 1
    uint32_t base;         // Линейный адрес первого дескриптора
} __attribute__((packed));

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
// Функция инициализации GDT
void gdt_install(void);

#endif

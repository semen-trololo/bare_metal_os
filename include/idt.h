#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// Структура одного дескриптора IDT (ровно 8 байт)
struct idt_entry {
    uint16_t base_low;     // Младшие 16 бит адреса обработчика
    uint16_t sel;          // Селектор сегмента кода (0x08)
    uint8_t  always0;      // Всегда 0 (reserved)
    uint8_t  flags;        // Тип gate + DPL + Present
    uint16_t base_high;    // Старшие 16 бит адреса обработчика
} __attribute__((packed));

// Структура указателя на IDT (для инструкции lidt)
struct idt_ptr {
    uint16_t limit;        // Размер таблицы - 1
    uint32_t base;         // Адрес первого дескриптора
} __attribute__((packed));

// Структура регистров, сохраняемых перед вызовом C-обработчика
// Этот layout должен точно соответствовать тому, что пушит ASM-код
struct regs {
    // Сегментные регистры (пушим сами)
    uint32_t gs, fs, es, ds;
    
    // pusha пушит 8 регистров в таком порядке
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    
    // Пушим сами: номер прерывания и код ошибки
    uint32_t int_no, err_code;
    
    // CPU автоматически пушит при прерывании
    uint32_t eip, cs, eflags, useresp, ss;
};

// Инициализация IDT
void idt_install(void);

// Установка одного дескриптора в IDT
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

// Тип функции-обработчика
typedef void (*isr_handler_t)(struct regs* r);

// Регистрация C-обработчиков для ISR и IRQ
void isr_register_handler(uint8_t n, isr_handler_t handler);
void irq_register_handler(uint8_t n, isr_handler_t handler);

#endif

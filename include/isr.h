#ifndef ISR_H
#define ISR_H

#include "idt.h"

// Устанавливает все ISR и IRQ в таблицу IDT
void isr_install(void);
void irq_install(void);

// Включить/выключить прерывания процессора
// cli (Clear Interrupt Flag) — запрещает прерывания
// sti (Set Interrupt Flag) — разрешает прерывания
static inline void cli(void) { __asm__ volatile("cli"); }
static inline void sti(void) { __asm__ volatile("sti"); }

#endif

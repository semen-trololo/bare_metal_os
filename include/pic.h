#ifndef PIC_H
#define PIC_H

#include <stdint.h>

// Перенастройка PIC: сдвиг IRQ 0-15 на INT 32-47
void pic_remap(void);

//Новые функции для управления доступом IRQ
void irq_set_mask(uint8_t IRQline);   // Запретить прерывание
void irq_clear_mask(uint8_t IRQline); // Разрешить прерывание

#endif

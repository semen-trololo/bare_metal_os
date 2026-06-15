#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

// Инициализация PIT с заданной частотой (Гц)
void timer_init(uint32_t frequency);

// Получить количество тиков с момента инициализации
uint32_t timer_get_ticks(void);

// Получить текущую частоту таймера
uint32_t timer_get_frequency(void);

// Блокирующая задержка в миллисекундах
void k_sleep(uint32_t ms);

#endif

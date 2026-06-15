#include "timer.h"
#include "port_io.h"
#include "isr.h"
#include "klib.h"
#include "pic.h"

#define PIT_BASE_FREQ 1193182
#define PIT_CHANNEL0  0x40
#define PIT_COMMAND   0x43

// volatile критичен: переменная меняется в контексте IRQ0,
// а читается в основном потоке ядра. Без volatile компилятор
// может закэшировать значение в регистре и цикл sleep станет бесконечным.
static volatile uint32_t tick_count = 0;
static uint32_t pit_frequency = 0;

// Обработчик IRQ0 (INT 32 после remap)
static void pit_handler(struct regs* r) {
    (void)r; // Подавляем warning, регистры нам здесь не нужны
    tick_count++;
}

void timer_init(uint32_t freq) {
    pit_frequency = freq;
    uint32_t divisor = PIT_BASE_FREQ / freq;

    // Командный байт: 0x36
    // 00 - Канал 0
    // 11 - Доступ: сначала младший байт, затем старший
    // 011 - Режим 3 (Square Wave Generator)
    // 0 - Двоичный счетчик
    outb(PIT_COMMAND, 0x36);

    // Отправляем делитель: сначала Low, затем High
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

    // Регистрируем обработчик и разрешаем IRQ0
    irq_register_handler(0, pit_handler);
    irq_clear_mask(0);

    k_printf("[PIT] Timer initialized at %d Hz (divisor: %d)\n", freq, divisor);
}

uint32_t timer_get_ticks(void) {
    return tick_count;
}

uint32_t timer_get_frequency(void) {
    return pit_frequency;
}

void k_sleep(uint32_t ms) {
    uint32_t start = tick_count;
    uint32_t target = start + ms;  // 1 тик = 1 мс, просто складываем!
    
    while (tick_count < target) {
        __asm__ volatile("hlt");
    }
}


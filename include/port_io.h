#ifndef PORT_IO_H
#define PORT_IO_H

#include <stdint.h>

// Inline функции для записи в порты (out) и чтения из портов (in)
// volatile нужен, чтобы компилятор не удалил эти инструкции при оптимизации
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

#endif

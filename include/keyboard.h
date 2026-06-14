#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

// Инициализация и регистрация обработчика IRQ1
void keyboard_install(void);

// Читает один символ из кольцевого буфера.
// Возвращает символ, или 0, если буфер пуст.
// Не блокирует выполнение (non-blocking).
char k_getchar(void);

#endif
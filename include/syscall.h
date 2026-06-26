#ifndef SYSCALL_H
#define SYSCALL_H

#include "idt.h"

// Номера системных вызовов (Linux x86 ABI)
#define SYS_EXIT    1
#define SYS_FORK    2
#define SYS_READ    3
#define SYS_WRITE   4
#define SYS_YIELD   24

// Инициализация таблицы системных вызовов и регистрация в IDT
void syscall_init(void);

#endif

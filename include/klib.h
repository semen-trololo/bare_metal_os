#ifndef KLIB_H
#define KLIB_H

#include <stdint.h>
#include <stddef.h>

// Существующие функции
size_t k_strlen(const char* str);
void k_print(const char* str);
int k_strcmp(const char* s1, const char* s2);
int k_strncmp(const char* s1, const char* s2, size_t n);

// === НОВЫЕ ФУНКЦИИ ДЛЯ ЭТАПА 1 ===
void k_itoa(int value, char* buf, int base);
int k_atoi(const char* str);
void k_uitoa(unsigned int value, char* buf, int base);
void k_printf(const char* fmt, ...);

#endif
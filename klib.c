#include "klib.h"
#include "vga.h" // Нужен только для вызова vga_putc

size_t k_strlen(const char* str) {
    size_t len = 0;
    while(str[len]) len++;
    return len;
}

void k_print(const char* str) {
    while(*str) {
        vga_putc(*str); // Делегируем работу драйверу
        str++;
    }
}

int k_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    // Возвращаем разницу первых отличающихся символов
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}
#include "klib.h"
#include "vga.h"
#include <stdarg.h>

size_t k_strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

void k_print(const char* str) {
    while (*str) {
        vga_putc(*str);
        str++;
    }
}

int k_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int k_strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// === РЕАЛИЗАЦИЯ НОВЫХ ФУНКЦИЙ ===

// === РЕАЛИЗАЦИЯ k_atoi ===
int k_atoi(const char* str) {
    int result = 0;
    int sign = 1;
    
    // Пропускаем ведущие пробелы
    while (*str == ' ') str++;
    
    // Обрабатываем знак
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    // Парсим цифры
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result * sign;
}

void k_itoa(int value, char* buf, int base) {
    char tmp[33];
    int i = 0;
    int negative = 0;
    
    // Обработка нуля
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    
    // Обработка отрицательных чисел (только для base 10)
    if (value < 0 && base == 10) {
        negative = 1;
        value = -value;
    }
    
    unsigned int uvalue = (unsigned int)value;
    
    // Извлекаем цифры в обратном порядке
    while (uvalue > 0) {
        int rem = uvalue % base;
        tmp[i++] = (rem < 10) ? (rem + '0') : (rem - 10 + 'a');
        uvalue /= base;
    }
    
    // Формируем итоговую строку
    int j = 0;
    if (negative) {
        buf[j++] = '-';
    }
    
    // Разворачиваем цифры
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    
    buf[j] = '\0';
}

void k_uitoa(unsigned int value, char* buf, int base) {
    char tmp[33];
    int i = 0;
    
    // Обработка нуля
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    
    // Извлекаем цифры в обратном порядке
    while (value > 0) {
        int rem = value % base;
        tmp[i++] = (rem < 10) ? (rem + '0') : (rem - 10 + 'a');
        value /= base;
    }
    
    // Разворачиваем цифры
    int j = 0;
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    
    buf[j] = '\0';
}

void k_printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'd': {
                    int val = va_arg(args, int);
                    char buf[16];
                    k_itoa(val, buf, 10);
                    k_print(buf);
                    break;
                }
                case 'x': {
                    unsigned int val = va_arg(args, unsigned int);
                    k_print("0x");
                    char buf[16];
                    k_uitoa(val, buf, 16);
                    k_print(buf);
                    break;
                }
                case 's': {
                    const char* str = va_arg(args, const char*);
                    k_print(str ? str : "(null)");
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    vga_putc(c);
                    break;
                }
                case '%':
                    vga_putc('%');
                    break;
                case '\0':
                    goto end;
                default:
                    // Неизвестный спецификатор - выводим как есть
                    vga_putc('%');
                    vga_putc(*fmt);
                    break;
            }
        } else {
            vga_putc(*fmt);
        }
        fmt++;
    }
    
end:
    va_end(args);
}
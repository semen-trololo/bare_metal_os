#include "klib.h"
#include "vga.h"
#include <stdarg.h>

// ==========================================
// РАБОТА С ПАМЯТЬЮ (Нужно для VMM и Heap)
// ==========================================

void* k_memset(void* ptr, int value, size_t num) {
    uint8_t* p = (uint8_t*)ptr;
    while (num--) {
        *p++ = (uint8_t)value;
    }
    return ptr;
}

void* k_memcpy(void* dest, const void* src, size_t num) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (num--) {
        *d++ = *s++;
    }
    return dest;
}

int k_memcmp(const void* s1, const void* s2, size_t n) {
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++; p2++;
    }
    return 0;
}

// ==========================================
// СТРОКИ И ВЫВОД
// ==========================================

size_t k_strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

void k_print(const char* str) {
    if (!str) return;
    while (*str) vga_putc(*str++);
}

int k_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int k_strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) { s1++; s2++; n--; }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
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
                case 'u': { // НОВОЕ: Unsigned Decimal
                    unsigned int val = va_arg(args, unsigned int);
                    char buf[16];
                    k_uitoa(val, buf, 10);
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
                case 'p': { // НОВОЕ: Pointer (для адресов памяти)
                    void* ptr = va_arg(args, void*);
                    k_print("0x");
                    char buf[16];
                    k_uitoa((unsigned int)(uintptr_t)ptr, buf, 16);
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
                case '%': vga_putc('%'); break;
                case '\0': goto end;
                default: vga_putc('%'); vga_putc(*fmt); break;
            }
        } else {
            vga_putc(*fmt);
        }
        fmt++;
    }
end:
    va_end(args);
}

// ==========================================
// КОНВЕРТАЦИЯ ЧИСЕЛ
// ==========================================

int k_atoi(const char* str) {
    if (!str) return 0;
    int result = 0, sign = 1;
    while (*str == ' ') str++;
    if (*str == '-') { sign = -1; str++; } 
    else if (*str == '+') str++;
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result * sign;
}

uint32_t k_atoh(const char* str) {
    if (!str) return 0;
    uint32_t result = 0;
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) str += 2;
    while (*str) {
        char c = *str++;
        result <<= 4;
        if (c >= '0' && c <= '9') result |= (c - '0');
        else if (c >= 'a' && c <= 'f') result |= (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') result |= (c - 'A' + 10);
        else break;
    }
    return result;
}

void k_itoa(int value, char* buf, int base) {
    char tmp[33];
    int i = 0, negative = 0;
    if (value == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    
    unsigned int uvalue;
    // ИСПРАВЛЕНИЕ UB: Безопасная работа с INT_MIN
    if (value < 0 && base == 10) {
        negative = 1;
        uvalue = -(unsigned int)value; 
    } else {
        uvalue = (unsigned int)value;
    }
    
    while (uvalue > 0) {
        int rem = uvalue % base;
        tmp[i++] = (rem < 10) ? (rem + '0') : (rem - 10 + 'a');
        uvalue /= base;
    }
    
    int j = 0;
    if (negative) buf[j++] = '-';
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}

void k_uitoa(unsigned int value, char* buf, int base) {
    char tmp[33];
    int i = 0;
    if (value == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    while (value > 0) {
        int rem = value % base;
        tmp[i++] = (rem < 10) ? (rem + '0') : (rem - 10 + 'a');
        value /= base;
    }
    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}
#ifndef TSS_H
#define TSS_H

#include <stdint.h>

// Структура Task State Segment (104 байта)
struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0;       // Указатель на стек ядра
    uint32_t ss0;        // Селектор сегмента данных ядра
    uint32_t esp1, ss1;
    uint32_t esp2, ss2;
    uint32_t cr3;
    uint32_t eip, eflags;
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

void tss_install(void);
void tss_set_kernel_stack(uint32_t ss0, uint32_t esp0);

#endif

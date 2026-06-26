#include "tss.h"
#include "gdt.h"
#include <stddef.h>

// ASM-функция для загрузки Task Register
extern void tss_flush();

// Глобальный экземпляр TSS
static struct tss_entry tss;

void tss_install(void) {
    // 1. Получаем адрес стека ядра из boot.asm
    extern uint32_t stack_top;

    // 2. Обнуляем TSS
    // (В реальном проекте лучше использовать k_memset, но для простоты пока так)
    struct tss_entry* t = &tss;
    for (uint32_t i = 0; i < sizeof(struct tss_entry); i++) {
        ((uint8_t*)t)[i] = 0;
    }

    // 3. Настраиваем критически важные поля
    t->ss0 = 0x10;                  // Kernel Data Segment
    t->esp0 = (uint32_t)&stack_top; // Вершина стека ядра

    // IOMap Base должен указывать за пределы TSS, иначе CPU кинет #GP при in/out из Ring 3
    t->iomap_base = sizeof(struct tss_entry);

    // 4. Прописываем TSS в 5-й слот GDT (селектор 0x28)
    // Access: 0x89 (Present=1, DPL=0, S=0, Type=1001b - 32-bit TSS Available)
    // Granularity: 0x00 (Limit в байтах)
    gdt_set_gate(5, (uint32_t)&tss, sizeof(struct tss_entry) - 1, 0x89, 0x00);

    // 5. Загружаем селектор TSS в регистр TR процессора
    tss_flush();
}

// На случай, если понадобится менять стек ядра динамически (для многозадачности)
void tss_set_kernel_stack(uint32_t ss0, uint32_t esp0) {
    tss.ss0 = ss0;
    tss.esp0 = esp0;
}

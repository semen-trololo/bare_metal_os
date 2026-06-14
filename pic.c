#include "pic.h"
#include "port_io.h"
#include <stdint.h>

// Порты контроллеров PIC
#define PIC1_COMMAND 0x20  // Master PIC Command/Status
#define PIC1_DATA    0x21  // Master PIC Data/Mask
#define PIC2_COMMAND 0xA0  // Slave PIC Command/Status
#define PIC2_DATA    0xA1  // Slave PIC Data/Mask

// Initialization Command Words (ICW)
#define ICW1_ICW4      0x01  // Требуется ICW4
#define ICW1_SINGLE    0x02  // Single mode (0 = cascade)
#define ICW1_INTERVAL4 0x04  // Call address interval 4 (0 = 8)
#define ICW1_LEVEL     0x08  // Level triggered (0 = edge)
#define ICW1_INIT      0x10  // Initialization command

#define ICW4_8086       0x01  // 8086/88 mode (0 = MCS-80/85)
#define ICW4_AUTO       0x02  // Auto EOI
#define ICW4_BUF_SLAVE  0x08  // Buffered mode/slave
#define ICW4_BUF_MASTER 0x0C  // Buffered mode/master
#define ICW4_SFNM       0x10  // Special fully nested mode

void pic_remap(void) {
    // Сохраняем маски прерываний (какие IRQ разрешены)
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);

    // ICW1: начинаем инициализацию в каскадном режиме
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    
    // ICW2: задаём базовые векторы прерываний
    outb(PIC1_DATA, 0x20);  // Master PIC: IRQ 0-7 -> INT 32-39 (0x20 = 32)
    outb(PIC2_DATA, 0x28);  // Slave PIC:  IRQ 8-15 -> INT 40-47 (0x28 = 40)
    
    // ICW3: настраиваем каскадирование
    outb(PIC1_DATA, 0x04);  // Master: Slave подключён к IRQ 2 (бит 2 = 1)
    outb(PIC2_DATA, 0x02);  // Slave:  Cascade identity = 2
    
    // ICW4: режим 8086
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    // Восстанавливаем сохранённые маски
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}
void irq_set_mask(uint8_t IRQline) {
    uint16_t Port;
    uint8_t value;
    if(IRQline < 8) {
        Port = PIC1_DATA;
    } else {
        Port = PIC2_DATA;
        IRQline -= 8;
    }
    value = inb(Port) | (1 << IRQline); // Устанавливаем бит в 1 (маскируем)
    outb(Port, value);        
}

void irq_clear_mask(uint8_t IRQline) {
    uint16_t Port;
    uint8_t value;
    if(IRQline < 8) {
        Port = PIC1_DATA;
    } else {
        Port = PIC2_DATA;
        IRQline -= 8;
    }
    value = inb(Port) & ~(1 << IRQline); // Сбрасываем бит в 0 (разрешаем)
    outb(Port, value);        
}

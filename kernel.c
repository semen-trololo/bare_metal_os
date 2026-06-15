#include "klib.h"
#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "keyboard.h"
#include "shell.h"
#include "timer.h"

void kernel_main() {
    gdt_install();
    idt_install();
    vga_init();
    
    vga_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    k_print("\n Bare Metal OS v1.0\n");
    k_print(" ------------------\n");
    k_print(" [OK] GDT installed\n");
    k_print(" [OK] IDT installed\n");
    k_print(" [OK] PIC remapped\n");
    k_print(" [OK] Video mode: 80x50\n");
    
    // Установка драйвера клавиатуры
    keyboard_install();

    timer_init(1000);  // 1000 Гц = 1 тик каждые 1 мс
    
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    k_print("\n System ready. Start typing:\n ");

    
    k_print("\n");

    shell_run();
}

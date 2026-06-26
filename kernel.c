#include "klib.h"
#include "vga.h"
#include "framebuffer.h"
#include "gdt.h"
#include "idt.h"
#include "keyboard.h"
#include "shell.h"
#include "timer.h"
#include "pmm.h"
#include "paging.h"
#include "heap.h"
#include "tss.h"
#include "syscall.h"
#include "multiboot.h" // <-- Добавили для валидации magic

extern framebuffer_info_t fb_params;
extern void enter_usermode(uint32_t user_esp);
extern void user_task(void);

void kernel_main(uint32_t multiboot_magic, void* multiboot_info, framebuffer_info_t* fb_info) {
    (void)multiboot_magic;

    // === ШАГ 1: Инициализация Видео (ПОКА PAGING ВЫКЛЮЧЕН!) ===
    fb_init(fb_info);
    
    if (fb_is_available()) {
        fb_clear(COLOR_BLACK);
    } else {
        vga_init();
    }

    k_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    k_print("\n Bare Metal OS v1.0\n");
    k_print(" ------------------\n");
    k_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    // === ШАГ 2: Прерывания и защита ===
    gdt_install();
    k_print(" [OK] GDT installed\n");
    
    tss_install();
    k_print(" [OK] TSS installed\n");
    
    idt_install();
    k_print(" [OK] IDT installed\n");
    
    syscall_init();
    k_print(" [OK] Syscalls initialized\n");

    // === ШАГ 3: PMM (Логи видны, CR0.PG=0) ===
    k_print("\n [..] Initializing Memory Subsystem...\n");
    pmm_init((multiboot_info_t*)multiboot_info);
    k_print(" [OK] PMM initialized\n");

    // === ШАГ 4: VMM (Включение Paging) ===
    // paging_init() теперь САМА замапит фреймбуфер перед включением CR0.PG!
    paging_init();
    k_print(" [OK] VMM initialized (Paging ENABLED)\n");

    // === ШАГ 5: Kernel Heap ===
    heap_init();
    k_print(" [OK] Kernel Heap initialized\n");

    // === ШАГ 6: Устройства ===
    k_print("\n [..] Initializing Devices...\n");
    keyboard_install();
    k_print(" [OK] PS/2 Keyboard installed\n");
    
    timer_init(1000);
    k_print(" [OK] PIT Timer installed (1000 Hz)\n");

    // === ШАГ 7: Shell ===
    k_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    k_print("\n System ready. Start typing:\n\n");

    shell_run();
}
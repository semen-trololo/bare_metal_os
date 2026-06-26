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

extern framebuffer_info_t fb_params;
extern void enter_usermode(uint32_t user_esp);
extern void user_task(void);

void kernel_main(uint32_t multiboot_magic, void* multiboot_info, framebuffer_info_t* fb_info) {
    (void)multiboot_magic;
    (void)multiboot_info;
    
    gdt_install();
    tss_install();
    idt_install();
    syscall_init();
    
    // Инициализация памяти (обязательно ДО маппинга framebuffer!)
    pmm_init();
    paging_init();
    heap_init();
    
    // Инициализация framebuffer
    fb_init(fb_info);
    
    if (fb_is_available()) {
        // Мапим framebuffer в виртуальное пространство
        uint32_t fb_phys = fb_info->address;
        uint32_t fb_size = fb_info->width * fb_info->height * 4;
        uint32_t pages = (fb_size + 4095) / 4096;
        for (uint32_t i = 0; i < pages; i++) {
            vmm_map_page(fb_phys + i*4096, fb_phys + i*4096, 0x03);
        }
        
        fb_clear(COLOR_BLACK);
    } else {
        vga_init();
    }
    
    // === ТЕПЕРЬ ПРОСТО ИСПОЛЬЗУЕМ k_print / k_printf ===
    // Они сами выберут правильный бэкенд!
    
    k_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    k_print("\n Bare Metal OS v1.0\n");
    k_print(" ------------------\n");
    
    k_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    k_print(" [OK] GDT installed\n");
    k_print(" [OK] IDT installed\n");
    k_print(" [OK] Memory subsystem\n");
    
    if (fb_is_available()) {
        k_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
        k_printf(" [OK] Framebuffer: %u x %u @ 0x%x\n", 
                 fb_get_width(), fb_get_height(), fb_info->address);
    }
    
    keyboard_install();
    timer_init(1000);
    
    k_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    k_print("\n System ready. Start typing:\n");

    k_print("\n [OK] Preparing to enter User Mode (Ring 3)...\n");
    
    // Выделяем 4 КБ под стек пользователя (используем kmalloc)
    // В будущем мы будем выделять страницы через PMM и мапить их в User Space
    uint8_t* user_stack = (uint8_t*)kmalloc(4096);
    if (!user_stack) {
        k_print(" [FAIL] Cannot allocate user stack!\n");
        while(1) __asm__ volatile("hlt");
    }
    
    // Стек растет вниз, поэтому передаем адрес КОНца выделенного блока
    uint32_t user_esp = (uint32_t)(user_stack + 4096);
    
    k_printf(" [INFO] User stack allocated at 0x%x\n", user_esp);
    //k_print(" [INFO] Jumping to Ring 3... (Timer interrupts should keep working)\n");
    
    // Прыжок! Эта функция не вернется (она сделает iret в user_task)
    //enter_usermode(user_esp);

    // Если мы оказались здесь - значит iret не сработал (Triple Fault или ошибка)
    k_print(" [FAIL] Returned from enter_usermode! This shouldn't happen.\n");
    
    shell_run();
}
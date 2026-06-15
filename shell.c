#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "klib.h"
#include "timer.h"  // <--- ДОБАВИТЬ (для timer_get_ticks и timer_get_frequency)

static void print_help(void) {
    vga_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    k_print("Available commands:\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    k_print("  help    - Show this help message\n");
    k_print("  clear   - Clear the screen\n");
    k_print("  uptime  - Show system uptime\n");  // <--- ДОБАВИТЬ
}

static void execute_command(const char* cmd) {
    if (k_strcmp(cmd, "help") == 0) {
        print_help();
    }
    else if (k_strcmp(cmd, "clear") == 0) {
        clear();
    }
    // === НОВАЯ КОМАНДА: UPTIME ===
    else if (k_strcmp(cmd, "uptime") == 0) {
        uint32_t ticks = timer_get_ticks();
        uint32_t freq = timer_get_frequency(); // Должно быть 1000
        
        // Конвертация в секунды
        uint32_t total_seconds = ticks / freq;
        
        // Разбивка на часы, минуты, секунды
        uint32_t hours   = total_seconds / 3600;
        uint32_t minutes = (total_seconds % 3600) / 60;
        uint32_t seconds = total_seconds % 60;
        
        // Красивый вывод с цветами
        vga_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
        k_print("System Uptime: ");
        
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        k_printf("%d hours, %d minutes, %d seconds\n", hours, minutes, seconds);
        
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK); // Если есть такой цвет, иначе LIGHT_GREY
        k_printf("  (Raw ticks: %d @ %d Hz)\n", ticks, freq);
        
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    }
    else if (k_strlen(cmd) > 0) {
        vga_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
        k_print("Unknown command: ");
        k_print(cmd);
        k_print("\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    }
}

void shell_run(void) {
    char buffer[256];
    int pos = 0;
    
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    while (1) {
        k_print("> ");
        pos = 0;
        buffer[0] = '\0';
        
        // Read command (существующий код чтения из буфера клавиатуры)
        while (1) {
            char c = k_getchar();
            
            if (c == 0) {
                __asm__ volatile("hlt");
                continue;
            }
            
            if (c == '\n') {
                vga_putc('\n');
                buffer[pos] = '\0';
                break;
            }
            else if (c == '\b') {
                if (pos > 0) {
                    pos--;
                    buffer[pos] = '\0';
                    vga_putc('\b');
                }
            }
            else if (c >= 32 && c < 127) {
                if (pos < 255) {
                    buffer[pos++] = c;
                    buffer[pos] = '\0';
                    vga_putc(c);
                }
            }
        }
        
        // Execute command
        execute_command(buffer);
    }
}
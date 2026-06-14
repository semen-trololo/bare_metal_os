#include "shell.h"
#include "keyboard.h"
#include "vga.h"
#include "klib.h"

#define CMD_BUFFER_SIZE 256

// --- Внутренние команды Shell ---

static void cmd_help(void) {
    k_print("\n");
    k_print("Available commands:\n");
    k_print("  help   - Show this help message\n");
    k_print("  clear  - Clear the screen\n");
}

static void shell_execute(const char* cmd) {
    if (k_strcmp(cmd, "help") == 0) {
        cmd_help();
    } else if (k_strcmp(cmd, "clear") == 0) {
        clear(); // Вызываем функцию очистки VGA
    } else if (k_strlen(cmd) > 0) {
        k_print("shell: command not found: ");
        k_print(cmd);
        k_print("\n");
    }
}

// --- Главный цикл оболочки ---

void shell_run(void) {
    char cmd_buffer[CMD_BUFFER_SIZE];
    int cmd_index = 0;
    
    k_print("\nBare Metal OS Shell initialized.\n");
    k_print("Type 'help' for available commands.\n\n");
    k_print("shell# ");


    while (1) {
        char c = k_getchar();
        
        if (c == 0) {
            // Буфер клавиатуры пуст. 
            // Усыпляем CPU до следующего аппаратного прерывания (IRQ).
            __asm__ volatile("hlt");
            continue;
        }
        
        if (c == '\n') {
            // Нажат Enter
            vga_putc('\n');
            cmd_buffer[cmd_index] = '\0'; // Завершаем строку для k_strcmp
            
            shell_execute(cmd_buffer);
            
            cmd_index = 0; // Сбрасываем индекс буфера
            k_print("\n");
            k_print("shell# ");
        } 
        else if (c == '\b') {
            // Нажат Backspace
            if (cmd_index > 0) {
                cmd_index--;
                cmd_buffer[cmd_index] = '\0';
                vga_putc('\b'); // Делегируем стирание символа VGA-драйверу
            }
        } 
        else if (c >= 32 && c < 127) {
            // Только печатные ASCII символы (игнорируем Tab, ESC и прочее)
            if (cmd_index < CMD_BUFFER_SIZE - 1) {
                cmd_buffer[cmd_index] = c;
                cmd_index++;
                cmd_buffer[cmd_index] = '\0'; // Держим строку нуль-терминированной
                vga_putc(c); // Эхо на экран
            }
        }
    }
}

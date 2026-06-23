#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "klib.h"
#include "timer.h"
#include "pmm.h"
#include "heap.h"

#define CMD_BUFFER_SIZE 256
#define MAX_ARGS 4
#define MAX_ARG_LEN 64

// Статический массив для стресс-теста PMM (занимает 128 КБ в секции .bss)
static uint32_t test_allocations[PMM_PAGES_COUNT];

// Токенайзер: разбивает строку на аргументы
static int parse_args(char* buffer, char args[MAX_ARGS][MAX_ARG_LEN]) {
    int argc = 0;
    char* ptr = buffer;
    
    for (int i = 0; i < MAX_ARGS; i++) args[i][0] = '\0';

    while (*ptr && argc < MAX_ARGS) {
        while (*ptr == ' ') ptr++; // Пропускаем пробелы
        if (*ptr == '\0') break;
        
        int i = 0;
        while (*ptr && *ptr != ' ' && i < MAX_ARG_LEN - 1) {
            args[argc][i++] = *ptr++;
        }
        args[argc][i] = '\0';
        argc++;
    }
    return argc;
}

static void print_help(void) {
    vga_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    k_print("Available commands:\n");
    
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    k_print("  help             - Show this help message\n");
    k_print("  clear            - Clear the screen\n");
    k_print("  uptime           - Show system uptime\n");
    
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    k_print("  --- Memory Management (PMM) ---\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    k_print("  pmm status       - Show physical memory usage\n");
    k_print("  pmm alloc [num]  - Allocate physical pages\n");
    k_print("  pmm free <addr>  - Free a physical page (hex)\n");
    k_print("  pmm test         - Run PMM stress tests\n");
    k_print("  heap <status|alloc|free|test> - Test heap");
}

// Обработчик команд PMM
static void handle_pmm(int argc, char args[MAX_ARGS][MAX_ARG_LEN]) {
    if (argc < 2) {
        vga_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
        k_print("Usage: pmm <status|alloc|free|test>\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        return;
    }

    if (k_strcmp(args[1], "status") == 0) {
        uint32_t used = pmm_get_used_pages();
        uint32_t free = pmm_get_free_pages();
        uint32_t total = used + free;
        
        vga_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
        k_print("[PMM] --- Physical Memory Status ---\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        k_printf("[PMM] Total Tracked: %d MB (%d pages)\n", (total * 4) / 1024, total);
        k_printf("[PMM] Used:          %d MB (%d pages)\n", (used * 4) / 1024, used);
        k_printf("[PMM] Free:          %d MB (%d pages)\n", (free * 4) / 1024, free);
    } 
    else if (k_strcmp(args[1], "alloc") == 0) {
        uint32_t count = 1;
        if (argc > 2) count = k_atoi(args[2]);
        if (count == 0) count = 1;

        for (uint32_t i = 0; i < count; i++) {
            uint32_t addr = pmm_alloc_page();
            if (addr == 0) {
                vga_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
                k_print("[PMM] ERROR: Out of physical memory!\n");
                vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
                break;
            }
            vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            k_printf("[PMM] Allocated: %x\n", addr);
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        }
    } 
    else if (k_strcmp(args[1], "free") == 0) {
        if (argc < 3) {
            vga_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
            k_print("Usage: pmm free <hex_addr>\n");
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
            return;
        }
        uint32_t addr = k_atoh(args[2]);
        pmm_free_page(addr); // Ошибки (unaligned/out of range) напечатает сама pmm_free_page
    } 
    else if (k_strcmp(args[1], "test") == 0) {
        vga_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
        k_print("[PMM TEST] Starting automated tests...\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        
        uint32_t initial_free = pmm_get_free_pages();

        k_print("[PMM TEST] 1. Testing invalid free (unaligned)... ");
        pmm_free_page(0x200005); 
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        k_print("[OK - Error caught]\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

        k_print("[PMM TEST] 2. Testing OOM limit...\n");
        uint32_t allocated_count = 0;
        while (allocated_count < PMM_PAGES_COUNT) {
            uint32_t addr = pmm_alloc_page();
            if (addr == 0) break;
            test_allocations[allocated_count++] = addr;
        }
        k_printf("[PMM TEST]    Allocated %d pages. Limit reached.\n", allocated_count);

        k_print("[PMM TEST] 3. Freeing all allocated pages... ");
        for (uint32_t i = 0; i < allocated_count; i++) {
            pmm_free_page(test_allocations[i]);
        }
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        k_print("[OK]\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

        k_print("[PMM TEST] 4. Verifying memory is restored... ");
        uint32_t final_free = pmm_get_free_pages();
        if (initial_free == final_free) {
            vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            k_print("[OK]\n");
            k_print("[PMM TEST] All tests passed successfully!\n");
        } else {
            vga_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
            k_printf("[FAIL] Leaked %d pages!\n", initial_free - final_free);
        }
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    } 
    else {
        vga_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
        k_printf("Unknown pmm command: %s\n", args[1]);
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    }
}

static void execute_command(char* buffer) {
    char args[MAX_ARGS][MAX_ARG_LEN];
    int argc = parse_args(buffer, args);
    
    if (argc == 0) return;

    if (k_strcmp(args[0], "help") == 0) {
        print_help();
    }
    else if (k_strcmp(args[0], "clear") == 0) {
        clear();
    }
    else if (k_strcmp(args[0], "uptime") == 0) {
        uint32_t ticks = timer_get_ticks();
        uint32_t freq = timer_get_frequency(); 
        
        uint32_t total_seconds = ticks / freq;
        uint32_t hours   = total_seconds / 3600;
        uint32_t minutes = (total_seconds % 3600) / 60;
        uint32_t seconds = total_seconds % 60;
        
        vga_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
        k_print("System Uptime: ");
        
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        k_printf("%d hours, %d minutes, %d seconds\n", hours, minutes, seconds);
        
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK); 
        k_printf("  (Raw ticks: %d @ %d Hz)\n", ticks, freq);
        
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    }
    else if (k_strcmp(args[0], "heap") == 0) {
        if (argc < 2) {
            k_print("Usage: heap <status|alloc|free|test>\n");
        } else if (k_strcmp(args[1], "status") == 0) {
            heap_print_status();
        } else if (k_strcmp(args[1], "alloc") == 0) {
            if (argc < 3) { k_print("Usage: heap alloc <size>\n"); return; }
            uint32_t size = k_atoi(args[2]);
            void* ptr = kmalloc(size);
            if (ptr) k_printf("[HEAP] Allocated %u bytes at %p\n", size, ptr);
            else k_print("[HEAP] Allocation failed!\n");
        } else if (k_strcmp(args[1], "free") == 0) {
            if (argc < 3) { k_print("Usage: heap free <addr>\n"); return; }
            uint32_t addr = k_atoh(args[2]);
            kfree((void*)addr);
            k_print("[HEAP] Freed.\n");
        } else if (k_strcmp(args[1], "test") == 0) {
            heap_run_tests();
        } else {
            k_print("Unknown heap command.\n");
        }
    }
    else if (k_strcmp(args[0], "pmm") == 0) {
        handle_pmm(argc, args);
    }
    else if (k_strlen(args[0]) > 0) {
        vga_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
        k_print("Unknown command: ");
        k_print(args[0]);
        k_print("\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    }
}

void shell_run(void) {
    char buffer[CMD_BUFFER_SIZE];
    int pos = 0;
    
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    while (1) {
        k_print("> ");
        pos = 0;
        buffer[0] = '\0';
        
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
                if (pos < CMD_BUFFER_SIZE - 1) {
                    buffer[pos++] = c;
                    buffer[pos] = '\0';
                    vga_putc(c);
                }
            }
        }
        
        execute_command(buffer);
    }
}
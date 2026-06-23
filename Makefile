# Компиляторы
# Если ты успешно собрал i686-elf-gcc (Путь Б), оставь как есть.
# Если используешь системный (Путь А), замени на: i686-linux-gnu-gcc
CC = i686-linux-gnu-gcc
AS = nasm

# Директории
BUILD_DIR = build

# Флаги компиляции (Обезвреживание Kali/Debian)
# -fno-pie -fno-pic: Запрещаем генерацию позиционно-независимого кода (критично для ядра!)
# -fno-stack-protector: Отключаем stack canaries, чтобы не требовать __stack_chk_fail из glibc
# -std=gnu99: Включаем GNU-расширения для атрибутов и inline assembly
CFLAGS = -m32 -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Iinclude -fno-pie -fno-pic -fno-stack-protector

# Флаги ассемблера
ASFLAGS = -f elf32

# Флаги линковки
# -no-pie: Запрещаем линкеру создавать PIE-исполняемый файл
# -lgcc: Линкуем libgcc для встроенных функций (деление 64-битных чисел и т.д.)
LDFLAGS = -T linker.ld -nostdlib -no-pie -lgcc

# Автоматический поиск всех исходников в корне
C_SOURCES = $(wildcard *.c)
ASM_SOURCES = $(wildcard *.asm)

# Генерация путей к объектным файлам внутри папки build/
C_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
ASM_OBJS = $(patsubst %.asm,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
OBJ = $(C_OBJS) $(ASM_OBJS)

# Финальный бинарник
TARGET = $(BUILD_DIR)/metal_os.bin

# Цель по умолчанию
all: $(TARGET)

# Компоновка (Linking)
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

# Компиляция C файлов
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Компиляция ASM файлов
$(BUILD_DIR)/%.o: %.asm | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# Создание папки build, если её нет
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Очистка
clean:
	rm -rf $(BUILD_DIR)

# Запуск в QEMU
run: $(TARGET)
	qemu-system-i386 -m 512M -kernel $(TARGET)

.PHONY: all clean run
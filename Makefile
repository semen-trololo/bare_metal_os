# Компиляторы
CC = i686-elf-gcc
AS = nasm

# Директории
BUILD_DIR = build

# Флаги компиляции
CFLAGS = -m32 -std=c99 -ffreestanding -O2 -Wall -Wextra -nostdlib -Iinclude
ASFLAGS = -f elf32
LDFLAGS = -T linker.ld -nostdlib

# Автоматический поиск всех исходников в корне
C_SOURCES = $(wildcard *.c)
ASM_SOURCES = $(wildcard *.asm)

# Генерация путей к объектным файлам внутри папки build/
# patsubst заменяет 'kernel.c' на 'build/kernel.o'
C_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
ASM_OBJS = $(patsubst %.asm,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
OBJ = $(C_OBJS) $(ASM_OBJS)

# Финальный бинарник
TARGET = $(BUILD_DIR)/metal_os.bin

# Цель по умолчанию
all: $(TARGET)

# Компоновка (Linking)
$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

# Компиляция C файлов
# Символ '|' означает order-only prerequisite (папка должна существовать,
# но её наличие не триггерит пересборку, если она уже есть)
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Компиляция ASM файлов
$(BUILD_DIR)/%.o: %.asm | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# Создание папки build, если её нет
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Очистка (теперь просто удаляем всю папку build)
clean:
	rm -rf $(BUILD_DIR)

# Запуск в QEMU
run: $(TARGET)
	qemu-system-i386 -kernel $(TARGET)

.PHONY: all clean run
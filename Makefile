# Компиляторы
CC = i686-elf-gcc
AS = nasm

# Флаги компиляции
CFLAGS = -m32 -std=c99 -ffreestanding -O2 -Wall -Wextra -nostdlib -Iinclude
ASFLAGS = -f elf32
LDFLAGS = -T linker.ld -nostdlib

# Автоматический поиск всех исходников
C_SOURCES = $(wildcard *.c)
ASM_SOURCES = $(wildcard *.asm)

# Объектные файлы
OBJ = ${C_SOURCES:.c=.o} ${ASM_SOURCES:.asm=.o}

# Цель по умолчанию
all: metal_os.bin

# Компоновка
metal_os.bin: $(OBJ)
	i686-elf-gcc $(LDFLAGS) -o $@ $^

# Компиляция C файлов
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Компиляция ASM файлов
%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

# Очистка
clean:
	rm -f *.o metal_os.bin

# Запуск в QEMU
run: metal_os.bin
	qemu-system-i386 -kernel metal_os.bin

.PHONY: all clean run
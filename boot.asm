; ============================================================================
; BARE METAL OS - BOOT LOADER (Multiboot 1 + Bochs VBE)
; ============================================================================

; Константы Multiboot
MBOOT_PAGE_ALIGN    equ 1<<0
MBOOT_MEM_INFO      equ 1<<1
MBOOT_MMAP          equ 1<<6
MBOOT_HEADER_MAGIC  equ 0x1BADB002
MBOOT_HEADER_FLAGS  equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO | MBOOT_MMAP
MBOOT_CHECKSUM      equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

section .multiboot
align 4
    dd MBOOT_HEADER_MAGIC
    dd MBOOT_HEADER_FLAGS
    dd MBOOT_CHECKSUM

; ============================================================================
; BSS - Неинициализированные данные
; ============================================================================
section .bss
align 16
global stack_bottom   ; <--- ДОБАВИТЬ
global stack_top      ; <--- ДОБАВИТЬ (чтобы tss.c мог её видеть)
stack_bottom:
    resb 262144     ; 256 КБ стек ядра
stack_top:

; ============================================================================
; DATA - Структура параметров framebuffer
; ============================================================================
section .data
global fb_params                        ; Делаем видимым для C-кода
fb_params:
    .address    dd 0                    ; Физический адрес framebuffer
    .width      dd 0                    ; Ширина в пикселях
    .height     dd 0                    ; Высота в пикселях
    .pitch      dd 0                    ; Байт на строку (с учетом выравнивания)
    .bpp        dd 0                    ; Бит на пиксель

; ============================================================================
; Bochs VBE константы
; ============================================================================
VBE_DISPI_IOPORT_INDEX  equ 0x01CE
VBE_DISPI_IOPORT_DATA   equ 0x01CF

VBE_DISPI_INDEX_ID          equ 0
VBE_DISPI_INDEX_XRES        equ 1
VBE_DISPI_INDEX_YRES        equ 2
VBE_DISPI_INDEX_BPP         equ 3
VBE_DISPI_INDEX_ENABLE      equ 4
VBE_DISPI_INDEX_BANK        equ 5
VBE_DISPI_INDEX_VIRT_WIDTH  equ 6
VBE_DISPI_INDEX_VIRT_HEIGHT equ 7
VBE_DISPI_INDEX_X_OFFSET    equ 8
VBE_DISPI_INDEX_Y_OFFSET    equ 9

VBE_DISPI_ENABLED       equ 0x01
VBE_DISPI_LFB_ENABLED   equ 0x40

; Стандартный адрес LFB для Bochs VBE (проверен в QEMU -vga std)
VBE_LFB_PHYS_BASE       equ 0xFD000000

; Целевое разрешение
VBE_XRES                equ 1024
VBE_YRES                equ 768
VBE_BPP                 equ 32

; ============================================================================
; Macro: запись в VBE регистр
; ============================================================================
%macro VBE_WRITE 2
    mov dx, VBE_DISPI_IOPORT_INDEX
    mov ax, %1
    out dx, ax
    mov dx, VBE_DISPI_IOPORT_DATA
    mov ax, %2
    out dx, ax
%endmacro

; ============================================================================
; Macro: чтение из VBE регистра
; ============================================================================
%macro VBE_READ 1
    mov dx, VBE_DISPI_IOPORT_INDEX
    mov ax, %1
    out dx, ax
    mov dx, VBE_DISPI_IOPORT_DATA
    in  ax, dx
%endmacro

; ============================================================================
; TEXT - Код
; ============================================================================
section .text
global _start
extern kernel_main

_start:
    ; 1. Настраиваем стек ядра
    mov esp, stack_top

    ; 2. Инициализация графического режима (до вызова C-кода!)
    call init_bochs_vbe

    ; 3. Передача аргументов в kernel_main (cdecl: справа налево)
    push dword fb_params    ; arg3: структура с параметрами VBE
    push ebx                ; arg2: Multiboot Information Structure
    push eax                ; arg1: Multiboot Magic Number (0x2BADB002)

    ; 4. Вызов C-ядра
    call kernel_main
    add esp, 12             ; Очистка стека (3 аргумента × 4 байта)

    ; 5. Если ядро вернулось — останавливаем CPU
    cli
.hang:
    hlt
    jmp .hang

; ============================================================================
; init_bochs_vbe - Настройка Bochs VBE через I/O порты
; ============================================================================
init_bochs_vbe:
    pusha

    ; --- Проверка наличия Bochs VBE ---
    VBE_READ VBE_DISPI_INDEX_ID     ; Читаем ID
    ; ID должен быть 0xB0C0..0xB0C5 (версии VBE)
    ; Если 0xFFFF — VBE не поддерживается
    cmp ax, 0xB0C0
    jb .vbe_fail
    cmp ax, 0xB0C6
    jae .vbe_fail

    ; --- Отключаем VBE перед настройкой ---
    VBE_WRITE VBE_DISPI_INDEX_ENABLE, 0

    ; --- Устанавливаем разрешение и глубину ---
    VBE_WRITE VBE_DISPI_INDEX_XRES, VBE_XRES
    VBE_WRITE VBE_DISPI_INDEX_YRES, VBE_YRES
    VBE_WRITE VBE_DISPI_INDEX_BPP,  VBE_BPP

    ; --- Virtual dimensions (без скроллинга = реальным) ---
    VBE_WRITE VBE_DISPI_INDEX_VIRT_WIDTH,  VBE_XRES
    VBE_WRITE VBE_DISPI_INDEX_VIRT_HEIGHT, VBE_YRES

    ; --- Сбрасываем смещение ---
    VBE_WRITE VBE_DISPI_INDEX_X_OFFSET, 0
    VBE_WRITE VBE_DISPI_INDEX_Y_OFFSET, 0

    ; --- Включаем LFB (Linear Framebuffer) ---
    VBE_WRITE VBE_DISPI_INDEX_ENABLE, (VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED)

    ; --- Сохраняем параметры для C-кода ---
    mov dword [fb_params.address], VBE_LFB_PHYS_BASE
    mov dword [fb_params.width],   VBE_XRES
    mov dword [fb_params.height],  VBE_YRES
    mov dword [fb_params.pitch],   (VBE_XRES * 4)    ; 32bpp = 4 байта/пиксель
    mov dword [fb_params.bpp],     VBE_BPP

    popa
    ret

.vbe_fail:
    ; Fallback: обнуляем параметры — ядро перейдет на VGA text mode
    mov dword [fb_params.address], 0
    mov dword [fb_params.width],   0
    mov dword [fb_params.height],  0
    mov dword [fb_params.pitch],   0
    mov dword [fb_params.bpp],     0
    popa
    ret
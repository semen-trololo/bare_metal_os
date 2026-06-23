; Константы для заголовка Multiboot 1
MBOOT_PAGE_ALIGN    equ 1<<0             ; Выравнивать модули по границам страниц
MBOOT_MEM_INFO      equ 1<<1             ; Запросить информацию о памяти у BIOS
MBOOT_HEADER_MAGIC  equ 0x1BADB002       ; Магическое число (сигнатура) Multiboot
MBOOT_HEADER_FLAGS  equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO ; Флаги
MBOOT_CHECKSUM      equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS) ; Контрольная сумма

; Секция .multiboot ДОЛЖНА быть в самом начале бинарного файла!
section .multiboot
align 4
    dd MBOOT_HEADER_MAGIC   ; Магия
    dd MBOOT_HEADER_FLAGS   ; Флаги
    dd MBOOT_CHECKSUM       ; Чексумма (чтобы GRUB не подумал, что файл битый)

; Секция неинициализированных данных (BSS). Здесь мы выделим память под СТЕК.
section .bss
align 16
stack_bottom:
    resb 262144 ; Резервируем 256 Килобайт памяти для стека
stack_top:     ; Стек в архитектуре x86 растет "вниз" (от старших адресов к младшим).

; Секция кода
section .text
global _start      ; Делаем метку _start видимой для линкера (точка входа)
extern kernel_main ; Говорим, что эта функция написана в другом файле (в C)

_start:
    ; 1. Настраиваем стек. Указатель стека (ESP) должен указывать на ВЕРХУШКУ стека.
    mov esp, stack_top

    ; 2. Передача аргументов в функцию C. 
    ; GRUB кладет магическое число в регистр EAX, а адрес структуры информации в EBX.
    ; В языке C (соглашение cdecl) аргументы передаются через стек СПРАВА НАЛЕВО.
    push ebx    ; Правый аргумент: адрес Multiboot Information Structure
    push eax    ; Левый аргумент: Магическое число Multiboot (0x2BADB002)

    ; 3. Вызываем наше ядро на C
    call kernel_main

    ; 4. Если ядро вернет управление (чего быть не должно), мы должны остановить процессор.
    cli         ; Command Line Interrupts - запрещаем аппаратные прерывания
.hang:
    hlt         ; Halt - останавливаем процессор до следующего прерывания
    jmp .hang   ; Бесконечный цикл на случай, если прерывание всё же случится

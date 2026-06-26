[bits 32]

; Объявляем внешние C-функции
extern isr_handler
extern irq_handler

; ============================================
; Макрос для ISR БЕЗ кода ошибки
; Большинство исключений НЕ пушат err_code автоматически
; Поэтому мы пушим 0 сами, чтобы стек был одинаковым для всех
; ============================================
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push dword 0           ; Фейковый код ошибки (для единообразия стека)
    push dword %1          ; Номер прерывания (0-31)
    jmp isr_common_stub    ; Переход к общему обработчику
%endmacro

; ============================================
; Макрос для ISR С кодом ошибки
; Некоторые исключения (8, 10-14, 17) CPU пушит err_code автоматически
; ============================================
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    ; err_code уже в стеке (от CPU)
    push dword %1          ; Номер прерывания
    jmp isr_common_stub
%endmacro

; ============================================
; Макрос для IRQ
; IRQ 0-15 маппятся на INT 32-47
; ============================================
%macro IRQ 2
global irq%1
irq%1:
    push dword 0           ; Фейковый код ошибки
    push dword %2          ; Номер прерывания (32 + IRQ номер)
    jmp irq_common_stub
%endmacro

; ============================================
; Генерируем все ISR (0-31)
; ============================================
ISR_NOERRCODE 0   ; Division By Zero
ISR_NOERRCODE 1   ; Debug
ISR_NOERRCODE 2   ; Non Maskable Interrupt
ISR_NOERRCODE 3   ; Breakpoint
ISR_NOERRCODE 4   ; Into Detected Overflow
ISR_NOERRCODE 5   ; Out of Bounds
ISR_NOERRCODE 6   ; Invalid Opcode
ISR_NOERRCODE 7   ; No Coprocessor
ISR_ERRCODE   8   ; Double Fault
ISR_NOERRCODE 9   ; Coprocessor Segment Overrun
ISR_ERRCODE   10  ; Bad TSS
ISR_ERRCODE   11  ; Segment Not Present
ISR_ERRCODE   12  ; Stack Fault
ISR_ERRCODE   13  ; General Protection Fault
ISR_ERRCODE   14  ; Page Fault
ISR_NOERRCODE 15  ; Reserved
ISR_NOERRCODE 16  ; Floating Point Exception
ISR_ERRCODE   17  ; Alignment Check
ISR_NOERRCODE 18  ; Machine Check
ISR_NOERRCODE 19  ; SIMD Floating-Point Exception
ISR_NOERRCODE 20  ; Virtualization Exception
ISR_NOERRCODE 21  ; Control Protection Exception
ISR_NOERRCODE 22  ; Reserved
ISR_NOERRCODE 23  ; Reserved
ISR_NOERRCODE 24  ; Reserved
ISR_NOERRCODE 25  ; Reserved
ISR_NOERRCODE 26  ; Reserved
ISR_NOERRCODE 27  ; Reserved
ISR_NOERRCODE 28  ; Hypervisor Injection Exception
ISR_NOERRCODE 29  ; VMM Communication Exception
ISR_ERRCODE   30  ; Security Exception
ISR_NOERRCODE 31  ; Reserved
ISR_NOERRCODE 128 ; System Call (INT 0x80)

; ============================================
; Генерируем все IRQ (0-15)
; ============================================
IRQ  0, 32   ; System Timer
IRQ  1, 33   ; Keyboard
IRQ  2, 34   ; Cascade for PIC 2
IRQ  3, 35   ; COM2
IRQ  4, 36   ; COM1
IRQ  5, 37   ; LPT2
IRQ  6, 38   ; Floppy Disk
IRQ  7, 39   ; LPT1 / Spurious
IRQ  8, 40   ; CMOS Real-time Clock
IRQ  9, 41   ; Free
IRQ 10, 42   ; Free
IRQ 11, 43   ; Free
IRQ 12, 44   ; PS/2 Mouse
IRQ 13, 45   ; FPU
IRQ 14, 46   ; Primary ATA Hard Disk
IRQ 15, 47   ; Secondary ATA Hard Disk

; ============================================
; Общий обработчик для CPU-исключений (ISR)
; ============================================
isr_common_stub:
    pusha                  ; Сохраняем 8 регистров общего назначения
    
    ; Пушим все сегментные регистры (C ожидает 16 байт)
    mov ax, ds
    push eax
    mov ax, es
    push eax
    mov ax, fs
    push eax
    mov ax, gs
    push eax
    
    ; Переключаемся на Kernel Data Segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp               ; Передаём указатель на struct regs в C
    call isr_handler
    add esp, 4             ; Очищаем аргумент
    
    ; Восстанавливаем сегментные регистры в обратном порядке
    pop eax
    mov gs, ax
    pop eax
    mov fs, ax
    pop eax
    mov es, ax
    pop eax
    mov ds, ax
    
    popa                   ; Восстанавливаем EAX...EDI
    add esp, 8             ; Убираем err_code и int_no
    iret                   ; Возврат из прерывания

; ============================================
; Общий обработчик для IRQ
; ============================================
irq_common_stub:
    pusha
    
    mov ax, ds
    push eax
    mov ax, es
    push eax
    mov ax, fs
    push eax
    mov ax, gs
    push eax
    
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp
    call irq_handler
    add esp, 4
    
    pop eax
    mov gs, ax
    pop eax
    mov fs, ax
    pop eax
    mov es, ax
    pop eax
    mov ds, ax
    
    popa
    add esp, 8
    iret
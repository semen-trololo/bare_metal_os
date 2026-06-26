[bits 32]

; Внешняя C-функция, которая будет работать в Ring 3
extern user_task

; ============================================
; void enter_usermode(uint32_t user_esp);
; Готовит стек и делает iret в Ring 3
; ============================================
global enter_usermode
enter_usermode:
    ; 1. Загружаем пользовательские сегментные регистры данных
    ; 0x23 = 0x20 (User Data) | 3 (RPL=3)
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 2. Получаем указатель на user-стек из аргументов функции
    mov ecx, [esp+4] 

    ; 3. Формируем стек для инструкции iret
    push 0x23             ; SS (User Data Segment, RPL=3)
    push ecx              ; ESP (Указатель на верхушку user-стека)
    
    pushf                 ; Читаем текущий EFLAGS в стек
    pop eax               ; Достаем его в EAX
    or eax, 0x200         ; Включаем бит IF (Interrupt Flag), чтобы прерывания работали!
    push eax              ; Пушим модифицированный EFLAGS обратно

    push 0x1B             ; CS (User Code Segment, RPL=3. 0x18 | 3)
    push user_task        ; EIP (Адрес функции, с которой начнем выполнение)

    ; 4. Прыжок! Процессор видит DPL=3 в CS и переключает CPL на 3.
    iret                  
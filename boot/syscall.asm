; syscall.asm - точка входа SYSCALL для x86-64
;
; При SYSCALL: RCX=user RIP, R11=user RFLAGS. Стек не переключается — делаем вручную.
; Аргументы: RAX=номер, RDI,RSI,RDX,R10,R8,R9 (R10 вместо RCX в ABI).

BITS 64

SECTION .text

global syscall_entry
extern syscall_dispatch
extern tss64

; Временное сохранение user RSP (однопроцессорно)
SECTION .bss
align 8
user_rsp_save: resq 1

SECTION .text

syscall_entry:
    ; Сохраняем user RSP (SYSCALL не переключает стек)
    mov [rel user_rsp_save], rsp

    ; Переключаемся на kernel stack из TSS.RSP0 (offset 4)
    mov rsp, [rel tss64 + 4]

    ; Сохраняем user context на kernel stack (RCX=RIP, R11=RFLAGS — сохранил CPU)
    push qword [rel user_rsp_save]   ; user RSP
    push r11                         ; user RFLAGS
    push rcx                         ; user RIP

    ; Выравнивание стека до 16 байт (System V ABI) перед call
    ; Сейчас: 3 pushes = 24 байта. RSP должен быть 16-align перед call.
    ; После 3 push: rsp % 16 = 8. Ещё один push -> 32, rsp%16=0.
    push rax                         ; syscall number (сохраняем, align станет 0)

    ; Аргументы уже в RDI,RSI,RDX,R10,R8,R9. syscall_dispatch(num, ...)
    ; RAX = номер, остальное по ABI.
    mov rdi, rax                     ; 1-й arg: syscall number
    ; RSI,RDX,RCX,R8,R9 — аргументы syscall (R10->RCX для 4-го, но C ABI: RDI,RSI,RDX,RCX,R8,R9)
    ; SYSCALL сохраняет всё кроме RCX,R11. R10 уже содержит 4-й аргумент user.
    mov rcx, r10                     ; 4-й arg в RCX

    call syscall_dispatch

    ; Возврат: RAX = результат. Восстанавливаем user context.
    pop rbx                          ; syscall number (discard)
    pop rcx                          ; user RIP
    pop r11                          ; user RFLAGS
    pop rsp                          ; user RSP — переключаемся обратно на user stack

    ; RCX=RIP, R11=RFLAGS, RAX=return value. SYSRET.
    sysret

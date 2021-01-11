[bits 32]
section .text
global task_switch
; task_switch(cur, next)
; those parameters are two PCBs
; and the first 4 bytes of a PCB is kernel stack pointer
; the default calling convention is 'cdecl' that means
; 'next' pushed first, then 'cur'.
task_switch:
    push    esi
    push    edi
    push    ebx
    push    ebp

    ; [esp + 20] = cur PCB
    ; save 'cur PCB' in eax
    mov     eax, [esp + 20]
    ; save current kernel stack pointer into first 4 byte of PCB
    mov     [eax], esp

    ; load next->kstack into eax
    mov     eax, [esp + 24]
    ; switch stack
    mov     esp, [eax]

    ; do the task switching
    pop     ebp
    pop     ebx
    pop     edi
    pop     esi
    ret

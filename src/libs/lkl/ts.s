[bits 32]
section .text
global __task_switch
; task_switch(cur, next)
; those parameters are two PCBs
; and the first 4 bytes of a PCB is kernel stack pointer
; the default calling convention is 'cdecl' that means
; 'next' pushed first, then 'cur'.
__task_switch:
; why are we saving those 4 registers? 
; the most important reason is that we're in the middle of kernel code
; and we don't want to crash the kernel after 'switching'. according to
; ABI(application binary interface), we don't have to all general purpose
; registers.
; 
; ┌─────┬────────────────────────────────────────────────────────┬──────┐
; │ Reg │ Usage                                                  │ PRSV │
; ├─────┼────────────────────────────────────────────────────────┼──────┤
; │ EAX │ scratch register; also used to return integer and poi- │ NO   │
; │     │ nter values from functions; also stores the address of │      │
; │     │ a returned struct or union.                            │      │
; ├─────┼────────────────────────────────────────────────────────┼──────┤
; │ EBX │ callee-saved register; also used to hold the GOT poin- │ YES  │
; │     │ ter when making function calls via the PLT.            │      │
; ├─────┼────────────────────────────────────────────────────────┼──────┤
; │ ECX │ scratch register; also used as count register          │ NO   │
; ├─────┼────────────────────────────────────────────────────────┼──────┤
; │ EDX │ scratch register; also used to return the upper 32bits │ NO   │
; │     │ of some 64bit return types.                            │      │
; ├─────┼────────────────────────────────────────────────────────┼──────┤
; │ ESP │ stack pointer                                          │ YES  │
; ├─────┼────────────────────────────────────────────────────────┼──────┤
; │ EBP │ callee-saved register; optionally used as frame pointer│ YES  │
; ├─────┼────────────────────────────────────────────────────────┼──────┤
; │ ESI │ callee-saved register;                                 │ YES  │
; ├─────┼────────────────────────────────────────────────────────┼──────┤
; │ EDI │ callee-saved register;                                 │ YES  │
; └─────┴────────────────────────────────────────────────────────┴──────┘
;  
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
; CAUTION:
; those ebp, ebx, edi, esi are not the same register values we saved 
; above. 
    pop     ebp
    pop     ebx
    pop     edi
    pop     esi
    ret 
;   eip         ; 'ret' will go back to caller (say caller_b)
;   eip_dummy   ; this is caller of caller_b (say caller_a)
;   thread_func ; this is arg1 of caller_b
;   arg         ; this is arg2 of caller_b
;
; when we creating new thread, there's no caller_a, the eip_dummy has
; a fake value, and CPU never goes back to eip_dummy (fake caller_a).


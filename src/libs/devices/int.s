[bits 32]

%define ERROR_CODE nop
%define ZERO push 0

;
; Basic INTERRUPT Procedures: 
;
; If an interrupt occurred in userspace (say CPL = 3), CPU does the
; following:
; 1. CPU saves SS, ESP, EFLAGS, CS and EIP registers in somewhere
;    temporarily. leaves 'user stack' intact.
; 
;   │ User Stack               │         │ ESP0 from TSS            │ H
;   ├──────────────────────────┤         ├──────────────────────────┤
;   │           0x56           │         │                          │
;   ├──────────────────────────┤         ├──────────────────────────┤
;   │           0x34           │         │                          │
;   ├──────────────────────────┤         ├──────────────────────────┤
;   │           0x12           │         │                          │
;   ├──────────────────────────┤         ├──────────────────────────┤
;   │                          │         │                          │
;   ├──────────────────────────┤         ├──────────────────────────┤
;   │                          │         │                          │
;   ├──────────────────────────┤         ├──────────────────────────┤
;   │                          │         │                          │ L
; 
; 2. Loads SS0 and ESP0 (CPL = 0) from the TSS into the SS and ESP
;    registers and switches to new stack.
; 
;   │ ESP = ESP0   (kernel)    │ H
;   ├──────────────────────────┤
;   │                          │
;   ├──────────────────────────┤
;   │                          │
;   ├──────────────────────────┤
;   │                          │
;   ├──────────────────────────┤
;   │                          │
;   ├──────────────────────────┤
;   │                          │
;   ├──────────────────────────┤
;   │                          │ L
;
; 3. Pushes registers which CPU saved in step 1 into new stack.
; 4. Pushes an error code on the new stack if it has one.
; 
;   │ ESP = ESP0   (kernel)    │ H
;   ├──────────────────────────┤
;   │          SS              │
;   ├──────────────────────────┤
;   │          ESP             │
;   ├──────────────────────────┤
;   │          EFLAGS          │
;   ├──────────────────────────┤
;   │          CS              │
;   ├──────────────────────────┤
;   │          EIP             │
;   ├──────────────────────────┤
;   │          Error Code      │
;   ├──────────────────────────┤
;   │                          │ L
; 
; 5. Loads 'selector' and 'offset' from 'IDT' into CS and EIP.
; 6. If call is through an interrupt gate, clears the 'IF' flag in the
;    EFLAGS register.
; 7. Execute ISR at new privilege level.
; 
;   │ User Stack               │         │ ESP0 from TSS            │ H
;   ├──────────────────────────┤         ├──────────────────────────┤
;   │           0x56           │         │                          │
;   ├──────────────────────────┤         ├──────────────────────────┤
;   │           0x34           │         │                          │
;   ├──────────────────────────┤         ├──────────────────────────┤
;   │           0x12           │         │                          │
;   ├──────────────────────────┤         ├──────────────────────────┤
;   │                          │         │                          │
;   ├──────────────────────────┤         ├──────────────────────────┤
;   │                          │         │                          │
;   ├──────────────────────────┤         ├──────────────────────────┤
;   │                          │         │                          │ L
; 
; KERNEL:
; If an interrupt occurred in kernel space, no stack switching occurs.
; CPU just uses current kernel stack, those steps are:
; 1. Push EFLAGS, CS, EIP into stack.
; 2. Push an error code if there is one.
;  
;    │ Kernel Stack             │ H
;    ├──────────────────────────┤
;    │          EFLAGS          │
;    ├──────────────────────────┤
;    │          CS              │
;    ├──────────────────────────┤
;    │          EIP             │
;    ├──────────────────────────┤
;    │          Error Code      │
;    ├──────────────────────────┤
;    │                          │
;    ├──────────────────────────┤
;    │                          │ L
;
; 3. Load new CS, EIP from IDT.
; 4. Clear 'IF' flag in the EFLAGS if call is through an 
;    'interrupt gate'.
; 5  Execute the ISR.
; 
; The major difference between two cases is 'Stack Switching' and in
; user space, SS, ESP pushed into kernel stack. but don't worry that,
; CPU will automatically switch back to ESP3 and SS3 after 'iret'.
; 
; Remeber, when interrupt occurs, we are in the middle of some code in
; user space or kernel space. in order to go back there, ISRs have to 
; save current procedure state before it gets corrupted. 
; those registers are:
; segment registers: DS, FS, ES, GS (CPU already saved CS)
; generic registers: EAX ECX EDX EBX ESP EBP ESI and EDI
; those registers are context of a thread, that's where we do the 
; task switching.
; after those registers saved and environment is ready, ISR should
; do its work, but most important thing to do is to acknowledge 
; interrupt by sending 'EOI' command to PIC.
; 
; after doing all its work there should be clean return from interrupt,
; that will restore the state of interrupted procedure (popa, restore
; data segment), enable interrupts (sti) that were disabled by CPU
; before entering ISR
; 

extern main_cxx_isr

; we cannot use C/C++ to write functions which using 'iret' to return to 
; callers. that's the reason we are writing those intr_?_entry.

section .data

; declare isr_tbl as 'global'
global isr_tbl

; isr_tbl saves all ISRs
; it's just a table filled with 'function pointers'
isr_tbl:
%macro MAKE_ISR 2

; code section
section .text
isr_%1:
    ; some interrupt has a parameter (error code), others don't.
    ; we pad a dummy parameter if no parameter passed in
    ; to make stack aligned.
    ; this %2 isn't always pushing a dummy parameter
    %2          ; push first parameter #1
    push    ds
    push    es
    push    fs
    push    gs
    pushad  ; backup EAX ECX EDX EBX ESP EBP ESI and EDI

    mov     al, 0x20 ; EOI(End of Interrupt) signal
    out     0xa0, al ; send to 8259A master port
    out     0x20, al ; send to 8259A slave port

    push    %1 ; push second parameter

    call    main_cxx_isr

    ; all entries share same exit code.
    jmp     isr_exit

; we uglily defined isr_%1 here because we need to keep
; this code segment inside the macro.
section .data
    dd      isr_%1 ; interrupt service routine
%endmacro

; code section
section     .text

; all ISRs share same exit code
global      restore_kstack
isr_exit:
restore_kstack:
    add     esp, 4 ; pop second parameter
    popad   ; pop EDI ESI EBP ESP EBX EDX ECX and EAX
    pop     gs
    pop     fs
    pop     es
    pop     ds
    add     esp, 4 ; pop first parameter
    iret
; Note that the iret instruction restores the state of EFLAGS before
; the interrupt handler began, thus allowing further interrupts to
; occur after the interrupt handler is complete.



; use macros to define ISRs
; those are not real handlers
MAKE_ISR 0x00,ZERO
MAKE_ISR 0x01,ZERO
MAKE_ISR 0x02,ZERO
MAKE_ISR 0x03,ZERO 
MAKE_ISR 0x04,ZERO
MAKE_ISR 0x05,ZERO
MAKE_ISR 0x06,ZERO
MAKE_ISR 0x07,ZERO 
MAKE_ISR 0x08,ERROR_CODE
MAKE_ISR 0x09,ZERO
MAKE_ISR 0x0A,ERROR_CODE
MAKE_ISR 0x0B,ERROR_CODE 
MAKE_ISR 0x0C,ZERO
MAKE_ISR 0x0D,ERROR_CODE
MAKE_ISR 0x0E,ERROR_CODE
MAKE_ISR 0x0F,ZERO 
MAKE_ISR 0x10,ZERO
MAKE_ISR 0x11,ERROR_CODE
MAKE_ISR 0x12,ZERO
MAKE_ISR 0x13,ZERO 
MAKE_ISR 0x14,ZERO
MAKE_ISR 0x15,ZERO
MAKE_ISR 0x16,ZERO
MAKE_ISR 0x17,ZERO 
MAKE_ISR 0x18,ERROR_CODE
MAKE_ISR 0x19,ZERO
MAKE_ISR 0x1A,ERROR_CODE
MAKE_ISR 0x1B,ERROR_CODE 
MAKE_ISR 0x1C,ZERO
MAKE_ISR 0x1D,ERROR_CODE
MAKE_ISR 0x1E,ERROR_CODE
MAKE_ISR 0x1F,ZERO 
MAKE_ISR 0x20,ZERO ; timer
MAKE_ISR 0x21,ZERO ; keyboard
MAKE_ISR 0x22,ZERO ; cascade
MAKE_ISR 0x23,ZERO ; serial port 2
MAKE_ISR 0x24,ZERO ; serial port 1
MAKE_ISR 0x25,ZERO ; parallel port 2
MAKE_ISR 0x26,ZERO ; floppy
MAKE_ISR 0x27,ZERO ; parallel port 1
MAKE_ISR 0x28,ZERO ; real time clock
MAKE_ISR 0x29,ZERO ; redirect
MAKE_ISR 0x2A,ZERO ; reserved
MAKE_ISR 0x2B,ZERO ; reserved
MAKE_ISR 0x2C,ZERO ; ps/2
MAKE_ISR 0x2D,ZERO ; fpu exception
MAKE_ISR 0x2E,ZERO ; hard disk
MAKE_ISR 0x2F,ZERO ; reserved

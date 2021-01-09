[bits 32]

%define ERROR_CODE nop
%define ZERO push 0

; extern .init_array

; section .text
; global my_init;
; my_init:
;     call .init_array
;     ret

extern main_intr_handler

; we cannot use C/C++ write functions which use 'iret' to return to 
; callers. that's the reason we are writing those intr_?_entry.

section .data

; declare intr_entry_table as 'global'
global intr_entry_table

; intr_entry_table saves all interrupt handlers
; it's just a table filled with 'function pointers'
intr_entry_table:
%macro INTR_VECTOR 2

; code section
section .text
intr_%1_entry:
    ; some interrupts have a parameter, others don't
    ; we pad a dummy parameter if no parameter passed in
    ; to make stack aligned.
    ; so, this %2 isn't always pushing a dummy parameter
    %2          ; push first parameter #1
    push    ds
    push    es
    push    fs
    push    gs
    pushad  ; backup EAX ECX EDX EBX ESP EBP ESI and EDI

    mov     al, 0x20 ; EOI(End of Interrupt) signal
    out     0xa0, al ; send to 8259A master port
    out     0x20, al ; send to 8259A slave port
    ;mov     ebx, 0xb8000
    ;mov     byte [ebx], 0x48
    push    %1 ; push second parameter
    call    main_intr_handler
    ; [idt_table + %1 * 4] ; call real interrupt handler

    ; all entries share same exit code.
    jmp     intr_exit

; we uglily defined intr_%1_entry here because we need to keep
; this code segment inside the macro.
section .data
    dd      intr_%1_entry ; interrupt entry point
%endmacro

; code section
section     .text

; all interrupt handlers share same exit code
global      intr_exit
intr_exit:
    add     esp, 4 ; pop second parameter
    popad   ; pop EDI ESI EBP ESP EBX EDX ECX and EAX
    pop     gs
    pop     fs
    pop     es
    pop     ds
    add     esp, 4 ; pop first parameter
    iret

; use macros to define interrupt handlers
; those are not real handlers
INTR_VECTOR 0x00,ZERO
INTR_VECTOR 0x01,ZERO
INTR_VECTOR 0x02,ZERO
INTR_VECTOR 0x03,ZERO 
INTR_VECTOR 0x04,ZERO
INTR_VECTOR 0x05,ZERO
INTR_VECTOR 0x06,ZERO
INTR_VECTOR 0x07,ZERO 
INTR_VECTOR 0x08,ERROR_CODE
INTR_VECTOR 0x09,ZERO
INTR_VECTOR 0x0a,ERROR_CODE
INTR_VECTOR 0x0b,ERROR_CODE 
INTR_VECTOR 0x0c,ZERO
INTR_VECTOR 0x0d,ERROR_CODE
INTR_VECTOR 0x0e,ERROR_CODE
INTR_VECTOR 0x0f,ZERO 
INTR_VECTOR 0x10,ZERO
INTR_VECTOR 0x11,ERROR_CODE
INTR_VECTOR 0x12,ZERO
INTR_VECTOR 0x13,ZERO 
INTR_VECTOR 0x14,ZERO
INTR_VECTOR 0x15,ZERO
INTR_VECTOR 0x16,ZERO
INTR_VECTOR 0x17,ZERO 
INTR_VECTOR 0x18,ERROR_CODE
INTR_VECTOR 0x19,ZERO
INTR_VECTOR 0x1a,ERROR_CODE
INTR_VECTOR 0x1b,ERROR_CODE 
INTR_VECTOR 0x1c,ZERO
INTR_VECTOR 0x1d,ERROR_CODE
INTR_VECTOR 0x1e,ERROR_CODE
INTR_VECTOR 0x1f,ZERO 
INTR_VECTOR 0x20,ZERO ; timer
INTR_VECTOR 0x21,ZERO ; keyboard
INTR_VECTOR 0x22,ZERO ; cascade
INTR_VECTOR 0x23,ZERO ; serial port 2
INTR_VECTOR 0x24,ZERO ; serial port 1
INTR_VECTOR 0x25,ZERO ; parallel port 2
INTR_VECTOR 0x26,ZERO ; floppy
INTR_VECTOR 0x27,ZERO ; parallel port 1
INTR_VECTOR 0x28,ZERO ; real time clock
INTR_VECTOR 0x29,ZERO ; redirect
INTR_VECTOR 0x2a,ZERO ; reserved
INTR_VECTOR 0x2b,ZERO ; reserved
INTR_VECTOR 0x2c,ZERO ; ps/2
INTR_VECTOR 0x2d,ZERO ; fpu exception
INTR_VECTOR 0x2e,ZERO ; hard disk
INTR_VECTOR 0x2f,ZERO ; reserved

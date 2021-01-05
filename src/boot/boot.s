; ----------------------------------------------------------------------------
; Hello XOS, a simple bootable image
[bits 16]
    org     07c00h

    cli     ; turn off interrupt

    mov     ax, cs
    mov     ds, ax
    mov     es, ax

    call    clear_screen
    mov     bx, hello_xos
    call    print_str

    jmp     $
; ----------------------------------------------------------------------------

clear_screen:
; parameters:
; no parameter needed.
    pusha
    mov     ah, 0x06 ; option 06
    mov     al, 0h ; clear screen
    mov     ch, 0 ; row of upper left corner of scroll window
    mov     cl, 0 ; column of upper left corner of scroll window
    mov     dh, 24 ; row of lower right corner of scroll window
    mov     dl, 79 ; column of lower right corner of scroll window
    mov     bh, 00000111b ; color attributes, black background and white character color
    int     0x10 ; execute
    mov     dx, 0 ; set cursor position to top-left
    call    move_cursor_pos
    popa
    ret

print_str:
; parameters
; bx (address of string)
; string must end with 0
    pusha
	mov     ah, 0x0e ; Write text in teletype mode
.loop:
	mov     al, [bx] ; put a 'char' into 'al'
	cmp     al, 0 ; check if 'al' is '0'
	je      .out ; end loop
	int     0x10 ; print
	add     bx, 0x01 ; next char
	jmp     .loop
.out:
    popa
	ret

move_cursor_pos:
; parameters:
; dh x-coord (row), range: 0-24, 
; dl y-coord (column), range: 0-79 
    pusha
    mov     ah, 0x02 ; option 2, set cursor position
    mov     bh, 0 ; page 0
    int     0x10 ; execute
    popa
    ret

; ----------------------------------------------------------------------------
; Tail Part
hello_xos:
    db "Hello XOS!", 0 ; define the string

    times 510-($-$$) db 0 ; fill up with zeroes
    dw 0Xaa55 ; signature
; ----------------------------------------------------------------------------

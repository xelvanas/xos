; ----------------------------------------------------------------------------
; PREDEFINITIONs:
; GDT segment index
SYS_CODE_SEGMENT equ 0x08
SYS_DATA_SEGMENT equ 0x10
USR_CODE_SEGMENT equ 0x18
USR_DATA_SEGMENT equ 0x20

; OLD SOLUTION:
; LOADER_MODULE_ADDRESS           equ 0x0500
; LOADER_MODULE_SECTOR_COUNT      equ 59
; Q: why does 'loader module' start at 0x500 and 59 sectors??
;
; A: 1. partitions normally start and stop at cylinder boundaries. a disk
;    track normally has 63 sectors. since the first sector used by 'mbr',
;    partition 0 can't contain the first track. which leaves up 62 available
;    sectors (62*512 = 31744 bytes = 31 KB).
;
;    2. the 'real mode' IVT(interrupt vector table) used 0x0000-0x03ff (1KB),
;    BDA (BIOS Data Area) used 0x0400-0x04ff and MBR will be loaded at 0x7c00.
;    there's a gap between 0x0500-0x7bff. that's 0x76ff (30463 ≈ 29.7 KB)
;    bytes available space.
;
;    3. we cannot load whole 31 KB data into 29.7 KB memory space, that's
;    why we choose only to load 59 sectors (30208 bytes = 29.5 KB) data.
;
; NEW SOLUTION:
; Q: why do we change this?
;
; A: 1. the first reason is that 29.7KB is NOT a big space to store loaders.
;       I encountered a couple of weired bugs while developing Loader. for 
;       example: some static global variables have 0 values. I spent a lot of
;       time to figure out what happend. It came out that my boot.bin(on MBR)
;       only loaded 59 sectors. no real '.data sections' loaded into memory.
;       trust me, that's an awful experience.
;
;    2. the OLD Hard Disk Drivers(HDD) do have disk inside, each disk has 63
;       sectors per track. Solid State Disks(SSD) do NOT have same limit, many 
;       tools make partitions aligned on 1MB. for a disk with 512 bytes per
;       sector this equates to 2048 sector alignment. there're 2047 free
;       sectors between 'MBR' and 'first Partition'. it's really a big space
;       for a loader.
;
;    3. in 'Kernel' stage, 'Loader' is no longer used. It seems to store 
;       'Loader' in a higher address would be a good choice. because it can be 
;       safely replaced later. and in most cases, 0x00100000 is the starting
;       address of the biggest valid memory block. I decide to read 'Loader'
;       File at 0x00500000, then load "Loader" at 0x00300000.
LOADER_FILE_ADDRESS             equ 0x00500000
LOADER_MODULE_ADDRESS           equ 0x00300000
LOADER_MODULE_SECTOR_COUNT      equ 2047

; ----------------------------------------------------------------------------
; BOOT MODULE
[bits 16]
    org     07c00h ; tell compiler where the code will be loaded.
    cli            ; disable interrupt

    mov     ax, cs ; set segment registers
    mov     ds, ax ;
    mov     es, ax ;
    call    e820_detect_memory
    call    clear_screen

    mov     dx, 0
    call    move_cursor_pos
    
    ; ENABLE A20 LINE
    ; wait til 8042 input buffer empty
wait_8042_buf_empty:
    in      al, 0x64 ; read a byte from 8042 status register into al register
    test    al, 2  ; test the second bit
    jnz     wait_8042_buf_empty
    mov     al, 0xd1    ; store command value 0xd1 in 'al'
    out     0x64, al

wait_8042_buf_empty_again:
    in      al, 0x64 ; read a byte from 8042 status register into al register
    test    al, 2  ; test the second bit
    jnz     wait_8042_buf_empty_again
    mov     al, 0xdf    ; save value in 'al'
    out     0x60, al    ; send 'al' to 0x60 data port

    ; ENABLE GDT
    ; GDTR is a register to store GDT address and size.
    lgdt    [GLOBAL_DESCRIPTOR_TABLE_INFO]

    ; enable 32 bit protected mode
    mov     eax, cr0    ; we cannot change value of cr0 directly
    or      al, 1       ; cr0.PM first bit is responsible for protected mode
    
    ; Privileged instruction, cr0.PM = 1, enable PROTECTED MODE
    mov     cr0, eax

    ; jump into PROTECTED MODE
    ; 1. jmp will cause 'cs' reloaded.
    ; 2. branch prediction will be cleared.
    jmp     dword SYS_CODE_SEGMENT:protected_mode_code

; PROTECTED MODE (32-Bit code)
[bits 32]
protected_mode_code:
    ; data segment descriptor is the third entry of GDT, offset is 16 byte
    mov     ax, 0x0010  
    mov     ds, ax      ; update data segment
    mov     ss, ax
    mov     es, ax
    mov     ax, 0x0000
    mov     fs, ax
    mov     gs, ax

    ; change stack address 
    mov     ax, 0x10
    mov     ss, ax
    mov     eax, 0x9f000
    mov     esp, eax

    call    read_first_2047_sectors

; PARSE ELF 
kernel_init:
    xor     eax, eax
    xor     ebx, ebx
    xor     ecx, ecx
    xor     edx, edx
    ; read e_phentsize
    ; the size in bytes of one entry in the file's program header table
    mov     dx,  [LOADER_FILE_ADDRESS + 42] ; e_phentsize
    ; read e_phoff
    ; the program header table's file offset in bytes
    mov     ebx, [LOADER_FILE_ADDRESS + 28] ; e_phoff
    ; real address of program header table
    add     ebx,  LOADER_FILE_ADDRESS
    ; e_phnum
    ; the number of entries in the program header table.
    mov     cx,  [LOADER_FILE_ADDRESS + 44] ; e_phnum
.each_segment:
    cmp     byte [ebx + 0], 0 ; PT_NULL
    je      .PTNULL ; program header is NULL, skip it.
    ; p_filesz (size)
    ; the number of bytes in the file image of the segment. It may be zero.
    push    dword [ebx + 16] ; p_filesz
    ; p_offset (source)
    ; the offset from the beginning of the file at which the first byte of
    ; the segment resides.
    mov     eax, [ebx + 4] ; p_offset
    ; where to read program header table entry
    add     eax, LOADER_FILE_ADDRESS
    push    eax
    ; read p_vaddr (dest)
    ; the virtual address at which the first byte of the segment resides
    ; in memory.
    push    dword [ebx + 8] ; p_vaddr
    call    mem_cpy
    add     esp, 12
.PTNULL:
    add     ebx, edx
    loop    .each_segment
    ; the virtual address to which the system first transfers control, thus
    ; starting the process.
    mov     eax, [LOADER_FILE_ADDRESS + 24] ; e_entry
    jmp     eax

    ; should NOT get there
    jmp     $   ; infinite loop
; end of BOOT MODULE
; ----------------------------------------------------------------------------



; ----------------------------------------------------------------------------
; 16-Bit FUNCTIONs
[bits 16]

# -------------------------------------
; FUNCTION: e820_detect_memory
e820_detect_memory:
    pusha
; point ES:DI at ards_buffer
    ; back ds:si es:di
    push    es
    mov     ax, 0x9000
    mov     es, ax
    mov     di, 0xf000
; clear ebx
    xor     ebx, ebx
; set magic number
    mov     edx, 0x534D4150
; set command
.again:
    mov     eax, 0xE820
; set size of ards entry
    mov     ecx, 20
; excute
    int     0x15
; jump if 'carry flag' is set
    jc      .failed
    add     di, cx  ; di+20
    inc     byte [es:0xf104]
    cmp     ebx, 0
    jnz     .again ; 
; successed
    ; mov     bx, msg_e820_successed
    ; call    print_string
    jmp     .end
; end of detecting
.failed:
    ; mov     bx, msg_e820_failed
    ; call    print_string
.end:
    pop     es
    popa
    ret
; end of FUNCTION: e820_detect_memory
# -------------------------------------

# -------------------------------------
; FUNCTION: clear_screen
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
    ; color attributes, black background and white character color
    mov     bh, 00000111b
    int     0x10 ; execute
    mov     dx, 0 ; set cursor position to top-left
    call    move_cursor_pos
    popa
    ret
; end of FUNCTION: clear_screen
# -------------------------------------


# -------------------------------------
; FUNCTION: move_cursor_pos
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
; end of FUNCTION: move_cursor_pos
# -------------------------------------


# -------------------------------------
; FUNCTION: print_string
print_string:
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
; end of FUNCTION: print_string
# -------------------------------------

; end of 16-Bit FUNCTIONs
; ----------------------------------------------------------------------------



; ----------------------------------------------------------------------------
; 32-Bit FUNCTIONs
[bits 32]


# -------------------------------------
; FUNCTION: mem_cpy
; [ebx + 00] = ecx
; [ebx + 04] = ebp
; [ebx + 08] = dst address : parameter 1
; [ebx + 12] = src address : parameter 2
; [ebx + 16] = length      : parameter 3
mem_cpy:
    cld
    push    ebp
    mov     ebp, esp
    push    ecx
    mov     edi, [ebp + 8]
    mov     esi, [ebp + 12]
    mov     ecx, [ebp + 16]
    rep     movsb
    pop     ecx
    pop     ebp
    ret
; end of FUNCTION: mem_cpy
# -------------------------------------


# -------------------------------------
; FUNCTION: read_first_2047_sectors
read_first_2047_sectors:
    xor     eax, eax
    xor     edx, edx
    ; Careful: first index of LBA28 is ZERO(0).
    ; Don't be confused with CHS, which first sector index is 1
    mov     eax, 1 ; read from LBA
    mov     ecx, 7 ; read 7*256 = 1792 sectors
    mov     edi, LOADER_FILE_ADDRESS ; where to store the file
.read_1792:
    push    eax
    push    ecx
    mov     ecx, 0
    call    read_sectors
    pop     ecx
    pop     eax
    add     eax, 256
    loop    .read_1792
    ; read last 255 sectors
    ; 1MBR + 7*256 + 254 = 2047
    mov     ecx, 255 ; read last 255 sectors
    call    read_sectors
    ret
; end of FUNCTION: read_first_2047_sectors
# -------------------------------------


# -------------------------------------
; FUNCTION: read_sectors
; Parameters:
;   cx: how many sectors to read, range: 0-255, 0 indicates 256
;   eax: where to read data, LBA-28 Address
;   edi: where to store data
read_sectors: 
    mov     ebx, eax ; backup eax
    ; write 0x1F1 
    xor     eax, eax
    mov     dx, 0x1F1 ; standard procedure
    out     dx, al
    ; write 0x1F2, how many sectors do you want to read.
    mov     ax, cx
    inc     dx
    out     dx, al ; value: 0-255, 0 indicates 256 sectors
    ; LBA 0-23 bits
    mov     eax, ebx ; restore eax 
    ; write 00-07 bits 
    inc     dx     ; dx = 0x1F3
    out     dx, al ; LBA 00-07 Bits
    ; write 08-15 bits
    inc     dx     ; dx = 0x1F4
    shr     eax, 8
    out     dx, al ; LBA 08-15 Bits
    ; write 16-23 bits
    inc     dx     ; dx = 0x1F5
    shr     eax, 8
    out     dx, al ; LBA 16-23 Bits
    ; write 'Device Register' (0x1F6, 0x176)
    inc     dx     ; dx = 0x1F6
    shr     eax, 8
    and     al, 0x0F
    ; select 'LBA mode', 'master device', and fill LBA 24-27
    or      al, 01000000b ;
    out     dx, al
    ; write 0x1F7, 'Command Register' (0x20 indicates 'read')
    inc     dx     ; dx = 0x1F7
    mov     al, 0x20
    out     dx, al
; read 0x1F7, status register
.disk_data_not_ready:
    nop     ; sleep for a while
    in      al, dx ; dx = 0x1F7
    and     al, 10001000b ; keep BSY and DRQ
    cmp     al, 00001000b ; check if 'BSY' == 0 && 'DRQ' == '1'
    jnz     .disk_data_not_ready ; jump iff 'BSY=0 && DRQ=1'
; read data from data register (port 0x1F0)
; mov     bx, MEMORY_ADDRESS_TO_STORE_DATA ; already done by passing parameter
; mov     cx, READ_SECTOR_COUNT * 256 ; sector*count/sizeof(word)
; cx is sector count, we need word count here
; 1 sector = 512 bytes = 256 words
; 1 sector = 512 bytes = 128 dwords
    cmp     cx, 0
    jne     .sector_not_zero
    mov     cx, 256
.sector_not_zero:
    ; shift left 7 bit
    shl     ecx, 7
    mov     dx, 0x1F0
    rep     insd
    ret ;
; end of FUNCTION: read_sectors
# -------------------------------------


; end of 32-Bit FUNCTIONs
; ----------------------------------------------------------------------------


; ----------------------------------------------------------------------------
; GDT
align 16

GLOBAL_DESCRIPTOR_TABLE_START:
GDT_NULL_ENTRY:
    dd      0x00000000   ; dd means define double words (4 bytes, 32 bits)
    dd      0x00000000
; system code segment (DPL = 0)
    dw      0xffff  ; segment limit (bits 0-15)
    dw      0x0000  ; base (16 bits: 0-15)
    db      0x00    ; base (8 bits: 16-23)
    db      10011010b   ; first flags, type flags
    db      11001111b   ; second flags and limit (4 bits)
    db      0x00        ; base (8 bits: 24-31)
; system data segment (DPL = 0)
    dw      0xffff  ; segment limit (bits 0-15)
    dw      0x0000  ; base (16 bits: 0-15)
    db      0x00    ; base (8 bits: 16-23)
    db      10010010b   ; first flags, type flags
    db      11001111b   ; second flags and limit (4 bits)
    db      0x00        ; base (8 bits: 24-31)
; user code segment (DPL = 3)
    dw      0xffff  ; segment limit (bits 0-15)
    dw      0x0000  ; base (16 bits: 0-15)
    db      0x00    ; base (8 bits: 16-23)
    db      11111010b   ; first flags, type flags
    db      11001111b   ; second flags and limit (4 bits)
    db      0x00        ; base (8 bits: 24-31)
; user data segment (DPL = 3)
    dw      0xffff  ; segment limit (bits 0-15)
    dw      0x0000  ; base (16 bits: 0-15)
    db      0x00    ; base (8 bits: 16-23)
    db      11110010b   ; first flags, type flags
    db      11001111b   ; second flags and limit (4 bits)
    db      0x00        ; base (8 bits: 24-31)

; tss descriptor
    dd      0x00000000
    dd      0x00000000
GLOBAL_DESCRIPTOR_TABLE_END:

GLOBAL_DESCRIPTOR_TABLE_INFO:
    ; size of GDT, always less one of the true size
    dw GLOBAL_DESCRIPTOR_TABLE_END - GLOBAL_DESCRIPTOR_TABLE_START - 1
    ; start address of gdt
    dd GLOBAL_DESCRIPTOR_TABLE_START
; end of GDT
; ----------------------------------------------------------------------------


; ----------------------------------------------------------------------------
; 
DISK_ERROR:
	db 'Disk Error!', 0
; ----------------------------------------------------------------------------

    times 510-($-$$) db 0 ; fill up with zeroes
    dw 0Xaa55 ; signature which indicates it's bootable
; ----------------------------------------------------------------------------

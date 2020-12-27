; z88dk/bin/z80asm.exe --cpu=z80 --list -b bootstrap.z80
; z88dk/bin/appmake.exe +hex -b bootstrap.bin -o bootstrap.hex --org 0x8000


defc VMPR = 252
defc HMPR = 251
defc LMPR = 250

; The ROM normally keeps the machine stack and system variables in section B at
; 4000H-5CD0H, so that they are not paged out when the screen, Basic program or variables area
; are paged in in the top half of memory.

; Page
; 0 = 0x00000
; 1 = 0x04000
; 2 = 0x08000
; 3 = 0x0c000
; 4 = 0x10000
defc DEST_PAGE = 4

DEFC payload_dest = 0x0000

    org 0x8000

    di

 	ld (sp_store),sp		;save sp
 	ld sp, temp_stack_top

; page in 4 to LOWER
    in a, (LMPR)
    ld (lmpr_store), a
    and 0xe0
    or DEST_PAGE
    or 0b00100000      ; Set Bit 5 which, when loaded into the LMPR register...
                       ; ... will Set the RAM in Page 0
    out (LMPR), a

; copy code to LOWER
    ld hl, payload
    ld de, payload_dest
    ld bc, payload_end - payload
    ldir

    call payload_dest

    ; hopefully return here!
    ld a, (lmpr_store)
    out (LMPR), a

    ld sp, (sp_store)
    ret;

sp_store:
    DEFW 0		;memory to save sp

lmpr_store:
    DEFB 0

temp_stack:
    DEFW 0
    DEFW 0
    DEFW 0
    DEFW 0

    DEFC temp_stack_top = $

payload:
    BINARY "payload.rom"
    
    DEFC payload_end = $
    
    

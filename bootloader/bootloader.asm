
; This is an initial bootloader for the emulator
; The largest the bootloader can be is 128 bytes.
; This means a maximum of 32 instructions.

; The processor starts execution at address 2048

; Assembly code
MOV  R1  8h    ; Number of bytes to read from serial port
MOV  R0  880h  ; Location that the 8 bytes will be written to
SYSCALL  2h
LD   R1  R0    ; Load the number of operands to read from serial port
MUL  R1  4h    ; Get number of bytes from number of operands
MOV  R0  A00h  ; Location that values will be read to
SYSCALL  2h
MOV  R1  8h
LD   R0  RIP
ADD  R0  10h
SYSCALL  1h    ; Output "Booted" to the serial port
MOV  R2  A00h
LD   R3  R2
JMP  R3        ; Jump to loaded code
"Booted"

; Assembly code compiled to numerical codes
Op |IT|CF|
07h 1h 0h 1h 8h
07h 1h 0h 0h 880h
02h 2h 0h 2h
08h 0h 0h 1h 0h 0h
05h 1h 0h 1h 4h
07h 1h 0h 0h A00h
02h 2h 0h 2h
07h 1h 0h 1h 8h
08h 1h 0h 0h 1h
03h 1h 0h 0h 10h
02h 2h 0h 1h
07h 1h 0h 2h A00h
08h 0h 0h 3h 2h 0h
13h 0h 0h 3h 0h 0h
"Booted\0"

; Assembly Bytes
00080887
08800887
00004102
00000808
00040885
0A000087
00004102
00080887
00100083
00002102
0A001087
00021808
00001813
424F4F54
45440000

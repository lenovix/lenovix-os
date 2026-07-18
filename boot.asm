; Standar Multiboot agar bisa dibaca oleh emulator/bootloader
MODULEALIGN equ  1 << 0
MEMINFO     equ  1 << 1
FLAGS       equ  MODULEALIGN | MEMINFO
MAGIC       equ  0x1BADB002
CHECKSUM    equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .bss
align 16
stack_bottom:
    resb 16384 ; Menyediakan stack sebesar 16KB
stack_top:

section .text
global _start
extern kernel_main

_start:
    mov esp, stack_top   ; Set up stack pointer
    call kernel_main     ; Panggil fungsi di kernel.c
    
    cli
.hang:                   ; Jika kernel selesai, komputer diam di sini
    hlt
    jmp .hang

global inb
inb:
    mov dx, [esp + 4] ; Ambil argumen pertama (nomor port) dari stack
    in al, dx         ; Baca byte dari port DX dan simpan ke AL
    ret               ; Kembali ke fungsi C, mengembalikan nilai di AL

global outb
outb:
    mov dx, [esp + 4] ; Ambil argumen pertama (nomor port) dari stack
    mov al, [esp + 8] ; Ambil argumen kedua (data byte) dari stack
    out dx, al        ; Kirim data di AL ke port DX
    ret
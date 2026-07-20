MBALIGN     equ  1 << 0                   ; Align loaded modules
MEMINFO     equ  1 << 1                   ; Provide memory map
GRAPHICS    equ  1 << 2                   ; Request video mode
FLAGS       equ  MBALIGN | MEMINFO | GRAPHICS
MAGIC       equ  0x1BADB002
CHECKSUM    equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
    
    ; Address fields (Diisi 0 jika pakai ELF format)
    dd 0, 0, 0, 0, 0
    
    ; Graphic Mode Request
    dd 0                          ; Mode control (0 = Linear Graphics Mode)
    dd 1024                       ; Width
    dd 768                        ; Height
    dd 32                         ; Depth (BPP)

section .text
global _start
extern kernel_main

_start:
    cli
    mov esp, stack_top
    
    ; Push Multiboot Info Structure Pointer (EBX) & Magic Number (EAX) ke Kernel
    push ebx
    push eax

    call kernel_main

.hang:
    hlt
    jmp .hang

; =========================================================================
; SEKSI .BSS (HANYA UNTUK ALOKASI STACK/DATA TANPA INISIALISASI)
; =========================================================================
section .bss
align 16
stack_bottom:
    resb 16384 ; 16 KB Stack
stack_top:

; =========================================================================
; KEMBALI KE SEKSI .TEXT (UNTUK KODE INSTRUKSI KERNEL/HELPER ASSEMBLY)
; =========================================================================
section .text

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

global shutdown_qemu
shutdown_qemu:
    mov dx, 0x604
    mov ax, 0x2000
    out dx, ax
    
    cli
.halt:
    hlt
    jmp .halt

global load_idt
load_idt:
    mov edx, [esp + 4] ; Ambil argumen pertama (alamat IDT pointer) dari stack
    lidt [edx]         ; Jalankan instruksi Load IDT
    sti                ; Aktifkan interupsi global
    ret

extern keyboard_handler_c

global keyboard_handler_asm
keyboard_handler_asm:
    pusha              ; Simpan semua register umum ke stack
    call keyboard_handler_c ; Panggil fungsi utama driver keyboard
    popa               ; Kembalikan semua register
    iret               ; Interrupt Return

extern default_handler_c

global default_handler_asm
default_handler_asm:
    pusha
    call default_handler_c
    popa
    iret

global gdt_flush
gdt_flush:
    mov edx, [esp + 4] ; Ambil argumen pertama (alamat GDT pointer) dari stack
    lgdt [edx]         ; Jalankan instruksi Load GDT

    jmp 0x08:.reload_segments

.reload_segments:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret

extern timer_handler_c

global timer_handler_asm
timer_handler_asm:
    pusha               ; Simpan semua register umum
    call timer_handler_c ; Panggil fungsi C untuk memproses detak timer
    popa                ; Kembalikan semua register
    iret                ; Selesai interupsi

global syscall_handler
extern isr_syscall_handler

syscall_handler:
    pusha               ; Simpan EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX
    push ebx            ; Argumen 2 C
    push eax            ; Argumen 1 C
    call isr_syscall_handler
    add esp, 8          ; Clean up 2 argumen dari stack
    popa                ; Kembalikan register
    iret                ; Return dari interupsi

global switch_to_task

; void switch_to_task(unsigned int *old_esp, unsigned int new_esp);
switch_to_task:
    pushfd              ; Simpan EFLAGS
    push cs             ; Simpan Code Segment
    pusha               ; Simpan EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI

    mov eax, [esp + 44] ; Ambil argumen pertama (old_esp pointer)
    mov [eax], esp      ; Simpan nilai ESP saat ini ke *old_esp

    mov edx, [esp + 48] ; Ambil argumen kedua (new_esp)
    mov esp, edx        ; Pindahkan ESP CPU ke Stack Task Baru!

    popa                ; Pop 8 general registers
    iret                ; Jump & Restore EIP, CS, EFLAGS

section .note.GNU-stack noalloc noexec nowrite progbits
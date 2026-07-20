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
    push ebx                ; Push alamat Multiboot Info sebagai argumen pertama kernel_main
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

global shutdown_qemu
shutdown_qemu:
    ; Mengirim sinyal shutdown ke port ACPI bawaan QEMU/VirtualBox
    mov dx, 0x604
    mov ax, 0x2000
    out dx, ax
    
    ; Jika gagal (tidak di emulator), matikan gangguan dan hentikan CPU
    cli
.halt:
    hlt
    jmp .halt

global load_idt
load_idt:
    mov edx, [esp + 4] ; Ambil argumen pertama (alamat IDT pointer) dari stack
    lidt [edx]         ; Jalankan instruksi Load IDT
    sti                ; Aktifkan interupsi global (Set Interrupt Flag)
    ret

; Beritahu Assembly bahwa fungsi C ini ada di file lain
extern keyboard_handler_c

global keyboard_handler_asm
keyboard_handler_asm:
    pusha               ; Simpan semua register umum (EAX, ECX, dll) ke stack
    
    call keyboard_handler_c ; Panggil fungsi utama driver keyboard di kernel.c
    
    popa                ; Kembalikan semua register ke kondisi semula
    iret                ; Interrupt Return (perintah khusus untuk menyudahi interupsi)

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

    ; Lakukan Far Jump untuk memperbarui register CS (Code Segment) ke 0x08
    jmp 0x08:.reload_segments

.reload_segments:
    ; Perbarui semua register data segment ke 0x10
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

    push ebx            ; Argumen 2 C (arg1 = pointer ke msg)
    push eax            ; Argumen 1 C (syscall_num = 1)
    call isr_syscall_handler
    add esp, 8          ; Clean up 2 argumen dari stack

    popa                ; Kembalikan register
    iret                ; Return dari interupsi

global switch_to_task

; void switch_to_task(unsigned int *old_esp, unsigned int new_esp);
switch_to_task:
    ; 1. Simpan konteks task saat ini ke stack
    pushfd              ; Simpan EFLAGS
    push cs             ; Simpan Code Segment
    ; (EIP disimpan otomatis saat 'call')
    pusha               ; Simpan EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI

    ; 2. Simpan ESP lama ke *old_esp
    mov eax, [esp + 44] ; Ambil argumen pertama (old_esp pointer)
    mov [eax], esp      ; Simpan nilai ESP saat ini ke *old_esp

    ; 3. Ambil ESP baru dari argumen kedua (new_esp)
    mov edx, [esp + 48] ; Ambil argumen kedua (new_esp)
    mov esp, edx        ; Pindahkan ESP CPU ke Stack Task Baru!

    ; 4. Restore register task baru dari stack barunya
    popa                ; Pop 8 general registers
    iret                ; Jump & Restore EIP, CS, EFLAGS (Task Baru Berjalan!)
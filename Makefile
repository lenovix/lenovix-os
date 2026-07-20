CC = x86_64-linux-gnu-gcc
AS = nasm
LD = x86_64-linux-gnu-ld

# -I src/include memberitahu GCC lokasi folder header (.h)
CFLAGS = -m32 -c -ffreestanding -O2 -Wall -Wextra -I src/include
LDFLAGS = -m elf_i386 -T linker.ld

# Daftar semua Object File yang akan dibentuk
OBJS = boot.o \
       gdt.o \
       idt.o \
       keyboard.o \
       tty.o \
	   vesa.o \
       vfs.o \
       heap.o \
       kernel.o \
       pmm.o \
       task.o \
       timer.o \
       shell.o

all: lenovix.bin

# Linker Step
lenovix.bin: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o lenovix.bin

# --- Arch / x86 ---
boot.o: src/acrh/x86/boot.asm
	$(AS) -f elf32 $< -o $@

gdt.o: src/acrh/x86/gdt.c
	$(CC) $(CFLAGS) $< -o $@

idt.o: src/acrh/x86/idt.c
	$(CC) $(CFLAGS) $< -o $@

# --- Drivers ---
vesa.o: src/drivers/vesa.c
	$(CC) $(CFLAGS) $< -o $@

keyboard.o: src/drivers/keyboard.c
	$(CC) $(CFLAGS) $< -o $@

tty.o: src/drivers/tty.c
	$(CC) $(CFLAGS) $< -o $@

# --- File System ---
vfs.o: src/fs/vfs.c
	$(CC) $(CFLAGS) $< -o $@

# --- Core Kernel ---
heap.o: src/kernel/heap.c
	$(CC) $(CFLAGS) $< -o $@

kernel.o: src/kernel/kernel.c
	$(CC) $(CFLAGS) $< -o $@

pmm.o: src/kernel/pmm.c
	$(CC) $(CFLAGS) $< -o $@

task.o: src/kernel/task.c
	$(CC) $(CFLAGS) $< -o $@

timer.o: src/kernel/timer.c
	$(CC) $(CFLAGS) $< -o $@

# --- Shell ---
shell.o: src/shell/shell.c
	$(CC) $(CFLAGS) $< -o $@

# --- Clean & Run ---
lenovix.iso: lenovix.bin
	mkdir -p iso/boot/grub
	cp lenovix.bin iso/boot/lenovix.bin
	echo 'set timeout=0' > iso/boot/grub/grub.cfg
	echo 'set default=0' >> iso/boot/grub/grub.cfg
	echo 'menuentry "Lenovix OS" {' >> iso/boot/grub/grub.cfg
	echo '    multiboot /boot/lenovix.bin' >> iso/boot/grub/grub.cfg
	echo '    boot' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	grub-mkrescue -o lenovix.iso iso

clean:
	rm -f *.o lenovix.bin lenovix.iso
	rm -rf iso

run: lenovix.iso
	qemu-system-i386 -cdrom lenovix.iso -vga std
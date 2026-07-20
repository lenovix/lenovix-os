CC = x86_64-linux-gnu-gcc
AS = nasm
LD = x86_64-linux-gnu-ld

CFLAGS = -m32 -c -ffreestanding -O2 -Wall -Wextra
LDFLAGS = -m elf_i386 -T linker.ld

all: lenovix.bin

lenovix.bin: boot.o kernel.o idt.o gdt.o timer.o pmm.o heap.o vfs.o
	x86_64-linux-gnu-ld -m elf_i386 -T linker.ld boot.o kernel.o idt.o gdt.o timer.o pmm.o heap.o vfs.o -o lenovix.bin

vfs.o: vfs.c
	x86_64-linux-gnu-gcc -m32 -c -ffreestanding -O2 -Wall -Wextra vfs.c -o vfs.o

heap.o: heap.c
	x86_64-linux-gnu-gcc -m32 -c -ffreestanding -O2 -Wall -Wextra heap.c -o heap.o

pmm.o: pmm.c
	$(CC) $(CFLAGS) pmm.c -o pmm.o

boot.o: boot.asm
	$(AS) -f elf32 boot.asm -o boot.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) kernel.c -o kernel.o

idt.o: idt.c
	$(CC) $(CFLAGS) idt.c -o idt.o

gdt.o: gdt.c
	$(CC) $(CFLAGS) gdt.c -o gdt.o

timer.o: timer.c
	$(CC) $(CFLAGS) timer.c -o timer.o

clean:
	rm -f *.o lenovix.bin

run: lenovix.bin
	qemu-system-i386 -kernel lenovix.bin
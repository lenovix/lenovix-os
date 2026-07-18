CC = x86_64-linux-gnu-gcc
AS = nasm
LD = x86_64-linux-gnu-ld

CFLAGS = -m32 -c -ffreestanding -O2 -Wall -Wextra
LDFLAGS = -m elf_i386 -T linker.ld

all: lenovix.bin

boot.o: boot.asm
	$(AS) -f elf32 boot.asm -o boot.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) kernel.c -o kernel.o

lenovix.bin: boot.o kernel.o
	$(LD) $(LDFLAGS) boot.o kernel.o -o lenovix.bin

clean:
	rm -f *.o lenovix.bin

run: lenovix.bin
	qemu-system-i386 -kernel lenovix.bin
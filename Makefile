CC = x86_64-linux-gnu-gcc
AS = nasm
LD = x86_64-linux-gnu-ld

# Flags
CFLAGS = -m32 -c -ffreestanding -O2 -Wall -Wextra -I src/include
LDFLAGS = -m elf_i386 -T linker.ld

# Folder Output
BUILD_DIR = build

# Daftar Object File (diarahkan ke $(BUILD_DIR)/)
OBJS = $(BUILD_DIR)/boot.o \
       $(BUILD_DIR)/gdt.o \
       $(BUILD_DIR)/idt.o \
       $(BUILD_DIR)/keyboard.o \
       $(BUILD_DIR)/tty.o \
       $(BUILD_DIR)/vesa.o \
       $(BUILD_DIR)/mouse.o \
       $(BUILD_DIR)/window.o \
       $(BUILD_DIR)/menu.o \
       $(BUILD_DIR)/terminal.o \
       $(BUILD_DIR)/vfs.o \
       $(BUILD_DIR)/heap.o \
       $(BUILD_DIR)/kernel.o \
       $(BUILD_DIR)/pmm.o \
       $(BUILD_DIR)/task.o \
       $(BUILD_DIR)/timer.o \
       $(BUILD_DIR)/shell.o

.PHONY: all clean run

all: $(BUILD_DIR)/lenovix.bin

# Linker Step
$(BUILD_DIR)/lenovix.bin: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

# --- Arch / x86 ---
$(BUILD_DIR)/boot.o: src/arch/x86/boot.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) -f elf32 $< -o $@

$(BUILD_DIR)/gdt.o: src/arch/x86/gdt.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/idt.o: src/arch/x86/idt.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# --- Drivers ---
$(BUILD_DIR)/menu.o: src/drivers/menu.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/window.o: src/drivers/window.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/terminal.o: src/drivers/terminal.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/mouse.o: src/drivers/mouse.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/vesa.o: src/drivers/vesa.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/keyboard.o: src/drivers/keyboard.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/tty.o: src/drivers/tty.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# --- File System ---
$(BUILD_DIR)/vfs.o: src/fs/vfs.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# --- Core Kernel ---
$(BUILD_DIR)/heap.o: src/kernel/heap.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/kernel.o: src/kernel/kernel.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/pmm.o: src/kernel/pmm.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/task.o: src/kernel/task.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/timer.o: src/kernel/timer.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# --- Shell ---
$(BUILD_DIR)/shell.o: src/shell/shell.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# --- ISO & Run ---
lenovix.iso: $(BUILD_DIR)/lenovix.bin
	mkdir -p iso/boot/grub
	cp $(BUILD_DIR)/lenovix.bin iso/boot/lenovix.bin
	echo 'set timeout=0' > iso/boot/grub/grub.cfg
	echo 'set default=0' >> iso/boot/grub/grub.cfg
	echo 'menuentry "Lenovix OS" {' >> iso/boot/grub/grub.cfg
	echo '    multiboot /boot/lenovix.bin' >> iso/boot/grub/grub.cfg
	echo '    boot' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	grub-mkrescue -o lenovix.iso iso

clean:
	rm -rf $(BUILD_DIR) iso lenovix.iso *.o lenovix.bin

run: lenovix.iso
	qemu-system-i386 -cdrom lenovix.iso -vga std
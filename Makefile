CC = x86_64-linux-gnu-gcc
AS = nasm
LD = x86_64-linux-gnu-ld

CFLAGS = -m32 -c -ffreestanding -O2 -Wall -Wextra -I src/include
LDFLAGS = -m elf_i386 -T linker.ld

BUILD_DIR = build

OBJS = $(BUILD_DIR)/boot.o \
       $(BUILD_DIR)/gdt.o \
       $(BUILD_DIR)/idt.o \
       $(BUILD_DIR)/keyboard.o \
       $(BUILD_DIR)/tty.o \
       $(BUILD_DIR)/ata.o \
       $(BUILD_DIR)/pci.o \
       $(BUILD_DIR)/rtl8139.o \
       $(BUILD_DIR)/net.o \
       $(BUILD_DIR)/tcp.o \
       $(BUILD_DIR)/speaker.o \
       $(BUILD_DIR)/sb16.o \
       $(BUILD_DIR)/rtc.o \
       $(BUILD_DIR)/acpi.o \
       $(BUILD_DIR)/bmp.o \
       $(BUILD_DIR)/vesa.o \
       $(BUILD_DIR)/mouse.o \
       $(BUILD_DIR)/window.o \
       $(BUILD_DIR)/menu.o \
       $(BUILD_DIR)/terminal.o \
       $(BUILD_DIR)/vfs.o \
       $(BUILD_DIR)/fat16.o \
       $(BUILD_DIR)/tar.o \
       $(BUILD_DIR)/heap.o \
       $(BUILD_DIR)/kernel.o \
       $(BUILD_DIR)/pmm.o \
       $(BUILD_DIR)/vmm.o \
       $(BUILD_DIR)/elf.o \
       $(BUILD_DIR)/string_util.o \
       $(BUILD_DIR)/task.o \
       $(BUILD_DIR)/timer.o \
       $(BUILD_DIR)/basic.o \
       $(BUILD_DIR)/audio_codec.o \
       $(BUILD_DIR)/player.o \
       $(BUILD_DIR)/html_parser.o \
       $(BUILD_DIR)/browser.o \
       $(BUILD_DIR)/shell.o

.PHONY: all clean run

all: $(BUILD_DIR)/lenovix.bin

$(BUILD_DIR)/lenovix.bin: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

$(BUILD_DIR)/boot.o: src/arch/x86/boot.asm
	@mkdir -p $(BUILD_DIR)
	$(AS) -f elf32 $< -o $@

$(BUILD_DIR)/gdt.o: src/arch/x86/gdt.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/idt.o: src/arch/x86/idt.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

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

$(BUILD_DIR)/ata.o: src/drivers/ata.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/pci.o: src/drivers/pci.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/rtl8139.o: src/drivers/rtl8139.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/net.o: src/drivers/net.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/tcp.o: src/drivers/tcp.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/speaker.o: src/drivers/speaker.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/sb16.o: src/drivers/sb16.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/rtc.o: src/drivers/rtc.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/acpi.o: src/drivers/acpi.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/bmp.o: src/drivers/bmp.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/vfs.o: src/fs/vfs.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/fat16.o: src/fs/fat16.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/tar.o: src/fs/tar.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/heap.o: src/kernel/heap.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/kernel.o: src/kernel/kernel.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/pmm.o: src/kernel/pmm.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/vmm.o: src/kernel/vmm.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/elf.o: src/kernel/elf.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/string_util.o: src/kernel/string_util.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/task.o: src/kernel/task.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/timer.o: src/kernel/timer.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/basic.o: src/kernel/basic.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/audio_codec.o: src/kernel/audio_codec.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/player.o: src/drivers/player.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/html_parser.o: src/kernel/html_parser.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/browser.o: src/drivers/browser.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/shell.o: src/shell/shell.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

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
	qemu-system-i386 -cdrom lenovix.iso -vga std -nic model=rtl8139 -audiodev sdl,id=snd0 -device sb16,audiodev=snd0
# Minimal Coco OS build for QEMU (both 32-bit and 64-bit)
OBJDIR = build
SRCDIR = kernel
BOOTDIR = boot

AS32 = as --32
AS64 = as --64
CC32 = gcc
CC64 = gcc
LD32 = ld -m elf_i386
LD64 = ld -m elf_x86_64

ISO32 = cocoos-32bit.iso
ISO64 = cocoos-64bit.iso

GRUB_MKRESCUE := $(shell command -v grub-mkrescue 2>/dev/null)
ifeq ($(GRUB_MKRESCUE),)
$(warning grub-mkrescue not found; install grub2-common (Debian/Ubuntu) or grub-pc-bin to enable ISO creation)
endif

.PHONY: all clean
all: $(ISO32) $(ISO64)

$(OBJDIR)/boot32.o: $(BOOTDIR)/boot.S
	mkdir -p $(OBJDIR)
	$(AS32) -o $@ $<

$(OBJDIR)/boot64.o: $(BOOTDIR)/boot64.S
	mkdir -p $(OBJDIR)
	$(AS64) -o $@ $<

$(OBJDIR)/main32.o: $(SRCDIR)/main.c $(SRCDIR)/kernel.h
	mkdir -p $(OBJDIR)
	$(CC32) -m32 -ffreestanding -O2 -fno-pie -fno-stack-protector -fno-builtin -c -o $@ $<

$(OBJDIR)/main64.o: $(SRCDIR)/main.c $(SRCDIR)/kernel.h
	mkdir -p $(OBJDIR)
	$(CC64) -m64 -ffreestanding -O2 -fno-pie -fno-stack-protector -fno-builtin -c -o $@ $<

$(OBJDIR)/kernel32.bin: $(OBJDIR)/boot32.o $(OBJDIR)/main32.o
	$(LD32) -Ttext 0x00100000 -o $@ --oformat binary -nostdlib -static --no-pie $^

$(OBJDIR)/kernel64.bin: $(OBJDIR)/boot64.o $(OBJDIR)/main64.o
	$(LD64) -Ttext 0x00100000 -o $@ --oformat binary -nostdlib -static --no-pie $^

# shared iso target generation
define MAKE_ISO
$(1): $(2)
	mkdir -p iso/boot/grub
	cp $$^ iso/boot/kernel.bin
	echo 'set timeout=1' > iso/boot/grub/grub.cfg
	echo 'set default=0' >> iso/boot/grub/grub.cfg
	echo 'insmod gfxterm' >> iso/boot/grub/grub.cfg
	echo 'set gfxpayload=1024x768x32' >> iso/boot/grub/grub.cfg
	echo 'menuentry "Coco OS" {' >> iso/boot/grub/grub.cfg
	echo '  multiboot /boot/kernel.bin' >> iso/boot/grub/grub.cfg
	echo '  boot' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	@if [ -z "$(GRUB_MKRESCUE)" ]; then \
		echo "warning: grub-mkrescue missing, skipping ISO generation for $@"; \
		exit 0; \
	else \
		$(GRUB_MKRESCUE) -o $$@ iso; \
	fi
endef

$(eval $(call MAKE_ISO,$(ISO32),$(OBJDIR)/kernel32.bin))
$(eval $(call MAKE_ISO,$(ISO64),$(OBJDIR)/kernel64.bin))

clean:
	rm -rf $(OBJDIR) iso $(ISO32) $(ISO64)

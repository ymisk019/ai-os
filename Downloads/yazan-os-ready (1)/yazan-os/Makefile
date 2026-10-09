# ============================================================================
# Yazan OS 3.0 (Fire Edition) - Build System
#   make               build build/yazan.elf
#   make iso           build build/yazanos.iso (needs grub-mkrescue + xorriso)
#   make iso FAULT=pf  build crash ISO for testing panic screen (div0, ud, pf, gp)
#   make run           run in QEMU with 256MB RAM and serial output
#   make run-lowram    run in QEMU with 32MB RAM to demonstrate the Red Screen RAM panic!
#   make preview       run Python preview tool to render OS UI screenshots
#   make clean         clean build artifacts
# ============================================================================

CROSS   ?=
CC      := $(CROSS)gcc
LD      := $(CROSS)ld
OBJDUMP := $(CROSS)objdump

FAULT    ?=
FAULT_ID := $(if $(filter div0,$(FAULT)),1,$(if $(filter ud,$(FAULT)),2,$(if $(filter pf,$(FAULT)),3,$(if $(filter gp,$(FAULT)),4,0))))

CFLAGS  := -std=gnu11 -O2 -g -Wall -Wextra -Werror \
           -ffreestanding -fno-stack-protector -fno-pic -fno-pie \
           -mno-red-zone -mgeneral-regs-only -mno-mmx -mno-sse -mno-sse2 \
           -fcf-protection=none -fno-asynchronous-unwind-tables -fno-omit-frame-pointer \
           -fno-tree-loop-distribute-patterns -DTEST_FAULT=$(FAULT_ID) -Ikernel -Igui -Iapps
LDFLAGS := -nostdlib -static -no-pie -z max-page-size=0x1000 -z noexecstack -T linker.ld

BUILD   := build
KERNEL  := $(BUILD)/yazan.elf
ISO     := $(BUILD)/yazanos.iso
ISODIR  := $(BUILD)/iso

C_SRC   := $(wildcard kernel/*.c) $(wildcard gui/*.c) $(wildcard apps/*.c)
S_SRC   := $(wildcard kernel/*.S)
OBJS    := $(BUILD)/boot.o $(patsubst %.c,$(BUILD)/%.o,$(C_SRC)) $(patsubst kernel/%.S,$(BUILD)/%.o,$(S_SRC))

.PHONY: all iso run run-lowram preview check clean FORCE
all: $(KERNEL)

$(BUILD):
	mkdir -p $(BUILD) $(BUILD)/kernel $(BUILD)/gui $(BUILD)/apps

$(BUILD)/flags: FORCE | $(BUILD)
	@echo '$(CFLAGS)' | cmp -s - $@ || echo '$(CFLAGS)' > $@
FORCE:

$(BUILD)/boot.o: boot/boot.S | $(BUILD)
	$(CC) -c $< -o $@

$(BUILD)/%.o: kernel/%.S | $(BUILD)
	$(CC) -c $< -o $@

$(BUILD)/%.o: %.c $(wildcard kernel/*.h) $(wildcard gui/*.h) $(wildcard apps/*.h) $(BUILD)/flags | $(BUILD)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

iso: $(ISO)
$(ISO): $(KERNEL) boot/grub.cfg
	rm -rf $(ISODIR)
	mkdir -p $(ISODIR)/boot/grub
	cp $(KERNEL) $(ISODIR)/boot/yazan.elf
	cp boot/grub.cfg $(ISODIR)/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(ISODIR)

run: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO) -m 256M -vga std -serial stdio

run-lowram: $(ISO)
	@echo "=== Booting with 32MB RAM to test the Red Screen of Death! ==="
	qemu-system-x86_64 -cdrom $(ISO) -m 32M -vga std -serial stdio

preview:
	python3 tools/preview_full.py

clean:
	rm -rf $(BUILD)

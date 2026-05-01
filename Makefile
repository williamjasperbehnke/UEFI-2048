SHELL := /bin/sh
.DEFAULT_GOAL := all

APP_NAME := 2048
SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)
SO := $(APP_NAME).so
EFI := $(APP_NAME).efi

ARCH ?= x86_64
GNUEFI ?= /usr
CC ?= gcc
LD ?= ld
OBJCOPY ?= objcopy

EFIINC := $(GNUEFI)/include/efi
EFIINC_ARCH := $(GNUEFI)/include/efi/$(ARCH)
EFILIB := $(GNUEFI)/lib

CFLAGS := -I$(EFIINC) -I$(EFIINC_ARCH) -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -Wall -Wextra -DEFI_FUNCTION_WRAPPER
LDFLAGS := -nostdlib -znocombreloc -T $(EFILIB)/elf_$(ARCH)_efi.lds -shared -Bsymbolic -L$(EFILIB) $(EFILIB)/crt0-efi-$(ARCH).o
LDLIBS := -lefi -lgnuefi

OBJCOPY_FLAGS := \
	-j .text \
	-j .sdata \
	-j .data \
	-j .dynamic \
	-j .dynsym \
	-j .rel \
	-j .rela \
	-j .reloc \
	--target efi-app-$(ARCH)

BREW_PREFIX := $(shell brew --prefix 2>/dev/null)
OVMF_PATHS := \
	"$(BREW_PREFIX)/share/qemu/edk2-x86_64-code.fd" \
	"$(BREW_PREFIX)/share/qemu/edk2-i386-code.fd" \
	"$(BREW_PREFIX)/share/edk2-x86_64-code.fd" \
	"/opt/homebrew/share/qemu/edk2-x86_64-code.fd" \
	"/opt/homebrew/share/qemu/edk2-i386-code.fd" \
	"/usr/local/share/qemu/edk2-x86_64-code.fd" \
	"/usr/local/share/qemu/edk2-i386-code.fd"
AUDIO_ARGS := -audiodev coreaudio,id=snd0 -machine pcspk-audiodev=snd0

all: $(EFI)

$(SO): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) $(LDLIBS) -o $@

$(EFI): $(SO)
	$(OBJCOPY) $(OBJCOPY_FLAGS) $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

stage: $(EFI)
	mkdir -p esp/EFI/BOOT
	cp $(EFI) esp/EFI/BOOT/BOOTX64.EFI
	printf 'map -r\nfs0:\\EFI\\BOOT\\BOOTX64.EFI\nfs1:\\EFI\\BOOT\\BOOTX64.EFI\n' > esp/startup.nsh

run: stage
	@if ! command -v qemu-system-x86_64 >/dev/null 2>&1; then \
		echo "qemu-system-x86_64 not found. Install with: brew install qemu"; \
		exit 1; \
	fi
	@OVMF_CODE=""; \
	for p in $(OVMF_PATHS); do \
		if [ -f "$$p" ]; then OVMF_CODE="$$p"; break; fi; \
	done; \
	if [ -z "$$OVMF_CODE" ]; then \
		echo "Could not find OVMF firmware. Install with: brew install qemu"; \
		echo "Expected under /opt/homebrew/share/qemu or /usr/local/share/qemu"; \
		exit 1; \
	fi; \
	echo "Using firmware: $$OVMF_CODE"; \
	qemu-system-x86_64 -m 256 $(AUDIO_ARGS) -drive if=pflash,format=raw,readonly=on,file="$$OVMF_CODE" -drive format=raw,file=fat:rw:esp

run-clean:
	docker build -t uefi-2048 .
	docker run --rm -v "$(CURDIR)":/work uefi-2048
	$(MAKE) run

help:
	@echo "Targets:"
	@echo "  all      	Build $(EFI)"
	@echo "  stage    	Copy to esp/EFI/BOOT/BOOTX64.EFI and write startup.nsh"
	@echo "  run     	Boot in QEMU with auto-detected OVMF"
	@echo "  run-clean 	Build in Docker and run in QEMU"
	@echo "  clean    	Remove build artifacts"

clean:
	rm -f $(OBJ) $(SO) $(EFI)
	rm -rf esp

.PHONY: all clean stage run run-clean help

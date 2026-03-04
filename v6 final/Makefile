DEVKITPPC ?= /opt/devkitpro/devkitPPC
WUT       ?= /opt/devkitpro/wut
TOOLS     ?= /opt/devkitpro/tools

CC      := $(DEVKITPPC)/bin/powerpc-eabi-gcc
ELF2RPL := $(TOOLS)/bin/elf2rpl

CFLAGS  := -Os -Wall -ffreestanding -mcpu=750 -meabi -I$(WUT)/include
LDFLAGS := -nostartfiles -L$(WUT)/lib -lwut

TARGET := 90ab_w

all: $(TARGET).ipx

$(TARGET).o: ../source/autoboot_wake.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(TARGET).o
	$(CC) $< -o $@ $(LDFLAGS)

$(TARGET).ipx: $(TARGET).elf
	$(ELF2RPL) $< $@

clean:
	rm -f *.o *.elf *.ipx

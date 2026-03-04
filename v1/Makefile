DEVKITPRO ?= /opt/devkitpro
DEVKITPPC := $(DEVKITPRO)/devkitPPC
WUT := $(DEVKITPRO)/wut

CC := $(DEVKITPPC)/bin/powerpc-eabi-gcc
ELF2RPL := /opt/devkitpro/tools/bin/elf2rpl


TARGET := 90ab_w
SRC := source/autoboot_wake.c

INCLUDES := -I$(WUT)/include
LIBDIRS  := -L$(WUT)/lib

CFLAGS := -Os -Wall -ffreestanding -mcpu=750 -meabi $(INCLUDES)
LDFLAGS := -nostartfiles $(LIBDIRS) -lwut

all: $(TARGET).ipx

$(TARGET).o: $(SRC)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(TARGET).o
	$(CC) $< -o $@ $(LDFLAGS)

$(TARGET).ipx: $(TARGET).elf
	$(ELF2RPL) $< $@

clean:
	rm -f *.o *.elf *.ipx

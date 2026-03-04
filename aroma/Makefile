# AutoBootWake Makefile for Aroma
# Builds as an RPX module

DEVKITPRO ?= /opt/devkitpro
WUT       ?= $(DEVKITPRO)/wut

CC      := $(DEVKITPRO)/devkitPPC/bin/powerpc-eabi-gcc
CXX     := $(DEVKITPRO)/devkitPPC/bin/powerpc-eabi-g++
LD      := $(DEVKITPRO)/devkitPPC/bin/powerpc-eabi-ld
ELF2RPX := $(DEVKITPRO)/tools/bin/elf2rpx

CFLAGS  := -Os -Wall -mcpu=750 -meabi -I$(WUT)/include
CXXFLAGS := -Os -Wall -mcpu=750 -meabi -I$(WUT)/include -std=c++11
LDFLAGS := -nostartfiles -L$(WUT)/lib -lwut -specs=$(WUT)/share/wut.specs

TARGET := autoboot_wake

all: $(TARGET).rpx

$(TARGET).o: autoboot_wake.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET).elf: $(TARGET).o
	$(CXX) $< -o $@ $(LDFLAGS)

$(TARGET).rpx: $(TARGET).elf
	$(ELF2RPL) $< $@

clean:
	rm -f *.o *.elf *.rpx

.PHONY: all clean

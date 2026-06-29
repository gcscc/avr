MCU=atmega2560
F_CPU=16000000UL

CC=avr-gcc
OBJCOPY=avr-objcopy

CFLAGS=\
-std=c17 \
-mmcu=$(MCU) \
-DF_CPU=$(F_CPU) \
-Wall \
-Wextra \
-Wpedantic \
-Os \
-Iinclude

CFLAGS += -std=gnu23

SRC=$(wildcard src/*.c)

TARGET=build/main

all: $(TARGET).hex

$(TARGET).elf: $(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) $^ -o $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

SERIAL_DEVICE := /dev/ttyUSB0

flash: $(TARGET).hex
	avrdude \
	-p m2560 \
	-c wiring \
	-P $(SERIAL_DEVICE) \
	-b 115200 \
	-D \
	-U flash:w:$<:i

clean:
	rm -rf build

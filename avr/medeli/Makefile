MCU=atmega2560

F_CPU=16000000UL
BAUD=115200

CC=avr-gcc \
	-DBAUD=$(BAUD) \

OBJCOPY=avr-objcopy

CFLAGS=\
-mmcu=$(MCU) \
-DF_CPU=$(F_CPU) \
-Wall \
-Wextra \
-Wpedantic \
-Os \
-std=gnu99 \
-Iinclude

SRC=$(wildcard src/*.c)

TARGET=build/main

all: $(TARGET).hex

$(TARGET).elf: $(SRC)
	mkdir -p build
	$(CC) $(CFLAGS) $^ -o $@ -lm

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

# SERIAL_DEVICE := /dev/cu.wchusbserial500
SERIAL_DEVICE := /dev/ttyUSB0

flash: $(TARGET).hex
	avrdude \
	-p m2560 \
	-c wiring \
	-P $(SERIAL_DEVICE) \
	-b $(BAUD) \
	-D \
	-U flash:w:$<:i

scr:
	# echo "Para sair: Ctrl-A Ctrl-K y"
	screen $(SERIAL_DEVICE) $(BAUD)

mon:
	picocom -b $(BAUD) --nolock --echo $(SERIAL_DEVICE)

clean:
	rm -rf build

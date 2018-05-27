CC = arm-none-eabi-gcc
CFLAGS = --std=gnu11 -Ofast  -mlong-calls -fPIC -mno-pic-data-is-text-relative -msingle-pic-base -mpic-register=r9  -mcpu=cortex-m3 -mthumb -fno-builtin  -ffreestanding   --specs=nosys.specs --specs=nano.specs -Tkmod.ld -Wl,-N -Wl,--just-symbols=../armboy-kernel/src/kernel.elf
OBYCP = arm-none-eabi-objcopy

INCLUDE = -I ../armboy-kernel/src/ -I ../../../tools/ATMEL -I ../../../tools/CMSIS/include
CFILES = sdDriver.c FAT32/*.c volume.c

fs.bin : $(CFILES) kmod.S
	$(CC) $(CFLAGS) -o fs.elf $(CFILES) kmod.S $(INCLUDE)
	$(OBYCP) -O binary fs.elf fs.bin

.PHONY: upload
upload: fs.bin
	python3 ../tool/deleteAll.py
	../tool/abFlasher.a fs.bin /dev/cu.usbmodem14121

clean :
	rm input.elf
	rm input.bin

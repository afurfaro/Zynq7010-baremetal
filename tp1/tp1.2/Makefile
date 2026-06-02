CROSS = arm-none-eabi

CFLAGS = -Wall -O0 -g -ffreestanding -nostdlib -nostartfiles -mcpu=cortex-a9
ASFLAGS = -mcpu=cortex-a9 -g
LDFLAGS = -T linker.ld -Map=kernel.map --cref

OBJS = start.o kernel.o 

all: kernel.elf Makefile

start.o: start.S Makefile
	$(CROSS)-as $(ASFLAGS) -a=start.lst -c $< -o $@

kernel.o: kernel.c Makefile
	$(CROSS)-gcc $(CFLAGS) -c $< -o $@

kernel.elf: $(OBJS) Makefile
	$(CROSS)-ld $(LDFLAGS) $(OBJS) -o $@

kernel.bin: kernel.elf Makefile
	$(CROSS)-objcopy -O binary $< $@

run: kernel.bin
	qemu-system-arm \
	-M xilinx-zynq-a9 \
	-nographic \
	-device loader,file=kernel.bin,addr=0x0

debug: kernel.bin
	qemu-system-arm \
	-M xilinx-zynq-a9 \
	-nographic \
	-S -s \
	-device loader,file=kernel.bin,addr=0x0

clean:
	rm -f *.o *.elf *.bin *.lst *.map

CC = riscv64-unknown-elf-gcc
LD = riscv64-unknown-elf-ld
OBJCOPY = riscv64-unknown-elf-objcopy
QEMU = qemu-system-riscv64

CFLAGS = -march=rv64gc -mabi=lp64 -mcmodel=medany -ffreestanding -nostdlib -O2 -Wall

OBJS = start.o uart.o main.o

all: kernel.bin

kernel.bin: kernel.elf
	$(OBJCOPY) -O binary kernel.elf kernel.bin

kernel.elf: $(OBJS) linker.ld
	$(LD) -T linker.ld -o kernel.elf $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

run: kernel.bin
	$(QEMU) -M virt -kernel kernel.bin -display none -serial mon:stdio

clean:
	rm -f *.o kernel.elf kernel.bin
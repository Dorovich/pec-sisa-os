KERNEL_OBJS  = kernel-entry.o kuserblob.o kernel.o libc.o list.o
USER_OBJS  = user-entry.o umain.o fibonacci.o corre_letras.o

CFLAGS = -O0 -fno-builtin

all: kernel.code.bin kernel.data.bin kernel.user.bin kernel.code.hex kernel.data.hex kernel.user.hex

kuserblob.o: user.code.bin user.data.bin

kernel.elf: $(KERNEL_OBJS)
	sisa-ld -T kernel.ld $(KERNEL_OBJS) -o $@

user.elf: $(USER_OBJS)
	sisa-ld -T user.ld $^ -o $@

.c.o:
	sisa-gcc $(CFLAGS) -c $< -o $@

.s.o:
	sisa-as $< -o $@

%.hex: %.bin
	@hexdump -ve '1/2 "%.4x\n"' $< > $@

%.code.bin: %.elf
	sisa-objcopy -O binary -j .text $< $@

%.data.bin: %.elf
	sisa-objcopy -O binary -j .data $< $@

%.user.bin: %.elf
	sisa-objcopy -O binary --set-section-flags .user=alloc,load -j .user $< $@

kdisasm: kernel.elf
	@sisa-objdump -d $<

udisasm: user.elf
	@sisa-objdump -d $<

clean:
	@rm -f $(KERNEL_OBJS) $(USER_OBJS) \
	kernel.elf user.elf \
	kernel.code.hex kernel.data.hex kernel.user.hex \
	kernel.code.bin kernel.data.bin kernel.user.bin \
	user.code.bin user.data.bin \

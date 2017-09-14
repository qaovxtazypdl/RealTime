XCC     = gcc
AS	= as
LD      = ld

ifdef EMULATOR_BUILD
    CFLAGS  = -c -O2 -fPIC -Wall -I. -Iemul -Ibwio -mcpu=arm920t -msoft-float -fno-builtin -DEMULATOR_BUILD
    LDFLAGS = -init kernel -Map linker.map -N  -T ./emul/linkerscript.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2
else
    CFLAGS  = -c -O2 -fPIC -Wall -I. -Iemul -Ibwio -mcpu=arm920t -msoft-float -fno-builtin
    LDFLAGS = -init kernel -Map linker.map -N  -T ./linkerscript.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2
endif

ASFLAGS = -mcpu=arm920t -mapcs-32 -mfloat-abi=soft

KERNEL_DIR=kernel
TASKS_DIR=tasks
COMMON_DIR=common
ASM_DIR=asm
OBJ_DIR=obj
OBJ_FILES=$(wildcard $(OBJ_DIR)/*.o)
# -g: include hooks for gdb
# -c: only compile
# -mcpu=arm920t: generate code for the 920t architecture
# -fpic: emit position-independent code
# -Wall: report all warnings
# -mapcs: always generate a complete stack frame
#


.PHONY: prep

all: clean prep kernel.elf

prep:
	-mkdir $(OBJ_DIR)
	-mkdir $(ASM_DIR)

algs.S:
	$(XCC) $(CFLAGS) -S -o $(ASM_DIR)/algs.S $(COMMON_DIR)/algs.c

kernel.S:
	$(XCC) $(CFLAGS) -S -o $(ASM_DIR)/kernel.S $(KERNEL_DIR)/kernel.c

assert.S:
	$(XCC) $(CFLAGS) -S -o $(ASM_DIR)/assert.S $(COMMON_DIR)/assert.c

syscall_handlers.S:
	$(XCC) $(CFLAGS) -S -o $(ASM_DIR)/syscall_handlers.S $(KERNEL_DIR)/syscall_handlers.c

scheduler.S:
	$(XCC) $(CFLAGS) -S -o $(ASM_DIR)/scheduler.S $(KERNEL_DIR)/scheduler.c

irq_handler.S:
	$(XCC) $(CFLAGS) -S -o $(ASM_DIR)/irq_handler.S $(KERNEL_DIR)/irq_handler.c

byte_buffer.S:
	$(XCC) $(CFLAGS) -S -o $(ASM_DIR)/byte_buffer.S $(COMMON_DIR)/byte_buffer.c

byte_buffer.o: byte_buffer.S
	$(AS) $(ASFLAGS) -o $(OBJ_DIR)/byte_buffer.o $(ASM_DIR)/byte_buffer.S

task_first.S:
	$(XCC) $(CFLAGS) -S -o $(ASM_DIR)/task_first.S $(TASKS_DIR)/first.c

task_idle.S:
	$(XCC) $(CFLAGS) -S -o $(ASM_DIR)/task_idle.S $(TASKS_DIR)/idle.c

io_controller.S:
	$(XCC) $(CFLAGS) -S -o $(ASM_DIR)/io_controller.S $(TASKS_DIR)/io_controller.c

task_printf.S:
	$(XCC) $(CFLAGS) -S -o $(ASM_DIR)/task_printf.S $(TASKS_DIR)/utils/printf.c

task_name_server.S:
	$(XCC) $(CFLAGS) -S -o $(ASM_DIR)/task_name_server.S $(TASKS_DIR)/name_server.c

task_timer_server_notifier.S:
	$(XCC) $(CFLAGS) -S -o $(ASM_DIR)/task_timer_server_notifier.S $(TASKS_DIR)/timer_server_notifier.c

task_uart_server_notifier.S:
	$(XCC) $(CFLAGS) -S -o $(ASM_DIR)/task_uart_server_notifier.S $(TASKS_DIR)/uart_server_notifier.c

context_switch.o:
	$(AS) $(ASFLAGS) -o $(OBJ_DIR)/context_switch.o $(KERNEL_DIR)/context_switch.S

syscall.o:
	$(AS) $(ASFLAGS) -o $(OBJ_DIR)/syscall.o $(KERNEL_DIR)/syscall.S

irq_handler.o: irq_handler.S
	$(AS) $(ASFLAGS) -o $(OBJ_DIR)/irq_handler.o $(ASM_DIR)/irq_handler.S

startup.o:
	$(AS) $(ASFLAGS) -o $(OBJ_DIR)/startup.o emul/startup.S

task_printf.o: task_printf.S
	$(AS) $(ASFLAGS) -o $(OBJ_DIR)/task_printf.o $(ASM_DIR)/task_printf.S

task_first.o: task_first.S
	$(AS) $(ASFLAGS) -o $(OBJ_DIR)/task_first.o $(ASM_DIR)/task_first.S

task_idle.o: task_idle.S
	$(AS) $(ASFLAGS) -o $(OBJ_DIR)/task_idle.o $(ASM_DIR)/task_idle.S

io_controller.o: io_controller.S
	$(AS) $(ASFLAGS) -o $(OBJ_DIR)/io_controller.o $(ASM_DIR)/io_controller.S

task_name_server.o: task_name_server.S
	$(AS) $(ASFLAGS) -o $(OBJ_DIR)/task_name_server.o $(ASM_DIR)/task_name_server.S

task_timer_server_notifier.o: task_timer_server_notifier.S
	$(AS) $(ASFLAGS) -o $(OBJ_DIR)/task_timer_server_notifier.o $(ASM_DIR)/task_timer_server_notifier.S

task_uart_server_notifier.o: task_uart_server_notifier.S
	$(AS) $(ASFLAGS) -o $(OBJ_DIR)/task_uart_server_notifier.o $(ASM_DIR)/task_uart_server_notifier.S

algs.o: algs.S
	$(AS) $(ASFLAGS) -o $(OBJ_DIR)/algs.o $(ASM_DIR)/algs.S

kernel.o: kernel.S
	$(AS) $(ASFLAGS) -o $(OBJ_DIR)/kernel.o $(ASM_DIR)/kernel.S

assert.o: assert.S
	$(AS) $(ASFLAGS) -o $(OBJ_DIR)/assert.o $(ASM_DIR)/assert.S

syscall_handlers.o: syscall_handlers.S
	$(AS) $(ASFLAGS) -o $(OBJ_DIR)/syscall_handlers.o $(ASM_DIR)/syscall_handlers.S

scheduler.o: scheduler.S
	$(AS) $(ASFLAGS) -o $(OBJ_DIR)/scheduler.o $(ASM_DIR)/scheduler.S

bwio.o:
	$(XCC) $(CFLAGS) -c bwio/bwio.c -o $(OBJ_DIR)/bwio.o

kernel.elf: bwio.o byte_buffer.o startup.o context_switch.o task_printf.o irq_handler.o syscall.o task_idle.o algs.o scheduler.o io_controller.o syscall_handlers.o assert.o task_name_server.o task_uart_server_notifier.o task_timer_server_notifier.o task_first.o kernel.o
	$(LD) $(LDFLAGS) -o $@ $(OBJ_FILES) -lgcc

clean:
	-rm -rf $(OBJ_DIR)
	-rm -rf $(ASM_DIR)

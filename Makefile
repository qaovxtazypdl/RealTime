XCC = gcc
AS = as
LD = ld
 
ifndef LIBGCC_DIR 
    LIBGCC_DIR = /u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2
endif

INCLUDE_DIR=include

# -g: include hooks for gdb
# -c: only compile
# -mcpu=arm920t: generate code for the 920t architecture
# -fpic: emit position-independent code
# -Wall: report all warnings
# -mapcs: always generate a complete stack frame



ifdef EMULATOR_BUILD
	LINKER_SCRIPT=src/emul/linkerscript.ld
	CFLAGS += -DEMULATOR_BUILD 
else
	LINKER_SCRIPT=linkerscript.ld
endif

LDFLAGS = -init main -Map linker.map -N	-T $(LINKER_SCRIPT) -L$(LIBGCC_DIR) -g

.PHONY: kern user common emul clean

main: | clean all

all: kern user common emul
	$(LD) $(LDFLAGS) src/kern/kern.o src/user/user.o src/common/common.o -o main.elf -lgcc

clean:
	-$(MAKE) -C src/kern clean
	-$(MAKE) -C src/user clean
	-$(MAKE) -C src/common clean
	-$(MAKE) -C src/emul clean

kern: common
	$(MAKE) -C src/kern
user: common
	$(MAKE) -C src/user
common:
	$(MAKE) -C src/common
emul:
	$(MAKE) -C src/emul

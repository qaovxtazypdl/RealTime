XCC = gcc
AS = as
LD = ld

LDFLAGS = -N -g -r
ASFLAGS = -mcpu=arm920 -mapcs-32 -mfloat-abi=soft
CFLAGS = -c \
	 -fPIC \
	 -Wall \
	 -I. \
	 -I$(INCLUDE_DIR) \
	 -mtune=arm920t \
	 -msoft-float \
	 -fno-builtin \
	 -std=c99 \
	 -fasm \
	 -include $(INCLUDE_DIR)/dbg.h \
	 -include $(INCLUDE_DIR)/common/gen/enum_map.h \
	 -include $(INCLUDE_DIR)/common/types.h \
	 -g

# -c: only compile
# -mcpu=arm920t: generate code for the 920t architecture
# -fpic: emit position-independent code
# -Wall: report all warnings
# -mapcs: always generate a complete stack frame

INCLUDE_DIR=include
OBJ_DIR=obj
CFG_DIR=cfg
SRC_DIR=.

CFILES=$(shell find $(SRC_DIR) -name '*.c' -type f -printf "%P\n")
ASMFILES=$(shell find $(SRC_DIR) -name '*.s' -type f -printf "%P\n")
OFILES=$(CFILES:%.c=$(OBJ_DIR)/%.o) $(ASMFILES:%.s=$(OBJ_DIR)/%_asm.o)

ifdef EMULATOR_BUILD
	CFLAGS += -DEMULATOR_BUILD 
endif

ifndef LIBGCC_DIR 
	LIBGCC_DIR = /u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2
endif

.PHONY: clean

kern.o: $(OFILES)
	$(LD) $(LDFLAGS) -o $@ $(OFILES)

$(OBJ_DIR)/%_asm.o: %.s
	-mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<

$(OBJ_DIR)/%.o: %.c
	-mkdir -p $(dir $@)
	$(XCC) $(CFLAGS) -o $@ $<

clean:
	-rm *.map
	-rm -rf $(OBJ_DIR)
	-rm main.elf

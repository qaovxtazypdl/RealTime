XCC = gcc
AS = as
LD = ld

LDFLAGS = -N -g -r
ASFLAGS = -mcpu=arm920 -mapcs-32 -mfloat-abi=soft
CFLAGS = -c \
				 -O2 \
	 -fPIC \
	 -Wall \
	 -I. \
	 -I$(INCLUDE_DIR) \
	 -mtune=arm920t \
	 -msoft-float \
	 -fno-builtin \
	 -std=c99 \
	 -fasm \
	 -include $(INCLUDE_DIR)/types.h \
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

CFILES:=$(shell find $(SRC_DIR) -name '*.c' -type f -printf "%P\n")
ASMFILES:=$(shell find $(SRC_DIR) -name '*.s' -type f -printf "%P\n")
OFILES=$(CFILES:%.c=$(OBJ_DIR)/%.o) $(ASMFILES:%.s=$(OBJ_DIR)/%_asm.o) $(OBJ_DIR)/gen/enum_map.o

ifdef EMULATOR_BUILD
	CFLAGS += -DEMULATOR_BUILD 
endif

ifndef LIBGCC_DIR 
	LIBGCC_DIR = /u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2
endif

.PHONY: clean enum_map

common.o: $(OFILES)
	$(LD) $(LDFLAGS) -o $@ $(OFILES)

enum_map:
	-mkdir gen include/gen
	perl util/gen_enum_map.pl $(shell find .. -name '*.[ch]' -type f)
	mv enum_map.h include/gen
	mv enum_map.c gen

$(OBJ_DIR)/%_asm.o: %.s
	-mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<

$(OBJ_DIR)/%.o: %.c enum_map
	-mkdir -p $(dir $@)
	$(XCC) $(CFLAGS) -o $@ $<

clean:
	-rm *.map
	-rm -rf $(OBJ_DIR)
	-rm main.elf
	-rm -r $(shell find . -type d -name 'gen')

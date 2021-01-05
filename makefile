# ----------------------------------------------------------------------------
# COMMON DEFINITIONs

# --------------------------------
# Specify Default Target
.DEFAULT_GOAL	:= all

# Default ASM Compiler
AS        := nasm

# Default 'rm' command
RM        := rm -rf
# --------------------------------



# --------------------------------
# Misc:
# source code directory
SRCDIR    := ./src

# build directory
BLDIR     := ./build

# End of COMMON DEFINITIONs
# ----------------------------------------------------------------------------




# ----------------------------------------------------------------------------
# BOOT.bin
BTNAME      := boot

BTROOT      := $(SRCDIR)/$(BTNAME)

BOOT        := $(BLDIR)/$(BTNAME).bin

$(BOOT) : $(BTROOT)/$(BTNAME).s
	@mkdir -p $(dir $@)
	nasm -f bin $< -o $@
# End of BOOT.bin
# ----------------------------------------------------------------------------



# ----------------------------------------------------------------------------
# VIRTUAL DISK IMAGE
DISK         := $(BLDIR)/vdisk.img

$(DISK) : $(BOOT) $(LOADER)
ifeq (,$(wildcard $(DISK)))
	bximage -hd="10M" -mode="create" -q $(DISK)
endif
	dd if=$(BOOT) of=$(DISK) bs=512 count=1 conv=notrunc
# End of VIRTUAL DISK IMAGE
# ----------------------------------------------------------------------------




# ----------------------------------------------------------------------------
# 
.PHONY: all clean run test 

all: $(DISK)
	echo done
 
clean:
	$(RM) ./build

run: all
	-bochsdbg.exe -f ./boot.bxrc

test: $(TESTS)
	./$(TESTS)

# ----------------------------------------------------------------------------
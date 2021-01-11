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
# Default C Compiler Flags
CFLAGS    = -m32#                 # Generate 32-bit code.

CFLAGS   += -ffreestanding

CFLAGS   += -Wreturn-type#        # Warn return-type that defaults to "int"

CFLAGS   += -Werror=return-type#  # Make warnings into hard errors.

CFLAGS   += -fno-stack-protector# # Don't Emit extra code to check for buffer
                                  # overflows.

CFLAGS   += -fno-builtin#         # Don't recognize built-in functions that
                                  # do not begin with __builtin_ as prefix

#CFLAGS   += -ffunction-sections#  # Place each function or data item into its 
#CFLAGS   += -fdata-sections#      # own section in the output file if the 
                                  # target supports arbitrary sections. The 
								  # name of the function or the name of the 
								  # item determines the section's name in the
								  # data output file.

CFLAGS   += -nostdinc#            # Do not search the standard system
                                  # directories for header files.

CFLAGS	 += -nostdlib#            # Do not use the standard system startup
                                  # files or libraries when linking.
								  
CFLAGS   += -MMD#                 # Make describing the dependencies of the
                                  # main source file, no system header files.

CFLAGS   += -s#                   # Remove all symbol table and relocation 
                                  # information from the executable.

CFLAGS   += -Os#                  # Optimize for size. -Os enables all -O2
                                  # optimizations that do not typically
								  # increase code size.

CFLAGS   += -c#                   # Do not to run the linker
# --------------------------------


# --------------------------------
# Default C++ Compiler Flags
CXXFLAGS  = -std=c++2a#           # Use C++ 20 standard

CXXFLAGS += -nostdinc++#          # Do not search for header files in the C++
                                  # specific standard directories
CXXFLAGS += ${CFLAGS}#            # Append C compiler flags

CXXFLAGS += -fno-exceptions

# Default Linker Options 
LDFLAGS   = -m elf_i386#          # Generate x86 elf

LDFLAGS  += --gc-sections#        # Enable garbage collection of unused input
                                  # sections.

# LDFLAGS  += -T ldscript#        # Load linker script

LDFLAGS  += -e main#              # Entry point
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

$(BOOT): $(BTROOT)/$(BTNAME).s
	@mkdir -p $(dir $@)
	nasm -f bin $< -o $@
# End of BOOT.bin
# ----------------------------------------------------------------------------



# ----------------------------------------------------------------------------
# Common LIBs
LBNAME       := libs

LBROOT       := $(SRCDIR)/$(LBNAME)

# Lib Include Directory
LBDIRS        = $(LBROOT)/std

LBDIRS       += $(LBROOT)/arch

LBDIRS       += $(LBROOT)/devices

LBDIRS       += $(LBROOT)/lkl

# Lib Source Code
LBSRCS       := $(shell find $(LBROOT) -name *.cpp)

# Lib Assembler Code
LBASMS       := $(shell find $(LBROOT) -name *.s)

# Lib Assembler Objects
LBAOBJS      := $(LBASMS:./src/%.s=$(BLDIR)/%.o)

# Lib Objects
LBOBJS       := $(LBSRCS:./src/%.cpp=$(BLDIR)/%.o)

# Lib Include Flags
LBINCS       := $(addprefix -I,$(LBDIRS)) #$(addprefix -I,$(LBDIRS))

$(filter %.o,$(LBAOBJS)):./build/%.o: $(SRCDIR)/%.s
	@mkdir -p $(dir $@)
	$(AS) -f elf $< -o $@

$(filter %.o,$(LBOBJS)):./build/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(LBINCS) $(CXXFLAGS) $< -o $@

# End of Common LIBs
# ----------------------------------------------------------------------------



# ----------------------------------------------------------------------------
# LOADER.bin

# Loader Name
LNAME       := loader

# Loader Binary File Name
LOADER		:= $(BLDIR)/$(LNAME).bin

# Loader Root Folder
LROOT       := $(SRCDIR)/$(LNAME)

# Loader Include Directory
LDIRS       := $(shell find $(LROOT) -type d)

# Loader Source Code
LSRCS       := $(shell find $(LROOT) -name *.cpp)

# Loader Objects
LOBJS       := $(LSRCS:./src/%.cpp=$(BLDIR)/%.o)

# Loader Include Flags
LINCS       := $(addprefix -I,$(LROOT)) #$(addprefix -I,$(LDIRS))

# Generate Loader's object files.
$(filter %.o,$(LOBJS)):./build/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(LBINCS) $(LINCS) $(CXXFLAGS)  $< -o $@

# Generate Executable Loader
$(LOADER): $(LOBJS) $(LBOBJS) $(LBAOBJS)
	$(LD) $(LDFLAGS) -T $(LROOT)/ldscript -o $@ $^

# End of LOADER.bin
# ----------------------------------------------------------------------------



# ----------------------------------------------------------------------------
# Kernel.bin
#KNAME       := kernel
#LOADER	     := $(BLDIR)/$(KNAME).bin
#KROOT       := $(SRCDIR)/$(KNAME)
#KDIRS       := $(shell find $(KROOT) -type d)
#KSRCS       := $(shell find $(KDIR) -name *.cpp)
#
# ----------------------------------------------------------------------------



# ----------------------------------------------------------------------------
# VIRTUAL DISK IMAGE
DISK         := $(BLDIR)/vdisk.img

$(DISK) : $(BOOT) $(LOADER)
ifeq (,$(wildcard $(DISK)))
	bximage -hd="10M" -mode="create" -q $(DISK)
endif
	dd if=$(BOOT) of=$(DISK) bs=512 count=1 conv=notrunc
	dd if=$(LOADER) of=$(DISK) bs=512 count=2047 seek=1 \
    conv=notrunc
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

-include $(LOBJS:.o=.d)
-include $(LBOBJS:.o=.d)
# ----------------------------------------------------------------------------
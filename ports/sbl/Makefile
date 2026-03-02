# Include the core environment definitions; this will set $(TOP).
include ../../py/mkenv.mk

# Include py core make definitions.
include $(TOP)/py/py.mk
include $(TOP)/extmod/extmod.mk

# Set CFLAGS and libraries.
CFLAGS += -I. -I$(BUILD) -I$(TOP) -IInclude -maccumulate-outgoing-args -mno-red-zone -Wno-address -mcmodel=small -fpie -fno-asynchronous-unwind-tables -Wno-address  -fno-omit-frame-pointer
LIBS += -lm
#LDFLAGS += -nostdlib -static -Wl,-n,-q,--gc-sections -z common-page-size=0x40 -Wl,--whole-archive -Wl,--entry SecStartup -Os -flto -Wl,--script=GccBase.lds
LDFLAGS += -nostdlib -static -Wl,-n,-q,--gc-sections -z common-page-size=0x40 -Wl,--whole-archive -Wl,--entry SecStartup -Os -flto
#LDFLAGSx64 = -Wl,-melf_x86_64,--oformat=elf64-x86-64,-pie -Wl,--defsym=PECOFF_HEADER_SIZE=0x228 -Wl,-Ttext=0x00800000
LDFLAGSx64 = -Wl,-melf_x86_64,--oformat=elf64-x86-64
LDFLAGSia32 = -Wl,-m,elf_i386,--oformat=elf32-i386 -no-pie -Wl,--defsym=PECOFF_HEADER_SIZE=0x220 -Wl,-Ttext=0x00800000

# Define the required source files.
SRC_C = \
    main.c \
    mphalport.c \
    shared/readline/readline.c \
    shared/runtime/gchelper_generic.c \
    shared/runtime/pyexec.c \
    shared/runtime/stdout_helpers.c \
    extmod/modtime.c \
    sblentry.c \
    modpyb.c \
    gccollect.c #\
    
    #string.c \

# Define source files containung qstrs.
SRC_QSTR += shared/readline/readline.c shared/runtime/pyexec.c modpyb.c

# Define the required object files.
OBJ = $(PY_CORE_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o))

# Define the top-level target, the main firmware.
all: $(BUILD)/Micropythonx64.elf

# Define how to build the firmware.
$(BUILD)/Micropythonia32.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CC) $(LDFLAGS) $(LDFLAGSia32) -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@

$(BUILD)/Micropythonx64.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CC) $(LDFLAGS) $(LDFLAGSx64) -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@

# Include remaining core make rules.
include $(TOP)/py/mkrules.mk
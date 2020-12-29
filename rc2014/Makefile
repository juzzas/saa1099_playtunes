# RC2014 (Basic)


CC=zcc
AS=zcc
ASM=z80asm
TARGET=+rc2014
SUBTYPE=basic
VERBOSITY=-v


PRAGMA_FILE=zpragma.inc

DEFINES=-DSAA1099_PORT_REG=511 -DSAA1099_PORT_DATA=255

C_OPT_FLAGS=-SO3 --max-allocs-per-node200000

CFLAGS=$(TARGET) $(VERBOSITY) -c $(C_OPT_FLAGS) -compiler sdcc -clib=sdcc_iy -pragma-include:$(PRAGMA_FILE) -I../common -I../score
LDFLAGS=$(TARGET) $(VERBOSITY) -clib=sdcc_iy -pragma-include:$(PRAGMA_FILE)
ASFLAGS=$(TARGET) $(VERBOSITY) -c


EXEC=samtunes.hex
EXEC_OUTPUT=samtunes.hex

COMMON = @../common/common.lst @../score/score.lst

OBJECTS = main.o

%.o: %.c $(PRAGMA_FILE)
	$(CC) $(CFLAGS) $(DEFINES) -o $@ $<

%.o: %.asm
	$(AS) $(ASFLAGS) -o $@ $<

all : $(EXEC)

$(EXEC) : $(OBJECTS)
	 $(CC) $(LDFLAGS) $(DEFINES) -subtype=$(SUBTYPE) $(OBJECTS) $(COMMON) -I../common/include -o $(EXEC_OUTPUT) -create-app

.PHONY: clean
clean:
	rm -f *.o *.bin *.tap *.map zcc_opt.def *~ /tmp/tmpXX* *.ihx

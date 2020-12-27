CC=zcc
AS=zcc
TARGET=+rc2014
SUBTYPE=basic
VERBOSITY=-vn


PRAGMA_FILE=zpragma.inc

C_OPT_FLAGS=-SO3 --max-allocs-per-node200000

CFLAGS=$(TARGET) $(VERBOSITY) -c $(C_OPT_FLAGS) -compiler sdcc -clib=sdcc_iy -pragma-include:$(PRAGMA_FILE) -I../common/include
LDFLAGS=$(TARGET) $(VERBOSITY) -clib=sdcc_iy -pragma-include:$(PRAGMA_FILE)
ASFLAGS=$(TARGET) $(VERBOSITY) -c


EXEC=xenozoids2.hex
EXEC_OUTPUT=xenozoids2.hex

COMMON = @../common/common.lst

OBJECTS = main.o impl_start_menu.o

%.o: %.c $(PRAGMA_FILE)
	$(CC) $(CFLAGS) -o $@ $<

%.o: %.asm
	$(AS) $(ASFLAGS) -o $@ $<

all : $(EXEC)

$(EXEC) : $(OBJECTS)
	 $(CC) $(LDFLAGS) -subtype=$(SUBTYPE) $(OBJECTS) $(COMMON) -I../common/include -o $(EXEC_OUTPUT) -create-app

.PHONY: clean
clean:
	rm -f *.o *.bin *.tap *.map zcc_opt.def *~ /tmp/tmpXX* *.ihx

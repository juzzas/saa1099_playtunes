# sam coupe


CC=zcc
AS=zcc
ASM=z80asm
APPMAKE=appmake
TARGET=+embedded
STARTUP=0   # ram model
VERBOSITY=-v


PRAGMA_FILE=zpragma.inc

C_OPT_FLAGS=-SO3 --max-allocs-per-node200000

CFLAGS=$(TARGET) $(VERBOSITY) -c $(C_OPT_FLAGS) -compiler sdcc -clib=sdcc_iy -pragma-include:$(PRAGMA_FILE) -Ilib/include/sdcc -I../common -I../score
LDFLAGS=$(TARGET) $(VERBOSITY) -clib=sdcc_iy -pragma-include:$(PRAGMA_FILE)
ASFLAGS=$(TARGET) $(VERBOSITY)


EXEC=samtunes.bin
EXEC_PAYLOAD=payload.bin
LIB_SAM=libsam

COMMON = @../common/common.lst @../score/score.lst

OBJECTS = main.o
LIBS = @lib/libsam_sdcc_iy.lst

%.o: %.c $(PRAGMA_FILE)
	$(CC) $(CFLAGS) --list -o $@ $<

%.o: %.asm
	$(AS) $(ASFLAGS) --list -c -o $@ $<

all : $(EXEC)

#$(LIB_SAM) :
#	$(AS) $(ASFLAGS) --list -x -o $@ @lib/libsam_sdcc_iy.lst

$(EXEC_PAYLOAD) : $(OBJECTS) #$(LIB_SAM)
	$(CC) $(LDFLAGS) --list -startup=$(STARTUP) -m $(OBJECTS) $(COMMON) -o $(EXEC_PAYLOAD) -create-app

$(EXEC) : $(EXEC_PAYLOAD) bootstrap.asm
	$(ASM) -mz80 -l -m -b bootstrap.asm


.PHONY: clean
clean:
	rm -f *.o *.bin *.rom *.lib *.lis *.tap *.map zcc_opt.def *~ /tmp/tmpXX*

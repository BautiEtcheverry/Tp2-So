include Makefile.inc

KERNEL=kernel.bin

# Selección del memory manager en tiempo de compilación.
# Default: free list. `make MM=buddy` o `make buddy` desde la raíz para buddy system.
ifeq ($(MM),buddy)
MM_SRC=memory/buddy.c
GCCFLAGS+=-DUSE_BUDDY
else
MM_SRC=memory/free_list.c
endif

SOURCES=$(wildcard *.c cdrivers/*.c processes/*.c ipc/*.c) $(MM_SRC)
SOURCES_ASM=$(wildcard asm/*.asm)
OBJECTS=$(SOURCES:.c=.o)
OBJECTS_ASM=$(SOURCES_ASM:.asm=.o)
LOADERSRC=loader.asm


LOADEROBJECT=$(LOADERSRC:.asm=.o)
STATICLIBS=

all: $(KERNEL)

$(KERNEL): $(LOADEROBJECT) $(OBJECTS) $(STATICLIBS) $(OBJECTS_ASM)
	$(LD) $(LDFLAGS) -T kernel.ld -o $(KERNEL) $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_ASM) $(STATICLIBS)

%.o: %.c
	$(GCC) $(GCCFLAGS) -I./include -c $< -o $@

%.o : %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

$(LOADEROBJECT):
	$(ASM) $(ASMFLAGS) $(LOADERSRC) -o $(LOADEROBJECT)

clean:
	rm -rf asm/*.o *.o *.bin cdrivers/*.o memory/*.o processes/*.o ipc/*.o

.PHONY: all clean

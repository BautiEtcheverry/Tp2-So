include Makefile.inc

KERNEL=kernel.bin

# --- Memory manager: se compila UNO SOLO (interfaz comun: include/memory_manager.h) ---
#   make        -> manager elegido por el grupo
#   make buddy  -> buddy system
# Por eso memory/ NO entra por wildcard (compilaria los dos -> simbolos duplicados):
# se selecciona el .c segun MM.
# NOTA: hasta integrar el manager elegido, ambas ramas usan buddy.c. Cuando llegue,
#       cambiar la rama 'else' a memory/<elegido>.c.
MM ?= default
ifeq ($(MM),buddy)
  MM_SRC = memory/buddy.c
else
  MM_SRC = memory/buddy.c
endif

SOURCES=$(wildcard *.c cdrivers/*.c processes/*.c ipc/*.c) $(MM_SRC)
SOURCES_ASM=$(wildcard asm/*.asm)
OBJECTS=$(SOURCES:.c=.o)
OBJECTS_ASM=$(SOURCES_ASM:.asm=.o)
LOADERSRC=loader.asm


LOADEROBJECT=$(LOADERSRC:.asm=.o)
STATICLIBS=

all: $(KERNEL)

# Compila con el buddy system (make buddy)
buddy: MM := buddy
buddy: all

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

.PHONY: all buddy clean

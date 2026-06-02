# --- Makefile for Shell ---
include ../Makefile.inc

MODULE=0000-shell.bin
SOURCES=$(wildcard [^_]*.c)  $(wildcard tests/*.c)

LIBS=../asm/uland_lib_syscalls.a
all: $(MODULE)

$(MODULE): $(SOURCES) $(LIBS)
	$(GCC) $(GCCFLAGS) -T shell.ld _loader.c $(SOURCES) $(LIBS) -o ../$(MODULE)


$(OBJS):

$(LIBS):
	$(MAKE) -C ../asm all

clean:
	rm -rf *.o

.PHONY: all clean

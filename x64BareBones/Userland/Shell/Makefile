# --- Makefile for Shell ---
include ../Makefile.inc

MODULE=0000-shell.bin
SOURCES=$(wildcard [^_]*.c) \
        $(wildcard core/*.c) \
        $(wildcard commands/*.c) $(wildcard commands/builtins/*.c) $(wildcard commands/programs/*.c) \
        tests/test_util.c tests/syscall.c tests/test_mm.c tests/test_sync.c
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

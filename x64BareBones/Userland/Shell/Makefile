# --- Makefile for Shell ---
include ../Makefile.inc

MODULE=0000-shell.bin
SOURCES=$(wildcard [^_]*.c)
BENCH_SOURCES=../benchmarks/Float.c ../benchmarks/IO_latency.c ../benchmarks/Mem_Latency.c

TRON_SOURCES=../Tron/tron.c ../Tron/text.c ../Tron/counter.c
TRON_OBJECTS=$(patsubst ../Tron/%.c,%.o,$(TRON_SOURCES))


LIBS=../asm/uland_lib_syscalls.a
all: $(MODULE)

$(MODULE): $(SOURCES) $(BENCH_SOURCES) $(TRON_OBJECTS) $(LIBS)
	$(GCC) $(GCCFLAGS) -T shell.ld _loader.c $(TRON_OBJECTS) $(SOURCES) $(BENCH_SOURCES) $(LIBS) -o ../$(MODULE)

$(TRON_OBJECTS): %.o: ../Tron/%.c
	$(GCC) $(GCCFLAGS) -I.. -c $< -o $@

$(OBJS):

$(LIBS):
	$(MAKE) -C ../asm all

clean:
	rm -rf *.o $(TRON_OBJECTS)

.PHONY: all clean

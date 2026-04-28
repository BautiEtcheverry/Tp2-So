# --- Makefile for asm library
include ../Makefile.inc

LIB=uland_lib_syscalls.a
OBJS=syscalls.o excepTriggers.o

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

%.o : %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

clean:
	rm -f $(OBJS) $(LIB)

.PHONY: all clean

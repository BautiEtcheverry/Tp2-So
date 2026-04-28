# --- Makefile for Userland ---
include Makefile.inc

all: asm shell 

asm:
	$(MAKE) -C asm

shell: asm
	$(MAKE) -C Shell

clean:
	$(MAKE) -C Shell clean
	$(MAKE) -C asm clean
	rm -rf *.bin *.o *.a


.PHONY: shell all clean
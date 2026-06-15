
# Memory manager elegido en tiempo de compilacion (se propaga al Kernel):
#   make        -> manager elegido por el grupo
#   make buddy  -> buddy system
MM ?= default

all:  toolchain bootloader kernel userland image

# make buddy: rebuild completo seleccionando el buddy system
buddy: MM := buddy
buddy: all

toolchain:
	cd Toolchain; make all

bootloader:
	cd Bootloader; make all

kernel:
	cd Kernel; make all MM=$(MM)

userland:
	cd Userland; make all

image: toolchain kernel bootloader userland
	cd Image; make all

clean:
	cd Toolchain; make clean
	cd Bootloader; make clean
	cd Image; make clean
	cd Kernel; make clean
	cd Userland; make clean

.PHONY: toolchain bootloader image collections kernel userland all buddy clean

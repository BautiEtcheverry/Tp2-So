
all:  bootloader kernel userland image

buddy:
	$(MAKE) MM=buddy all

bootloader:
	cd Bootloader; make all

kernel:
	cd Kernel; $(MAKE) MM=$(MM) all

userland:
	cd Userland; make all

image: kernel bootloader userland
	cd Image; make all

clean:
	cd Bootloader; make clean
	cd Image; make clean
	cd Kernel; make clean
	cd Userland; make clean

.PHONY: bootloader image collections kernel userland all clean buddy

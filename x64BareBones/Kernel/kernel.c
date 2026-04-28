#include "include/libasm.h"
#include <stdint.h>
#include <lib.h>
#include <moduleLoader.h>
#include <naiveConsole.h>
#include <videoDriver.h>
#include <audioDriver.h>
#include <syscall.h>
#include <gfxConsole.h>
extern void syscall_gate_init();

extern uint8_t text;
extern uint8_t rodata;
extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

static const uint64_t PageSize = 0x1000;

// User modules packed in this order by Image/Makefile:
// 0: 0000-shell.bin              -> 0x400000
// 1: 0001-sampleCodeModule.bin   -> 0x500000
// 2: 0002-sampleDataModule.bin   -> 0x600000
static void *const shellModuleAddress = (void *)0x400000;
static void *const sampleCodeModuleAddress = (void *)0x500000;
static void *const sampleDataModuleAddress = (void *)0x600000;

typedef int (*EntryPoint)();

void clearBSS(void *bssAddress, uint64_t bssSize)
{
	memset(bssAddress, 0, bssSize);
}

void *getStackBase()
{
	return (void *)((uint64_t)&endOfKernel + PageSize * 8 // The size of the stack itself, 32KiB
					- sizeof(uint64_t)					  // Begin at the top of the stack
	);
}

// Initialize interrupts
// Initialize the PICs and enable CPU interrupts
void init_irqs(void) {
    picMasterMask(0xF9);   
    picSlaveMask(0xFF);    
    sti_enable();          // enable CPU interrupts
}

void *initializeKernelBinary()
{
	char buffer[10];

	ncPrint("[x64BareBones]");
	ncNewline();
	ncPrint("Init start");
	ncNewline();

	ncPrint("CPU Vendor:");
	ncPrint(cpuVendor(buffer));
	ncNewline();

	ncPrint("[Loading modules]");
	ncNewline();
	void *moduleAddresses[] = {
		shellModuleAddress,
		sampleCodeModuleAddress,
		sampleDataModuleAddress};

	loadModules(&endOfKernelBinary, moduleAddresses);
	ncPrint("[Done]");
	ncNewline();
	ncNewline();

	ncPrint("[Initializing kernel's binary]");
	ncNewline();

	clearBSS(&bss, &endOfKernel - &bss);

	ncPrint("  text: 0x");
	ncPrintHex((uint64_t)&text);
	ncNewline();
	ncPrint("  rodata: 0x");
	ncPrintHex((uint64_t)&rodata);
	ncNewline();
	ncPrint("  data: 0x");
	ncPrintHex((uint64_t)&data);
	ncNewline();
	ncPrint("  bss: 0x");
	ncPrintHex((uint64_t)&bss);
	ncNewline();

	ncPrint("[Done]");
	ncNewline();
	ncNewline();

	// Initialize syscall gate (int 0x80)
	syscall_gate_init();
	syscall_init();
	ncPrint("Syscall gate set");
	ncNewline();

	// Enable keyboard interrupts: unmask IRQ1 on PIC and enable IF
	init_irqs();
	  
	// Initialize audio driver (PC speaker / PCI probe)
	(void)audioInitDriver();

	return getStackBase();
}

int main()
{
	// Initialize graphics console if VESA LFB is active
	if (videoIsLFB())
	{
		gfx_init(0xFFFFFF, 0x202020);
	}

	ncPrint("[Kernel Main]");
	ncNewline();
	ncPrint("  Shell module at 0x");
	ncPrintHex((uint64_t)shellModuleAddress);
	ncNewline();
	ncPrint("  Jumping to shell...\n");
	((EntryPoint)shellModuleAddress)();
	
	return 0;
}

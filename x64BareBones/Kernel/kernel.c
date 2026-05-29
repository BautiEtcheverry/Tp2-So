#include "include/libasm.h"
#include "include/memory_manager.h"
#include "include/process.h"
#include "include/scheduler.h"
#include <gfxConsole.h>
#include <lib.h>
#include <moduleLoader.h>
#include <naiveConsole.h>
#include <stdint.h>
#include <syscall.h>
#include <videoDriver.h>
extern void syscall_gate_init();

extern uint8_t text;
extern uint8_t rodata;
extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

static const uint64_t PageSize = 0x1000;

// User modules packed in this order by Image/Makefile:
// 0: 0000-shell.bin       -> 0x400000
static void *const shellModuleAddress = (void *) 0x400000;

typedef int (*EntryPoint)();

void clearBSS(void *bssAddress, uint64_t bssSize) {
	memset(bssAddress, 0, bssSize);
}

void *getStackBase() {
	return (void *) ((uint64_t) &endOfKernel + PageSize * 8 // The size of the stack itself, 32KiB
					 - sizeof(uint64_t)						// Begin at the top of the stack
	);
}

// Initialize interrupts
// Initialize the PICs and enable CPU interrupts
void init_irqs(void) {
	picMasterMask(0xF8); // Antes era 0xF9 (IRQ0 maskeado, IRQ1 libre)
						 // 0xF8 = 11111000 → IRQ0 (timer) e IRQ1 (teclado) demascarados
	picSlaveMask(0xFF);
}

int idleMain(int argc, char **argv) {
	(void) argc;
	(void) argv;
	while (1)
		__asm__ volatile("hlt");
	return 0;
}
int shellMain(int argc, char **argv) {
	(void) argc;
	(void) argv;
	((EntryPoint) shellModuleAddress)(); // 0x400000, donde loadModules monta la shell
	return 0;
}

void *initializeKernelBinary() {
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
	void *moduleAddresses[] = {shellModuleAddress};

	loadModules(&endOfKernelBinary, moduleAddresses);
	ncPrint("[Done]");
	ncNewline();
	ncNewline();

	ncPrint("[Initializing kernel's binary]");
	ncNewline();

	clearBSS(&bss, &endOfKernel - &bss);

	ncPrint("  text: 0x");
	ncPrintHex((uint64_t) &text);
	ncNewline();
	ncPrint("  rodata: 0x");
	ncPrintHex((uint64_t) &rodata);
	ncNewline();
	ncPrint("  data: 0x");
	ncPrintHex((uint64_t) &data);
	ncNewline();
	ncPrint("  bss: 0x");
	ncPrintHex((uint64_t) &bss);
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

	// Inicialización del memory manager, sin importar si es el buddy o el que elegimos nosotros(no definimos cual
	// todavía).
	create_memory_manager((void *) HEAP_START, HEAP_SIZE);
	ncPrint("Memory manager initialized");
	ncNewline();
	return getStackBase();
}

int main() {
	// Initialize graphics console if VESA LFB is active
	if (videoIsLFB()) {
		gfx_init(0xFFFFFF, 0x202020);
	}

	ncPrint("[Kernel Main]");
	ncNewline();
	ncPrint("  Shell module at 0x");
	ncPrintHex((uint64_t) shellModuleAddress);
	ncNewline();
	ncPrint("  Jumping to shell...\n");
	// Crear proceso idle (se ejecuta cuando no hay otros ready)
	PCB *idleProc = createProcess("idle", idleMain, 0, NULL, 255, 0);
	initScheduler(idleProc);

	// Crear proceso shell (primer proceso de usuario)
	PCB *shellProc = createProcess("sh", shellMain, 0, NULL, 0, 1);
	addProcess(shellProc);

	sti_enable(); // Enable CPU interrupts. Lo haciamos en init_irqs, pero todavía no estaba inicializado el scheduler y
				  // se desreferenciaba un puntero con basura.

	// A partir de acá el scheduler toma el control via IRQ0
	// Este código nunca llega más allá del primer tick
	while (1) {
		__asm__ volatile("hlt");
	}
	((EntryPoint) shellModuleAddress)();

	return 0;
}

#ifndef LIBASM_H
#define LIBASM_H

#include <stdint.h>

//Implemented in asm/libasm.asm
uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t val);
uint32_t inl(uint16_t port);
void outl(uint16_t port, uint32_t val);

// CPU control helpers
void cpu_halt(void);

// Master PIC setup
// Enable/disable interrupts on the master PIC (IRQs 0..7)
void picMasterMask(uint8_t mask);

// Slave PIC setup
// Enable/disable interrupts on the slave PIC (IRQs 8..15)
void picSlaveMask(uint8_t mask);

// Enable interrupts
// sti sets the interrupt flag(IF) to 1, to enable interrupts.
void sti_enable(void);

#endif 

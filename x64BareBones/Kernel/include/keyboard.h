#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stddef.h>

// Low-level ISR entry (existing)
void keyboard_isr_handler(uint8_t scancode);
size_t kbd_available(void);
size_t kbd_read(char *buf, size_t len);

// Simple high-level API (matches user's header expectations)
void keyboard_irq_handler(void);
void keyboard_init(void);
char keyboard_getchar(void);
void keyboard_clear_buffer(void);
char keyboard_getchar_nonblocking(void);
int keyboard_has_key(void);

#endif

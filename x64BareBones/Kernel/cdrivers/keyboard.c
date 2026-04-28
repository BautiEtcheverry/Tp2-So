#include <keyboard.h>
#include <stdint.h>

//To avoid use in-line ASM(not allowed by the professors).
#include <libasm.h>
extern uint64_t capture_provisoria[19];
extern uint64_t capture_definitiva[19];

#define KBD_BUF_SIZE 256
static volatile char kbuf[KBD_BUF_SIZE];
static volatile unsigned int head = 0; // write
static volatile unsigned int tail = 0; // read

static const char keymap[128] = {
    [0x48] = 72, [0x50] = 80, [0x4D] = 77, [0x4B] = 75, [0x01] = 27, 
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4', [0x06] = '5', [0x07] = '6', [0x08] = '7', [0x09] = '8', [0x0A] = '9', [0x0B] = '0', [0x0C] = '-', [0x0D] = '=', 
    [0x0E] = '\b', [0x0F] = '\t', [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r', [0x14] = 't', [0x15] = 'y', [0x16] = 'u', [0x17] = 'i', [0x18] = 'o', [0x19] = 'p', [0x1A] = '[', [0x1B] = ']', [0x1C] = '\n', 
    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f', [0x22] = 'g', [0x23] = 'h', [0x24] = 'j', [0x25] = 'k', [0x26] = 'l', [0x27] = ';', [0x28] = '\'', [0x29] = '`', 
    [0x2B] = '\\', [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v', [0x30] = 'b', [0x31] = 'n', [0x32] = 'm', [0x33] = ',', [0x34] = '.', [0x35] = '/', [0x39] = ' '};

static const char shifted_keymap[128] = {
    [0x02] = '!', [0x03] = '@', [0x04] = '#', [0x05] = '$', [0x06] = '%', [0x07] = '^', [0x08] = '&', [0x09] = '*', [0x0A] = '(', [0x0B] = ')', [0x0C] = '_', [0x0D] = '+', [0x0E] = '\b', 
    [0x0F] = '\t', [0x10] = 'Q', [0x11] = 'W', [0x12] = 'E', [0x13] = 'R', [0x14] = 'T', [0x15] = 'Y', [0x16] = 'U', [0x17] = 'I', [0x18] = 'O', [0x19] = 'P', [0x1A] = '{', [0x1B] = '}', [0x1C] = '\n', 
    [0x1E] = 'A', [0x1F] = 'S', [0x20] = 'D', [0x21] = 'F', [0x22] = 'G', [0x23] = 'H', [0x24] = 'J', [0x25] = 'K', [0x26] = 'L', [0x27] = ':', [0x28] = '"', [0x29] = '~', [0x2B] = '|', 
    [0x2C] = 'Z', [0x2D] = 'X', [0x2E] = 'C', [0x2F] = 'V', [0x30] = 'B', [0x31] = 'N', [0x32] = 'M', [0x33] = '<', [0x34] = '>', [0x35] = '?', [0x39] = ' '};

// Modifier state
static volatile uint8_t shift_pressed = 0;
static volatile uint8_t caps_lock = 0;
static volatile uint8_t e0_prefix = 0;

//Handles the scanCode(converts to char) send by the irq handler(bellow) and it considers shifts and capsLock.
void keyboard_isr_handler(uint8_t scancode)
{
    // E0 prefix: extended scancodes.
    if (scancode == 0xE0){
        e0_prefix = 1;
        return;
    }

    if (e0_prefix){
        e0_prefix = 0;
        return;
    }

    // If key '0' make code (0x0B), store definitive snapshot. ( 0 is used to capture registers)
    if ((scancode == 0x0B && !shift_pressed)){
        for (int i = 0; i < 19; i++)
            capture_definitiva[i] = capture_provisoria[i];
           return;
    }

    // Key release
    if (scancode & 0x80){
        uint8_t code = scancode & 0x7F;
        if (code == 0x2A || code == 0x36)
        { // LSHIFT or RSHIFT release
            shift_pressed = 0;
        }
        return;
    }

    // Key press
    if (scancode == 0x2A || scancode == 0x36){ 
        // LSHIFT or RSHIFT press
        shift_pressed = 1;
        return;
    }
    if (scancode == 0x3A){
        // Caps Lock toggle
        caps_lock ^= 1;
        return;
    }

    char ch = 0;
    if (scancode < (sizeof(keymap) / sizeof(keymap[0]))){
        char base = keymap[scancode];
        char shft = shifted_keymap[scancode];
        if (base >= 'a' && base <= 'z'){
            // Letters: caps XOR shift -> uppercase
            if ((caps_lock ^ shift_pressed))
                ch = (char)(base - ('a' - 'A'));
            else
                ch = base;
        } else{
            // Non-letters: use shifted symbol if shift pressed
            ch = (shift_pressed && shft) ? shft : base;
        }
    }
    if (ch == 0)
        return;
    unsigned int next = (head + 1) % KBD_BUF_SIZE;
    if (next == tail)
        return; 
    kbuf[head] = ch;
    head = next;
}


//Function to which is jumped when the keyboard makes an IR.
void keyboard_irq_handler(void){
    // Read scancode from controller and dispatch to existing handler
    uint8_t scancode = inb(0x60);
    keyboard_isr_handler(scancode);
}

void keyboard_init(void){
    // Reset buffer indices and modifier state
    head = tail = 0;
    shift_pressed = 0;
    caps_lock = 0;
    e0_prefix = 0;
}

char keyboard_getchar(void){
    // Block until available
    while (kbd_available() == 0) {
       cpu_halt();
    }
    char c;
    (void)kbd_read(&c, 1);
    return c;
}

void keyboard_clear_buffer(void){
    tail = head; // drop pending keys
}

char keyboard_getchar_nonblocking(void){
    if (kbd_available() == 0)
        return 0;
    char c;
    (void)kbd_read(&c, 1);
    return c;
}

int keyboard_has_key(void){
    return (kbd_available() > 0) ? 1 : 0;
}

size_t kbd_available(void){
    if (head >= tail)
        return head - tail;
    return KBD_BUF_SIZE - (tail - head);
}

size_t kbd_read(char *buf, size_t len){
    size_t n = 0;
    while (n < len && tail != head)
    {
        buf[n++] = kbuf[tail];
        tail = (tail + 1) % KBD_BUF_SIZE;
    }
    return n;
}

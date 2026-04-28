// Minimal print of definitive registers stored by IRQ handler and keyboard hook
/*
    I think we should move this to UserLand. CHECK!
*/ 
#include <gfxConsole.h>
extern unsigned long long capture_definitiva[19];

static void print_hex64(uint64_t v)
{
    char buf[19];
    static const char hex[] = "0123456789ABCDEF";
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < 16; i++)
    {
        buf[2 + i] = hex[(v >> (4 * (15 - i))) & 0xF];
    }
    buf[18] = 0;
    gfx_write(buf, 18);
}

void regs_print(void)
{
    gfx_write("Registers:\n", 11);
    gfx_write("RAX=", 4);
    print_hex64(capture_definitiva[0]);
    gfx_write("\n", 1);
    gfx_write("RBX=", 4);
    print_hex64(capture_definitiva[1]);
    gfx_write("\n", 1);
    gfx_write("RCX=", 4);
    print_hex64(capture_definitiva[2]);
    gfx_write("\n", 1);
    gfx_write("RDX=", 4);
    print_hex64(capture_definitiva[3]);
    gfx_write("\n", 1);

    gfx_write("RSI=", 4);
    print_hex64(capture_definitiva[4]);
    gfx_write("\n", 1);
    gfx_write("RDI=", 4);
    print_hex64(capture_definitiva[5]);
    gfx_write("\n", 1);
    gfx_write("RBP=", 4);
    print_hex64(capture_definitiva[6]);
    gfx_write("\n", 1);
    gfx_write("RSP=", 4);
    print_hex64(capture_definitiva[7]);
    gfx_write("\n", 1);

    gfx_write("R8=", 3);
    print_hex64(capture_definitiva[8]);
    gfx_write("\n", 1);
    gfx_write("R9=", 3);
    print_hex64(capture_definitiva[9]);
    gfx_write("\n", 1);
    gfx_write("R10=", 4);
    print_hex64(capture_definitiva[10]);
    gfx_write("\n", 1);
    gfx_write("R11=", 4);
    print_hex64(capture_definitiva[11]);
    gfx_write("\n", 1);

    gfx_write("R12=", 4);
    print_hex64(capture_definitiva[12]);
    gfx_write("\n", 1);
    gfx_write("R13=", 4);
    print_hex64(capture_definitiva[13]);
    gfx_write("\n", 1);
    gfx_write("R14=", 4);
    print_hex64(capture_definitiva[14]);
    gfx_write("\n", 1);
    gfx_write("R15=", 4);
    print_hex64(capture_definitiva[15]);
    gfx_write("\n", 1);

    gfx_write("RIP=", 4);
    print_hex64(capture_definitiva[16]);
    gfx_write("\n", 1);
    gfx_write("CS=", 3);
    print_hex64(capture_definitiva[17]);
    gfx_write("\n", 1);
    gfx_write("RFLAGS=", 7);
    print_hex64(capture_definitiva[18]);
    gfx_write("\n", 1);
}

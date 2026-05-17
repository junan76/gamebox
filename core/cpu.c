#include <stdint.h>

struct cpu_struct
{
    /*
     * Register file of CPU, ONLY support little-endian for now.
     */
    struct
    {
        union
        {
            uint16_t af;
            struct
            {
                uint8_t __padding : 4;
                uint8_t c_flag : 1;
                uint8_t h_flag : 1;
                uint8_t n_flag : 1;
                uint8_t z_flag : 1;
                uint8_t a;
            };
        };
        union
        {
            uint16_t bc;
            struct
            {
                uint8_t c;
                uint8_t b;
            };
        };
        union
        {
            uint16_t de;
            struct
            {
                uint8_t e;
                uint8_t d;
            };
        };
        union
        {
            uint16_t hl;
            struct
            {
                uint8_t l;
                uint8_t h;
            };
        };
        uint16_t sp;
        uint16_t pc;
    } regs;

    /*Interrupt master enable flag*/
    uint8_t ime;
    /*
     * Interrupt enable, reg address: 0xFFFF
     * bit 0: vblank
     * bit 1: lcd
     * bit 2: timer
     * bit 3: serial
     * bit 4: joypad
     * bits[7:5]: NONE
     */
    uint8_t ie;

    uint8_t halted;
    uint8_t stopped;
};

static struct cpu_struct cpu;
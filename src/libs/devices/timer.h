#pragma once
#include <stdint.h>
#include <x86/io.h>

/* Intel 8253 and 8254 are Programable Interval Timers (PITs) designed
 * for microprocessors to perform timing and counting functions using
 * three 16-bit registers. each counter has 2 input pins, i.e. Clock &
 * Gate, and 1 pin for 'OUT' output. to operate a counter, a 16-bit count
 * is loaded in its register. on command, it begins to decrement the 
 * count until it reaches 0, then it generate a pulse that can be used to
 * interrupt the CPU.
 * 
 * there are 3 counters (or timers), which are labeled as 'counter 0',
 * 'counter 1' and 'counter 2'. Each counter has 2 input pins – 'CLK'
 * (clock input) and 'GATE' – and 1-pin, 'OUT', for data output. The 3
 * counters are 16-bit down counters independent of each other, and can
 * be easily read by the CPU.
 * 
 * In the original IBM PCs, the first counter (selected by setting 
 * A1=A0=0, by set Control Word Register) is used to generate a
 * timekeeping interrupt. The second counter (A1=0, A0=1) is used to
 * trigger the refresh of DRAM memory. The last counter (A1=1, A0=0) is
 * used to generate tones via the PC speaker.
 * 
 * Besides the counters, a typical Intel 8253 microchip also contains the
 * following components:
 * 
 * Data/Bus Buffer 
 * This block contains the logic to buffer the data bus to/from the
 * microprocessor, and to the internal registers. It has 8 input pins,
 * usually labelled as D7..D0, where D7 is the MSB.
 * 
 * Read/Write Logic
 * The Read/Write Logic block has 5 pins, which are listed below. Notice
 * that "/X" denotes an active low signal.
 * /RD: read signal
 * /WR: write signal
 * /CS: chip select signal
 * A0, A1: address lines
 * 
 * Operation mode of the PIT is changed by setting the above hardware
 * signals. For example, to write to the Control Word Register, one needs
 * to set /CS=0, /RD=1, /WR=0, A1=A0=1.
 * 
 * Control Word Register
 * Port 43h R/W
 * Port 53h R/W – second chip *
 *
 * OPERATION MODEs:
 * The D3, D2, and D1 bits of the Control Word set the operating mode of
 * the timer. There are 6 modes in total; for modes 2 and 3, the D3 bit
 * is ignored, so the missing modes 6 and 7 are aliases for modes 2 
 * and 3. Notice that, for modes 0, 2, 3 and 4, GATE must be set to HIGH
 * to enable counting. For mode 5, the rising edge of GATE starts the
 * count.
 * Mode 0 (000): Interrupt on Terminal Count
 * Mode 1 (001): Programmable One Shot
 * Mode 2 (X10): Rate Generator
 *               In this mode, the device acts as a divide-by-n counter,
 *               which is commonly used to generate a real-time clock
 *               interrupt. Like other modes, counting process will start
 *               the next clock cycle after COUNT is sent. OUT will then
 *               remain high until the counter reaches 1, and will go low
 *               for one clock pulse. OUT will then go high again, and
 *               the whole process repeats itself.
 *               The time between the high pulses depends on the preset
 *               count in the counter's register, and is calculated using
 *               the following formula:
 *               value to be loaded into counter = finput/foutput
 *               Note that the values in the COUNT register range from n
 *               to 1; the register never reaches zero.
 * Mode 3 (X11): Square Wave Generator
 * Mode 4 (100): Software Triggered Strobe
 * Mode 5 (101): Hardware Triggered Strobe
 * 
 * Count 0 at port 40h (0x40)
 * Count 1 at port 41h (0x41)
 * Count 2 at port 42h (0x42)
 * 
 * 
 */

// Programable Interval Timer 8253
class pit8253
{
public:
    enum
    {
        PIT_CTRL_PORT   = 0x43,
        PIT_CNTR0_PORT  = 0x40,
        PIT_CNTR1_PORT  = 0x41,
        PIT_CNTR2_PORT  = 0x42,
        PIT_INPUT_FREQ  = 1193181,
        PIT_RW_LATCH    = 3,
        PIT_OPT_MODE    = 2
    };
public:
    static void
    freq(uint16_t fq) {
        // frequency cannot be zero
        fq = fq == 0 ? 1 : fq;
        // write control word register: port 0x43
        x86_io::outb(
            PIT_CTRL_PORT, 
            (uint8_t)
            (PIT_CNTR0_PORT << 6 |
            PIT_RW_LATCH    << 4 |
            PIT_OPT_MODE    << 1));
        
        uint16_t cnt = PIT_INPUT_FREQ / fq;

        x86_io::outb(PIT_CNTR0_PORT, (uint8_t)cnt);
        x86_io::outb(PIT_CNTR0_PORT, (uint8_t)(cnt>>8));
    }
};
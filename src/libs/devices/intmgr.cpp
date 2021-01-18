#include <intmgr.h>
#include <print.h>

using namespace lkl;

const char* s_irq_names[] = 
{
    "00h #DE: divided-by-zero error",
    "01h #DB: debug",
    "02h #NMI:  non-maskable interrupt",
    "03h #BP breakpoint",
    "04h #OF overflow",
    "05h #BR bound range exceeded",
    "06h #UD undefined opcode",
    "07h #NM device not available",
    "08h #DF double fault",
    "09h #unknown",
    "0Ah #TS invlaid TSS",
    "0Bh #NP segment not present",
    "0Ch #SS stack-segment fault",
    "0Dh #GP general protection fault",
    "0Eh #PF page fault",
    "0Fh #unknown",
    "10h #MF x87 floating-point exception",
    "11h #AC alignment check",
    "12h #MC machine check",
    "13h #XM SIMD floating-point exception",
    "14h #VE virtualization exception",
    "15h #unknown",
    "16h #unknown",
    "17h #unknown",
    "18h #unknown",
    "19h #unknown",
    "1Ah #unknown",
    "1Bh #unknown",
    "1Ch #unknown",
    "1Dh #unknown",
    "1Eh #SX security exception",
    "1Fh #unknown",
    "20h Device: Timer",
    "21h Device: Keyboard",
    "22h Device: cascade PIC",
    "23h Device: COM2",
    "24h Device: COM1",
    "25h Device: Parallel Port 2 (LPT2)",
    "26h Device: Floppy",
    "27h Device: Parallel Port 1 (LPT1)",
    "28h Device: CMOS real time clock",
    "29h Device: unknown",
    "2Ah Device: unknown",
    "2Bh Device: unknown",
    "2Ch Device: PS/2 Mouse",
    "2Dh Peripheral: FPU exception",
    "2Eh Peripheral: HDD1",
    "2Fh Peripheral: HHD2",
};

const char* s_err_irq = "error irq";

const char*
intr_mgr::irq_to_name(uint32_t vct) {
    if(vct < SYSTEM_IRQ_COUNT) {
        return s_irq_names[vct];
    }
    return s_err_irq;
}
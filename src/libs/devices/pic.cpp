#include <pic.h>
#include <print.h>

using namespace lkl;



void pic8259a::init()
{
    // OS only needs initialization once
    if(_initialized)
        return;
    //-------------------------------------------------------------------
    // INITIALIZE MASTER 8259A
    // ICW 1: port 0x20
    // Bit 0: IC4
    //        1 = ICW4 needed, 0 = no ICW4 needed, x86: 1
    // Bit 1: Single
    //        1 = single, 0 = cascade mode (has slave(s))
    // Bit 2: for 8025, x86: 0
    // Bit 3: 1 = level triggered mode, 0 = edge triggered mode
    // Bit 4: always 1 on x86
    // Bit 5-7: for 8085, x86: 0
    x86_io::outb(PIC_M_CTRL, 0b00010001); // bit 4 and bit 0
    
    // ICW 2: port 0x21/0xA1
    // 3-7 bits indicate starting interrupt vector (SIV)
    // 0-2 bits indicate current interrupt vector (CIV)
    // the real interrupt vector = SIV + CIV
    // 8259A only needs to know SIV
    // NOTICE: we specify 0x20 (decimal:32, binary: 0b00100000)
    // it doesn't mean that most-significant 5 bits 0xb00100 indicates
    // that our starting 'interrupt-vector' is 0b00100 = 4.
    // it says, our starting 'interrupt-vector' is 32.
    // you cannot specify a number like 0b0000'1111 = 15, 3 least
    // significant bits are ignored and will be filled by 8259A.
    x86_io::outb(PIC_M_DATA, 0b0010'0000); // 32
    
    // ICW 3 (Master): port 0x21/0xA1
    // any bit set 1 indicates that IR input has a slave
    x86_io::outb(PIC_M_DATA, 0b0000'0100);
    
    // ICW 4:
    // only bit we need to set for now is: μPM = 1 = 8086/8088 mode
    x86_io::outb(PIC_M_DATA, 0b0000'0001);

    //-------------------------------------------------------------------
    // INITIALIZE SLAVE 8259A
    // ICW 1: same as master
    x86_io::outb(PIC_S_CTRL, 0x11);

    // ICW 2: 0x28 = 0b0010'1000 = 40
    // starting 'interrupt-vector' is 40
    x86_io::outb(PIC_S_DATA, 0x28);

    // ICW 3: which master IR input the slave 8259A is connecting.
    x86_io::outb(PIC_S_DATA, 0x02);

    // ICW 4: same as master
    x86_io::outb(PIC_S_DATA, 0x01);

    // OCW 1: 1 indicates the channel is masked
    //        0 indicates the channel is enabled.
    // all irq masked at beginning.
    x86_io::outb(PIC_M_DATA, 0b1111'1111);

    // Slave OCW 1: all channel masked
    x86_io::outb(PIC_S_DATA, 0b1111'1111);

    _initialized = true;
}


// void main_cxx_isr(uint32_t no) {
//     ASSERT(no < interrupt<x86_asm>::IDT_ENT_CNT);
    
//     auto han = interrupt<x86_asm>::s_isrs[no];

//     if(han != nullptr) {
//         han(no);
//     } else {
//         interrupt<x86_asm>::display_name(no);
//     }
//}



#include <x86/asm.h>
#include <x86/io.h>

// #include <x86/idt.h>
#include <kbd.h>
#include <inbb.h>

lkl::inbb_t g_kbd_buffer;


uint8_t keyboard::s_states    = 0;
lkl::kbdms_t keyboard::s_modifiers;

void
keyboard::init()
{
    // interrupt<x86_asm>::reg(
    //     pic8259a::DEV_KEYBOARD,
    //     keyboard_handler
    // );
}

void
keyboard::keyboard_handler(uint32_t vct) 
{
    
    auto sc = x86_io::inb(KBD_PORT);
    if(s_modifiers.update(sc))
        return;
    
    lkl::scode_t scode(sc, s_modifiers);
    //char ch = scode.to_char();
    g_kbd_buffer.putc(scode);
    
}

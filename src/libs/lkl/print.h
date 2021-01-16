#pragma once
#include <stdint.h>
#include <debug.h>
#include <lkl.h>

ns_lite_kernel_lib_begin

/*
 * COLOR attribute:
 *   7   6   5   4   3   2   1   0
 * ┌───┬───┬───┬───┬───┬───┬───┬───┐
 * │ B │ r │ g │ b │ I │ R │ G │ B │
 * └───┴───┴───┴───┴───┴───┴───┴───┘
 *
 * 7 bit:    blinking mode
 *           0     =   not blinking
 *           1     =   blinking
 *
 * 4-6 bits: background color
 *           000   =   black
 *           001   =   blue
 *           010   =   green
 *           011   =   cyan
 *           100   =   red
 *           101   =   magenta  
 *           110   =   brown
 *           111   =   light gray
 *
 * 0-3 bits: character color 
 *           0000  =   Black	
 *           0001  =   Blue	
 *           0010  =   Green	
 *           0011  =   Cyan	
 *           0100  =   Red	
 *           0101  =   Magenta	
 *           0110  =   Brown	
 *           0111  =   Light Gray	
 *           1000  =   Dark Gray	
 *           1001  =   Light Blue	
 *           1010  =   Light Green	
 *           1011  =   Light Cyan	
 *           1100  =   Light Red	
 *           1101  =   Light Magenta	
 *           1110  =   Yellow	
 *           1111  =   White
 * 	
 * by default, there are 16 colors for text and only 8 colors for
 * background. There is a way to get all the 16 colors for background,
 * which requires turning off the "blinking attribute".
 */

class color_t
{
private:
    uint8_t _color;

public:
    enum mask
    {
        MASK_BLINK       = 0X80,  // 10000000B
        MASK_BCOLOR      = 0X70,  // 01110000B
        MASK_FCOLOR      = 0x0F   // 00001111b
    };

    enum bcolor_t : uint8_t
    {
        B_BLACK         = 0x00, // 00000000b
        B_BLUE          = 0x10, // 00010000b
        B_GREEN         = 0x20, // 00100000b
        B_CYAN          = 0x30, // 00110000b
        B_RED           = 0x40, // 01000000b
        B_MAGENTA       = 0x50, // 01010000b
        B_BROWN         = 0x60, // 01100000b
        B_LIGHT_GRAY    = 0x70  // 01110000b
    };

    enum fcolor_t : uint8_t
    {
        F_BLACK         = 0x00, // 00000000b
        F_BLUE          = 0x01, // 00000001b
        F_GREEN         = 0x02, // 00000010b
        F_CYAN          = 0x03, // 00000011b
        F_RED           = 0x04, // 00000100b
        F_MAGENTA       = 0x05, // 00000101b
        F_BROWN         = 0x06, // 00000110b
        F_LIGHT_GRAY    = 0x07, // 00000111b
        F_DARK_GRAY     = 0x08, // 00001000b
        F_LIGHT_BLUE    = 0x09, // 00001001b
        F_LIGHT_GREEN   = 0x0A, // 00001010b
        F_LIGHT_CYAN    = 0x0B, // 00001011b
        F_LIGHT_RED     = 0x0C, // 00001100b
        F_LIGHT_MAGENTA = 0x0D, // 00001101b
        F_YELLOW        = 0x0E, // 00001110b
        F_WHITE         = 0x0F  // 00001111b
    };

    color_t(uint8_t c = F_WHITE) {
        _color = c;
    }

    color_t(fcolor_t fc, bcolor_t bc) {
        set_fcolor(fc);
        set_bcolor(bc);
    }

    void toggle_blink() {
        _color ^= MASK_BLINK;
    }

    void set_blink(bool b) { 
        _color = b ?
        _color | MASK_BLINK :
        _color & ~MASK_BLINK;
    }

    void set_color(bcolor_t bc, fcolor_t fc) {
        set_bcolor(bc);
    }

    void set_bcolor(bcolor_t bc) {
        _color = (_color & MASK_FCOLOR) + bc;
    }

    void set_fcolor(fcolor_t fc) {
        _color = (_color & MASK_BCOLOR) + fc;
    }

    uint8_t get_bcolor() const {
        return _color & MASK_BCOLOR;
    }

    uint8_t get_fcolor() const {
        return _color & MASK_FCOLOR;
    }    

    bool is_blinking() {
        return _color & MASK_BLINK != 0;
    }

    operator uint8_t() const {
        return _color;
    }

    const color_t& operator=(uint8_t c) {
        _color = c;
        return *this;
    }
};

class char_t
{
public:
    enum
    {
        VISIBLE_CHAR_START  = 0x20,
        VISIBLE_CHAR_END    = 0x7e,

        CH_NULL             = 0x00,
        CH_BELL             = 0x07,
        CH_BACKSPACE        = 0x08,
        CH_HORI_TAB         = 0x09,
        CH_LINE_FEED        = 0x0A,
        CH_RETURN           = 0x0D,
        CH_ESCAPE           = 0x1B,
        CH_SPACE            = 0x20
    };

    char _char;

    char_t(uint8_t ch = 0) {
        _char = ch;
    }
    
    bool is_null() const { 
        return _char == CH_NULL; 
    }

    bool is_visible() const {
        return _char >= VISIBLE_CHAR_START && 
               _char < VISIBLE_CHAR_END;
    }

    bool is_backspace() const {
        return _char == CH_BACKSPACE;
    }

    bool is_line_feed() const {
        return _char == CH_LINE_FEED ||
               _char == CH_RETURN;
    }

    operator uint8_t() const { return _char; }

    const char_t& operator=(uint8_t ch) {
        _char = ch;
        return *this;
    }
};

class col_char_t
{
public:
    enum mask
    {
        MASK_CHAR   = 0x00FF,
        MASK_COLOR  = 0xFF00
    };

    char _char;

    color_t _color = color_t(color_t::F_WHITE, color_t::B_BLACK);

    char_t read_char(uint16_t cc) {
        return (cc&MASK_CHAR);
    }

    color_t read_color(uint16_t cc) {
        return (cc&MASK_COLOR)>>8;
    }

    enum
    {
        INVISIBLE_CHAR = 0x0F00,
    };

    col_char_t(char_t ch, color_t col) {
        _char  = ch;
        _color = col;
    }

    const col_char_t& operator=(uint16_t cc) {
        _char  = read_char(cc);
        _color = read_color(cc);
        return *this; 
    }

    operator uint16_t() const {
        return ((uint16_t)_color << 8) + _char;
    }
};


/*
 * Using template makes code more flexible, testable.
 * 
 * Cannot access those constant template parameters from outside like 
 * screen::CPL unless we re-define them inside the class.
 * 
 * Before 'paging' enabled, kernel uses physical addresses, but those
 * addresses may be changed later (virtual address).
 * screen also has different resolutions.
 */
template<uint16_t _CPL, uint16_t _LPS, uint32_t _BUF>
class screen_t
{
    uint16_t* _buf = (uint16_t*)_BUF;
public:
    enum
    {
        // CPL: chars per line
        CPL = _CPL,

        // LPS: lines per screen
        LPS = _LPS,

        // CPS: chars per screen
        CPS = CPL * LPS,

        // text mode video buffer
        BUF_ADDR = (uint32_t)_BUF
    };

    // test if linear position of cursor in range
    static inline bool
    out_of_screen(int16_t pos) {
        return pos < 0 || pos > CPS - 1;
    }

    // test if cursor coodinates in screen
    static inline bool
    out_of_screen(int16_t x, int16_t y) {
        return x >= CPL || y >= LPS;
    }

    // text mode video buffer address
    uint16_t* buffer() {
        return (uint16_t*)BUF_ADDR;
    }

    // const versio of text mode video buffer by providing an index
    const uint16_t* buffer() const {
        return (uint16_t*)BUF_ADDR;
    }

    // access element of text buffer
    uint16_t& operator[](int16_t idx) {
        ASSERT(!out_of_screen(idx));
        return _buf[idx];
    }

    // const version method to access text buffer by providing an index
    const uint16_t& operator[](int16_t idx) const {
        ASSERT(!out_of_screen(idx));
        return _buf[idx];
    }

    void fill(uint16_t start, uint16_t len, uint16_t val) {
        if(start >= CPS || len == 0)
            return;
        
        uint16_t end = start + len >= CPS ? 
                       CPS :
                       start + len;

        for(uint16_t i = start; i < end; ++i) {
            _buf[i] = val;
        }
    }

    void copy(uint16_t* dst, uint16_t start, uint16_t len) {
        if(start >= CPS || len == 0)
            return;
        
        len = (start + len >= CPS ? CPS : start + len) - start;

        for(uint16_t i = 0; i < len; ++i) {
            dst[i] = _buf[i + start];
        }
    }

    // only copy characters
    void copy_text(char* dst, uint16_t start, uint16_t len) {
        if(start >= CPS || len == 0)
            return;
        
        len = ((start + len >= CPS ? CPS : start + len) - start) * 2;
        char* _tex =  (char*)&_buf[start];
        start = 0;
        for(uint16_t i = 0; i < len; i+=2) {
            dst[start++] = _tex[i];
        }
    }

    // fill screen with invisible chars + background color
    void clear(uint8_t bgc = 0x0F) {
        fill(0, CPS, (uint16_t)bgc << 8);
    }

    void scroll_up(uint32_t lines) {
        if(lines == 0 || lines > 24) {
            clear();
            return;
        }

        uint16_t  total = lines * CPL;
        int count = CPS - total;
        copy(_buf, total, count);
        fill(count, CPS - count, 0);
    }
};

using def_screen_t = screen_t<80, 25, 0xb8000>;



/*
 * NOTICE:
 * function: 'update' without parameter used to READ cursor position
 * function: 'update' with parameter(s) used to WRITE cursor position
 */
template<typename screen, typename isa>
struct cursor_t {
private:
    int16_t _x;
    int16_t _y;

public:

    // read cursor position from screen
    // return an offset from top-left
    int16_t update() {
        isa::outb(0x03d4, 0x0e);
        uint16_t val = isa::inb(0x03d5) << 8;
        isa::outb(0x03d4, 0x0f);
        val |= isa::inb(0x03d5);
        _x = val % screen::CPL;
        _y = val / screen::CPL;
        return (int16_t)val;
    }

    // write an absolute position of cursor
    void update(int16_t pos) {
        if(screen::out_of_screen(pos))
            return;

        isa::outb(0x03d4, 0x0e);
        isa::outb(0x03d5, (uint8_t)(pos >> 8));
        isa::outb(0x03d4, 0x0f);
        isa::outb(0x03d5, (uint8_t)(pos & 0xff));
    }

    // write cursor position if it's legal
    void update(int16_t x, int16_t y) {
        update(y * screen::CPL + x);
    }

    // return true if screen needs to scroll up.
    bool increase() {
        update();
        int16_t x = _x + 1 >= screen::CPL ? 0 : _x + 1;
        int16_t y = x == 0 ? _y + 1 : _y;
        
        // write new position
        update(x, y >= screen::LPS ? screen::LPS - 1 : y);
        return y >= screen::LPS;
    }

    // return nothing, cuz only support scroll up by now
    void decrease() {
        update();
        int16_t x = _x - 1 < 0 ? screen::CPL - 1 : _x;
        int16_t y = x == screen::CPL - 1 ? _y - 1 : _y;
        update(x, y < 0 ? 0 : y);
    }

    // return true if screen needs to scroll up
    bool line_feed() {
        update();
        int16_t y = _y + 1;
        update(0, y >= screen::LPS ? screen::LPS - 1 : y);
        return y >= screen::LPS;
    }

    // writing any new cursor position won't cause _x, _y change
    // caller should never trust get_x()/get_y() unless 
    // 'void update()' invoked before
    int16_t get_x() const { return _x; }

    // ensure 'void update()' invoked before this.
    int16_t get_y() const { return _y; }
};


template<typename screen, typename isa>
class print_t
{
private:
    screen                  _scr;
    cursor_t<screen, isa>   _cur;
    color_t                 _def_color = color_t::F_WHITE |
                                         color_t::B_GREEN;
public:
    print_t(color_t col = color_t::F_WHITE | color_t::B_BLACK) :
        _def_color(col) {

    }

    color_t set_default_color(color_t col) {
        color_t old = _def_color;
        _def_color = col;
        return old;
    }

    color_t get_default_color() const {
        return _def_color;
    }

    // put_char only write visible chars
    void put_char(char_t ch, color_t col = 0) {
        if(ch.is_visible())
            _scr[_cur.update()] = 
            col_char_t(ch, col == 0 ? _def_color : col);
    }

    // won't cause cursor moving.
    // only display visible chars
    void put_char(uint16_t x,
                  uint16_t y,
                  char_t ch,
                  color_t col = 0) {
        if(_scr.out_of_screen(x, y))
            return;
       
        if(ch.is_visible())
            _scr[y * screen::CPL + x] = 
            col_char_t(ch, col == 0 ? _def_color : col);
    }

    // write visible chars, line-feed or backspace at current corsor position
    // move cursor to next/previous position
    void add_char(char_t ch, color_t col = 0) {
        _cur.update();

        add_char(_cur.get_x(),
                 _cur.get_y(),
                 ch,
                 col == 0 ? _def_color : col);
    }

    // write visible chars, line-feed or backspace at specific position (x, y)
    // move cursor to right position
    void add_char(uint16_t x, uint16_t y, char_t ch, color_t col = 0)
    {
        if(_scr.out_of_screen(x, y))
            return;

        col = col == 0 ? _def_color : col;

        if(ch.is_visible()) {
            put_char(x, y, ch, col);
            if(_cur.increase()) {
                _scr.scroll_up(1);
            }
        } else if(ch.is_line_feed()) {
            if(_cur.line_feed()) {
                _scr.scroll_up(1);
            }
        } else if(ch.is_backspace()) {
            int16_t pos = _cur.update();

            if(pos < 1)
                return;

            // clear char value, update color            
            while(pos--) {
                _scr[pos] = (uint16_t)col << 8;
                if((_scr[pos] & 0xFF00) != 0) {
                    break;
                }
            }

            _cur.update(pos < 0 ? 0 : pos);
        }
    }

    void show(int32_t num) {
        if(num == 0x80000000) {
            show("-2147483648");
            return;
        }

        if(num <= 0) {
            add_char('-');
            num = ~num + 1;
        }

        show((uint32_t)num);
    }

    void show(uint32_t num) {
        if(num/10) {
            show(num/10);
        }
        add_char('0' + num%10);
    }

    void hex(uint32_t num, bool pfx = false, bool fill0 = true) {
        char tmp = 0;
        char buf[9] = { '0','0','0','0','0','0','0','0', 0};
        char idx = 7;
        while(num > 0) {
            tmp = num % 16;
            buf[idx--] = tmp > 9 ? 0x41 + tmp - 0x0a : '0' + tmp;
            num /= 16;
        }

        idx = 0;
        if(fill0 == false) {
            for(; idx < 7; ++idx) {
                if(buf[idx] != '0')
                    break;
            }
        }

        if(pfx) {
            show("0x");
        }

        show(&buf[idx]);
    }

    void show(const char* str) {
        if(str == nullptr)
            return;
        
        int i = 0;
        while(str[i] != 0) {
            add_char(str[i++]);
        }
    }

    void line_feed() {
        add_char('\n');
    }

    auto& get_cursor() {
        return _cur;
    }

    screen& get_screen() {
        return _scr;
    }
};

ns_lite_kernel_lib_end
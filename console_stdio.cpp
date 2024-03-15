
#include <pico/stdio.h>
#include <pico/stdio/driver.h>
#include "console.hpp"
#include "hid_app.hpp"

extern VGAConsole *console;
extern HIDHost *usb_hid;

void vga_out_chars(const char *buf, int len)
{
    if (!console) return;
    
    for (int i=0; i<len; i++) {
        console->add_char(buf[i]);
    }
}

int vga_in_chars(char *buf, int len)
{
    if (!usb_hid) return 0;
    
    int count = 0;
    while (count < len) {
        int ch = usb_hid->get_buf_key();
        if (ch >= 0) {
            *buf++ = ch;
            count++;
        } else {
            return count;
        }
    }
    return count;
}

stdio_driver_t stdio_vga = {
    .out_chars = vga_out_chars,
    .in_chars = vga_in_chars,
    //.set_chars_available_callback = stdio_usb_set_chars_available_callback,
#if PICO_STDIO_ENABLE_CRLF_SUPPORT
    .crlf_enabled = PICO_STDIO_DEFAULT_CRLF
#endif
};

void init_stdio_vga()
{
    stdio_set_driver_enabled(&stdio_vga, true);
}

#if 0
struct stdio_driver {
    void (*out_chars)(const char *buf, int len);
    void (*out_flush)(void);
    int (*in_chars)(char *buf, int len);
    void (*set_chars_available_callback)(void (*fn)(void*), void *param);
    stdio_driver_t *next;
#if PICO_STDIO_ENABLE_CRLF_SUPPORT
    bool last_ended_with_cr;
    bool crlf_enabled;
#endif
};
#endif
#include <stdio.h>
#include "pico/stdlib.h"
#include <string.h>
#include "pico/multicore.h"
#include "bsp/board.h"
#include "tusb.h"

#include "video.hpp"
#include "graphics.hpp"
#include "vgaterm.hpp"
#include "mouse.hpp"
#include "console.hpp"
#include "hid_app.hpp"
#include "console_stdio.hpp"

VGAVideo *video = 0;
VGAGraphics *graphics = 0;
VGATerm *term = 0;
VGAConsole *console = 0;
VGAMouse *mouse = 0;
extern HIDHost *usb_hid;

extern void hblank_isr();
extern void hid_app_task();

// Entry point for second CPU
void core1_entry()
{
    printf("Core 1 start\n");
    
    // Configure VGA here so that IRQs get routed to core1
    video = new VGAVideo(hblank_isr);
    graphics = new VGAGraphics(video);
    term = new VGATerm(graphics);
    mouse = new VGAMouse(graphics);
    console = new VGAConsole(term, mouse);
    video->start();
    init_stdio_vga();

    printf("Starting USB\n");
    usb_hid = new HIDHost();
    // usb_hid->setKeyReceiver(console);
    usb_hid->setMouseReceiver(mouse);
    usb_hid->start();
    
    console->add_task(hid_app_task);
    console->add_task(tuh_task);
    console->console_loop();
}

#if 0
void draw_test_pattern()
{
    uint8_t c = 0;
    for (int y=0; y<480; y++) {
        for (int x=0; x<640; x+=2) {
            c = (x>>4) + (y>>4);
            c &= 15;
            uint8_t v = 0;
            v |= c;
            v |= c<<4;
            line_pointers[y][x>>1] = v;
        }
    }
}
#endif

int main()
{
    // stdio_init_all();
    board_init();
    // sleep_ms(1000);
    // board_init();
    // tuh_init(BOARD_TUH_RHPORT);
    //stdio_set_translate_crlf(&stdio_uart, false);

    // printf("Starting USB\n");
    // usb_hid = new HIDHost();
    // board_init();
    // usb_hid->setKeyReceiver(console);
    // usb_hid->setMouseReceiver(mouse);
    // usb_hid->start(); 

    printf("\n\nStarting VGA\n");
    multicore_launch_core1(core1_entry);
    
    // sleep_ms(1000);
    //draw_test_pattern();
            
    printf("Entering main loop on core 0\n");
    while (1) {
        int ch = getchar();
        if (ch >= 0) putchar(ch);
        // Currently it's the console we're using for input, so just fetch
        // bytes and send them to the other CPU through the ring buffer
        //int ch = getchar_timeout_us(1);
        //if (ch != PICO_ERROR_TIMEOUT && console) console->add_char(ch);
        // if (video) printf("scanline %d\n", video->scanline);
        //printf("test counter %d\n", test_counter);
        // tuh_task();
        // hid_app_task();
        
        // Might need this later
        // int ch = getchar_timeout_us(100);
        // if (PICO_ERROR_TIMEOUT == ch) ...
    }
}


void tuh_mount_cb(uint8_t dev_addr)
{
  // application set-up
  printf("A device with address %d is mounted\r\n", dev_addr);
}

void tuh_umount_cb(uint8_t dev_addr)
{
  // application tear-down
  printf("A device with address %d is unmounted \r\n", dev_addr);
}

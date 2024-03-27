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
// #include "msc_app.hpp"
#include "console_stdio.hpp"
#include "lisp.hpp"

VGAVideo *video = 0;
VGAGraphics *graphics = 0;
VGATerm *term = 0;
VGAConsole *console = 0;
VGAMouse *mouse = 0;
extern HIDHost *usb_hid;
// extern MSCHost *usb_msc;

extern void hblank_isr();
extern void hid_app_task();

#if 1
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
            video->line_pointers[y][x>>1] = v;
        }
    }
}
#endif


void read_line(std::string& line)
{
    line.clear();
    for (;;) {
        int ch = getchar();
        switch (ch) {
        case '\r':
        case '\n':
            putchar(13);
            putchar(10);
            return;
        case '\b':
        case 127:
            putchar(8);
            putchar(32);
            putchar(8);
            if (line.size() > 0) line.resize(line.size()-1);
            break;
        default:
            putchar(ch);
            line.push_back(ch);
            break;
        }
    }
}

uint64_t last_print = 0;
uint64_t last_task = 0;
uint64_t max_delay[4] = {0};

void all_core1_tasks()
{
    uint64_t now = time_us_64();
    if (last_task) {
        uint64_t dif = now - last_task;
        if (dif > max_delay[0]) max_delay[0] = dif;
    }
    last_task = now;
    if (now - last_print > 10000000) {
        printf("delay %d, %d, %d, %d\n", (int)max_delay[0], (int)max_delay[1], (int)max_delay[2], (int)max_delay[3]);
        last_print = now;
    }
    
    uint64_t t1 = time_us_64();
    hid_app_task();
    uint64_t t2 = time_us_64();
    tuh_task();
    uint64_t t3 = time_us_64();
    console->console_task();
    uint64_t t4 = time_us_64();
    
    if (t2 - t1 > max_delay[1]) max_delay[1] = t2 - t1;
    if (t3 - t2 > max_delay[2]) max_delay[2] = t3 - t2;
    if (t4 - t3 > max_delay[3]) max_delay[3] = t4 - t3;
}

// Entry point for second CPU
void core1_entry()
{
    sleep_ms(1000);
    printf("Core 1 start\n");
    
    // Configure VGA here so that IRQs get routed to core1
    graphics = new VGAGraphics(video);
    term = new VGATerm(graphics);
    mouse = new VGAMouse(graphics);
    console = new VGAConsole(term, mouse);
    init_stdio_vga();

    printf("Starting USB\n");
    usb_hid = new HIDHost();
    // usb_msc = new MSCHost();
    // usb_hid->setKeyReceiver(console);
    usb_hid->setMouseReceiver(mouse);
    //usb_hid->start(); // Obsolete
    tuh_init(BOARD_TUH_RHPORT);
    
    
    // XXX Make a console_task
    // console->add_task(hid_app_task);
    // console->add_task(tuh_task);
    
    draw_test_pattern();
    
    // console->console_loop();
    for (;;) {
        // XXX Start disk I/O operations from here
        // ...
        
        all_core1_tasks();
    }
}

void core0_entry()
{
    video = new VGAVideo(hblank_isr);
    video->start();
    
    LispInterpreter li;
    
                
    printf("Entering main loop on core 0\n");
    while (1) {
        std::string line;
        read_line(line);
        if (line.size() > 0) {
            TokenPtr p = li.evaluate_string(line);
            li.print_list(std::cout, p);
            std::cout << std::endl;
        }
        //int ch = getchar();
        //if (ch >= 0) putchar(ch);
        
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
    multicore_launch_core1(core0_entry);
    core1_entry();
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

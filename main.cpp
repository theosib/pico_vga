#include <stdio.h>
#include "pico/stdlib.h"
#include "pio-vga.pio.h"
#include <string.h>
#include "myfont_rotated.h"
#include "pico/multicore.h"
#include "vgaterm.hpp"
#include "hardware/clocks.h"
#include <algorithm>

constexpr int HACTIVE = 640;
constexpr int HFP = 16;
constexpr int HSYNC = 96;
constexpr int HBP = 48;

constexpr int VACTIVE = 480;
constexpr int VFP = 11;
constexpr int VSYNC = 2;
constexpr int VBP = 31;
constexpr int VTOTAL = VACTIVE + VFP + VSYNC + VBP;

inline bool in_vactive(int n) { return n < VACTIVE; }
inline bool in_vfp(int n) { return n>=VACTIVE && n<(VACTIVE+VFP); }
inline bool in_vsync(int n) { return n>=(VACTIVE+VFP) && n<(VACTIVE+VFP+VSYNC); }
inline bool in_vbp(int n) { return n>=(VACTIVE+VFP+VSYNC) && n<VTOTAL; }

constexpr int CMD_DMA_A = 0;
constexpr int CMD_DMA_B = 1;
constexpr int VID_DMA = 2;

constexpr int PIN_BASE = 2;
constexpr int HSYNC_BIT = 6;
constexpr int VSYNC_BIT = 7;
constexpr int R_BIT = 2;
constexpr int G_BIT = 3;
constexpr int B_BIT = 4;
constexpr int L_BIT = 5;

uint32_t pattern_mask[256];

void compute_pattern_mask()
{
    for (int p=0; p<256; p++) {
        uint32_t v = 0;
        for (int i=0; i<8; i++) {
            if ((p>>i) & 1) v |= 0xf0000000 >> (i<<2);
        }
        pattern_mask[p] = v;
    }
}

//uint8_t line_buffer_active[481][322] = {};
uint8_t *framebuffer = 0;
uint8_t *line_pointers[481];

//uint8_t text_buffer[30][80];

void init_line_pointers()
{
    uint32_t bytes = 324 * 481;
    framebuffer = new uint8_t[bytes];
    memset(framebuffer, 0, bytes);
    for (int i=0; i<481; i++) {
        line_pointers[i] = framebuffer + i*324;
    }
}

void scroll(int start, int end, int by, uint8_t co)
{
    co *= 0x11;
    for (int i=0; i<by; i++) {
        uint8_t *tmp = line_pointers[start];
        for (int j=start+1; j<end; j++) {
            line_pointers[j-1] = line_pointers[j];
        }
        line_pointers[end-1] = tmp;
        memset(tmp, co, 320);
    }
}

void draw_char(uint8_t ch, uint8_t co, int x, int y)
{
    y <<= 4;
    uint8_t *pc = &customfont[(uint)ch<<4];
    uint32_t fg = (co & 15) * 0x11111111;
    uint32_t bg = (co >> 4) * 0x11111111;
    for (int j=0; j<16; j++) {
        uint32_t *row = (uint32_t*)line_pointers[y+j];
        row += x;
        uint8_t p = pc[j];
        uint32_t m = pattern_mask[p];
        uint32_t v = (fg & m) | (bg & ~m);
        *row = v;
    }
    // for (int j=0; j<16; j++) {
    //     uint32_t *row = (uint32_t*)line_pointers[y+j];
    //     printf("y=%d, j=%d, row=%p\n", y, j, row);
    //     *row = 0xAAAAAAAA;
    //     printf("Done\n");
    // }
}

#if 0
void clear_line(uint8_t co, int y)
{
    co *= 0x11;
    for (int j=0; j<16; j++) {
        memset(line_pointers[y+j], co, 320);
    }
}

uint cursor_x=0, cursor_y=0;
bool cursor_on = false;

void show_cursor(bool show)
{
    if (show == cursor_on) return;
    cursor_on = show;
    
    int y = cursor_y << 4;
    for (int j=0; j<16; j++) {
        uint32_t *row = (uint32_t*)line_pointers[y+j];
        row += cursor_x;
        *row ^= 0xffffffff;
    }
}

void write_char(uint8_t ch, uint8_t co)
{
    show_cursor(false);
    switch (ch) {
    case 13:
    case 10:
        cursor_x = 0;
        cursor_y++;
        break;
    default:
        draw_char(ch, co, cursor_x, cursor_y);
        cursor_x++;
        break;
    }
    if (cursor_x == 80) {
        cursor_x = 0;
        cursor_y++;
    }
    if (cursor_y == 30) {
        scroll(0, 480, 16, 0);
        cursor_y = 29;
    }
    show_cursor(true);
}
#endif

void vga_draw_string(int x, int y, uint8_t colors, uint8_t *str, int len)
{
    //printf("Drawing string from %d,%d, len %d\n", x, y, len);
    for (int i=0; i<len; i++) {
        draw_char(str[i], colors, x+i, y);
    }
}

void vga_clear_area(int x, int y, uint8_t color, int w)
{
    //printf("Clearing area from %d,%d, len %d, color=%d\n", x, y, w, color);
    y <<= 4;
    uint8_t co = color * 0x11;
    for (int j=0; j<16; j++) {
        uint32_t *row = (uint32_t*)line_pointers[y+j];
        memset(row+x, co, w<<2);
    }
}

void vga_draw_line(int x, int y, uint8_t color, int w)
{
    //printf("Drawing line from %d,%d, width %d\n", x, y, w);
    uint8_t co = color * 0x11;
    uint32_t *row = (uint32_t*)line_pointers[(y<<4) + 15];
    memset(row + x, co, w<<2);
}

void vga_copy_area(int sx, int sy, int dx, int dy, int w, int h)
{
    //printf("Copy area (%d,%d) to (%d,%d) %d x %d\n", sx, sy, dx, dy, w, h);
    if (sx == dx && sy == dy) return;
    
    dy <<= 4;
    sy <<= 4;
    h <<= 4;
    if (w == 80 && sx == 0 && dx == 0) {
        if (dy >= sy+h || dy+h <= sy) {
            // Nonoverlapping regions, swap row pointers
            for (int y=0; y<h; y++) {
                std::swap(line_pointers[dy+y], line_pointers[sy+y]);
            }
        } else if (dy > sy) {
            // Scrolling down
            int rep = dy - sy;
            h += rep;
            for (int j=0; j<rep; j++) {
                uint8_t *tmp = line_pointers[sy+h-1];
                for (int i=h-1; i>0; i--) {
                    line_pointers[sy+i] = line_pointers[sy+i-1];
                }
                line_pointers[sy] = tmp;
            }
        } else {
            // Scrolling up
            int rep = sy - dy;
            h += rep;
            for (int j=0; j<rep; j++) {
                uint8_t *tmp = line_pointers[dy];
                for (int i=1; i<h; i++) {
                    line_pointers[dy+i-1] = line_pointers[dy+i];
                }
                line_pointers[dy+h-1] = tmp;
            }
        }
    } else {
        sx <<= 2;
        dx <<= 2;
        w <<= 2;
        if (dy > sy) {
            // Bottom to top
            for (int i=h-1; i>=0; i--) {
                uint8_t *sp = line_pointers[sy + i] + sx;
                uint8_t *dp = line_pointers[dy + i] + dx;
                memmove(dp, sp, w);
            }
        } else {
            // Top to bottom
            for (int i=0; i<h; i++) {
                uint8_t *sp = line_pointers[sy + i] + sx;
                uint8_t *dp = line_pointers[dy + i] + dx;
                memmove(dp, sp, w);
            }
        }
    }
}


int scanline = 0;

void __not_in_flash_func(irq_handler)()
{
    bool active = in_vactive(scanline);
    if (active) {
        vga_start_send_dma(VID_DMA, line_pointers[scanline], 321);
    } else {
        gpio_put(VSYNC_BIT, in_vsync(scanline));
        vga_start_send_dma(VID_DMA, line_pointers[480], 321);
    }

    scanline++;
    if (scanline == VTOTAL) scanline = 0;
    
    pio_interrupt_clear(pio0, 0);
}

void setup()
{
    uint vga_data_offset = pio_add_program(pio0, &vga_data_program);        
    vga_configure_send_dma(VID_DMA, pio0, 0);    
    vga_data_init(pio0, 0, vga_data_offset, PIN_BASE, HSYNC_BIT, 50000000);
    
    // printf("Program at %u\n", vga_data_offset);
    
    // Sync bits are active low
    gpio_set_outover(VSYNC_BIT, GPIO_OVERRIDE_INVERT);
    gpio_set_outover(HSYNC_BIT, GPIO_OVERRIDE_INVERT);

    setup_vga_irq(pio0, irq_handler);
    
    //pio_sm_set_enabled(pio0, 0, true);
    //pio_sm_set_enabled(pio0, 1, true);
    
    gpio_init(VSYNC_BIT);
    gpio_set_dir(VSYNC_BIT, GPIO_OUT);    
}

volatile int buf_head = 0;
volatile int buf_tail = 0;
uint8_t text_buffer[4096];

void add_char(uint8_t c)
{
    int next_tail = (buf_tail + 1) & 4095;
    //if (next_tail == buf_head) printf("Losing chars\n");
    while (next_tail == buf_head);
    text_buffer[buf_tail] = c;
    buf_tail = next_tail;
}

void core1_entry()
{
    setup();
    VGATerm term;
    
    bool pending = true;
    for (;;) {
        uint64_t timeout = time_us_64() + 100;
        while (buf_head == buf_tail && time_us_64() < timeout);
        if (buf_head == buf_tail) {
            if (pending) term.Update();
            pending = false;
        } else {
            int tail = buf_tail;
            int head = buf_head;
            if (tail > head) {
                int count = tail - head;
                //printf("%d\n", count);
                term.ProcessInput(count, text_buffer + head);
                if (count < 2048) term.Update();
                buf_head = tail;
            } else {
                int tc;
                int count = 4096 - head;
                tc = count;
                //printf("%d\n", count);
                term.ProcessInput(count, text_buffer + head);
                count = tail;
                tc += count;
                term.ProcessInput(count, text_buffer);
                if (tc < 2048) term.Update();
                buf_head = tail;
            }
            pending = true;
        }
    }
}

int main()
{
    //sleep_us(5000);
    stdio_init_all();
    //stdio_set_translate_crlf(&stdio_uart, false);
    
    compute_pattern_mask();
    init_line_pointers();
    
    printf("Starting VGA\n");
    //printf("Buffer from %p to %p\n", &line_buffer_active[0][0], &line_buffer_active[479][319]);
    // printf("Buffer from %p to %p\n", &line_pointers[0][0], &line_pointers[479][319]);
    multicore_launch_core1(core1_entry);
    
    // uint8_t c = 0;
    // for (int y=0; y<480; y++) {
    //     for (int x=0; x<640; x+=2) {
    //         c = (x>>4) + (y>>4);
    //         c &= 15;
    //         uint8_t v = 0;
    //         v |= c;
    //         v |= c<<4;
    //         line_pointers[y][x>>1] = v;
    //     }
    //     //printf("%p ", line_pointers[y]);
    // }
    //printf("\n\n");
    
    //show_cursor(true);
    
    uint8_t char_buf[1000];
    int num_char = 0;
    
    while (1) {
        add_char(getchar());
        // int ch = getchar_timeout_us(100);
        // if (PICO_ERROR_TIMEOUT == ch) {
        //     term.ProcessInput(num_char, char_buf);
        //     num_char = 0;
        //     term.Update();
        // } else {
        //     putchar(ch);
        //     char_buf[num_char++] = ch;
        //     if (num_char == 1000) {
        //         term.ProcessInput(num_char, char_buf);
        //         num_char = 0;
        //     }
        // }
    }
}


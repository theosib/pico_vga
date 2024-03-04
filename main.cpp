#include <stdio.h>
#include "pico/stdlib.h"
#include "pio-vga.pio.h"
#include <string.h>
#include "myfont_rotated.h"

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

constexpr int PIN_BASE = 0;
constexpr int HSYNC_BIT = 6;
constexpr int VSYNC_BIT = 7;
constexpr int R_BIT = 0;
constexpr int G_BIT = 1;
constexpr int B_BIT = 2;
constexpr int L_BIT = 3;

uint32_t pattern_mask[256];

void compute_pattern_mask()
{
    for (int p=0; p<256; p++) {
        uint32_t v = 0;
        for (int i=0; i<8; i++) {
            if ((p>>i) & 1) v |= 15 << (i<<2);
        }
        pattern_mask[p] = v;
    }
}

uint8_t line_buffer_active[481][322] = {};
uint8_t *line_pointers[480];

//uint8_t text_buffer[30][80];

void init_line_pointers()
{
    for (int i=0; i<480; i++) {
        line_pointers[i] = line_buffer_active[i];
    }
}

void scroll(int start, int end, int by)
{
    for (int i=0; i<by; i++) {
        uint8_t *tmp = line_pointers[start];
        for (int j=start+1; j<end; j++) {
            line_pointers[j-1] = line_pointers[j];
        }
        line_pointers[end-1] = tmp;
    }
}

void draw_char(uint8_t ch, uint8_t co, int x, int y)
{
    uint8_t *pc = &customfont[ch<<4];
    uint32_t fg = (co & 15) * 0x11111111;
    uint32_t bg = (co >> 4) * 0x11111111;
    for (int j=0; j<16; j++) {
        uint32_t *row = (uint32_t*)line_pointers[y+j];
        row += x;
        uint8_t p = pc[j];
        uint32_t m = pattern_mask[p];
        *row = (fg & m) | (bg & ~m);
    }
}

uint cursor_x=0, cursor_y=0;
void write_char(uint8_t ch)
{
    draw_char(ch, 15, cursor_x, cursor_y*16);
    cursor_x++;
    if (cursor_x == 80) {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y == 30) {
            scroll(0, 400, 8);
            cursor_y = 29;
        }
    }
}

int scanline = 0;

void irq_handler()
{
    bool active = in_vactive(scanline);
    if (active) {
        vga_start_send_dma(VID_DMA, line_buffer_active[scanline], 321);
    } else {
        gpio_put(VSYNC_BIT, in_vsync(scanline));
        vga_start_send_dma(VID_DMA, line_buffer_active[480], 321);
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
    
    printf("Program at %u\n", vga_data_offset);
    
    // Sync bits are active low
    gpio_set_outover(PIN_BASE + VSYNC_BIT, GPIO_OVERRIDE_INVERT);
    gpio_set_outover(PIN_BASE + HSYNC_BIT, GPIO_OVERRIDE_INVERT);

    setup_vga_irq(pio0, irq_handler);
    
    pio_sm_set_enabled(pio0, 0, true);
    pio_sm_set_enabled(pio0, 1, true);
    
    gpio_init(VSYNC_BIT);
    gpio_set_dir(VSYNC_BIT, GPIO_OUT);    
}



int main()
{
    stdio_init_all();
    compute_pattern_mask();
    init_line_pointers();
    
    printf("Starting VGA\n");
    setup();
    
    uint8_t c = 0;
    for (int y=0; y<480; y++) {
        for (int x=0; x<640; x+=2) {
            c = (x>>4) + (y>>4);
            c &= 15;
            uint8_t v = 0;
            v |= c;
            v |= c<<4;
            line_buffer_active[y][x>>1] = v;
        }
    }
    
    uint old_pc = 0;
    uint8_t ch = 32;
    while (1) {
        //write_char(ch);
        ch++;
        if (ch == 128) ch = 32;
        sleep_ms(1000);
    }
}


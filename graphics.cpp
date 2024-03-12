
#include "graphics.hpp"
#include "myfont_rotated.h"
#include <stdint.h>
#include <string.h>
#include <algorithm>


// 32-bit memset
inline void set_words(uint32_t *p, uint32_t v, int c)
{
    while (c) {
        c--;
        *p++ = v;
    }
}

// Copy pointers in forward order
inline void copy_ptrs(uint8_t **d, uint8_t **s, int c)
{
    while (c) {
        c--;
        *d++ = *s++;
    }
}

// Copy pointers in reverse order
inline void copy_ptrs_r(uint8_t **d, uint8_t **s, int c)
{
    d += c;
    s += c;
    while (c) {
        c--;
        *--d = *--s;
    }
}

void VGAGraphics::compute_pattern_mask()
{
    for (int p=0; p<256; p++) {
        uint32_t v = 0;
        for (int i=0; i<8; i++) {
            if ((p>>i) & 1) v |= 0xf0000000 >> (i<<2);
        }
        pattern_mask[p] = v;
    }
}

// Draw a single character to the framebuffer
void VGAGraphics::draw_char(uint8_t ch, uint8_t co, int x, int y)
{
    y <<= 4;
    uint8_t *pc = &customfont[(uint32_t)ch<<4];
    uint32_t fg = (co & 15) * 0x11111111;
    uint32_t bg = (co >> 4) * 0x11111111;
    for (int j=0; j<16; j++) {
        uint32_t *row = (uint32_t*)video->get_row(y+j);
        row += x;
        uint8_t p = pc[j];
        uint32_t m = pattern_mask[p];
        uint32_t v = (fg & m) | (bg & ~m);
        *row = v;
    }
}


// Draw a string to the framebuffer
void VGAGraphics::draw_string(int x, int y, uint8_t co, uint8_t *str, int len)
{
    y <<= 4; // Text rows are 16 scanlines
    
    // Extract foreground and background colors and replicate them across 32-bit word
    uint32_t fg = (co & 15) * 0x11111111;
    uint32_t bg = (co >> 4) * 0x11111111;
    
    // Find each string character in the display font
    uint8_t *pc[len];
    for (int i=0; i<len; i++) {
        pc[i] = &customfont[(uint32_t)str[i]<<4];
    }
    
    for (int j=0; j<16; j++) {
        // Look up the scanline pointer for row j of the text line,
        // convert to pointer to 32-bit word, and add x.
        // Characters are 8 pixels wide, which is one 32-bit word
        // at 4bpp.
        uint32_t *rp = (uint32_t*)video->get_row(y+j) + x;
        for (int i=0; i<len; i++) {
            uint8_t p = *pc[i]++;               // 8-bit pattern for this row of character 1
            uint32_t m = pattern_mask[p];       // Convert to 4bpp mask
            uint32_t v = (fg & m) | (bg & ~m);  // Mix fg and bg accoring to mask
            *rp++ = v;                          // Store word to framebuffer
        }
    }
}


void VGAGraphics::plot_pixel(int x, int y, uint8_t color)
{
    if (x < 0 || x >= 640 || y < 0 || y >= 480) return;
    uint8_t *row = video->get_row(y);
    row += x>>1;
    color &= 15;
    if (x&1) {
        *row = (*row & 0x0f) | (color << 4);
    } else {
        *row = (*row & 0xf0) | (color);
    }
}

void VGAGraphics::and_pixel(int x, int y, uint8_t color)
{
    if (x < 0 || x >= 640 || y < 0 || y >= 480) return;
    uint8_t *row = video->get_row(y);
    row += x>>1;
    color &= 15;
    if (x&1) {
        *row &= (color << 4) | 0xf;
    } else {
        *row &= (color) | 0xf0;
    }
}

void VGAGraphics::or_pixel(int x, int y, uint8_t color)
{
    if (x < 0 || x >= 640 || y < 0 || y >= 480) return;
    uint8_t *row = video->get_row(y);
    row += x>>1;
    color &= 15;
    if (x&1) {
        *row |= (color << 4);
    } else {
        *row |= (color);
    }
}

void VGAGraphics::mix_pixel(int x, int y, uint8_t mask, uint8_t color)
{
    if (x < 0 || x >= 640 || y < 0 || y >= 480) return;
    uint8_t *row = video->get_row(y);
    row += x>>1;
    color &= 15;
    if (x&1) {
        *row = (*row & ((mask << 4) | 0xf)) | (color << 4);
    } else {
        *row = (*row & (mask | 0xf0)) | (color);
    }
}

int VGAGraphics::read_pixel(int x, int y)
{
    if (x < 0 || x >= 640 || y < 0 || y >= 480) return 0;
    uint8_t *row = video->get_row(y);
    row += x>>1;
    return (x&1) ? (*row >> 4) : (*row & 15);
}


// Clear a box of characters
void VGAGraphics::clear_area(int x, int y, uint8_t color, int w)
{
    y <<= 4;
    uint32_t co = color * 0x11111111;
    for (int j=0; j<16; j++) {
        uint32_t *row = (uint32_t*)video->get_row(y+j);
        //memset(row+x, co, w<<2);
        set_words(row+x, co, w);
    }
}

// Draw a 1-pixel tall horizontal line
void VGAGraphics::draw_line(int x, int y, uint8_t color, int w)
{
    uint32_t co = color * 0x11111111;
    uint32_t *row = (uint32_t*)video->get_row((y<<4) + 15);
    //memset(row + x, co, w<<2);
    set_words(row+x, co, w);
}

// Perform bitblt operation for text scrolling
void VGAGraphics::copy_area(int sx, int sy, int dx, int dy, int w, int h)
{
    if (sx == dx && sy == dy) return;
    
    // Characters rae 16-pixels tall
    dy <<= 4;
    sy <<= 4;
    h <<= 4;
    
    if (w == 80 && sx == 0 && dx == 0) {
        // If the area being copied is the full width of the screen, we can
        // perform the copy by just reordering scanlines
        if (dy >= sy+h || dy+h <= sy) {
            // Nonoverlapping regions, swap row pointers
            for (int y=0; y<h; y++) {
                std::swap(video->get_row(dy+y), video->get_row(sy+y));
            }
        } else if (dy > sy) {
            // Scrolling down
            int dif = dy - sy;
            uint8_t *tmp[dif];
            copy_ptrs(tmp, &video->get_row(sy+h), dif);
            copy_ptrs_r(&video->get_row(dy), &video->get_row(sy), h);
            copy_ptrs(&video->get_row(sy), tmp, dif);
        } else {
            // Scrolling up
            int dif = sy - dy;
            uint8_t *tmp[dif];
            copy_ptrs(tmp, &video->get_row(dy), dif);
            copy_ptrs(&video->get_row(dy), &video->get_row(sy), h);
            copy_ptrs(&video->get_row(dy+h), tmp, dif);
        }
    } else {
        // General bitblt for any other size area
        sx <<= 2;   // Characters are 4 bytes wide
        dx <<= 2;
        w <<= 2;
        if (dy > sy) {
            // Bottom to top
            for (int i=h-1; i>=0; i--) {
                uint8_t *sp = video->get_row(sy + i) + sx;
                uint8_t *dp = video->get_row(dy + i) + dx;
                memmove(dp, sp, w);
            }
        } else {
            // Top to bottom
            for (int i=0; i<h; i++) {
                uint8_t *sp = video->get_row(sy + i) + sx;
                uint8_t *dp = video->get_row(dy + i) + dx;
                memmove(dp, sp, w);
            }
        }
    }
}

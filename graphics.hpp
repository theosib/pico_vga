#ifndef INCLUDED_GRAPHICS_HPP
#define INCLUDED_GRAPHICS_HPP

#include "video.hpp"

struct VGAGraphics {
    VGAVideo *video = 0;
        
    // LUT to translate ABCDEFGH to AAAABBBBCCCCDDDDEEEEFFFFGGGGHHHH
    uint32_t pattern_mask[256];
    void compute_pattern_mask();
    
    void draw_char(uint8_t ch, uint8_t co, int x, int y);
    void draw_string(int x, int y, uint8_t co, uint8_t *str, int len);
    void plot_pixel(int x, int y, uint8_t color);
    void and_pixel(int x, int y, uint8_t color);
    void or_pixel(int x, int y, uint8_t color);
    void mix_pixel(int x, int y, uint8_t mask, uint8_t color);
    int  read_pixel(int x, int y);
    void clear_area(int x, int y, uint8_t color, int w);
    void draw_line(int x, int y, uint8_t color, int w);
    void copy_area(int sx, int sy, int dx, int dy, int w, int h);
    
    
    VGAGraphics(VGAVideo *v) {
        video = v;
        compute_pattern_mask();
    }
};

#endif
#ifndef INCLUDED_MOUSE_HPP
#define INCLUDED_MOUSE_HPP

#include "graphics.hpp"

struct VGAMouse {
    VGAGraphics *graphics;
    
    volatile int mouse_x = 0, mouse_y = 0;
    int mouse_x_old = 0, mouse_y_old = 0;
    bool mouse_visible = false;
    bool mouse_moved = false;
    
    uint8_t mask[256], color[256], save[256];
    
    VGAMouse(VGAGraphics *g);
    
    void save_background(int x, int y, uint8_t *p);
    void save_background(int x, int y);
    void restore_background(int x, int y, uint8_t *p);
    void restore_background(int x, int y);
    void draw_pointer(int x, int y, uint8_t *mask, uint8_t *color);
    void draw_pointer(int x, int y);
    
    void move_mouse(int x, int y);
    void hide_mouse();
    void draw_mouse();
};

#endif
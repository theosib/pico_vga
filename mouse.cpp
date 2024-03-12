

#include "pointer.h"
#include <stdint.h>
#include <stdio.h>
#include "mouse.hpp"

static void extract_pointer(uint8_t *color, uint8_t *mask)
{
    const char *data = header_data;
    for (int j=0; j<height; j++) {
        for (int i=0; i<width; i++) {
            uint8_t pixel[3];
            HEADER_PIXEL(data, pixel);
            //printf("%d %d %d\n", pixel[0], pixel[1], pixel[2]);
            if (pixel[0]) {
                // White
                mask[i+j*16] = 0;
                color[i+j*16] = 15;
            } else if (pixel[2]) {
                // Background
                mask[i+j*16] = 15;
                color[i+j*16] = 0;
            } else {
                // Black
                mask[i+j*16] = 0;
                color[i+j*16] = 0;
            }
        }
    }
}

VGAMouse::VGAMouse(VGAGraphics *g)
{
    graphics = g;
    extract_pointer(color, mask);
}

void VGAMouse::save_background(int x, int y, uint8_t *p)
{
    for (int j=0; j<height; j++) {
        for (int i=0; i<width; i++) {
            p[i+16*j] = graphics->read_pixel(x+i, y+j);
        }
    }
}

void VGAMouse::save_background(int x, int y)
{
    save_background(x, y, save);
}

void VGAMouse::restore_background(int x, int y, uint8_t *p)
{
    for (int j=0; j<height; j++) {
        for (int i=0; i<width; i++) {
            graphics->plot_pixel(x+i, y+j, p[i+j*16]);
        }
    }
}

void VGAMouse::restore_background(int x, int y)
{
    restore_background(x, y, save);
}

void VGAMouse::draw_pointer(int x, int y, uint8_t *mask, uint8_t *color)
{
    for (int j=0; j<height; j++) {
        for (int i=0; i<width; i++) {
            graphics->mix_pixel(x+i, y+j, mask[i+j*16], color[i+j*16]);
        }
    }
}

void VGAMouse::draw_pointer(int x, int y)
{
    draw_pointer(x, y, mask, color);
}

void VGAMouse::move_mouse(int x, int y)
{
    mouse_x += x;
    if (mouse_x < 0) mouse_x = 0;
    if (mouse_x > 639) mouse_x = 639;
    mouse_y += y;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_y > 479) mouse_y = 479;
    mouse_moved = true;
}

void VGAMouse::hide_mouse()
{
    if (!mouse_visible) return;
    restore_background(mouse_x_old, mouse_y_old);
    mouse_visible = false;
}

void VGAMouse::draw_mouse()
{
    if (mouse_visible && !mouse_moved) return;
    
    hide_mouse();
    int x = mouse_x;
    int y = mouse_y;
    save_background(x, y);
    draw_pointer(x, y);
    mouse_visible = true;
    mouse_x_old = x;
    mouse_y_old = y;
}

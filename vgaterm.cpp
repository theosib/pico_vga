#include "vgaterm.hpp"
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

extern void vga_draw_string(int x, int y, uint8_t colors, uint8_t *str, int len);
extern void vga_draw_line(int x, int y, uint8_t color, int w);
extern void vga_copy_area(int sx, int sy, int dx, int dy, int w, int h);
extern void vga_clear_area(int x, int y, uint8_t color, int w);


VGATerm::~VGATerm() {}

void VGATerm::DrawText(int fg_color, int bg_color, int flags,
                int x, int y, int len, unsigned char *string)
{
	if (flags & INVERSE) {
		int t = fg_color;
		fg_color = bg_color;
		bg_color = t;
	}

    uint8_t colors = (bg_color << 4) | (fg_color & 7) | ((!!flags & BOLD) << 3);
    vga_draw_string(x, y, colors, string, len);

	if (flags&UNDERLINE) {
        uint8_t colors = (fg_color & 7) | ((!!flags & BOLD) << 3);
        vga_draw_line(x, y, colors, len);
    }
}

void VGATerm::DrawCursor(int fg_color, int bg_color, int flags,
                int x, int y, unsigned char c)
{
	unsigned char str[2] = {c, 0};
	DrawText(7-fg_color, 7-bg_color, flags, x, y, 1, str);
}

void VGATerm::MoveChars(int sx, int sy, int dx, int dy, int w, int h)
{
    vga_copy_area(sx, sy, dx, dy, w, h);
}

void VGATerm::ClearChars(int bg_color, int x, int y, int w, int h)
{
    vga_clear_area(x, y, bg_color, w);
}

void VGATerm::SendBack(const char *data)
{
	write(1, data, strlen(data));
}

void VGATerm::Bell()
{
	//XBell(display, 0);
}

void VGATerm::RequestSizeChange(int w, int h)
{
    // if (w != Width() || h != Height()) {
    //     ResizeTerminal(w, h);
    // }
}

#include "vgaterm.hpp"
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

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
    graphics->draw_string(x, y, colors, string, len);

	if (flags&UNDERLINE) {
        uint8_t colors = (fg_color & 7) | ((!!flags & BOLD) << 3);
        graphics->draw_line(x, y, colors, len);
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
    graphics->copy_area(sx, sy, dx, dy, w, h);
}

void VGATerm::ClearChars(int bg_color, int x, int y, int w, int h)
{
    graphics->clear_area(x, y, bg_color, w);
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

void VGATerm::PlotPixel(int x, int y, int c)
{
    //if (x < 0 || y < 0 || x >= 640 || y >= 480) return;
    graphics->plot_pixel(x, y, c);
}

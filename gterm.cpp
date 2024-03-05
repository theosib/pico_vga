
// Copyright Timothy Miller, 1999

#include "gterm.hpp"
#include <cassert>
#include <cstring>
#include <iostream>

#ifndef min
#define min(x,y) ((x)<(y)?(x):(y))
#endif
#ifndef max
#define max(x,y) ((x)<(y)?(y):(x))
#endif

#define ASSERT_X(x) (assert((x) >= 0 && (x) < width))
#define ASSERT_Y(y) (assert((y) >= 0 && (y) < height))

void GTerm::Update()
{
	update_changes();
}

void GTerm::ProcessInput(int len, unsigned char *data)
{
    int i;
    StateOption *last_state;

    data_len = len;
    input_data = data;
#if 0
    //std::string s((const char*)data,unsigned(len));
    cerr<<"len=" << len << ",data:[";
    for (i=0; i<len; i++) {
        if (data[i]<32 || data[i]>126) {
            cout << '\\' << ((int)(data[i]&0700)>>6) <<
                ((int)(data[i]&070)>>3) << ((int)(data[i]&07));
        } else {
            cout << data[i];
        }
    }
    cout <<"]"<<endl;
#endif

    while (data_len) {
	i = 0;
	while (current_state[i].byte != -1 &&
	    current_state[i].byte != *input_data) i++;

	// action must be allowed to redirect state change
	last_state = current_state+i;
	current_state = last_state->next_state;
	if (last_state->action)
            (this->*(last_state->action))();
	input_data++;
	data_len--;
    }

    if (!(mode_flags & DEFERUPDATE) ||
	(pending_scroll > scroll_bot-scroll_top)) update_changes();
}

void GTerm::Reset()
{
	reset();
}

void GTerm::ExposeArea(int x, int y, int w, int h)
{
    ASSERT_X(x);
    ASSERT_Y(y);
    assert(w > 0 && h > 0);

	int i;
	for (i=0; i<h; i++) changed_line(i+y, x, x+w-1);
	if (!(mode_flags & DEFERUPDATE)) update_changes();
}

void GTerm::ResizeTerminal(int w, int h)
{
    assert(w > 0 && h > 0);

	if (width == w && height == h)
    	    return;
	int cx, cy;
	cx = min(w-1, cursor_x);
	cy = min(h-1, cursor_y);
	move_cursor(cx, cy);

    delete[] dirty_endx;
    delete[] dirty_startx;
    delete[] linenumbers;
    delete[] tab_stops;
	delete[] text;
	delete[] color;

	width = w;
	height = h;

	text         = new unsigned char[width * height];
	color        = new unsigned short[width * height];
	tab_stops    = new char[width];
	linenumbers  = new short[height];
	dirty_startx = new unsigned char[height];
	dirty_endx   = new unsigned char[height];

	save_x         = 0;
	save_y         = 0;
	scroll_top     = 0;
	scroll_bot     = height-1;

    int i;
	for (i=0; i<height; i++) {
		// make it draw whole terminal to start
		dirty_startx[i] = 0;
		dirty_endx[i] = width - 1;
	}
	for (i=0; i<height; i++) linenumbers[i] = i;
	memset(tab_stops, 0, width);

	clear_area(0, 0, width-1, height-1);
}

GTerm::GTerm(int w, int h) : width(w), height(h),mode_flags(0),cur_charset(0)
{
    assert(w > 0 && h > 0);

	doing_update = false;
    charset[0] = charset[1] = 'B';

	text         = new unsigned char[width * height];
	color        = new unsigned short[width * height];
	tab_stops    = new char[width];
	linenumbers  = new short[height];
	dirty_startx = new unsigned char[height];
	dirty_endx   = new unsigned char[height];

	reset();
}

GTerm::~GTerm()
{
    delete[] dirty_endx;
    delete[] dirty_startx;
    delete[] linenumbers;
    delete[] tab_stops;
	delete[] text;
	delete[] color;
}


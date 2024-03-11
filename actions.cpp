
// Copyright Timothy Miller, 1999

#include "gterm.hpp"
#include <cstdio>
#include <cstring>
#include <cassert>

#define ASSERT_X(x) (assert((x) >= 0 && (x) < width))
#define ASSERT_Y(y) (assert((y) >= 0 && (y) < height))

using namespace std;

void GTerm::gfx_input()
{
    //printf("gfx_input data_len=%d\n", data_len);
    while (data_len>0) {
        //printf("%d\n", *input_data);
        switch (*input_data) {
        case 27:
            input_data--;
            data_len++;
            return;
        case 13:
            gfx_x = 0;
            break;
        case 10:
            gfx_y++;
            break;
        case 11:
            gfx_y--;
            break;
        case 12:
            gfx_x = 0;
            gfx_y = 0;
            break;
        case 32:
            gfx_x++;
            break;
        case 8:
            gfx_x--;
            break;
        default:
            if (*input_data >= 64 && *input_data < 80 && gfx_x < 640 && gfx_y < 480) {
                PlotPixel(gfx_x, gfx_y, *input_data - 64);
                gfx_x++;
            }
            break;
        }
        input_data++;
        data_len--;
    }
    input_data--;
    data_len++;
}

// For efficiency, this grabs all printing characters from buffer, up to
// the end of the line or end of buffer
void GTerm::normal_input()
{
    int n, n_taken, i, c, y;
#if 0
    char str[100];
#endif
    assert(data_len > 0);

    if (*input_data < 32 || *input_data == 0177) return;

    ASSERT_X(cursor_x);

    n = 0;
    if (mode_flags & NOEOLWRAP) {
        while (n<data_len && input_data[n]>31) n++;
        n_taken = n;
        if (cursor_x+n>=width) n = width-cursor_x;
    } else {
        while (n<data_len && input_data[n]>31 && cursor_x+n<width) n++;
        n_taken = n;
        if (cursor_x == width - 1 && force_wrap) {
            next_line();
        }
    }

    assert(n > 0);

	translate_charset(input_data, input_data+n);

#if 0
    memcpy(str, input_data, n);
    str[n] = 0;
    printf("Processing %d characters (%d): %s\n", n, str[0], str);
#endif

    if (mode_flags & INSERT) {
        changed_line(cursor_y, cursor_x, width-1);
    } else {
        changed_line(cursor_y, cursor_x, cursor_x+n-1);
    }

    y = linenumbers[cursor_y]*width;
    if (mode_flags & INSERT)
        for (i=width-1; i>=cursor_x+n; i--) {
            text[y+i] = text[y+i-n];
            color[y+i] = color[y+i-n];
        }

    c = calc_color(fg_color, bg_color, mode_flags);

    for (i=0; i<n; i++) {
        text[y+cursor_x] = input_data[i];
        color[y+cursor_x] = c;
        cursor_x++;
    }

    if (cursor_x >= width) {
        if (cursor_x > width && !(mode_flags & NOEOLWRAP)) {
            cursor_x = width-1;
            next_line();
        } else {
            cursor_x = width-1;
            force_wrap = true;
        }

    }

    input_data += n_taken-1;
    data_len -= n_taken-1;
}

void GTerm::cr()
{
	move_cursor(0, cursor_y);
}

void GTerm::lf()
{
	if (cursor_y < scroll_bot) {
		move_cursor(cursor_x, cursor_y+1);
	} else {
		scroll_region(scroll_top, scroll_bot, 1);
	}
    if (mode_flags & NEWLINE)
        move_cursor(0,cursor_y);
}

void GTerm::ff()
{
	clear_area(0, scroll_top, width-1, scroll_bot);
	move_cursor(0, scroll_top);
}

void GTerm::tab()
{
	int i, x = 0;

	for (i=cursor_x+1; i<width && !x; i++)
        if (tab_stops[i]) x = i;

	//if (!x) x = (cursor_x+8) & -8;
	if (!x) x = width - 1;

    move_cursor(x, cursor_y);

//	if (x < width) {
//		move_cursor(x, cursor_y);
//	} else {
//		if (mode_flags & NOEOLWRAP) {
//			move_cursor(width-1, cursor_y);
//		} else {
//			next_line();
//		}
//	}
}

void GTerm::bs()
{
	if (cursor_x>0) move_cursor(cursor_x-1, cursor_y);
	if (mode_flags & DESTRUCTBS) {
	    clear_area(cursor_x, cursor_y, cursor_x, cursor_y);
	}
}

void GTerm::bell()
{
	Bell();
}

void GTerm::clear_param()
{
	nparam = 0;
	memset(param, 0, sizeof(param));
	q_mode = 0;
	got_param = 0;
}

void GTerm::keypad_numeric() { clear_mode_flag(KEYAPPMODE); }
void GTerm::keypad_application() { set_mode_flag(KEYAPPMODE); }

void GTerm::save_cursor()
{
	save_attrib = mode_flags;
	save_x = cursor_x;
	save_y = cursor_y;
}

void GTerm::restore_cursor()
{
	mode_flags = (mode_flags & ~15) | (save_attrib & 15);
	move_cursor(save_x, save_y);
}

void GTerm::set_tab()
{
	tab_stops[cursor_x] = 1;
}

void GTerm::index_down()
{
    if (cursor_y < scroll_bot) {
        move_cursor(cursor_x, cursor_y+1);
    } else {
        scroll_region(scroll_top, scroll_bot, 1);
    }
}

void GTerm::next_line()
{
    index_down();
	cr();
}

void GTerm::index_up()
{
	if (cursor_y > scroll_top) {
		move_cursor(cursor_x, cursor_y-1);
	} else {
		scroll_region(scroll_top, scroll_bot, -1);
	}
}

void GTerm::reset()
{
	int i;

	pending_scroll = 0;
    force_wrap     = false;
    inverse_mode   = false;
	cursor_x       = 0;
	cursor_y       = 0;
	save_x         = 0;
	save_y         = 0;
	bg_color       = 0;
	fg_color       = 7;
	scroll_top     = 0;
	scroll_bot     = height-1;
    gfx_x = 0;
    gfx_y = 0;

	for (i=0; i<height; i++) {
		// make it draw whole terminal to start
		dirty_startx[i] = 0;
		dirty_endx[i] = width - 1;
	}
	for (i=0; i<height; i++) linenumbers[i] = i;
	memset(tab_stops, 0, width);
	for (i=7; i<width; i+=8)
		tab_stops[i] = 1;
	current_state = GTerm::normal_state;

	clear_mode_flag(NOEOLWRAP | CURSORAPPMODE | CURSORRELATIVE |
		NEWLINE | INSERT | UNDERLINE | BLINK | KEYAPPMODE);

	clear_area(0, 0, width-1, height-1);
	move_cursor(0, 0);
}

void GTerm::set_q_mode()
{
	q_mode = 1;
}

// The verification test used some strange sequence which was
// ^[[61"p
// in a function called set_level,
// but it didn't explain the meaning.  Just in case I ever find out,
// and just so that it doesn't leave garbage on the screen, I accept
// the quote and mark a flag.
void GTerm::set_quote_mode()
{
	quote_mode = 1;
}

// for performance, this grabs all digits
void GTerm::param_digit()
{
	got_param = 1;
	param[nparam] = param[nparam]*10 + (*input_data)-'0';
}

void GTerm::next_param()
{
	nparam++;
}

void GTerm::cursor_left()
{
	int n, x;
	n = param[0]; if (n<1) n = 1;
	x = cursor_x-n; if (x<0) x = 0;
	move_cursor(x, cursor_y);
}

void GTerm::cursor_right()
{
	int n, x;
	n = param[0]; if (n<1) n = 1;
	x = cursor_x+n; if (x>=width) x = width-1;
	move_cursor(x, cursor_y);
}

void GTerm::cursor_up()
{
	int n, y;
	n = param[0]; if (n<1) n = 1;
	y = cursor_y-n; if (y<scroll_top) y = scroll_top;
	move_cursor(cursor_x, y);
}

void GTerm::cursor_down()
{
	int n, y;
	n = param[0]; if (n<1) n = 1;
	y = cursor_y+n; if (y>scroll_bot) y = scroll_bot;
	move_cursor(cursor_x, y);
}

void GTerm::cursor_position()
{
	int x, y;
	x = param[1];	if (x<1) x=1;
	y = param[0];	if (y<1) y=1;
	if (mode_flags & CURSORRELATIVE) {
		move_cursor(x-1, y-1+scroll_top);
	} else {
		move_cursor(x-1, y-1);
	}
}

void GTerm::device_attrib()
{
    if (param[0] == 0)
        SendBack("\033[?1;2c");
}

void GTerm::delete_char()
{
	int n, mx;
	n = param[0]; if (n<1) n = 1;
	mx = width-cursor_x;
	if (n>=mx) {
		clear_area(cursor_x, cursor_y, width-1, cursor_y);
	} else {
		shift_text(cursor_y, cursor_x, width-1, -n);
	}
}

void GTerm::set_mode()
{
    switch (param[0] + 1000*q_mode) {
        case 1001:	set_mode_flag(CURSORAPPMODE);	break;
        case 1003:	RequestSizeChange(132, height);	break;
        case 1005:	{
                        inverse_mode = true;
                        for (int i = 0;i < height;i++)
                            changed_line(i,0,width - 1);
                        update_changes();
                    }
                    break;
        case 1006:	set_mode_flag(CURSORRELATIVE);
                    move_cursor(0,0);break;
        case 1007:	clear_mode_flag(NOEOLWRAP);	break;
        case 1025:
                    clear_mode_flag(CURSORINVISIBLE);
                    move_cursor(cursor_x, cursor_y);
                    break;
        case 4:		set_mode_flag(INSERT);		break;
        case 12:	clear_mode_flag(LOCALECHO);	break;
        case 20:	set_mode_flag(NEWLINE);		break;
    }
}

void GTerm::clear_mode()
{
    switch (param[0] + 1000*q_mode) {
        case 1001:	clear_mode_flag(CURSORAPPMODE);	break;
        case 1002:	current_state = vt52_normal_state; break;
        case 1003:	RequestSizeChange(80, height);	break;
        case 1005:	{
                        inverse_mode = false;
                        for (int i = 0;i < height;i++)
                            changed_line(i,0,width - 1);
                        update_changes();
                    }
                    break;
        case 1006:	clear_mode_flag(CURSORRELATIVE);
                    move_cursor(0,0);break;
        case 1007:	set_mode_flag(NOEOLWRAP);	break;
        case 1025:
                    set_mode_flag(CURSORINVISIBLE);	break;
                    move_cursor(cursor_x, cursor_y);
                    break;
        case 4:		clear_mode_flag(INSERT);	break;
        case 12:	set_mode_flag(LOCALECHO);	break;
        case 20:	clear_mode_flag(NEWLINE);	break;
    }
}

void GTerm::request_param()
{
	char str[40];
	sprintf(str, "\033[%d;1;1;120;120;1;0x", param[0]+2);
	SendBack(str);
}

void GTerm::set_margins()
{
    int t, b;

    t = param[0];
    if (t<1) t = 1;
    b = param[1];
    if (b<1) b = height;

    /* Minimum allowed region is 2 lines */
    if (t < b && b <= height) {
        if (pending_scroll) update_changes();

        scroll_top = t-1;
        scroll_bot = b-1;

        /*****************************************************************
         * if (cursor_y < scroll_top) move_cursor(cursor_x, scroll_top); *
         * if (cursor_y > scroll_bot) move_cursor(cursor_x, scroll_bot); *
         *****************************************************************/
        if (mode_flags & CURSORRELATIVE) 
            move_cursor(0,scroll_top);
        else 
            move_cursor(0,0);
    }
}

void GTerm::delete_line()
{
    //This sequence is ignored when cursor is outside scrolling region
    if (cursor_y < scroll_top || cursor_y > scroll_bot)
        return;
	int n, mx;
	n = param[0]; if (n<1) n = 1;
	mx = scroll_bot-cursor_y+1;
	if (n>=mx) {
		clear_area(0, cursor_y, width-1, scroll_bot);
	} else {
		scroll_region(cursor_y, scroll_bot, n);
	}
}

void GTerm::status_report()
{
	char str[20];
	if (param[0] == 5) {
		SendBack("\033[0n");
	} else if (param[0] == 6) {
		sprintf(str, "\033[%d;%dR", cursor_y+1, cursor_x+1);
		SendBack(str);
	}
}

void GTerm::erase_display()
{
    switch (param[0]) {
        case 0:
            clear_area(cursor_x, cursor_y, width-1, cursor_y); 
            if (cursor_y<height-1)                             
                clear_area(0, cursor_y+1, width-1, height-1);      
            break;
        case 1:
            clear_area(0, cursor_y, cursor_x, cursor_y); 
            if (cursor_y>0)                              
                clear_area(0, 0, width-1, cursor_y-1);       
            break;
        case 2:
            clear_area(0, 0, width-1, height-1);
            break;
    }
}

void GTerm::erase_line()
{
    switch (param[0]) {
        case 0:
            clear_area(cursor_x, cursor_y, width-1, cursor_y);
            break;
        case 1:
            clear_area(0, cursor_y, cursor_x, cursor_y);
            break;
        case 2:
            clear_area(0, cursor_y, width-1, cursor_y);
            break;
    }
}

void GTerm::insert_line()
{
    //This sequence is ignored when cursor is outside scrolling region
    if (cursor_y < scroll_top || cursor_y > scroll_bot)
        return;

	int n, mx;
	n = param[0]; if (n<1) n = 1;
	mx = scroll_bot-cursor_y+1;
	if (n>=mx) {
		clear_area(0, cursor_y, width-1, scroll_bot);
	} else {
		scroll_region(cursor_y, scroll_bot, -n);
	}
}
  
void GTerm::set_colors()
{
    int n;

    if (!nparam && param[0] == 0) {
        clear_mode_flag(0x0f);
        fg_color = 7;
        bg_color = 0;
        return;
    }

    for (n=0; n<=nparam; n++) {
        if (param[n] >= 40 && param[n] <= 47) {
            bg_color = param[n]%10;
        } else if (param[n] >= 30 && param[n] <= 37) {
            fg_color = param[n]%10;
        } else switch (param[n]) {
            case 0:
                clear_mode_flag(0x0f);
                fg_color = 7;
                bg_color = 0;
                break;
            case 1:
                set_mode_flag(BOLD);
                break;
            case 2:
                clear_mode_flag(BOLD);
                break;
            case 4:
                set_mode_flag(UNDERLINE);
                break;
            case 5:
                set_mode_flag(BLINK);
                break;
            case 7:
                set_mode_flag(INVERSE);
                break;
            case 10:
                /* ANSI X3.64-1979 (SCO-ish?)
                 * Select primary font, don't display
                 * control chars if defined, don't set
                 * bit 8 on output.
                 */
                //mCharSet = PRIMARY;
                break;
            case 11:
                /* ANSI X3.64-1979 (SCO-ish?)
                 * Select first alternate font, lets
                 * chars < 32 be displayed as ROM chars.
                 */
                //mCharSet = ALT1;
                break;
            case 12:
                /* ANSI X3.64-1979 (SCO-ish?)
                 * Select second alternate font, toggle
                 * high bit before displaying as ROM char.
                 */
                //mCharSet = ALT2;
                break;
            case 21:
            case 22:
                set_mode_flag(BOLD);
                break;
            case 24:
                clear_mode_flag(UNDERLINE);
                break;
            case 25:
                clear_mode_flag(BLINK);
                break;
            case 27:
                clear_mode_flag(INVERSE);
                break;
            case 38:
                /* ANSI X3.64-1979 (SCO-ish?)
                 * Enables underscore, white foreground
                 * with white underscore (Linux - use
                 * default foreground).
                 */
                fg_color = 7;
                set_mode_flag(UNDERLINE);
                break;
            case 39:
                /* ANSI X3.64-1979 (SCO-ish?)
                 * Disable underline option.
                 * Reset colour to default? It did this
                 * before...
                 */
                fg_color = 7;
                clear_mode_flag(UNDERLINE);
                break;
            case 49:
                bg_color = 0;
                break;
        }
    }
}
  
void GTerm::clear_tab()
{
	if (param[0] == 3) {
		memset(tab_stops, 0, width);
	} else if (param[0] == 0) {
  		tab_stops[cursor_x] = 0;
  	}
}

void GTerm::insert_char()
{
	int n, mx;
	n = param[0]; if (n<1) n = 1;
	mx = width-cursor_x;
	if (n>=mx) {
		clear_area(cursor_x, cursor_y, width-1, cursor_y);
	} else {
		shift_text(cursor_y, cursor_x, width-1, n);
	}
}

void GTerm::screen_align()
{
    int y, yp, x, c;

    c = calc_color(7, 0, 0);
    for (y=0; y<height; y++) {
        yp = linenumbers[y]*width;
        changed_line(y, 0, width-1);
        for (x=0; x<width; x++) {
            text[yp+x] = 'E';
            color[yp+x] = c;
        }
    }
    move_cursor(0,0);
}

void GTerm::erase_char()
{
	int n, mx;
	n = param[0]; if (n<1) n = 1;
	mx = width-cursor_x;
	if (n>mx) n = mx;
	clear_area(cursor_x, cursor_y, cursor_x+n-1, cursor_y);
}

void GTerm::charset_g1()
{
    cur_charset = 1;
}

void GTerm::charset_g0()
{
    cur_charset = 0;
}

void GTerm::cset_g0_a()
{
    charset[0] = 'A';
}

void GTerm::cset_g0_b()
{
    charset[0] = 'B';
}

void GTerm::cset_g0_0()
{
    charset[0] = '0';
}

void GTerm::cset_g0_1()
{
    charset[0] = '1';
}

void GTerm::cset_g0_2()
{
    charset[0] = '2';
}

void GTerm::cset_g1_a()
{
    charset[1] = 'A';
}

void GTerm::cset_g1_b()
{
    charset[1] = 'B';
}

void GTerm::cset_g1_0()
{
    charset[1] = '0';
}

void GTerm::cset_g1_1()
{
    charset[1] = '1';
}

void GTerm::cset_g1_2()
{
    charset[1] = '2';
}

void GTerm::cursor_ypos() {
    int  y;
    y = param[0];	if (y < 1) y=1;
    //	if (mode_flags & CURSORRELATIVE) {
    //		move_cursor(x-1, y-1+scroll_top);
    //	} else {
    //		move_cursor(x-1, y-1);
    //	}
    move_cursor(cursor_x,y - 1);
}
void GTerm::vt52_cursory()
{
	// store y coordinate
	param[0] = (*input_data) - 32;
	if (param[0]<0) param[0] = 0;
	if (param[0]>=height) param[0] = height-1;
}

void GTerm::vt52_cursorx()
{
	int x;
	x = (*input_data)-32;
	if (x<0) x = 0;
	if (x>=width) x = width-1;
	move_cursor(x, param[0]);
}

void GTerm::vt52_ident()
{
	SendBack("\033/Z");
}

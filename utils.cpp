
// Copyright Timothy Miller, 1999


#include "gterm.hpp"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#define ASSERT_X(x) (assert((x) >= 0 && (x) < width))
#define ASSERT_Y(y) (assert((y) >= 0 && (y) < height))

int GTerm::calc_color(int fg, int bg, int flags)
{
	return (flags & 0x0f) | (fg << 4) | (bg << 8);
}

void GTerm::update_changes()
{
    int yp, start_x, mx, end_x;
    int blank, c, x, y;
    constexpr int no_blank = UNDERLINE | INVERSE;
    
    // prevent recursion for scrolls which cause exposures
    if (doing_update) return;
    doing_update = true;

    // first perform scroll-copy
    // TODO: understand pending scroll
    mx = scroll_bot-scroll_top+1;
    if (!(mode_flags & TEXTONLY) && pending_scroll && 
            pending_scroll<mx && -pending_scroll<mx) {

        if (pending_scroll<0) {
            //scroll down
            MoveChars(0, scroll_top, 0, scroll_top-pending_scroll,
                    width, scroll_bot-scroll_top+pending_scroll+1);
        } else {
            MoveChars(0, scroll_top+pending_scroll, 0, scroll_top,
                    width, scroll_bot-scroll_top-pending_scroll+1);
        }
    }
    pending_scroll = 0;

    // then update characters
    for (y=0; y<height; y++) {
        if (dirty_startx[y]>=width) continue;
        yp = linenumbers[y]*width;

        blank = !(mode_flags & TEXTONLY);
        start_x = dirty_startx[y];
        end_x = dirty_endx[y];
        // printf("Line %d dirty from %d to %d\n", y, start_x, end_x);
        c = color[yp+start_x];
        //TODO:optimize
        //use blank to fast clearing
        for (x=start_x; x<=end_x; x++) {
            if ((text[yp+x]!=32 && text[yp+x]) || (no_blank & FLAG(c))!=0) blank = 0;
            if (c != color[yp+x]) {
                if (!blank) {
                    if (inverse_mode)
                        DrawText(0,7, FLAG(c), start_x,
                                y, x-start_x, text+yp+start_x);
                    else
                        DrawText(FGCOLOR(c),BGCOLOR(c), FLAG(c), start_x,
                                y, x-start_x, text+yp+start_x);
                } else {
                    ClearChars(inverse_mode?7:BGCOLOR(c), start_x, y, x-start_x, 1);
                }
                start_x = x;
                c = color[yp+x];
                blank = !(mode_flags & TEXTONLY);
                if ((text[yp+x]!=32 && text[yp+x]) || (no_blank & FLAG(c))!=0) blank = 0;
            }
        }
        if (!blank) {
            if (inverse_mode)
                DrawText(0,7, FLAG(c), start_x,
                        y, x-start_x, text+yp+start_x);
            else
                DrawText(FGCOLOR(c),BGCOLOR(c), FLAG(c), start_x,
                        y, x-start_x, text+yp+start_x);
        } else {
            ClearChars(inverse_mode?7:BGCOLOR(c), start_x, y, x-start_x, 1);
        }

        dirty_endx[y] = 0;
        dirty_startx[y] = width;
    }

    if (!(mode_flags & CURSORINVISIBLE)) {
        x = cursor_x;
        ASSERT_X(x);
        //if (x>=width) x = width-1;
        yp = linenumbers[cursor_y] * width + x;
        c = color[yp];
        DrawCursor(FGCOLOR(c),BGCOLOR(c), FLAG(c), x, cursor_y, text[yp]);
    }

    doing_update = false;
}

/**
 * scroll region in [start_y,end_y] ciclicaly
 * num > 0:scroll up
 * num < 0:scroll down
 * fill scrolled lined with blanks
 * FIXME:vt102 specification(IL,DL) requires the filled blanks have the same
 *       attributes as that of the lines scrolled, which is replaced
 *       with default attribute in this implementation.
 */
void GTerm::scroll_region(int start_y, int end_y, int num)
{
    ASSERT_Y(start_y);
    ASSERT_Y(end_y);
    assert(end_y >= start_y);
    assert(num != 0);

    if (cursor_y >= start_y && cursor_y <= end_y) {
        // Cause the cursor to be erased after scroll
        changed_line(cursor_y, cursor_x, cursor_x);
    }

    int y, takey, fast_scroll, mx, clr, x, yp, c;
    //TODO:optimize
    short *temp = new short[height];
    unsigned char *temp_sx = new unsigned char[height];
    unsigned char *temp_ex = new unsigned char[height];

    //if (!num) return;
    mx = end_y-start_y+1;
    if (num > mx) num = mx;
    if (-num > mx) num = -mx;

    fast_scroll = (start_y == scroll_top && end_y == scroll_bot && 
            !(mode_flags & TEXTONLY));

    if (fast_scroll) pending_scroll += num;

    memcpy(temp, linenumbers, height * sizeof(linenumbers[0]));
    if (fast_scroll) {
        memcpy(temp_sx, dirty_startx,height);
        memcpy(temp_ex, dirty_endx,height);
    }

    c = calc_color(fg_color, bg_color, mode_flags);

    // move the lines by renumbering where they point to
    if (num<mx && -num<mx)
        for (y=start_y; y<=end_y; y++) {
            takey = y+num;
            clr = (takey<start_y) || (takey>end_y);
            if (takey<start_y) takey = end_y+1-(start_y-takey);
            if (takey>end_y) takey = start_y-1+(takey-end_y);

            linenumbers[y] = temp[takey];
            if (!fast_scroll || clr) {
                dirty_startx[y] = 0;
                dirty_endx[y] = width-1;
            } else {
                dirty_startx[y] = temp_sx[takey];
                dirty_endx[y] = temp_ex[takey];
            }
            if (clr) {
                yp = linenumbers[y]*width;
                memset(text+yp, ' ', width);
                //TODO:optimize
                for (x=0; x<width; x++) {
                    color[yp++] = c;
                }
            }
        }

    delete[] temp;
    delete[] temp_sx;
    delete[] temp_ex;
}

/**
 * shift text in [(start_x,y),(end_x,y)]
 * num > 0:shift right
 * num < 0:shift left
 * fill shifted text with blanks
 */
void GTerm::shift_text(int y, int start_x, int end_x, int num)
{
    ASSERT_Y(y);
    ASSERT_X(start_x);
    ASSERT_X(end_x);
    assert(num != 0);
    int x, yp, mx, c;

    //     if (!num) return;

    yp = linenumbers[y]*width;

    mx = end_x-start_x+1;
    if (num>mx) num = mx;
    if (-num>mx) num = -mx;

    if (num<mx && -num<mx) {
        if (num<0) {
            memmove(text+yp+start_x, text+yp+start_x-num, mx+num);
            memmove(color+yp+start_x, color+yp+start_x-num, (mx+num) * sizeof(*color));
        } else {
            memmove(text+yp+start_x+num, text+yp+start_x, mx-num);
            memmove(color+yp+start_x+num, color+yp+start_x, (mx-num) * sizeof(*color));
        }
    }

    if (num<0) {
        x = yp+end_x+num+1;
    } else {
        x = yp+start_x;
    }
    num = abs(num);
    memset(text+x, ' ', num);
    c = calc_color(fg_color, bg_color, mode_flags);
    //TODO:optimize
    while (num--) color[x++] = c;

    changed_line(y, start_x, end_x);
}

void GTerm::clear_area(int start_x, int start_y, int end_x, int end_y)
{
    ASSERT_X(start_x);
    ASSERT_X(end_x);
    ASSERT_Y(start_y);
    ASSERT_Y(end_y);

    int x, y, c, yp, w;

    c = calc_color(fg_color, bg_color, mode_flags);

    w = end_x - start_x + 1;
    assert(w >= 0);
    //if (w<1) return;

    for (y=start_y; y<=end_y; y++) {
        yp = linenumbers[y]*width;
        memset(text+yp+start_x, ' ', w);
        for (x=start_x; x<=end_x; x++) {
            color[yp+x] = c;
        }
        changed_line(y, start_x, end_x);
    }
}

void GTerm::changed_line(int y, int start_x, int end_x)
{
    ASSERT_X(start_x);
    ASSERT_X(end_x);
    ASSERT_Y(y);

	if (dirty_startx[y] > start_x) dirty_startx[y] = start_x;
	if (dirty_endx[y] < end_x) dirty_endx[y] = end_x;
}

void GTerm::move_cursor(int x, int y)
{
    ASSERT_X(x);
    ASSERT_Y(y);

    changed_line(cursor_y, cursor_x, cursor_x);
    cursor_x = x;
    cursor_y = y;
    if (cursor_x>=width) cursor_x = width-1;
    if (cursor_y>=height) cursor_y = height-1;

    force_wrap = false;
}

void GTerm::set_mode_flag(int flag)
{
	mode_flags |= flag;
	ModeChange(mode_flags);
}

void GTerm::clear_mode_flag(int flag)
{
	mode_flags &= ~flag;
	ModeChange(mode_flags);
}

/*
 * Translate a string to the display form.  This assumes the font has the
 * DEC graphic characters in cells 0-31, and otherwise is ISO-8859-1.
 * modified from xterm source:charsets.c:xtermCharSetOut()
 */
void GTerm::translate_charset(unsigned char *buf,unsigned char *ptr)
{
	unsigned char *s;
    char cs = charset[cur_charset];

	for (s = buf; s < ptr; ++s) {
		switch (cs) {
		case 'A':	/* United Kingdom set (or Latin 1)	*/

#if OPT_XMC_GLITCH
		case '?':
#endif
		case '1':	/* Alternate Character ROM standard characters */
		case '2':	/* Alternate Character ROM special graphics */
		case 'B':	/* ASCII set				*/
			break;

		case '0':	/* special graphics (line drawing)	*/
			if (*s >= 0x5f && *s <= 0x7e) {
#if OPT_WIDE_CHARS
				if (screen->utf8_mode)
				    *s = dec2ucs[chr - 0x5f];
				else
#endif
				    *s = (*s == 0x5f) ? 0x7f : (*s - 0x5f);
			}
			break;

		default:	/* any character sets we don't recognize*/
			break;
		}
	}
}

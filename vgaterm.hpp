#ifndef INCLUDED_VGA_TERM_HPP
#define INCLUDED_VGA_TERM_HPP

#include "gterm.hpp"

class VGATerm : public GTerm {
public:
    VGATerm() : GTerm(80, 24) { 
        //set_mode_flag(TEXTONLY);
        set_mode_flag(DEFERUPDATE);
        set_mode_flag(NEWLINE);
    }
    virtual ~VGATerm();

    void DrawText(int fg_color, int bg_color, int flags,
            int x, int y, int len, unsigned char *string);
    void DrawCursor(int fg_color, int bg_color, int flags,
            int x, int y, unsigned char c);

    void MoveChars(int sx, int sy, int dx, int dy, int w, int h);
    void ClearChars(int bg_color, int x, int y, int w, int h);
	void SendBack(const char *data);
	void Bell();
	void RequestSizeChange(int w, int h);
};


#endif
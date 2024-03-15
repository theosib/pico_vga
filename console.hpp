#ifndef INCLUDED_VGACONSOLE_HPP
#define INCLUDED_VGACONSOLE_HPP

#include "vgaterm.hpp"
#include "mouse.hpp"
#include <vector>

typedef void (*task_f)();

struct VGAConsole : public KeyReceiver {
    VGATerm *term;
    VGAMouse *mouse;
    
    VGAConsole(VGATerm *t, VGAMouse *m) {
        term = t;
        mouse = m;
    }
    
    std::vector<task_f> tasks;
    void add_task(task_f t) { tasks.push_back(t); }
    
    // Ring buffer for characters from serial input
    volatile int buf_head = 0;
    volatile int buf_tail = 0;
    uint8_t text_buffer[4096];
    
    // Insert character into ring buffer
    void add_char(uint8_t c) {
        int next_tail = (buf_tail + 1) & 4095;
        while (next_tail == buf_head);
        text_buffer[buf_tail] = c;
        buf_tail = next_tail;
    }
    
    void console_loop();
    
    virtual void report_key_pressed(int ch) {
        add_char(ch);
    }
};


#endif

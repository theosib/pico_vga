#include "console.hpp"

void VGAConsole::console_loop()
{
    bool pending = true;
    //uint64_t timeout = time_us_64() + 16666;
    bool was_in_vblank = false;
    bool in_vblank_now = false;
    bool do_terminal = false;
    for (;;) {
        do_terminal = false;
        for (;;) {
            for (task_f t : tasks) (*t)();
            
            in_vblank_now = term->graphics->video->in_vblank();
            if (!was_in_vblank && in_vblank_now) do_terminal = true;
            was_in_vblank = in_vblank_now;
            if (do_terminal) break;
            if (buf_head != buf_tail) break;
        }
        
        int tc = 0;
        if (buf_head != buf_tail) {
            // If we have characters from the serial bus, send them in bulk 
            // to the terminal emulator
            int tail = buf_tail;
            int head = buf_head;
            if (tail > head) {
                int count = tail - head;
                tc = count;
                // XXX Calls to ProcessInput might draw text; must hide mouse
                term->ProcessInput(count, text_buffer + head);
                buf_head = tail;
            } else {
                int count = 4096 - head;
                tc = count;
                term->ProcessInput(count, text_buffer + head);
                count = tail;
                tc += count;
                term->ProcessInput(count, text_buffer);
                buf_head = tail;
            }
            pending = true;
        }
        
        // We only need to update the display once every video frame, so we
        // wait until 1/60 sec has elapsed and tell the emulator to perform
        // all pending display updates. The update is skipped if the 
        // terminal is falling behind.
        if (do_terminal && tc < 2048) {
            if (pending) {
                mouse->hide_mouse();
                term->Update();
                mouse->draw_mouse();
            } else {
                mouse->draw_mouse();
            }
            pending = false;
            // timeout = time_us_64() + 16666;
        }
    }
}
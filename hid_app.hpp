#ifndef INCLUDED_HIDHOST_HPP
#define INCLUDED_HIDHOST_HPP

#include "bsp/board.h"
#include "tusb.h"


extern void hid_app_task();

// Some code copied from https://github.com/dquadros/RPTerm/blob/main/keybd.cpp

struct KeyReceiver {
    virtual void report_key_pressed(int ch) = 0;
};

struct MouseReceiver {
    virtual void report_mouse_moved(int x, int y) = 0;
};

struct HIDHost {    
    KeyReceiver *key_rec = 0;
    MouseReceiver *mouse_rec = 0;
    
    void setKeyReceiver(KeyReceiver *p) { key_rec = p; }
    void setMouseReceiver(MouseReceiver *p) { mouse_rec = p; }
        
    static const int USE_ANSI_ESCAPE = 0;
    static const int MAX_REPORT = 4;
    static const int MAX_KEY = 6;
    struct {
      uint8_t report_count;
      tuh_hid_report_info_t report_info[MAX_REPORT];
    } hid_info[CFG_TUH_HID];
    
    hid_keyboard_report_t prev_keyboard_report = { 0, 0, {0} }; // previous report to check key released
    hid_mouse_report_t prev_mouse_report = { 0 };
    
    uint8_t keybd_dev_addr = 0xFF;
    uint8_t keybd_instance;
    
    static const int REPEAT_START = 1000 * 1000;
    static const int REPEAT_INTERVAL = 100 * 1000;
    uint8_t  repeat_keycode [MAX_KEY] = {};
    uint8_t  repeat_char [MAX_KEY] = {};
    uint32_t repeat_time [MAX_KEY] = {};

    // Caps lock control
    bool capslock_key_down_in_last_report = false;
    bool capslock_key_down_in_this_report = false;
    bool capslock_on = false;

    // Keyboard LED control
    uint8_t leds = 0;
    uint8_t prev_leds = 0xFF;
    
    void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len);
    void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance);
    void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
    void hid_app_task();
    
    void cursor_movement(int8_t x, int8_t y, int8_t wheel);
    void process_kbd_report(hid_keyboard_report_t const *report);
    void process_mouse_report(hid_mouse_report_t const * report);
    void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
    
    
    uint8_t key_buffer[256];
    volatile uint8_t key_buf_head = 0;
    volatile uint8_t key_buf_tail = 0;
    
    void add_buf_key(uint8_t ch) {
        uint8_t next_tail = key_buf_tail+1;
        if (next_tail != key_buf_head) {
            key_buffer[key_buf_tail] = ch;
            key_buf_tail = next_tail;
        }
    }
        
    int get_buf_key() {
        if (key_buf_head != key_buf_tail) {
            return key_buffer[key_buf_head++];
        } else {
            return -1;
        }
    }
    
    void put_kbd(int ch);
    
    // void start() {
    //     tuh_init(BOARD_TUH_RHPORT);
    // }
    
    HIDHost() {
        //printf("Starting HID host\n");
    }
};


#endif
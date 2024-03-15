/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021, Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "hid_app.hpp"

HIDHost *usb_hid = 0;

//--------------------------------------------------------------------+
// MACRO TYPEDEF CONSTANT ENUM DECLARATION
//--------------------------------------------------------------------+

// If your host terminal support ansi escape code such as TeraTerm
// it can be use to simulate mouse cursor movement within terminal

static uint8_t const keycode2ascii[128][2] =  { HID_KEYCODE_TO_ASCII };
static constexpr int NKEYS = (sizeof(keycode2ascii) / sizeof(keycode2ascii[0]));

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void HIDHost::tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
  printf("HID device address = %d, instance = %d is mounted\r\n", dev_addr, instance);

  // Interface protocol (hid_interface_protocol_enum_t)
  const char* protocol_str[] = { "None", "Keyboard", "Mouse" };
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  printf("HID Interface Protocol = %s\r\n", protocol_str[itf_protocol]);

  // By default host stack will use activate boot protocol on supported interface.
  // Therefore for this simple example, we only need to parse generic report descriptor (with built-in parser)
    hid_info[instance].report_count = tuh_hid_parse_report_descriptor(hid_info[instance].report_info, MAX_REPORT, desc_report, desc_len);
    printf("HID has %u reports \r\n", hid_info[instance].report_count);
    
    for (int i = 0; i < hid_info[instance].report_count; i++) 
    {
      if ((hid_info[instance].report_info[i].usage_page == HID_USAGE_PAGE_DESKTOP) && 
          (hid_info[instance].report_info[i].usage == HID_USAGE_DESKTOP_KEYBOARD)) 
      {
          keybd_dev_addr = dev_addr;
          keybd_instance = instance;
      }
    }

  // request to receive report
  // tuh_hid_report_received_cb() will be invoked when report is available
  if ( !tuh_hid_receive_report(dev_addr, instance) )
  {
    printf("Error: cannot request to receive report\r\n");
  }
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
    if (usb_hid) {
        usb_hid->tuh_hid_mount_cb(dev_addr, instance, desc_report, desc_len);
    } else {
        printf("tuh_hid_mount_cb trampoline error\n");
    }
}

void HIDHost::tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
    printf("HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
    keybd_dev_addr = 0xFF; // keyboard not available
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
    if (usb_hid) {
        usb_hid->tuh_hid_umount_cb(dev_addr, instance);
    } else {
        printf("tuh_hid_umount_cb trampoline error\n");
    }
}

// Invoked when received report from device via interrupt endpoint
void HIDHost::tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  switch (itf_protocol)
  {
    case HID_ITF_PROTOCOL_KEYBOARD:
      TU_LOG2("HID receive boot keyboard report\r\n");
      process_kbd_report( (hid_keyboard_report_t const*) report );
    break;

    case HID_ITF_PROTOCOL_MOUSE:
      TU_LOG2("HID receive boot mouse report\r\n");
      process_mouse_report( (hid_mouse_report_t const*) report );
    break;

    default:
      // Generic report requires matching ReportID and contents with previous parsed report info
      process_generic_report(dev_addr, instance, report, len);
    break;
  }

  // continue to request to receive report
  if ( !tuh_hid_receive_report(dev_addr, instance) )
  {
    printf("Error: cannot request to receive report\r\n");
  }
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    if (usb_hid) {
        usb_hid->tuh_hid_report_received_cb(dev_addr, instance, report, len);
    } else {
        printf("tuh_hid_report_received_cb trampoline error\n");
    }
}


//--------------------------------------------------------------------+
// Keyboard
//--------------------------------------------------------------------+

// look up new key in previous keys
static inline bool find_key_in_report(hid_keyboard_report_t const *report, uint8_t keycode, int max_key)
{
  for(uint8_t i=0; i<max_key; i++)
  {
    if (report->keycode[i] == keycode)  return true;
  }

  return false;
}

void HIDHost::process_kbd_report(hid_keyboard_report_t const *report)
{
    // clear released keys in the auto repeat control
    for (int i = 0; i < MAX_KEY; i++)
    {
      if (repeat_keycode[i] && !find_key_in_report(report, repeat_keycode[i], MAX_KEY)) {
        repeat_keycode[i] = 0;
        repeat_char[i] = 0;
      }
    }
    
    // Check caps lock
    capslock_key_down_in_this_report = find_key_in_report(report, HID_KEY_CAPS_LOCK, MAX_KEY);
    if (capslock_key_down_in_this_report && !capslock_key_down_in_last_report)
    {
      // CAPS LOCK was pressed
      capslock_on = !capslock_on;
      if (capslock_on)
      {
        leds |= KEYBOARD_LED_CAPSLOCK;
      }
      else
      {
        leds &= ~KEYBOARD_LED_CAPSLOCK;
      }
    }
    
    
  //------------- example code ignore control (non-printable) key affects -------------//
    for(uint8_t i=0; i<MAX_KEY; i++) {
        uint8_t key = report->keycode[i];
        if (key == 0) continue;
        if (key == HID_KEY_CAPS_LOCK) continue;
    
        if ( !find_key_in_report(&prev_keyboard_report, report->keycode[i], MAX_KEY) ) {
            uint8_t ch = (key < NKEYS) ? keycode2ascii[key][0] : 0; // unshifted key code, to test for letters
            bool const is_ctrl = report->modifier & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL);
            bool is_shift = report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
            bool is_alt = report->modifier & (KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_RIGHTALT);
            
            if (capslock_on && (ch >= 'a') && (ch <= 'z'))
            {
              // capslock affects only letters
              is_shift = !is_shift;
            }
            ch = (key < NKEYS) ? keycode2ascii[key][is_shift ? 1 : 0] : 0;
            
            if (is_ctrl)
            {
              // control char
              if ((ch >= 0x60) && (ch <= 0x7F))
              {
                ch = ch - 0x60;
              }
              else if ((ch >= 0x40) && (ch <= 0x5F))
              {
                ch = ch - 0x40;
              }
            }
            
            // record key for auto repeat
            for (int j = 0; j < MAX_KEY; j++)
            {
              if (repeat_keycode[j] == 0)
              {
                repeat_keycode[j] = key;
                repeat_char[j] = ch;
                repeat_time[j] = time_us_64() + REPEAT_START;
                break;
              }
            }
            
            put_kbd(ch);
        }
      // TODO example skips key released
    }

    prev_keyboard_report = *report;
    capslock_key_down_in_last_report = capslock_key_down_in_this_report;
}

void HIDHost::put_kbd(int ch)
{
    // char buf[10];
    // sprintf(buf, "%d", ch);
    // for (int i=0; buf[i]; i++) add_buf_key(buf[i]);
    add_buf_key(ch);
    // if (key_rec) key_rec->report_key_pressed(ch);
    // putchar(ch);
    // if ( ch == '\r' ) putchar('\n'); // added new line for enter key
    // fflush(stdout); // flush right away, else nanolib will wait for newline
}


//--------------------------------------------------------------------+
// Mouse
//--------------------------------------------------------------------+

void HIDHost::cursor_movement(int8_t x, int8_t y, int8_t wheel)
{
  //printf("(%d %d %d)\r\n", x, y, wheel);
  if (mouse_rec) mouse_rec->report_mouse_moved(x, y);
}

void HIDHost::process_mouse_report(hid_mouse_report_t const * report)
{
  //------------- button state  -------------//
  uint8_t button_changed_mask = report->buttons ^ prev_mouse_report.buttons;
  if ( button_changed_mask & report->buttons)
  {
    printf(" %c%c%c ",
       report->buttons & MOUSE_BUTTON_LEFT   ? 'L' : '-',
       report->buttons & MOUSE_BUTTON_MIDDLE ? 'M' : '-',
       report->buttons & MOUSE_BUTTON_RIGHT  ? 'R' : '-');
  }

  //------------- cursor movement -------------//
  cursor_movement(report->x, report->y, report->wheel);
  
  prev_mouse_report = *report;
}

//--------------------------------------------------------------------+
// Generic Report
//--------------------------------------------------------------------+
void HIDHost::process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  (void) dev_addr;

  uint8_t const rpt_count = hid_info[instance].report_count;
  tuh_hid_report_info_t* rpt_info_arr = hid_info[instance].report_info;
  tuh_hid_report_info_t* rpt_info = NULL;

  if ( rpt_count == 1 && rpt_info_arr[0].report_id == 0)
  {
    // Simple report without report ID as 1st byte
    rpt_info = &rpt_info_arr[0];
  }else
  {
    // Composite report, 1st byte is report ID, data starts from 2nd byte
    uint8_t const rpt_id = report[0];

    // Find report id in the array
    for(uint8_t i=0; i<rpt_count; i++)
    {
      if (rpt_id == rpt_info_arr[i].report_id )
      {
        rpt_info = &rpt_info_arr[i];
        break;
      }
    }

    report++;
    len--;
  }

  if (!rpt_info)
  {
    printf("Couldn't find the report info for this report !\r\n");
    return;
  }

  // For complete list of Usage Page & Usage checkout src/class/hid/hid.h. For examples:
  // - Keyboard                     : Desktop, Keyboard
  // - Mouse                        : Desktop, Mouse
  // - Gamepad                      : Desktop, Gamepad
  // - Consumer Control (Media Key) : Consumer, Consumer Control
  // - System Control (Power key)   : Desktop, System Control
  // - Generic (vendor)             : 0xFFxx, xx
  if ( rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP )
  {
    switch (rpt_info->usage)
    {
      case HID_USAGE_DESKTOP_KEYBOARD:
        TU_LOG1("HID receive keyboard report\r\n");
        // Assume keyboard follow boot report layout
        process_kbd_report( (hid_keyboard_report_t const*) report );
      break;

      case HID_USAGE_DESKTOP_MOUSE:
        TU_LOG1("HID receive mouse report\r\n");
        // Assume mouse follow boot report layout
        process_mouse_report( (hid_mouse_report_t const*) report );
      break;

      default: break;
    }
  }
}

void HIDHost::hid_app_task()
{
  // update keyboard leds
  if (keybd_dev_addr != 0xFF)
  { // only if keyboard attached
    if (leds != prev_leds)
    {
      tuh_hid_set_report(keybd_dev_addr, keybd_instance, 0, HID_REPORT_TYPE_OUTPUT, &leds, sizeof(leds));
      prev_leds = leds;
    }

    // auto-repeat keys
    for (int i = 0; i < MAX_KEY; i++)
    {
      if (repeat_char[i] && (time_us_64() > repeat_time[i]))
      {
        put_kbd(repeat_char[i]);
        repeat_time[i] += REPEAT_INTERVAL;
      }
    }
  }
}

void hid_app_task()
{
    if (usb_hid) {
        usb_hid->hid_app_task();
    } else {
        printf("hid_app_task trampoline error\n");
    }
}

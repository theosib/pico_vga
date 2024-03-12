#ifndef INCLUDED_VGAVIDEO_HPP
#define INCLUDED_VGAVIDEO_HPP

#include <stdint.h>
#include <stdio.h>

typedef void (*hsr_f)();

struct VGAVideo {
    // Video timing numbers for 640x480@60
    static constexpr int HACTIVE = 640;
    static constexpr int HFP = 16;
    static constexpr int HSYNC = 96;
    static constexpr int HBP = 48;
    static constexpr int VACTIVE = 480;
    static constexpr int VFP = 11;
    static constexpr int VSYNC = 2;
    static constexpr int VBP = 31;
    static constexpr int VTOTAL = VACTIVE + VFP + VSYNC + VBP;
    
    static constexpr int VID_DMA = 0;
    
    static constexpr int PIN_BASE = 2;
    static constexpr int HSYNC_PIN = 6;
    static constexpr int VSYNC_PIN = 7;
    static constexpr int R_PIN = 2;    // Red
    static constexpr int G_PIN = 3;    // Green
    static constexpr int B_PIN = 4;    // Blue
    static constexpr int L_PIN = 5;    // Lighten
    
    volatile int scanline = 0;
    hsr_f isr_ptr;
    void hblank_isr();
    
    static bool in_vactive(int n) { return n < VACTIVE; }
    static bool in_vfp(int n) { return n>=VACTIVE && n<(VACTIVE+VFP); }
    static bool in_vsync(int n) { return n>=(VACTIVE+VFP) && n<(VACTIVE+VFP+VSYNC); }
    static bool in_vbp(int n) { return n>=(VACTIVE+VFP+VSYNC) && n<VTOTAL; }
    static bool in_vblank(int n) { return n >= VACTIVE; }
    
    bool in_vactive() { return in_vactive(scanline); }
    bool in_vfp() { return in_vfp(scanline); }
    bool in_vsync() { return in_vsync(scanline); }
    bool in_vbp() { return in_vbp(scanline); }
    bool in_vblank() { return in_vbp(scanline); }
    
    // Video memory
    uint8_t *framebuffer = 0;
    // Pointers to individual scanlines
    uint8_t *line_pointers[481];
    void init_line_pointers();
    void start();
    
    VGAVideo(hsr_f p) {
        printf("Setting up video\n");
        isr_ptr = p;
        printf("Setting up pointers\n");
        init_line_pointers();
        printf("Setting up pio\n");
        printf("Done setting up\n");
    }
    
    uint8_t *& get_row(int y) { return line_pointers[y]; }
    
    
};


#endif

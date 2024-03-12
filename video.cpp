
#include "video.hpp"
#include "pio-vga.pio.h"
#include <string.h>
#include <stdio.h>
// #include "hardware/clocks.h"


extern VGAVideo *video;

void __not_in_flash_func(hblank_isr)()
{
    video->hblank_isr();
}

// Allocate framebuffer and load scanline pointers
void VGAVideo::init_line_pointers()
{
    uint32_t bytes = 324 * 481;
    framebuffer = new uint8_t[bytes];
    memset(framebuffer, 0, bytes);
    for (int i=0; i<481; i++) {
        line_pointers[i] = framebuffer + i*324;
    }
}

// Horizontal blank interrupt service routine
void __not_in_flash_func(VGAVideo::hblank_isr)()
{
    bool active = in_vactive(scanline);
    if (active) {
        // For active video, start DMA for current scanline
        vga_start_send_dma(VID_DMA, line_pointers[scanline], 321);
    } else {
        // For vertical blank, control vsync signal and send black scanline
        gpio_put(VSYNC_PIN, in_vsync(scanline));
        vga_start_send_dma(VID_DMA, line_pointers[480], 321);
    }

    // Increment scanline
    scanline++;
    if (scanline == VTOTAL) scanline = 0;    
    
    // This kind of ISR requires explicit reset of the IRQ
    pio_interrupt_clear(pio0, 0);
}

// Setup the PIO to scan out video
void VGAVideo::start()
{
    printf("Enter setup\n");
    // Sync bits are active low
    printf("Sync pins\n");
    gpio_set_outover(VSYNC_PIN, GPIO_OVERRIDE_INVERT);
    gpio_set_outover(HSYNC_PIN, GPIO_OVERRIDE_INVERT);
    
    // Vblank ISR handles VSYNC manually.
    gpio_init(VSYNC_PIN);
    gpio_set_dir(VSYNC_PIN, GPIO_OUT);    
    
    printf("Add program\n");
    uint vga_data_offset = pio_add_program(pio0, &vga_data_program);        
    vga_configure_send_dma(VID_DMA, pio0, 0);
    
    printf("Setting ISR to %p\n", isr_ptr);
    setup_vga_irq(pio0, isr_ptr);

    // The pixel clock is 25MHz, but two cycles of the SM are required for
    // each pixel.
    printf("Init PIO\n");
    vga_data_init(pio0, 0, vga_data_offset, PIN_BASE, HSYNC_PIN, 50000000);
    
    printf("Done setup");
}

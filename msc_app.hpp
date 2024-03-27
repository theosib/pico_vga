#ifndef INCLUDED_MSCHOST_HPP
#define INCLUDED_MSCHOST_HPP

#include <ctype.h>
#include "tusb.h"
#include "bsp/board.h"
#include "ff.h"
#include "diskio.h"


extern void msc_app_task();


struct MSCHost {
    FATFS fatfs[CFG_TUH_DEVICE_MAX]; // for simplicity only support 1 LUN per device
    volatile bool _disk_busy[CFG_TUH_DEVICE_MAX] = {};
    scsi_inquiry_resp_t inquiry_resp;
    
    void init();
    void msc_app_task();
    
    bool inquiry_complete_cb(uint8_t dev_addr, tuh_msc_complete_data_t const * cb_data);
    void tuh_msc_mount_cb(uint8_t dev_addr);
    void tuh_msc_umount_cb(uint8_t dev_addr);
    
    void wait_for_disk_io(BYTE pdrv);
    
};


#endif

#include "msc_app.hpp"

MSCHost *usb_msc = 0;

void msc_app_task()
{
    if (usb_msc) {
        usb_msc->msc_app_task();
    } else {
        printf("msc_app_task trampoline error\n");
    }
}

void MSCHost::init()
{
}

void MSCHost::msc_app_task()
{
    
}

bool inquiry_complete_cb(uint8_t dev_addr, tuh_msc_complete_data_t const * cb_data)
{
    if (usb_msc) {
        return usb_msc->inquiry_complete_cb(dev_addr, cb_data);
    } else {
        printf("inquiry_complete_cb trampoline error\n");
        return false;
    }
}

bool MSCHost::inquiry_complete_cb(uint8_t dev_addr, tuh_msc_complete_data_t const * cb_data)
{
  msc_cbw_t const* cbw = cb_data->cbw;
  msc_csw_t const* csw = cb_data->csw;

  if (csw->status != 0)
  {
    printf("Inquiry failed\r\n");
    return false;
  }

  // Print out Vendor ID, Product ID and Rev
  printf("%.8s %.16s rev %.4s\r\n", inquiry_resp.vendor_id, inquiry_resp.product_id, inquiry_resp.product_rev);

  // Get capacity of device
  uint32_t const block_count = tuh_msc_get_block_count(dev_addr, cbw->lun);
  uint32_t const block_size = tuh_msc_get_block_size(dev_addr, cbw->lun);

  printf("Disk Size: %lu MB\r\n", block_count / ((1024*1024)/block_size));
  // printf("Block Count = %lu, Block Size: %lu\r\n", block_count, block_size);

  // For simplicity: we only mount 1 LUN per device
  uint8_t const drive_num = dev_addr-1;
  char drive_path[3] = "0:";
  drive_path[0] += drive_num;

  if ( f_mount(&fatfs[drive_num], drive_path, 1) != FR_OK )
  {
    puts("mount failed");
  }

  // change to newly mounted drive
  f_chdir(drive_path);

  // print the drive label
//  char label[34];
//  if ( FR_OK == f_getlabel(drive_path, label, NULL) )
//  {
//    puts(label);
//  }

  return true;
}


void tuh_msc_mount_cb(uint8_t dev_addr)
{
    if (usb_msc) {
        usb_msc->tuh_msc_mount_cb(dev_addr);
    } else {
        printf("tuh_msc_mount_cb trampoline error\n");
    }
}

void tuh_msc_umount_cb(uint8_t dev_addr)
{
    if (usb_msc) {
        usb_msc->tuh_msc_umount_cb(dev_addr);
    } else {
        printf("tuh_msc_umount_cb trampoline error\n");
    }
}


void MSCHost::tuh_msc_mount_cb(uint8_t dev_addr)
{
  printf("A MassStorage device is mounted\r\n");

  uint8_t const lun = 0;
  tuh_msc_inquiry(dev_addr, lun, &inquiry_resp, ::inquiry_complete_cb, 0);
}

void MSCHost::tuh_msc_umount_cb(uint8_t dev_addr)
{
  printf("A MassStorage device is unmounted\r\n");

  uint8_t const drive_num = dev_addr-1;
  char drive_path[3] = "0:";
  drive_path[0] += drive_num;

  f_unmount(drive_path);

//  if ( phy_disk == f_get_current_drive() )
//  { // active drive is unplugged --> change to other drive
//    for(uint8_t i=0; i<CFG_TUH_DEVICE_MAX; i++)
//    {
//      if ( disk_is_ready(i) )
//      {
//        f_chdrive(i);
//        cli_init(); // refractor, rename
//      }
//    }
//  }
}


extern void all_core1_tasks();


void MSCHost::wait_for_disk_io(BYTE pdrv)
{
  while(_disk_busy[pdrv])
  {
    all_core1_tasks();
  }
}

static bool disk_io_complete(uint8_t dev_addr, tuh_msc_complete_data_t const * cb_data)
{
  (void) dev_addr; (void) cb_data;
  if (!usb_msc) return false;
  usb_msc->_disk_busy[dev_addr-1] = false;
  return true;
}


DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
  uint8_t dev_addr = pdrv + 1;
  return tuh_msc_mounted(dev_addr) ? 0 : STA_NODISK;
}

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
  (void) pdrv;
	return 0; // nothing to do
}

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	uint8_t const dev_addr = pdrv + 1;
	uint8_t const lun = 0;

	usb_msc->_disk_busy[pdrv] = true;
	tuh_msc_read10(dev_addr, lun, buff, sector, (uint16_t) count, disk_io_complete, 0);
    if (!usb_msc) usb_msc->wait_for_disk_io(pdrv);

	return RES_OK;
}

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	uint8_t const dev_addr = pdrv + 1;
	uint8_t const lun = 0;

	usb_msc->_disk_busy[pdrv] = true;
	tuh_msc_write10(dev_addr, lun, buff, sector, (uint16_t) count, disk_io_complete, 0);
    if (!usb_msc) usb_msc->wait_for_disk_io(pdrv);

	return RES_OK;
}

#endif

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
  uint8_t const dev_addr = pdrv + 1;
  uint8_t const lun = 0;
  switch ( cmd )
  {
    case CTRL_SYNC:
      // nothing to do since we do blocking
      return RES_OK;

    case GET_SECTOR_COUNT:
      *((DWORD*) buff) = (WORD) tuh_msc_get_block_count(dev_addr, lun);
      return RES_OK;

    case GET_SECTOR_SIZE:
      *((WORD*) buff) = (WORD) tuh_msc_get_block_size(dev_addr, lun);
      return RES_OK;

    case GET_BLOCK_SIZE:
      *((DWORD*) buff) = 1;    // erase block size in units of sector size
      return RES_OK;

    default:
      return RES_PARERR;
  }

	return RES_OK;
}

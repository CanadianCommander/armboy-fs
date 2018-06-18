# armboy-fs
army boy file system driver.

## hardware support 
this driver is designed to operate on the Arduino Due (sam3x8e) MCU. It supports the following block devices 
- SD / SDHC card in SPI mode

## file system support 
this driver currently supports  MBR and GPT partion maps. Within a partion it supports the FAT32 file system w/ VFAT support. This driver has been tested and is compatiable with MacOs and Linux FAT32 drivers (though MacOs does like to all CAPS file names, look out for that").

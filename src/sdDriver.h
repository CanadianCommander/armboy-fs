#ifndef SD_DRIVER_H_
#define SD_DRIVER_H_
#include <stdbool.h>
#include <stdint.h>

#define TIMEOUT_CLK 16
#define RW_BLOCK_SIZE 512

bool initDefault(void);

/**
  read a block of data from the SD card.
  buffer MUST be RW_BLCOK_SIZE bytes.
  addr is in blocks.
*/
bool readBlock(uint8_t * buffer, uint32_t addr);

#endif /* SD_DRIVER_H_ */

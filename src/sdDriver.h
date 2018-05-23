#ifndef SD_DRIVER_H_
#define SD_DRIVER_H_
#include <stdbool.h>
#include <stdint.h>

#define TIMEOUT_CLK 16
#define TIMEOUT_RW_BLOCK 16000
#define RW_BLOCK_SIZE 512

/**
  init SD Card
*/
bool initDefault(void);

/**
  get card CSD register. put its value in buffer.
  buffer MUST be 16 or more bytes.
*/
bool getCardCSD(uint8_t * buffer);
#define CSD_RESPONSE_SIZE 16

/**
  read a block of data from the SD card.
  buffer MUST be RW_BLCOK_SIZE bytes.
  addr is in blocks.
*/
bool readBlock(uint8_t * buffer, uint32_t addr);


/**
  write block in buffer to addr.
  buffer must be RW_BLOCK_SIZE
*/
bool writeBlock(uint8_t * buffer, uint32_t addr);

#endif /* SD_DRIVER_H_ */

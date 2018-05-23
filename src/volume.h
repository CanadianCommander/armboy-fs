#ifndef VOLUME_H_
#define VOLUME_H_
#include <stdbool.h>
#include <stdint.h>

/**
  underlying interface abstraction pointers
*/
typedef uint32_t (*volumeSizeFunction)  (void);
typedef uint16_t (*blockSizeFunction)   (void);
typedef bool     (*readBlockFunction)   (uint8_t *, uint32_t);
typedef bool     (*writeBlockFunction)  (uint8_t *, uint32_t);

typedef struct {
  volumeSizeFunction  vsf;
  blockSizeFunction   bsf;
  readBlockFunction   rbf;
  writeBlockFunction  wbf;
} Volume;

//fill out Volume structure with provided abstraction functions
void setAbstractionPtrs(Volume * v, volumeSizeFunction vsf, blockSizeFunction bsf, readBlockFunction rbf, writeBlockFunction wbf);

/**
  like setAbstractionPtrs but uses default abstraction ptrs for SD card
*/
void setAbstractionPtrsSDCard(Volume * v);

uint32_t getVolumeSize(Volume * v);
uint16_t getVolumeBlockSize(Volume * v);
bool     readVolumeBlock(Volume * v, uint8_t * buffer, uint32_t addr);
bool     writeVolumeBlock(Volume * v, uint8_t * buffer, uint32_t addr);


#endif /*VOLUME_H_*/

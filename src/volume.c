#include "volume.h"
#include "sdDriver.h"
#include <stdio.h>

void setAbstractionPtrs(Volume * v, volumeSizeFunction vsf, blockSizeFunction bsf, readBlockFunction rbf, writeBlockFunction wbf){
  v->vsf = vsf;
  v->bsf = bsf;
  v->rbf = rbf;
  v->wbf = wbf;
}

void setAbstractionPtrsSDCard(Volume * v){
  v->vsf = getCardSize;
  v->bsf = getBlockSize;
  v->rbf = readBlock;
  v->wbf = writeBlock;
}

uint32_t getVolumeSize(Volume * v){
  return v->vsf();
}

uint16_t getVolumeBlockSize(Volume * v){
    return v->bsf();
}

bool readVolumeBlock(Volume * v, uint8_t * buffer, uint32_t addr){
  return v->rbf(buffer, addr);
}

bool writeVolumeBlock(Volume * v, uint8_t * buffer, uint32_t addr){
  return v->wbf(buffer, addr);
}

#include "fat32.h"
#include "volume.h"
#include <util/debug.h>
#include <memory.h>

static Volume currVolume;

static FATInfo fatInformation;


#define IS_EFI_PART(buff) (buff[0] == 0x45 && buff[1] == 0x46 && buff[2] == 0x49 && buff[3] == 0x20 && buff[4] == 0x50 \
  && buff[5] == 0x41 && buff[6] == 0x52 && buff[7] == 0x54)

#define IS_FAT_PART_ENTRY(buff) (*(uint64_t*)(buff + 0x10) == 0x416b150d6e6e79b8 && \
  *(uint64_t*)(buff + 0x18) == 0x0c9b223ef6e13186)


static uint64_t locateFAT(){
  uint8_t buff[getVolumeBlockSize(&currVolume)];

  readVolumeBlock(&currVolume, buff, 1);

  if(IS_EFI_PART(buff)){
    uint64_t partArrayStart = *(uint64_t*)(buff + 0x48);
    uint32_t partArrayLen = *(uint32_t*)(buff + 0x50);
    printf("Partition Array Start: %d len: %d\n", (uint32_t)partArrayStart, partArrayLen);
    for(uint64_t i = partArrayStart; i < partArrayStart + partArrayLen; i++){
      readVolumeBlock(&currVolume, buff, i);
      if(buff[0x10] != 0x00){
        dumpHex(buff,512);
      }
      if(IS_FAT_PART_ENTRY(buff)){
        printf("FAT\n");
        dumpHex(buff,512);
        //return *(uint64_t*)(buff + 0x20);
      }
    }
  }
  else {
    return 0;
  }
}

//fill in the FAT information structure
static void populateFATInformation(uint64_t fatStart){
  uint8_t buff[getVolumeBlockSize(&currVolume)];

  readVolumeBlock(&currVolume, buff, fatStart);

  fatInformation.partitionStart = fatStart;
  memcpy(fatInformation.oemName, buff + 0x03, 8);
  fatInformation.blockSize = *(uint16_t*)(buff + 0x0b);
  fatInformation.blocksPerCluster = *(uint8_t*)(buff + 0x0d);
  fatInformation.prologBlocks = *(uint16_t*)(buff + 0x0e);
  fatInformation.numFATs = *(buff + 0x10);
  fatInformation.blocksPerFAT = *(uint32_t*)(buff + 0x24);
  fatInformation.version = *(uint16_t*)(buff + 0x2a);
  fatInformation.rootCluster = *(uint32_t*)(buff + 0x2c);

  //readVolumeBlock(&currVolume, buff, fatInformation.prologBlocks + fatStart);
  //dumpHex(buff,512);
  printf("ADDR %d \n", fatInformation.prologBlocks + (uint32_t)fatInformation.partitionStart +
     fatInformation.blocksPerFAT*fatInformation.numFATs + fatInformation.rootCluster);
  readVolumeBlock(&currVolume, buff, fatInformation.prologBlocks + (uint32_t)fatInformation.partitionStart +
     fatInformation.blocksPerFAT*fatInformation.numFATs + fatInformation.rootCluster);
  dumpHex(buff,512);
}

void initFat32Driver(void){
  setAbstractionPtrsSDCard(&currVolume);

  uint64_t fatAddress = locateFAT();
  if(fatAddress == 0){
    printf("ERROR: could not locate FAT partition!\n");
    return;
  }

  populateFATInformation(fatAddress);
}


void printFATInfo(void){
  printf("##### FAT INFO #####\n");
  printf("Partition start: \t%d\n", (uint32_t)fatInformation.partitionStart);
  printf("OEM: \t\t\t\"%.8s\"\n", fatInformation.oemName);
  printf("Version: \t\t%d\n", fatInformation.version);
  printf("Block Size: \t\t%d\n", fatInformation.blockSize);
  printf("Blocks per cluster: \t%d\n", fatInformation.blocksPerCluster);
  printf("Blocks per FAT: \t%d\n", fatInformation.blocksPerFAT);
  printf("Prolog blocks: \t\t%d\n", fatInformation.prologBlocks);
  printf("Num FATS: \t\t%d \n", fatInformation.numFATs);
  printf("Root Cluster: \t\t%d\n", fatInformation.rootCluster);
  printf("#### FAT INFO #####\n");
}

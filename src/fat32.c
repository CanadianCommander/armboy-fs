#include "fat32.h"
#include "volume.h"
#include <util/debug.h>
#include <memory.h>

static Volume currVolume;

static FATInfo fatInformation;


#define IS_GPT_PART(buff) (buff[0] == 0x45 && buff[1] == 0x46 && buff[2] == 0x49 && buff[3] == 0x20 && buff[4] == 0x50 \
  && buff[5] == 0x41 && buff[6] == 0x52 && buff[7] == 0x54)

#define IS_FAT_PART_ENTRY(buff) (*(uint64_t*)(buff) == 0x4433b9e5ebd0a0a2 && \
  *(uint64_t*)(buff + 0x8) == 0xc79926b7b668c087)


static uint64_t locateFAT(){
  uint8_t buff[getVolumeBlockSize(&currVolume)];

  readVolumeBlock(&currVolume, buff, 1);

  if(IS_GPT_PART(buff)){
    uint64_t partArrayStart = *(uint64_t*)(buff + 0x48);
    uint32_t partArrayLen = *(uint32_t*)(buff + 0x50);

    for(uint32_t i = 0; i < partArrayLen; i++){
      if(i % 4 == 0){
        readVolumeBlock(&currVolume, buff, (i/4) + partArrayStart);
      }

      if(IS_FAT_PART_ENTRY((buff + 128*(i%4)))){
        return *(uint64_t*)(buff + 128*(i%4) + 0x20);
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

  readVolumeBlock(&currVolume, buff, fatInformation.prologBlocks + fatInformation.partitionStart +
     fatInformation.blocksPerFAT*fatInformation.numFATs +
     fatInformation.rootCluster*fatInformation.blocksPerCluster);
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

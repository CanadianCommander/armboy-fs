#include "fat32.h"
#include "volume.h"
#include <util/debug.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

static Volume currVolume;

static FATInfo fatInformation;

static uint32_t __populateFD(ClusterIter * ci, uint16_t offset, FileDescriptor * fd);
static uint32_t __decodeVFAT(ClusterIter * ci, uint16_t offset, char * fName);

static void extractVFATName(wchar_t * dest, uint8_t * src);

#define IS_GPT_PART(buff) (buff[0] == 0x45 && buff[1] == 0x46 && buff[2] == 0x49 && buff[3] == 0x20 && buff[4] == 0x50 \
  && buff[5] == 0x41 && buff[6] == 0x52 && buff[7] == 0x54)

#define IS_MBR_PART(buff) (buff[510] == 0x55 && buff[511] == 0xaa)

#define IS_MBR_PART_VALID(ptr) ((*ptr & 0x7F) == 0x00)

#define IS_MBR_PART_FAT(ptr)(*(ptr + 0x4) & 0xfe)

#define IS_FAT_PART_ENTRY(buff) (*(uint64_t*)(buff) == 0x4433b9e5ebd0a0a2 && \
  *(uint64_t*)(buff + 0x8) == 0xc79926b7b668c087)

#define IS_VFAT_DIR_ENTRY(ptr) (*(ptr + 0xB) == 0x0F)

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

    return 0;
  }
  else {
    //attempt to read MBR (even GTP has backup MBR at 0x00)
    readVolumeBlock(&currVolume, buff, 0);

    if (IS_MBR_PART(buff)){
      //four 16 byte partition entries (NO EBR support... come on if you want more than four just use GPT)
      for(uint32_t i = 0x1BE; i < 0x1FE; i += 0x10){
        if(IS_MBR_PART_VALID((buff + i)) && IS_MBR_PART_FAT((buff + i))){
          return (uint64_t)(*(uint32_t*)(buff + i + 0x8));
        }
      }
      return 0;
    }
    else {
      return 0;
    }
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
#ifndef FORCE_ROOT_CLUSTER_ZERO
  fatInformation.rootCluster = *(uint32_t*)(buff + 0x2c);
#else
  fatInformation.rootCluster = 0;
#endif
}

void initFat32Driver(void){
  setAbstractionPtrsSDCard(&currVolume);

  uint64_t fatAddress = locateFAT();
  if(fatAddress == 0){
    printf("ERROR: could not locate FAT partition!\n");
    return;
  }

  populateFATInformation(fatAddress);

  ClusterIter ci;
  FileDescriptor fd;

  newClusterIterator(fatInformation.rootCluster, &ci);
  __populateFD(&ci, 32, &fd);

  dumpHex(ci.blockData, 512);

  printf("NAME: \"%s\" Cluster: %d\n", fd.fileName, fd.startCluster);
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

void readCluster(uint32_t cluster, uint16_t block, uint8_t * buffer){
  uint32_t bAddr = fatInformation.prologBlocks + fatInformation.partitionStart +
                   fatInformation.blocksPerFAT*fatInformation.numFATs +
                   cluster*fatInformation.blocksPerCluster + block;
  readVolumeBlock(&currVolume, buffer, bAddr);
}

uint32_t fetchNextCluster(uint32_t cluster){
  uint32_t vblock = getVolumeBlockSize(&currVolume);
  uint8_t buffer[vblock];
  uint32_t fatAddress = fatInformation.prologBlocks + fatInformation.partitionStart + (cluster / vblock);

  readVolumeBlock(&currVolume, buffer, fatAddress);

  uint32_t nextCluster = *(buffer + (cluster % vblock));
  if(nextCluster < 0x0fffffff){
    return nextCluster;
  }
  else{
    return 0;
  }
}

bool getFirstFileInDirectory(char * path, FileDescriptor * fd){

}

bool getFirstFileInDirectoryCluster(uint32_t cluster, FileDescriptor * fd){

}

bool getNextFileInDirectory(FileDescriptor * fd){

}

static uint32_t __getDirectory(char * path);
void getDirectory(char * path, FileDescriptor * fd){

}

static uint32_t __populateFD(ClusterIter * ci, uint16_t offset, FileDescriptor * fd){
  if(IS_VFAT_DIR_ENTRY((ci->blockData + offset)) && *(ci->blockData + offset) & 0x40 ){
    //first entry in a VFAT chain
    uint32_t realEntryOffset = __decodeVFAT(ci, offset, fd->fileName);
    return __populateFD(ci, realEntryOffset, fd);
  }
  else if (!IS_VFAT_DIR_ENTRY((ci->blockData + offset))){
    //normal entry (name should have been populated from VFAT)
    uint8_t * entryPtr = ci->blockData + offset;
    fd->containingCluster = ci->currCluster;
    fd->directoryEntryOffset = offset;
    fd->startCluster =  ((uint32_t)*(entryPtr + 0x15)<<24) | ((uint32_t)*(entryPtr + 0x14)<<16) |
                        ((uint32_t)*(entryPtr + 0x1B)<<8) | (uint32_t)*(entryPtr + 0x1A);
    if(*(entryPtr + 0xB) & 0x10){
      fd->fileType = FILE_TYPE_DIRECTORY;
    }
    else{
      fd->fileType = FILE_TYPE_BINARY;
    }
  }
  return offset;
}

static uint32_t __decodeVFAT(ClusterIter * ci, uint16_t offset, char * fName){
  uint8_t entries[(FAT32_MAX_NAME_LEN/8)*FAT32_DIR_ENTRY_SIZE];
  memset(entries, 0 , (FAT32_MAX_NAME_LEN/8)*FAT32_DIR_ENTRY_SIZE);
  uint32_t countEntries = 0;

  for(int i =offset; countEntries < (FAT32_MAX_NAME_LEN/8); i+=FAT32_DIR_ENTRY_SIZE){
    if(IS_VFAT_DIR_ENTRY((ci->blockData + i))){
      uint8_t * vFatPtr = ci->blockData + i;
      wchar_t wName[14];
      extractVFATName(wName, vFatPtr);
      dumpHex((uint8_t*)wName,14*4);
      wcstombs(entries + countEntries*FAT32_DIR_ENTRY_SIZE, wName, FAT32_DIR_ENTRY_SIZE-1);
      printf("PART: %s \n", (char*)(entries + countEntries*FAT32_DIR_ENTRY_SIZE));
      countEntries ++;
    }
    else {
      //end of chain.
      memset(fName, 0, FAT32_MAX_NAME_LEN);
      for(int z = countEntries -1; z >= 0; z--){
        strcat(fName, entries + z*FAT32_DIR_ENTRY_SIZE);
      }
      return i;
    }

    if(i + FAT32_DIR_ENTRY_SIZE >= getVolumeBlockSize(&currVolume)){
      getNextBlockClusterIterator(ci);
      i = 0;

      if(ci->eof){
        break;
      }
    }
  }

  return 0;
}

static uint32_t __getDirectory(char * path){
  char subPath[strlen(path)];
  memset(subPath, 0, strlen(path));
  char * slashPtr = strrchr(path, '/');

  if((uint32_t)slashPtr - (uint32_t)path > 0){
    //there are directories bellow this one
    memcpy(subPath, path, (uint32_t)slashPtr - (uint32_t)path);
    uint32_t parentDir = __getDirectory(subPath);

    if(parentDir > 0){
      //look for dir
      memset(subPath, 0, strlen(path));
      strcpy(subPath, slashPtr);

      FileDescriptor fd;
      getFirstFileInDirectoryCluster(parentDir, &fd);

      for(;;){
        if(strcmp(fd.fileName,subPath) == 0 && fd.fileType == FILE_TYPE_DIRECTORY){
          return fd.startCluster;
        }
        if(!getNextFileInDirectory(&fd)){
          return 0;
        }
      }
    }
    else {
      //error
      return 0;
    }
  }
  else {
    //we must be in root directory
    return fatInformation.rootCluster;
  }
}


void newClusterIterator(uint32_t clusterStart, ClusterIter * ci){
  ci->eof = false;
  ci->currCluster = clusterStart;
  ci->currBlock = 0;
  ci->blockData = malloc(getVolumeBlockSize(&currVolume));

  readCluster(ci->currCluster, ci->currBlock, ci->blockData);
}

void freeClusterIterator(ClusterIter * ci){
  free(ci->blockData);
}

void getNextBlockClusterIterator(ClusterIter * ci){
  if(ci->eof){
    return;
  }

  if(ci->currBlock + 1 > fatInformation.blocksPerCluster - 1){
    //fetch next cluster
    ci->currCluster = fetchNextCluster(ci->currCluster);
    ci->currBlock = 0;
    if(ci->currCluster == 0){
      ci->eof = true;
    }
  }
  else {
    ci->currBlock ++;
    readCluster(ci->currCluster, ci->currBlock, ci->blockData);
  }
}

static void extractVFATName(wchar_t * dest, uint8_t * src){
  //NOTE wchar_t is 32 bit!!!!!! (It is 16 on disk)
  uint8_t * p = src + 0x1;
  for(uint16_t i = 0; i < 10; i+=2){
    *dest++ = (uint32_t)((*(p + i)) | (*(p + i + 1) << 8));
  }

  p = src + 0x0E;
  for(uint16_t i = 0; i < 12; i+=2){
    *dest++ = (uint32_t)((*(p + i)) | (*(p + i + 1) << 8));
  }

  p = src + 0x1C;
  for(uint16_t i = 0; i < 4; i+=2){
    *dest++ = (uint32_t)((*(p + i)) | (*(p + i + 1) << 8));
  }

  *dest = 0x00;
}

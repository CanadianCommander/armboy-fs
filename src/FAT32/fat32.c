#include "fat32.h"
#include "path.h"
#include "partition.h"
#include "../volume.h"
#include <util/debug.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "fileOps.h"

static Volume currVolume;

static FATInfo fatInformation;

static bool __populateFD(ClusterIter * ci, uint16_t offset, FileDescriptor * fd, bool vfatBefore);
static uint32_t __decodeVFAT(ClusterIter * ci, uint16_t offset, char * fName);
static FileDescriptor __getDirectory(char * path);

/***** handy directory entry macros ******/
#define IS_VFAT_DIR_ENTRY(ptr) (*(ptr + 0xB) == 0x0F)
#define IS_VALID_DIR_ENTRY(ptr) (*ptr != 0x0)
#define IS_DELETE_DIR_ENTRY(ptr) (*ptr == 0xe5)
#define IS_VOLUME_DIR_ENTRY(ptr) (*(ptr + 0xB) & 0x08)
#define IS_DIR_DIR_ENTRY(ptr) (*(ptr + 0xB) & 0x10)
/*###### handy directory entry macros ######*/

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
}

void initFat32Driver(void){
  setAbstractionPtrsSDCard(&currVolume);

  uint64_t fatAddress = locateFAT(&currVolume);
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

void readCluster(uint32_t cluster, uint16_t block, uint8_t * buffer){
  uint32_t bAddr = fatInformation.prologBlocks + fatInformation.partitionStart +
                   fatInformation.blocksPerFAT*fatInformation.numFATs +
                   (cluster - 2)*fatInformation.blocksPerCluster + block;// -2 because sector numbers start at 2

  readVolumeBlock(&currVolume, buffer, bAddr);
}

uint32_t fetchNextCluster(uint32_t cluster){
  uint32_t vblock = getVolumeBlockSize(&currVolume);
  uint8_t buffer[vblock];
  uint32_t fatAddress = fatInformation.prologBlocks + fatInformation.partitionStart + ((cluster*4) / vblock);

  readVolumeBlock(&currVolume, buffer, fatAddress);

  uint32_t nextCluster = *(uint32_t*)(buffer + ((cluster*4) % vblock));
  if(nextCluster < 0x0fffffff){
    return nextCluster;
  }
  else{
    return 0;
  }
}

bool getFirstFileInDirectory(char * path, FileDescriptor * fd){
  FileDescriptor dir;
  getDirectory(path, &dir);
  return getFirstFileInDirectoryCluster(dir.startCluster, fd);
}

bool getFirstFileInDirectoryCluster(uint32_t cluster, FileDescriptor * fd){
  ClusterIter ci;
  newClusterIterator(cluster, &ci);
  bool res = __populateFD(&ci, 0, fd, false);
  freeClusterIterator(&ci);
  return res;
}

bool getNextFileInDirectory(FileDescriptor * fd){
  ClusterIter ci;
  newClusterIterator(fd->containingCluster, &ci);
  uint32_t blockOffset = seekByteClusterIterator(&ci, fd->directoryEntryOffset + FAT32_DIR_ENTRY_SIZE);

  bool res = __populateFD(&ci, blockOffset, fd, false);
  freeClusterIterator(&ci);
  return res;
}

void getDirectory(char * path, FileDescriptor * fd){
  char pathCpy[strlen(path)];
  strcpy(pathCpy, path);

  stripPath(pathCpy);
  FileDescriptor dir = __getDirectory(pathCpy);
  if(dir.fileType != FILE_TYPE_INVALID){
    char * slashPtr = strrchr(pathCpy, '/');
    *fd = dir;
    memset(fd->fileName, 0, FAT32_MAX_NAME_LEN);
    strcpy(fd->fileName, slashPtr);
  }
}

static bool __populateFD(ClusterIter * ci, uint16_t offset, FileDescriptor * fd, bool vfatBefore){
  if(IS_VFAT_DIR_ENTRY((ci->blockData + offset)) && *(ci->blockData + offset) & 0x40 ){
    //first entry in a VFAT chain
    uint32_t realEntryOffset = __decodeVFAT(ci, offset, fd->fileName);
    stripAppleBullShit(fd->fileName);
    return __populateFD(ci, realEntryOffset, fd, true);
  }
  else if (IS_VALID_DIR_ENTRY((ci->blockData + offset)) && !IS_VFAT_DIR_ENTRY((ci->blockData + offset))){
    if(!vfatBefore){
      //use short name
      memset(fd->fileName, 0 , FAT32_MAX_NAME_LEN);
      memcpy(fd->fileName, ci->blockData + offset, 8);
      stripAppleBullShit(fd->fileName);
    }
    else {
      // file name is already filled out from VFAT
    }

    uint8_t * entryPtr = ci->blockData + offset;

    fd->containingCluster = ci->currCluster;
    fd->directoryEntryOffset = offset;
    fd->startCluster =  ((uint32_t)*(entryPtr + 0x15)<<24) | ((uint32_t)*(entryPtr + 0x14)<<16) |
                        ((uint32_t)*(entryPtr + 0x1B)<<8) | (uint32_t)*(entryPtr + 0x1A);
    fd->fileSize = *(uint32_t*)(entryPtr + 0x1C);
    fd->filePosition = 0;

    if(IS_DELETE_DIR_ENTRY(entryPtr)){
      fd->fileType = FILE_TYPE_INVALID;
    }
    else if(IS_DIR_DIR_ENTRY(entryPtr)){
      fd->fileType = FILE_TYPE_DIRECTORY;
    }
    else if (IS_VOLUME_DIR_ENTRY(entryPtr)){
      fd->fileType = FILE_TYPE_VOLUME;
    }
    else{
      fd->fileType = FILE_TYPE_BINARY;
    }
    return true;
  }
  return false;
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
      wcstombs(entries + countEntries*FAT32_DIR_ENTRY_SIZE, wName, FAT32_DIR_ENTRY_SIZE-1);
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

static FileDescriptor __getDirectory(char * path){
  char subPath[strlen(path)];
  memset(subPath, 0, strlen(path));
  char * slashPtr = strrchr(path, '/');

  if(slashPtr && *(uint8_t*)(slashPtr + 1) != 0x00){
    //there are directories bellow this one
    memcpy(subPath, path, ((uint32_t)slashPtr) - (uint32_t)path);
    FileDescriptor parentDir = __getDirectory(subPath);

    if(parentDir.fileType != FILE_TYPE_INVALID){
      //look for dir
      memset(subPath, 0, strlen(path));
      strcpy(subPath, slashPtr + 1);

      FileDescriptor fd;
      getFirstFileInDirectoryCluster(parentDir.startCluster, &fd);

      for(;;){
        if(strcmp(fd.fileName, subPath) == 0 && fd.fileType == FILE_TYPE_DIRECTORY){
          return fd;
        }
        if(!getNextFileInDirectory(&fd)){
          FileDescriptor bad;
          bad.fileType = FILE_TYPE_INVALID;
          return bad;
        }
      }
    }
    else {
      //error
      FileDescriptor bad;
      bad.fileType = FILE_TYPE_INVALID;
      return bad;
    }
  }
  else {
    //we must be in root directory
    FileDescriptor rfd;
    rfd.startCluster = fatInformation.rootCluster;
    rfd.fileType = FILE_TYPE_DIRECTORY;
    return rfd;
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

  if(ci->currBlock + 1 >= fatInformation.blocksPerCluster){
    //fetch next cluster
    ci->currCluster = fetchNextCluster(ci->currCluster);
    ci->currBlock = 0;
    if(ci->currCluster == 0){
      ci->eof = true;
    }
    else {
      readCluster(ci->currCluster, ci->currBlock, ci->blockData);
    }
  }
  else {
    ci->currBlock ++;
    readCluster(ci->currCluster, ci->currBlock, ci->blockData);
  }
}

void seekBlockClusterIterator(ClusterIter * ci, uint32_t blockOffset){

  if(ci->currBlock + blockOffset + 1 > fatInformation.blocksPerCluster){
    ci->currCluster = fetchNextCluster(ci->currCluster);
    blockOffset -= (fatInformation.blocksPerCluster - ci->currBlock);
    ci->currBlock = 0;
    seekBlockClusterIterator(ci, blockOffset);
  }
  else {
    ci->currBlock += blockOffset;
    readCluster(ci->currCluster, ci->currBlock, ci->blockData);
  }
}

uint32_t seekByteClusterIterator(ClusterIter * ci, uint32_t byteOffset){
  uint32_t blocks = (byteOffset / fatInformation.blockSize);
  uint32_t bOffset = (byteOffset % fatInformation.blockSize);

  seekBlockClusterIterator(ci, blocks);

  ci->seekOffset = bOffset;
  return bOffset;
}

uint32_t readClusterIterator(ClusterIter * ci, uint8_t * buff, uint32_t offset, uint32_t count){
  if(offset + count > fatInformation.blockSize){
    uint32_t readBytes = (fatInformation.blockSize - offset);
    memcpy(buff, ci->blockData + offset, readBytes);
    return readBytes;
  }
  else {
    memcpy(buff, ci->blockData + offset, count);
    return count;
  }
}

uint32_t readClusterIteratorSeekOffset(ClusterIter * ci, uint8_t * buff, uint32_t count){
  uint32_t read =  readClusterIterator(ci, buff, ci->seekOffset, count);
  if(ci->seekOffset + read >= fatInformation.blockSize){
    ci->seekOffset = 0;
    getNextBlockClusterIterator(ci);
  }
  else {
    ci->seekOffset += read;
  }
  return read;
}

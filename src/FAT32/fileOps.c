#include "fileOps.h"
#include "path.h"
#include <string.h>

bool getFile(char * path, FileDescriptor * fd){
  char dirPath[strlen(path)];
  char fileName[strlen(path)];
  FileDescriptor currFd;

  strcpy(dirPath, path);
  strcpy(fileName, path);
  stripFileName(dirPath);
  getFileName(fileName);

  bool eod = getFirstFileInDirectory(dirPath, &currFd);
  while(eod){
    if(strcmp(currFd.fileName, fileName) == 0){
      copyFd(fd, &currFd);
      return true;
    }
    eod = getNextFileInDirectory(&currFd);
  }

  return false;
}

uint32_t readBytes(uint8_t * buffer, uint32_t count, FileDescriptor * fd){
  ClusterIter ci;
  newClusterIterator(fd->startCluster, &ci);
  uint32_t blockOff = seekByteClusterIterator(&ci, fd->filePosition);
  uint32_t readBytes = 0;

  if(fd->filePosition + count >= fd->fileSize){
    count = fd->fileSize - fd->filePosition;
  }

  while(readBytes < count){
    readBytes += readClusterIterator(&ci, buffer+readBytes, blockOff, count);
    count = count - readBytes;
    blockOff = 0;

    if(fd->filePosition + readBytes >= fd->fileSize){
      break;
    }
  }


  freeClusterIterator(&ci);
  return readBytes;
}

void seek(int offset, uint8_t ref, FileDescriptor * fd){
  switch(ref){
    case SEEK_CURR:
      fd->filePosition + offset;
      break;
    case SEEK_START:
      fd->filePosition = offset;
      break;
    case SEEK_END:
      fd->filePosition = fd->fileSize + offset;
      break;
    default:
      break;
  }
}

static bool __listDir(char * path, FileDescriptor * last, LsDir * lsDir);
bool listDir(char * path, LsDir * lsDir){
  if(lsDir->isInit == LIST_DIR_INIT_CODE){
    return __listDir(path, &lsDir->files[LIST_DIR_FILE_COUNT-1], lsDir);
  }
  else {
    lsDir->isInit = 0xFE;
    return __listDir(path, 0x00, lsDir);
  }
}

static bool __listDir(char * path, FileDescriptor * last, LsDir * lsDir){
  uint32_t i = 0;
  lsDir->countFiles = 0;

  if(!last) {
    if(!getFirstFileInDirectory(path,&lsDir->files[0])){
      return false;
    }
    last = &lsDir->files[0];
    lsDir->countFiles++;
    i++;
  }

  copyFd(&lsDir->files[i], last);
  for(i; i < LIST_DIR_FILE_COUNT; i++){
    if(i - 1 >= 0){
      copyFd(&lsDir->files[i], &lsDir->files[i-1]);
    }
    if(getNextFileInDirectory(&lsDir->files[i])){
      lsDir->countFiles++;
    }
    else {
      return false;
    }
  }

  return true;
}


void copyFd(FileDescriptor * copy, FileDescriptor * src){
  memcpy(copy->fileName, src->fileName, FAT32_MAX_NAME_LEN);
  copy->fileType              = src->fileType;
  copy->containingCluster     = src->containingCluster;
  copy->directoryEntryOffset  = src->directoryEntryOffset;
  copy->startCluster          = src->startCluster;
  copy->fileSize              = src->fileSize;
  copy->filePosition          = src->filePosition;
}

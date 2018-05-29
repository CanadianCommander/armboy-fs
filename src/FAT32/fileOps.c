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
  seekByteClusterIterator(&ci, fd->filePosition);
  uint32_t readBytes = 0;
  uint32_t bytesLeft = count;

  if(fd->filePosition + count >= fd->fileSize){
    count = fd->fileSize - fd->filePosition;
  }

  while(readBytes < count){
    uint32_t read = readClusterIteratorSeekOffset(&ci, buffer+readBytes, bytesLeft);
    bytesLeft -= read;
    readBytes += read;

    if(fd->filePosition + readBytes >= fd->fileSize){
      readBytes = fd->fileSize - fd->filePosition;
      break;
    }
  }

  fd->filePosition += readBytes;


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

static bool __listDir(char * path, int offset, LsDir * lsDir);
bool listDir(char * path, LsDir * lsDir){
  if(lsDir->isInit == LIST_DIR_INIT_CODE){
    copyFd(&lsDir->files[0], &lsDir->files[lsDir->countFiles-1]);
    lsDir->more = __listDir(path, 0, lsDir);
    return lsDir->more;
  }
  else {
    lsDir->isInit = LIST_DIR_INIT_CODE;
    getFirstFileInDirectory(path, &lsDir->files[0]);
    lsDir->more = __listDir(path, 1, lsDir);
    lsDir->countFiles++;
    return lsDir->more;
  }
}

static bool __listDir(char * path, int offset, LsDir * lsDir){
  lsDir->countFiles = 0;
  for(int i = offset; i < LIST_DIR_FILE_COUNT; i++){
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

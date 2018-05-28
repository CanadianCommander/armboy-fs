#ifndef FS_SYSCALLS_H_
#define FS_SYSCALLS_H_
#include <stdint.h>
#include <stdbool.h>
#include "fileOps.h"

typedef struct {
  LsDir * lsDir;
  char * path;
  bool res;
} LsDirArgs;
void listDirCall(void * lsDirArg);


typedef struct {
  FileDescriptor * fd;
  char * path;
  bool res;
} GetFileArgs;
void getFileCall(void * fileArg);

typedef struct {
  FileDescriptor * fd;
  uint32_t count;
  uint32_t res;
  uint8_t * buffer;
} ReadBytesArgs;
void readBytesCall(void * readArgs);

typedef struct {
  FileDescriptor * fd;
  uint32_t offset;
  uint8_t reference;
} SeekArgs;
void seekCall(void * seekArgs);

#endif /*FS_SYSCALLS_H_*/

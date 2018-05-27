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


#endif /*FS_SYSCALLS_H_*/

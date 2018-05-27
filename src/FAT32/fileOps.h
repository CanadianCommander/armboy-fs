#ifndef FILE_OPS_H_
#define FILE_OPS_H_

#include <stdbool.h>
#include <stdint.h>
#include "fat32.h"

/**
  get file descriptor of the file denoted by path.
  returns false if file descriptor cannot be retrieved
*/
bool getFile(char * path, FileDescriptor * fd);

// read next count bytes in to buffer.
// @return the real number of bytes written to buffer is returned
uint32_t readBytes(uint8_t * buffer, uint32_t count, FileDescriptor * fd);

#define SEEK_CURR   0
#define SEEK_START  1
#define SEEK_END    2
// move the file read pointer by offset bytes relative to ref
void seek(int offset, uint8_t ref, FileDescriptor * fd);

#define LIST_DIR_FILE_COUNT 4
#define LIST_DIR_INIT_CODE 0xFE
typedef struct {
  FileDescriptor files[LIST_DIR_FILE_COUNT];
  uint16_t countFiles;
  bool more;
  uint8_t isInit; // used to check if structure has been initialized by a previous call to listDir
} LsDir;

/**
  get the first LIST_DIR_FILE_COUNT files from the directory indicated by path.
  if there are more files, this function will return true. also the more field in LsDir will be set
*/
bool listDir(char * path, LsDir * lsDir);

void copyFd(FileDescriptor * copy, FileDescriptor * src);

#endif /*FILE_OPS_H_*/

#ifndef FAT_32_H_
#define FAT_32_H_
#include <stdio.h>

typedef struct {
  char      oemName[8];
  uint16_t  blockSize;
  uint8_t   blocksPerCluster;
  uint32_t  blocksPerFAT;
  uint16_t  version;
  uint16_t  prologBlocks;
  uint8_t   numFATs;
  uint32_t  rootCluster;
  uint64_t  partitionStart;
} FATInfo;


/**
  set up data structures and what not. DOES NOT AFFECT DISK
*/
void initFat32Driver(void);


void printFATInfo(void);
#endif /*FAT_32_H_*/

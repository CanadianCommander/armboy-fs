#ifndef FAT_32_H_
#define FAT_32_H_
#include <stdio.h>
#include <stdbool.h>

#define FAT32_MAX_NAME_LEN 128
#define FAT32_DIR_ENTRY_SIZE 32

/*** FAT32 file types ***/
#define FILE_TYPE_DIRECTORY 0
#define FILE_TYPE_BINARY    1
#define FILE_TYPE_VFAT      2

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
  I cannot find any information on this any where, but as far as
  I can tell the root cluster field in the FAT "header" is (some what)
  incorrect. It indicates root cluster as cluster 2. this cluster does contain
  a directory but it does not contain files placed in the root. rather it contains
  a sqlLite 3 database!!!! I suspect this is some Apple Bull Shit!

  remove def to use root cluster value in FAT header
*/
#define FORCE_ROOT_CLUSTER_ZERO 

typedef struct {
  char fileName[FAT32_MAX_NAME_LEN];
  // binary or directory
  uint8_t fileType;

  // first cluster of the directory that contains this file
  uint32_t containingCluster;
  // the offset of this entry from the start of the containing directory
  // i.e. firstCluster + directoryEntryOffset*32 (because 32 bytes an entry)
  uint32_t directoryEntryOffset;
  // start cluster of this file or directory
  uint32_t startCluster;

} FileDescriptor;


/**
  set up data structures and what not. DOES NOT AFFECT DISK
*/
void initFat32Driver(void);


void printFATInfo(void);

void readCluster(uint32_t cluster, uint16_t block, uint8_t * buffer);

/**
  fetch the next cluster in the FAT given the current cluster.
  or zero if EOF
*/
uint32_t fetchNextCluster(uint32_t cluster);

/**
  fill fd with information about the first file in the
  directory denoted by path.
  @return false if there are no files in directory (impossible because of . and .. )
*/
bool getFirstFileInDirectory(char * path, FileDescriptor * fd);
/**
  like getFirstFileInDirectory but uses a cluster index instead of a path to specify
  the directory of interest
*/
bool getFirstFileInDirectoryCluster(uint32_t cluster, FileDescriptor * fd);

/**
  overwrite fd with the next file in the directory
  @return false if there is no file after the one denoted by fd
*/
bool getNextFileInDirectory(FileDescriptor * fd);

/**
  get file descriptor for the directory indicated by path
*/
void getDirectory(char * path, FileDescriptor * fd);

typedef struct {
  uint32_t currCluster;
  uint8_t currBlock;
  bool eof;
  uint8_t * blockData;
} ClusterIter;

// create a cluster iterator and fetch the first block
void newClusterIterator(uint32_t clusterStart, ClusterIter * ci);
void freeClusterIterator(ClusterIter * ci);

// get the next block in the given cluster iterator
void getNextBlockClusterIterator(ClusterIter * ci);

#endif /*FAT_32_H_*/

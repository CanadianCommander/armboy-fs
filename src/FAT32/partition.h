#ifndef PARTITION_H_
#define PARTITION_H_
#include <stdint.h>
#include "../volume.h"

/** partition data structure checking macros **/
#define IS_GPT_PART(ptr) (ptr[0] == 0x45 && ptr[1] == 0x46 && ptr[2] == 0x49 && ptr[3] == 0x20 && ptr[4] == 0x50 \
  && ptr[5] == 0x41 && ptr[6] == 0x52 && ptr[7] == 0x54)
#define IS_MBR_PART(ptr) (ptr[510] == 0x55 && ptr[511] == 0xaa)
#define IS_MBR_PART_VALID(ptr) ((*ptr & 0x7F) == 0x00)
#define IS_MBR_PART_FAT(ptr)(*(ptr + 0x4) & 0xfe)
#define IS_FAT_PART_ENTRY(buff) (*(uint64_t*)(buff) == 0x4433b9e5ebd0a0a2 && \
  *(uint64_t*)(buff + 0x8) == 0xc79926b7b668c087)
/*#### partition macros ####*/


/** return the block offset of the first found FAT partition or zero on failure**/
uint64_t locateFAT(Volume * vol);

#endif /* PARTITION_H_ */

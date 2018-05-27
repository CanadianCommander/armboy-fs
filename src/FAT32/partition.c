#include "partition.h"

uint64_t locateFAT(Volume * vol){
  uint8_t buff[getVolumeBlockSize(vol)];

  readVolumeBlock(vol, buff, 1);

  if(IS_GPT_PART(buff)){
    uint64_t partArrayStart = *(uint64_t*)(buff + 0x48);
    uint32_t partArrayLen = *(uint32_t*)(buff + 0x50);

    for(uint32_t i = 0; i < partArrayLen; i++){
      if(i % 4 == 0){
        readVolumeBlock(vol, buff, (i/4) + partArrayStart);
      }

      if(IS_FAT_PART_ENTRY((buff + 128*(i%4)))){
        return *(uint64_t*)(buff + 128*(i%4) + 0x20);
      }
    }

    return 0;
  }
  else {
    //attempt to read MBR (even GTP has backup MBR at 0x00)
    readVolumeBlock(vol, buff, 0);

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

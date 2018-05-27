#include "path.h"
#include <string.h>

void stripAppleBullShit(char * fName){
  uint32_t len = strlen(fName);
  for(uint32_t i = 0; i < len; i++){
    if(fName[i] == 0x20){
      fName[i] = 0x0;
    }
  }
}

void stripPath(char * path){
  char * spacePtr = strchr(path, ' ');
  if(spacePtr){
    *spacePtr = 0x0;
  }
  else {
    spacePtr = (path + strlen(path) -1);
  }

  if(*(spacePtr - 1) == '/'){
    *(spacePtr - 1) = 0x0;
  }
}

void stripFileName(char * path){
  char * slashPtr = strrchr(path, '/');
  if(slashPtr){
    *slashPtr = 0x0;
  }
}

void getFileName(char * path){
  char * slashPtr = strrchr(path, '/');
  if(slashPtr){
    strcpy(path, slashPtr +1);
  }

}


void extractVFATName(wchar_t * dest, uint8_t * src){
  //NOTE wchar_t is 32 bit!!!!!! (It is 16 on disk)
  uint8_t * p = src + 0x1;
  for(uint16_t i = 0; i < 10; i+=2){
    *dest++ = (uint32_t)((*(p + i)) | (*(p + i + 1) << 8));
  }

  p = src + 0x0E;
  for(uint16_t i = 0; i < 12; i+=2){
    *dest++ = (uint32_t)((*(p + i)) | (*(p + i + 1) << 8));
  }

  p = src + 0x1C;
  for(uint16_t i = 0; i < 4; i+=2){
    *dest++ = (uint32_t)((*(p + i)) | (*(p + i + 1) << 8));
  }

  *dest = 0x00;
}

#include "fsSysCalls.h"

void listDirCall(void * lsDirArg){
  ((LsDirArgs*)lsDirArg)->res = listDir(((LsDirArgs*)lsDirArg)->path, ((LsDirArgs*)lsDirArg)->lsDir);
}

void getFileCall(void * fileArg){
  ((GetFileArgs*)fileArg)->res = getFile(((GetFileArgs*)fileArg)->path, ((GetFileArgs*)fileArg)->fd);
}

void readBytesCall(void * readArgs){
  ((ReadBytesArgs*)readArgs)->res = readBytes(((ReadBytesArgs*)readArgs)->buffer,
                                              ((ReadBytesArgs*)readArgs)->count, ((ReadBytesArgs*)readArgs)->fd);
}

void seekCall(void * seekArgs){
  seek(((SeekArgs*)seekArgs)->offset, ((SeekArgs*)seekArgs)->reference, ((SeekArgs*)seekArgs)->fd);
}

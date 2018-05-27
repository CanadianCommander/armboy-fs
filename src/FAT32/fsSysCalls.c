#include "fsSysCalls.h"

void listDirCall(void * lsDirArg){
  ((LsDirArgs*)lsDirArg)->res = listDir(((LsDirArgs*)lsDirArg)->path, ((LsDirArgs*)lsDirArg)->lsDir);
}

void getFileCall(void * fileArg){
  ((GetFileArgs*)fileArg)->res = getFile(((GetFileArgs*)fileArg)->path, ((GetFileArgs*)fileArg)->fd);
}

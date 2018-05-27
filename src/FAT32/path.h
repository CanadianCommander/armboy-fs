#ifndef PATH_H_
#define PATH_H_
#include <wchar.h>
#include <stdint.h>

/** strip 0x20 characters from file names. replace with 0x00 */
void stripAppleBullShit(char * fName);

/** remove tailing white space and "/"s **/
void stripPath(char * path);

/** extract wchar_t file name from directory entry */
void extractVFATName(wchar_t * dest, uint8_t * src);



#endif /* PATH_H_ */

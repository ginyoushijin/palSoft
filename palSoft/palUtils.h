#ifndef PAL_UTILS_H
#define PAL_UTILS_H

#include <list>
#include <io.h>
#include <string>


#include "palDefine.h"

#define RORBYTE(val,bits) (val>>(bits%8))|(val<<(8-(bits%8))) 

#define ROLBYTE(val,bits) (val<<(bits%8))|(val>>(8-(bits%8)))


std::list<std::string> getDirFiles(char const* source, char const* filter);

bool tryDataFileOpen(char* fileFullPath, char* nameSectionPointer, FILE** scriptSrc, FILE** textDat);

bool dataBufferTransform(uint8_t* buffer, size_t bufferSize, bool decryptSwitch);

bool dataFileTransform(char const* target, bool decryptSwitch);

#endif // ! PAL_UTILS_H


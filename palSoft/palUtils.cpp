#define _CRT_SECURE_NO_WARNINGS

#pragma warning(disable:6387 6031 6054)

#include "palUtils.h"

/**
 * @brief 查找指定路径一级目录下的文件
 * @param source 源目录
 * @param filter 查找条件
 * @return 查找到的文件集
*/
std::list<std::string> getDirFiles(char const* source, char const* filter)
{
	std::list<std::string> files;
	std::string currentPath;
	_finddata64i32_t findData;
	intptr_t findHandle = NULL;

	currentPath.reserve(strlen(source) + strlen(filter));
	currentPath.append(source);
	currentPath.append(filter);
	findHandle = _findfirst64i32(currentPath.c_str(), &findData);

	if (findHandle == -1) return files;

	do
	{
		if (findData.attrib & _A_ARCH) files.emplace_back(findData.name);

	} while (!_findnext64i32(findHandle, &findData));

	_findclose(findHandle);

	return files;
}

/**
 * @brief script.src与text.dat的校验
 * @param fileFullPath 文件全路径缓冲区
 * @param nameSectionPointer 文件名部分的指针
 * @param scriptSrc script.src 的二级文件指针
 * @param textDat text.dat 的二级文件指针
 * @return 校验结果
*/
bool tryDataFileOpen(char* fileFullPath, char* nameSectionPointer, FILE** scriptSrc, FILE** textDat)
{

	char fileHeader[0xC] = { 0 };

	strcpy(nameSectionPointer, "SCRIPT.SRC");
	*scriptSrc = fopen(fileFullPath, "rb");

	if (!*scriptSrc)
	{
		printf("ERROR : script.src open failed\n");
		return false;
	}

	fread(fileHeader, sizeof(scriptHeader), 1, *scriptSrc);

	if (0 != memcmp(fileHeader, scriptHeader, sizeof(scriptHeader)))
	{
		printf("ERROR : script.src format error \n");
		fclose(*scriptSrc);
		return false;
	}

	strcpy(nameSectionPointer, "TEXT.DAT");
	*textDat = fopen(fileFullPath, "rb");

	if (!*textDat)
	{
		printf("ERROR : text.dat open failed\n");
		return false;
	}

	fread(fileHeader, sizeof(textHeader), 1, *textDat);

	if (0 != memcmp(fileHeader, textHeader, sizeof(textHeader)))
	{
		printf("ERROR : text.dat format error\n");
		fclose(*scriptSrc);
		fclose(*textDat);
		return false;
	}

	return true;
}

/**
 * @brief 加密或解密一个缓冲区
 * @param buffer 目标缓冲区
 * @param bufferSize 缓冲区大小
 * @param decryptSwitch 加解密标志,true为解密,false为加密
 * @return true
*/
bool dataBufferTransform(uint8_t* buffer, size_t bufferSize, bool decryptSwitch)
{
	int firstKey = 0x84DF873;
	int secondKey = 0xFF987DEE;

	bufferSize -= (bufferSize & 3);

	if (decryptSwitch)
	{
		uint8_t leftShift = 4;

		for (size_t i = 0; i < bufferSize; i += 4)
		{
			_asm
			{
				mov eax, buffer;
				mov ebx, i;
				add eax, ebx;
				mov cl, leftShift;
				rol byte ptr ds : [eax] , cl;
			}
			//buffer[i] = ROLBYTE(buffer[i], leftShift); 也可以用宏
			leftShift++;
			*((int*)(buffer + i)) ^= firstKey;
			*((int*)(buffer + i)) ^= secondKey;
		}
	}
	else
	{
		uint8_t rightShift = 4;

		for (size_t i = 0; i < bufferSize; i += 4)
		{

			*((int*)(buffer + i)) ^= firstKey;
			*((int*)(buffer + i)) ^= secondKey;
			_asm
			{
				mov eax, buffer;
				mov ebx, i;
				add eax, ebx;
				mov cl, rightShift;
				ror byte ptr ds : [eax] , cl;
			}
			//buffer[i] = RORBYTE(buffer[i], rightShift); 也可以用宏
			rightShift++;
		}
	}

	return true;
}

/**
 * @brief 加密或解密单个文件
 * @param target 目标文件
 * @param decryptSwitch 加解密标志,true为解密,false为加密
 * @return 
*/
bool dataFileTransform(char const* target, bool decryptSwitch)
{
	std::vector<uint8_t> buffer;
	FILE* stream = fopen(target, "rb+");

	if (!stream) 
	{
		printf("ERROR : file open failed\n");
		return false;
	}

	fseek(stream, 0, SEEK_END);
	buffer.resize(ftell(stream));
	rewind(stream);
	fread(buffer.data(), buffer.capacity(), 1, stream);

	if (buffer[0] != '$')
	{
		printf("ERROR : file open failed\n");
		fclose(stream);
		return false;
	}

	dataBufferTransform(buffer.data() + 0x10, buffer.size() - 0x10, decryptSwitch);
	rewind(stream);
	fwrite(buffer.data(), buffer.size(), 1, stream);
	fclose(stream);
	return true;


}
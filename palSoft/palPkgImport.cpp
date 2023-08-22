#define _CRT_SECURE_NO_WARNINGS

#pragma warning(disable:6387 6031)

#include "palDefine.h"
#include "palUtils.h"

struct palImportObject
{
	palContentsEntry entry;
	FILE* stream = nullptr;
};

/**
 * @brief 指定区域校验和计算
 * @param buffer 目标缓冲区
 * @param bufferSize 缓冲区大小
 * @return 区域校验和
*/
inline uint32_t checkSumCalc(uint8_t const* buffer, size_t bufferSize)
{
	uint32_t checkSum = 0;

	for (size_t i = 0; i < bufferSize; i++) checkSum += buffer[i];

	return checkSum;
}

bool palPackageCreate(char const* const source, char const* const createPackage, bool checkSumSwitch)
{
	if (!source || !createPackage) return false;

	uint32_t checkSum = 0;

	FILE* archive = fopen(createPackage, "wb+");

	if (!archive)
	{
		printf("ERROR : packge create failed\n");
		return false;
	}

	palArchiveHeader archiveHeader;

	fwrite(&archiveHeader, sizeof(palArchiveHeader), 1, archive);

	if (checkSumSwitch) checkSum += checkSumCalc((uint8_t*)&archiveHeader, sizeof(palArchiveHeader));

	initialsIndexEntry charIndex[0xFF];	//字符索引表
	char prevChar = 0;	//上一次的字符
	int firstIndex = 0;	//首次出现时的索引

	fwrite(charIndex, sizeof(charIndex), 1, archive);

	std::list<std::string> files = getDirFiles(source, "\\*.*");
	std::list<palImportObject> objects;
	std::vector<palContentsEntry> entries;
	size_t entryCount = 0;
	char fileFullPath[_MAX_PATH] = { 0 };
	char* nameSectionPointer = nullptr;

	strcpy(fileFullPath, source);
	strcat(fileFullPath, "\\");
	nameSectionPointer = strrchr(fileFullPath, '\\') + 1;
	for (std::string& fileName : files)
	{
		if (fileName.length() > 0x20)
		{
			printf("ERROR : %s fileName too long\n", fileName.c_str());
			continue;
		}

		strcpy(nameSectionPointer, fileName.c_str());
		FILE* fileStream = fopen(fileFullPath, "rb");

		if (!fileStream)
		{
			printf("ERROR : %s open failed\n", fileName.c_str());
			continue;
		}

		palImportObject importObject;

		strcpy(importObject.entry.entryName, fileName.c_str());
		fseek(fileStream, 0, SEEK_END);
		importObject.entry.entrySize = ftell(fileStream);
		rewind(fileStream);
		importObject.stream = fileStream;

		if (prevChar != fileName[0])
		{
			charIndex[fileName[0]].firstFileIndex = firstIndex;
			prevChar = fileName[0];
		}

		charIndex[fileName[0]].usageCount++;
		firstIndex++;
		objects.emplace_back(importObject);
	}

	entryCount = objects.size();
	entries.reserve(entryCount);

	std::vector<uint8_t> dataBuffer;
	size_t contentsSize = sizeof(palContentsEntry) * entryCount;

	dataBuffer.resize(contentsSize);
	fwrite(dataBuffer.data(), contentsSize, 1, archive);
	for (palImportObject& importObject : objects)
	{
		importObject.entry.entryOffset = ftell(archive);
		dataBuffer.resize(importObject.entry.entrySize);
		fread(dataBuffer.data(), importObject.entry.entrySize, 1, importObject.stream);

		if (dataBuffer[0] == '$') dataBufferTransform(dataBuffer.data() + 0x10, importObject.entry.entrySize - 0x10, false);

		fwrite(dataBuffer.data(), importObject.entry.entrySize, 1, archive);

		if (checkSumSwitch) checkSum += checkSumCalc(dataBuffer.data(), importObject.entry.entrySize);

		fclose(importObject.stream);
		importObject.stream = nullptr;
		entries.emplace_back(std::move(importObject.entry));
	}

	fseek(archive, sizeof(palArchiveHeader::magicNumber), SEEK_SET);
	fwrite(&entryCount, 4, 1, archive);

	if (checkSumSwitch) checkSum += checkSumCalc((uint8_t*)entryCount, sizeof(entryCount));

	fwrite(charIndex, sizeof(charIndex), 1, archive);

	if (checkSumSwitch) checkSum += checkSumCalc((uint8_t*)charIndex, sizeof(charIndex));

	fwrite(entries.data(), contentsSize, 1, archive);

	if (checkSumSwitch) checkSum += checkSumCalc((uint8_t*)entries.data(), contentsSize);

	fseek(archive, 0, SEEK_END);

	if (checkSumSwitch) fwrite(&checkSum, 4, 1, archive);

	fwrite(archiveFooter, sizeof(archiveFooter), 1, archive);
	fclose(archive);
	printf("COMPELETED : packed %d files \n", entryCount);

	return true;

}
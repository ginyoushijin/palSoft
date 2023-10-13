#define _CRT_SECURE_NO_WARNINGS

#pragma warning(disable:6387 6031 6054)

#include "palDefine.h"
#include "palFunc.h"
#include "palUtils.h"

#include <Windows.h>
#include <vector>
#include <ShlObj.h>

bool palPackgeExtract(char const* const targetArchive, char const* const extractPath)
{

	if (!targetArchive || !extractPath) return false;

	SHCreateDirectoryExA(NULL, extractPath, NULL);

	char fileFullPath[_MAX_PATH] = { 0 };
	char* nameSectionPointer = nullptr;
	std::vector<uint8_t> buffer;

	palArchiveHeader fileHeader;
	uint8_t fileFooter[4] = { 0 };
	int extractCount = 0;

	FILE* archive = fopen(targetArchive, "rb");

	if (!archive)
	{
		printf("ERROR : open file failed\n");
		return false;
	}

	fread(&fileHeader, sizeof(palArchiveHeader), 1, archive);
	fseek(archive, -4, SEEK_END);
	fread(fileFooter, 4, 1, archive);

	if (0 != memcmp(fileHeader.magicNumber, archiveHeader, sizeof(archiveHeader))
		|| fileHeader.entryCount == 0
		|| 0 != memcmp(fileFooter, archiveFooter, sizeof(archiveFooter)))
	{
		fclose(archive);
		printf("ERROR : packge format don't match\n");
		return false;
	}

	strcpy(fileFullPath, extractPath);
	strcat(fileFullPath, "\\");
	nameSectionPointer = strrchr(fileFullPath, '\\') + 1;

	std::vector<palContentsEntry> entries;
	entries.resize(fileHeader.entryCount);
	fseek(archive, PAL_ARCHIVE_CONTENTS_OFFSET, SEEK_SET);
	fread(entries.data(), sizeof(palContentsEntry) * fileHeader.entryCount, 1, archive);

	for (palContentsEntry& entry : entries)
	{
		if (entry.entrySize == 0) continue;

		strcpy(nameSectionPointer, entry.entryName);
		FILE* entryStream = fopen(fileFullPath, "wb+");

		if (!entryStream)
		{
			printf("ERROR : %s create failed\n",entry.entryName);
			continue;
		}

		buffer.resize(entry.entrySize);
		fseek(archive, entry.entryOffset, SEEK_SET);
		fread(buffer.data(), entry.entrySize, 1, archive);

		if (buffer[0] == '$') dataBufferTransform(buffer.data() + 0x10, entry.entrySize - 0x10, true);

		fwrite(buffer.data(), entry.entrySize, 1, entryStream);
		fclose(entryStream);
		extractCount++;
	}

	fclose(archive);
	printf("COMPELETED : Extracted %d files \n", extractCount);

	return true;
}
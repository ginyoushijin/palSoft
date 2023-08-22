#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE

#include "palDefine.h"

int main(int argc, char* argv[])
{
	if (argc < 4)
	{
		printf("palSoft adv system tool\n");
		printf("Uagse:\n");
		printf("Extract package : <ToolName> -e <package file> <extract folder>\n");
		printf("Create package : <ToolName> -c <source directory> <create package file> <checkSum flag(y/other)>\n");
		printf("Extarct text : <ToolName> -s <script.src and text.dat path> <extract path>\n");
		printf("Import translated text : <ToolName> -i <original script.src and text.dat path> <translated text path> <imported newFile path>\n");
		printf("Decrypt/Encrypt file : <ToolName> -t <target file> <Decrypt/Encrypt flag (y/other)>\n");
		system("pause");
	}
	else
	{
		if (stricmp(argv[1], "-e") == 0)
		{
			palPackgeExtract(argv[2], argv[3]);
		}
		else if (stricmp(argv[1], "-c") == 0)
		{
			if (argc >= 5)
			{
				palPackageCreate(argv[2], argv[3], stricmp(argv[4], "y") == 0 ? true : false);
			}
			else
			{
				palPackageCreate(argv[2], argv[3]);
			}
		}
		else if (stricmp(argv[1], "-s") == 0)
		{
			palStringExtract(argv[2], argv[3]);
		}
		else if (stricmp(argv[1], "-i") == 0)
		{
			if (argc < 5)
			{
				printf("please enter <imported newFile path>\n");
				return 1;
			}
			palStringImport(argv[2], argv[3], argv[4]);
		}
		else if (stricmp(argv[1], "-t") == 0)
		{
			dataFileTransform(argv[2], stricmp(argv[3], "y") == 0 ? true : false);
		}
		else
		{
			printf("sorry, unkonw commnad\n");
			return 1;
		}
	}

	return 0;
}
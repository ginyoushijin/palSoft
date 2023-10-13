#define _CRT_SECURE_NO_WARNINGS

#pragma warning(disable:6387 6031 6054)

#include "palDefine.h"
#include "palFunc.h"
#include "palUtils.h"

#include <ShlObj.h>

bool palStringExtract(char const* const source, char const* const extractPath)
{
	if (!source || !extractPath) return false;

	SHCreateDirectoryExA(NULL, extractPath, NULL);

	char fileFullPath[_MAX_PATH] = { 0 };
	FILE* scriptSrc = nullptr;
	FILE* textDat = nullptr;
	char* nameSectionPointer = nullptr;

	strcpy(fileFullPath, source);
	strcat(fileFullPath, "\\");
	nameSectionPointer = strrchr(fileFullPath, '\\') + 1;

	if (!tryDataFileOpen(fileFullPath, nameSectionPointer, &scriptSrc, &textDat)) return false;

	fseek(scriptSrc, 0, SEEK_END);
	size_t scriptSize = ftell(scriptSrc) - (sizeof(scriptHeader) + 8);
	fseek(scriptSrc, sizeof(scriptHeader) + 8, SEEK_SET);

	textRenderParamter textParam;
	scenarioCheckParamter scenaParam;
	selectBranchSetParamter selectParam;
	buttonHoverSignature hoverSig;
	char charaName[0x40];
	char text[0x200];
	char scenarioName[0x40];
	char selectText[0x400];
	uint32_t opcode = 0;
	uint32_t textOrdinal = 0;
	uint32_t selectOrdinal = 0;
	uint32_t hoverOrdinal = 0;
	uint32_t extractCount = 0;
	FILE* scenarioFile = nullptr;
	FILE* selectFile = nullptr;
	FILE* hoverFile = nullptr;
	char* selectTextPointer = nullptr;

	strcpy(fileFullPath, extractPath);
	strcat(fileFullPath, "\\");
	nameSectionPointer = strrchr(fileFullPath, '\\') + 1;
	strcpy(nameSectionPointer, "hover.txt");
	hoverFile = fopen(fileFullPath, "w+");

	if (!hoverFile) printf("ERROR : extract file hover.txt create failed\n");

	while (!feof(scriptSrc))
	{
		fread(&opcode, 4, 1, scriptSrc);
		switch (opcode)
		{
			case vmOpcode::VariableAssignment:
			{
				if (!hoverFile) break;

				memset(text, 0, sizeof(text));
				fread(&hoverSig, sizeof(buttonHoverSignature), 1, scriptSrc);

				if (hoverSig.firstAction == 0x40000002
					&& hoverSig.setValue < PAL_SCRIPT_NULL
					&& hoverSig.setPsuh.opcode == vmOpcode::Push
					&& hoverSig.setPsuh.value == 0x40000002)
				{
					fseek(textDat, hoverSig.setValue + 4, SEEK_SET);
					fscanf(textDat, "%[\x01-\xFF]", text);
					fprintf(hoverFile, "☆%.5d☆%s\n", hoverOrdinal, text);
					fprintf(hoverFile, "★%.5d★%s\n", hoverOrdinal, text);
					fprintf(hoverFile, "\n");
					hoverOrdinal++;
				}
				else
				{
					fseek(scriptSrc, -8, SEEK_CUR);
				}
				break;
			}
			case vmOpcode::ScriptRun:
			{
				fread(&opcode, 4, 1, scriptSrc);
				switch (opcode)
				{
					case scriptOpcode::SpecialTextRender:
					case scriptOpcode::TextRender:
					{
						if (!scenarioFile) break;

						memset(charaName, 0, sizeof(charaName));
						memset(text, 0, sizeof(text));
						fseek(scriptSrc, -0x28, SEEK_CUR);
						fread(&textParam, sizeof(textRenderParamter), 1, scriptSrc);

						if (textParam.charaName.value != PAL_SCRIPT_NULL)
						{
							fseek(textDat, textParam.charaName.value + 4, SEEK_SET);
							fscanf(textDat, "%[\x01-\xFF]", charaName);
							fprintf(scenarioFile, "☆%.5d☆%s\n", textOrdinal, charaName);
							fprintf(scenarioFile, "★%.5d★%s\n", textOrdinal, charaName);
							fprintf(scenarioFile, "\n");
							textOrdinal++;
						}

						fseek(textDat, textParam.text.value + 4, SEEK_SET);
						fscanf(textDat, "%[\x01-\xFF]", text);
						fprintf(scenarioFile, "☆%.5d☆%s\n", textOrdinal, text);
						fprintf(scenarioFile, "★%.5d★%s\n", textOrdinal, text);
						fprintf(scenarioFile, "\n");
						textOrdinal++;
						fseek(scriptSrc, +8, SEEK_CUR);
						break;
					}
					case scriptOpcode::ScenarioCheck:
					{
						memset(scenarioName, 0, sizeof(scenarioName));

						if (scenarioFile) fclose(scenarioFile);

						fseek(scriptSrc, -0x18, SEEK_CUR);
						fread(&scenaParam, sizeof(scenarioCheckParamter), 1, scriptSrc);
						fseek(textDat, scenaParam.scenario.value + 4, SEEK_SET);
						fscanf(textDat, "%[\x01-\xFF]", scenarioName);
						strcpy(nameSectionPointer, scenarioName);
						strcat(fileFullPath, ".txt");
						scenarioFile = fopen(fileFullPath, "w+");

						if (!scenarioFile)
							printf("ERROR : extract file %s.txt create failed\n", scenarioName);
						else
							extractCount++;

						textOrdinal ^= textOrdinal;
						fseek(scriptSrc, +8, SEEK_CUR);
						break;
					}
					case scriptOpcode::SelectStart:
					{
						if (!selectFile)
						{
							strcpy(nameSectionPointer, "select.txt");
							selectFile = fopen(fileFullPath, "w+");

							if (!selectFile)
							{
								printf("ERROR : extract file select.txt create failed\n");
								break;
							}
						}
						memset(selectText, 0, sizeof(selectText));
						selectTextPointer = selectText;
						break;
					}
					case scriptOpcode::SelectBranch:
					{
						if (!selectFile) break;

						fseek(scriptSrc, -0x20, SEEK_CUR);
						fread(&selectParam, sizeof(selectBranchSetParamter), 1, scriptSrc);
						fseek(textDat, selectParam.selectText.value + 4, SEEK_SET);
						fscanf(textDat, "%[\x01-\xFF]", selectTextPointer);
						strcat(selectText, "|");
						selectTextPointer = strrchr(selectText, '|') + 1;
						fseek(scriptSrc, +8, SEEK_CUR);
						break;
					}
					case scriptOpcode::SelectEnd:
					{
						if (!selectFile) break;

						*(selectTextPointer - 1) = 0;
						fprintf(selectFile, "☆%.5d☆%s\n", selectOrdinal, selectText);
						fprintf(selectFile, "★%.5d★%s\n", selectOrdinal, selectText);
						fprintf(selectFile, "\n");
						selectOrdinal++;
						break;
					}
					default: break;
				}
				break;
			}
			default: break;
		}

	};

	if (hoverFile)
	{
		extractCount++;
		fclose(hoverFile);
	}
	if (selectFile)
	{
		extractCount++;
		fclose(selectFile);
	}
	if (scenarioFile) fclose(scenarioFile);
	fclose(textDat);
	fclose(scriptSrc);
	printf("COMPELETED : Extracted %d files \n", extractCount);

	return true;
}
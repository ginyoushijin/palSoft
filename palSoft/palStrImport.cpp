#define _CRT_SECURE_NO_WARNINGS

#pragma warning(disable:6387 6031)

#include "palDefine.h"
#include "palFunc.h"
#include "palUtils.h"

#include <future>
#include <unordered_map>

using fileTextPair = std::pair<std::string, std::list<std::string>>;

uint32_t strCount = 0;	//Text.dat中字符串的计数
std::vector<std::string> allStrings;	//Text.dat中所有字符串的集合
std::vector<uint32_t> datOrdToPosMap;	//依照字符集编号获取新坐标
std::unordered_map<uint32_t, uint32_t> datPosToOrdMap;	//保存旧字符集偏移-序号的映射集合
std::unordered_map<std::string, std::list<std::string>> textList;	//翻译文本目录中，所有文本文件及其翻译文本的集合
std::list<std::string> currentScenarioTexts;	//当前剧本的文本
std::list<std::string> selectTexts;	//选项分支文本，单独拿出
std::list<std::string> hoverTexts;	//鼠标hover按钮时的提示语句文本，单独拿出

void readAllStringFromTextDat(FILE* origTextDat);

void readAllFileTextFromDirectory(char* const fileFullPath, char* const nameSectionPointer, char const* const scenarioPath);

void replaceOrgTextToTranText(FILE* origTextDat, FILE* origScriptSrc);

void WriteToNewTextDat(FILE* newTextDat);

void WriteToNewScriptSrc(FILE* origTextDat, FILE* origScriptSrc, FILE* newScriptSrc);

fileTextPair getScenarioText(std::string fileName, FILE* stream);


bool palStringImport(char const* const dataPath, char const* const scenarioPath, char const* const createPath)
{
	if (!dataPath || !scenarioPath || !createPath) return false;

	char fileFullPath[_MAX_PATH] = { 0 };
	char* nameSectionPointer = nullptr;	//
	FILE* origScriptSrc = nullptr;
	FILE* origTextDat = nullptr;
	FILE* newScriptSrc = nullptr;
	FILE* newTextDat = nullptr;

	strcpy(fileFullPath, dataPath);
	strcat(fileFullPath, "\\");
	nameSectionPointer = strrchr(fileFullPath, '\\') + 1;

	if (!tryDataFileOpen(fileFullPath, nameSectionPointer, &origScriptSrc, &origTextDat)) return false;	//尝试打开源SCRIPT.SRC与TEXT.DAT

	readAllStringFromTextDat(origTextDat);	//读取源TEXT.DAT中的所有字符串到allStrings中

	strcpy(fileFullPath, scenarioPath);	//设置翻译文本的路径
	strcat(fileFullPath, "\\");
	nameSectionPointer = strrchr(fileFullPath, '\\') + 1;	//更新nameSectionPointer

	readAllFileTextFromDirectory(fileFullPath,nameSectionPointer,scenarioPath);	//读取翻译文本

	replaceOrgTextToTranText(origTextDat,origScriptSrc);	//将翻译过的文本替换到allStrings中

	strcpy(fileFullPath, createPath);	//设置导入后SCRIPT.SRC与TEXT.DAT的导出路径
	strcat(fileFullPath, "\\");
	nameSectionPointer = strrchr(fileFullPath, '\\') + 1;	//更新nameSectionPointer
	strcpy(nameSectionPointer, "TEXT.DAT");
	newTextDat = fopen(fileFullPath, "wb+");	//尝试创建导入后的TEXT.DAT

	if (!newTextDat)
	{
		printf("ERROR : imported TEXT.DAT create failed\n");
		fclose(origTextDat);
		fclose(origScriptSrc);
		return false;
	}

	WriteToNewTextDat(newTextDat);	//为新的TEXT.DAT写入数据

	fclose(newTextDat);
	newTextDat = nullptr;

	dataFileTransform(fileFullPath, false);	//新的TEXT.DAT需要加密，偷懒直接用文件了
	
	strcpy(nameSectionPointer, "SCRIPT.SRC");
	newScriptSrc = fopen(fileFullPath, "wb+"); //尝试创建导入后的SCRIPT.SRC

	if (!newScriptSrc)
	{
		printf("ERROR : imported SCRIPT.SRC create failed\n");
		fclose(origTextDat);
		fclose(origTextDat);
		return false;
	}

	WriteToNewScriptSrc(origTextDat,origScriptSrc,newScriptSrc);	////为新的SCRIPT.SRC写入数据

	if(newScriptSrc) fclose(newScriptSrc);
	if(origScriptSrc) fclose(origScriptSrc);
	if (origTextDat) fclose(origTextDat);
	printf("COMPELETED : import sucess \n");

	return true;
}

/**
 * @brief 读取源TEXT.DAT的所有文本至allStrings中
 * @param origTextDat 源TEXT.DAT的stream
*/
void readAllStringFromTextDat(FILE* origTextDat)
{
	char textBuffer[0x200] = { 0 };
	uint32_t datStrOrdinal = 0;
	uint32_t datOrdinalPosition = 0;

	fread(&strCount, 4, 1, origTextDat);
	allStrings.reserve(strCount);

	while (!feof(origTextDat))	//读取原text.dat中的所有字符串
	{
		datOrdinalPosition = ftell(origTextDat);
		fread(&datStrOrdinal, 4, 1, origTextDat);
		datPosToOrdMap.emplace(datOrdinalPosition, datStrOrdinal);
		fscanf(origTextDat, "%[\x01-\xff]", textBuffer);
		fgetc(origTextDat);
		allStrings.emplace_back(textBuffer);
	}
}

/**
 * @brief 读取翻译目录下所有文本文件的翻译文本
 * @param fileFullPath 
 * @param nameSectionPointer 
 * @param scenarioPath 翻译路径
*/
void readAllFileTextFromDirectory(char* const fileFullPath,char* const nameSectionPointer,char const* const scenarioPath)
{
	std::forward_list<std::string> files = getDirFiles(scenarioPath, "\\*.txt");	//获取一级目录下所有的txt文件
	std::vector<std::future<fileTextPair>> tasks;
	uint32_t maxThreads = std::thread::hardware_concurrency();

	tasks.reserve(maxThreads);

	while (!files.empty())	//获取所有翻译文本中的翻译字符串，并加入至map中
	{
		uint32_t taskCount = 0;

		tasks.clear();

		while (taskCount < maxThreads)
		{
			if (files.empty()) break;

			std::string fileName = std::move(files.front());
			files.pop_front();
			strcpy(nameSectionPointer, fileName.c_str());
			FILE* scenarioFile = fopen(fileFullPath, "r");

			if (!scenarioFile)
			{
				printf("ERROR : %s open failed\n", fileName.c_str());
				continue;
			}

			std::future<fileTextPair> task = std::async(std::launch::async, getScenarioText, std::move(fileName), scenarioFile);
			tasks.emplace_back(std::move(task));
			taskCount++;
		}

		for (std::future<fileTextPair>& task : tasks) textList.emplace(std::move(task.get()));
	}
	
}

/**
 * @brief 替换allstrings中被翻译过的文本
 * @param origTextDat 源TEXT.DAT的stream	 读取被翻译文本的序号用
 * @param origScriptSrc 源SCRIPT.SRC的stream 确定需要被翻译的文本用
*/
void replaceOrgTextToTranText(FILE* origTextDat, FILE* origScriptSrc)
{
	char scenarioName[0x40] = { 0 };
	uint32_t ordinalInAllStr = 0;
	bool scenarioSwitch = false;
	std::string selectLine;	//一次分支的所有选项
	textRenderParamter textParam;
	scenarioCheckParamter scenaParam;
	selectBranchSetParamter selectParam;
	configLoadParamter configParam;
	debugStrSetParamter debugStrParam;
	buttonHoverSignature hoverSig;

	fseek(origScriptSrc, 8, SEEK_CUR);

	if (textList.count("select.txt"))
	{
		selectTexts = std::move(textList["select.txt"]);
		textList.erase("select.txt");
	}

	if (textList.count("hover.txt"))
	{
		hoverTexts = std::move(textList["hover.txt"]);
		textList.erase("hover.txt");
	}

	for (uint32_t opcode = 0; !feof(origScriptSrc);)	//导入翻译文本，替换字符集
	{
		fread(&opcode, 4, 1, origScriptSrc);
		switch (opcode)
		{
			case vmOpcode::VariableAssignment:
			{
				if (hoverTexts.empty()) break;

				fread(&hoverSig, sizeof(buttonHoverSignature), 1, origScriptSrc);

				if (hoverSig.firstAction == 0x40000002
					&& hoverSig.setValue < PAL_SCRIPT_NULL
					&& hoverSig.setPsuh.opcode == vmOpcode::Push
					&& hoverSig.setPsuh.value == 0x40000002)
				{
					fseek(origTextDat, hoverSig.setValue, SEEK_SET);
					fread(&ordinalInAllStr, 4, 1, origTextDat);
					allStrings[ordinalInAllStr] = std::move(hoverTexts.front());
					hoverTexts.pop_front();
				}
				else
				{
					fseek(origScriptSrc, -8, SEEK_CUR);
				}

				break;
			}
			case vmOpcode::ScriptRun:
			{
				fread(&opcode, 4, 1, origScriptSrc);
				switch (opcode)
				{
					case scriptOpcode::SpecialTextRender:
					case scriptOpcode::TextRender:
					{
						if (!scenarioSwitch) break;

						fseek(origScriptSrc, -0x28, SEEK_CUR);
						fread(&textParam, sizeof(textRenderParamter), 1, origScriptSrc);

						if (textParam.charaName.value != PAL_SCRIPT_NULL)
						{
							fseek(origTextDat, textParam.charaName.value, SEEK_SET);
							fread(&ordinalInAllStr, 4, 1, origTextDat);
							allStrings[ordinalInAllStr] = std::move(currentScenarioTexts.front());
							currentScenarioTexts.pop_front();
						}

						fseek(origTextDat, textParam.text.value, SEEK_SET);
						fread(&ordinalInAllStr, 4, 1, origTextDat);
						allStrings[ordinalInAllStr] = std::move(currentScenarioTexts.front());
						currentScenarioTexts.pop_front();
						fseek(origScriptSrc, +8, SEEK_CUR);
						break;

					}
					case scriptOpcode::ScenarioCheck:
					{
						fseek(origScriptSrc, -0x18, SEEK_CUR);
						fread(&scenaParam, sizeof(scenarioCheckParamter), 1, origScriptSrc);
						fseek(origTextDat, scenaParam.scenario.value + 4, SEEK_SET);
						fscanf(origTextDat, "%[\x01-\xFF]", scenarioName);
						strcat(scenarioName, ".txt");

						if (textList.count(scenarioName))
						{
							scenarioSwitch = true;
							currentScenarioTexts = std::move(textList[scenarioName]);
						}
						else
						{
							scenarioSwitch = false;
						}

						fseek(origScriptSrc, +8, SEEK_CUR);
						textList.erase(scenarioName);
						break;
					}
					case scriptOpcode::SelectStart:
					{
						if (selectTexts.empty()) break;

						selectLine = std::move(selectTexts.front());
						selectTexts.pop_front();
						break;
					}
					case scriptOpcode::SelectBranch:
					{
						if (selectLine.empty()) break;

						fseek(origScriptSrc, -0x20, SEEK_CUR);
						fread(&selectParam, sizeof(selectBranchSetParamter), 1, origScriptSrc);
						fseek(origTextDat, selectParam.selectText.value, SEEK_SET);
						fread(&ordinalInAllStr, 4, 1, origTextDat);

						size_t spiltPos = selectLine.find('|');

						if (spiltPos != std::string::npos)
						{
							allStrings[ordinalInAllStr] = std::move(selectLine.substr(0, spiltPos));
							selectLine.erase(0, spiltPos + 1);
						}
						else
						{
							allStrings[ordinalInAllStr] = std::move(selectLine.substr(0, selectLine.size()));
							selectLine.clear();
						}

						fseek(origScriptSrc, 8, SEEK_CUR);
						break;
					}
					default: break;
				}
				break;
			}
			default: break;
		}
	}
}


/**
 * @brief 将被替换过的allstrings的字符串写入新的TEXT.DAT中，同时会将新string的起始位置记入datOrdToPosMap中
 * @param newTextDat 新TEXT.DAT的stream
*/
void WriteToNewTextDat(FILE* newTextDat)
{

	datOrdToPosMap.reserve(strCount);
	fwrite(textHeader, sizeof(textHeader), 1, newTextDat);
	fwrite(&strCount, 4, 1, newTextDat);

	for (size_t i = 0; i < strCount; i++)	//将替换后的字符串写入一个新的text.dat，并保存其中每个字符段的起始偏移
	{
		datOrdToPosMap.emplace_back(ftell(newTextDat));
		fwrite(&i, 4, 1, newTextDat);
		fwrite(allStrings[i].c_str(), allStrings[i].size(), 1, newTextDat);
		fputc(0, newTextDat);
	}
}

/**
 * @brief 更新翻译文本后，将新的字符串位置写入新的SCRIPT.SRC
 * @param origTextDat	源TEXT.DAT的stream 获取字符串序号用
 * @param origScriptSrc 源SCRIPT.SRC的stream 重写参照
 * @param newScriptSrc  新SCRIPT.SRC的stream
 * @warning 由于并不会更新所有字符串的位置，所以不保证能完美允许
*/
void WriteToNewScriptSrc(FILE* origTextDat,FILE* origScriptSrc,FILE* newScriptSrc)
{
	char scriptHeader[0xC] = { 0 };
	size_t origScriptSize = ftell(origScriptSrc);	//源SCRIPT.SRC的size，新的SCRIPT.SRC的size不应该超过它
	textRenderParamter textParam;
	scenarioCheckParamter scenaParam;
	selectBranchSetParamter selectParam;
	configLoadParamter configParam;
	debugStrSetParamter debugStrParam;
	buttonHoverSignature hoverSig;
	dynamicStrSignature dynamicSig;
	scrollRollStrSignature scrollSig;
	doubleParamterCommand callCommandBuffer;

	rewind(origScriptSrc);
	fread(scriptHeader, sizeof(scriptHeader), 1, origScriptSrc);
	fwrite(scriptHeader, sizeof(scriptHeader), 1, newScriptSrc);

	for (uint32_t opcode = 0; ftell(newScriptSrc) < origScriptSize;)
	{
		fread(&opcode, 4, 1, origScriptSrc);

		switch (opcode)	//分析脚本opcode采取不同操作
		{
			case vmOpcode::VariableAssignment:
			{
				fwrite(&variableAssignmentOpcode, 4, 1, newScriptSrc);
				fread(&hoverSig, sizeof(buttonHoverSignature), 1, origScriptSrc);

				if (hoverSig.firstAction == 0x40000002
					&& hoverSig.setValue < PAL_SCRIPT_NULL
					&& hoverSig.setPsuh.opcode == vmOpcode::Push
					&& hoverSig.setPsuh.value == 0x40000002)
				{
					hoverSig.setValue = datOrdToPosMap[datPosToOrdMap[hoverSig.setValue]];
					datPosToOrdMap.erase(hoverSig.setValue);
				}
				fwrite(&hoverSig, sizeof(buttonHoverSignature), 1, newScriptSrc);
				break;
			}
			case vmOpcode::VariableXor:
			{
				fseek(origScriptSrc, -0xC, SEEK_CUR);
				fread(&dynamicSig, sizeof(dynamicStrSignature), 1, origScriptSrc);

				if (dynamicSig.prevAction.opcode == vmOpcode::Push && dynamicSig.prevAction.value < PAL_SCRIPT_NULL)
				{
					if (memcmp(&dynamicSig.currentAction, soundStrSetRefer, sizeof(soundStrSetRefer)) == 0
						|| memcmp(&dynamicSig.currentAction, favoriteStrSetRefer, sizeof(favoriteStrSetRefer)) == 0
						|| memcmp(&dynamicSig.currentAction, hideStrSetRefer, sizeof(hideStrSetRefer)) == 0
						|| memcmp(&dynamicSig.currentAction, debutStrSetRefer, sizeof(debutStrSetRefer)) == 0
						|| memcmp(&dynamicSig.currentAction, musicStrSetRefer, sizeof(musicStrSetRefer)) == 0
						|| memcmp(&dynamicSig.currentAction, cgStrSetRefer, sizeof(cgStrSetRefer)) == 0)
					{
						dynamicSig.prevAction.value = datOrdToPosMap[datPosToOrdMap[dynamicSig.prevAction.value]];
						datPosToOrdMap.erase(hoverSig.setValue);
						fseek(newScriptSrc, -0x8, SEEK_CUR);
						fwrite(&dynamicSig, sizeof(dynamicStrSignature), 1, newScriptSrc);
						break;
					}
				}
				fseek(origScriptSrc, -0x20, SEEK_CUR);
				fwrite(&dynamicSig.currentAction, sizeof(doubleParamterCommand), 1, newScriptSrc);
				break;

			}
			case vmOpcode::ScriptRun:
			{
				fread(&opcode, 4, 1, origScriptSrc);
				switch (opcode)
				{
					case scriptOpcode::TextRender:
					case scriptOpcode::SpecialTextRender:
					{
						fseek(origScriptSrc, -0x28, SEEK_CUR);
						fread(&textParam, sizeof(textRenderParamter), 1, origScriptSrc);
						fseek(newScriptSrc, -(int)sizeof(textRenderParamter), SEEK_CUR);

						if (textParam.charaName.value != PAL_SCRIPT_NULL)
						{
							textParam.charaName.value = datOrdToPosMap[datPosToOrdMap[textParam.charaName.value]];
							datPosToOrdMap.erase(textParam.charaName.value);
						}

						textParam.text.value = datOrdToPosMap[datPosToOrdMap[textParam.text.value]];
						datPosToOrdMap.erase(textParam.text.value);
						fwrite(&textParam, sizeof(textRenderParamter), 1, newScriptSrc);
						fread(&callCommandBuffer, sizeof(callCommandBuffer), 1, origScriptSrc);
						fwrite(&callCommandBuffer, sizeof(callCommandBuffer), 1, newScriptSrc);
						break;
					}
					case scriptOpcode::ScenarioCheck:
					{
						fseek(origScriptSrc, -0x18, SEEK_CUR);
						fread(&scenaParam, sizeof(scenarioCheckParamter), 1, origScriptSrc);
						fseek(newScriptSrc, -(int)sizeof(scenarioCheckParamter), SEEK_CUR);
						scenaParam.scenario.value = datOrdToPosMap[datPosToOrdMap[scenaParam.scenario.value]];
						fwrite(&scenaParam, sizeof(scenarioCheckParamter), 1, newScriptSrc);
						fread(&callCommandBuffer, sizeof(callCommandBuffer), 1, origScriptSrc);
						fwrite(&callCommandBuffer, sizeof(callCommandBuffer), 1, newScriptSrc);
						break;
					}
					case scriptOpcode::SelectBranch:
					{
						fseek(origScriptSrc, -0x20, SEEK_CUR);
						fread(&selectParam, sizeof(selectBranchSetParamter), 1, origScriptSrc);
						fseek(newScriptSrc, -(int)sizeof(selectBranchSetParamter), SEEK_CUR);
						selectParam.selectText.value = datOrdToPosMap[datPosToOrdMap[selectParam.selectText.value]];
						fwrite(&selectParam, sizeof(selectBranchSetParamter), 1, newScriptSrc);
						fread(&callCommandBuffer, sizeof(callCommandBuffer), 1, origScriptSrc);
						fwrite(&callCommandBuffer, sizeof(callCommandBuffer), 1, newScriptSrc);
						break;
					}
					case scriptOpcode::ConfigLoadStr:
					{
						fseek(origScriptSrc, -0x28, SEEK_CUR);
						fread(&configParam, sizeof(configLoadParamter), 1, origScriptSrc);
						fseek(newScriptSrc, -(int)sizeof(configLoadParamter), SEEK_CUR);

						if (configParam.openFile.value != PAL_SCRIPT_NULL) configParam.openFile.value = datOrdToPosMap[datPosToOrdMap[configParam.openFile.value]];

						configParam.loadSection.value = datOrdToPosMap[datPosToOrdMap[configParam.loadSection.value]];
						configParam.loadParamter.value = datOrdToPosMap[datPosToOrdMap[configParam.loadParamter.value]];
						fwrite(&configParam, sizeof(configLoadParamter), 1, newScriptSrc);
						fread(&callCommandBuffer, sizeof(callCommandBuffer), 1, origScriptSrc);
						fwrite(&callCommandBuffer, sizeof(callCommandBuffer), 1, newScriptSrc);
						break;
					}
					case scriptOpcode::ConfigLoadInt:
					{
						fseek(origScriptSrc, -0x28, SEEK_CUR);
						fread(&configParam, sizeof(configLoadParamter), 1, origScriptSrc);
						fseek(newScriptSrc, -(int)sizeof(configLoadParamter), SEEK_CUR);
						configParam.loadSection.value = datOrdToPosMap[datPosToOrdMap[configParam.loadSection.value]];
						configParam.loadParamter.value = datOrdToPosMap[datPosToOrdMap[configParam.loadParamter.value]];
						fwrite(&configParam, sizeof(configLoadParamter), 1, newScriptSrc);
						fread(&callCommandBuffer, sizeof(callCommandBuffer), 1, origScriptSrc);
						fwrite(&callCommandBuffer, sizeof(callCommandBuffer), 1, newScriptSrc);
						break;
					}
					case scriptOpcode::DebugStrSet:
					{
						fseek(origScriptSrc, -0x18, SEEK_CUR);
						fread(&debugStrParam, sizeof(debugStrParam), 1, origScriptSrc);
						fseek(newScriptSrc, -(int)sizeof(debugStrSetParamter), SEEK_CUR);
						debugStrParam.firstStr.value = datOrdToPosMap[datPosToOrdMap[debugStrParam.firstStr.value]];
						fwrite(&debugStrParam, sizeof(debugStrSetParamter), 1, newScriptSrc);
						fread(&callCommandBuffer, sizeof(callCommandBuffer), 1, origScriptSrc);
						fwrite(&callCommandBuffer, sizeof(callCommandBuffer), 1, newScriptSrc);
						break;
					}
					case scriptOpcode::ConfigScrollRoll:
					{

						fseek(origScriptSrc, -0x24, SEEK_CUR);
						fread(&scrollSig, sizeof(scrollRollStrSignature), 1, origScriptSrc);

						if (scrollSig.prevBy3Action.opcode == vmOpcode::Push
							&& memcmp(&scrollSig.prevBy2Action.opcode, scrollRollSetRefer, sizeof(scrollRollSetRefer)) == 0)
						{
							scrollSig.prevBy3Action.value = datOrdToPosMap[datPosToOrdMap[scrollSig.prevBy3Action.value]];
							fseek(newScriptSrc, -(int)sizeof(scrollRollStrSignature), SEEK_CUR);
							fwrite(&scrollSig, sizeof(scrollRollStrSignature), 1, newScriptSrc);
						}

						fread(&callCommandBuffer, sizeof(callCommandBuffer), 1, origScriptSrc);
						fwrite(&callCommandBuffer, sizeof(callCommandBuffer), 1, newScriptSrc);
						break;
					}
					default:
					{
						fwrite(&functionCallOpcode, 4, 1, newScriptSrc);
						fwrite(&opcode, 4, 1, newScriptSrc);
						break;
					}
				}
				break;
			}
			default:
			{
				fwrite(&opcode, 4, 1, newScriptSrc);
				break;
			}
		}
	}
}

/**
 * @brief 获取指定文件中的所有翻译文本
 * @param fileName 读取文件的文件名
 * @param stream 读取文件的文件指针
 * @return 文件名与翻译文本集合
*/
fileTextPair getScenarioText(std::string fileName, FILE* stream)
{
	char buffer[0x200] = { 0 };
	std::list<std::string> texts;

	while (!feof(stream))
	{
		fgets(buffer, sizeof(buffer), stream);
		if (*(uint16_t*)buffer == 0xEFA1  /* gb2312 的 '★' */ && buffer[1] == buffer[8])
		{
			*strrchr(buffer, '\n') = 0;
			texts.emplace_back(buffer + 9);
		}
	}

	fclose(stream);

	return std::make_pair(std::move(fileName), std::move(texts));
}

#ifndef PAL_DEFINE_H
#define PAL_DEFINE_H

#include <stdint.h>
#include <Windows.h>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <io.h>
#include <chrono>
#include <thread>
#include <future>
#include <stdio.h>
#include <ShlObj.h>

#define PAL_ARCHIVE_CONTENTS_OFFSET 0x804
#define PAL_SCRIPT_NULL 0x0FFFFFFF

enum vmOpcode
{
	VariableAssignment = 0x10001,
	VariableXor = 0x10008,
	ScriptRun = 0x10017,
	Push = 0x1001F,
};

enum scriptOpcode
{
	TextRender = 0x20002,
	SpecialTextRender = 0x2000F,
	SelectStart = 0x60001,
	SelectBranch = 0x60002,
	SelectEnd = 0x60003,
	ScenarioCheck = 0xF0002,
	DebugStrSet = 0xF0004,
	ConfigScrollRoll = 0x120009,
	ConfigLoadStr = 0x120024,
	ConfigLoadInt = 0x120025,
};

//----------------------------------------------------------------------------------

struct palArchiveHeader
{
	char magicNumber[8] = { 0x50,0x41,0x43,0x20,0x0,0x0,0x0,0x0 };
	size_t entryCount = 0;
};

static_assert(sizeof(palArchiveHeader) == 12, "palArchiveHeader size error");

struct initialsIndexEntry
{
	uint32_t firstFileIndex = 0;
	uint32_t usageCount = 0;
};

static_assert(sizeof(initialsIndexEntry) == 8, "initialsIndexEntry size error");

struct palContentsEntry
{
	char entryName[0x20] = { 0 };
	size_t entrySize = 0;
	uint32_t entryOffset = 0;
};

static_assert(sizeof(palContentsEntry) == 0x28, "alContentsEntry size error");

struct singleParamterCommand
{
	uint32_t opcode = 0;
	int value = 0;
};

static_assert(sizeof(singleParamterCommand) == 8, "singleParamterCommand size error");

struct doubleParamterCommand
{
	uint32_t opcode = 0;
	int firstValue = 0;
	int secondValue = 0;
};

static_assert(sizeof(doubleParamterCommand) == 0xC, "doubleParamterCommand size error");

struct textRenderParamter
{
	singleParamterCommand zeroValue;
	singleParamterCommand text;
	singleParamterCommand charaName;
	singleParamterCommand voice;
};

struct scenarioCheckParamter
{
	singleParamterCommand zeroValue;
	singleParamterCommand scenario;
};

struct selectBranchSetParamter
{
	singleParamterCommand zeroValue;
	singleParamterCommand jumpAddress;
	singleParamterCommand selectText;
};

struct configLoadParamter
{
	singleParamterCommand readFile;
	singleParamterCommand openFile;
	singleParamterCommand loadParamter;
	singleParamterCommand loadSection;
};

struct debugStrSetParamter	//应该是个变参指令，最多好像能有8个，不过应该只要看第一个就好了吧
{
	singleParamterCommand secondStr;
	singleParamterCommand firstStr;
};

struct buttonHoverSignature
{
	uint32_t firstAction = 0;
	uint32_t setValue = 0;
	singleParamterCommand setPsuh;
};

struct dynamicStrSignature
{
	singleParamterCommand prevAction;
	doubleParamterCommand currentAction;
	doubleParamterCommand nextAction;
	singleParamterCommand secondAction;
	doubleParamterCommand thridAction;
};

struct scrollRollStrSignature
{
	singleParamterCommand prevBy3Action;
	doubleParamterCommand prevBy2Action;
	singleParamterCommand prevAction;
};


//----------------------------------------------------------------------------------

constexpr uint8_t archiveHeader[4] = { 0x50,0x41,0x43,0x20 };	//pac封包文件头: PAC+0x20
constexpr uint8_t archiveFooter[4] = { 0x45,0x4F,0x46,0x20 };	//pac封包文件尾: EOF+0x20
constexpr uint8_t scriptHeader[4] = { 0x53,0x76,0x32,0x30 };	//script.dat文件头
constexpr uint8_t textHeader[0xC] = { 0x24,0x54,0x45,0x58,0x54,0x5F,0x4C,0x49,0x53,0x54,0x5F,0x5F };	//text.dat文件头

constexpr uint32_t functionCallOpcode = 0x10017;
constexpr uint32_t variableAssignmentOpcode = 0x10001;
constexpr uint32_t soundStrSetRefer[11] = { 0x10008,0x40000000,0x40000000,0x10001,0x40000000,0x50830000,0x1001F,0x40000000,0x10017,0x120009,0x0 };
constexpr uint32_t favoriteStrSetRefer[11] = { 0x10008,0x40000000,0x40000000,0x10001,0x40000000,0x50860000,0x1001F,0x40000000,0x10017,0x120009,0x0 };
constexpr uint32_t hideStrSetRefer[11] = { 0x10008,0x40000000,0x40000000,0x10001,0x40000000,0x50850000,0x1001F,0x40000000,0x10017,0x120009,0x0 };
constexpr uint32_t debutStrSetRefer[11] = { 0x10008,0x40000000,0x40000000,0x10001,0x40000000,0x50800000,0x1001F,0x40000000,0x10017,0x120009,0x0 };
constexpr uint32_t musicStrSetRefer[11] = { 0x10008,0x40000000,0x40000000,0x10001,0x40000000,0x50840000,0x1001F,0x40000000,0x10017,0x120009,0x0 };
constexpr uint32_t cgStrSetRefer[11] = { 0x10008,0x40000000,0x40000000,0x10001,0x40000000,0x508A0000,0x1001F,0x40000000,0x10017,0x120009,0x0 };
constexpr uint32_t scrollRollSetRefer[5] = { 0x10001,0x40000000,0x80000001,0x1001F,0x40000000 };

//----------------------------------------------------------------------------------

bool palPackgeExtract(char const* const targetArchive, char const* const extractPath);

bool palPackageCreate(char const* const source, char const* const createPackage, bool checkSumSwitch = false);

bool palStringExtract(char const* const source, char const* const extractPath);

bool palStringImport(char const* const dataPath, char const* const scenarioPath, char const* const createPath);

extern bool dataFileTransform(char const* target, bool decryptSwitch);

#endif // PAL_DEFINE_H



#ifndef PAL_FUNCTION_H
#define PAL_FUNCTION_H

/**
 * @brief 导出封包文件到指定目录
 * @param targetArchivec
 * @param extractPath
 * @param dataFileDecrypt
 * @return 执行结果
*/
bool palPackgeExtract(char const* const targetArchive, char const* const extractPath);

/**
 * @brief 创建一个新的封包到指定目录
 * @param source 要导入封包的文件
 * @param createPackage 封包的创建地址
 * @param checkSumSwitch 是否写入校验和，默认false
 * @return 
*/
bool palPackageCreate(char const* const source, char const* const createPackage, bool checkSumSwitch = false);

/**
 * @brief 通过script.src和text.dat导出文本
 * @param source script.src和text.dat的所在路径
 * @param extractPath 导出文本的生成路径
 * @return 执行结果
*/
bool palStringExtract(char const* const source, char const* const extractPath);

/**
 * @brief 借由原script.src与text.dat将文本导入，并生成新的script.src与text.dat
 * @param source 源script.src与text.dat所在路径
 * @param scenarioPath 文本路径
 * @param createPath 导入后script.src与text.dat的生成路径
 * @return 执行结果
*/
bool palStringImport(char const* const dataPath, char const* const scenarioPath, char const* const createPath);

/**
 * @brief 加密或解密单个文件
 * @param target 目标文件
 * @param decryptSwitch 加解密标志,true为解密,false为加密
 * @return 执行结果
*/
bool dataFileTransform(char const* target, bool decryptSwitch);

#endif // PAL_FUNCTION_H

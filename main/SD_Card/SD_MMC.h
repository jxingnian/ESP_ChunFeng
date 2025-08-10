
#pragma once  // 防止头文件被多次包含

// ===================== 头文件包含 =====================
// 字符串操作相关头文件
#include <string.h>
// POSIX文件系统相关头文件（如文件操作、目录操作等）
#include <sys/unistd.h>
#include <sys/stat.h>
// ESP-IDF FAT文件系统虚拟文件系统适配层
#include "esp_vfs_fat.h"
// 目录操作相关头文件
#include "dirent.h"
// SDMMC协议命令头文件
#include "sdmmc_cmd.h"
// SDMMC主机驱动头文件
#include "driver/sdmmc_host.h"
// ESP-IDF日志输出头文件
#include "esp_log.h" 
// 错误码定义头文件
#include <errno.h>
// ESP-IDF闪存操作头文件
#include "esp_flash.h"    

// ===================== SD卡引脚宏定义 =====================
// SD卡时钟引脚（CLK）
#define CONFIG_EXAMPLE_PIN_CLK  14
// SD卡命令引脚（CMD）
#define CONFIG_EXAMPLE_PIN_CMD  17
// SD卡数据0引脚（D0）
#define CONFIG_EXAMPLE_PIN_D0   16
// SD卡数据1引脚（D1），未使用时设为-1
#define CONFIG_EXAMPLE_PIN_D1   -1
// SD卡数据2引脚（D2），未使用时设为-1
#define CONFIG_EXAMPLE_PIN_D2   -1
// SD卡数据3引脚（D3），未使用时设为-1
#define CONFIG_EXAMPLE_PIN_D3   -1  
// 备用SD卡D3引脚定义（如需切换引脚时使用）
#define CONFIG_SD_Card_D3       21  

#ifdef __cplusplus
extern "C" {
#endif  

// ===================== SD卡片选控制函数声明 =====================

/**
 * @brief 使能SD卡片选（CS）信号
 * 
 * @return esp_err_t 返回ESP-IDF标准错误码
 */
esp_err_t SD_Card_CS_EN(void);

/**
 * @brief 失能SD卡片选（CS）信号
 * 
 * @return esp_err_t 返回ESP-IDF标准错误码
 */
esp_err_t SD_Card_CS_Dis(void);

// ===================== SD卡文件操作函数声明 =====================

/**
 * @brief 向指定路径写入数据（示例函数）
 * 
 * @param path 文件路径
 * @param data 待写入的数据指针
 * @return esp_err_t 返回ESP-IDF标准错误码
 */
esp_err_t s_example_write_file(const char *path, char *data);

/**
 * @brief 从指定路径读取数据（示例函数）
 * 
 * @param path 文件路径
 * @return esp_err_t 返回ESP-IDF标准错误码
 */
esp_err_t s_example_read_file(const char *path);

// ===================== 全局变量声明 =====================

// SD卡容量（单位：字节），需在实现文件中定义
extern uint32_t SDCard_Size;
// Flash容量（单位：字节），需在实现文件中定义
extern uint32_t Flash_Size;

// ===================== SD卡与Flash初始化及文件操作接口声明 =====================

/**
 * @brief SD卡初始化
 * 
 * 初始化SD卡主机、挂载文件系统等，需在主程序启动时调用
 */
void SD_Init(void);

/**
 * @brief Flash存储空间扫描
 * 
 * 用于检测Flash内的文件或空间信息
 */
void Flash_Searching(void);

/**
 * @brief 打开指定路径的文件
 * 
 * @param file_path 文件路径
 * @return FILE* 文件指针，失败返回NULL
 */
FILE* Open_File(const char *file_path);

/**
 * @brief 检索指定目录下指定扩展名的文件
 * 
 * @param directory     目录路径
 * @param fileExtension 文件扩展名（如".mp3"）
 * @param File_Name     文件名存储数组（二维字符数组）
 * @param maxFiles      最大检索文件数
 * @return uint16_t     实际检索到的文件数量
 */
uint16_t Folder_retrieval(const char* directory, const char* fileExtension, char File_Name[][100], uint16_t maxFiles);

#ifdef __cplusplus
}
#endif

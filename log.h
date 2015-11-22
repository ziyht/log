/* 简单日志系统
 * 此模块用于简单输出日志到屏幕和写入到文件
 *
 *
*/

#include <stdio.h>      // FILE
#include <stdbool.h>    //
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef LOG_H
#define LOG_H

#define LOG_ERR 0
#define LOG_OK  1


#define DF_LOG_DIR      "./logs/"                // 默认的日志文件存放地点

#define NMUTE false
#define MUTE  true
typedef struct Log{
    char* name;         // 本日志的名称, 每次输出的时候都会附带, 以区分不同的日志信息
    char* path;         // 存储日志文件的位置
    FILE* fp;           // 文件流指针, 指向存储日志的本地文件
    size_t maxsize;     // 最大文件大小, 默认为 0, 表示不设限制
    bool mutetype;      // 静默属性, 决定在添加日志时是否显示到控制台上
}* LogPtr;

/* ------------------------------- logsys API ------------------------------------*/
#define LOGSYS_PATH     "./logs/sys.out"
#define LOGSYS_SIZE     1                       // 系统日志大小, 默认为 1 M

static LogPtr _sys = NULL;                      // 系统日志结构指针
static bool _logsys_service = false;                  // 系统日志初始化状态, 只有为 true , 以下 SET API 才有效
static bool LOGSYS_MUTETYPE = MUTE;

int  logsysInit();
void logsysStop();
void logsysSetMutetype(bool mutetype) {if(_logsys_service) _sys->mutetype = mutetype; LOGSYS_MUTETYPE = mutetype;}
void logsysSetFileSize(size_t size_mb) {if(_logsys_service)  _sys->maxsize = size_mb<<20;}

void logsysAdd(LogPtr log, const char* text, ...);
void logsysAddMute(LogPtr log, const char* text, ...);
void logsysAddNMute(LogPtr log, const char* text, ...);

/* ------------------------------- API ------------------------------------*/
// 独立日志输出
void logShowTime();                         // 打印当前时间到控制台, 自动添加空格
void logShowText(const char* text, ...);    // 打印 内容 到控制台, 不添加任何东西, 和 printf 相同的功能
void logShow(const char* text, ...);        // 打印时间和内容到控制台

// 非独立, 需要定义的日志结构体
LogPtr logCreate(const char* name, const char* path, bool mutetype);    // 创建一个 日志 结构
void   logDestroy(LogPtr log);                          // 销毁一个 日志 结构
size_t logFileSize(LogPtr log);                         // 获取日志结构所指文件的大小
int    logSetFileSize(LogPtr log, size_t size_mb);      // 设置文件大小限制, 单位为 MB
void   logSetMutetype(LogPtr log, bool mutetype);       // 设置日志结构的 静默 属性
int    logFlieEmpty(LogPtr log);                        // 清空结构所指日志文件

void logAddTime(LogPtr log);                            // 添加当前时间到 日志 中, 由 (*log).mute 决定是否静默处理
void logAddTimeMute(LogPtr log);                        // 添加当前时间到 日志 中, 强制静默处理
void logAddTimeNMute(LogPtr log);                       // 添加当前时间到 日志 中, 强制非静默处理
void logAddText(LogPtr log, const char* text, ...);     // 添加 text 到 日志 中, 由 (*log).mute 决定是否静默处理
void logAddTextMute(LogPtr log, const char* text, ...); // 添加 text 到 日志 中, 强制静默处理
void logAddTextNMute(LogPtr log, const char* text, ...);// 添加 text 到 日志 中, 强制非静默处理
void logAdd(LogPtr log, const char* text, ...);         // 添加 时间 和 text 到 日志中, 由 (*log).mute 决定是否静默处理
void logAddMute(LogPtr log, const char* text, ...);     // 添加 时间 和 text 到 日志中, 强制静默处理
void logAddNMute(LogPtr log, const char* text, ...);    // 添加 时间 和 text 到 日志中, 强制非静默处理

/* ------------------------------- Test Function ------------------------------------*/
void logTest();

#endif

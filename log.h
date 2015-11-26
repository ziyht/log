/*  简单日志系统 v1.0
 *  此模块用于简单输出日志到屏幕和写入到文件
 *  此模块包含两个系统:
 *      1. 内部日志系统, 此系统用于记录各用户日志的操作过程, 如创建, 添加日志, 销毁, 清空文件等等
 *      2. 用户日志系统, 此系统只记录用户自己添加的日志信息
 *  使用:
 *      1. 内部日志系统:
 *          使用 logsysInit() 进行初始化, 使用 logsysStop() 停止
 *          其它相关设置请查看 logsys API , 但不建议用户使用 logsysAdd()相关函数;
 *          注意: 内部日志系统 用不用均可
 *      2. 用户日志系统:
 *          2.1 使用 独立日志输出API, 输出日志到屏幕, 此部分API只负责输出, 不进行任何记录
 *          2.2 使用 创建及修改相关 API 创建和修改 相关日志结构
 *              然后使用 logAdd* 添加日志
 *
 * 编者语: 考虑到使用起来不太方便, 添加日志必须知道具体的 日志 结构, 这意味着在函数中使用时 要么传入一个日志结构,
 * 要么新建一个日志结构, 而新建的日志结构和原先的必然不一样, 所以最好的方法是传入一个日志结构, 这样无疑所有
 * 要使用日志系统的函数都要新加一个参数, 更加极端的, 如果要操作多个日志, 那么要新增多个参数, 这无疑对原来的
 * 工程项目有很大的影响, 所以此日志系统有必要更新, 使用新的模式.
 *
 * 新模式设想:
 *      1. 使用内部日志系统维护 用户的日志结构, 使用日志的名称来区分维护的用户日志
 *      2. 添加日志时, 提供名称, 那么日志系统就知道该往哪个日志里面写
 *      3. 因为只需要知道名称, 就可以写入日志, 这意味着不需要添加参数来传递日志结构, 能很好地嵌入到没使用日志系统的项目中
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


#define DF_LOG_DIR      "./logs/"              // 默认的日志文件存放地点
#define DF_LOG_SIZE    100                     // 默认日志文件大小 100 M

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
// 独立日志输出API, 这部分直接输出到 控制台, 不和任何文件关联
void logShowTime();                         // 打印当前时间到控制台, 自动添加空格
void logShowText(const char* text, ...);    // 打印 内容 到控制台, 不添加任何东西, 和 printf 相同的功能
void logShow(const char* text, ...);        // 打印时间和内容到控制台

// 创建及修改相关API
LogPtr logCreate(const char* name, const char* path, bool mutetype);    // 创建一个 日志 结构
void   logDestroy(LogPtr log);                          // 销毁一个 日志 结构
size_t logFileSize(LogPtr log);                         // 获取日志结构所指文件的大小
int    logSetFileSize(LogPtr log, size_t size_mb);      // 设置文件大小限制, 单位为 MB
void   logSetMutetype(LogPtr log, bool mutetype);       // 设置日志结构的 静默 属性
int    logFlieEmpty(LogPtr log);                        // 清空结构所指日志文件

// 日志添加API
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

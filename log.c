#include "log.h"

/* -------------------------- private prototypes ---------------------------- */
#define TS_LOG  0
#define TS_FILE 1
static char* _timeStr(int type);                                    // 返回一个存储当前本地时间的静态字符串指针

typedef int status;
#define FILE_NOTEXIST   0
#define FILE_NOTWRITE   2
#define FILE_CANWRITE   1
static status _GetFileStatus(const char* path);                     // 获取文件状态

#define MAX_PATH_LENGTH     255
static char* _logPath(const char* dir, const char* name);           // 获取一个临时的 path 字串, 不要 free

static void _mkdir(const char* path, mode_t mode);                  // 根据路径依次创建文件夹, 直到文件的最底层
static void _logFileShrink(LogPtr log);                                    // 若 日志文件 已达上限, 则清空文件

/* ------------------------------- SYS API ------------------------------------*/
/**
 * @brief logsysInit - 初始化内部日志系统, 可执行可不执行
 * 若执行, 所有的日志创建销毁操作都会记录到默认的日志文件中, 通过 LOGSYS_PATH 查看具体文件
 * 若不执行, 则不记录相关操作
 */
int logsysInit()
{
    if(_logsys_service)
        return LOG_OK;

    _sys = logCreate(NULL, LOGSYS_PATH, LOGSYS_MUTETYPE);
    if(!_sys)
    {
        logShow("log system init err!\n");
        return LOG_ERR;
    }
    _sys->maxsize = LOGSYS_SIZE << 20;      // 设置内部日志文件最大限制, 默认为 1 MB
    _logsys_service = true;

    logsysAdd(NULL, "--------------- log system init ok! ------------------\n");

    return LOG_OK;
}

/**
 * @brief logsysStop - 停止内部日志系统
 */
void logsysStop()
{
    if(!_logsys_service)  return;

    logsysAdd(NULL, "log system Stoped!\n");
    logDestroy(_sys);
    _sys = NULL;
    _logsys_service = false;
}

/**
 * @brief logsysAdd - 添加日志到系统日志
 * @param log   现在操作的日志, 主要是为了得到 log->name, 用以区分, 可以为 NULL
 * @param text  内容
 */
void logsysAdd(LogPtr log, const char* text, ...)
{
    if(!_logsys_service || !_sys || !text || !(*text))  return;

    _logFileShrink(_sys);

    va_list argptr;
    va_start(argptr, text);

    /* 写入日志到 系统日志 中 */
    fprintf(_sys->fp, "%s", _timeStr(TS_LOG));
    if(log && log->name)
    {
        fprintf(_sys->fp, "[%s] ", log->name);
    }
    vfprintf(_sys->fp, text, argptr);
    fflush(_sys->fp);

    /* 输出日志到 控制台 中 */
    if(!_sys->mutetype)
    {
        va_start(argptr, text);
        fprintf(stderr, "%s", _timeStr(TS_LOG));
        if(log && log->name)
        {
            fprintf(stderr, "[%s] ", log->name);
        }
        vfprintf(stderr, text, argptr);
    }

    va_end(argptr);
}

/**
 * @brief logsysAdd - 添加日志到系统日志, 静默模式
 * @param log   现在操作的日志, 主要是为了得到 log->name, 用以区分, 可以为 NULL
 * @param text  内容
 */
void logsysAddMute(LogPtr log, const char* text, ...)
{
    if(!_logsys_service || !_sys || !text || !(*text))  return;

    _logFileShrink(_sys);

    va_list argptr;
    va_start(argptr, text);

    /* 写入日志到 系统日志 中 */
    fprintf(_sys->fp, "%s", _timeStr(TS_LOG));
    if(log && log->name)
    {
        fprintf(_sys->fp, "[%s] ", log->name);
    }
    vfprintf(_sys->fp, text, argptr);
    fflush(_sys->fp);

    va_end(argptr);
}

/**
 * @brief logsysAdd - 添加日志到系统日志, 非静默模式
 * @param log   现在操作的日志, 主要是为了得到 log->name, 用以区分, 可以为 NULL
 * @param text  内容
 */
void logsysAddNMute(LogPtr log, const char* text, ...)
{
    if(!_logsys_service || !_sys || !text || !(*text))  return;

    _logFileShrink(_sys);

    va_list argptr;
    va_start(argptr, text);

    /* 写入日志到 系统日志 中 */
    fprintf(_sys->fp, "%s", _timeStr(TS_LOG));
    if(log && log->name)
    {
        fprintf(_sys->fp, "[%s] ", log->name);
    }
    vfprintf(_sys->fp, text, argptr);
    fflush(_sys->fp);

    /* 输出日志到 控制台 中 */
    va_start(argptr, text);
    fprintf(stderr, "%s", _timeStr(TS_LOG));
    if(log && log->name)
    {
        fprintf(stderr, "[%s] ", log->name);
    }
    vfprintf(stderr, text, argptr);

    va_end(argptr);
}


/* ----------------------------- API implementation ------------------------- */

/**
 * @brief logShowTime - 输出时间信息到控制台 "[%02d-%02d-%02d %02d:%02d:%02d] "
 */
void logShowTime()
{
    fprintf(stderr, "%s", _timeStr(TS_LOG));
}

/**
 * @brief logShowText - 显示 text 到控制台
 * @param text
 */
void logShowText(const char* text, ...)
{
    if(!text || !(*text)) return;

    va_list argptr;
    va_start(argptr, text);

    vfprintf(stderr, text, argptr);

    va_end(argptr);
}

/**
 * @brief logShow - 输出 时间 和 text 到控制台
 * @param text
 */
void logShow(const char* text, ...)     // 打印时间和内容到控制台
{
    if(!text || !(*text)) return;

    va_list argptr;
    va_start(argptr, text);

    logShowTime();
    vfprintf(stderr, text, argptr);

    va_end(argptr);
}


static void _logReset(LogPtr log)
{
    if(log->name)   free(log->name);
    if(log->path)   free(log->path);
    if(log->fp)     fclose(log->fp);
    bzero(log, sizeof(*log));
}

/**
 * @brief _logInit - 根据传入参数初始化 log 结构体
 * @param log       要初始化的log结构体指针
 * @param name      名称
 * @param path      路径
 * @param mutetype  静默模式
 * @return 成功返回 LOG_OK; 失败返回 LOG_ERR
 * @note   如果返回 LOG_OK, 说明 路径一定合法, 并且已成功打开
 */
static int _logInit(LogPtr log, const char* name, const char* path, bool mutetype)
{
    if(name && *name) log->name = strdup(name);
    if(path && *path)
    {
        _mkdir(path, 0755);
        log->path     = strdup(path);
        log->fp       = fopen(log->path, "a+");
        log->maxsize  = DF_LOG_SIZE << 20;          // 默认日志文件大小 DF_LOG_SIZE MB
        log->mutetype = mutetype;
    }
    if(!path || !log->fp)
    {
        logsysAddNMute(log, "%s(%d)-%s:%s\n", __FILE__, __LINE__, __FUNCTION__ ,strerror(errno));
        return LOG_ERR;
    }

    return LOG_OK;
}

/**
 * @brief logCreate - 创建一个日志结构
 * @param name      日志名称, 若输出到控制台, 则通过名称进行区分
 * @param path      文件路径, 日志信息会存到此处; 若不可读写, 会在 DF_LOG_DIR 下创建一个临时文件
 * @param mutetype  所创建日志的静默属性
 * @return 创建的日志结构指针, 若失败, 则返回 NULL
 * @note   只要返回值不为 NULL, 那么 path 和 fp 肯定不是 NULL
 */
LogPtr logCreate(const char* name, const char* path, bool mutetype)
{
    LogPtr r_log = (LogPtr)calloc(sizeof(*r_log), 1);
    status f_s = _GetFileStatus(path);
    logsysAdd(NULL, "[%s] Creating...\n", name);
    switch(f_s)
    {
        case FILE_CANWRITE:
        case FILE_NOTEXIST:
            /* 使用指定的文件路径初始化, 如果成功, 跳出返回  */
            if(LOG_OK == _logInit(r_log, name, path, mutetype))
                break;
            /* 如果失败, 重置log, 继续执行 FILE_NOTWRITE 分支 */
            _logReset(r_log);
        case FILE_NOTWRITE:
            /* 在程序目录下产生临时文件进行初始化, 如果失败, 那么销毁 log, 返回 null  */
            logsysAdd(NULL, "[%s] \"%s\" cannot write, try to create a temp file\n", name, path);
            if(LOG_ERR == _logInit(r_log, name, _logPath(DF_LOG_DIR, name), mutetype))
            {
                logsysAdd(NULL, "[%s] Create err: cannot create file \"%s\"\n", name, r_log->path);
                logDestroy(r_log);
                return r_log = NULL;
            }
    }
    logsysAdd(r_log, "Create ok, link file: \"%s\"\n", r_log->path);
    return r_log;
}

/**
 * @brief logDestroy - 销毁一个日志结构, 释放内存
 * @param log   要销毁的日志结构指针
 */
void logDestroy(LogPtr log)
{
    if(!log)    return ;
    logsysAdd(log, "log destroied\n");
    _logReset(log);

    free(log);
}

/**
 * @brief logFileSize - 获取日志文件大小
 * @param log
 * @return 大小, 单位为字节
 */
size_t logFileSize(LogPtr log)
{
    if(!log->fp) return -1;
    fseek(log->fp, 0, SEEK_END);
    return ftell(log->fp);
}

/**
 * @brief logSetFileSize - 设置日志文件大小限制
 * @param log
 * @param size_mb   大小, 单位为 MB
 * @return 失败返回 -1, 成功返回设置后的大小
 */
int logSetFileSize(LogPtr log, size_t size_mb)
{
    if(!log || size_mb > INT_MAX>>20)   return -1;
    log->maxsize = size_mb << 20;
    logsysAdd(log, "set file size to %d \n", log->maxsize);
    return log->maxsize;
}

/**
 * @brief logSetMutetype - 设置日志的静默属性
 * @param log
 * @param mutetype
 */
void logSetMutetype(LogPtr log, bool mutetype)
{
    log->mutetype = mutetype;
    if(mutetype)
        logsysAdd(log, "set mutetype to MUTE \n");
    else
        logsysAdd(log, "set mutetype to NMUTE \n");
}

/**
 * @brief logFlieEmpty  - 清空日志结构所指文件
 * @param log
 * @return
 */
int logFlieEmpty(LogPtr log)
{
    if(!log->fp)   return -1;

    int fd = fileno(log->fp);
    fd = ftruncate(fd, 0);

    rewind(log->fp);

    logsysAdd(log, "Log file had been truncated \n");

    return fd;
}

/**
 * @brief logAddTime - 添加当前时间到 日志 中, 由 (*log).mute 决定是否静默处理
 * @param log
 */
void logAddTime(LogPtr log)
{
    if(!log)    return;

    _logFileShrink(log);

    if(log->fp)
        fprintf(log->fp, "%s", _timeStr(TS_LOG));
    if(!log->mutetype)
        fprintf(stderr, "%s", _timeStr(TS_LOG));
    logsysAdd(log, "add time\n");
}

/**
 * @brief logAddTimeMute - 添加当前时间到 日志 中, 强制静默处理
 * @param log
 */
void logAddTimeMute(LogPtr log)
{
    if(!log)    return;

    _logFileShrink(log);

    if(log->fp)
        fprintf(log->fp, "%s", _timeStr(TS_LOG));
    logsysAdd(log, "add time\n");
}

/**
 * @brief logAddTimeNMute - 添加当前时间到 日志 中, 强制非静默处理
 * @param log
 */
void logAddTimeNMute(LogPtr log)
{
    if(!log)    return;

    _logFileShrink(log);

    if(log->fp)
        fprintf(log->fp, "%s", _timeStr(TS_LOG));
    fprintf(stderr, "%s", _timeStr(TS_LOG));
    logsysAdd(log, "add time\n");
}

/**
 * @brief logAddText - 添加 text 到 日志 中, 由 (*log).mute 决定是否静默处理
 * @param log
 * @param text  内容
 */
void logAddText(LogPtr log, const char* text, ...)
{
    if(!log || !text || !(*text)) return;

    _logFileShrink(log);

    va_list argptr;
    va_start(argptr, text);

    if(log->fp)
        vfprintf(log->fp, text, argptr);
    if(!log->mutetype)
        vfprintf(stderr, text, argptr);
    logsysAdd(log, "add a text \n");

    va_end(argptr);
}
/**
 * @brief logAddTextMute - 添加 text 到 日志 中, 强制静默处理
 * @param log
 * @param text
 */
void logAddTextMute(LogPtr log, const char* text, ...)
{
    if(!log || !text || !(*text)) return;

    _logFileShrink(log);

    va_list argptr;
    va_start(argptr, text);

    if(log->fp)
        vfprintf(log->fp, text, argptr);
    logsysAdd(log, "add a text \n");

    va_end(argptr);
}
/**
 * @brief logAddTextNMute - 添加 text 到 日志 中, 强制非静默处理
 * @param log
 * @param text
 */
void logAddTextNMute(LogPtr log, const char* text, ...)
{
    if(!log || !text || !(*text)) return;

    _logFileShrink(log);

    va_list argptr;
    va_start(argptr, text);

    if(log->fp)
        vfprintf(log->fp, text, argptr);
    vfprintf(stderr, text, argptr);
    logsysAdd(log, "add a text \n");

    va_end(argptr);
}


/**
 * @brief logAdd - 添加 时间 和 text 到 日志中, 由 (*log).mute 决定是否静默处理
 * @param log
 * @param text
 */
void logAdd(LogPtr log, const char* text, ...)
{
    if(!log || !text || !(*text))   return;

    _logFileShrink(log);

    va_list argptr;
    va_start(argptr, text);

    fprintf(log->fp, "%s", _timeStr(TS_LOG));
    vfprintf(log->fp, text, argptr);

    if(!log->mutetype)
    {
        fprintf(stderr, "%s", _timeStr(TS_LOG));
        if(log->name)   fprintf(stderr, "[%s] :", log->name);
        vfprintf(stderr, text, argptr);
    }
    logsysAdd(log, "add a log\n");

    va_end(argptr);
}
void logAddMute(LogPtr log, const char* text, ...)     // 添加 时间 和 text 到 日志中, 强制静默处理
{
    if(!log || !text || !(*text))   return;

    _logFileShrink(log);

    va_list argptr;
    va_start(argptr, text);


    fprintf(log->fp, "%s", _timeStr(TS_LOG));
    vfprintf(log->fp, text, argptr);

    logsysAdd(log, "add a log\n");

    va_end(argptr);
}
/**
 * @brief logAddNMute - 添加 时间 和 text 到 日志中, 强制非静默处理
 * @param log
 * @param text
 */
void logAddNMute(LogPtr log, const char* text, ...)
{
    if(!log || !text || !(*text))   return;

    _logFileShrink(log);

    va_list argptr;
    va_start(argptr, text);

    // 添加到文件流中
    fprintf(log->fp, "%s", _timeStr(TS_LOG));
    vfprintf(log->fp, text, argptr);

    // 输出到控制台
    fprintf(stderr, "%s", _timeStr(TS_LOG));
    if(log->name)   fprintf(stderr, "[%s] :", log->name);
    vfprintf(stderr, text, argptr);

    logsysAdd(log, "add a log\n");

    va_end(argptr);
}

/* ------------------------- private functions ------------------------------ */

static status _GetFileStatus(const char* path)
{
    if (access(path, F_OK) == 0)       // 如果文件存在
    {
        if (access(path, W_OK) != 0)    // 如果不可写
        {
            return FILE_NOTWRITE;
        }
        return FILE_CANWRITE;
    }
    // 文件不存在
    return FILE_NOTEXIST;
}

/**
 * 返回一个存储当前本地时间的静态字符串指针
 * @param  type  根据 type 返回不同形式的字串
 * @return char* 指向静态区的字符串指针
 * 注意, 不可 free
*/
char* _timeStr(int type)
{
    // static char* timestr = (char*)calloc(30, 1); // C 不支持
    static char timestr[30];    // 因为不大, 所以直接放到 栈 中
    static struct tm *local;
    static time_t t;

    memset(timestr, 0, 30);
    t = time(NULL);         // 获取日历时间
    local = localtime(&t);  // 将日历时间转化为本地时间，并保存在 struct tm 结构中
    switch(type)
    {
        case TS_LOG:
            sprintf(timestr, "[%02d-%02d-%02d %02d:%02d:%02d] ",local->tm_year+1900, local->tm_mon, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
            break;
        case TS_FILE:
            sprintf(timestr, "-%02d%02d%02d%02d%02d%02d.out",local->tm_year+1900, local->tm_mon, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
            break;
    }
    return timestr;
}

/**
 * 根据传入的 dir 和 filename 获取 path
 * @param  dir      文件夹路径
 * @param  filename 文件名
 * @return 指向一个存储路径的字符串, 不要 free
*/
static char* _logPath(const char* dir, const char* name)
{
    static char* r_path = NULL;
    if(!r_path)
        r_path = (char*)malloc(MAX_PATH_LENGTH + 1);
    bzero(r_path, MAX_PATH_LENGTH + 1);
    if(dir)
        strcat(r_path, dir);
    if(name)
        strcat(r_path, name);
    strcat(r_path, _timeStr(TS_FILE));
    return r_path;
}

/**
 * @brief _mkdir - 根据路径依次创建文件夹, 直到文件的最底层
 * @param path  路径, 文件夹或文件路径均可
 * @param mode  权限, 推荐 0755
 */
void _mkdir(const char* path, mode_t mode)
{
    char* head = strdup(path);
    char* tail = head;

    while(*tail)
    {
        if('/' == *tail)
        {
            *tail = '\0';
            if(mkdir(head, mode))
                logsysAdd(NULL, "%s(%d)-[mkdir]: \"%s\" %s\n", __FILE__, __LINE__, head, strerror(errno));
            else
                logsysAdd(NULL, "mkdir \"%s\" ok\n", head);
            *tail = '/';
        }
        tail++;
    }

    free(head);
}

/**
 * @brief _logFileShrink - 若 日志文件 已达上限, 则清空文件
 * @param log
 */
void _logFileShrink(LogPtr log)
{
    if(0 != log->maxsize && logFileSize(log) > log->maxsize)
    {
        logsysAdd(log, "Test to reach the upper file limitation ~!, Empty file...\n");
        logFlieEmpty(log);
    }
}

/* ------------------------- private functions ------------------------------ */
#ifdef TESTMODE

void logTest()
{
    logShow("------- logShowAPI test ------\n");
    logShowTime();
    logShowText("[logShowText]\n");
    logShow("[logShow] This is a test logshow\n");
    logShow("[logShow] Get a temp file path: %s\n\n", _logPath(DF_LOG_DIR, "test"));

    logShow("----- logsysAPI test -----\n");
    logsysInit();
    logsysAddNMute(NULL, "logsysAddNMute\n");
    logsysAddMute(NULL, "logsysAddMute\n");
    logsysSetMutetype(NMUTE);
    logsysAdd(NULL, "logsysAdd\n");
    printf("%u\n", (unsigned int)logFileSize(_sys));
//    int i = 0;
//    for(i = 0; i < 10000; i++)
//        logsysAdd(NULL, "1234567890\n");
    printf("%u\n", (unsigned int)logFileSize(_sys));
    logsysStop();


    logShow("----- logAPI test -----\n");
    logsysSetMutetype(MUTE);
    logsysInit();

    LogPtr test_log_nmute = logCreate("a_nomute_test_Log", "./test_log_nmute.out", NMUTE);
    LogPtr test_log_mute = logCreate("a_mute_test_Log", "./test_log_mute.out", MUTE);

    logAdd(test_log_nmute, "test_log_nmute[out ok]\n");
    logAdd(test_log_mute, "test_log_mute[out not ok]\n");

    logSetMutetype(test_log_nmute, MUTE);
    logSetMutetype(test_log_mute, NMUTE);

    logAdd(test_log_nmute, "test_log_nmute[out not ok]\n");
    logAdd(test_log_mute, "test_log_mute[out ok]\n");

    logAddNMute(test_log_nmute,"test_log_nmute in logAddNMute[out ok]\n");

    logDestroy(test_log_nmute);
    logDestroy(test_log_mute);




}

#endif


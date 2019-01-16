/*************************************************
 ** Copyright(c) 2019, luzeya
 ** All rights reserved
 **
 ** 文件名：watchdog.c
 ** 创建人：zeya
 ** 创建日期： 2019-01-16
 **
 ** 描  述：用来监视不同进程，并在指定时间间隔内检查该进程是否启动
 **************************************************/
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define LOG(fmt, args...)                                           \
    fprintf(stderr, "[%s, %d] " fmt, __FILE__, __LINE__, ##args)

/* 监视的最大子进程数 */
#define MAX_PROCESS (128)

/* 返回值定义 */
enum {
    SUCCESS = 0,
    ERROR_ARGS = -1,
    ERROR_SYSTEM = -2,
};

/**
 * 参数变量
 */
/* 程序死亡后默认间隔2s重启 */
static int gInterval = 2;

/* 监控进程的pid数组以及指针 */
static pid_t       gProcPid[MAX_PROCESS] = {0};
static const char *gProcCmd[MAX_PROCESS] = {0};
static char **     gProcArg[MAX_PROCESS] = {0};
static int         gProcPointer = -1;

/* 程序环境 */
extern char **environ;

/* 声明 */
void freearg();
pid_t exec(const char *path, char *args[]);

/**
 * 在子进程的pid数组中找到对应项
 * @return 返回-1, 找不到；返回>=0的数字，即索引
 */
int findpid(pid_t pid)
{
    int i = 0;
    for (i = 0; i <= gProcPointer; i++) {
        if (gProcPid[i] == pid) {
            return i;
        }
    }
    return 0;
}

/**
 * SIGCHLD信号处理函数，避免僵尸进程出现
 * 并非实时信号，在一些情况下可能会丢信号而出现僵尸进程
 */
void waitsig(int signum)
{
    pid_t pid = 0;
    int wstatus = 0;
    int index = 0;

    if (signum != SIGCHLD) {
        return;
    }

    pid = waitpid(-1, &wstatus, 0);
    if (pid < 0) {
        LOG("waitpid error: %s\n", strerror(errno));
        return;
    }
    LOG("current parent pid: %d, dead child pid: %d\n", getpid(), pid);


    sleep(gInterval);
    /* 这里重启进程 */
    index = findpid(pid);
    if (index < 0) {
        return;
    }

    pid = exec(gProcCmd[index], gProcArg[index]);
    if (pid < 0) {
        return;
    }

    gProcPid[index] = pid;
    LOG("new create process pid: %d\n", pid);
}

/**
 * 当接收到停止的信号时，杀死子进程
 */
void killsig(int signum)
{
    int i = 0;

    if (signum != SIGINT) {
        return;
    }
    LOG("receive SIGINT....\n");

    signal(SIGCHLD, SIG_IGN);
    /* 杀掉子进程 */
    for (i = 0; i <= gProcPointer; i++) {
        if (kill(gProcPid[i], SIGKILL) < 0) {
            LOG("kill pid (%d) error: %s\n", gProcPid[i], strerror(errno));
        }
    }
    freearg();
    exit(0);
}

/**
 * 创建子进程并执行命令
 * @return 返回大于0的值为创建的子进程pid
 *         返回小与0的值为错误码
 */
pid_t exec(const char *path, char *args[])
{
    pid_t ret = fork();

    if (ret < 0) {
        /* fork error */
        LOG("fork: %s\n", strerror(errno));
        return ERROR_SYSTEM;
    } else if (ret == 0) {
        /* child */
        ret = execve(path, args, environ);
        LOG("execve: %s(%s)\n", path, strerror(errno));
        freearg();
        exit((ret == 0) ? 0 : ERROR_SYSTEM);
    } else {
        /* father */
        return ret;
    }
}

/**
 * 获得pointer的长度
 */
int getargnum(char **pointer)
{
    int i = 0;
    char **p = NULL;

    if (!pointer) {
        return 0;
    }

    for (i = 0, p = pointer; *p != NULL; i++, p++) {
        /* LOG("%d, %s\n", i, *p); */
    }
    return i;
}

/**
 * pointer即真正的参数，以NULL结尾
 */
char **addarg(char **pointer, char *arg)
{
    int num = getargnum(pointer);
    char **newpointer = NULL;

    if (num == 0) {
        free(pointer);
        pointer = malloc(sizeof(char*) + sizeof(NULL));
        if (!pointer) {
            return NULL;
        }
        pointer[0] = arg;
        pointer[1] = NULL;
        return pointer;
    }

    /* 申请，复制，释放 */
    newpointer = malloc(sizeof(char*) * (num + 1) + sizeof(NULL));
    if (!newpointer) {
        return NULL;
    }
    memcpy(newpointer, pointer, sizeof(char*) * num);
    free(pointer);

    newpointer[num] = arg;
    newpointer[num + 1] = NULL;
    return newpointer;
}

void freearg()
{
    int i = 0;
    for (i = 0; i <= gProcPointer; i++) {
        free(gProcArg[i]);
    }
}

/**
 * 解析参数并启动进程
 */
int main(int argc, char *argv[])
{
    int opt = 0;

    signal(SIGINT, killsig);
    signal(SIGCHLD, waitsig);

    while ((opt = getopt(argc, argv, "a:e:d")) != -1) {
        /* LOG("opt: %c\n", opt); */
        switch (opt) {
        case 'a':
            /* LOG("%s\n", optarg); */
            gProcPointer++;

            if (gProcPointer >= MAX_PROCESS){
                break;
            }

            gProcCmd[gProcPointer] = optarg;
            gProcArg[gProcPointer] = addarg(gProcArg[gProcPointer], optarg);
            break;
        case 'e':
            /* 执行命令的一个参数 */
            /* LOG("%s\n", optarg); */
            if (gProcPointer >= MAX_PROCESS){
                break;
            }
            gProcArg[gProcPointer] = addarg(gProcArg[gProcPointer], optarg);
            break;
        case 'd':
            daemon(1, 1);
            break;
        case 's':
            gInterval = atoi(optarg);
            break;
        default:
            LOG("help: \n");
            LOG("watchdog -a 'ls' -a 'sleep 500'\n");
            freearg();
            exit(ERROR_ARGS);
        }
    }

    /* 打印所有进程参数 */
    int i = 0, j = 0;
    char **p = NULL;
    for (i = 0; i <= gProcPointer; i++) {
        LOG("Proc%d: %s\n", i, gProcCmd[i]);
        for (j = 0, p = gProcArg[i]; *p != NULL; p++, j++) {
            LOG("Arg%d: %s\n", j, *p);
        }
    }

    /* 启动进程 */
    for (i = 0; i <= gProcPointer; i++) {
        pid_t pid = exec(gProcCmd[i], gProcArg[i]);

        gProcPid[i] = pid;
        LOG("exec process pid: %d\n", pid);
    }

    /* 无限循环 */
    for (;;) {
        sleep(10);
    }
    return 0;
}
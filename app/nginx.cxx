
//整个程序入口函数放这里

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h> 
#include <errno.h>
#include <arpa/inet.h>

#include "ngx_macro.h"   //各种宏定义
#include "ngx_func.h"    //各种函数声明
#include "ngx_c_conf.h"  //和配置文件处理相关的类,名字带c_表示和类有关
#include "ngx_c_socket.h"
#include "ngx_comm.h"
#include "ngx_c_memory.h"
#include "ngx_c_threadpool.h"


//本文件用的函数声明
static void freeresource();

//和设置标题有关的全局量
size_t  g_argvneedmem=0;        //保存下这些argv参数所需要的内存大小
size_t  g_envneedmem=0;         //环境变量所占内存大小
int     g_os_argc;              //参数个数 
char    **g_os_argv;            //原始命令行参数数组,在main中会被赋值
char    *gp_envmem=NULL;        //指向自己分配的env环境变量的内存，在ngx_init_setproctitle()函数中会被分配内存
int     g_daemonized=0;         //守护进程标记，标记是否启用了守护进程模式，0：未启用，1：启用了

//socket相关
CSocekt g_socket;               //socket全局对象

CThreadPool  g_threadpool;      //线程池全局对象
//和进程本身有关的全局量
pid_t   ngx_pid;                //当前进程的pid
pid_t   ngx_parent;             //父进程的pid
int     ngx_process;            //进程类型，比如master,worker进程等

sig_atomic_t  ngx_reap;         //标记子进程状态变化[一般是子进程发来SIGCHLD信号表示退出],sig_atomic_t:系统定义的类型：访问或改变这些变量需要在计算机的一条指令内完成
                                   //一般等价于int【通常情况下，int类型的变量通常是原子访问的，也可以认为 sig_atomic_t就是int类型的数据】

int main(int argc, char *const *argv)
{             
    //time_t mytime = time(NULL);
    //printf("time = %u",mytime);
    //exit(0);
    //#ifdef _POSIX_THREADS 
    //    printf("henhao");
    //#endif    
    //exit(0);
    //printf("unsigned long sizeof=%d",sizeof(unsigned long));
    //printf("htonl(100)=%d",htonl(100));
    //printf("ntohl(htonl(100)=%d",ntohl(htonl(100)));
    //exit(0);
    int exitcode = 0;
    int i;

    // CMemory *p_memory;

    //1. 不需要释放的最先调用
    //取得进程pid
    ngx_pid = getpid();
    ngx_parent = getppid();
    
    //统计argv所占的内存
    g_argvneedmem = 0;
    for(i = 0; i < argc; ++i)
    {
        g_argvneedmem += strlen(argv[i]) + 1; //还要计算\0
    }

    for(int i = 0; environ[i]; ++i)
    {
        g_envneedmem += strlen(environ[i]) + 1;
    }
    //在程序中所有涉及到argv的地方都用全局变量
    g_os_argv = (char **) argv;
    g_os_argc = argc;

     //全局量有必要初始化的
    ngx_log.fd = -1;                  //-1：表示日志文件尚未打开；因为后边ngx_log_stderr要用所以这里先给-1
    ngx_process = NGX_PROCESS_MASTER; //先标记本进程是master进程
    ngx_reap = 0;                     //标记子进程没有发生变化
   

    // -------------------------------------------------------
    //2. 初始化失败, 就要直接退出的代码
    //在main中，先把配置读出来，供后续使用,不要在子线程中首次调用GetInstance
    CConfig *p_config = CConfig::GetInstance(); //单例类
    //把项目根目录下的配置文件加载到内存
    if(p_config->Load("nginx.conf") == false) 
    {
        ngx_log_init(); //初始化日志
        ngx_log_stderr(0, "配置文件[%s]载入失败, 退出!", "nginx.conf");
        /// exit(1); //终止进程, 在main中出现和return的效果一样, exit(1)(-1)表示异常退出
        exitcode = 2;
        goto lblexit;
    }

    //(2.1)内存单例类可以在这里初始化，返回值不用保存
    CMemory::GetInstance();	

    // -------------------------------------------------------
    //3. 一些初始化函数
    //创建/打开日志文件
    ngx_log_init();
    //信号初始化函数
    if(ngx_init_signals() != 0)
    {
        exitcode = 1;
        goto lblexit;
    }


    if(g_socket.Initialize() == false)//初始化socket
    {
        exitcode = 1;
        goto lblexit;
    }

    // -------------------------------------------------------
    //4. 不好归类的代码
    ngx_init_setproctitle();    //把环境变量移走
    // ngx_setproctitle("nginx master"); //修改主进程为nginx master的工作在ngx_master_process_cycle()中进行


    // -------------------------------------------------------
    //5. 创建守护进程
    if(p_config->GetIntDefault("Daemon", 0) == 1) //读配置文件，拿到配置文件中是否按守护进程方式启动的选项, 默认是不以守护进程启动
    {
        //1：按守护进程方式运行
        int cdaemonresult = ngx_daemon();
        if(cdaemonresult == -1) //fork()失败
        {
            exitcode = 1;    //标记失败
            goto lblexit;
        }
        //2. 返回1代表父进程
        if(cdaemonresult == 1)
        {
            //这是原始的父进程
            freeresource();   //只有进程退出了才goto到 lblexit，用于提醒用户进程退出了
                              //而现在这个情况属于正常fork()守护进程后的正常退出，不应该跑到lblexit()去执行，因为那里有一条打印语句标记整个进程的退出，这里不该限制该条打印语句；
            exitcode = 0;
            return exitcode;
        }
        //走到这里，成功创建了守护进程并且这里已经是fork()出来的进程，现在这个进程做了master进程
        g_daemonized = 1;    //守护进程标记，标记是否启用了守护进程模式，0：未启用，1：启用了
    }

    //(7)开始正式的主工作流程，主流程一致在下边这个函数里循环，暂时不会走下来，资源释放啥的日后再慢慢完善和考虑  
    ngx_master_process_cycle();

    // for(;;)
    // {
    //    sleep(1); //休息1秒
    //    printf("休息1秒\n");
    // }



lblexit:
    ngx_log_stderr(0,"程序退出，再见了!");
    freeresource();
    return exitcode;
}


//专门在程序执行末尾释放资源的函数【一系列的main返回前的释放动作函数】
void freeresource()
{
    //(1)对于因为设置可执行程序标题导致的环境变量分配的内存，我们应该释放
    if(gp_envmem)
    {
        delete []gp_envmem;
        gp_envmem = NULL;
    }

    //(2)关闭日志文件
    //如果日志文件 != -1 同时 不指向标准错误
    if(ngx_log.fd != STDERR_FILENO && ngx_log.fd != -1)  
    {        
        close(ngx_log.fd); //不用判断结果了
        ngx_log.fd = -1; //标记下，防止被再次close        
    }
}




#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "ngx_c_conf.h"  //和配置文件处理相关的类,名字带c_表示和类有关
#include "ngx_func.h"    //各种函数声明

//和设置标题有关的全局量
size_t g_argvneedmem = 0; //保存argv参数所需要的内存大小
size_t g_envneedmem = 0; //环境变量所占用的内存大小
char **g_os_argv;            //原始命令行参数数组,在main中会被赋值
char *gp_envmem = NULL;      //指向自己分配的env环境变量的内存
int g_os_argc;


//和进程本身有关的全局量
pid_t ngx_pid;               //当前进程的pid
pid_t ngx_parent;            //父进程的pid
//本文件用的函数声明
static void freeresource();
int main(int argc, char *const *argv)
{             
    int exitcode = 0;
    int i;

    //1. 不需要释放的最先调用
    //取得进程pid
    ngx_pid = getpid();
    ngx_parent = getppid();
    
    //统计argv所占的内存
    g_argvneedmem = 0;
    for(i = 0; i < argc; ++i)
    {
        g_argvneedmem += strlen(argv[i] + 1); //还要计算\0
    }

    for(int i = 0; environ[i]; ++ i)
    {
        g_envneedmem += strlen(environ[i] + 1);
    }
    //在程序中所有涉及到argv的地方都用全局变量
    g_os_argv = (char **) argv;
    g_os_argc = argc;

    // -------------------------------------------------------
    //2. 初始化失败, 就要直接退出的代码
    //在main中，先把配置读出来，供后续使用,不要在子线程中首次调用GetInstance
    CConfig *p_config = CConfig::GetInstance(); //单例类
    //把项目根目录下的配置文件加载到内存
    if(p_config->Load("nginx.conf") == false) 
    {
        ngx_log_stderr(0, "配置文件[%s]载入失败, 退出!", "nginx.conf");
        /// exit(1); //终止进程, 在main中出现和return的效果一样, exit(1)(-1)表示异常退出
        exitcode = 2;
        goto lblexit;
    }

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


    // -------------------------------------------------------
    //4. 不好归类的代码
    ngx_init_setproctitle();    //把环境变量移走
    // ngx_setproctitle("nginx master"); //修改主进程为nginx master的工作在ngx_master_process_cycle()中进行


    // -------------------------------------------------------
    //5. 开始正式工作的主流程
    //不管父进程还是子进程, 正常工作都在这个函数中循环,不会往下面走了
    ngx_master_process_cycle();

    // for(;;)
    // {
    //    sleep(1); //休息1秒
    //    printf("休息1秒\n");
    // }

    //对于因为设置可执行程序标题导致的环境变量分配的内存，我们应该释放
    if(gp_envmem)
    {
        delete []gp_envmem;
        gp_envmem = NULL;
    }

lblexit:
    freeresource();
    printf("程序退出，再见!\n");
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




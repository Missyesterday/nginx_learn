#include <stdio.h>
#include <unistd.h>
#include <signal.h>

int main(int argc, char* const* argv)
{
    printf("你好, 世界!\n");
    
    //忽略SIGHUP信号,设置某个信号来的时候处理程序。
    //SIG_IGN: 要求忽略这个信号，请操作系统不要用缺省的方式对待本进程（不要杀掉我）
//    signal(SIGHUP, SIG_IGN);
    //新建立一个不同的session，但是进程组组长调用setid()是无效的
    pid_t pid;
    pid = fork(); //创建一个新进程
    //子进程从fork开始执行
    if(pid < 0)
    {
        printf("fork()进程出错\n");
    }

    else if(pid == 0)
    {
        //子进程
        setsid();
        for(;;)
        {
        printf("子进程休息一秒\n");
        sleep(1);
        }
    }
    else if(pid > 0)
    {
        //父进程
        setsid();//父进程的setsid()无效
        for(;;)
        {

        printf("父进程休息一秒\n");
        sleep(1);
        }
        
    }

    setsid();

    for (;;)
    {
        sleep(1);
        printf("休息一秒\n");
    }
    
    printf("程序退出,再见 !");
    return 0;

}

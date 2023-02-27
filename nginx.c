#include <stdio.h>
#include <unistd.h>
#include <signal.h>

int main(int argc, char* const* argv)
{
    printf("你好, 世界!\n");
    
    //忽略SIGHUP信号,设置某个信号来的时候处理程序。
    //SIG_IGN: 要求忽略这个信号，请操作系统不要用缺省的方式对待本进程（不要杀掉我）
    signal(SIGHUP, SIG_IGN);
    for (;;)
    {
        sleep(1);
        printf("休息一秒\n");
    }
    
    printf("程序退出,再见 !");
    return 0;

}

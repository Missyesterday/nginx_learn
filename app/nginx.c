#include <stdio.h>
#include <unistd.h>

//头文件路径,已经使用gcc -I参数指定了
#include "ngx_func.h"
#include "ngx_signal.h"

int main(int argc, char* const* argv)
{

    printf("hello, world\n");
    myconf();
    mysignal();

    printf("程序结束\n");
    return 0;
}

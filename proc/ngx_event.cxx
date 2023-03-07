﻿//和开启子进程相关
/*
王健伟老师 《Linux C++通讯架构实战》
商业级质量的代码，完整的项目，帮你提薪至少10K
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>   //信号相关头文件 
#include <errno.h>    //errno
#include <unistd.h>

#include "ngx_func.h"
#include "ngx_macro.h"
#include "ngx_c_conf.h"

//处理网络事件和定时器事件，我们遵照nginx引入这个同名函数
void ngx_process_events_and_timers()
{
    g_socket.ngx_epoll_process_events(-1); //-1表示卡着等待把

    //...再完善
}


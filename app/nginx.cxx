#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "ngx_c_conf.h"  //和配置文件处理相关的类,名字带c_表示和类有关
#include "ngx_signal.h"
#include "ngx_func.h"    //各种函数声明

//和设置标题有关的全局量
char **g_os_argv;            //原始命令行参数数组,在main中会被赋值
char *gp_envmem = NULL;      //指向自己分配的env环境变量的内存
int  g_environlen = 0;       //环境变量所占内存大小

int main(int argc, char *const *argv)
{             
    //在程序中所有涉及到argv的地方都用全局变量
    g_os_argv = (char **) argv;
    ngx_init_setproctitle();    //把环境变量移走, 应该尽早调用
    ngx_setproctitle("nginx master"); //修改主进程为nginx master
    //我们在main中，先把配置读出来，供后续使用,不要在子线程中首次调用GetInstance
    CConfig *p_config = CConfig::GetInstance(); //单例类
    
    //把项目根目录下的配置文件加载到内存
    if(p_config->Load("nginx.conf") == false) 
    {
        printf("配置文件载入失败，退出!\n");
        exit(1);
    }
    
       
    // 测试argv和environ的连续
    // for(int i = 0; i < argc; ++i)
    // {        
    //    printf("argv[%d]地址=%x    " ,i,(unsigned int)((unsigned long)argv[i]));
    //    printf("argv[%d]内容=%s\n",i,argv[i]);
    // }
    // //下边环境变量随便打两个
    // for(int i = 0; i < 2; ++i)
    // {
    //    printf("evriron[%d]地址=%x    " ,i, (unsigned int)((unsigned long)environ[i]));
    //    printf("evriron[%d]内容=%s\n" ,i,environ[i]);
    // }

    // //证明argv后面就是environ
     /* points to environment, in Un*x */
	// if ( (argv + argc + 1) == environ ) 
	// {
	// 	printf("========== ptr equals environ ==========\n\n");
	// }

    //测试, 打印一些配置项信息  
    // int port = p_config->GetIntDefault("ListenPort", 0); //0是缺省值
    // printf("port=%d\n",port);
    // const char *pDBInfo = p_config->GetString("DBInfo");
    // if(pDBInfo != NULL)
    // {
    //   printf("DBInfo=%s\n",pDBInfo);
    // }
    

    
    for(;;)
    {
       sleep(1); //休息1秒
       printf("休息1秒\n");
    }

    //对于因为设置可执行程序标题导致的环境变量分配的内存，我们应该释放
    if(gp_envmem)
    {
        delete []gp_envmem;
        gp_envmem = NULL;
    }
    printf("程序退出，再见!\n");
    return 0;
}



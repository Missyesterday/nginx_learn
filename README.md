通过学习nginx源码，将其中的模块添加到自己的代码中，并详细注释。

源码下载：[https://nginx.org/download/nginx-1.14.2.tar.gz](https://nginx.org/download/nginx-1.14.2.tar.gz)


源码在`src/`目录下，使用VSCode查看源码。

**nginx源码入口函数：**

`src/core/nginx.c`

## 项目的目录规划

本项目的规划如下：

-   `_include/`：专门存放各种头文件，如果分散的话，引用头文件还需要不同的目录前缀
-   `app/`：放主应用程序.c（main函数所在文件）以及一些比较核心的文件，`app/`下还有：
    -   `link_obj/`：临时目录，存放临时的`.o`文件，这个目录不是手工创建，由makefile创建
    -   `dep/`：临时目录，存放`.d`的依赖文件，依赖文件能够告知系统哪些相关的文件发生变化，需要重新编译，也是由makefile创建
    -   `nginx.c`：主文件，`main()`入口函数
    -   `nginx_conf.c`：普通的源码文件，与`nginx.c`关系密切
-   `misc/`：专门存放不好归类的`.c`文件
-   `net/`：专门存放和网络处理相关的`.c`文件
-   `proc/`：专门存放和进程处理的相关`.c`文件
-   `signal/`：专门存放和信号处理相关的`.c`文件



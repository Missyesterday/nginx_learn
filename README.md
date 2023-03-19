# 仿照nginx的高性能服务器项目

仿照nginx实现的一个高性能服务器项目

源码下载：[https://nginx.org/download/nginx-1.14.2.tar.gz](https://nginx.org/download/nginx-1.14.2.tar.gz)


nginx的源码在`src/`目录下，可以使用VSCode查看源码。

**nginx源码入口函数：**

`src/core/nginx.c`

## IO模型
![](pic/%E9%A1%B9%E7%9B%AEIO%E6%A8%A1%E5%9E%8B.pdf)

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

## makefile说明
-   根目录下三个文件：
    -   `makefile`：编译项目的入口脚本，编译项目从这里开始，起总体控制作用
    -   `config.mk`：配置脚本，被makefile包含，单独分离出来有利于修改，有变动的东西都可以写到这里
    -   `common.mk`：核心的编译脚本，用来定义makefile的编译规则，依赖规则等，通用性很强，每个子目录的`makefile`都用到了这个脚本
-   每个子目录都有一个`makefile`文件，每个文件都会包含根目录下的`common.mk`
-   本项目的`makefile`不支持目录中还有子目录，只支持一级目录。


# 仿照nginx的服务器项目

通过学习nginx源码，将其中的模块添加到自己的代码中，并详细注释。

源码下载：[https://nginx.org/download/nginx-1.14.2.tar.gz](https://nginx.org/download/nginx-1.14.2.tar.gz)


nginx的源码在`src/`目录下，可以使用VSCode查看源码。

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

## makefile说明
-   根目录下三个文件：
    -   `makefile`：编译项目的入口脚本，编译项目从这里开始，起总体控制作用
    -   `config.mk`：配置脚本，被makefile包含，单独分离出来有利于修改，有变动的东西都可以写到这里
    -   `common.mk`：核心的编译脚本，用来定义makefile的编译规则，依赖规则等，通用性很强，每个子目录的`makefile`都用到了这个脚本
-   每个子目录都有一个`makefile`文件，每个文件都会包含根目录下的`common.mk`
-   本项目的`makefile`不支持目录中还有子目录，只支持一级目录。

## 1. 读取配置文件
1.  规定以`#`开始的为注释行，
2.  规定用`=`分割配置项与配置信息
3.  `[`开头表示组信息，对于程序而言等价于注释行

## 2. 设置进程的标题

本项目也仿照nginx，设置了一个主进程和多个子进程。我们要设置主进程和子进程名，区分主进程和子进程，否则都叫`nginx`。

参照eginx源码的`src/os/unix/ngx_setproctitle.c`，实现这个功能。

1.   重新分配一块内存保存环境变量`environ`，用`environ`指向这块内存，原来的就算被修改也无所谓了。
2.   再修改`argv[0]`所指向的内存，所以后面的参数是有可能覆盖的，但是无所谓，只要在修改前使用这些参数就行了
  

## 3. 创建worker子进程

官方的nginx中，一个master中，创建了多个worker进程。master进程被`sigsuspend()`阻塞, worker进程才是真正干活的.

本项目在`proc/ngx_process_cycle.cxx/ngx_master_process_cycle()`函数中，做了如下事情：

1.   调用`ngx_setproctitle()`设置主进程的标题
2.   创建worker子进程`ngx_start_worker_processes()`
3.   父进程继续在这个函数执行，子进程不会继续执行该函数！
4.   二者都使用了信号量和信号处理函数

>   也就是说, 父进程的主要流程在`ngx_master_procsess_cycle()`的循环中, 子进程的流程在`ngx_worker_process_cycle()`的循环中.


## 4. 网络通讯之开启监听端口

`Initialize()`调用`ngx_open_listening_sockets()`, 后者用来开启端口，在创建worker进程之前就要执行这个函数。（为什么？，这样work进程就都会监听80和443端口）

## 5. 网络通讯之epoll代码

在`net/ngx_c_sockt`中定义`CSocket`类，围绕`CSocket`类来编写代码.

### 5.1 连接池

有两个非常重要的结构体, `ngx_listening_t`和`ngx_connection_s`,分别用来存储 **与监听端口相关的结构**和 **一个TCP连接的信息**. 初始化的时候，需要把连接池作为「既是一个链表，又是一个数组」的结构。把空闲的元素连接成链表（初始化的时候全是空闲的），然后在需要的时候，只需要从链表中取出第一个元素，然后「头指针」指向下一个元素即可。始终保持链表中全是空闲的元素，这样在连接池中找一个空闲的连接就非常快。回收的时候，把回收的元素放到链表的最前面，同时改变「`m_pfree_connections`」（指向链表头节点的指针)的指向。

###  5.2 `ngx_epoll_init()`函数

-   创建epoll对象
-   创建连接池

在子进程初始化完成后调用.

### 5.3 `ngx_epoll_add_event()`
把感兴趣的事件通过`epoll_ctl()`添加到系统，当这些事件来的时候，系统会通知我们。



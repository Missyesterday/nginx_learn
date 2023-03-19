# 仿照nginx的高性能服务器项目

仿照nginx实现的一个高性能服务器项目

源码下载：[https://nginx.org/download/nginx-1.14.2.tar.gz](https://nginx.org/download/nginx-1.14.2.tar.gz)


nginx的源码在`src/`目录下，可以使用VSCode查看源码。

**nginx源码入口函数：**

`src/core/nginx.c`

## IO模型


![](pic/%E9%A1%B9%E7%9B%AEIO%E6%A8%A1%E5%9E%8B.svg)

使用同步IO模型模拟Proactor模式, 见《Linux高性能服务器编程》P129, 但对于发送数据, 进行了一定优化. 

工作流程如下:
1.  主线程添加监听socket的读事件
2.  主线程调用`epoll_wait()`函数等待事件发生
3.  当事件发生, `epoll_wait()`通知主线程, 主线程根据`epoll_wait()`的第二个参数和返回值循环处理事件:
    1.  如果是来自连接socket的读事件,则调用`accept4()`获取新的连接,并添加连接Socket的读事件
    2.  如果是来自监听Socket的读事件, 则`read()`一个数据包, 并唤醒一个工作线程, 根据数据包的包头决定执行的Handler函数, 如果业务需要发送数据, 就把数据交给发送消息队列, 并唤醒 ==专门用来发送数据的线程==. **这个线程只会发送一次数据, 如果没发完, 则把写事件添加到epoll, 剩下的数据发送交给epoll驱动!!**
    3.  如果是写事件, 则证明上次的数据没有发送完, 继续发送.

由于主线程负责数据的读和(一部分的)写, 工作线程仅仅负责业务逻辑, 不涉及IO操作,所以是Proactor模式.

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


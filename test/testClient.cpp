#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <iostream>

#include "ngx_c_crc32.h"


//结构定义------------------------------------
#pragma pack (1) //对齐方式,1字节对齐 
//一些和网络通讯相关的结构放在这里
typedef struct _COMM_PKG_HEADER
{
	unsigned short pkgLen;    //报文总长度【包头+包体】--2字节，2字节可以表示的最大数字为6万多，我们定义_PKG_MAX_LENGTH 30000，所以用pkgLen足够保存下
	unsigned short msgCode;   //消息类型代码--2字节，用于区别每个不同的命令【不同的消息】

	int            crc32;     //CRC32效验--4字节，为了防止收发数据中出现收到内容和发送内容不一致的情况，引入这个字段做一个基本的校验用	
}COMM_PKG_HEADER, *LPCOMM_PKG_HEADER;

typedef struct _STRUCT_REGISTER
{
	int           iType;          //类型
	char          username[56];   //用户名 
	char          password[40];   //密码

}STRUCT_REGISTER, *LPSTRUCT_REGISTER;


typedef struct _STRUCT_LOGIN
{
	char          username[56];   //用户名 
	char          password[40];   //密码

}STRUCT_LOGIN, *LPSTRUCT_LOGIN;

int  g_iLenPkgHeader = sizeof(COMM_PKG_HEADER);

//发送数据，值得讲解
int SendData(int sSocket, char *p_sendbuf, int ibuflen)
{
	int usend = ibuflen; //要发送的数目
	int uwrote = 0;      //已发送的数目
	int tmp_sret;

	while (uwrote < usend)
	{
		tmp_sret = send(sSocket, p_sendbuf + uwrote, usend - uwrote, 0);
		if ((tmp_sret <= 0) || (tmp_sret == 0))
		{
			//有错误发生了
            std::cout << "错误send" << std::endl;

			return -1;
		}
		uwrote += tmp_sret;
	}//end while
	return uwrote;
}

int main()
{
    //创建CRC32
    CCRC32   *p_crc32 = CCRC32::GetInstance();
    //创建套接字
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    //连接服务器
    //
    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &serveraddr.sin_addr.s_addr);
    serveraddr.sin_port = htons(80);
    int ret = connect(fd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if(ret == -1)
    {
        perror("connect");
        exit(-1);
    } 
    if(ret == 0)
    {
        std::cout << "连接成功" << std::endl;
    }
    //通信
    //写入数据
    char recvBuf[1024] = {0};
    int i = 0;
    while(1)
    {
        CCRC32   *p_crc32 = CCRC32::GetInstance();

        char *p_sendbuf = (char *)new char[g_iLenPkgHeader + sizeof(STRUCT_REGISTER)];

        LPCOMM_PKG_HEADER         pinfohead;
        pinfohead = (LPCOMM_PKG_HEADER)p_sendbuf;
        pinfohead->msgCode = 5;
        pinfohead->msgCode = htons(pinfohead->msgCode);
        //pinfohead->crc32 = htonl(123); //测试，所以随便来 一个
        pinfohead->pkgLen = htons(g_iLenPkgHeader + sizeof(STRUCT_REGISTER));

        LPSTRUCT_REGISTER pstruc_sendstruc = (LPSTRUCT_REGISTER)(p_sendbuf + g_iLenPkgHeader);
        pstruc_sendstruc->iType = htonl(100);
        strcpy(pstruc_sendstruc->username, "1234");
        strcpy(pstruc_sendstruc->password, "12354");

        //crc值需要最后算的
        pinfohead->crc32   = p_crc32->Get_CRC((unsigned char *)pstruc_sendstruc, sizeof(STRUCT_REGISTER));
        pinfohead->crc32   = htonl(pinfohead->crc32); //针对四字节数字，主机序转网络序
        if(SendData(fd, p_sendbuf, g_iLenPkgHeader + sizeof(STRUCT_REGISTER)) > 0)
            std::cout << "发送成功" << std::endl;
	


	    delete[] p_sendbuf; //释放上边的内存
        // sprintf(recvBuf, "data : %d\n", i++);
        // //把字符串的结束符带上
        // write(fd, recvBuf, strlen(recvBuf) + 1);

        //读取数据
        
    
        // int len = read(fd, recvBuf, sizeof(recvBuf));
        // if(len == -1) {
        //     perror("read");
        //     exit(-1);
        // } else if(len > 0) {
        //     printf("recv server : %s\n", recvBuf);
        // } else if(len == 0) {
        //     // 表示服务器端断开连接
        //     printf("server closed...");
        //     //客户端结束应该退出循环
        // }
        // sleep(1);

        
        //再发个登录命令
        p_sendbuf = (char *)new char[g_iLenPkgHeader + sizeof(STRUCT_LOGIN)];

        //LPCOMM_PKG_HEADER         pinfohead;
        pinfohead = (LPCOMM_PKG_HEADER)p_sendbuf;
        pinfohead->msgCode = 6;  //变化一下
        pinfohead->msgCode = htons(pinfohead->msgCode);
        //pinfohead->crc32 = htonl(345); //测试，所以随便来 一个
        pinfohead->pkgLen = htons(g_iLenPkgHeader + sizeof(STRUCT_LOGIN));

        LPSTRUCT_LOGIN pstruc_sendstruc2 = (LPSTRUCT_LOGIN)(p_sendbuf + g_iLenPkgHeader);
        strcpy(pstruc_sendstruc2->username, "5678");

        //crc值需要最后算的
        pinfohead->crc32 = p_crc32->Get_CRC((unsigned char *)pstruc_sendstruc2, sizeof(STRUCT_LOGIN));
        pinfohead->crc32 = htonl(pinfohead->crc32); //针对四字节数字，主机序转网络序

        if (SendData(fd, p_sendbuf, g_iLenPkgHeader + sizeof(STRUCT_LOGIN)) > 0)
        {
            std::cout << "发送成功" << std::endl;
        }
        sleep(1);
        close(fd);
        break;
    }
    return 0;
    
}

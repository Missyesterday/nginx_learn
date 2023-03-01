
//系统头文件放上面
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

//自定义头文件放下面,因为g++中用了-I参数，所以这里用<>扩起来也可以
#include "ngx_func.h"     //函数声明
#include "ngx_c_conf.h"   //和配置文件处理相关的类,名字带c_表示和类有关

//静态成员赋值
CConfig *CConfig::m_instance = NULL;

//构造函数
CConfig::CConfig()
{		
}

//析构函数
CConfig::~CConfig()
{    
    //删除迭代器中的内容
	std::vector<LPCConfItem>::iterator pos;	
	for(pos = m_ConfigItemList.begin(); pos != m_ConfigItemList.end(); ++pos)
	{		
		delete (*pos);
	}//end for
	m_ConfigItemList.clear(); 
}

//加载配置文件的函数
bool CConfig::Load(const char *pconfName) 
{   
    //首先根据传入的字符串打开文件
    FILE *fp;
    fp = fopen(pconfName,"r");
    if(fp == NULL)
        return false;

    //每一行配置文件读出来都放到linebuf
    char  linebuf[501];   //每行配置都不要太长，保持<500字符内，防止出现问题
    
    //走到这里，文件打开成功 
    while(!feof(fp))  //检查文件是否结束 ，没有结束则条件成立
    {    
        //注意代码的严密性
        //从文件中读数据，每次读一行，一行最多不要超过500个字符 
        if(fgets(linebuf,500,fp) == NULL) 
            continue;

        //读到空行
        if(linebuf[0] == 0)
            continue;

        //处理注释行, 也就是判断第一个字符是下面这些字符开头的
        //以及处理其他的情况,例如换行,空行等, 直接跳过
        if(*linebuf==';' || *linebuf==' ' || *linebuf=='#' || *linebuf=='\t'|| *linebuf=='\n')
			continue;
        
    //与goto语句一起使用
    lblprocstring:
        //读取的字符串后边有换行，回车，空格等都截取掉
		if(strlen(linebuf) > 0)
		{
            //ascii码的形势来判断
			if(linebuf[strlen(linebuf)-1] == 10 || linebuf[strlen(linebuf)-1] == 13 || linebuf[strlen(linebuf)-1] == 32) 
			{
				linebuf[strlen(linebuf)-1] = 0;
				goto lblprocstring;
			}		
		}
        //空行
        if(linebuf[0] == 0)
            continue;
        //也等价于配置行
        if(*linebuf=='[') //[开头的也不处理
			continue;

        // 类似于的“ListenPort = 5678”配置项走下来；
        //用 = 分割, 右边的保存到ptmp中
        char *ptmp = strchr(linebuf,'=');
        if(ptmp != NULL)
        {
            //LPConfItem是一个指向结构体的指针, CConfItem是结构体, 里面存储配置项名字ItemName和配置项内容ItemContent
            //new的时候是new结构
            LPCConfItem p_confitem = new CConfItem;                    //注意前边类型带LP，后边new这里的类型不带

            //清零
            memset(p_confitem,0,sizeof(CConfItem));
            //拷贝前n个字符
            strncpy(p_confitem->ItemName,linebuf,(int)(ptmp-linebuf)); //等号左侧的拷贝到p_confitem->ItemName
            strcpy(p_confitem->ItemContent,ptmp+1);                    //等号右侧的拷贝到p_confitem->ItemContent, 遇到'\0'结束

            //去掉首位的空格
            Rtrim(p_confitem->ItemName);
			Ltrim(p_confitem->ItemName);
			Rtrim(p_confitem->ItemContent);
			Ltrim(p_confitem->ItemContent);

            //printf("itemname=%s | itemcontent=%s\n",p_confitem->ItemName,p_confitem->ItemContent);            
            
            
            m_ConfigItemList.push_back(p_confitem);  //内存要释放，因为这里是new出来的 
        } //end if
    } //end while(!feof(fp)) 

    
    //关闭文件
    fclose(fp); 
    return true;
}

//根据ItemName获取配置信息字符串，没有修改不用考虑互斥
const char *CConfig::GetString(const char *p_itemname)
{
	std::vector<LPCConfItem>::iterator pos;	
	for(pos = m_ConfigItemList.begin(); pos != m_ConfigItemList.end(); ++pos)
	{	
		if(strcasecmp( (*pos)->ItemName,p_itemname) == 0)
			return (*pos)->ItemContent;
	}//end for
	return NULL;
}
//根据ItemName获取int类型配置信息，没有修改,所以不用考虑互斥
//def是缺省值, 如果没找到则返回缺省值
int CConfig::GetIntDefault(const char *p_itemname,const int def)
{
	std::vector<LPCConfItem>::iterator pos;	
	for(pos = m_ConfigItemList.begin(); pos !=m_ConfigItemList.end(); ++pos)
	{	
		if(strcasecmp( (*pos)->ItemName,p_itemname) == 0)
            //转为int类型
			return atoi((*pos)->ItemContent);
	}//end for
	return def;
}




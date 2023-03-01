
#ifndef __NGX_CONF_H__
#define __NGX_CONF_H__

#include <vector>

#include "ngx_global.h"  //一些全局/通用定义

//类名可以遵照用C开头的命名规范,代表Class
class CConfig
{

//单例设计模式
private:
	CConfig();
public:
	~CConfig();
private:
	static CConfig *m_instance;

public:	
	static CConfig* GetInstance() 
	{	
		if(m_instance == NULL)
		{
			//锁,多线程才用,第一次调用要在主线程中调用
            //这个代码可能不是那么好
			if(m_instance == NULL)
			{					
				m_instance = new CConfig();
                //定义一个静态的变量,这个变量是类中类型的，用来释放单例对象,而不是程序结束,系统自动释放
				static CGarhuishou cl; 
			}
			//放锁		
		}
		return m_instance;
	}	

	class CGarhuishou  //类中套类，用于释放单例对象
	{
	public:				
		~CGarhuishou()
		{
			if (CConfig::m_instance)
			{						
				delete CConfig::m_instance;
				CConfig::m_instance = NULL;				
			}
		}
	};
//---------------------------------------------------
public:
    //重要的成员函数
    bool Load(const char *pconfName); //装载配置文件
	const char *GetString(const char *p_itemname);
	int  GetIntDefault(const char *p_itemname,const int def);

public:
    //存储配置信息的列表, 里面存的是指针
	std::vector<LPCConfItem> m_ConfigItemList; 

};

#endif

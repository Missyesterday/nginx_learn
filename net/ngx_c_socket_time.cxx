
//和网络 中 时间 有关的函数放这里

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>    //uintptr_t
#include <stdarg.h>    //va_start....
#include <unistd.h>    //STDERR_FILENO等
#include <sys/time.h>  //gettimeofday
#include <time.h>      //localtime_r
#include <fcntl.h>     //open
#include <errno.h>     //errno
//#include <sys/socket.h>
#include <sys/ioctl.h> //ioctl
#include <arpa/inet.h>

#include "ngx_c_conf.h"
#include "ngx_macro.h"
#include "ngx_global.h"
#include "ngx_func.h"
#include "ngx_c_socket.h"
#include "ngx_c_memory.h"
#include "ngx_c_lockmutex.h"

//设置踢出时钟(向multimap表中增加内容)，用户三次握手成功连入，然后我们开启了踢人开关【Sock_WaitTimeEnable = 1】，那么本函数被调用；
void CSocket::AddToTimerQueue(lpngx_connection_t pConn)
{
    CMemory *p_memory = CMemory::GetInstance();

    //获取当前时间
    time_t futtime = time(NULL);
    futtime += m_iWaitTime;  //当前时间+20秒

    CLock lock(&m_timequeueMutex); //互斥，因为要操作m_timeQueuemap了
    LPSTRUC_MSG_HEADER tmpMsgHeader = (LPSTRUC_MSG_HEADER)p_memory->AllocMemory(m_iLenMsgHeader,false);
    tmpMsgHeader->pConn = pConn;
    tmpMsgHeader->iCurrsequence = pConn->iCurrsequence;
    m_timerQueuemap.insert(std::make_pair(futtime,tmpMsgHeader)); //按键 自动排序 小->大
    m_cur_size_++;  //计时队列尺寸+1

    //计时队列头部时间值保存到m_timer_value_里
    //GetEarliestTime()函数用来获取定时器队列的第一个元素, 保存在CSocket对象中, 有利于之后直接使用
    m_timer_value_ = GetEarliestTime();
    return;    
}

//从multimap中取得最早的时间返回去，调用者负责互斥，所以本函数不用互斥，调用者确保m_timeQueuemap中一定不为空
time_t CSocket::GetEarliestTime()
{
    std::multimap<time_t, LPSTRUC_MSG_HEADER>::iterator pos;	
	pos = m_timerQueuemap.begin();		
	return pos->first;	
}

//从m_timeQueuemap移除最早的时间，并把最早这个时间所在的项的值所对应的指针 返回，调用者负责互斥，所以本函数不用互斥，
LPSTRUC_MSG_HEADER CSocket::RemoveFirstTimer()
{
	std::multimap<time_t, LPSTRUC_MSG_HEADER>::iterator pos;	
	LPSTRUC_MSG_HEADER p_tmp;
	if(m_cur_size_ <= 0)
	{
		return NULL;
	}
	pos = m_timerQueuemap.begin(); //调用者负责互斥的，这里直接操作没问题的
	p_tmp = pos->second;
	m_timerQueuemap.erase(pos);
	--m_cur_size_;
	return p_tmp;
}

//根据给的当前时间，从m_timeQueuemap找到比这个时间更老（更早）的节点【1个】返回去，这些节点都是时间超过了，要处理的节点
//调用者负责互斥，所以本函数不用互斥
LPSTRUC_MSG_HEADER CSocket::GetOverTimeTimer(time_t cur_time)
{	
	CMemory *p_memory = CMemory::GetInstance();
	LPSTRUC_MSG_HEADER ptmp;

	if (m_cur_size_ == 0 || m_timerQueuemap.empty())
		return NULL; //队列为空

	time_t earliesttime = GetEarliestTime(); //到multimap中去查询
	if (earliesttime <= cur_time)
	{
		//这回确实是有到时间的了【超时的节点】
		ptmp = RemoveFirstTimer();    //把这个超时的节点从 m_timerQueuemap 删掉，并把这个节点的第二项返回来；

	    if(/*m_ifkickTimeCount == 1 && */m_ifTimeOutKick != 1)  //能调用到本函数第一个条件肯定成立，所以第一个条件加不加无所谓，主要是第二个条件
        //也就是说没有开启超时踢人选项 
		{
			//如果不是要求超时就提出，则才做这里的事：

			//因为下次超时的时间我们也依然要判断，所以还要把这个节点加回来        
			time_t newinqueutime = cur_time+(m_iWaitTime);
			LPSTRUC_MSG_HEADER tmpMsgHeader = (LPSTRUC_MSG_HEADER)p_memory->AllocMemory(sizeof(STRUC_MSG_HEADER),false);
			tmpMsgHeader->pConn = ptmp->pConn;
			tmpMsgHeader->iCurrsequence = ptmp->iCurrsequence;			
			m_timerQueuemap.insert(std::make_pair(newinqueutime,tmpMsgHeader)); //自动排序 小->大			
			m_cur_size_++;       
		}   

		if(m_cur_size_ > 0) //这个判断条件必要，因为以后我们可能在这里扩充别的代码
		{
            //始终保存需要取出的最小时间
			m_timer_value_ = GetEarliestTime(); //计时队列头部时间值保存到m_timer_value_里
		}
		return ptmp;
	}
	return NULL;
}

//把指定用户tcp连接从timer表中抠出去
void CSocket::DeleteFromTimerQueue(lpngx_connection_t pConn)
{
    std::multimap<time_t, LPSTRUC_MSG_HEADER>::iterator pos,posend;
	CMemory *p_memory = CMemory::GetInstance();

    CLock lock(&m_timequeueMutex);

    //删除时间队列中的元素
	pos    = m_timerQueuemap.begin();
	posend = m_timerQueuemap.end();
	for(; pos != posend; )	
	{
		if(pos->second->pConn == pConn)
		{			
			p_memory->FreeMemory(pos->second);  //释放内存
			pos = m_timerQueuemap.erase(pos);
			--m_cur_size_; //减去一个元素，必然要把尺寸减少1个;								
		}		
        else
        {
            pos++;
        }
	}
	if(m_cur_size_ > 0)
	{
        //只要消息队列中有变化, 都需要把时间队列的最早时间更新
		m_timer_value_ = GetEarliestTime();
	}
    return;    
}

//清理时间队列中所有内容
void CSocket::clearAllFromTimerQueue()
{	
	std::multimap<time_t, LPSTRUC_MSG_HEADER>::iterator pos,posend;

	CMemory *p_memory = CMemory::GetInstance();	
	pos    = m_timerQueuemap.begin();
	posend = m_timerQueuemap.end();    
	for(; pos != posend; ++pos)	
	{
		p_memory->FreeMemory(pos->second);		
		--m_cur_size_; 		
	}
	m_timerQueuemap.clear();
}

//时间队列监视和处理线程，处理到期不发心跳包的用户踢出的线程
void* CSocket::ServerTimerQueueMonitorThread(void* threadData)
{
    ThreadItem *pThread = static_cast<ThreadItem*>(threadData);
    CSocket *pSocketObj = pThread->_pThis;

    time_t absolute_time,cur_time;
    int err;

    while(g_stopEvent == 0) //代表整个程序不退出
    {
        //这里没互斥判断，所以只是个初级判断，目的至少是队列为空时避免系统损耗(一直加锁)
        //临界会增加性能消耗
		if(pSocketObj->m_cur_size_ > 0)//队列不为空，有内容吗?(没有临界, 所以不一定有数据!)
        {
            //都没有用互斥
			//时间队列中最近发生事情的时间放到 absolute_time里；
            absolute_time = pSocketObj->m_timer_value_; //节省一个互斥, m_timer_value_就是之前GetEarliestTime()的返回值
            
            //获取当前时间
            cur_time = time(NULL);

            if(absolute_time < cur_time)
            {
                //if为真代表, 时间到了，可以处理了(没有临界, 只是一种估计)
                std::list<LPSTRUC_MSG_HEADER> m_lsIdleList; //保存所有要处理的消息头
                LPSTRUC_MSG_HEADER result;

                //要到时间序列中取数据才真正临界, 前面的代码只是大概判断
                err = pthread_mutex_lock(&pSocketObj->m_timequeueMutex);  
                if(err != 0) ngx_log_stderr(err,"CSocket::ServerTimerQueueMonitorThread()中pthread_mutex_lock()失败，返回的错误码为%d!",err);//有问题，要及时报告
                while ((result = pSocketObj->GetOverTimeTimer(cur_time)) != NULL) //一次性的把所有超时节点都拿过来
				{
					m_lsIdleList.push_back(result); 
				}//end while
                
                //释放锁
                err = pthread_mutex_unlock(&pSocketObj->m_timequeueMutex); 
                if(err != 0)  ngx_log_stderr(err,"CSocket::ServerTimerQueueMonitorThread()pthread_mutex_unlock()失败，返回的错误码为%d!",err);//有问题，要及时报告 
                LPSTRUC_MSG_HEADER tmpmsg;

                //到心跳时间了
                while(!m_lsIdleList.empty())
                {
                    tmpmsg = m_lsIdleList.front();
					m_lsIdleList.pop_front(); 
                    pSocketObj->procPingTimeOutChecking(tmpmsg,cur_time); //这里需要检查心跳超时问题
                } //end while(!m_lsIdleList.empty())
            }
        } //end if(pSocketObj->m_cur_size_ > 0)
        
        usleep(500 * 1000); //为简化问题，直接每次休息500毫秒
    } //end while

    return (void*)0;
}

//心跳包检测时间到，该去检测心跳包是否超时的事宜，本函数只是把内存释放，子类应该重新事先该函数以实现具体的判断动作
//父类中是虚函数
void CSocket::procPingTimeOutChecking(LPSTRUC_MSG_HEADER tmpmsg,time_t cur_time)
{
	CMemory *p_memory = CMemory::GetInstance();
	p_memory->FreeMemory(tmpmsg);    
}



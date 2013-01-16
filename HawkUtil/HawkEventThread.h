#ifndef HAWK_EVENTTHREAD_H
#define HAWK_EVENTTHREAD_H

#include "HawkMutex.h"
#include "HawkThread.h"
#include "HawkSession.h"
#include "HawkSocketPair.h"
#include "HawkEventNotice.h"

namespace Hawk
{
	/************************************************************************/
	/* 基于事件的线程处理,主要用于事件管理器                                */
	/************************************************************************/
	class UTIL_API HawkEventThread : public HawkRefCounter
	{
	public:
		//构造
		HawkEventThread();

		//析构
		virtual ~HawkEventThread();

	public:
		//会话结构
		struct ESession
		{
			//投递的事件
			UInt32			 Flag;
			//会话读事件
			void*			 Event;
			//会话
			HawkSession*	 Session;
			//所在事件线程
			HawkEventThread* Owner;
		};

		//新事件队列
		typedef list<HawkEventNotice*>	NoticeList;
		//会话列表
		typedef map<SID, ESession*>		SessionMap;
		//会话缓冲池
		typedef list<ESession*>			SessionCache;
		
	public:
		//初始化
		virtual Bool  Init(UInt32 iBaseId);

		//运行
		virtual Bool  Run();			

		//关闭
		virtual Bool  Close();

	public:
		//检测会话ID存在
		virtual Bool  CheckSidIn(SID iSid) const;

		//通知事件
		virtual Bool  NotifyNotice(HawkEventNotice* pNotice);

		//获取线程ID
		virtual Int32 GetThreadId() const;

	protected:
		//取出通知
		virtual Bool  PopNotice(HawkEventNotice*& pNotice);

		//处理通知
		virtual Bool  ProcessNotice(HawkEventNotice* pNotice);

		//分配会话
		virtual Bool  AllocSession(ESession*& pSession);

		//缓存会话
		virtual Bool  CacheSession(ESession* pSession);		

		//释放会话列表
		virtual Bool  FreeSessionMap();

		//释放会话缓存
		virtual Bool  FreeSessionCache();

		//释放通知队列
		virtual Bool  FreeNoticeList();

		//生成会话ID
		virtual SID   GenSessionId();

	public:
		//线程执行函数, 线程函数调用
		virtual Bool  OnThreadLoop();

		//管道事件处理, 线程函数调用
		virtual Bool  OnPipeEvent();

		//会话事件处理, 线程函数调用
		virtual Bool  OnSessionEvent(UInt32 iEvent, ESession* pSession);

	protected:
		//开启服务器会话, iSocketType: TCP | UDP
		virtual Bool  StartServer(const SocketAddr& sAddr, UInt8 iSocketType, HawkSessionProxy* pProxy = 0);

		//开启客户端会话, iSocketType: TCP | UDP
		virtual Bool  StartClient(const SocketAddr& sAddr, UInt8 iSocketType, HawkSessionProxy* pProxy = 0);

		//开启连接会话
		virtual Bool  StartPeer(SOCKET hSocket, const SocketAddr& sAddr);

		//写会话数据
		virtual Bool  SendRawData(ESession* pSession, HawkOctets* pData, const SocketAddr* pAddr = 0);

		//建立会话事件
		virtual Bool  LaunchEvent(SID iSid, ESession* pSession);

		//更新会话事件
		virtual Bool  UpdateEvent(ESession* pSession, UInt32 iEvent);

	protected:
		//会话开启
		virtual Bool  OnSessionStart(SID iSid, ESession* pSession);

		//会话关闭
		virtual Bool  OnSessionClose(SID iSid);

	protected:
		//事件处理线程
		HawkThread*		m_pThread;
		//管道
		SocketPair		m_sPipe;
		//管道事件
		void*			m_pEvent;
		//事件基础对象
		void*			m_pEventBase;		
		//会话ID数组
		SID*			m_vSidInfo;
		//基础ID
		UInt32			m_iBaseSid;
		//会话索引
		UInt32			m_iCurSid;
		//当前会话列表
		SessionMap		m_mSession;
		//会话缓存
		SessionCache	m_vSessionCache;
		//新通知队列
		NoticeList		m_vNotice;
		//通知队列锁
		HawkSpinLock*	m_pNoticeLock;
		//通知互斥
		HawkMutex*		m_pNoticeMutex;
	};
}
#endif

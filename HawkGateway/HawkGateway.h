#ifndef HAWK_GATEWAY_H
#define HAWK_GATEWAY_H

#include "HawkGateDef.h"
#include "HawkGateThread.h"
#include "HawkProfiler.h"

namespace Hawk
{
	/************************************************************************/
	/* 网关服务器基本封装                                                   */
	/************************************************************************/
	class PRO_API HawkGateway : public HawkRefCounter
	{
	public:
		//构造
		HawkGateway();

		//析构
		virtual ~HawkGateway();

		//服务列表
		struct Service 
		{
			UInt32  SvrId;
			SOCKET  SvrFd;

			Service(UInt32 iSvrId = 0, SOCKET iSvrFd = INVALID_SOCKET) : SvrId(iSvrId), SvrFd(iSvrFd) {}
		};
		typedef vector<Service> ServiceVec;

	public:
		//初始化网关服务
		virtual Bool    Init(const AString& sFrontend, const AString& sBackend, Int32 iThread = 4, Bool bEchoMode = false);

		//开启服务
		virtual Bool    Run();

		//关闭服务(通知结束,不阻塞)
		virtual Bool    Stop();		

	public:
		//获取线程数
		virtual Int32   GetThreadNum() const;

		//获取线程ID
		virtual Int32   GetThreadId(Int32 iIdx) const;

		//设置会话缓冲大小
		virtual void    SetBufSize(Int32 iBufSize);

		//获取会话缓冲大小
		virtual Int32   GetBufSize() const;	

		//设置连接数限制
		virtual void    SetConnLimit(Int32 iLimit);

		//获取连接数限制
		virtual Int32   GetConnLimit() const;

		//注册连接数
		virtual Bool    RegConnection(Bool bConn);

		//设置会话超时时间
		virtual void    SetSessionTimeout(Int32 iTimeout);

		//获取会话超时时间
		virtual Int32   GetSessionTimeout() const;

		//开启性能监视器
		virtual Bool    TurnOnProfiler(const AString& sAddr);
	
	public:
		//创建网关线程
		virtual Bool    CreateGateThread(HawkGateThread*& pThread);

		//创建会话加密组件
		virtual Bool    CreateISecurity(HawkSecurity*& pSecurity);

		//创建会话加密组件
		virtual Bool    CreateOSecurity(HawkSecurity*& pSecurity);

		//获取性能监视器
		HawkProfiler*   GetProfiler();

		//获取线程ZMQ的服务地址
		virtual AString GetThreadZmqAddr() const;

		//获取后端服务监视ZMQ地址
		virtual AString GetServerZmqAddr() const;		

	protected:
		//接收网关线程格式化数据
		virtual Bool    RecvThreadMsg(UInt32& iThread, SID& iSid, OctetsStream* pOctets);		

		//向网关线程发送通知消息
		virtual Bool    SendThreadMsg(UInt32 iThread, SID iSid, void* pData, Size_t iSize);

		//接收后端服务格式化数据
		virtual Bool    RecvServerMsg(UInt32& iSvrId, SID& iSid, OctetsStream* pOctets);

		//向后端服务发送通知消息
		virtual Bool    SendServerMsg(UInt32 iSvrId, SID iSid, void* pData, Size_t iSize);

	protected:
		//接收新连接
		virtual Bool    OnSessionAccept();

		//ThreadZmq可读事件
		virtual Bool    OnGateThreadEvent();

		//ServerZmq可读事件
		virtual Bool    OnGateServerEvent();

		//MonitorZmq可读事件
		virtual Bool    OnGateMonitorEvent();

		//网关空闲状态处理
		virtual Bool    OnGateIdleEvent();

		//关闭服务(阻塞等待结束)
		virtual Bool    OnGatewayClose();

		//后端服务连接到网关
		virtual Bool    OnServerConnected(SOCKET hSocket);

		//后端服务和网关断开
		virtual Bool    OnServerDisConnect(SOCKET hSocket);

	protected:		
		//事件线程
		HawkGateThread** m_ppThread;
		//网关线程数
		Int32			 m_iThread;
		//轮流线程ID
		UInt32			 m_iTurnIdx;
		//会话Buffer大小
		Int32			 m_iBufSize;
		//会话超时时间
		Int32			 m_iTimeout;
		//连接数限制
		Int32			 m_iConnLimit;
		//当前连接数
		UInt32			 m_iConnCount;
		//消息数据Buffer
		OctetsStream*	 m_pOctets;
		//服务器套接字
		HawkSocket		 m_sSocket;
		//挂载的服务ID
		ServiceVec		 m_vSvrvice;
		//当前的服务ID
		UInt32			 m_iCurSvrId;
		//和网关线程通信的ZMQ
		HawkZmq*		 m_pThreadZmq;
		//和后端服务通信的ZMQ
		HawkZmq*		 m_pServerZmq;
		//后端服务ZMQ监视器
		HawkZmq*		 m_pMonitorZmq;
		//性能监视器
		HawkProfiler*	 m_pProfiler;
		//Echo模式
		Bool			 m_bEchoMode;
		//循环空闲状态
		Bool			 m_bIdle;
		//运行状态
		volatile Bool    m_bRunning;		
	};
}
#endif

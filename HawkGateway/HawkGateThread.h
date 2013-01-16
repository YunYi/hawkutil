#ifndef HAWK_GATETHREAD_H
#define HAWK_GATETHREAD_H

#include "HawkGateDef.h"

namespace Hawk
{
	/************************************************************************/
	/* 网关服务线程                                                         */
	/************************************************************************/
	class HawkGateway;
	class GATE_API HawkGateThread : public HawkRefCounter
	{
	public:
		//构造
		HawkGateThread(HawkGateway* pGateway = 0);

		//析构
		virtual ~HawkGateThread();

		friend class HawkGateway;

	public:
		//会话信息定义
		struct Session
		{
			//会话ID
			SID				Sid;
			//套接字
			SOCKET			Socket;
			//地址信息
			SocketAddr*		Addr;
			//会话读事件
			void*			Event;
			//所属线程对象
			HawkGateThread* GThread;
			//输入缓冲区
			OctetsStream*	IBuffer;
			//加密对象
			HawkSecurity*	ISecurity;
			//解密对象
			HawkSecurity*	OSecurity;
		};

		//会话列表定义
		typedef map<SID, Session*> SessionMap;

	public:
		//初始化
		virtual Bool  Init(UInt32 iBaseId);

		//运行
		virtual Bool  Start();			

		//关闭
		virtual Bool  Close();

		//获取线程ID
		virtual Int32 GetThreadId() const;

	protected:
		//分配会话
		Session*	  AllocSession(SOCKET hSocket, const SocketAddr& sAddr);

		//缓存会话
		virtual Bool  FreeSession(Session* pSession);		

		//释放会话列表
		virtual Bool  FreeSessionMap();

		//生成会话ID
		virtual SID   GenSessionId();

		//开启连接会话
		virtual Bool  StartSession(SOCKET hSocket, const SocketAddr& sAddr);

		//关闭会话
		virtual Bool  CloseSession(SID iSid);

		//接收网关数据消息
		virtual Bool  RecvGateMsg(SID& iSid, OctetsStream* pOctets);

		//给网关发送数据消息
		virtual Bool  SendGateMsg(SID iSid, void* pData, Size_t iSize);		

		//会话发送协议
		virtual Bool  SendProtocol(Session* pSession, Protocol* pProto);

	public:
		//线程执行函数, 线程函数调用
		virtual Bool  OnThreadLoop();

		//会话事件
		virtual Bool  OnSessionEvent(UInt32 iEvent, Session* pSession);

	protected:
		//消息队列事件
		virtual Bool  OnGatewayEvent();

		//空闲状态
		virtual Bool  OnThreadIdle();

		//会话数据可读回调
		virtual Bool  OnSessionRead(Session* pSession);
		
		//会话数据可写回调
		virtual Bool  OnSessionWrite(Session* pSession);

		//会话发生错误回调
		virtual Bool  OnSessionError(Session* pSession);

	protected:
		//会话输入缓冲区可解码判断
		virtual Bool  OnSessionDecode(Session* pSession, OctetsStream* pBuffer);

		//会话输出缓冲区可加码判断
		virtual Bool  OnSessionEncode(Session* pSession, OctetsStream* pBuffer);

	protected:
		//事件处理线程
		HawkThread*		m_pThread;
		//事件基础对象
		void*			m_pBase;
		//消息队列
		HawkZmq*		m_pZmq;
		//消息数据Buffer
		OctetsStream*	m_pOctets;
		//基础会话ID
		UInt32			m_iBaseSid;
		//当前会话索引
		UInt32			m_iCurSid;
		//当前会话列表
		SessionMap		m_mSession;
		//所属网关应用
		HawkGateway*	m_pGateway;
		//空闲状态
		Bool			m_bIdle;
		//运行中状态
		volatile Bool   m_bRunning;
	};
}
#endif

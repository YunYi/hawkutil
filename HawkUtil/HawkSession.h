#ifndef HAWK_SESSION_H
#define HAWK_SESSION_H

#include "HawkSecurity.h"
#include "HawkSessionProxy.h"

namespace Hawk
{
	/************************************************************************/
	/* 网络会话对象封装                                                     */
	/************************************************************************/
	class UTIL_API HawkSession : public HawkRefCounter
	{
	public:
		//构造
		HawkSession();
		
		//析构
		virtual ~HawkSession();

	public:
		//会话信息统计(非线程安全统计)
		struct Collect
		{
			UInt64  ConnCount;
			UInt64  RecvTimes;
			UInt64  RecvProtos;
			UInt64  RecvBytes;
			UInt64  SendTimes;
			UInt64  SendProtos;
			UInt64  SendBytes;

			void Reset()
			{
				memset(this, 0, sizeof(*this));
			}

			void OnAccept()
			{
				ConnCount ++;
			}

			void OnRecv(Size_t iSize)
			{
				RecvTimes ++;
				RecvBytes += iSize;
			}

			void OnSend(Size_t iSize)
			{
				SendTimes ++;
				SendBytes += iSize;
			}

			void OnRecvProto()
			{
				RecvProtos ++;
			}

			void OnSendProto()
			{
				SendProtos ++;
			}
		};

	public:
		//会话状态
		enum
		{
			STATE_CLOSED = 0,
			STATE_INITIALIZED,
			STATE_ACTIVATED,
			STATE_CLOSING,
		};

		//会话类型
		enum
		{
			TYPE_SERVER = 1,
			TYPE_CLIENT,
			TYPE_PEER,
		};

	public:	
		//UDP发送的消息报缓存
		struct UdpPacket
		{
			OctetsStream Data;
			SocketAddr   Addr;
		};

		//UDP包队列
		typedef list<UdpPacket> UdpPacketVec;

		//事件处理器可以使用Session的回调
		friend class HawkEventThread;

	public:
		//初始化, SessionType: (SERVER | CLIENT | PEER)
		virtual Bool	Init(const HawkSocket& sSocket, const SocketAddr& sAddr, UInt8 iSessionType, HawkSessionProxy* pProxy = 0);

		//关闭会话
		virtual Bool	Close();

		//是否有效
		virtual Bool    IsValid() const;	

		//设置会话ID
		virtual void	SetSid(SID iSid);

		//会话ID
		virtual SID		GetSid() const;	

		//设置状态
		virtual void	SetState(UInt32 iState);

		//获取状态
		virtual UInt32  GetState() const;			

		//会话是否为激活状态
		virtual Bool	IsActive() const;

		//会话是否为待关闭状态
		virtual Bool    IsClosing() const;

		//会话是否已关闭
		virtual Bool	IsClosed() const;

	public:
		//设置会话回调
		virtual void	SetSessionProxy(HawkSessionProxy* pProxy);

		//设置读入加密组件
		virtual void	SetISecurity(HawkSecurity* pSecurity);

		//设置写出加密组件
		virtual void	SetOSecurity(HawkSecurity* pSecurity);

		//获取会话类型
		virtual UInt8	GetSessionType() const;

		//获取套接字类型
		virtual UInt8	GetSocketType() const;

		//获取统计信息
		const Collect&	GetCollect() const;

		//获取会话读入加密组件
		HawkSecurity*	GetISecurity();

		//获取会话写出加密组件
		HawkSecurity*	GetOSecurity();	

		//获取输入Buffer
		OctetsStream*	GetInputBuf();

		//获取输出Buffer
		OctetsStream*	GetOutputBuf();

		//获取会话Socket
		HawkSocket*		GetSocket();

		//获取会话地址
		SocketAddr		GetAddress();

		//获取句柄
		SOCKET			GetHandle();

	public:
		//发送原始数据
		virtual Bool	SendRawData(void* pData, Int32 iSize, const SocketAddr* pAddr = 0);

		//发送协议
		virtual Bool	SendProtocol(HawkProtocol* pProtocol, const SocketAddr* pAddr = 0);

	protected:
		//数据可读
		virtual Bool    OnSessionRead();

		//数据可写
		virtual Bool    OnSessionWrite();

		//有连接可用
		virtual Bool    OnSessionAccept(HawkSocket& sSocket, SocketAddr& sAddr);

		//会话通知
		virtual Bool    OnSessionNotify(UInt32 iMsg, UInt32 iArgs);

		//解析协议进行分发
		virtual Bool    OnDecodeProtocol(UInt32& iCount);

		//是否等待输出
		virtual Bool    IsWaitOutput() const;

	protected:
		//会话ID
		SID					m_iSid;
		//状态
		UInt32				m_iState;		
		//会话套接字
		HawkSocket			m_sSocket;
		//会话回调
		HawkSessionProxy*	m_pProxy;
		//地址
		SocketAddr			m_sAddress;	
		//输入缓冲区
		OctetsStream*		m_pInputBuf;
		//输出缓冲区
		OctetsStream*		m_pOutputBuf;
		//临时缓冲区
		OctetsStream*		m_pCommonBuf;
		//加密对象
		HawkSecurity*		m_pISecurity;
		//解密对象
		HawkSecurity*		m_pOSecurity;		
		//统计信息
		Collect				m_sCollect;
		//UDP发送缓冲区
		UdpPacketVec		m_vUdpPacket;
		//UDP发送地址
		SocketAddr			m_sUdpTarget;	
		//类型(SERVER | CLIENT | PEER)
		UInt8				m_iSessionType;
	};
}
#endif

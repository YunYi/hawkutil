#ifndef HAWK_ZMP_H
#define HAWK_ZMP_H

#include "HawkObjBase.h"
#include "HawkProtocolManager.h"

namespace Hawk
{
	/************************************************************************/
	/* 0MQ分布式消息队列操作封装                                            */
	/************************************************************************/
	class UTIL_API HawkZmq : public HawkRefCounter
	{
	protected:
		//构造
		HawkZmq();

		//析构
		virtual ~HawkZmq();

		//友员什么便于manager的Init调用
		friend class HawkZmqManager;

	public:
		//ZMQ通讯类型
		enum
		{
			HZMQ_PAIR	= 0,
			HZMQ_PUB	= 1,
			HZMQ_SUB	= 2,
			HZMQ_REQ	= 3,
			HZMQ_REP	= 4,
			HZMQ_DEALER	= 5,
			HZMQ_ROUTER	= 6,
			HZMQ_PULL	= 7,
			HZMQ_PUSH	= 8,
			HZMQ_XPUB	= 9,
			HZMQ_XSUB	= 10,
		};

		//Send和Recv的Option参数
		enum
		{
			HZMQ_DONTWAIT = 1,
			HZMQ_SNDMORE  = 2,
		};

		//Message的Option属性
		enum
		{
			HZMQ_MSGMORE = 1,
		};

	public:
		//Svr模式绑定地址
		virtual Bool   Bind(const AString& sAddr);

		//Clt模式连接
		virtual Bool   Connect(const AString& sAddr);

		//发送数据(基于消息发送)
		virtual Bool   Send(void* pBuf, Size_t iSize, Int32 iFlag = 0);

		//接收数据(基于消息接收)
		virtual Bool   Recv(void* pBuf, Size_t& iSize, Int32 iFlag = 0);	

		//发送数据(基于协议发送)
		virtual Bool   SendProtocol(HawkProtocol* pProto, Int32 iFlag = 0);

		//接收数据(基于协议接收)
		virtual Bool   RecvProtocol(HawkProtocol*& pProto, Int32 iFlag = 0);

		//事件检测
		virtual UInt32 PollEvent(UInt32 iEvents = HEVENT_READ, Int32 iTimeout = -1);	

		//开启事件监视器
		virtual Bool   StartMonitor(const AString& sAddr, UInt32 iEvents);

		//关闭事件监视器
		virtual Bool   StopMonitor();

	public:
		//获取句柄
		virtual void*  GetHandle();

		//获取类型
		virtual Int32  GetType() const;

		//错误码
		virtual Int32  GetErrCode() const;

		//有更多数据要读取
		virtual Bool   IsWaitRecv() const;		

		//设置IDENTITY属性
		virtual Bool   SetIdentity(const void* pOptVal, Int32 iSize);	

		//设置ZMQ参数
		virtual Bool   SetOption(Int32 iOption, const void* pOptVal, Size_t iSize);

		//获取ZMQ参数
		virtual Bool   GetOption(Int32 iOption, void* pOptVal, Size_t& iSize);

	protected:
		//初始化ZMQ句柄
		virtual Bool   Init(Int32 iType, void* pHandle);	

		//填充错误信息
		virtual Bool   FillErr();

		//关闭队列
		virtual Bool   Close();

	protected:
		void*			m_pHandle;
		Int32			m_iType;			
		Int32			m_iErrCode;
		AString			m_sErrMsg;
		Int32			m_iRecvMore;
		OctetsStream*	m_pRecvBuf;
	};
}
#endif

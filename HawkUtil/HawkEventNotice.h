#ifndef HAWK_EVENTNOTICE_H
#define HAWK_EVENTNOTICE_H

#include "HawkOctets.h"

namespace Hawk
{
	/************************************************************************/
	/* 时间通知对象                                                         */
	/************************************************************************/
	class UTIL_API HawkEventNotice : public HawkRefCounter
	{
	public:
		//构造
		HawkEventNotice();

		//析构
		virtual ~HawkEventNotice();

		//赋值操作
		HawkEventNotice& operator = (const HawkEventNotice& sNotice);

		//事件线程通知标记
		enum
		{
			NOTIFY_SESSION	= 0x01,
			NOTIFY_PEER		= 0x02,
			NOTIFY_MSG		= 0x03,
			NOTIFY_WRITE	= 0x04,
			NOTIFY_CLOSE	= 0x05,
			NOTIFY_EXIT		= 0x06,
		};

	public:
		//通知标记
		UInt8  NoticeType;

		//通知参数
		union
		{
			//开启会话
			struct
			{
				//会话地址
				Char	    Address[IPV_LENGTH];
				//地址长度
				UInt8	    AddrLen;
				//会话类型, SERVER | CLIENT
				UInt8	    SessionType;
				//套接字类型, TCP | UDP
				UInt8	    SocketType;
				//会话回调
				void*	    SessionProxy;
			}eSession;

			//新建连接
			struct
			{
				//连接端套接字
				SOCKET	    Handle;
				//会话地址
				Char	    Address[IPV_LENGTH];
				//地址长度
				UInt8	    AddrLen;
			}ePeer;

			//消息通知
			struct
			{
				SID			Sid;
				UInt32		Msg;
				UInt32		Args;
			}eMsg;

			//发送数据
			struct
			{
				//会话ID
				SID			Sid;
				//发送数据Buffer
				HawkOctets* Octets;
				//会话地址
				Char	    Address[IPV_LENGTH];
				//地址长度
				UInt8	    AddrLen;
			}eWrite;

			//关闭会话
			struct
			{
				SID			Sid;
			}eClose;
		}NoticeParam;

	public:
		//清理数据
		virtual void  Clear();
	};
}
#endif

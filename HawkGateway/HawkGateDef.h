#ifndef HAWK_GATEDEF_H
#define HAWK_GATEDEF_H

#include "HawkUtil.h"

namespace Hawk
{
	//内部通知数据
	struct GateNotify
	{
		//通知类型标记
		enum
		{
			NOTIFY_UNKNOWN = 0,
			NOTIFY_SESSION_CONNECT,		//新建会话连接
			NOTIFY_SESSION_DISCONN,		//会话连接断开
			NOTIFY_SESSION_CLOSE,		//主动关闭会话			
			NOTIFY_SERVICE_EXIT,		//退出网关服务
			NOTIFY_SERVICE_ATTACH,		//服务挂载
			NOTIFY_SERVICE_DETACH,		//服务卸载
		};

		//默认构造
		GateNotify(UInt8 iType = NOTIFY_UNKNOWN) : Type(iType) {};

		//通知类型
		UInt8  Type;

		//通知参数
		union
		{
			//接收新连接
			struct
			{
				//连接端套接字
				SOCKET	Handle;
				//会话地址
				Char	Address[IPV_LENGTH];
				//地址长度
				UInt8	AddrLen;
			}eConnect;				

			//连接会话断开(被动)
			struct
			{
				//会话ID
				SID		Sid;
				//会话地址
				Char	Address[IPV_LENGTH];
				//地址长度
				UInt8	AddrLen;
			}eDisConn;

			//关闭连接会话(主动)
			struct
			{
				//会话ID
				SID		Sid;
			}eClose;

			//服务挂载
			struct 
			{
				UInt32  SvrId;
			}eAttach;
		};
	};
}
#endif

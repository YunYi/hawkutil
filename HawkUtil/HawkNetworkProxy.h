#ifndef HAWK_NETWORKPROXY_H
#define HAWK_NETWORKPROXY_H

#include "HawkSocket.h"
#include "HawkProtocol.h"

namespace Hawk
{
	/************************************************************************/
	/* 网络会话回调代理                                                     */
	/* 全局会话事件回调: (会话开启 | 会话关闭 | 会话协议)					*/
	/* 备注: 接口内不采用会话锁以及会话对象,用消息的机制向应用投递会话事件	*/
	/************************************************************************/
	class UTIL_API HawkNetworkProxy : public HawkRefCounter
	{
	public:
		//会话开启事件回调
		//SessionType: HawkSession::SERVER | HawkSession::CLIENT | HawkSession::PEER
		//SocketType:  HawkSocket::TCP | HawkSocket::UDP
		virtual Bool  OnSessionStart(SID iSid, UInt8 iSessionType, UInt8 iSocketType, const SocketAddr& sAddr);

		//会话关闭事件回调
		virtual Bool  OnSessionClose(SID iSid);

		//会话协议事件回调,返回true即表示应用接管协议并由应用释放,否则系统自行释放
		virtual Bool  OnSessionProtocol(SID iSid, Protocol* pProto, const SocketAddr* pAddr = 0);
	};
}
#endif

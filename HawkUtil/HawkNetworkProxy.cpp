#include "HawkNetworkProxy.h"

namespace Hawk
{
	Bool HawkNetworkProxy::OnSessionStart(SID iSid, UInt8 iSessionType, UInt8 iSocketType, const SocketAddr& sAddr) 
	{ 
		return true; 
	}

	Bool HawkNetworkProxy::OnSessionClose(SID iSid) 
	{ 
		return true; 
	};

	Bool HawkNetworkProxy::OnSessionProtocol(SID iSid, Protocol* pProto, const SocketAddr* pAddr)
	{
		return true;
	}
}

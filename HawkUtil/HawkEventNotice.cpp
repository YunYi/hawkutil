#include "HawkEventNotice.h"
#include "HawkSessionManager.h"

namespace Hawk
{
	HawkEventNotice::HawkEventNotice()
	{
		NoticeType = 0;
		memset(&NoticeParam, 0, sizeof(NoticeParam));
	}

	HawkEventNotice::~HawkEventNotice()
	{
	}

	HawkEventNotice& HawkEventNotice::operator = (const HawkEventNotice& sNotice)
	{
		if (this != &sNotice)
		{
			NoticeType = sNotice.NoticeType;
			memcpy(&NoticeParam, &sNotice.NoticeParam, sizeof(NoticeParam));
		}
		return *this;
	}

	void HawkEventNotice::Clear()
	{
		//释放回调
		if (NoticeType == NOTIFY_SESSION)
		{
			HawkSessionProxy* pProxy = (HawkSessionProxy*)NoticeParam.eSession.SessionProxy;
			HAWK_RELEASE(pProxy);

			NoticeParam.eSession.SessionProxy = 0;
		}
		//套接字
		else if (NoticeType == NOTIFY_PEER)
		{
			if (NoticeParam.ePeer.Handle && NoticeParam.ePeer.Handle != INVALID_SOCKET)
				closesocket(NoticeParam.ePeer.Handle);

			NoticeParam.ePeer.Handle = 0;
		}
		//释放内存
		else if (NoticeType == NOTIFY_WRITE)
		{
			P_SessionManager->FreeOctets(NoticeParam.eWrite.Octets);

			NoticeParam.eWrite.Octets = 0;
		}

		memset(&NoticeParam, 0, sizeof(NoticeParam));
	}
}

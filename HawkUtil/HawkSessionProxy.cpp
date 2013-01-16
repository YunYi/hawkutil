#include "HawkSessionProxy.h"

namespace Hawk
{
	HawkSessionProxy::HawkSessionProxy() : m_iSid(0)
	{
	}

	HawkSessionProxy::~HawkSessionProxy()
	{
		m_iSid = 0;
	}

	Bool HawkSessionProxy::Init(SID iSid)
	{
		m_iSid = iSid;
		return m_iSid != 0;
	}

	HawkSessionProxy* HawkSessionProxy::Clone() const
	{
		return new HawkSessionProxy;
	}

	Bool HawkSessionProxy::OnAccept(const HawkSocket& sSocket, const SocketAddr& sAddr)
	{
		return true;
	}

	Bool HawkSessionProxy::OnStart()
	{
		return true;
	}

	Bool HawkSessionProxy::OnRead()
	{
		return true;
	}

	Bool HawkSessionProxy::OnDecode(OctetsStream* pOS)
	{
		return true;
	}

	Bool HawkSessionProxy::OnProtocol(Protocol* pProto, const SocketAddr* pAddr)
	{
		return true;
	}

	Bool HawkSessionProxy::OnWrite()
	{
		return true;
	}	

	Bool HawkSessionProxy::OnNotify(UInt32 iMsg, UInt32 iArgs)
	{
		return true;
	}

	Bool HawkSessionProxy::OnClose()
	{
		m_iSid = 0;
		return true;
	}

	Bool HawkSessionProxy::OnRdRedirect(Size_t iSize)
	{
		return true;
	}

	Bool HawkSessionProxy::OnWrRedirect(Size_t iSize)
	{
		return true;
	}

	Bool HawkSessionProxy::GetRdRedirectOS(OctetsStream*& pOS, Size_t& iSize)
	{
		return false;
	}

	Bool HawkSessionProxy::GetWrRedirectOS(OctetsStream*& pOS, Size_t& iSize)
	{
		return false;
	}
}

#include "HawkAppObj.h"

namespace Hawk
{
	HawkAppObj::HawkAppObj()
	{
		m_bMsgTrack = false;
	}

	HawkAppObj::~HawkAppObj()
	{
		m_vObjMsg.clear();
	}

	XID HawkAppObj::GetXid() const
	{
		return m_sXid;
	}

	Bool HawkAppObj::OnMessage(const HawkMsg& sMsg)
	{
		if (m_bMsgTrack)
			TrackMsg(HOM_MSG, sMsg.Msg);

		return false;
	}

	Bool HawkAppObj::OnProtocol(SID iSid, Protocol* pProto)
	{
		if (m_bMsgTrack && pProto && pProto->GetType() > SysProtocol::SYS_MAX_PROTO)
			TrackMsg(HOM_PROTO, pProto->GetType());

		return false;
	}

	Bool HawkAppObj::EnableMsgTrack(Bool bEnable)
	{
		m_bMsgTrack = bEnable;
		return m_bMsgTrack;
	}

	void HawkAppObj::TrackMsg(UInt32 iType, UInt32 iId)
	{
		if (m_bMsgTrack)
			m_vObjMsg.push_back(ObjMsg(iType, iId));
	}
}

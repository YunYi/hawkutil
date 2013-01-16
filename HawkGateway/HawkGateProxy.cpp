#include "HawkGateProxy.h"
#include "HawkGateDef.h"

namespace Hawk
{
	HawkGateProxy::HawkGateProxy()
	{
		m_pProxyZmq = 0;
		m_pOctets   = 0;
	}

	HawkGateProxy::~HawkGateProxy()
	{
		P_ZmqManager->CloseZmq(m_pProxyZmq);
		HAWK_RELEASE(m_pOctets);
	}

	Bool HawkGateProxy::Init(const AString& sAddr, UInt32 iSvrId)
	{
		if (!m_pOctets)
			m_pOctets = new OctetsStream(PAGE_SIZE*2);

		if (!m_pProxyZmq)
		{
			m_pProxyZmq = P_ZmqManager->CreateZmq(HawkZmq::HZMQ_DEALER);
			if (!m_pProxyZmq->SetIdentity(&iSvrId, (Int32)sizeof(iSvrId)))
				return false;

			AString sZmqAddr = sAddr;
			if (sAddr.find("tcp://") == AString::npos)
				sZmqAddr = "tcp://" + sAddr;

			if (!m_pProxyZmq->Connect(sZmqAddr))
				return false;

			GateNotify sNotify;
			sNotify.Type = GateNotify::NOTIFY_SERVICE_ATTACH;
			sNotify.eAttach.SvrId = iSvrId;
			SendNotify(sNotify);
		}

		return true;
	}

	Bool HawkGateProxy::RecvProtocol(SID& iSid, HawkProtocol*& pProto, Int32 iTimeout)
	{
		if (m_pProxyZmq && m_pOctets)
		{
			if (m_pProxyZmq->PollEvent(HEVENT_READ, iTimeout))
			{
				//先接会话ID
				m_pOctets->Clear();
				Size_t iSize = (Size_t)m_pOctets->Capacity();
				if (!m_pProxyZmq->Recv(m_pOctets->Begin(), iSize))
					return false;

				//标记校验
				HawkAssert(iSize == sizeof(iSid));
				if (iSize != sizeof(iSid))
					return false;

				//再接协议内容
				iSid = *((SID*)m_pOctets->Begin());
				m_pOctets->Clear();
				iSize = (Size_t)m_pOctets->Capacity();
				if (!m_pProxyZmq->Recv(m_pOctets->Begin(), iSize))
					return false;

				//协议解析
				m_pOctets->Resize(iSize);
				try
				{
					pProto = P_ProtocolManager->Decode(*m_pOctets);
				}
				catch (HawkException& rhsExcep)
				{
					//异常退出
					HawkFmtError(rhsExcep.GetMsg().c_str());
					//释放协议
					P_ProtocolManager->ReleaseProto(pProto);
					//记录会话日志
					P_SessionManager->FmtSessionLog(iSid, "Session Decode Protocol Error, Msg: %s", rhsExcep.GetMsg().c_str());
					return false;
				}
				return pProto != 0;
			}			
		}
		return false;
	}

	Bool HawkGateProxy::SendProtocol(SID iSid, HawkProtocol* pProto)
	{
		if (iSid && pProto)
		{
			m_pOctets->Clear();
			if (pProto->Encode(*m_pOctets))
			{
				m_pProxyZmq->Send(&iSid, sizeof(iSid), HawkZmq::HZMQ_SNDMORE);
				m_pProxyZmq->Send(m_pOctets->Begin(), m_pOctets->Size());
				return true;
			}			
		}
		return false;
	}

	Bool  HawkGateProxy::CloseSession(SID iSid)
	{
		if (iSid)
		{
			GateNotify sNotify;
			sNotify.Type = GateNotify::NOTIFY_SESSION_CLOSE;
			sNotify.eClose.Sid = iSid;
			return SendNotify(sNotify, 0);
		}
		return false;
	}

	Bool HawkGateProxy::SendNotify(const GateNotify& sNotify, SID iSid)
	{
		if (m_pProxyZmq)
		{
			m_pProxyZmq->Send(&iSid, sizeof(iSid), HawkZmq::HZMQ_SNDMORE);
			m_pProxyZmq->Send((void*)&sNotify, sizeof(sNotify));
			return true;
		}
		return true;
	}
}

#include "HawkSessionManager.h"
#include "HawkProtocolManager.h"
#include "HawkAtomic.h"
#include "event.h"

namespace Hawk
{
	HAKW_SINGLETON_IMPL(SessionManager);

	HawkSessionManager::HawkSessionManager()
	{				
		m_ppThread	    = 0;
		m_iThread	    = 0;
		m_iTurnIdx	    = 0;
		m_pNetworkProxy = 0;
		m_pSessionProxy = 0;
		m_bSessionLog   = false;
		m_iBufSize      = PAGE_SIZE * 2;

		event_set_mem_functions(HawkMalloc, HawkRealloc, HawkFree);

#ifdef _DEBUG
		EnableSessionLog(true);
#endif
	}

	HawkSessionManager::~HawkSessionManager()
	{
		for (Int32 i=0;i<m_iThread;i++)
		{
			HAWK_RELEASE(m_ppThread[i]);
		}
		HAWK_DELETE_ARRAY(m_ppThread);

		HAWK_RELEASE(m_pSessionProxy);
		HAWK_RELEASE(m_pNetworkProxy);
	}

	Bool HawkSessionManager::Init(Int32 iThread)
	{		
		m_iThread = iThread + 1;
		HawkAssert(m_iThread > 0);
		if (m_iThread > 0)
		{
			m_ppThread = new HawkEventThread*[m_iThread];
			for (Int32 i=0;i<m_iThread;i++)
			{
				m_ppThread[i] = new HawkEventThread;
				if (!m_ppThread[i]->Init(i+1))
				{
					return false;
				}
			}
		}
		return true;
	}

	Bool HawkSessionManager::Run()
	{
		for (Int32 i=0;i<m_iThread;i++)
		{
			if (m_ppThread[i] && !m_ppThread[i]->Run())
				return false;
		}
		return true;
	}

	Bool HawkSessionManager::Stop()
	{
		for (Int32 i=0;i<m_iThread;i++)
		{
			if (m_ppThread[i])
				m_ppThread[i]->Close();
		}

		return HawkManagerBase::Stop();
	}

	void HawkSessionManager::SetBufSize(Int32 iBufSize)
	{
		m_iBufSize = iBufSize;
	}

	Int32 HawkSessionManager::GetBufSize() const
	{
		return m_iBufSize;
	}

	Int32  HawkSessionManager::GetThreadNum() const
	{
		return m_iThread;
	}

	Int32  HawkSessionManager::GetThreadId(Int32 iIdx) const
	{
		if (iIdx >= 0 && iIdx < m_iThread && m_ppThread && m_ppThread[iIdx])
		{
			return m_ppThread[iIdx]->GetThreadId();
		}
		return 0;
	}

	Bool HawkSessionManager::SetNetworkProxy(HawkNetworkProxy* pProxy)
	{
		HAWK_RELEASE(m_pNetworkProxy);
		if (pProxy)
		{			
			m_pNetworkProxy = pProxy;
			m_pNetworkProxy->AddRef();
			return true;
		}
		return false;
	}

	Bool HawkSessionManager::SetSessionProxyTmpl(HawkSessionProxy* pProxy)
	{
		HAWK_RELEASE(m_pSessionProxy);
		if (pProxy)
		{
			m_pSessionProxy = pProxy;
			m_pSessionProxy->AddRef();
			return true;
		}
		return false;
	}

	Bool HawkSessionManager::AllocSessionProxy(HawkSessionProxy*& pProxy)
	{
		if (m_pSessionProxy)
		{
			pProxy = m_pSessionProxy->Clone();
			return pProxy != 0;
		}
		return false;
	}

	void  HawkSessionManager::EnableSessionLog(Bool bEnable)
	{
		m_bSessionLog = bEnable;
		if (m_bSessionLog)
		{
			P_LoggerManager->CreateLogger("Logs/HawkSession.log","Session");
		}
	}

	Bool  HawkSessionManager::FmtSessionLog(SID iSid, const Char* pFmt, ...)
	{
		if (m_bSessionLog)
		{
			HawkLogger* pLogger = P_LoggerManager->GetLogger("Session");
			if (pLogger)
			{
				va_list args;
				Char sMsg[LOG_DEFAULT_SIZE] = {0};
				va_start(args, pFmt);
				_vsnprintf(sMsg, LOG_DEFAULT_SIZE-1, pFmt, args);
				va_end(args);

				pLogger->LogFmtMsg("SID: %d, Msg: %s\r\n", iSid, sMsg);
				return true;
			}			
		}
		return false;
	}

	Bool HawkSessionManager::DispatchSessionStart(SID iSid, UInt8 iSessionType, UInt8 iSocketType, const SocketAddr& sAddr)
	{
		if (iSid)
		{
			if (m_pNetworkProxy)
			{
				return m_pNetworkProxy->OnSessionStart(iSid, iSessionType, iSocketType, sAddr);
			}
			return true;
		}
		return false;
	}	

	Bool HawkSessionManager::DispatchSessionClose(SID iSid)
	{
		if (iSid)
		{
			if (m_pNetworkProxy)
			{
				return m_pNetworkProxy->OnSessionClose(iSid);
			}
			return true;
		}
		return false;
	}	

	Bool HawkSessionManager::DispatchSessionProtocol(SID iSid, HawkProtocol* pProto, const SocketAddr* pAddr)
	{
		if (iSid && pProto)
		{
			if (!m_pNetworkProxy || !m_pNetworkProxy->OnSessionProtocol(iSid, pProto, pAddr))
			{
				P_ProtocolManager->ReleaseProto(pProto);
			}		
			return true;
		}
		return false;
	}	

	Bool HawkSessionManager::StartSession(const AString& sAddr, UInt8 iSessionType, UInt8 iSocketType, HawkSessionProxy* pProxy)
	{
		if (IsRunning())
		{
			HawkEventNotice* pNotice = 0;
			if (AllocNotice(pNotice))
			{
				pNotice->NoticeType	= HawkEventNotice::NOTIFY_SESSION;
				pNotice->NoticeParam.eSession.AddrLen	   = SocketAddr(sAddr).CopyAddr(pNotice->NoticeParam.eSession.Address);
				pNotice->NoticeParam.eSession.SessionType  = iSessionType;
				pNotice->NoticeParam.eSession.SocketType   = iSocketType;
				pNotice->NoticeParam.eSession.SessionProxy = pProxy;
				if (pProxy) pProxy->AddRef();

				return DispatchNotice(0, pNotice);
			}
		}		
		return false;
	}

	Bool HawkSessionManager::SendRawData(SID iSid, void* pData, Int32 iSize, const SocketAddr* pAddr)
	{
		//TODO: 发送数据具备优化基础
		if (IsRunning() && iSid && pData && iSize)
		{
			HawkEventNotice* pNotice = 0;
			if (AllocNotice(pNotice))
			{
				pNotice->NoticeType = HawkEventNotice::NOTIFY_WRITE;
				pNotice->NoticeParam.eWrite.Sid = iSid;
				if (pAddr)
					pNotice->NoticeParam.eWrite.AddrLen = pAddr->CopyAddr(pNotice->NoticeParam.eWrite.Address);
				else
					pNotice->NoticeParam.eWrite.AddrLen = 0;

				pNotice->NoticeParam.eWrite.Octets = AllocOctets(iSize);
				if (!pNotice->NoticeParam.eWrite.Octets)
				{
					FreeNotice(pNotice);
					return false;
				}
				pNotice->NoticeParam.eWrite.Octets->Replace(pData, iSize);

				return DispatchNotice(iSid, pNotice);
			}
		}
		return false;
	}

	Bool HawkSessionManager::SendProtocol(SID iSid, HawkProtocol* pProto, const SocketAddr* pAddr)
	{
		if (IsRunning() && iSid && pProto)
		{			
			OctetsStream xOS;
			if (pProto->Encode(xOS))
				return SendRawData(iSid, xOS.Begin(), xOS.Size(), pAddr);
		}
		return false;
	}

	Bool HawkSessionManager::NotifySession(SID iSid, UInt32 iMsg, UInt32 iArgs)
	{
		if (IsRunning() && iMsg)
		{
			HawkEventNotice* pNotice = 0;
			if (AllocNotice(pNotice))
			{
				pNotice->NoticeType = HawkEventNotice::NOTIFY_MSG;
				pNotice->NoticeParam.eMsg.Sid  = iSid;
				pNotice->NoticeParam.eMsg.Msg  = iMsg;
				pNotice->NoticeParam.eMsg.Args = iArgs;

				return DispatchNotice(iSid, pNotice);
			}			
		}
		return false;
	}	

	Bool HawkSessionManager::CloseSession(SID iSid)
	{
		if (IsRunning() && iSid)
		{
			HawkEventNotice* pNotice = 0;
			if (AllocNotice(pNotice))
			{
				pNotice->NoticeType = HawkEventNotice::NOTIFY_CLOSE;
				pNotice->NoticeParam.eClose.Sid = iSid;
			
				return DispatchNotice(iSid, pNotice);
			}
		}
		return false;
	}

	Bool HawkSessionManager::AllocNotice(HawkEventNotice*& pNotice)
	{
		pNotice = new HawkEventNotice;		
		return pNotice != 0;
	}

	Bool HawkSessionManager::FreeNotice(HawkEventNotice* pNotice)
	{
		if (pNotice)
		{
			pNotice->Clear();
			HAWK_RELEASE(pNotice);
			return true;
		}
		return false;
	}

	HawkOctets*	HawkSessionManager::AllocOctets(Int32 iSize)
	{
		HawkOctets* pOctets = new HawkOctets(iSize);
		return pOctets;
	}

	Bool HawkSessionManager::FreeOctets(HawkOctets* pOctets)
	{
		if (pOctets)
		{
			HAWK_RELEASE(pOctets);
			return true;
		}
		return false;
	}

	Bool HawkSessionManager::DispatchNotice(SID iSid, HawkEventNotice* pNotice)
	{
		if (!IsRunning() || !pNotice)
		{
			FreeNotice(pNotice);
			return false;
		}

		if (pNotice->NoticeType == HawkEventNotice::NOTIFY_SESSION)
		{			
			if (m_ppThread[0] && m_ppThread[0]->NotifyNotice(pNotice))
				return true;
		}
		else if (pNotice->NoticeType == HawkEventNotice::NOTIFY_PEER)
		{
			Int32 iIdx = 0;
			if (m_iThread > 1)
				iIdx = (HawkAtomic::Inc<UInt32>(&m_iTurnIdx) - 1) % (m_iThread - 1) + 1;

			if (m_ppThread[iIdx] && m_ppThread[iIdx]->NotifyNotice(pNotice))
				return true;
		}
		else if (pNotice->NoticeType == HawkEventNotice::NOTIFY_WRITE || pNotice->NoticeType == HawkEventNotice::NOTIFY_CLOSE)
		{
			Int32 iIdx = (iSid - 1) % m_iThread;
			if (m_ppThread[iIdx] && m_ppThread[iIdx]->NotifyNotice(pNotice))
				return true;
		}
		else if (pNotice->NoticeType == HawkEventNotice::NOTIFY_MSG)
		{
			//每个线程都通知
			if (!iSid)
			{				
				for (Int32 i=0;i<m_iThread;i++)
				{
					if (m_ppThread[i])
					{
						HawkEventNotice* pTmpNotice = 0;
						if (AllocNotice(pTmpNotice))
						{
							*pTmpNotice = *pNotice; 
							if (!m_ppThread[i]->NotifyNotice(pTmpNotice))
								FreeNotice(pTmpNotice);
						}
					}
				}

				//释放已分配的通知对象
				FreeNotice(pNotice);
				return true;
			}

			Int32 iIdx = (iSid - 1) % m_iThread;
			if (m_ppThread[iIdx] && m_ppThread[iIdx]->NotifyNotice(pNotice))
				return true;
		}
		else
		{
			HawkFmtError("Disptach Unknown Notice, Type: %u", pNotice->NoticeType);
		}

		//处理错误需要释放已分配的通知对象
		FreeNotice(pNotice);
		return false;
	}	
}

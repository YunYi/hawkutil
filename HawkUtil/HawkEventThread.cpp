#include "HawkEventThread.h"
#include "HawkSessionManager.h"
#include "HawkLoggerManager.h"
#include "HawkOSOperator.h"
#include "event.h"

namespace Hawk
{
	//线程入口函数
	PVoid hawk_EventThreadRoutine(void* pArgs)
	{
		HawkEventThread* pThread = (HawkEventThread*)pArgs;
		if (pThread)
			pThread->OnThreadLoop();

		return 0;
	}

	//管道事件处理函数
	void hawk_EventPipeRoutine(Int32 iSocket, Short iWhich, void* pArgs) 
	{
		HawkEventThread* pThread = (HawkEventThread*)pArgs;		
		if (pThread)
			pThread->OnPipeEvent();
	}

	//会话事件处理函数
	void hawk_EventSessionRoutine(Int32 iSocket, Short iWhich, void* pArgs) 
	{
		HawkEventThread::ESession* pSession = (HawkEventThread::ESession*)pArgs;		
		if (pSession && pSession->Owner)
		{
			if (iWhich & EV_READ)
				pSession->Owner->OnSessionEvent(HEVENT_READ,  pSession);	
			else if (iWhich & EV_WRITE)
				pSession->Owner->OnSessionEvent(HEVENT_WRITE, pSession);
		}
	}	

	//////////////////////////////////////////////////////////////////////////

	HawkEventThread::HawkEventThread()
	{
		m_pThread	   = 0;
		m_pEvent	   = 0;
		m_pEventBase   = 0;
		m_iBaseSid     = 0;
		m_iCurSid	   = 0;		
		m_pNoticeLock  = 0;
		m_pNoticeMutex = 0;

		m_vSidInfo = new SID[MAX_SESSION];
		memset(m_vSidInfo, 0, sizeof(SID)*MAX_SESSION);
	}

	HawkEventThread::~HawkEventThread()
	{
		//关闭管道
		if (m_sPipe.IsValid())
			m_sPipe.Close();

		//删除事件对象
		if (m_pEvent)
		{
			event_free((event*)m_pEvent);
			m_pEvent = 0;
		}

		//删除基础事件
		if (m_pEventBase)
		{
			event_base_free((event_base*)m_pEventBase);
			m_pEventBase = 0;
		}

		//释放线程和锁
		HAWK_RELEASE(m_pThread);
		HAWK_RELEASE(m_pNoticeLock);
		HAWK_RELEASE(m_pNoticeMutex);
		HAWK_DELETE_ARRAY(m_vSidInfo);
	}	

	Bool HawkEventThread::Init(UInt32 iBaseId)
	{
		m_iBaseSid = iBaseId;

		if (!m_sPipe.Create())
			return false;

		if (!m_pNoticeLock)
			m_pNoticeLock = new HawkSpinLock;	
		
		if (!m_pNoticeMutex)
			m_pNoticeMutex = new HawkMutex;

		event_config* pCfg = event_config_new();
		if (!pCfg) 
		{
			HawkError("Create EventConfig Failed.");
			return false;
		}

		m_pEventBase = (void*)event_base_new_with_config(pCfg);
		if (!m_pEventBase)
		{
			HawkError("Create EventBase Failed.");
			return false;
		}
		event_config_free(pCfg);

		const Char* pszMethod = event_base_get_method((event_base*)m_pEventBase);
		if (iBaseId == 1 && pszMethod && strlen(pszMethod))
		{
			HawkFmtLog("Kernel Event Notification Mechanism: %s", pszMethod);

#ifdef _DEBUG
			HawkFmtPrint("Kernel Event Notification Mechanism: %s", pszMethod);
#endif
		}

		m_pEvent = (void*)event_new((event_base*)m_pEventBase, m_sPipe.GetReadFd(), EV_READ | EV_PERSIST, hawk_EventPipeRoutine, this);
		if (!m_pEvent)
		{
			HawkError("Create PipeEvent Failed.");
			return false;
		}

		if (event_add((event*)m_pEvent, 0) == HAWK_ERROR)
		{
			HawkError("Add PipeEvent Failed.");
			return false;
		}

		return true;
	}

	Bool HawkEventThread::Run()
	{
		if (!m_pThread && m_pEventBase && m_pEvent)
		{
			m_pThread = new HawkThread(hawk_EventThreadRoutine);
			if (m_pThread && m_pThread->Start(this))
			{
				return true;
			}
		}
		return false;
	}

	Bool HawkEventThread::Close()
	{
		if (m_pThread && m_pThread->IsRunning())
		{
			HawkEventNotice* pNotice = 0;
			if (!P_SessionManager->AllocNotice(pNotice))
				return false;

			pNotice->NoticeType  = HawkEventNotice::NOTIFY_EXIT;

			//发送事件
			if (!NotifyNotice(pNotice))
			{
				P_SessionManager->FreeNotice(pNotice);
				return false;
			}

			//关闭线程会等待结束
			m_pThread->Close();

			//释放会话
			FreeSessionMap();
			FreeSessionCache();

			//释放通知
			FreeNoticeList();

			return true;
		}
		return false;
	}
	
	Bool HawkEventThread::NotifyNotice(HawkEventNotice* pNotice)
	{
		if (pNotice)
		{
			UInt8 iFlag  = pNotice->NoticeType;
			Size_t iSize = sizeof(UInt8);

			//TODO: 多重锁的优化点
			HawkAutoMutex(mutex, m_pNoticeMutex);
			if (m_pNoticeLock)
			{
				HawkAutoSpinLock(lock, m_pNoticeLock);
				m_vNotice.push_back(pNotice);
			}

			m_sPipe.Send(&iFlag, iSize);			
			return true;
		}
		return false;
	}

	Bool HawkEventThread::PopNotice(HawkEventNotice*& pNotice)
	{
		HawkAutoSpinLock(lock, m_pNoticeLock);
		if (m_vNotice.size())
		{
			pNotice = m_vNotice.front();
			m_vNotice.pop_front();
			return pNotice != 0;
		}
		return false;
	}

	Bool HawkEventThread::OnThreadLoop()
	{
		if (m_pEventBase && m_pEvent)
		{
			HawkFmtLog("Launch EventLoop, ThreadId: %u", HawkOSOperator::GetThreadId());	

			event_base_loop((event_base*)m_pEventBase,0);
			return true;
		}
		return false;
	}

	Bool HawkEventThread::AllocSession(ESession*& pSession)
	{
		pSession = 0;
		if (m_vSessionCache.size())
		{
			pSession = m_vSessionCache.front();
			m_vSessionCache.pop_front();
		}

		if (!pSession)
		{
			pSession = (ESession*)HawkMalloc(sizeof(ESession));

			pSession->Flag    = 0;
			pSession->Owner   = 0;
			pSession->Session = new HawkSession;
			pSession->Event   = event_new((event_base*)m_pEventBase, 0, 0, 0, 0);

			HawkSessionProxy* pProxy = 0;
			if (P_SessionManager->AllocSessionProxy(pProxy))
			{
				pSession->Session->SetSessionProxy(pProxy);
				HAWK_RELEASE(pProxy);
			}
		}
		return pSession != 0;
	}

	Bool HawkEventThread::CacheSession(ESession* pSession)
	{
		if (pSession)
		{
			pSession->Flag   = 0;
			pSession->Owner  = 0;

			if (pSession->Session)
				pSession->Session->Close();

			if (pSession->Event)
				event_del((event*)pSession->Event);

			m_vSessionCache.push_back(pSession);
			return true;
		}
		return false;
	}

	Bool HawkEventThread::FreeSessionMap()
	{
		SessionMap::iterator it = m_mSession.begin();
		for (;it!=m_mSession.end();it++)
		{
			ESession* pSession = it->second;
			CacheSession(pSession);
		}
		m_mSession.clear();
		return true;
	}

	Bool HawkEventThread::FreeSessionCache()
	{
		SessionCache::iterator it = m_vSessionCache.begin();
		for (;it!=m_vSessionCache.end();it++)
		{
			ESession* pSession = *it;
			if (pSession)
			{
				event_free((event*)pSession->Event);
				HAWK_RELEASE(pSession->Session);
				HawkFree(pSession);
			}			
		}
		m_vSessionCache.clear();
		return true;
	}

	Bool HawkEventThread::FreeNoticeList()
	{
		NoticeList::iterator it = m_vNotice.begin();
		for (;it!=m_vNotice.end();it++)
		{
			HawkEventNotice* pNotice = *it;
			if (pNotice)
			{
				pNotice->Clear();
				HAWK_RELEASE(pNotice);

			}
		}
		m_vNotice.clear();
		return true;
	}

	Bool HawkEventThread::OnSessionStart(SID iSid, ESession* pSession)
	{
		SessionMap::iterator it = m_mSession.find(iSid);
		
		HawkAssert(it == m_mSession.end());
		if (it == m_mSession.end())
		{			
			m_mSession[iSid] = pSession;

			//添加会话ID到查询表
			for (UInt32 i=0;i<MAX_SESSION;i++)
			{
				if (!m_vSidInfo[i])
				{
					m_vSidInfo[i] = iSid;
					break;
				}
			}

			P_SessionManager->DispatchSessionStart(iSid, pSession->Session->GetSessionType(), 
				pSession->Session->GetSocketType(), pSession->Session->GetAddress());

			return true;
		}
		return false;
	}

	Bool HawkEventThread::OnSessionClose(SID iSid)
	{
		if (iSid)
		{
			SessionMap::iterator it = m_mSession.find(iSid);
			if (it != m_mSession.end())
			{
				P_SessionManager->DispatchSessionClose(iSid);

				ESession* pSession = it->second;
				m_mSession.erase(it);
				CacheSession(pSession);

				//从查询ID表中删除会话ID
				for (UInt32 i=0;i<MAX_SESSION;i++)
				{
					if (m_vSidInfo[i] == iSid)
					{
						m_vSidInfo[i] = 0;
						break;
					}
				}

				return true;
			}
		}		
		return false;
	}

	SID  HawkEventThread::GenSessionId()
	{
		SID iSid = P_SessionManager->GetThreadNum() * m_iCurSid + m_iBaseSid;
		m_iCurSid ++;
		return iSid;
	}

	Bool HawkEventThread::CheckSidIn(SID iSid) const
	{
		for (UInt32 i=0;i<MAX_SESSION;i++)
		{
			if (m_vSidInfo[i] == iSid)
				return true;
		}
		return false;
	}

	Int32 HawkEventThread::GetThreadId() const
	{
		if (m_pThread)
			return m_pThread->GetThreadId();
		
		return 0;
	}

	Bool HawkEventThread::OnPipeEvent()
	{
		if (m_sPipe.IsValid())
		{
			Size_t iSize = DEFAULT_SIZE;
			UInt8 vSignal[DEFAULT_SIZE] = {0};
			
			if (!m_sPipe.Receive(vSignal, iSize))
				return false;

			for (Size_t i=0;i<iSize;i++)
			{
				HawkEventNotice* pNotice = 0;
				if (!PopNotice(pNotice) || !pNotice)
					return false;

				HawkAssert(vSignal[i] == pNotice->NoticeType);

				ProcessNotice(pNotice);

				P_SessionManager->FreeNotice(pNotice);
			}
			
			return true;
		}
		return false;
	}

	Bool HawkEventThread::ProcessNotice(HawkEventNotice* pNotice)
	{
		HawkAssert(pNotice);

		//退出线程
		if (pNotice->NoticeType == HawkEventNotice::NOTIFY_EXIT)
		{
			if (event_base_loopbreak((event_base*)m_pEventBase) == HAWK_ERROR)
			{
				HawkFmtError("Break EventLoop Failed, ThreadId: %u", HawkOSOperator::GetThreadId());
			}
			else
			{
				HawkFmtLog("Break EventLoop Success, ThreadId: %u", HawkOSOperator::GetThreadId());
			}
		}
		//通知会话关闭
		else if (pNotice->NoticeType == HawkEventNotice::NOTIFY_CLOSE)
		{
			OnSessionClose(pNotice->NoticeParam.eClose.Sid);
		}
		//开启新会话
		else if (pNotice->NoticeType == HawkEventNotice::NOTIFY_SESSION)
		{
			HawkSessionProxy* pProxy = (HawkSessionProxy*)pNotice->NoticeParam.eSession.SessionProxy;
			if (pNotice->NoticeParam.eSession.AddrLen)
			{
				SocketAddr sAddr((sockaddr*)pNotice->NoticeParam.eSession.Address, pNotice->NoticeParam.eSession.AddrLen);
				if (pNotice->NoticeParam.eSession.SessionType == HawkSession::TYPE_SERVER)
				{						
					StartServer(sAddr, pNotice->NoticeParam.eSession.SocketType, pProxy);
				}
				else if (pNotice->NoticeParam.eSession.SessionType == HawkSession::TYPE_CLIENT)
				{
					StartClient(sAddr, pNotice->NoticeParam.eSession.SocketType, pProxy);
				}
			}
		}
		//接收新连接
		else if (pNotice->NoticeType == HawkEventNotice::NOTIFY_PEER)
		{
			if (pNotice->NoticeParam.ePeer.Handle != 0 && pNotice->NoticeParam.ePeer.Handle != INVALID_SOCKET)
			{
				if (pNotice->NoticeParam.ePeer.AddrLen)
				{
					SocketAddr sAddr((sockaddr*)pNotice->NoticeParam.ePeer.Address, pNotice->NoticeParam.ePeer.AddrLen);
					if (StartPeer(pNotice->NoticeParam.ePeer.Handle, sAddr))
					{
						pNotice->NoticeParam.ePeer.Handle = 0;
					}
				}
			}								
		}
		//发送会话数据
		else if (pNotice->NoticeType == HawkEventNotice::NOTIFY_WRITE)
		{
			SessionMap::iterator it = m_mSession.find(pNotice->NoticeParam.eWrite.Sid);
			if (it != m_mSession.end() && it->second->Session)
			{
				ESession* pSession = it->second;
				if (pSession->Session->GetSocketType() == HawkSocket::TYPE_TCP)
				{
					SendRawData(pSession, pNotice->NoticeParam.eWrite.Octets, 0);
				}
				else
				{
					if (pNotice->NoticeParam.eWrite.AddrLen)
					{
						SocketAddr sAddr((sockaddr*)pNotice->NoticeParam.eWrite.Address, pNotice->NoticeParam.eWrite.AddrLen);
						SendRawData(pSession, pNotice->NoticeParam.eWrite.Octets, &sAddr);
					}
					else
					{
						SendRawData(pSession, pNotice->NoticeParam.eWrite.Octets, &pSession->Session->m_sAddress);
					}
				}				

				//投递写事件
				UpdateEvent(pSession, HEVENT_WRITE);
			}
			//接收消息通知
			else if (pNotice->NoticeType == HawkEventNotice::NOTIFY_MSG)
			{
				if (pNotice->NoticeParam.eMsg.Sid)
				{
					SessionMap::iterator it = m_mSession.find(pNotice->NoticeParam.eMsg.Sid);
					if (it != m_mSession.end() && it->second->Session)
					{
						it->second->Session->OnSessionNotify(pNotice->NoticeParam.eMsg.Msg, pNotice->NoticeParam.eMsg.Args);
					}
				}
				else
				{
					SessionMap::iterator it = m_mSession.begin();
					for (;it != m_mSession.end();it++)
					{
						if (it->second->Session)
							it->second->Session->OnSessionNotify(pNotice->NoticeParam.eMsg.Msg, pNotice->NoticeParam.eMsg.Args);
					}
				}							
			}
		}
		return true;
	}

	Bool HawkEventThread::OnSessionEvent(UInt32 iEvent, ESession* pSession)
	{
		if (pSession && pSession->Event && pSession->Session)
		{
			HawkAssert(iEvent == pSession->Flag);
			if (iEvent == HEVENT_READ)
			{
				//服务器接收连接
				if (pSession->Session->GetSocket()->IsTcpTransport() && pSession->Session->GetSessionType() == HawkSession::TYPE_SERVER)
				{
					HawkSocket sSocket;
					SocketAddr sAddr;
					if (pSession->Session->OnSessionAccept(sSocket, sAddr))
					{
						HawkEventNotice* pNotice = 0;
						if (P_SessionManager->AllocNotice(pNotice))
						{
							pNotice->NoticeType = HawkEventNotice::NOTIFY_PEER;
							pNotice->NoticeParam.ePeer.AddrLen = sAddr.CopyAddr(pNotice->NoticeParam.ePeer.Address);
							pNotice->NoticeParam.ePeer.Handle  = sSocket.Handle();
							P_SessionManager->DispatchNotice(0, pNotice);
						}						
					}
					return true;
				}

				//读取数据
				if (!pSession->Session->OnSessionRead())
				{
					OnSessionClose(pSession->Session->GetSid());
					return false;
				}
				return true;
			}
			else if (iEvent == HEVENT_WRITE)
			{
				//写数据
				if (!pSession->Session->OnSessionWrite())
				{
					OnSessionClose(pSession->Session->GetSid());
					return false;
				}

				//切换事件失败
				if (!pSession->Session->IsWaitOutput() && !UpdateEvent(pSession, HEVENT_READ))
				{
					OnSessionClose(pSession->Session->GetSid());
					return false;
				}
				return true;
			}
		}
		return false;
	}

	Bool HawkEventThread::LaunchEvent(SID iSid, ESession* pSession)
	{
		if (pSession && pSession->Session && pSession->Session->IsValid())
		{
			event_set((event*)pSession->Event, pSession->Session->GetHandle(), EV_READ | EV_PERSIST, hawk_EventSessionRoutine, pSession);

			if (event_base_set((event_base*)m_pEventBase, (event*)pSession->Event) == HAWK_ERROR)
			{
				P_SessionManager->FmtSessionLog(iSid, "Session Set EventBase Failed.");
				return false;
			}

			if (event_add((event*)pSession->Event, 0) == HAWK_ERROR)
			{
				P_SessionManager->FmtSessionLog(iSid, "Session Add ReadEvent Failed.");
				return false;
			}
			return true;			
		}
		return false;
	}

	Bool HawkEventThread::UpdateEvent(ESession* pSession, UInt32 iEvent)
	{
		if (pSession && m_pEventBase && pSession->Event && pSession->Session && pSession->Session->IsActive())
		{
			if (pSession->Flag == iEvent)
				return true;

			if (event_del((event*)pSession->Event) == HAWK_ERROR)
				return false;

			pSession->Flag = iEvent;

			event_set((event*)pSession->Event, pSession->Session->GetHandle(), iEvent == HEVENT_READ? (EV_READ | EV_PERSIST):(EV_WRITE| EV_PERSIST), 
				hawk_EventSessionRoutine, pSession);

			if (event_base_set((event_base*)m_pEventBase, (event*)pSession->Event) == HAWK_ERROR)
				return false;			

			if (event_add((event*)pSession->Event, 0) == HAWK_ERROR) 
				return false;

			return true;
		}
		return false;
	}

	Bool HawkEventThread::StartServer(const SocketAddr& sAddr, UInt8 iSocketType, HawkSessionProxy* pProxy)
	{
		HawkSocket sSocket;
		if (iSocketType == HawkSocket::TYPE_TCP)
		{
			if (!sSocket.InitTcpServer(sAddr))
			{
				P_SessionManager->FmtSessionLog(0, "Init TcpServer Error, Addr: %s, ErrCode: %d", 
					sAddr.ToString().c_str(), sSocket.GetSocketErr());

				return false;
			}
		}
		else if (iSocketType == HawkSocket::TYPE_UDP)
		{
			if (!sSocket.InitUdpServer(sAddr))
			{
				P_SessionManager->FmtSessionLog(0, "Init UdpServer Error, Addr: %s, ErrCode: %d", 
					sAddr.ToString().c_str(), sSocket.GetSocketErr());

				return false;
			}
		}

		ESession* pSession = 0;
		if (AllocSession(pSession))
		{			
			A_Exception(pSession->Event != 0 && pSession->Session != 0);

			SID iSid = GenSessionId();
			pSession->Flag  = 0;
			pSession->Owner = this;
			pSession->Session->SetSid(iSid);
			
			if (pSession->Session->Init(sSocket, sAddr, HawkSession::TYPE_SERVER, pProxy))
			{
				if (LaunchEvent(iSid, pSession))
				{
					pSession->Flag = HEVENT_READ;
					pSession->Session->SetState(HawkSession::STATE_ACTIVATED);
					if (OnSessionStart(iSid, pSession))
						return true;
				}
			}
			else
			{
				P_SessionManager->FmtSessionLog(iSid, "Init SvrSession Error, Addr: %s", sAddr.ToString().c_str());
			}

			CacheSession(pSession);
		}
		return false;
	}

	Bool HawkEventThread::StartClient(const SocketAddr& sAddr, UInt8 iSocketType, HawkSessionProxy* pProxy)
	{
		HawkSocket sSocket;
		if (iSocketType == HawkSocket::TYPE_TCP)
		{
			if (!sSocket.InitTcpClient(sAddr))
			{
				P_SessionManager->FmtSessionLog(0, "Init TcpClient Error, Addr: %s, ErrCode: %d", 
					sAddr.ToString().c_str(), sSocket.GetSocketErr());

				return false;
			}
		}
		else if (iSocketType == HawkSocket::TYPE_UDP)
		{
			if (!sSocket.InitUdpClient(sAddr))
			{
				P_SessionManager->FmtSessionLog(0, "Init UdpClient Error, Addr: %s, ErrCode: %d", 
					sAddr.ToString().c_str(), sSocket.GetSocketErr());

				return false;
			}
		}

		ESession* pSession = 0;
		if (AllocSession(pSession))
		{
			A_Exception(pSession->Event != 0 && pSession->Session != 0);

			SID iSid = GenSessionId();
			pSession->Flag  = 0;
			pSession->Owner = this;
			pSession->Session->SetSid(iSid);

			if (pSession->Session->Init(sSocket, sAddr, HawkSession::TYPE_CLIENT, pProxy))
			{
				if (LaunchEvent(iSid, pSession))
				{
					pSession->Flag = HEVENT_READ;
					pSession->Session->SetState(HawkSession::STATE_ACTIVATED);
					if (OnSessionStart(iSid, pSession))
						return true;
				}
			}
			else
			{
				P_SessionManager->FmtSessionLog(iSid, "Init CltSession Error, Addr: %s", sAddr.ToString().c_str());
			}

			CacheSession(pSession);
		}
		return false;
	}

	Bool HawkEventThread::StartPeer(SOCKET hSocket, const SocketAddr& sAddr)
	{
		HawkSocket sSocket;
		if (!sSocket.Attach(hSocket, SOCK_STREAM, true))
		{
			P_SessionManager->FmtSessionLog(0, "Attach TcpPeer Error, Addr: %s, ErrCode: %d", 
				sAddr.ToString().c_str(), sSocket.GetSocketErr());

			return false;
		}

		if (!sSocket.InitTcpPeer())
		{
			P_SessionManager->FmtSessionLog(0, "Init TcpPeer Error, Addr: %s, ErrCode: %d", 
				sAddr.ToString().c_str(), sSocket.GetSocketErr());

			return false;
		}

		ESession* pSession = 0;
		if (AllocSession(pSession))
		{
			A_Exception(pSession->Event != 0 && pSession->Session != 0);
			SID iSid = GenSessionId();
			pSession->Flag  = 0;
			pSession->Owner = this;
			pSession->Session->SetSid(iSid);

			if (pSession->Session->Init(sSocket, sAddr, HawkSession::TYPE_PEER))
			{
				if (LaunchEvent(iSid, pSession))
				{
					pSession->Flag = HEVENT_READ;
					pSession->Session->SetState(HawkSession::STATE_ACTIVATED);
					if (OnSessionStart(iSid, pSession))
						return true;
				}
			}
			else
			{
				P_SessionManager->FmtSessionLog(iSid, "Init PeerSession Error, Addr: %s", sAddr.ToString().c_str());
			}

			CacheSession(pSession);
		}
		return false;
	}

	Bool HawkEventThread::SendRawData(ESession* pSession, HawkOctets* pData, const SocketAddr* pAddr)
	{
		if (pSession && pData && pData->Size())
		{
			if (pSession->Session && pSession->Session->IsActive())
			{
				return pSession->Session->SendRawData(pData->Begin(), pData->Size(), pAddr);
			}
		}
		return false;
	}
}

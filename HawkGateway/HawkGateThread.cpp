#include "HawkGateThread.h"
#include "HawkGateway.h"
#include "event.h"
#include "zmq.h"

namespace Hawk
{
	//�߳���ں���
	PVoid hawk_GateThreadRoutine(void* pArgs)
	{
		HawkGateThread* pThread = (HawkGateThread*)pArgs;
		HawkAssert(pThread);
		if (pThread)
			pThread->OnThreadLoop();

		return 0;
	}

	//�Ự���¼�������
	void hawk_GateSessionRead(bufferevent* pEvent, void* pArgs) 
	{
		HawkGateThread::Session* pSession = (HawkGateThread::Session*)pArgs;
		HawkAssert(pSession);
		if (pSession && pSession->GThread)
			pSession->GThread->OnSessionEvent(HEVENT_READ, pSession);
	}

	//�Ựд�¼�������
	void hawk_GateSessionWrite(bufferevent* pEvent, void* pArgs) 
	{
		HawkGateThread::Session* pSession = (HawkGateThread::Session*)pArgs;
		HawkAssert(pSession);
		if (pSession && pSession->GThread)
			pSession->GThread->OnSessionEvent(HEVENT_WRITE, pSession);
	}

	//�Ự�����¼�������
	void hawk_GateSessionError(bufferevent* pEvent, Short iWhat, void* pArgs) 
	{
		HawkGateThread::Session* pSession = (HawkGateThread::Session*)pArgs;
		HawkAssert(pSession);
		if (pSession && pSession->GThread)
			pSession->GThread->OnSessionEvent(HEVENT_EXCEPT, pSession);
	}

	//////////////////////////////////////////////////////////////////////////

	HawkGateThread::HawkGateThread(HawkGateway* pGateway)
	{
		m_pThread	= 0;
		m_pBase		= 0;
		m_pZmq		= 0;
		m_iBaseSid	= 0;
		m_iCurSid	= 0;
		m_pOctets	= 0;
		m_bIdle	    = true;
		m_bRunning  = false;
		m_pGateway	= pGateway;
	}

	HawkGateThread::~HawkGateThread()
	{
		//ɾ�������¼�
		if (m_pBase)
		{
			event_base_free((event_base*)m_pBase);
			m_pBase = 0;
		}

		//�ر���Ϣ����
		P_ZmqManager->CloseZmq(m_pZmq);

		//�ͷ���Ϣ����
		HAWK_RELEASE(m_pOctets);

		//�ͷ��̺߳���
		HAWK_RELEASE(m_pThread);

		//�ͷŻỰ�б�
		FreeSessionMap();
	}

	Bool HawkGateThread::Init(UInt32 iBaseId)
	{
		HawkAssert(!m_pThread && !m_pBase && !m_pZmq);

		m_iBaseSid = iBaseId;

		//����ͨ�û���
		if (!m_pOctets)
			m_pOctets = new OctetsStream(m_pGateway->GetBufSize());

		//�����߳�
		if (!m_pThread)
			m_pThread = new HawkThread(hawk_GateThreadRoutine);

		//�����¼���������
		if (!m_pBase)
		{
			event_config* pCfg = event_config_new();
			if (!pCfg) 
			{
				HawkPrint("Create EventConfig Failed.");
				return false;
			}
			
#ifdef PLATFORM_LINUX
			event_config_require_features(pCfg, EV_FEATURE_ET);
#endif
			event_config_set_flag(pCfg, EVENT_BASE_FLAG_NOLOCK);

			m_pBase = (void*)event_base_new_with_config(pCfg);
			event_config_free(pCfg);
			if (!m_pBase)
			{
				HawkPrint("Create EventBase Failed.");
				return false;
			}

			if (m_iBaseSid == 1)
			{
				const Char* pszMethod = event_base_get_method((event_base*)m_pBase);
				if (pszMethod && strlen(pszMethod))
				{
					HawkFmtPrint("Kernel Event Notification Mechanism: %s", pszMethod);
				}
			}			
		}

		//����ZMQ����
		if (!m_pZmq)
		{
			m_pZmq = P_ZmqManager->CreateZmq(HawkZmq::HZMQ_DEALER);
			m_pZmq->SetIdentity(&m_iBaseSid, sizeof(m_iBaseSid));
			m_pZmq->Connect(m_pGateway->GetThreadZmqAddr());
		}

		return true;
	}

	Bool HawkGateThread::Start()
	{
		if (m_pThread && m_pBase && m_pZmq && !m_bRunning)
		{
			m_bRunning = true;

			if (m_pThread)
				m_pThread->Start(this);

			return true;
		}
		return false;
	}

	Bool HawkGateThread::Close()
	{
		//�ȴ��߳̽���
		if (m_pThread)
			m_pThread->Close();

		//�ͷŵ�ǰ����
		FreeSessionMap();

		return true;
	}	

	Int32 HawkGateThread::GetThreadId() const
	{
		if (m_pThread)
			return m_pThread->GetThreadId();

		return 0;
	}

	SID  HawkGateThread::GenSessionId()
	{
		HawkAssert(m_pGateway);		
		SID iSid = m_iCurSid++ * m_pGateway->GetThreadNum() + m_iBaseSid;
		return iSid;
	}

	HawkGateThread::Session* HawkGateThread::AllocSession(SOCKET hSocket, const SocketAddr& sAddr)
	{
		if (hSocket != INVALID_SOCKET)
		{			
			Session* pSession = (Session*)HawkMalloc(sizeof(Session));
			memset(pSession, 0, sizeof(Session));

			pSession->GThread = this;
			pSession->Socket  = hSocket;
			pSession->Sid     = GenSessionId();
			pSession->Addr	  = new SocketAddr(sAddr);
			pSession->IBuffer = new OctetsStream(m_pGateway->GetBufSize());

			HawkSocket::SetBlocking(pSession->Socket, false);
			m_pGateway->CreateISecurity(pSession->ISecurity);
			m_pGateway->CreateOSecurity(pSession->OSecurity);
	
			return pSession;
		}
		return 0;
	}

	Bool HawkGateThread::FreeSession(Session* pSession)
	{
		if (pSession)
		{
			//�ͷ��¼�
			if (pSession->Event)
			{
				bufferevent_free((bufferevent*)pSession->Event);
				pSession->Event = 0;
			}
	
			//�ͷ��ڴ�
			HAWK_RELEASE(pSession->Addr);
			HAWK_RELEASE(pSession->IBuffer);
			HAWK_RELEASE(pSession->ISecurity);
			HAWK_RELEASE(pSession->OSecurity);
			
			//�ͷŻỰ
			HawkFree(pSession);

			return true;
		}
		return false;
	}

	Bool HawkGateThread::FreeSessionMap()
	{
		SessionMap::iterator it = m_mSession.begin();
		for (;it!=m_mSession.end();it++)
		{
			Session* pSession = it->second;
			FreeSession(pSession);
		}
		m_mSession.clear();
		return true;
	}

	Bool HawkGateThread::OnThreadLoop()
	{
		if (m_pBase)
		{
			//��־��¼
			HawkFmtPrint("GateThread EventLoop, ThreadId: %u", HawkOSOperator::GetThreadId());	

			while (m_bRunning)
			{
				//����Ĭ�Ͽ���״̬
				m_bIdle = true;

				//��ʼ��Ϣѭ��
				event_base_loop((event_base*)m_pBase, EVLOOP_ONCE | EVLOOP_NONBLOCK);

				//��ȡ���ط��͹�����������Ϣ
				OnGatewayEvent();

				//����״̬
				OnThreadIdle();
			}
			return true;
		}
		return false;
	}

	Bool HawkGateThread::StartSession(SOCKET hSocket, const SocketAddr& sAddr)
	{
		//�׽�����Ч
		if (hSocket == INVALID_SOCKET)
			return false;

		//���������¼�
		bufferevent* pEvent = bufferevent_socket_new((event_base*)m_pBase, hSocket, BEV_OPT_CLOSE_ON_FREE);
		if (!pEvent)
			return false;

		//�����Ự
		Session* pSession = AllocSession(hSocket, sAddr);
		if (!pSession)
		{
			bufferevent_free(pEvent);
			return false;
		}
		pSession->Event = pEvent;

		//���ûص�
		bufferevent_setcb(pEvent, hawk_GateSessionRead, hawk_GateSessionWrite, hawk_GateSessionError, pSession);

		//���ö���ʱ, ����Ϊ��������
		Int32 iTimeout = m_pGateway->GetSessionTimeout();
		if (iTimeout > 0)
		{
			struct timeval tv;
			tv.tv_sec  = iTimeout / 1000;
			tv.tv_usec = (iTimeout % 1000) * 1000; 
			bufferevent_set_timeouts(pEvent, &tv, NULL);
		}
		//������д�¼�
		bufferevent_enable(pEvent, EV_READ | EV_WRITE);

		//��ӵ��Ự�б�
		m_mSession[pSession->Sid] = pSession;

		//�������ܼ�����
		if (m_pGateway->GetProfiler())
			m_pGateway->GetProfiler()->RegConnect(true);		

		//�����־
		HawkFmtLog("SessionStart, Sid: %u, Address: %s", pSession->Sid, pSession->Addr->ToString().c_str());
		
		return true;
	}

	Bool HawkGateThread::CloseSession(SID iSid)
	{
		if (iSid)
		{
			SessionMap::iterator it = m_mSession.find(iSid);
			if (it != m_mSession.end())
			{
				//�޳��Ự�б�
				Session* pSession = it->second;
				m_mSession.erase(it);

				//�����־
				HawkFmtLog("SessionClose, SID: %u, Address: %s", pSession->Sid, pSession->Addr->ToString().c_str());

				//�ͷŻỰ
				FreeSession(pSession);

				//�������ܼ�����
				if (m_pGateway->GetProfiler())
					m_pGateway->GetProfiler()->RegConnect(false);

				//�޸�������
				m_pGateway->RegConnection(false);

				return true;
			}
		}		
		return false;
	}

	Bool HawkGateThread::RecvGateMsg(SID& iSid, OctetsStream* pOctets)
	{
		if (pOctets)
		{
			//����SIDͷ��Ϣ
			pOctets->Clear();
			Size_t iSize = (Size_t)pOctets->Capacity();
			if (!m_pZmq->Recv(pOctets->Begin(), iSize))
				return false;

			//SID����У��
			HawkAssert(iSize == sizeof(iSid));
			iSid = *((SID*)pOctets->Begin());
			if (iSize != sizeof(iSid))
				return false;

			//��Ϣ״̬У��
			Bool bRecvMore = m_pZmq->IsWaitRecv();
			HawkAssert(bRecvMore);
			if (!bRecvMore)
				return false;

			//����������
			pOctets->Clear();
			iSize = (Size_t)pOctets->Capacity();
			if (!m_pZmq->Recv(pOctets->Begin(), iSize))
				return false;

			pOctets->Resize((UInt32)iSize);
			return true;
		}
		return false;
	}

	Bool HawkGateThread::SendGateMsg(SID iSid, void* pData, Size_t iSize)
	{
		if (m_pZmq)
		{
			//���ͻỰID
			if (!m_pZmq->Send(&iSid, sizeof(iSid), HawkZmq::HZMQ_SNDMORE))
				return false;

			//����������
			if (!m_pZmq->Send(pData, iSize))
				return false;

			return true;
		}
		return false;
	}

	Bool HawkGateThread::SendProtocol(Session* pSession, Protocol* pProto)
	{
		if (pSession && pProto)
		{
			//����Э�鵽������
			m_pOctets->Clear();
			pProto->Encode(*m_pOctets);
			if(pSession->OSecurity)
				pSession->OSecurity->Update(*m_pOctets);

			//д��������
			if (bufferevent_write((bufferevent*)pSession->Event, m_pOctets->Begin(), m_pOctets->Size()) != HAWK_OK)
			{
				HawkFmtPrint("Event Buffer Write Error, Size: %u", m_pOctets->Size());				
				OnSessionError(pSession);
				return false;
			}

			//�������ܼ�����
			if (m_pGateway->GetProfiler())
				m_pGateway->GetProfiler()->RegSendProto(pProto->GetType(), m_pOctets->Size());

			return true;
		}
		return false;
	}

	Bool HawkGateThread::OnGatewayEvent()
	{
		while (m_pZmq && m_pZmq->PollEvent(HEVENT_READ, 0))
		{
			//���÷ǿ���
			m_bIdle = false;

			//�������ط��͵�����
			SID iSid = 0;
			if (!RecvGateMsg(iSid, m_pOctets))
				return false;

			//�ỰЭ��ת��
			if (iSid)
			{
				SessionMap::iterator it = m_mSession.find(iSid);
				if (it != m_mSession.end() && it->second)
				{
					Session* pSession = it->second;

					//�������������
					if (OnSessionEncode(pSession, m_pOctets))
					{
						//Э�����
						ProtoType iProtoType = *((ProtoType*)m_pOctets->Begin());
						if(pSession->OSecurity)
							pSession->OSecurity->Update(*m_pOctets);

						//д��������
						if (bufferevent_write((bufferevent*)pSession->Event, m_pOctets->Begin(), m_pOctets->Size()) != HAWK_OK)
						{
							HawkFmtPrint("Event Buffer Write Error, Size: %u", m_pOctets->Size());
							OnSessionError(pSession);
						}

						//�������ܼ�����
						if (m_pGateway->GetProfiler())
							m_pGateway->GetProfiler()->RegSendProto(iProtoType, m_pOctets->Size());
					}
				}
			}
			//ϵͳ��Ϣ֪ͨ
			else
			{
				HawkAssert(m_pOctets->Size() == sizeof(GateNotify));
				GateNotify* pNotify = (GateNotify*)m_pOctets->Begin();
				if (m_pOctets->Size() != sizeof(GateNotify))
					return false;
				
				//�����Ự����
				if (pNotify->Type == GateNotify::NOTIFY_SESSION_CONNECT)
				{
					if (pNotify->eConnect.Handle != INVALID_SOCKET && pNotify->eConnect.AddrLen)
					{
						SocketAddr sAddr((sockaddr*)pNotify->eConnect.Address, pNotify->eConnect.AddrLen);
						if (!StartSession(pNotify->eConnect.Handle, sAddr))
						{
							closesocket(pNotify->eConnect.Handle);
						}
					}
				}
				//�رջỰ����
				else if (pNotify->Type == GateNotify::NOTIFY_SESSION_CLOSE)
				{
					CloseSession(pNotify->eClose.Sid);
				}
				//��˷���ж��, Ĭ�ϹرջỰ
				else if (pNotify->Type == GateNotify::NOTIFY_SERVICE_DETACH)
				{
					while (m_mSession.size())
					{
						SID iCloseSid = m_mSession.begin()->first;
						CloseSession(iCloseSid);
					}
				}
				//�˳����ط���
				else if (pNotify->Type == GateNotify::NOTIFY_SERVICE_EXIT)
				{
					//�����˳����
					m_bRunning = false;
					HawkFmtPrint("Break EventLoop Success, ThreadId: %u", HawkOSOperator::GetThreadId());
				}
			}
		}
		return true;
	}

	Bool HawkGateThread::OnThreadIdle()
	{
		//���д���
		if (m_bIdle)
		{
			HawkSleep(DEFAULT_SLEEP);
		}		
		return true;
	}

	Bool HawkGateThread::OnSessionEvent(UInt32 iEvent, Session* pSession)
	{
		if (iEvent && pSession)
		{
			//���÷ǿ���
			m_bIdle = false;

			//�Ự���¼�
			if (iEvent & HEVENT_READ)
				OnSessionRead(pSession);

			//�Ựд�¼�
			if (iEvent & HEVENT_WRITE)
				OnSessionWrite(pSession);

			//�Ự�쳣�¼�
			if (iEvent & HEVENT_EXCEPT)
				OnSessionError(pSession);

			return true;
		}
		return false;
	}

	Bool HawkGateThread::OnSessionRead(Session* pSession)
	{
		if (!pSession || pSession->Socket == INVALID_SOCKET)
			return false;		

		//������뻺����
		struct evbuffer* pBuf = bufferevent_get_input((bufferevent*)pSession->Event);
		if (!pBuf || !evbuffer_get_length(pBuf)) 
		{
			OnSessionError(pSession);
			return false;
		}

		//ѭ�����������¼���Ե���������δ��ȡ����
		while (evbuffer_get_length(pBuf))
		{
			m_pOctets->Clear();

			//��ȡ����
			Int32 iReadSize = evbuffer_remove(pBuf, m_pOctets->Begin(), (Size_t)m_pOctets->Capacity());
			if (iReadSize <= 0)
			{
				OnSessionError(pSession);
				return false;
			}
			m_pOctets->Resize(iReadSize);

			//�������ܼ�����(ֻ������, ����ǰ������ʵ��������)
			if (m_pGateway->GetProfiler())
				m_pGateway->GetProfiler()->RegRecvProto(0, iReadSize);

			//����
			if (pSession->ISecurity)
				pSession->ISecurity->Update(*m_pOctets);

			//��䵽���뻺����
			UInt32 iFillPos = 0;
			UInt32 iBufSize = m_pOctets->Size();
			while(iBufSize)
			{
				//�����������뻺����
				UInt32 iFillSize = HawkMath::Min<UInt32>(pSession->IBuffer->EmptyCap(), iBufSize);
				if (iFillSize)
				{
					void* pData = (Char*)m_pOctets->Begin() + iFillPos;
					pSession->IBuffer->Insert(pSession->IBuffer->End(), pData, iFillSize);
					iFillPos += iFillSize;
					iBufSize -= iFillSize;
				}

				//����Ƿ���Խ������뻺����
				if (OnSessionDecode(pSession, pSession->IBuffer))
				{
					UInt32 iProtoSize = 0;
					while (P_ProtocolManager->CheckDecodeProtocol(*pSession->IBuffer, &iProtoSize))
					{
						Char* pProtoData = (Char*)pSession->IBuffer->AvailableData();
						ProtoType iProtoType = *((ProtoType*)pProtoData);

						//���Э��Ϸ���
						/*
						if (!P_ProtocolManager->CheckProtocolLegal(iProtoType))
						{
							HawkFmtError("UnknownProtocol: %d", iProtoType);						
							OnSessionError(pSession);
							return false;
						}
						*/

						//��Ӧ���ӵ�PingЭ��
						if (iProtoType == SysProtocol::SYS_CLT_PING)
						{
							//�ظ�Pong��Ӧ
							SysProtocol::Sys_SvrPong sProto((UInt32)HawkOSOperator::GetSysTime());
							if (!SendProtocol(pSession, &sProto))
								return false;
						}
						//���ݸ�����, �ٷ�����˷�����
						else
						{
							//�����ط�Э����Ϣ
							SendGateMsg(pSession->Sid, pProtoData, iProtoSize);
						}
						
						//�ƶ������α�
						pSession->IBuffer->MoveNonius(iProtoSize);

						//�������ܼ�����(ֻ��Э����, ��������ͳ�Ʋ�׼ȷ)
						if (m_pGateway->GetProfiler())
							m_pGateway->GetProfiler()->RegRecvProto(iProtoType, 0);
					}
				}				

				//�Ƴ����뻺���ǰ�οհ�
				pSession->IBuffer->RemoveBlank();	
			}
		}
		return true;
	}

	Bool HawkGateThread::OnSessionWrite(Session* pSession)
	{
		if (!pSession || pSession->Socket == INVALID_SOCKET)
			return false;

		return true;
	}

	Bool HawkGateThread::OnSessionError(Session* pSession)
	{
		if (pSession)
		{
			//���ͻỰ�Ͽ�Э��
			if (pSession->Sid)
			{
				SysProtocol::Sys_SessionBreak sProto(pSession->Sid);
				m_pOctets->Clear();
				sProto.Encode(*m_pOctets);
				SendGateMsg(pSession->Sid, m_pOctets->Begin(), m_pOctets->Size());
			}

			//�رջỰ
			CloseSession(pSession->Sid);
			return true;
		}
		return false;
	}

	Bool HawkGateThread::OnSessionDecode(Session* pSession, OctetsStream* pBuffer)
	{
		return true;
	}

	Bool HawkGateThread::OnSessionEncode(Session* pSession, OctetsStream* pBuffer)
	{
		return true;
	}
}

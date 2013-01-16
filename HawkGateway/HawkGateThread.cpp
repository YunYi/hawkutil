#include "HawkGateThread.h"
#include "HawkGateway.h"
#include "event.h"
#include "zmq.h"

namespace Hawk
{
	//线程入口函数
	PVoid hawk_GateThreadRoutine(void* pArgs)
	{
		HawkGateThread* pThread = (HawkGateThread*)pArgs;
		HawkAssert(pThread);
		if (pThread)
			pThread->OnThreadLoop();

		return 0;
	}

	//会话读事件处理函数
	void hawk_GateSessionRead(bufferevent* pEvent, void* pArgs) 
	{
		HawkGateThread::Session* pSession = (HawkGateThread::Session*)pArgs;
		HawkAssert(pSession);
		if (pSession && pSession->GThread)
			pSession->GThread->OnSessionEvent(HEVENT_READ, pSession);
	}

	//会话写事件处理函数
	void hawk_GateSessionWrite(bufferevent* pEvent, void* pArgs) 
	{
		HawkGateThread::Session* pSession = (HawkGateThread::Session*)pArgs;
		HawkAssert(pSession);
		if (pSession && pSession->GThread)
			pSession->GThread->OnSessionEvent(HEVENT_WRITE, pSession);
	}

	//会话错误事件处理函数
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
		//删除基础事件
		if (m_pBase)
		{
			event_base_free((event_base*)m_pBase);
			m_pBase = 0;
		}

		//关闭消息队列
		P_ZmqManager->CloseZmq(m_pZmq);

		//释放消息缓冲
		HAWK_RELEASE(m_pOctets);

		//释放线程和锁
		HAWK_RELEASE(m_pThread);

		//释放会话列表
		FreeSessionMap();
	}

	Bool HawkGateThread::Init(UInt32 iBaseId)
	{
		HawkAssert(!m_pThread && !m_pBase && !m_pZmq);

		m_iBaseSid = iBaseId;

		//创建通用缓冲
		if (!m_pOctets)
			m_pOctets = new OctetsStream(m_pGateway->GetBufSize());

		//创建线程
		if (!m_pThread)
			m_pThread = new HawkThread(hawk_GateThreadRoutine);

		//创建事件基础对象
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

		//创建ZMQ对象
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
		//等待线程结束
		if (m_pThread)
			m_pThread->Close();

		//释放当前连接
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
			//释放事件
			if (pSession->Event)
			{
				bufferevent_free((bufferevent*)pSession->Event);
				pSession->Event = 0;
			}
	
			//释放内存
			HAWK_RELEASE(pSession->Addr);
			HAWK_RELEASE(pSession->IBuffer);
			HAWK_RELEASE(pSession->ISecurity);
			HAWK_RELEASE(pSession->OSecurity);
			
			//释放会话
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
			//日志记录
			HawkFmtPrint("GateThread EventLoop, ThreadId: %u", HawkOSOperator::GetThreadId());	

			while (m_bRunning)
			{
				//设置默认空闲状态
				m_bIdle = true;

				//开始消息循环
				event_base_loop((event_base*)m_pBase, EVLOOP_ONCE | EVLOOP_NONBLOCK);

				//获取网关发送过来的数据消息
				OnGatewayEvent();

				//空闲状态
				OnThreadIdle();
			}
			return true;
		}
		return false;
	}

	Bool HawkGateThread::StartSession(SOCKET hSocket, const SocketAddr& sAddr)
	{
		//套接字有效
		if (hSocket == INVALID_SOCKET)
			return false;

		//创建缓存事件
		bufferevent* pEvent = bufferevent_socket_new((event_base*)m_pBase, hSocket, BEV_OPT_CLOSE_ON_FREE);
		if (!pEvent)
			return false;

		//创建会话
		Session* pSession = AllocSession(hSocket, sAddr);
		if (!pSession)
		{
			bufferevent_free(pEvent);
			return false;
		}
		pSession->Event = pEvent;

		//设置回调
		bufferevent_setcb(pEvent, hawk_GateSessionRead, hawk_GateSessionWrite, hawk_GateSessionError, pSession);

		//设置读超时, 可做为心跳机制
		Int32 iTimeout = m_pGateway->GetSessionTimeout();
		if (iTimeout > 0)
		{
			struct timeval tv;
			tv.tv_sec  = iTimeout / 1000;
			tv.tv_usec = (iTimeout % 1000) * 1000; 
			bufferevent_set_timeouts(pEvent, &tv, NULL);
		}
		//开启读写事件
		bufferevent_enable(pEvent, EV_READ | EV_WRITE);

		//添加到会话列表
		m_mSession[pSession->Sid] = pSession;

		//调用性能监视器
		if (m_pGateway->GetProfiler())
			m_pGateway->GetProfiler()->RegConnect(true);		

		//添加日志
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
				//剔除会话列表
				Session* pSession = it->second;
				m_mSession.erase(it);

				//添加日志
				HawkFmtLog("SessionClose, SID: %u, Address: %s", pSession->Sid, pSession->Addr->ToString().c_str());

				//释放会话
				FreeSession(pSession);

				//调用性能监视器
				if (m_pGateway->GetProfiler())
					m_pGateway->GetProfiler()->RegConnect(false);

				//修改连接数
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
			//接收SID头信息
			pOctets->Clear();
			Size_t iSize = (Size_t)pOctets->Capacity();
			if (!m_pZmq->Recv(pOctets->Begin(), iSize))
				return false;

			//SID数据校验
			HawkAssert(iSize == sizeof(iSid));
			iSid = *((SID*)pOctets->Begin());
			if (iSize != sizeof(iSid))
				return false;

			//消息状态校验
			Bool bRecvMore = m_pZmq->IsWaitRecv();
			HawkAssert(bRecvMore);
			if (!bRecvMore)
				return false;

			//接收数据体
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
			//发送会话ID
			if (!m_pZmq->Send(&iSid, sizeof(iSid), HawkZmq::HZMQ_SNDMORE))
				return false;

			//发送数据体
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
			//加密协议到缓冲区
			m_pOctets->Clear();
			pProto->Encode(*m_pOctets);
			if(pSession->OSecurity)
				pSession->OSecurity->Update(*m_pOctets);

			//写进缓冲区
			if (bufferevent_write((bufferevent*)pSession->Event, m_pOctets->Begin(), m_pOctets->Size()) != HAWK_OK)
			{
				HawkFmtPrint("Event Buffer Write Error, Size: %u", m_pOctets->Size());				
				OnSessionError(pSession);
				return false;
			}

			//调用性能监视器
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
			//设置非空闲
			m_bIdle = false;

			//接收网关发送的数据
			SID iSid = 0;
			if (!RecvGateMsg(iSid, m_pOctets))
				return false;

			//会话协议转发
			if (iSid)
			{
				SessionMap::iterator it = m_mSession.find(iSid);
				if (it != m_mSession.end() && it->second)
				{
					Session* pSession = it->second;

					//加密输出缓冲区
					if (OnSessionEncode(pSession, m_pOctets))
					{
						//协议加密
						ProtoType iProtoType = *((ProtoType*)m_pOctets->Begin());
						if(pSession->OSecurity)
							pSession->OSecurity->Update(*m_pOctets);

						//写进缓冲区
						if (bufferevent_write((bufferevent*)pSession->Event, m_pOctets->Begin(), m_pOctets->Size()) != HAWK_OK)
						{
							HawkFmtPrint("Event Buffer Write Error, Size: %u", m_pOctets->Size());
							OnSessionError(pSession);
						}

						//调用性能监视器
						if (m_pGateway->GetProfiler())
							m_pGateway->GetProfiler()->RegSendProto(iProtoType, m_pOctets->Size());
					}
				}
			}
			//系统消息通知
			else
			{
				HawkAssert(m_pOctets->Size() == sizeof(GateNotify));
				GateNotify* pNotify = (GateNotify*)m_pOctets->Begin();
				if (m_pOctets->Size() != sizeof(GateNotify))
					return false;
				
				//创建会话连接
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
				//关闭会话连接
				else if (pNotify->Type == GateNotify::NOTIFY_SESSION_CLOSE)
				{
					CloseSession(pNotify->eClose.Sid);
				}
				//后端服务卸载, 默认关闭会话
				else if (pNotify->Type == GateNotify::NOTIFY_SERVICE_DETACH)
				{
					while (m_mSession.size())
					{
						SID iCloseSid = m_mSession.begin()->first;
						CloseSession(iCloseSid);
					}
				}
				//退出网关服务
				else if (pNotify->Type == GateNotify::NOTIFY_SERVICE_EXIT)
				{
					//设置退出标记
					m_bRunning = false;
					HawkFmtPrint("Break EventLoop Success, ThreadId: %u", HawkOSOperator::GetThreadId());
				}
			}
		}
		return true;
	}

	Bool HawkGateThread::OnThreadIdle()
	{
		//空闲处理
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
			//设置非空闲
			m_bIdle = false;

			//会话读事件
			if (iEvent & HEVENT_READ)
				OnSessionRead(pSession);

			//会话写事件
			if (iEvent & HEVENT_WRITE)
				OnSessionWrite(pSession);

			//会话异常事件
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

		//检测输入缓冲区
		struct evbuffer* pBuf = bufferevent_get_input((bufferevent*)pSession->Event);
		if (!pBuf || !evbuffer_get_length(pBuf)) 
		{
			OnSessionError(pSession);
			return false;
		}

		//循环操作避免事件边缘触发引起的未读取问题
		while (evbuffer_get_length(pBuf))
		{
			m_pOctets->Clear();

			//读取数据
			Int32 iReadSize = evbuffer_remove(pBuf, m_pOctets->Begin(), (Size_t)m_pOctets->Capacity());
			if (iReadSize <= 0)
			{
				OnSessionError(pSession);
				return false;
			}
			m_pOctets->Resize(iReadSize);

			//调用性能监视器(只记流量, 解密前才是真实流量数据)
			if (m_pGateway->GetProfiler())
				m_pGateway->GetProfiler()->RegRecvProto(0, iReadSize);

			//解密
			if (pSession->ISecurity)
				pSession->ISecurity->Update(*m_pOctets);

			//填充到输入缓冲区
			UInt32 iFillPos = 0;
			UInt32 iBufSize = m_pOctets->Size();
			while(iBufSize)
			{
				//尽可能填充解码缓冲区
				UInt32 iFillSize = HawkMath::Min<UInt32>(pSession->IBuffer->EmptyCap(), iBufSize);
				if (iFillSize)
				{
					void* pData = (Char*)m_pOctets->Begin() + iFillPos;
					pSession->IBuffer->Insert(pSession->IBuffer->End(), pData, iFillSize);
					iFillPos += iFillSize;
					iBufSize -= iFillSize;
				}

				//检测是否可以解码输入缓冲区
				if (OnSessionDecode(pSession, pSession->IBuffer))
				{
					UInt32 iProtoSize = 0;
					while (P_ProtocolManager->CheckDecodeProtocol(*pSession->IBuffer, &iProtoSize))
					{
						Char* pProtoData = (Char*)pSession->IBuffer->AvailableData();
						ProtoType iProtoType = *((ProtoType*)pProtoData);

						//检测协议合法性
						/*
						if (!P_ProtocolManager->CheckProtocolLegal(iProtoType))
						{
							HawkFmtError("UnknownProtocol: %d", iProtoType);						
							OnSessionError(pSession);
							return false;
						}
						*/

						//响应连接的Ping协议
						if (iProtoType == SysProtocol::SYS_CLT_PING)
						{
							//回复Pong响应
							SysProtocol::Sys_SvrPong sProto((UInt32)HawkOSOperator::GetSysTime());
							if (!SendProtocol(pSession, &sProto))
								return false;
						}
						//传递给网关, 再发往后端服务器
						else
						{
							//给网关发协议消息
							SendGateMsg(pSession->Sid, pProtoData, iProtoSize);
						}
						
						//移动数据游标
						pSession->IBuffer->MoveNonius(iProtoSize);

						//调用性能监视器(只记协议数, 避免流量统计不准确)
						if (m_pGateway->GetProfiler())
							m_pGateway->GetProfiler()->RegRecvProto(iProtoType, 0);
					}
				}				

				//移除输入缓冲的前段空白
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
			//发送会话断开协议
			if (pSession->Sid)
			{
				SysProtocol::Sys_SessionBreak sProto(pSession->Sid);
				m_pOctets->Clear();
				sProto.Encode(*m_pOctets);
				SendGateMsg(pSession->Sid, m_pOctets->Begin(), m_pOctets->Size());
			}

			//关闭会话
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

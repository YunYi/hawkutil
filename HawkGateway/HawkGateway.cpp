#include "HawkGateway.h"
#include "event.h"
#include "zmq.h"

namespace Hawk
{
	HawkGateway::HawkGateway()
	{
		m_iThread	  = 0;
		m_iTurnIdx	  = 0;
		m_ppThread    = 0;
		m_pProfiler   = 0;
		m_pThreadZmq  = 0;
		m_pServerZmq  = 0;
		m_pMonitorZmq = 0;
		m_pOctets	  = 0;
		m_iConnCount  = 0;
		m_iCurSvrId	  = 0;
		m_iTimeout    = -1;
		m_iConnLimit  = -1;
		m_bIdle		  = true;
		m_bEchoMode   = false;		
		m_bRunning    = false;		
		m_iBufSize    = PAGE_SIZE * 2;
	}

	HawkGateway::~HawkGateway()
	{
		//关闭监听套接字
		m_sSocket.Close();

		//释放消息缓冲
		HAWK_RELEASE(m_pOctets);
		
		//释放性能监视器
		HAWK_RELEASE(m_pProfiler);

		//关闭消息队列
		P_ZmqManager->CloseZmq(m_pMonitorZmq);
		P_ZmqManager->CloseZmq(m_pServerZmq);
		P_ZmqManager->CloseZmq(m_pThreadZmq);
		
		//释放线程
		for (Int32 i=0;m_ppThread && i<m_iThread;i++)
		{
			HAWK_RELEASE(m_ppThread[i]);
		}
		HAWK_DELETE_ARRAY(m_ppThread);
	}

	void HawkGateway::SetBufSize(Int32 iBufSize)
	{
		m_iBufSize = iBufSize;
	}

	Int32 HawkGateway::GetBufSize() const
	{
		return m_iBufSize;
	}

	void HawkGateway::SetConnLimit(Int32 iLimit)
	{
		m_iConnLimit = iLimit;
	}

	Int32 HawkGateway::GetConnLimit() const
	{
		return m_iConnLimit;
	}

	Bool HawkGateway::RegConnection(Bool bConn)
	{
		if (bConn)
			HawkAtomic::Inc<UInt32>(&m_iConnCount);
		else
			HawkAtomic::Dec<UInt32>(&m_iConnCount);

		return true;
	}

	void HawkGateway::SetSessionTimeout(Int32 iTimeout)
	{
		m_iTimeout = iTimeout;
	}

	Int32 HawkGateway::GetSessionTimeout() const
	{
		return m_iTimeout;
	}

	Int32  HawkGateway::GetThreadNum() const
	{
		return m_iThread;
	}

	Int32  HawkGateway::GetThreadId(Int32 iIdx) const
	{
		if (iIdx >= 0 && iIdx < m_iThread && m_ppThread && m_ppThread[iIdx])
			return m_ppThread[iIdx]->GetThreadId();
		
		return 0;
	}

	Bool HawkGateway::CreateGateThread(HawkGateThread*& pThread)
	{
		pThread = new HawkGateThread(this);
		return pThread != 0;
	}

	Bool HawkGateway::CreateISecurity(HawkSecurity*& pSecurity)
	{
		pSecurity = 0;
		return true;
	}

	Bool HawkGateway::CreateOSecurity(HawkSecurity*& pSecurity)
	{
		pSecurity = 0;
		return true;
	}

	Bool HawkGateway::Init(const AString& sFrontend, const AString& sBackend, Int32 iThread, Bool bEchoMode)
	{
		HawkAssert(sFrontend.size() && sBackend.size() && iThread > 0);
		if (!sFrontend.size() || !sBackend.size() || iThread <= 0)
			return false;

		//参数赋值
		m_iThread   = iThread;
		m_bEchoMode = bEchoMode;

		//创建通用缓冲
		if (!m_pOctets)
			m_pOctets = new OctetsStream(m_iBufSize);

		//创建服务器套接字
		if (!m_sSocket.IsValid())
		{
			if (!m_sSocket.InitTcpServer(sFrontend))
			{
				HawkFmtPrint("Init TcpServer Error, Addr: %s, ErrCode: %d", sFrontend.c_str(), m_sSocket.GetSocketErr());
				return false;
			}
		}

		//创建网关线程通信队列
		if (!m_pThreadZmq)
		{
			m_pThreadZmq = P_ZmqManager->CreateZmq(HawkZmq::HZMQ_ROUTER);
			if (!m_pThreadZmq->Bind(GetThreadZmqAddr()))
			{
				HawkFmtPrint("ThreadZmq Bind Error, Addr: %s", GetThreadZmqAddr().c_str());
				return false;
			}
		}

		//创建后端服务通信队列
		if (!m_pServerZmq)
		{
			AString sAddr = sBackend;
			if (sAddr.find("tcp://") == AString::npos)
				sAddr = "tcp://" + sBackend;

			m_pServerZmq = P_ZmqManager->CreateZmq(HawkZmq::HZMQ_ROUTER);
			if (!m_pServerZmq->Bind(sAddr))
			{
				HawkFmtPrint("ServerZmq Bind Error, Addr: %s", sAddr.c_str());
				return false;
			}
		}
		
		//创建服务监视器
		if (!m_pMonitorZmq && m_pServerZmq)
		{
			m_pServerZmq->StartMonitor(GetServerZmqAddr(), ZMQ_EVENT_ACCEPTED | ZMQ_EVENT_DISCONNECTED);

			m_pMonitorZmq = P_ZmqManager->CreateZmq(HawkZmq::HZMQ_PAIR);
			if (!m_pMonitorZmq->Connect(GetServerZmqAddr()))
			{
				HawkFmtPrint("MonitorZmq Connect Error, Addr: %s", GetServerZmqAddr().c_str());
				return false;
			}
		}

		//创建网关线程
		m_ppThread = new HawkGateThread*[m_iThread];
		memset(m_ppThread, 0, sizeof(HawkGateThread*) * m_iThread);

		for (UInt32 i=0;i<(UInt32)m_iThread;i++)
		{
			//创建网关线程
			if (!CreateGateThread(m_ppThread[i]))
			{
				HawkPrint("Create GateThread Failed.");
				return false;
			}

			//初始化网关线程
			if (!m_ppThread[i]->Init(i+1))
			{
				HawkPrint("Init GateThread Failed.");
				return false;
			}

			//开启网关线程
			if (!m_ppThread[i]->Start())
			{
				HawkPrint("Start GateThread Failed.");
				return false;
			}
		}		
		return true;
	}

	Bool HawkGateway::Stop()
	{
		//通知退出循环
		m_bRunning = false;
		return true;
	}

	AString HawkGateway::GetThreadZmqAddr() const
	{
		return "inproc://hawk-gateway";
	}

	AString HawkGateway::GetServerZmqAddr() const
	{
		return "inproc://hawk-monitor";
	}

	HawkProfiler* HawkGateway::GetProfiler()
	{
		return m_pProfiler;
	}

	Bool HawkGateway::TurnOnProfiler(const AString& sAddr)
	{
		if (!m_pProfiler)
		{
			m_pProfiler = new HawkProfiler;
			return m_pProfiler->Start(sAddr);
		}
		return true;
	}

	Bool HawkGateway::Run()
	{
		if (!m_bRunning)
		{
			m_bRunning = true;
			HawkPrint("Gateway Running......");
		}

		while (m_bRunning)
		{
			//设置默认空闲状态
			m_bIdle = true;

			//接收新网络连接
			OnSessionAccept();

			//检测监视器事件
			OnGateMonitorEvent();

			//检测网关线程事件
			OnGateThreadEvent();

			//检测后端服务事件
			OnGateServerEvent();

			//网关空闲处理
			OnGateIdleEvent();
		}

		//阻塞等待结束
		OnGatewayClose();

		return true;
	}		

	Bool HawkGateway::RecvThreadMsg(UInt32& iThread, SID& iSid, OctetsStream* pOctets)
	{
		if (m_pThreadZmq && pOctets)
		{
			//线程ID标识信息
			pOctets->Clear();
			Size_t iSize = (Size_t)pOctets->Capacity();
			if (!m_pThreadZmq->Recv(pOctets->Begin(), iSize))
				return false;

			HawkAssert(iSize == sizeof(iThread));
			iThread = *((UInt32*)pOctets->Begin());
			if (iSize != sizeof(iThread))
				return false;

			Bool bRecvMore = m_pThreadZmq->IsWaitRecv();
			HawkAssert(bRecvMore);
			if (!bRecvMore)
				return false;

			//会话ID标识信息
			pOctets->Clear();
			iSize = (Size_t)pOctets->Capacity();
			if (!m_pThreadZmq->Recv(pOctets->Begin(), iSize))
				return false;

			HawkAssert(iSize == sizeof(iSid));
			iSid = *((SID*)pOctets->Begin());
			if (iSize != sizeof(iSid))
				return false;

			bRecvMore = m_pThreadZmq->IsWaitRecv();
			HawkAssert(bRecvMore);
			if (!bRecvMore)
				return false;

			//消息数据体
			pOctets->Clear();
			iSize = (Size_t)pOctets->Capacity();
			if (!m_pThreadZmq->Recv(pOctets->Begin(), iSize))
				return false;

			pOctets->Resize((UInt32)iSize);
			return true;
		}
		return false;
	}

	Bool HawkGateway::SendThreadMsg(UInt32 iThread, SID iSid, void* pData, Size_t iSize)
	{
		if (m_pThreadZmq && iThread)
		{
			//目标
			if (!m_pThreadZmq->Send(&iThread, sizeof(iThread), HawkZmq::HZMQ_SNDMORE))
				return false;

			//会话
			if (!m_pThreadZmq->Send(&iSid, sizeof(iSid), HawkZmq::HZMQ_SNDMORE))
				return false;

			//数据
			if (!m_pThreadZmq->Send(pData, iSize))
				return false;

			return true;
		}
		return false;
	}

	Bool HawkGateway::RecvServerMsg(UInt32& iSvrId, SID& iSid, OctetsStream* pOctets)
	{
		if (m_pServerZmq && pOctets)
		{
			//线程ID标识信息
			pOctets->Clear();
			Size_t iSize = (Size_t)pOctets->Capacity();
			if (!m_pServerZmq->Recv(pOctets->Begin(), iSize))
				return false;

			HawkAssert(iSize == sizeof(iSvrId));
			iSvrId = *((UInt32*)pOctets->Begin());
			if (iSize != sizeof(iSvrId))
				return false;

			Bool bRecvMore = m_pServerZmq->IsWaitRecv();
			HawkAssert(bRecvMore);
			if (!bRecvMore)
				return false;

			//会话ID标识信息
			pOctets->Clear();
			iSize = (Size_t)pOctets->Capacity();
			if (!m_pServerZmq->Recv(pOctets->Begin(), iSize))
				return false;

			HawkAssert(iSize == sizeof(iSid));
			iSid = *((SID*)pOctets->Begin());
			if (iSize != sizeof(iSid))
				return false;

			bRecvMore = m_pServerZmq->IsWaitRecv();
			HawkAssert(bRecvMore);
			if (!bRecvMore)
				return false;

			//消息数据体
			pOctets->Clear();
			iSize = (Size_t)pOctets->Capacity();
			if (!m_pServerZmq->Recv(pOctets->Begin(), iSize))
				return false;

			pOctets->Resize((UInt32)iSize);
			return true;
		}
		return false;
	}

	Bool HawkGateway::SendServerMsg(UInt32 iSvrId, SID iSid, void* pData, Size_t iSize)
	{
		if (m_pServerZmq && iSvrId)
		{
			//目标
			if (!m_pServerZmq->Send(&iSvrId, sizeof(iSvrId), HawkZmq::HZMQ_SNDMORE))
				return false;

			//会话
			if (!m_pServerZmq->Send(&iSid, sizeof(iSid), HawkZmq::HZMQ_SNDMORE))
				return false;

			//数据
			if (!m_pServerZmq->Send(pData, iSize))
				return false;

			return true;
		}
		return false;
	}

	Bool HawkGateway::OnSessionAccept()
	{
		//接收新连接
		HawkSocket sSocket;
		SocketAddr sAddr;
		while (m_sSocket.Accept(sSocket, sAddr))
		{
			//设置非空闲
			m_bIdle = false;

			//拒绝连接
			if (m_iConnLimit > 0 && (Int32)m_iConnCount >= m_iConnLimit)
			{
				//先发送拒绝连接协议再关闭
				SysProtocol::Sys_RefuseConn sProto(SysProtocol::ERR_REFUSE_CONN);
				m_pOctets->Clear();
				sProto.Encode(*m_pOctets);
				Size_t iSize = (Size_t)m_pOctets->Size();
				sSocket.Send(m_pOctets->Begin(), iSize);
				sSocket.Close();
				return true;
			}

			//当前无可用服务
			if (!m_vSvrvice.size() && !m_bEchoMode)
			{
				//先发送拒绝连接协议再关闭
				SysProtocol::Sys_RefuseConn sProto(SysProtocol::ERR_SERVICE_INVALID);
				m_pOctets->Clear();
				sProto.Encode(*m_pOctets);
				Size_t iSize = (Size_t)m_pOctets->Size();
				sSocket.Send(m_pOctets->Begin(), iSize);
				sSocket.Close();
				return true;
			}

			//修改连接数
			RegConnection(true);

			//投递连接通知
			GateNotify sNotify(GateNotify::NOTIFY_SESSION_CONNECT);
			sNotify.eConnect.AddrLen = sAddr.CopyAddr(sNotify.eConnect.Address);
			sNotify.eConnect.Handle  = sSocket.Handle();
			UInt32 iIdx = m_iTurnIdx++ % m_iThread + 1;
			SendThreadMsg(iIdx, 0, &sNotify, sizeof(sNotify));

			return true;
		}
		return false;
	}

	Bool HawkGateway::OnGateThreadEvent()
	{
		while (m_pThreadZmq && m_pThreadZmq->PollEvent(HEVENT_READ, 0))
		{
			//设置非空闲
			m_bIdle = false;

			//接收网关线程信息
			UInt32 iThread = 0;
			SID    iSid    = 0;
			if (!RecvThreadMsg(iThread, iSid, m_pOctets))
				return false;

			//会话ID正确校验
			HawkAssert(iSid);
			if (!iSid)
				return false;
			
			//发送给后端服务
			if (m_iCurSvrId)
			{
				SendServerMsg(m_iCurSvrId, iSid, m_pOctets->Begin(), m_pOctets->Size());
			}
			//Echo模式直接回复
			else if (m_bEchoMode)
			{
				SendThreadMsg(iThread, iSid, m_pOctets->Begin(), m_pOctets->Size());
			}			
		}
		return true;
	}

	Bool HawkGateway::OnGateServerEvent()
	{
		while (m_pServerZmq && m_pServerZmq->PollEvent(HEVENT_READ, 0))
		{
			//设置非空闲
			m_bIdle = false;
			
			//接收后端服务信息
			UInt32 iSvrId = 0;
			SID iSid      = 0;
			if (!RecvServerMsg(iSvrId, iSid, m_pOctets))
				return false;

			//会话协议转发
			if (iSid)
			{
				UInt32 iIdx = (iSid-1) % m_iThread + 1;
				SendThreadMsg(iIdx, iSid, m_pOctets->Begin(), m_pOctets->Size());
			}
			//系统消息通知
			else
			{
				HawkAssert(m_pOctets->Size() == sizeof(GateNotify));
				GateNotify* pNotify = (GateNotify*)m_pOctets->Begin();
				if (m_pOctets->Size() != sizeof(GateNotify))
					return false;
				
				//通知网关线程关闭会话连接
				if (pNotify->Type == GateNotify::NOTIFY_SESSION_CLOSE)
				{
					UInt32 iIdx = (iSid-1) % m_iThread + 1;
					SendThreadMsg(iIdx, 0, pNotify, sizeof(*pNotify));
				}				
			}
		}
		return true;
	}

	Bool HawkGateway::OnGateMonitorEvent()
	{
		while (m_pMonitorZmq && m_pMonitorZmq->PollEvent(HEVENT_READ, 0))
		{
			//设置非空闲
			m_bIdle = false;

			//接收头信息
			m_pOctets->Clear();
			Size_t iSize = (Size_t)m_pOctets->Capacity();
			if (!m_pMonitorZmq->Recv(m_pOctets->Begin(), iSize))
				return false;

			//数据校验
			HawkAssert(iSize == sizeof(zmq_event_t));
			zmq_event_t* pZmqEvent = (zmq_event_t*)m_pOctets->Begin();
			if (iSize != sizeof(zmq_event_t))
				return false;

			//设置服务ID已经更新列表
			if (pZmqEvent->event == ZMQ_EVENT_ACCEPTED)
			{
				OnServerConnected(pZmqEvent->data.accepted.fd);
			}
			else if (pZmqEvent->event == ZMQ_EVENT_DISCONNECTED)
			{				
				OnServerDisConnect(pZmqEvent->data.disconnected.fd);
			}
		}
		return true;
	}

	Bool HawkGateway::OnGateIdleEvent()
	{
		//空闲处理
		if (m_bIdle)
		{
			HawkSleep(DEFAULT_SLEEP);
		}		
		return true;
	}

	Bool HawkGateway::OnGatewayClose()
	{
		//通知网关线程退出
		GateNotify sNotify(GateNotify::NOTIFY_SERVICE_EXIT);
		for (UInt32 j=0;m_pThreadZmq && j<(UInt32)m_iThread;j++)
		{
			SendThreadMsg(j+1, 0, &sNotify, sizeof(sNotify));
		}

		//等待线程结束
		for (UInt32 i=0;m_ppThread && i<(UInt32)m_iThread;i++)
		{
			if (m_ppThread[i])
				m_ppThread[i]->Close();
		}

		//关闭性能监视器
		if (m_pProfiler)
			m_pProfiler->Stop();

		//设置运行状态
		m_bRunning = false;

		return true;
	}

	Bool HawkGateway::OnServerConnected(SOCKET hSocket)
	{
		if (hSocket != INVALID_SOCKET)
		{
			if (m_pServerZmq->PollEvent(HEVENT_READ, DEFAULT_TIMEOUT))
			{				
				//接收连接通知
				UInt32 iSvrId = 0;
				SID    iSid   = 0;
				if (!RecvServerMsg(iSvrId, iSid, m_pOctets))
					return false;

				//进行数据校验
				HawkAssert(iSid == 0 && iSvrId && m_pOctets->Size() == sizeof(GateNotify));
				GateNotify* pNotify = (GateNotify*)m_pOctets->Begin();
				if (iSid || !iSvrId || m_pOctets->Size() != sizeof(GateNotify))
					return false;

				//服务挂载通知
				if (pNotify->Type == GateNotify::NOTIFY_SERVICE_ATTACH)
				{
					//判断服务ID重复
					Size_t i = 0;
					for (;i<m_vSvrvice.size();i++)
					{
						if (iSvrId == m_vSvrvice[i].SvrId)
							break;
					}

					//添加到列表
					if (i == m_vSvrvice.size())
					{
						m_vSvrvice.push_back(Service(iSvrId, hSocket));
						HawkFmtPrint("Service Attach, SvrId: %u", iSvrId);
					}

					//设置第一个为服务ID
					if (!m_iCurSvrId)
						m_iCurSvrId = iSvrId;

					return true;
				}				
			}		
		}
		return false;
	}

	Bool HawkGateway::OnServerDisConnect(SOCKET hSocket)
	{
		if (hSocket != INVALID_SOCKET)
		{
			ServiceVec::iterator it = m_vSvrvice.begin();
			for (;it!=m_vSvrvice.end();it++)
			{
				if (it->SvrFd == hSocket)
				{
					//判断当前ID有效
					if (m_iCurSvrId == it->SvrId)
					{
						//重置当前服务ID
						m_iCurSvrId = 0;

						//通知网关线程服务卸载
						GateNotify sNotify(GateNotify::NOTIFY_SERVICE_DETACH);
						for (UInt32 j=0;m_pThreadZmq && j<(UInt32)m_iThread;j++)
						{
							SendThreadMsg(j+1, 0, &sNotify, sizeof(sNotify));
						}						
					}

					//从服务列表删除
					HawkFmtPrint("Service Detach, SvrId: %u", it->SvrId);

					m_vSvrvice.erase(it);

					//切换备份服务
					if (!m_iCurSvrId && m_vSvrvice.size())
						m_iCurSvrId = m_vSvrvice.begin()->SvrId;

					break;
				}
			}
		}
		return false;
	}
}

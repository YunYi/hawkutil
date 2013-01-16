#include "HawkSession.h"
#include "HawkMath.h"
#include "HawkSessionManager.h"
#include "HawkProtocolManager.h"
#include "HawkLoggerManager.h"

namespace Hawk
{	
	HawkSession::HawkSession()
	{
		m_iSid		   = 0;	
		m_pISecurity   = 0;
		m_pOSecurity   = 0;
		m_pProxy	   = 0;
		m_pInputBuf	   = 0;
		m_pOutputBuf   = 0;
		m_pCommonBuf   = 0;
		m_iSessionType = 0;
		m_iState       = 0;	
		m_sCollect.Reset();
	}

	HawkSession::~HawkSession()
	{
		if (m_sSocket.IsValid())
			m_sSocket.Close();

		m_vUdpPacket.clear();

		HAWK_RELEASE(m_pInputBuf);
		HAWK_RELEASE(m_pOutputBuf);
		HAWK_RELEASE(m_pCommonBuf);
		HAWK_RELEASE(m_pISecurity);
		HAWK_RELEASE(m_pOSecurity);
		HAWK_RELEASE(m_pProxy);
	}

	Bool HawkSession::Init(const HawkSocket& sSocket, const SocketAddr& sAddr, UInt8 iSessionType, HawkSessionProxy* pProxy)
	{
		Int32 iBufSize = P_SessionManager->GetBufSize();

		if (m_pInputBuf)
			m_pInputBuf->Clear();
		else
			m_pInputBuf = new OctetsStream(iBufSize);

		if (m_pOutputBuf)
			m_pOutputBuf->Clear();
		else
			m_pOutputBuf = new OctetsStream(iBufSize);

		if (m_pCommonBuf)
			m_pCommonBuf->Clear();
		else
			m_pCommonBuf = new OctetsStream(iBufSize);

		if (m_sSocket.IsValid())
			m_sSocket.Close();

		m_vUdpPacket.clear();
		
		HAWK_RELEASE(m_pISecurity);
		HAWK_RELEASE(m_pOSecurity);

		m_sSocket	   = sSocket;
		m_sAddress	   = sAddr;
		m_iSessionType = iSessionType;

		if (m_sSocket.IsValid())
			m_iState = STATE_INITIALIZED;
		else
			m_iState = STATE_CLOSED;

		if (m_sSocket.IsValid())
		{
			m_sCollect.Reset();

			m_sSocket.SetSendBufferSize(iBufSize);
			m_sSocket.SetRecvBufferSize(iBufSize);

			if (pProxy)
			{
				if (pProxy != m_pProxy)
					SetSessionProxy(pProxy);
			}
			else if (m_pProxy)
			{
				m_pProxy->Init(m_iSid);
			}

			if (m_pProxy)
				m_pProxy->OnStart();
		}

		return true;
	}

	Bool HawkSession::Close()
	{
		if(!IsClosed())
		{
			if(m_pProxy)
				m_pProxy->OnClose();

			m_iSid			= 0;
			m_iState		= STATE_CLOSED;
			m_iSessionType  = 0;
			m_sSocket.Close();

			if (m_pInputBuf)
				m_pInputBuf->Clear();

			if (m_pOutputBuf)
				m_pOutputBuf->Clear();

			if (m_pCommonBuf)
				m_pCommonBuf->Clear();

			m_vUdpPacket.clear();

			HAWK_RELEASE(m_pISecurity);
			HAWK_RELEASE(m_pOSecurity);
		}
		return true;
	}	

	void HawkSession::SetSid(SID iSid)
	{
		m_iSid = iSid;
	}

	SID	HawkSession::GetSid() const
	{
		return m_iSid;
	}
	
	void HawkSession::SetState(UInt32 iState)
	{
		m_iState = iState;
	}

	UInt32 HawkSession::GetState() const
	{
		return m_iState;
	}

	Bool HawkSession::IsActive() const
	{
		return m_iState == STATE_ACTIVATED;
	}

	Bool HawkSession::IsClosing() const
	{
		return m_iState == STATE_CLOSING;
	}

	Bool HawkSession::IsClosed() const
	{
		return m_iState == STATE_CLOSED;
	}

	Bool HawkSession::IsValid() const
	{
		return m_sSocket.IsValid();
	}

	const HawkSession::Collect& HawkSession::GetCollect() const
	{
		return m_sCollect;
	}

	HawkSecurity* HawkSession::GetISecurity()
	{
		return m_pISecurity;
	}

	HawkSecurity* HawkSession::GetOSecurity()
	{
		return m_pOSecurity;
	}

	OctetsStream* HawkSession::GetInputBuf()
	{
		return m_pInputBuf;
	}

	OctetsStream* HawkSession::GetOutputBuf()
	{
		return m_pOutputBuf;
	}

	HawkSocket* HawkSession::GetSocket()
	{
		return &m_sSocket;
	}

	SocketAddr  HawkSession::GetAddress()
	{
		return m_sAddress;
	}

	SOCKET HawkSession::GetHandle()
	{
		if (m_sSocket.IsValid())
			return m_sSocket.Handle();

		return INVALID_SOCKET;
	}

	UInt8 HawkSession::GetSessionType() const
	{
		if (IsActive())
			return m_iSessionType;

		return 0;
	}

	UInt8 HawkSession::GetSocketType() const
	{
		if (IsActive())
			return m_sSocket.GetSocketType();

		return 0;
	}

	void HawkSession::SetSessionProxy(HawkSessionProxy* pProxy)
	{	
		HAWK_RELEASE(m_pProxy);
		if (pProxy)
		{
			m_pProxy = pProxy;
			m_pProxy->AddRef();
			m_pProxy->Init(m_iSid);
		}
	}

	void HawkSession::SetISecurity(HawkSecurity* pSecurity)
	{
		HAWK_RELEASE(m_pISecurity);
		if (pSecurity)
		{
			m_pISecurity = pSecurity;
			m_pISecurity->AddRef();
		}
	}

	void HawkSession::SetOSecurity(HawkSecurity* pSecurity)
	{
		HAWK_RELEASE(m_pOSecurity);
		if (pSecurity)
		{
			m_pOSecurity = pSecurity;
			m_pOSecurity->AddRef();
		}
	}

	Bool HawkSession::IsWaitOutput() const
	{
		if (IsActive())
		{
			if (m_sSocket.IsTcpTransport())
				return m_pOutputBuf->Size() > 0;
			
			if (m_sSocket.IsUdpTransport())
				return m_vUdpPacket.size() > 0;
		}
		return false;
	}

	Bool HawkSession::SendRawData(void* pData, Int32 iSize, const SocketAddr* pAddr)
	{
		if (IsActive())
		{
			//TCP模式
			if (m_sSocket.IsTcpTransport())
			{
				//加密
				HawkOctetsStream xOS(pData, iSize);
				if(m_pOSecurity)
					m_pOSecurity->Update(xOS);

				//数据包太大或者发送数据过慢会丢包
				if (xOS.Size() <= m_pOutputBuf->EmptyCap())
				{
					m_pOutputBuf->Insert(m_pOutputBuf->End(), xOS.Begin(), xOS.End());
					return true;
				}
				else
				{
					P_SessionManager->FmtSessionLog(m_iSid, "TcpOutputBuf Size Error, BufCap: %u, BufSize: %u, DataSize: %u", 
						m_pOutputBuf->Capacity(), m_pOutputBuf->Size(), xOS.Size());
				}
			}
			//UDP模式
			else if (m_sSocket.IsUdpTransport() && pAddr)
			{
				UdpPacket sPacket;
				sPacket.Addr = *pAddr;
				sPacket.Data.Replace(pData, iSize);

				//加密
				if(m_pOSecurity)
					m_pOSecurity->Update(sPacket.Data);

				//数据缓存太大或者发送数据过慢会丢包
				if (sPacket.Data.Size() <= m_pOutputBuf->Capacity())
				{
					m_vUdpPacket.push_back(sPacket);
					return true;
				}
				else
				{
					P_SessionManager->FmtSessionLog(m_iSid, "UdpOutputBuf Size Error, BufCap: %u, BufSize: %u, DataSize: %u",
						m_pOutputBuf->Capacity(), m_pOutputBuf->Size(), sPacket.Data.Size());
				}
			}			
		}
		return false;
	}

	Bool HawkSession::SendProtocol(HawkProtocol* pProtocol, const SocketAddr* pAddr)
	{
		if (IsActive())
		{
			HawkOctetsStream xOS;
			if(!pProtocol->Encode(xOS))
				return false;

			return SendRawData(xOS.Begin(), xOS.Size(), pAddr);		
		}
		return false;
	}

	Bool HawkSession::OnDecodeProtocol(UInt32& iCount)
	{
		if (IsActive())
		{
			if (m_sSocket.IsTcpTransport() && m_iSessionType == TYPE_SERVER)
				return true;

			HawkProtocol* pProto = 0;
			try
			{
				pProto = P_ProtocolManager->Decode(*m_pInputBuf);
				while(pProto && IsActive())
				{
					m_sCollect.OnRecvProto();

					//会话回调
					if (m_pProxy)
						m_pProxy->OnProtocol(pProto, m_sSocket.IsUdpTransport()? &m_sUdpTarget:0);

					//由会话管理器统一分发
					if (m_iSid)
						P_SessionManager->DispatchSessionProtocol(m_iSid, pProto, m_sSocket.IsUdpTransport()? &m_sUdpTarget:0);
					
					//解析协议个数
					iCount ++;

					//继续协议解析
					pProto = P_ProtocolManager->Decode(*m_pInputBuf);
				}					
			}
			catch (HawkException& rhsExcep)
			{
				//异常退出
				HawkPrint(rhsExcep.GetMsg());
				//释放协议
				P_ProtocolManager->ReleaseProto(pProto);
				//记录会话日志
				P_SessionManager->FmtSessionLog(m_iSid, "Session Decode Protocol Error, Msg: %s", rhsExcep.GetMsg().c_str());
				return false;
			}	

			//释放协议
			P_ProtocolManager->ReleaseProto(pProto);
			return true;
		}
		return false;
	}

	Bool HawkSession::OnSessionRead()
	{
		//非活动会话
		if (!IsActive())
			return false;

		//监听会话进入Accept接口
		if (m_sSocket.IsTcpTransport() && m_iSessionType == TYPE_SERVER)
			return false;

		//通知数据可读, 回调可控制数据读取
		if (m_pProxy && !m_pProxy->OnRead())
		{
			//读取重定向
			OctetsStream* pOS = 0;
			Size_t iBufSize   = 0;
			if (m_pProxy->GetRdRedirectOS(pOS, iBufSize) && pOS && iBufSize)
			{
				Size_t iRecv = iBufSize;
				void*  pData = (Char*)pOS->Begin() + pOS->Size();

				//接收数据
				if (m_sSocket.IsTcpTransport())
				{
					if (!m_sSocket.Receive(pData, iRecv))
						return false;
				}
				//UDP不支持输入输出重定向
				else if (m_sSocket.IsUdpTransport())
				{
					T_Exception("Cannot Support Udp Transport Redirect.");	
					return false;
				}

				//解密并拷贝到输入缓冲区
				if (iRecv > 0)
				{
					//记录会话状态
					m_sCollect.OnRecv(iRecv);

					//设置缓冲区
					UInt32 iSize = pOS->Size() + (UInt32)iRecv;
					pOS->Resize(iSize);

					if (!m_pProxy->OnRdRedirect(iRecv))
						return false;
				}
			}
			return true;
		}

		//清理通用缓冲用来接收原始数据
		m_pCommonBuf->Clear();
		Size_t iRecv = m_pCommonBuf->Capacity();

		//接收数据
		if (m_sSocket.IsTcpTransport())
		{
			if (!m_sSocket.Receive(m_pCommonBuf->Begin(), iRecv))
				return false;
		}
		else if (m_sSocket.IsUdpTransport())
		{
			if (!m_sSocket.ReceiveFrom(m_pCommonBuf->Begin(), iRecv, m_sUdpTarget))
				return false;			
		}

		//解密并拷贝到输入缓冲区
		if (iRecv > 0)
		{
			//记录会话状态
			m_sCollect.OnRecv(iRecv);

			//设置通用缓冲区
			m_pCommonBuf->Resize(iRecv);

			//解密
			if (m_pISecurity)
				m_pISecurity->Update(*m_pCommonBuf);

			//填充到输入缓冲区
			UInt32 iFillPos = 0;
			UInt32 iBufSize = m_pCommonBuf->Size();
			while(iBufSize)
			{
				//尽可能填充解码缓冲区
				UInt32 iFillSize = HawkMath::Min<UInt32>(m_pInputBuf->EmptyCap(), iBufSize);
				if (iFillSize)
				{
					void* pData = (Char*)m_pCommonBuf->Begin() + iFillPos;
					m_pInputBuf->Insert(m_pInputBuf->End(), pData, iFillSize);
					iFillPos += iFillSize;
					iBufSize -= iFillSize;
				}

				//协议解析
				if (!m_pProxy || m_pProxy->OnDecode(m_pInputBuf))
				{
					UInt32 iCount = 0;
					if (!OnDecodeProtocol(iCount))
						return false;

					//未全部填充，并且未解析出协议(协议包过大)
					if (iBufSize && !iCount)
					{
						//记录会话日志
						P_SessionManager->FmtSessionLog(m_iSid, "Session Read Packet Overflow, Size: %s", m_pCommonBuf->Size());

						//返回进行错误处理
						return false;
					}
				}				

				//移除输入缓冲的前段空白
				m_pInputBuf->RemoveBlank();				
			}			
		}

		return true;
	}

	Bool HawkSession::OnSessionWrite()
	{
		//非活动会话
		if (!IsActive())
			return false;
		
		//监听会话无写操作
		if (m_sSocket.IsTcpTransport() && m_iSessionType == TYPE_SERVER)
			return false;

		//通知数据可写,回调可控制数据写出
		if (m_pProxy && !m_pProxy->OnWrite())
		{
			//写出重定向
			OctetsStream* pOS = 0;
			Size_t iBufSize   = 0;
			if (m_pProxy->GetWrRedirectOS(pOS, iBufSize) && pOS && iBufSize)
			{
				if (m_sSocket.IsTcpTransport())
				{
					Size_t iSend = iBufSize;
					if (!m_sSocket.Send(pOS->Begin(), iSend))
						return false;

					if (iSend > 0)
					{
						m_sCollect.OnSend(iSend);

						pOS->Erase(pOS->Begin(), iSend);

						if (!m_pProxy->OnWrRedirect(iSend))
							return false;
					}
				}
				//UDP不支持输入输出重定向
				else if (m_sSocket.IsUdpTransport())
				{
					T_Exception("Cannot Support Udp Transport Redirect.");
				}
			}
			return true;
		}

		if (m_sSocket.IsTcpTransport())
		{
			//输出缓冲无数据
			if (!m_pOutputBuf->Size())
				return true;

			Size_t iSend = (Size_t)m_pOutputBuf->Size();
			if (!m_sSocket.Send(m_pOutputBuf->Begin(), iSend))
				return false;

			if (iSend > 0)
			{
				m_sCollect.OnSend(iSend);
				m_pOutputBuf->Erase(m_pOutputBuf->Begin(), iSend);
			}

			return true;
		}
		else if (m_sSocket.IsUdpTransport())
		{
			if (!m_vUdpPacket.size())
				return true;

			m_pOutputBuf->Clear();
			const UdpPacket& sPacket = m_vUdpPacket.front();

			//UDP数据过长会导致丢包
			if (sPacket.Data.Size() <= m_pOutputBuf->Capacity())
			{
				m_sUdpTarget = sPacket.Addr;
				m_pOutputBuf->Insert(m_pOutputBuf->End(), sPacket.Data.Begin(), sPacket.Data.Size());
			}
			m_vUdpPacket.pop_front();

			Size_t iSend = (Size_t)m_pOutputBuf->Size();
			if (!m_sSocket.SendTo(m_pOutputBuf->Begin(), iSend, m_sUdpTarget))
				return false;

			m_sCollect.OnSend(iSend);
			return true;
		}
		return false;
	}

	Bool HawkSession::OnSessionAccept(HawkSocket& sSocket, SocketAddr& sAddr)
	{
		if (!IsActive())
			return false;

		if (m_sSocket.IsTcpTransport() && m_iSessionType == TYPE_SERVER)
		{
			if (!m_sSocket.Accept(sSocket, sAddr))
				return false;

			//不允许连接的处理
			if (m_pProxy && !m_pProxy->OnAccept(sSocket, sAddr))
			{
				sSocket.Close();
				return true;
			}

			m_sCollect.OnAccept();
			return true;
		}
		return false;
	}

	Bool HawkSession::OnSessionNotify(UInt32 iMsg, UInt32 iArgs)
	{
		if (!IsActive())
			return false;

		if (m_pProxy)
			m_pProxy->OnNotify(iMsg, iArgs);

		return true;
	}
}

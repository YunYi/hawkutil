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
			//TCPģʽ
			if (m_sSocket.IsTcpTransport())
			{
				//����
				HawkOctetsStream xOS(pData, iSize);
				if(m_pOSecurity)
					m_pOSecurity->Update(xOS);

				//���ݰ�̫����߷������ݹ����ᶪ��
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
			//UDPģʽ
			else if (m_sSocket.IsUdpTransport() && pAddr)
			{
				UdpPacket sPacket;
				sPacket.Addr = *pAddr;
				sPacket.Data.Replace(pData, iSize);

				//����
				if(m_pOSecurity)
					m_pOSecurity->Update(sPacket.Data);

				//���ݻ���̫����߷������ݹ����ᶪ��
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

					//�Ự�ص�
					if (m_pProxy)
						m_pProxy->OnProtocol(pProto, m_sSocket.IsUdpTransport()? &m_sUdpTarget:0);

					//�ɻỰ������ͳһ�ַ�
					if (m_iSid)
						P_SessionManager->DispatchSessionProtocol(m_iSid, pProto, m_sSocket.IsUdpTransport()? &m_sUdpTarget:0);
					
					//����Э�����
					iCount ++;

					//����Э�����
					pProto = P_ProtocolManager->Decode(*m_pInputBuf);
				}					
			}
			catch (HawkException& rhsExcep)
			{
				//�쳣�˳�
				HawkPrint(rhsExcep.GetMsg());
				//�ͷ�Э��
				P_ProtocolManager->ReleaseProto(pProto);
				//��¼�Ự��־
				P_SessionManager->FmtSessionLog(m_iSid, "Session Decode Protocol Error, Msg: %s", rhsExcep.GetMsg().c_str());
				return false;
			}	

			//�ͷ�Э��
			P_ProtocolManager->ReleaseProto(pProto);
			return true;
		}
		return false;
	}

	Bool HawkSession::OnSessionRead()
	{
		//�ǻ�Ự
		if (!IsActive())
			return false;

		//�����Ự����Accept�ӿ�
		if (m_sSocket.IsTcpTransport() && m_iSessionType == TYPE_SERVER)
			return false;

		//֪ͨ���ݿɶ�, �ص��ɿ������ݶ�ȡ
		if (m_pProxy && !m_pProxy->OnRead())
		{
			//��ȡ�ض���
			OctetsStream* pOS = 0;
			Size_t iBufSize   = 0;
			if (m_pProxy->GetRdRedirectOS(pOS, iBufSize) && pOS && iBufSize)
			{
				Size_t iRecv = iBufSize;
				void*  pData = (Char*)pOS->Begin() + pOS->Size();

				//��������
				if (m_sSocket.IsTcpTransport())
				{
					if (!m_sSocket.Receive(pData, iRecv))
						return false;
				}
				//UDP��֧����������ض���
				else if (m_sSocket.IsUdpTransport())
				{
					T_Exception("Cannot Support Udp Transport Redirect.");	
					return false;
				}

				//���ܲ����������뻺����
				if (iRecv > 0)
				{
					//��¼�Ự״̬
					m_sCollect.OnRecv(iRecv);

					//���û�����
					UInt32 iSize = pOS->Size() + (UInt32)iRecv;
					pOS->Resize(iSize);

					if (!m_pProxy->OnRdRedirect(iRecv))
						return false;
				}
			}
			return true;
		}

		//����ͨ�û�����������ԭʼ����
		m_pCommonBuf->Clear();
		Size_t iRecv = m_pCommonBuf->Capacity();

		//��������
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

		//���ܲ����������뻺����
		if (iRecv > 0)
		{
			//��¼�Ự״̬
			m_sCollect.OnRecv(iRecv);

			//����ͨ�û�����
			m_pCommonBuf->Resize(iRecv);

			//����
			if (m_pISecurity)
				m_pISecurity->Update(*m_pCommonBuf);

			//��䵽���뻺����
			UInt32 iFillPos = 0;
			UInt32 iBufSize = m_pCommonBuf->Size();
			while(iBufSize)
			{
				//�����������뻺����
				UInt32 iFillSize = HawkMath::Min<UInt32>(m_pInputBuf->EmptyCap(), iBufSize);
				if (iFillSize)
				{
					void* pData = (Char*)m_pCommonBuf->Begin() + iFillPos;
					m_pInputBuf->Insert(m_pInputBuf->End(), pData, iFillSize);
					iFillPos += iFillSize;
					iBufSize -= iFillSize;
				}

				//Э�����
				if (!m_pProxy || m_pProxy->OnDecode(m_pInputBuf))
				{
					UInt32 iCount = 0;
					if (!OnDecodeProtocol(iCount))
						return false;

					//δȫ����䣬����δ������Э��(Э�������)
					if (iBufSize && !iCount)
					{
						//��¼�Ự��־
						P_SessionManager->FmtSessionLog(m_iSid, "Session Read Packet Overflow, Size: %s", m_pCommonBuf->Size());

						//���ؽ��д�����
						return false;
					}
				}				

				//�Ƴ����뻺���ǰ�οհ�
				m_pInputBuf->RemoveBlank();				
			}			
		}

		return true;
	}

	Bool HawkSession::OnSessionWrite()
	{
		//�ǻ�Ự
		if (!IsActive())
			return false;
		
		//�����Ự��д����
		if (m_sSocket.IsTcpTransport() && m_iSessionType == TYPE_SERVER)
			return false;

		//֪ͨ���ݿ�д,�ص��ɿ�������д��
		if (m_pProxy && !m_pProxy->OnWrite())
		{
			//д���ض���
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
				//UDP��֧����������ض���
				else if (m_sSocket.IsUdpTransport())
				{
					T_Exception("Cannot Support Udp Transport Redirect.");
				}
			}
			return true;
		}

		if (m_sSocket.IsTcpTransport())
		{
			//�������������
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

			//UDP���ݹ����ᵼ�¶���
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

			//���������ӵĴ���
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

#include "HawkZmq.h"
#include "HawkOSOperator.h"
#include "HawkLoggerManager.h"
#include "zmq.h"

namespace Hawk
{
	HawkZmq::HawkZmq()
	{		
		m_pHandle	 = 0;
		m_iErrCode  = 0;
		m_iType	 = 0;
		m_iRecvMore = 0;		 
		m_pRecvBuf  = 0;
		m_sErrMsg   = "";
	}

	HawkZmq::~HawkZmq()
	{
		Close();
	}

	Int32 HawkZmq::GetErrCode() const
	{
		return m_iErrCode;
	}

	Bool HawkZmq::Init(Int32 iType, void* pHandle)
	{
		A_Exception(m_pHandle == 0);
		m_pHandle  = pHandle;
		m_iType    = iType;
		return m_pHandle != 0;
	}

	void* HawkZmq::GetHandle()
	{
		return m_pHandle;
	}

	Int32 HawkZmq::GetType() const
	{
		return m_iType;
	}

	Bool HawkZmq::FillErr()
	{
		m_iErrCode = zmq_errno();

#ifdef _DEBUG
		m_sErrMsg  = zmq_strerror(m_iErrCode);
#endif

		return true;
	}

	Bool HawkZmq::Close()
	{
		if (m_pHandle)
		{
			Int32 iRet = zmq_close(m_pHandle);
			m_pHandle = 0;

			if(iRet == HAWK_ERROR)
			{
				FillErr();
				return false;
			}
			return true;
		}
		return false;
	}

	Bool HawkZmq::SetIdentity(const void* pOptVal, Int32 iSize)
	{
		if(pOptVal && iSize > 0)
		{
			return SetOption(ZMQ_IDENTITY, pOptVal, iSize);
		}

		return false;
	}

	Bool HawkZmq::SetOption(Int32 iOption, const void* pOptVal, Size_t iSize)
	{
		if (m_pHandle)
		{
			Int32 iRet = zmq_setsockopt(m_pHandle, iOption, pOptVal, iSize);
			if(iRet == HAWK_ERROR)
			{
				FillErr();
				return false;
			}
			return true;
		}
		return false;
	}

	Bool HawkZmq::GetOption(Int32 iOption,void* pOptVal,Size_t& iSize)
	{
		if (m_pHandle)
		{
			Int32 iRet = zmq_getsockopt(m_pHandle, iOption, pOptVal, &iSize);
			if(iRet == HAWK_ERROR)
			{
				FillErr();
				return false;
			}
			return true;
		}
		return false;
	}

	Bool HawkZmq::Bind(const AString& sAddr)
	{
		if (m_pHandle)
		{
			Int32 iRet = zmq_bind(m_pHandle, sAddr.c_str());
			if(iRet == HAWK_ERROR)
			{
				FillErr();
				return false;
			}
			return true;
		}
		return false;
	}

	Bool HawkZmq::Connect(const AString& sAddr)
	{
		if (m_pHandle)
		{
			Int32 iRet = zmq_connect(m_pHandle,sAddr.c_str());
			if(iRet == HAWK_ERROR)
			{
				FillErr();
				return false;
			}
			return true;
		}
		return false;
	}		

	Bool HawkZmq::IsWaitRecv() const
	{
		return m_iRecvMore > 0;
	}

	UInt32 HawkZmq::PollEvent(UInt32 iEvents, Int32 iTimeout)
	{
		if (m_pHandle)
		{
			zmq_pollitem_t items[] = 
			{
				{m_pHandle, 0, iEvents, 0},
			};

			Int32 iRet = zmq_poll(items, 1, iTimeout);
			if(iRet == -1)
			{
				FillErr();
				return 0;
			}
			return items[0].revents;
		}
		return 0;
	}

	Bool HawkZmq::StartMonitor(const AString& sAddr, UInt32 iEvents)
	{
		if (m_pHandle)
		{
			if (zmq_socket_monitor(m_pHandle, sAddr.c_str(), (Int32)iEvents) < 0)
				return false;

			return true;
		}
		return false;
	}

	Bool HawkZmq::StopMonitor()
	{
		if (m_pHandle)
		{
			if (zmq_socket_monitor(m_pHandle, 0, 0) < 0)
				return false;

			return true;
		}
		return false;
	}

	Bool HawkZmq::Send(void* pBuf, Size_t iSize, Int32 iFlag)
	{
		if (m_pHandle && pBuf && iSize)
		{
			Int32 iSend = zmq_send(m_pHandle, pBuf, iSize, iFlag);
			if(iSend < 0)
			{
				FillErr();
				return false;
			}
			return true;
		}
		return false;
	}

	Bool HawkZmq::Recv(void* pBuf, Size_t& iSize, Int32 iFlag)
	{
		if (m_pHandle && pBuf && iSize)
		{
			Int32 iRecv = zmq_recv(m_pHandle, pBuf, iSize, iFlag);
			if (iRecv < 0) 
				return false;

			iSize = iRecv;

			Size_t iLen = sizeof(m_iRecvMore);
			Int32 iRet  = zmq_getsockopt(m_pHandle, ZMQ_RCVMORE, &m_iRecvMore, &iLen);
			if(iRet == -1)
			{
				FillErr();
				return false;
			}
			return true;
		}
		return false;
	}

	Bool HawkZmq::SendProtocol(HawkProtocol* pProto, Int32 iFlag)
	{
		if (pProto)
		{
			HawkOctetsStream xOS;
			if(pProto->Encode(xOS))
			{
				return Send(xOS.Begin(), (Size_t)xOS.Size(), iFlag);
			}
		}
		return false;
	}

	Bool HawkZmq::RecvProtocol(HawkProtocol*& pProto, Int32 iFlag)
	{
		if(!m_pRecvBuf)
			m_pRecvBuf = new OctetsStream(RCVBUF_SIZE);

		if (m_pRecvBuf)
		{
			m_pRecvBuf->Clear();
			Size_t iSize = (Size_t)m_pRecvBuf->Capacity();
			if (Recv(m_pRecvBuf->Begin(), iSize, iFlag) && iSize)
			{
				m_pRecvBuf->Resize((UInt32)iSize);
				pProto = P_ProtocolManager->Decode(*m_pRecvBuf);
				if (pProto)
					return true;
			}
		}		
		return false;
	}
}

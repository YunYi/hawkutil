#include "HawkLogProxy.h"

namespace Hawk
{
	HawkLogProxy::HawkLogProxy() : m_iLogId(0), m_bConsole(false), m_bShowThread(false)
	{
		m_pLock = new HawkMutex;

#ifdef _DEBUG
		m_bConsole = true;
#endif
	}

	HawkLogProxy::~HawkLogProxy()
	{
		m_sSocket.Close();
		HAWK_RELEASE(m_pLock);
	}

	Bool  HawkLogProxy::Init(const AString& sAddr, UInt16 iLogId)
	{
		HawkAssert(sAddr.size());
		m_sAddr  = SocketAddr(sAddr);
		m_iLogId = iLogId;

		if(!m_sSocket.Create(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) 
		{
			HawkPrint("LogProxy Create Socket Error.");
			return false;
		}

		m_sSocket.SetNoDelay(true);
		m_sSocket.SetBlocking(false);

		HawkSocket::MaximizeSndBuf(m_sSocket.Handle());

		return true;
	}

	void  HawkLogProxy::EnableConsole(Bool bEnable)
	{
		m_bConsole = bEnable;
	}

	void  HawkLogProxy::SetShowThread(Bool bShow)
	{
		m_bShowThread = bShow;
	}

	Bool  HawkLogProxy::FmtMsg(const Char* pKey, const Char* pFmt, ...)
	{
		va_list args;
		Char sMsg[LOG_DEFAULT_SIZE] = {0};
		va_start(args,(const Char*)pFmt);
		_vsnprintf((Char*)sMsg, LOG_DEFAULT_SIZE-1, (Char*)pFmt, args);
		va_end(args);

		return SendLog(LT_MSG, pKey, sMsg);
	}

	Bool  HawkLogProxy::FmtWarn(const Char* pKey, const Char* pFmt, ...)
	{
		va_list args;
		Char sMsg[LOG_DEFAULT_SIZE] = {0};		
		va_start(args, (const Char*)pFmt);
		_vsnprintf((Char*)sMsg, LOG_DEFAULT_SIZE-1, (Char*)pFmt, args);
		va_end(args);

		return SendLog(LT_WARN, pKey, sMsg);
	}

	Bool  HawkLogProxy::FmtError(const Char* pKey, const Char* pFmt, ...)
	{
		va_list args;
		Char sMsg[LOG_DEFAULT_SIZE] = {0};		
		va_start(args, (const Char*)pFmt);
		_vsnprintf((Char*)sMsg, LOG_DEFAULT_SIZE-1, (Char*)pFmt, args);
		va_end(args);

		return SendLog(LT_ERROR, pKey, sMsg);
	}

	Bool  HawkLogProxy::SendLog(UInt8 iType, const Char* pKey, const Char* pMsg)
	{
		HawkAssert(pKey && pMsg);
		if (m_bConsole)
		{
			if (m_bShowThread)
			{
				if (iType == LT_ERROR)
					HawkFmtPrint("[***] [%u] %s, %s", HawkOSOperator::GetThreadId(), pKey, pMsg);
				else
					HawkFmtPrint("[%u] %s, %s", HawkOSOperator::GetThreadId(), pKey, pMsg);
			}
			else
			{
				if (iType == LT_ERROR)
					HawkFmtPrint("[***] %s, %s", pKey, pMsg);
				else
					HawkFmtPrint("%s, %s", pKey, pMsg);
			}
		}

		SysProtocol::Sys_LogMsg sCmd(m_iLogId, iType, (Utf8*)pKey, (Utf8*)pMsg);
		OctetsStream xOS;
		sCmd.Encode(xOS);

		Size_t iSize = (Size_t)xOS.Size();
		if(iSize) 
		{
			HawkAutoMutex(lock,m_pLock);
			return m_sSocket.SendTo(xOS.Begin(), iSize, m_sAddr);
		}

		return false;
	}
}

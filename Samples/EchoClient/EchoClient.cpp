#include "EchoProto.h"

namespace Hawk
{
	vector<SID>	g_Sid;
	Bool		g_Running = true;
	HawkThread*	g_Thread  = 0;
	HawkMutex*  g_Lock	  = 0;

	PVoid EchoClientLoop(void* pArg)
	{
		while(HawkOSOperator::WaitKeyboardInput("",false) != 'S');

		while(g_Running)
		{
			for (UInt32 i=0;i<g_Sid.size();i++)
			{
				if(g_Sid[i] && !SendProto(g_Sid[i]))
					g_Sid[i] = 0;
			}

			if (g_Sid.size() > 1)
			{
				HawkSleep(200);
			}
			else
			{
				HawkSleep(1000);
			}			
		}
		return 0;
	}

	class NetworkProxy : public HawkNetworkProxy
	{
	public:
		//»á»°¿ªÆô
		virtual Bool OnSessionStart(SID iSid, UInt8 iSessionType, UInt8 iSocketType, const SocketAddr& sAddr)
		{
			if (iSessionType == HawkSession::TYPE_CLIENT)
			{
				HawkAutoMutex(lock,g_Lock);
				g_Sid.push_back(iSid);
			}

			HawkFmtPrint("SessionStart: %u, Address: %s",iSid, sAddr.ToString().c_str());
			return true;
		}

		virtual Bool OnSessionClose(SID iSid)
		{
			HawkFmtPrint("SessionClose: %u", iSid);
			return true;
		}

		virtual Bool OnSessionProtocol(SID iSid, Protocol* pProto, const SocketAddr* pAddr = 0)
		{
#ifdef _DEBUG
			ProtoType iType = pProto->GetType();
			if (iType == PROTO_A)
			{
				ProtoA* pA = (ProtoA*)pProto;
				HawkFmtPrint("Msg: %s",pA->m_sMsg.c_str());
			}
			else if (iType == PROTO_B)
			{
				ProtoB* pB = (ProtoB*)pProto;
				HawkFmtPrint("Size: %d, Time: %d",pB->m_iSize, pB->m_iTime);
			}
			else if (iType == PROTO_C)
			{
				ProtoC* pC = (ProtoC*)pProto;
				HawkFmtPrint("X: %f, Y: %f, Z: %f",pC->m_fX, pC->m_fY, pC->m_fZ);
			}
#endif
			P_ProtocolManager->ReleaseProto(pProto);
			return true;
		}
	};	

	Bool RunClient(const AString& sAddr,Int32 iCount = 1)
	{
		REGISTER_PROTO(ProtoA);
		REGISTER_PROTO(ProtoB);
		REGISTER_PROTO(ProtoC);

		g_Lock   = new HawkMutex;
		g_Thread = new HawkThread(EchoClientLoop);
		g_Thread->Start();

		NetworkProxy* pProxy = new NetworkProxy;
		P_SessionManager->SetNetworkProxy(pProxy);
		HAWK_RELEASE(pProxy);

		P_SessionManager->Init();
		P_SessionManager->Run();		

		for (Int32 i=0;i<iCount;i++)
		{
			P_SessionManager->StartSession(sAddr, HawkSession::TYPE_CLIENT, HawkSocket::TYPE_TCP);

			HawkSleep(10);
		}		

		return true;
	}
}

int main(int argc, Char* argv[])
{
	HawkUtil::Init();

	AString sIpAddr = "";
	if (argc >= 2) 
		sIpAddr = argv[1];

	if (!sIpAddr.size())
		sIpAddr = HawkOSOperator::ConsoleInput();

	Int32 iCount = 1;
	if (argc >= 3)
		iCount = HawkStringUtil::StringToInt<AString>(argv[2]);

	if(!RunClient(sIpAddr, iCount))
		HawkPrint("EchoClient Start Failed.");
	else
		HawkPrint("EchoClient Start Success.");

	while(HawkOSOperator::WaitKeyboardInput("",false) != 'Q');
	
	g_Running = false;

	if (g_Thread)
	{
		g_Thread->Close();
		HAWK_RELEASE(g_Thread);
	}	
	HAWK_RELEASE(g_Lock);

	HawkUtil::Stop();
	HawkUtil::Release();
	return 0;
}


#include "EchoProto.h"

namespace Hawk 
{
	class NetworkProxy : public HawkNetworkProxy
	{
	public:
		NetworkProxy() : m_iLinkCount(0) {}

	public:
		virtual Bool OnSessionStart(SID iSid, UInt8 iSessionType, UInt8 iSocketType, const SocketAddr& sAddr)
		{
			UInt32 iCount = HawkAtomic::Inc<UInt32>(&m_iLinkCount);
			HawkFmtPrint("SessionStart: %u, Address: %s, LinkCount: %u",iSid, sAddr.ToString().c_str(), iCount);
			return true;
		}

		virtual Bool OnSessionClose(SID iSid)
		{
			UInt32 iCount = HawkAtomic::Dec<UInt32>(&m_iLinkCount);
			HawkFmtPrint("SessionClose: %u, LinkCount: %u",iSid, iCount);
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

			pProto->Send(iSid, pAddr);
			P_ProtocolManager->ReleaseProto(pProto);
			return true;
		}

	protected:
		volatile UInt32 m_iLinkCount;
	};

	Bool RunServer(const AString& sAddr)
	{
		REGISTER_PROTO(ProtoA);
		REGISTER_PROTO(ProtoB);
		REGISTER_PROTO(ProtoC);

		NetworkProxy* pProxy = new NetworkProxy;
		P_SessionManager->SetNetworkProxy(pProxy);
		HAWK_RELEASE(pProxy);

		P_SessionManager->Init(4);
		P_SessionManager->Run();

		P_SessionManager->StartSession(sAddr, HawkSession::TYPE_SERVER, HawkSocket::TYPE_TCP);

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

	if(!RunServer(sIpAddr))
		HawkPrint("EchoServer Start Failed.");
	else
		HawkPrint("EchoServer Start Success.");

	while(HawkOSOperator::WaitKeyboardInput("",false) != 'Q');
	
	HawkUtil::Stop();
	HawkUtil::Release();

	return 0;
}

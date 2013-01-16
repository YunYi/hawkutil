#include "HawkUtil.h"
#include "HawkLog.h"
#include "HawkGeometry.h"
#include "HawkProfiler.h"
#include "HawkGateProxy.h"
#include "EchoProto.h"

void DoLogProxy()
{
	HawkLogProxy log;
	log.Init("127.0.0.1:9595", 1);
	log.EnableConsole(true);
	log.SetShowThread(true);

	while(true)
	{
		AString sKey = HawkStringUtil::RandomString<AString>(HawkRand::RandInt(32, 64));
		AString sMsg = HawkStringUtil::RandomString<AString>(HawkRand::RandInt(128,1024));
		log.FmtMsg(sKey.c_str(), sMsg.c_str());
		HawkSleep(1000);
	}
}

void DoGatewayTest()
{
	REGISTER_PROTO(ProtoA);
	REGISTER_PROTO(ProtoB);
	REGISTER_PROTO(ProtoC);

	HawkGateProxy sProxy;
	sProxy.Init("127.0.0.1:9596", 1);

	Hawk::SID iSid   = 0;
	Protocol* pProto = 0;
	while (true)
	{
		if (sProxy.RecvProtocol(iSid, pProto, -1))
		{
			sProxy.SendProtocol(iSid, pProto);
			P_ProtocolManager->ReleaseProto(pProto);
		}
	}
}

int main(int argc, Char* argv[])
{
	HawkUtil::Init();

	DoGatewayTest();

	HawkUtil::Stop();
	HawkUtil::Release();

	return 0;
}

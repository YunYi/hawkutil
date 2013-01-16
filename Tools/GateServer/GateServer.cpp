#include "HawkUtil.h"
#include "HawkGateway.h"

HawkThread* g_DbgThread = 0;
PVoid DoKeyDownExit(void* pArg)
{
	HawkGateway* pGateway = (HawkGateway*)pArg;
	while (pGateway)
	{
		if(HawkOSOperator::WaitKeyboardInput("",false) == 'Q')
		{
			pGateway->Stop();
			return 0;
		}
	}
	return 0;
}

int main(int argc, Char* argv[])
{
	HawkUtil::Init();	

	if (argc < 3) 
	{
		printf("usage: gateserver <frontend-ip:port> <backend-ip:port> <profiler-ip:port>.\r\n");
		HawkOSOperator::WaitKeyboardInput();
		return -1;
	}

	HawkOSOperator::ClearConsole();
	HawkOSOperator::Gotoxy(0, 0);

	HawkPrint("==============================================================");
	HawkPrint("==================Gateway, Author: Daijunhua==================");
	HawkPrint("==============================================================");

	HawkGateway* pGateway = new HawkGateway;
	if (pGateway->Init(argv[1], argv[2], 4, true))
	{
#ifdef _DEBUG
		g_DbgThread = new HawkThread(DoKeyDownExit);
		if (g_DbgThread)
			g_DbgThread->Start(pGateway);
#endif	

		if (argc >= 4)
			pGateway->TurnOnProfiler(argv[3]);

		pGateway->Run();
	}

	HAWK_RELEASE(g_DbgThread);
	HAWK_RELEASE(pGateway);
	
	HawkUtil::Stop();
	HawkUtil::Release();

	return 0;
}

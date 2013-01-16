#include "HawkDetour.h"
#include "detours.h"

namespace Hawk
{
	HawkDetour::HawkDetour()
	{
	}

	HawkDetour::~HawkDetour()
	{
	}

	Bool  HawkDetour::DetourApi(void** pSysAddr, void* pUsrAddr,Bool bDetour)
	{
		if (bDetour)
		{
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());

			if (DetourAttach(pSysAddr, pUsrAddr) != NO_ERROR)
			{
				assert(false && "DetourAttach Failed");
				return false;
			}

			if (DetourTransactionCommit() != NO_ERROR)
			{
				assert(false && "DetourCommit Failed");
				return false;
			}

			return true;
		}
		else
		{
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());

			if(DetourDetach(pSysAddr, pUsrAddr) != NO_ERROR)
			{
				assert(false && "DetourAttach Failed");
				return false;
			}

			if (DetourTransactionCommit() != NO_ERROR)
			{
				assert(false && "DetourCommit Failed");
				return false;
			}

			return true;
		}
		return false;
	}
}

#include "HawkZlib.h"
#include "zlib.h"

namespace Hawk
{
	ULong HawkZip::GetRequiredSize(ULong iOriginal)
	{
		return compressBound(iOriginal);
	}

	Bool HawkZip::Compress(void* pOut, ULong& iOutLen, const void* pIn, ULong iInLen)
	{
		Int32 code  = compress2((Byte*)pOut, &iOutLen,(const Byte*)pIn, iInLen, Z_DEFAULT_COMPRESSION);
		if (code == Z_OK)
			return true;

		return false;
	}

	Bool HawkZip::UnCompress(void* pOut, ULong& iOutLen, const void* pIn, ULong iInLen)
	{ 
		Int32 code  = uncompress((Bytef *)pOut, &iOutLen,(const Bytef *)pIn, iInLen);
		if (code == Z_OK)
			return true;

		return false;
	}
}

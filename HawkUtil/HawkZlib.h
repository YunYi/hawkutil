#ifndef HAWK_ZIP_H
#define HAWK_ZIP_H

#include "HawkStdHeader.h"

namespace Hawk
{
	/************************************************************************/
	/* ZIPѹ���ͽ�ѹ                                                        */
	/************************************************************************/
	class UTIL_API HawkZip 
	{
	public:
		//ѹ����� Ԥ�����ݴ�С
		static ULong GetRequiredSize(ULong iOriginal);
		
		//ѹ��
		static Bool  Compress(void* pOut, ULong& iOutLen, const void* pIn, ULong iInLen);

		//��ѹ��
		static Bool  UnCompress(void* pOut, ULong& iOutLen, const void* pIn, ULong iInLen);
	};
}
#endif

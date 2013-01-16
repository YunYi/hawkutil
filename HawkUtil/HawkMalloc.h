#ifndef HAWK_MALLOC_H
#define HAWK_MALLOC_H

#include "HawkStdHeader.h"

namespace Hawk
{
	/************************************************************************/
	/* Malloc相关操作封装													*/
	/************************************************************************/
	//内存分配
	void*  UTIL_API HawkMalloc(Size_t iSize);

	//内存分配
	void*  UTIL_API HawkCalloc(Size_t iNum, Size_t iSize);

	//内存重分配
	void*  UTIL_API HawkRealloc(void* pMem, Size_t iSize);

	//内存释放
	void   UTIL_API HawkFree(void* pMem);
}

//////////////////////////////////////////////////////////////////////////
#ifdef HAWK_MALLOC
	using namespace Hawk;

	void* operator  new(Size_t iSize) throw(std::bad_alloc);
	void* operator  new[](Size_t iSize) throw(std::bad_alloc);
	void  operator  delete(void* pMem) throw();
	void  operator  delete[](void* pMem) throw();

	void* operator  new(Size_t iSize, const Char* szFile, Int32 iLine);
	void* operator  new[](Size_t iSize, const Char* szFile, Int32 iLine);
	void  operator  delete(void* pMem, const Char* szFile, Int32 iLine);
	void  operator  delete[](void* pMem, const Char* szFile, Int32 iLine);
#endif 
//////////////////////////////////////////////////////////////////////////

#endif

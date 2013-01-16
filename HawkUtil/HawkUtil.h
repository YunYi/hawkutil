#ifndef HAWK_UTIL_H
#define HAWK_UTIL_H

#if !defined(UTIL_EXPORT)
#	include "HawkAllocator.h"
#	include "HawkAnyType.h"
#	include "HawkApp.h"
#	include "HawkAppObj.h"
#	include "HawkAtomic.h"
#	include "HawkBase64.h"
#	include "HawkByteOrder.h"
#	include "HawkCallback.h"
#	include "HawkConfig.h"
#	include "HawkCounter.h"
#	include "HawkDatabase.h"
#	include "HawkDBManager.h"
#	include "HawkDemutexTable.h"
#	include "HawkDiskFile.h"
#	include "HawkEventNotice.h"
#	include "HawkEventThread.h"
#	include "HawkException.h"
#	include "HawkFile.h"
#	include "HawkHash.h"
#	include "HawkHeapArray.h"
#	include "HawkHMACSHA1Security.h"
#	include "HawkIPAddr.h"
#	include "HawkJson.h"
#	include "HawkLogger.h"
#	include "HawkLoggerManager.h"
#	include "HawkMalloc.h"
#	include "HawkManagerBase.h"
#	include "HawkMarshal.h"
#	include "HawkMarshalData.h"
#	include "HawkMath.h"
#	include "HawkMd5.h"
#	include "HawkMemoryFile.h"
#	include "HawkMsg.h"
#	include "HawkMsgManager.h"
#	include "HawkMutex.h"
#	include "HawkMysql.h"
#	include "HawkNetworkProxy.h"
#	include "HawkObjBase.h"
#	include "HawkObjManager.h"
#	include "HawkOctets.h"
#	include "HawkOctetsStream.h"
#	include "HawkOSOperator.h"
#	include "HawkParamVector.h"
#	include "HawkProtocol.h"
#	include "HawkProtocolManager.h"
#	include "HawkRand.h"
#	include "HawkRange.h"
#	include "HawkRC4Security.h"
#	include "HawkRefCounter.h"
#	include "HawkRingBuffer.h"
#	include "HawkRWEFds.h"
#	include "HawkRWLock.h"
#	include "HawkScope.h"
#	include "HawkSecurity.h"
#	include "HawkSession.h"
#	include "HawkSessionManager.h"
#	include "HawkSessionProxy.h"
#	include "HawkSHA1Security.h"
#	include "HawkSignal.h"
#	include "HawkSize.h"
#	include "HawkSmartPtr.h"
#	include "HawkSocket.h"
#	include "HawkSocketAddr.h"
#	include "HawkSocketPair.h"
#	include "HawkSpinLock.h"
#	include "HawkSqlite.h"
#	include "HawkStdHeader.h"
#	include "HawkStreamCompress.h"
#	include "HawkStringUtil.h"
#	include "HawkSysProtocol.h"
#	include "HawkTask.h"
#	include "HawkThread.h"
#	include "HawkThreadPool.h"
#	include "HawkTimerManager.h"
#	include "HawkUtil.h"
#	include "HawkValueHolder.h"
#	include "HawkXID.h"
#	include "HawkXmlAttribute.h"
#	include "HawkXmlDocument.h"
#	include "HawkXmlElement.h"
#	include "HawkXmlFile.h"
#	include "HawkZlib.h"
#	include "HawkZmq.h"
#	include "HawkZmqManager.h"
	using namespace Hawk;
#else
#	include "HawkStdHeader.h"
#endif

namespace Hawk
{
	/************************************************************************/
	/* Ӧ�õײ��ʼ��,����,ֹͣ,�ͷŵ�ͳһ�ӿڷ�װ                          */
	/************************************************************************/
	class UTIL_API HawkUtil
	{
	public:
		//��ʼ��
		static void Init();

		//���ڸ���
		static void Tick(UInt32 iPeriod);

		//ֹͣ����
		static void Stop();

		//�ͷ���Դ
		static void Release();
	};
};
#endif

#ifndef HAWK_LOGSERVER_H
#define HAWK_LOGSERVER_H

#include "HawkUtil.h"

namespace Hawk
{
	/************************************************************************/
	/* ��־��������װ, ���ݿ�ģʽ��Ĭ��дlogs��                             */
	/************************************************************************/
	class UTIL_API HawkLogServer : public HawkRefCounter
	{
	public:
		//����
		HawkLogServer();

		//����
		virtual ~HawkLogServer();

	public:
		//��ʼ���ļ���־
		virtual Bool Init(const AString& sSvrAddr, const AString& sLogFile, Int32 iCacheSize = MEGABYTE, Int32 iCacheTime = 60000);

		//��ʼ�����ݿ���־
		virtual Bool Init(const AString& sSvrAddr, const HawkDBConn& sConn, Int32 iCacheSize = MEGABYTE, Int32 iCacheTime = 60000);

		//������־������
		virtual Bool Run();

		//�ر���־������
		virtual Bool Stop();

		//������־��ӡ���
		virtual Bool EnableConsole(Bool bEnable);

	protected:
		//������־
		virtual Bool CacheLogs(Int32 iLogId, Int32	iType, const UString& sKey, const UString& sMsg);

		//��־���
		virtual Bool FlushLogs();

	protected:
		//����״̬
		volatile Bool m_bRunning;
		//��ӡ����
		Bool		  m_bConsole;
		//UDPģʽ��־����
		HawkSocket	  m_sSocket;					
		//UDP���ݽ���Buffer
		OctetsStream* m_pRecvBuf;
		//��־����Buffer
		HawkOctets*   m_pLogCache;
		//��־��ʽ��Buffer
		Utf8*		  m_pFmtBuf;
		//��־�洢���ݿ�
		HawkDatabase* m_pLogDB;
		//��־�洢�ļ�
		HawkDiskFile* m_pLogFile;
		//����ˢ���¼�
		Int32		  m_iCacheTime;
	};
}
#endif

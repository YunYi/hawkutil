#ifndef HAWK_LOGPROXY_H
#define HAWK_LOGPROXY_H

#include "HawkUtil.h"

namespace Hawk
{
	/************************************************************************/
	/* ��־��¼����˷�װ(��־����ΪUtf8��ʽ)                               */
	/************************************************************************/
	class UTIL_API HawkLogProxy : public HawkRefCounter
	{
	public:
		//����
		HawkLogProxy();

		//����
		virtual ~HawkLogProxy();

		//��־���
		enum LogType
		{
			LT_MSG = 0,
			LT_WARN,
			LT_ERROR,
		};

	public:
		//��־�����ʼ��
		virtual Bool  Init(const AString& sAddr, UInt16 iLogId = 0);		
		
		//���ñ��ش�ӡ
		virtual void  EnableConsole(Bool bEnable);

		//���ô�ӡ�߳���Ϣ
		virtual void  SetShowThread(Bool bShow);

	public:
		//��ͨ��־(Utf8����)
		virtual Bool  FmtMsg(const Char* pKey,const Char* pFmt, ...);
		
		//������־(Utf8����)
		virtual Bool  FmtWarn(const Char* pKey,const Char* pFmt, ...);
		
		//������־(Utf8����)
		virtual Bool  FmtError(const Char* pKey,const Char* pFmt, ...);

	public:
		//��־���ͽӿ�(Utf8����)
		virtual Bool  SendLog(UInt8 iType, const Char* pKey, const Char* pMsg);

	protected:
		//�̰߳�ȫ������
		HawkMutex*	m_pLock;
		//��־�����׽���
		HawkSocket	m_sSocket;
		//��־��¼Զ�̵�ַ
		SocketAddr	m_sAddr;
		//��־����ID��ʶ
		UInt16		m_iLogId;
		//�Ƿ񱾵ش�ӡ
		Bool		m_bConsole;
		//�Դ�ӡ��ʾ�߳�ID
		Bool		m_bShowThread;
	};
}
#endif

#ifndef HAWK_SESSIONMANAGER_H
#define HAWK_SESSIONMANAGER_H

#include "HawkSession.h"
#include "HawkManagerBase.h"
#include "HawkLoggerManager.h"
#include "HawkEventThread.h"
#include "HawkNetworkProxy.h"

namespace Hawk
{
	//////////////////////////////////////////////////////////////////////////
	//����Ự����
	//////////////////////////////////////////////////////////////////////////
	class UTIL_API HawkSessionManager : public HawkManagerBase
	{
	protected:
		//����
		HawkSessionManager();

		//����
		virtual ~HawkSessionManager();

		//��������
		HAKW_SINGLETON_DECL(SessionManager);

	public:
		//��ʼ���Ự������(iThreadΪ�����߳���Ŀ,�����̱߳�Ȼ����)
		virtual Bool   Init(Int32 iThread = 0);	

		//���лỰ������
		virtual Bool   Run();

		//ֹͣ�Ự������
		virtual Bool   Stop();

	public:
		//�����Ựͳһ�ӿ�, SessionType: HawkSession::SERVER | HawkSession::CLIENT | HawkSession::PEER & SocketType:  HawkSocket::TCP | HawkSocket::UDP
		virtual Bool   StartSession(const AString& sAddr, UInt8 iSessionType, UInt8 iSocketType, HawkSessionProxy* pProxy = 0);		

		//����ԭʼ����
		virtual Bool   SendRawData(SID iSid, void* pData, Int32 iSize, const SocketAddr* pAddr = 0);

		//��ָ���ỰID����Э��
		virtual Bool   SendProtocol(SID iSid, HawkProtocol* pProto, const SocketAddr* pAddr = 0);

		//֪ͨ�Ự, SIDΪ0��ʾ֪ͨ���лỰ
		virtual Bool   NotifySession(SID iSid, UInt32 iMsg, UInt32 iArgs);

		//�رջỰ(�ⲿ�رջỰ��ͳһ�ӿ�)
		virtual Bool   CloseSession(SID iSid);

	public:
		//��ȡ�߳���
		virtual Int32  GetThreadNum() const;

		//��ȡ�߳�ID
		virtual Int32  GetThreadId(Int32 iIdx) const;

		//���ûỰ�����С
		virtual void   SetBufSize(Int32 iBufSize);

		//��ȡ�Ự�����С
		virtual Int32  GetBufSize() const;	


		//����ȫ�����лỰ�¼��ص�, ��Ҫ���Ự�����رպͻỰЭ��
		virtual Bool   SetNetworkProxy(HawkNetworkProxy* pProxy);

		//���ûỰ�ص�ģ��
		//����ض��Ự�¼�ϸ�ڻص�����
		virtual Bool   SetSessionProxyTmpl(HawkSessionProxy* pProxy);

		//���ݻỰ�ص�ģ�崴���ص�����
		virtual Bool   AllocSessionProxy(HawkSessionProxy*& pProxy);

		//�Ự�Ƿ�������־
		virtual void   EnableSessionLog(Bool bEnable = true);

		//��¼�Ự��־
		virtual Bool   FmtSessionLog(SID iSid, const Char* pFmt, ...);			

	public:
		//Ͷ�ݻỰ�����¼�
		virtual Bool   DispatchSessionStart(SID iSid, UInt8 iSessionType, UInt8 iSocketType, const SocketAddr& sAddr);

		//Ͷ�ݻỰЭ���¼�(����true��ʾЭ��Ͷ�ݸ�Ӧ��,��Ӧ���ͷ�,�����ڲ��ͷ�)
		virtual Bool   DispatchSessionProtocol(SID iSid, HawkProtocol* pProto, const SocketAddr* pAddr = 0);

		//Ͷ�ݻỰ�ر��¼�
		virtual Bool   DispatchSessionClose(SID iSid);		

	public:
		//����֪ͨ����(ͳһ�ӿڷ���Notice�Ļ����Ż�)
		virtual Bool   AllocNotice(HawkEventNotice*& pNotice);

		//�ͷ�֪ͨ����(ͳһ�ӿڷ���Notice�Ļ����Ż�)
		virtual Bool   FreeNotice(HawkEventNotice* pNotice);

		//�ַ��¼��Ự
		virtual Bool   DispatchNotice(SID iSid, HawkEventNotice* pNotice);

		//������ڴ�
		HawkOctets*	   AllocOctets(Int32 iSize);

		//�ͷſ�
		virtual Bool   FreeOctets(HawkOctets* pOctets);

	protected:
		//�¼��ص�
		HawkNetworkProxy*	m_pNetworkProxy;
		//�Ự�ص�����ģ��
		HawkSessionProxy*	m_pSessionProxy;
		//�¼��߳�
		HawkEventThread**	m_ppThread;
		//�߳���
		Int32				m_iThread;		
		//�߳�����
		volatile UInt32		m_iTurnIdx;		
		//�ỰBuffer��С
		Int32				m_iBufSize;		
		//�Ự��־
		Bool				m_bSessionLog;
	};

	#define P_SessionManager  HawkSessionManager::GetInstance()
}
#endif

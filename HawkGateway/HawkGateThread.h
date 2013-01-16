#ifndef HAWK_GATETHREAD_H
#define HAWK_GATETHREAD_H

#include "HawkGateDef.h"

namespace Hawk
{
	/************************************************************************/
	/* ���ط����߳�                                                         */
	/************************************************************************/
	class HawkGateway;
	class GATE_API HawkGateThread : public HawkRefCounter
	{
	public:
		//����
		HawkGateThread(HawkGateway* pGateway = 0);

		//����
		virtual ~HawkGateThread();

		friend class HawkGateway;

	public:
		//�Ự��Ϣ����
		struct Session
		{
			//�ỰID
			SID				Sid;
			//�׽���
			SOCKET			Socket;
			//��ַ��Ϣ
			SocketAddr*		Addr;
			//�Ự���¼�
			void*			Event;
			//�����̶߳���
			HawkGateThread* GThread;
			//���뻺����
			OctetsStream*	IBuffer;
			//���ܶ���
			HawkSecurity*	ISecurity;
			//���ܶ���
			HawkSecurity*	OSecurity;
		};

		//�Ự�б���
		typedef map<SID, Session*> SessionMap;

	public:
		//��ʼ��
		virtual Bool  Init(UInt32 iBaseId);

		//����
		virtual Bool  Start();			

		//�ر�
		virtual Bool  Close();

		//��ȡ�߳�ID
		virtual Int32 GetThreadId() const;

	protected:
		//����Ự
		Session*	  AllocSession(SOCKET hSocket, const SocketAddr& sAddr);

		//����Ự
		virtual Bool  FreeSession(Session* pSession);		

		//�ͷŻỰ�б�
		virtual Bool  FreeSessionMap();

		//���ɻỰID
		virtual SID   GenSessionId();

		//�������ӻỰ
		virtual Bool  StartSession(SOCKET hSocket, const SocketAddr& sAddr);

		//�رջỰ
		virtual Bool  CloseSession(SID iSid);

		//��������������Ϣ
		virtual Bool  RecvGateMsg(SID& iSid, OctetsStream* pOctets);

		//�����ط���������Ϣ
		virtual Bool  SendGateMsg(SID iSid, void* pData, Size_t iSize);		

		//�Ự����Э��
		virtual Bool  SendProtocol(Session* pSession, Protocol* pProto);

	public:
		//�߳�ִ�к���, �̺߳�������
		virtual Bool  OnThreadLoop();

		//�Ự�¼�
		virtual Bool  OnSessionEvent(UInt32 iEvent, Session* pSession);

	protected:
		//��Ϣ�����¼�
		virtual Bool  OnGatewayEvent();

		//����״̬
		virtual Bool  OnThreadIdle();

		//�Ự���ݿɶ��ص�
		virtual Bool  OnSessionRead(Session* pSession);
		
		//�Ự���ݿ�д�ص�
		virtual Bool  OnSessionWrite(Session* pSession);

		//�Ự��������ص�
		virtual Bool  OnSessionError(Session* pSession);

	protected:
		//�Ự���뻺�����ɽ����ж�
		virtual Bool  OnSessionDecode(Session* pSession, OctetsStream* pBuffer);

		//�Ự����������ɼ����ж�
		virtual Bool  OnSessionEncode(Session* pSession, OctetsStream* pBuffer);

	protected:
		//�¼������߳�
		HawkThread*		m_pThread;
		//�¼���������
		void*			m_pBase;
		//��Ϣ����
		HawkZmq*		m_pZmq;
		//��Ϣ����Buffer
		OctetsStream*	m_pOctets;
		//�����ỰID
		UInt32			m_iBaseSid;
		//��ǰ�Ự����
		UInt32			m_iCurSid;
		//��ǰ�Ự�б�
		SessionMap		m_mSession;
		//��������Ӧ��
		HawkGateway*	m_pGateway;
		//����״̬
		Bool			m_bIdle;
		//������״̬
		volatile Bool   m_bRunning;
	};
}
#endif

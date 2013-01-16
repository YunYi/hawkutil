#ifndef HAWK_GATEWAY_H
#define HAWK_GATEWAY_H

#include "HawkGateDef.h"
#include "HawkGateThread.h"
#include "HawkProfiler.h"

namespace Hawk
{
	/************************************************************************/
	/* ���ط�����������װ                                                   */
	/************************************************************************/
	class PRO_API HawkGateway : public HawkRefCounter
	{
	public:
		//����
		HawkGateway();

		//����
		virtual ~HawkGateway();

		//�����б�
		struct Service 
		{
			UInt32  SvrId;
			SOCKET  SvrFd;

			Service(UInt32 iSvrId = 0, SOCKET iSvrFd = INVALID_SOCKET) : SvrId(iSvrId), SvrFd(iSvrFd) {}
		};
		typedef vector<Service> ServiceVec;

	public:
		//��ʼ�����ط���
		virtual Bool    Init(const AString& sFrontend, const AString& sBackend, Int32 iThread = 4, Bool bEchoMode = false);

		//��������
		virtual Bool    Run();

		//�رշ���(֪ͨ����,������)
		virtual Bool    Stop();		

	public:
		//��ȡ�߳���
		virtual Int32   GetThreadNum() const;

		//��ȡ�߳�ID
		virtual Int32   GetThreadId(Int32 iIdx) const;

		//���ûỰ�����С
		virtual void    SetBufSize(Int32 iBufSize);

		//��ȡ�Ự�����С
		virtual Int32   GetBufSize() const;	

		//��������������
		virtual void    SetConnLimit(Int32 iLimit);

		//��ȡ����������
		virtual Int32   GetConnLimit() const;

		//ע��������
		virtual Bool    RegConnection(Bool bConn);

		//���ûỰ��ʱʱ��
		virtual void    SetSessionTimeout(Int32 iTimeout);

		//��ȡ�Ự��ʱʱ��
		virtual Int32   GetSessionTimeout() const;

		//�������ܼ�����
		virtual Bool    TurnOnProfiler(const AString& sAddr);
	
	public:
		//���������߳�
		virtual Bool    CreateGateThread(HawkGateThread*& pThread);

		//�����Ự�������
		virtual Bool    CreateISecurity(HawkSecurity*& pSecurity);

		//�����Ự�������
		virtual Bool    CreateOSecurity(HawkSecurity*& pSecurity);

		//��ȡ���ܼ�����
		HawkProfiler*   GetProfiler();

		//��ȡ�߳�ZMQ�ķ����ַ
		virtual AString GetThreadZmqAddr() const;

		//��ȡ��˷������ZMQ��ַ
		virtual AString GetServerZmqAddr() const;		

	protected:
		//���������̸߳�ʽ������
		virtual Bool    RecvThreadMsg(UInt32& iThread, SID& iSid, OctetsStream* pOctets);		

		//�������̷߳���֪ͨ��Ϣ
		virtual Bool    SendThreadMsg(UInt32 iThread, SID iSid, void* pData, Size_t iSize);

		//���պ�˷����ʽ������
		virtual Bool    RecvServerMsg(UInt32& iSvrId, SID& iSid, OctetsStream* pOctets);

		//���˷�����֪ͨ��Ϣ
		virtual Bool    SendServerMsg(UInt32 iSvrId, SID iSid, void* pData, Size_t iSize);

	protected:
		//����������
		virtual Bool    OnSessionAccept();

		//ThreadZmq�ɶ��¼�
		virtual Bool    OnGateThreadEvent();

		//ServerZmq�ɶ��¼�
		virtual Bool    OnGateServerEvent();

		//MonitorZmq�ɶ��¼�
		virtual Bool    OnGateMonitorEvent();

		//���ؿ���״̬����
		virtual Bool    OnGateIdleEvent();

		//�رշ���(�����ȴ�����)
		virtual Bool    OnGatewayClose();

		//��˷������ӵ�����
		virtual Bool    OnServerConnected(SOCKET hSocket);

		//��˷�������ضϿ�
		virtual Bool    OnServerDisConnect(SOCKET hSocket);

	protected:		
		//�¼��߳�
		HawkGateThread** m_ppThread;
		//�����߳���
		Int32			 m_iThread;
		//�����߳�ID
		UInt32			 m_iTurnIdx;
		//�ỰBuffer��С
		Int32			 m_iBufSize;
		//�Ự��ʱʱ��
		Int32			 m_iTimeout;
		//����������
		Int32			 m_iConnLimit;
		//��ǰ������
		UInt32			 m_iConnCount;
		//��Ϣ����Buffer
		OctetsStream*	 m_pOctets;
		//�������׽���
		HawkSocket		 m_sSocket;
		//���صķ���ID
		ServiceVec		 m_vSvrvice;
		//��ǰ�ķ���ID
		UInt32			 m_iCurSvrId;
		//�������߳�ͨ�ŵ�ZMQ
		HawkZmq*		 m_pThreadZmq;
		//�ͺ�˷���ͨ�ŵ�ZMQ
		HawkZmq*		 m_pServerZmq;
		//��˷���ZMQ������
		HawkZmq*		 m_pMonitorZmq;
		//���ܼ�����
		HawkProfiler*	 m_pProfiler;
		//Echoģʽ
		Bool			 m_bEchoMode;
		//ѭ������״̬
		Bool			 m_bIdle;
		//����״̬
		volatile Bool    m_bRunning;		
	};
}
#endif

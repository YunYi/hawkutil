#ifndef HAWK_EVENTTHREAD_H
#define HAWK_EVENTTHREAD_H

#include "HawkMutex.h"
#include "HawkThread.h"
#include "HawkSession.h"
#include "HawkSocketPair.h"
#include "HawkEventNotice.h"

namespace Hawk
{
	/************************************************************************/
	/* �����¼����̴߳���,��Ҫ�����¼�������                                */
	/************************************************************************/
	class UTIL_API HawkEventThread : public HawkRefCounter
	{
	public:
		//����
		HawkEventThread();

		//����
		virtual ~HawkEventThread();

	public:
		//�Ự�ṹ
		struct ESession
		{
			//Ͷ�ݵ��¼�
			UInt32			 Flag;
			//�Ự���¼�
			void*			 Event;
			//�Ự
			HawkSession*	 Session;
			//�����¼��߳�
			HawkEventThread* Owner;
		};

		//���¼�����
		typedef list<HawkEventNotice*>	NoticeList;
		//�Ự�б�
		typedef map<SID, ESession*>		SessionMap;
		//�Ự�����
		typedef list<ESession*>			SessionCache;
		
	public:
		//��ʼ��
		virtual Bool  Init(UInt32 iBaseId);

		//����
		virtual Bool  Run();			

		//�ر�
		virtual Bool  Close();

	public:
		//���ỰID����
		virtual Bool  CheckSidIn(SID iSid) const;

		//֪ͨ�¼�
		virtual Bool  NotifyNotice(HawkEventNotice* pNotice);

		//��ȡ�߳�ID
		virtual Int32 GetThreadId() const;

	protected:
		//ȡ��֪ͨ
		virtual Bool  PopNotice(HawkEventNotice*& pNotice);

		//����֪ͨ
		virtual Bool  ProcessNotice(HawkEventNotice* pNotice);

		//����Ự
		virtual Bool  AllocSession(ESession*& pSession);

		//����Ự
		virtual Bool  CacheSession(ESession* pSession);		

		//�ͷŻỰ�б�
		virtual Bool  FreeSessionMap();

		//�ͷŻỰ����
		virtual Bool  FreeSessionCache();

		//�ͷ�֪ͨ����
		virtual Bool  FreeNoticeList();

		//���ɻỰID
		virtual SID   GenSessionId();

	public:
		//�߳�ִ�к���, �̺߳�������
		virtual Bool  OnThreadLoop();

		//�ܵ��¼�����, �̺߳�������
		virtual Bool  OnPipeEvent();

		//�Ự�¼�����, �̺߳�������
		virtual Bool  OnSessionEvent(UInt32 iEvent, ESession* pSession);

	protected:
		//�����������Ự, iSocketType: TCP | UDP
		virtual Bool  StartServer(const SocketAddr& sAddr, UInt8 iSocketType, HawkSessionProxy* pProxy = 0);

		//�����ͻ��˻Ự, iSocketType: TCP | UDP
		virtual Bool  StartClient(const SocketAddr& sAddr, UInt8 iSocketType, HawkSessionProxy* pProxy = 0);

		//�������ӻỰ
		virtual Bool  StartPeer(SOCKET hSocket, const SocketAddr& sAddr);

		//д�Ự����
		virtual Bool  SendRawData(ESession* pSession, HawkOctets* pData, const SocketAddr* pAddr = 0);

		//�����Ự�¼�
		virtual Bool  LaunchEvent(SID iSid, ESession* pSession);

		//���»Ự�¼�
		virtual Bool  UpdateEvent(ESession* pSession, UInt32 iEvent);

	protected:
		//�Ự����
		virtual Bool  OnSessionStart(SID iSid, ESession* pSession);

		//�Ự�ر�
		virtual Bool  OnSessionClose(SID iSid);

	protected:
		//�¼������߳�
		HawkThread*		m_pThread;
		//�ܵ�
		SocketPair		m_sPipe;
		//�ܵ��¼�
		void*			m_pEvent;
		//�¼���������
		void*			m_pEventBase;		
		//�ỰID����
		SID*			m_vSidInfo;
		//����ID
		UInt32			m_iBaseSid;
		//�Ự����
		UInt32			m_iCurSid;
		//��ǰ�Ự�б�
		SessionMap		m_mSession;
		//�Ự����
		SessionCache	m_vSessionCache;
		//��֪ͨ����
		NoticeList		m_vNotice;
		//֪ͨ������
		HawkSpinLock*	m_pNoticeLock;
		//֪ͨ����
		HawkMutex*		m_pNoticeMutex;
	};
}
#endif

#ifndef HAWK_SESSION_H
#define HAWK_SESSION_H

#include "HawkSecurity.h"
#include "HawkSessionProxy.h"

namespace Hawk
{
	/************************************************************************/
	/* ����Ự�����װ                                                     */
	/************************************************************************/
	class UTIL_API HawkSession : public HawkRefCounter
	{
	public:
		//����
		HawkSession();
		
		//����
		virtual ~HawkSession();

	public:
		//�Ự��Ϣͳ��(���̰߳�ȫͳ��)
		struct Collect
		{
			UInt64  ConnCount;
			UInt64  RecvTimes;
			UInt64  RecvProtos;
			UInt64  RecvBytes;
			UInt64  SendTimes;
			UInt64  SendProtos;
			UInt64  SendBytes;

			void Reset()
			{
				memset(this, 0, sizeof(*this));
			}

			void OnAccept()
			{
				ConnCount ++;
			}

			void OnRecv(Size_t iSize)
			{
				RecvTimes ++;
				RecvBytes += iSize;
			}

			void OnSend(Size_t iSize)
			{
				SendTimes ++;
				SendBytes += iSize;
			}

			void OnRecvProto()
			{
				RecvProtos ++;
			}

			void OnSendProto()
			{
				SendProtos ++;
			}
		};

	public:
		//�Ự״̬
		enum
		{
			STATE_CLOSED = 0,
			STATE_INITIALIZED,
			STATE_ACTIVATED,
			STATE_CLOSING,
		};

		//�Ự����
		enum
		{
			TYPE_SERVER = 1,
			TYPE_CLIENT,
			TYPE_PEER,
		};

	public:	
		//UDP���͵���Ϣ������
		struct UdpPacket
		{
			OctetsStream Data;
			SocketAddr   Addr;
		};

		//UDP������
		typedef list<UdpPacket> UdpPacketVec;

		//�¼�����������ʹ��Session�Ļص�
		friend class HawkEventThread;

	public:
		//��ʼ��, SessionType: (SERVER | CLIENT | PEER)
		virtual Bool	Init(const HawkSocket& sSocket, const SocketAddr& sAddr, UInt8 iSessionType, HawkSessionProxy* pProxy = 0);

		//�رջỰ
		virtual Bool	Close();

		//�Ƿ���Ч
		virtual Bool    IsValid() const;	

		//���ûỰID
		virtual void	SetSid(SID iSid);

		//�ỰID
		virtual SID		GetSid() const;	

		//����״̬
		virtual void	SetState(UInt32 iState);

		//��ȡ״̬
		virtual UInt32  GetState() const;			

		//�Ự�Ƿ�Ϊ����״̬
		virtual Bool	IsActive() const;

		//�Ự�Ƿ�Ϊ���ر�״̬
		virtual Bool    IsClosing() const;

		//�Ự�Ƿ��ѹر�
		virtual Bool	IsClosed() const;

	public:
		//���ûỰ�ص�
		virtual void	SetSessionProxy(HawkSessionProxy* pProxy);

		//���ö���������
		virtual void	SetISecurity(HawkSecurity* pSecurity);

		//����д���������
		virtual void	SetOSecurity(HawkSecurity* pSecurity);

		//��ȡ�Ự����
		virtual UInt8	GetSessionType() const;

		//��ȡ�׽�������
		virtual UInt8	GetSocketType() const;

		//��ȡͳ����Ϣ
		const Collect&	GetCollect() const;

		//��ȡ�Ự����������
		HawkSecurity*	GetISecurity();

		//��ȡ�Ựд���������
		HawkSecurity*	GetOSecurity();	

		//��ȡ����Buffer
		OctetsStream*	GetInputBuf();

		//��ȡ���Buffer
		OctetsStream*	GetOutputBuf();

		//��ȡ�ỰSocket
		HawkSocket*		GetSocket();

		//��ȡ�Ự��ַ
		SocketAddr		GetAddress();

		//��ȡ���
		SOCKET			GetHandle();

	public:
		//����ԭʼ����
		virtual Bool	SendRawData(void* pData, Int32 iSize, const SocketAddr* pAddr = 0);

		//����Э��
		virtual Bool	SendProtocol(HawkProtocol* pProtocol, const SocketAddr* pAddr = 0);

	protected:
		//���ݿɶ�
		virtual Bool    OnSessionRead();

		//���ݿ�д
		virtual Bool    OnSessionWrite();

		//�����ӿ���
		virtual Bool    OnSessionAccept(HawkSocket& sSocket, SocketAddr& sAddr);

		//�Ự֪ͨ
		virtual Bool    OnSessionNotify(UInt32 iMsg, UInt32 iArgs);

		//����Э����зַ�
		virtual Bool    OnDecodeProtocol(UInt32& iCount);

		//�Ƿ�ȴ����
		virtual Bool    IsWaitOutput() const;

	protected:
		//�ỰID
		SID					m_iSid;
		//״̬
		UInt32				m_iState;		
		//�Ự�׽���
		HawkSocket			m_sSocket;
		//�Ự�ص�
		HawkSessionProxy*	m_pProxy;
		//��ַ
		SocketAddr			m_sAddress;	
		//���뻺����
		OctetsStream*		m_pInputBuf;
		//���������
		OctetsStream*		m_pOutputBuf;
		//��ʱ������
		OctetsStream*		m_pCommonBuf;
		//���ܶ���
		HawkSecurity*		m_pISecurity;
		//���ܶ���
		HawkSecurity*		m_pOSecurity;		
		//ͳ����Ϣ
		Collect				m_sCollect;
		//UDP���ͻ�����
		UdpPacketVec		m_vUdpPacket;
		//UDP���͵�ַ
		SocketAddr			m_sUdpTarget;	
		//����(SERVER | CLIENT | PEER)
		UInt8				m_iSessionType;
	};
}
#endif

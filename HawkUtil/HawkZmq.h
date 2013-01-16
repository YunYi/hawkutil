#ifndef HAWK_ZMP_H
#define HAWK_ZMP_H

#include "HawkObjBase.h"
#include "HawkProtocolManager.h"

namespace Hawk
{
	/************************************************************************/
	/* 0MQ�ֲ�ʽ��Ϣ���в�����װ                                            */
	/************************************************************************/
	class UTIL_API HawkZmq : public HawkRefCounter
	{
	protected:
		//����
		HawkZmq();

		//����
		virtual ~HawkZmq();

		//��Աʲô����manager��Init����
		friend class HawkZmqManager;

	public:
		//ZMQͨѶ����
		enum
		{
			HZMQ_PAIR	= 0,
			HZMQ_PUB	= 1,
			HZMQ_SUB	= 2,
			HZMQ_REQ	= 3,
			HZMQ_REP	= 4,
			HZMQ_DEALER	= 5,
			HZMQ_ROUTER	= 6,
			HZMQ_PULL	= 7,
			HZMQ_PUSH	= 8,
			HZMQ_XPUB	= 9,
			HZMQ_XSUB	= 10,
		};

		//Send��Recv��Option����
		enum
		{
			HZMQ_DONTWAIT = 1,
			HZMQ_SNDMORE  = 2,
		};

		//Message��Option����
		enum
		{
			HZMQ_MSGMORE = 1,
		};

	public:
		//Svrģʽ�󶨵�ַ
		virtual Bool   Bind(const AString& sAddr);

		//Cltģʽ����
		virtual Bool   Connect(const AString& sAddr);

		//��������(������Ϣ����)
		virtual Bool   Send(void* pBuf, Size_t iSize, Int32 iFlag = 0);

		//��������(������Ϣ����)
		virtual Bool   Recv(void* pBuf, Size_t& iSize, Int32 iFlag = 0);	

		//��������(����Э�鷢��)
		virtual Bool   SendProtocol(HawkProtocol* pProto, Int32 iFlag = 0);

		//��������(����Э�����)
		virtual Bool   RecvProtocol(HawkProtocol*& pProto, Int32 iFlag = 0);

		//�¼����
		virtual UInt32 PollEvent(UInt32 iEvents = HEVENT_READ, Int32 iTimeout = -1);	

		//�����¼�������
		virtual Bool   StartMonitor(const AString& sAddr, UInt32 iEvents);

		//�ر��¼�������
		virtual Bool   StopMonitor();

	public:
		//��ȡ���
		virtual void*  GetHandle();

		//��ȡ����
		virtual Int32  GetType() const;

		//������
		virtual Int32  GetErrCode() const;

		//�и�������Ҫ��ȡ
		virtual Bool   IsWaitRecv() const;		

		//����IDENTITY����
		virtual Bool   SetIdentity(const void* pOptVal, Int32 iSize);	

		//����ZMQ����
		virtual Bool   SetOption(Int32 iOption, const void* pOptVal, Size_t iSize);

		//��ȡZMQ����
		virtual Bool   GetOption(Int32 iOption, void* pOptVal, Size_t& iSize);

	protected:
		//��ʼ��ZMQ���
		virtual Bool   Init(Int32 iType, void* pHandle);	

		//��������Ϣ
		virtual Bool   FillErr();

		//�رն���
		virtual Bool   Close();

	protected:
		void*			m_pHandle;
		Int32			m_iType;			
		Int32			m_iErrCode;
		AString			m_sErrMsg;
		Int32			m_iRecvMore;
		OctetsStream*	m_pRecvBuf;
	};
}
#endif

#ifndef HAWK_EVENTNOTICE_H
#define HAWK_EVENTNOTICE_H

#include "HawkOctets.h"

namespace Hawk
{
	/************************************************************************/
	/* ʱ��֪ͨ����                                                         */
	/************************************************************************/
	class UTIL_API HawkEventNotice : public HawkRefCounter
	{
	public:
		//����
		HawkEventNotice();

		//����
		virtual ~HawkEventNotice();

		//��ֵ����
		HawkEventNotice& operator = (const HawkEventNotice& sNotice);

		//�¼��߳�֪ͨ���
		enum
		{
			NOTIFY_SESSION	= 0x01,
			NOTIFY_PEER		= 0x02,
			NOTIFY_MSG		= 0x03,
			NOTIFY_WRITE	= 0x04,
			NOTIFY_CLOSE	= 0x05,
			NOTIFY_EXIT		= 0x06,
		};

	public:
		//֪ͨ���
		UInt8  NoticeType;

		//֪ͨ����
		union
		{
			//�����Ự
			struct
			{
				//�Ự��ַ
				Char	    Address[IPV_LENGTH];
				//��ַ����
				UInt8	    AddrLen;
				//�Ự����, SERVER | CLIENT
				UInt8	    SessionType;
				//�׽�������, TCP | UDP
				UInt8	    SocketType;
				//�Ự�ص�
				void*	    SessionProxy;
			}eSession;

			//�½�����
			struct
			{
				//���Ӷ��׽���
				SOCKET	    Handle;
				//�Ự��ַ
				Char	    Address[IPV_LENGTH];
				//��ַ����
				UInt8	    AddrLen;
			}ePeer;

			//��Ϣ֪ͨ
			struct
			{
				SID			Sid;
				UInt32		Msg;
				UInt32		Args;
			}eMsg;

			//��������
			struct
			{
				//�ỰID
				SID			Sid;
				//��������Buffer
				HawkOctets* Octets;
				//�Ự��ַ
				Char	    Address[IPV_LENGTH];
				//��ַ����
				UInt8	    AddrLen;
			}eWrite;

			//�رջỰ
			struct
			{
				SID			Sid;
			}eClose;
		}NoticeParam;

	public:
		//��������
		virtual void  Clear();
	};
}
#endif

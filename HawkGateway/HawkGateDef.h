#ifndef HAWK_GATEDEF_H
#define HAWK_GATEDEF_H

#include "HawkUtil.h"

namespace Hawk
{
	//�ڲ�֪ͨ����
	struct GateNotify
	{
		//֪ͨ���ͱ��
		enum
		{
			NOTIFY_UNKNOWN = 0,
			NOTIFY_SESSION_CONNECT,		//�½��Ự����
			NOTIFY_SESSION_DISCONN,		//�Ự���ӶϿ�
			NOTIFY_SESSION_CLOSE,		//�����رջỰ			
			NOTIFY_SERVICE_EXIT,		//�˳����ط���
			NOTIFY_SERVICE_ATTACH,		//�������
			NOTIFY_SERVICE_DETACH,		//����ж��
		};

		//Ĭ�Ϲ���
		GateNotify(UInt8 iType = NOTIFY_UNKNOWN) : Type(iType) {};

		//֪ͨ����
		UInt8  Type;

		//֪ͨ����
		union
		{
			//����������
			struct
			{
				//���Ӷ��׽���
				SOCKET	Handle;
				//�Ự��ַ
				Char	Address[IPV_LENGTH];
				//��ַ����
				UInt8	AddrLen;
			}eConnect;				

			//���ӻỰ�Ͽ�(����)
			struct
			{
				//�ỰID
				SID		Sid;
				//�Ự��ַ
				Char	Address[IPV_LENGTH];
				//��ַ����
				UInt8	AddrLen;
			}eDisConn;

			//�ر����ӻỰ(����)
			struct
			{
				//�ỰID
				SID		Sid;
			}eClose;

			//�������
			struct 
			{
				UInt32  SvrId;
			}eAttach;
		};
	};
}
#endif

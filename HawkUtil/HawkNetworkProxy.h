#ifndef HAWK_NETWORKPROXY_H
#define HAWK_NETWORKPROXY_H

#include "HawkSocket.h"
#include "HawkProtocol.h"

namespace Hawk
{
	/************************************************************************/
	/* ����Ự�ص�����                                                     */
	/* ȫ�ֻỰ�¼��ص�: (�Ự���� | �Ự�ر� | �ỰЭ��)					*/
	/* ��ע: �ӿ��ڲ����ûỰ���Լ��Ự����,����Ϣ�Ļ�����Ӧ��Ͷ�ݻỰ�¼�	*/
	/************************************************************************/
	class UTIL_API HawkNetworkProxy : public HawkRefCounter
	{
	public:
		//�Ự�����¼��ص�
		//SessionType: HawkSession::SERVER | HawkSession::CLIENT | HawkSession::PEER
		//SocketType:  HawkSocket::TCP | HawkSocket::UDP
		virtual Bool  OnSessionStart(SID iSid, UInt8 iSessionType, UInt8 iSocketType, const SocketAddr& sAddr);

		//�Ự�ر��¼��ص�
		virtual Bool  OnSessionClose(SID iSid);

		//�ỰЭ���¼��ص�,����true����ʾӦ�ýӹ�Э�鲢��Ӧ���ͷ�,����ϵͳ�����ͷ�
		virtual Bool  OnSessionProtocol(SID iSid, Protocol* pProto, const SocketAddr* pAddr = 0);
	};
}
#endif

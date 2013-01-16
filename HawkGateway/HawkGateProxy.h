#ifndef HAWK_GATEPROXY_H
#define HAWK_GATEPROXY_H

#include "HawkGateDef.h"

namespace Hawk
{
	/************************************************************************/
	/* ������ƥ������Ӷ˷�װ                                               */
	/************************************************************************/
	class GATE_API HawkGateProxy : public HawkRefCounter
	{
	public:
		//����
		HawkGateProxy();

		//����
		virtual ~HawkGateProxy();

	public:
		//��ʼ��
		virtual Bool  Init(const AString& sAddr, UInt32 iSvrId);

		//����Э��
		virtual Bool  RecvProtocol(SID& iSid, HawkProtocol*& pProto, Int32 iTimeout = -1);

		//����Э��
		virtual Bool  SendProtocol(SID iSid, HawkProtocol* pProto);

		//�رջỰ
		virtual Bool  CloseSession(SID iSid);

	protected:
		//����֪ͨ
		virtual Bool  SendNotify(const GateNotify& sNotify, SID iSid = 0);

	protected:
		//��Ϣͨ��
		HawkZmq*		m_pProxyZmq;
		//ͨ�û�����
		OctetsStream*	m_pOctets;
	};
}
#endif

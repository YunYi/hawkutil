#ifndef HAWK_GATEPROXY_H
#define HAWK_GATEPROXY_H

#include "HawkGateDef.h"

namespace Hawk
{
	/************************************************************************/
	/* 和网关匹配的连接端封装                                               */
	/************************************************************************/
	class GATE_API HawkGateProxy : public HawkRefCounter
	{
	public:
		//构造
		HawkGateProxy();

		//析构
		virtual ~HawkGateProxy();

	public:
		//初始化
		virtual Bool  Init(const AString& sAddr, UInt32 iSvrId);

		//接收协议
		virtual Bool  RecvProtocol(SID& iSid, HawkProtocol*& pProto, Int32 iTimeout = -1);

		//发送协议
		virtual Bool  SendProtocol(SID iSid, HawkProtocol* pProto);

		//关闭会话
		virtual Bool  CloseSession(SID iSid);

	protected:
		//发送通知
		virtual Bool  SendNotify(const GateNotify& sNotify, SID iSid = 0);

	protected:
		//消息通道
		HawkZmq*		m_pProxyZmq;
		//通用缓冲区
		OctetsStream*	m_pOctets;
	};
}
#endif

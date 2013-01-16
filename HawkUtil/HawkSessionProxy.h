#ifndef HAWK_SESSIONPROXY_H
#define HAWK_SESSIONPROXY_H

#include "HawkSocket.h"
#include "HawkProtocol.h"

namespace Hawk
{
	//会话事件回调对象
	class UTIL_API HawkSessionProxy : public HawkRefCounter
	{
	public:
		//构造
		HawkSessionProxy();

		//析构
		virtual ~HawkSessionProxy();

	public:
		//初始化回调代理
		virtual Bool Init(SID iSid);

		//克隆回调代理(继承的必须实现此方法)
		virtual HawkSessionProxy* Clone() const;

	public:
		//服务端接收Socket连接(返回True即允许连接，否则直接关闭)
		virtual Bool OnAccept(const HawkSocket& sSocket, const SocketAddr& sAddr);

		//会话开启
		virtual Bool OnStart();		

		//数据可读, 返回false即不进行读操作
		virtual Bool OnRead();				

		//协议解析, 返回false即不进行协议解析操作
		virtual Bool OnDecode(OctetsStream* pOS);

		//会话协议
		virtual Bool OnProtocol(HawkProtocol* pProto, const SocketAddr* pAddr = 0);

		//数据可写, 返回false即不进行写操作
		virtual Bool OnWrite();	

		//消息通知
		virtual Bool OnNotify(UInt32 iMsg, UInt32 iArgs);

		//会话关闭
		virtual Bool OnClose();

	public:
		//读取重定向完成通知(返回
		virtual Bool OnRdRedirect(Size_t iSize);

		//写出重定向完成通知
		virtual Bool OnWrRedirect(Size_t iSize);

		//获取读取重定向Octets(返回true表示有重定向,反之亦然)
		virtual Bool GetRdRedirectOS(OctetsStream*& pOS, Size_t& iSize);

		//获取写出重定向Octets(返回true表示有重定向,反之亦然)
		virtual Bool GetWrRedirectOS(OctetsStream*& pOS, Size_t& iSize);

	protected:
		//会话ID
		SID  m_iSid;
	};
}
#endif

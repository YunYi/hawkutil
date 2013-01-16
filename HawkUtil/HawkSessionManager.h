#ifndef HAWK_SESSIONMANAGER_H
#define HAWK_SESSIONMANAGER_H

#include "HawkSession.h"
#include "HawkManagerBase.h"
#include "HawkLoggerManager.h"
#include "HawkEventThread.h"
#include "HawkNetworkProxy.h"

namespace Hawk
{
	//////////////////////////////////////////////////////////////////////////
	//网络会话管理
	//////////////////////////////////////////////////////////////////////////
	class UTIL_API HawkSessionManager : public HawkManagerBase
	{
	protected:
		//构造
		HawkSessionManager();

		//析构
		virtual ~HawkSessionManager();

		//单例申明
		HAKW_SINGLETON_DECL(SessionManager);

	public:
		//初始化会话管理器(iThread为连接线程数目,监听线程必然存在)
		virtual Bool   Init(Int32 iThread = 0);	

		//运行会话管理器
		virtual Bool   Run();

		//停止会话管理器
		virtual Bool   Stop();

	public:
		//开启会话统一接口, SessionType: HawkSession::SERVER | HawkSession::CLIENT | HawkSession::PEER & SocketType:  HawkSocket::TCP | HawkSocket::UDP
		virtual Bool   StartSession(const AString& sAddr, UInt8 iSessionType, UInt8 iSocketType, HawkSessionProxy* pProxy = 0);		

		//发送原始数据
		virtual Bool   SendRawData(SID iSid, void* pData, Int32 iSize, const SocketAddr* pAddr = 0);

		//向指定会话ID发送协议
		virtual Bool   SendProtocol(SID iSid, HawkProtocol* pProto, const SocketAddr* pAddr = 0);

		//通知会话, SID为0表示通知所有会话
		virtual Bool   NotifySession(SID iSid, UInt32 iMsg, UInt32 iArgs);

		//关闭会话(外部关闭会话的统一接口)
		virtual Bool   CloseSession(SID iSid);

	public:
		//获取线程数
		virtual Int32  GetThreadNum() const;

		//获取线程ID
		virtual Int32  GetThreadId(Int32 iIdx) const;

		//设置会话缓冲大小
		virtual void   SetBufSize(Int32 iBufSize);

		//获取会话缓冲大小
		virtual Int32  GetBufSize() const;	


		//设置全局所有会话事件回调, 主要监测会话开启关闭和会话协议
		virtual Bool   SetNetworkProxy(HawkNetworkProxy* pProxy);

		//设置会话回调模板
		//针对特定会话事件细节回调处理
		virtual Bool   SetSessionProxyTmpl(HawkSessionProxy* pProxy);

		//依据会话回调模板创建回调对象
		virtual Bool   AllocSessionProxy(HawkSessionProxy*& pProxy);

		//会话是否允许日志
		virtual void   EnableSessionLog(Bool bEnable = true);

		//记录会话日志
		virtual Bool   FmtSessionLog(SID iSid, const Char* pFmt, ...);			

	public:
		//投递会话开启事件
		virtual Bool   DispatchSessionStart(SID iSid, UInt8 iSessionType, UInt8 iSocketType, const SocketAddr& sAddr);

		//投递会话协议事件(返回true表示协议投递给应用,由应用释放,否则内部释放)
		virtual Bool   DispatchSessionProtocol(SID iSid, HawkProtocol* pProto, const SocketAddr* pAddr = 0);

		//投递会话关闭事件
		virtual Bool   DispatchSessionClose(SID iSid);		

	public:
		//创建通知对象(统一接口方便Notice的缓存优化)
		virtual Bool   AllocNotice(HawkEventNotice*& pNotice);

		//释放通知对象(统一接口方便Notice的缓存优化)
		virtual Bool   FreeNotice(HawkEventNotice* pNotice);

		//分发事件会话
		virtual Bool   DispatchNotice(SID iSid, HawkEventNotice* pNotice);

		//分配块内存
		HawkOctets*	   AllocOctets(Int32 iSize);

		//释放块
		virtual Bool   FreeOctets(HawkOctets* pOctets);

	protected:
		//事件回调
		HawkNetworkProxy*	m_pNetworkProxy;
		//会话回调代理模板
		HawkSessionProxy*	m_pSessionProxy;
		//事件线程
		HawkEventThread**	m_ppThread;
		//线程数
		Int32				m_iThread;		
		//线程索引
		volatile UInt32		m_iTurnIdx;		
		//会话Buffer大小
		Int32				m_iBufSize;		
		//会话日志
		Bool				m_bSessionLog;
	};

	#define P_SessionManager  HawkSessionManager::GetInstance()
}
#endif

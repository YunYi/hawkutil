#ifndef HAWK_SYSPROTOCOL_H
#define HAWK_SYSPROTOCOL_H

#include "HawkProtocol.h"

namespace Hawk
{
	/************************************************************************/
	/* �����ڲ�Э�鶨��                                                      */
	/************************************************************************/
	namespace SysProtocol
	{
		//Э��ID����
		enum
		{
			//����֪ͨ
			SYS_HEART_BEAT		= 1,
			//�����Ͽ�
			SYS_HEART_BREAK		= 2,
			//�ͻ��˻Ự�Ͽ�
			SYS_SESSION_BREAK	= 3,
			//����������رջỰ
			SYS_CLOSE_SESSION	= 4,
			//��¼��־
			SYS_LOG_MSG			= 5,
			//ϵͳ���ܲ�ѯ
			SYS_PROF_REQ		= 6,
			//ϵͳ������Ϣ
			SYS_PROF_INFO		= 7,
			//�ܾ�����
			SYS_REFUSE_CONN		= 8,
			//Ping������
			SYS_CLT_PING		= 9,
			//��������ӦPing
			SYS_SVR_PONG		= 10,

			//ϵͳ�����Э��ID
			SYS_MAX_PROTO		= 100,
		};
		
		//����ID����
		enum
		{
			//�ܾ�����
			ERR_REFUSE_CONN = 1,
			//�޿��÷���
			ERR_SERVICE_INVALID,
		};

		//////////////////////////////////////////////////////////////////////////
		//ϵͳ�ڲ�ʹ��Э�鶨��
		//////////////////////////////////////////////////////////////////////////

		class Sys_HeartBeat : public HawkProtocol
		{
		public:
			Sys_HeartBeat(UInt32 iTime = 0) : HawkProtocol(SYS_HEART_BEAT)
			{
				m_iTime = iTime;
			};

			virtual HawkProtocol* Clone() const
			{
				return new Sys_HeartBeat;
			};

			virtual HawkOctetsStream& Marshal(HawkOctetsStream& rhsOS)
			{
				rhsOS << m_iTime;
				return rhsOS;
			};

			virtual HawkOctetsStream& Unmarshal(HawkOctetsStream& rhsOS)
			{
				rhsOS >> m_iTime;
				return rhsOS;
			};

		public:
			UInt32 m_iTime;
		};
		
		class Sys_HeartBreak : public HawkProtocol
		{
		public:
			Sys_HeartBreak(UInt32 iTime = 0) : HawkProtocol(SYS_HEART_BREAK)
			{
				m_iTime = iTime;
			};

			virtual HawkProtocol* Clone() const
			{
				return new Sys_HeartBreak;
			};

			virtual HawkOctetsStream& Marshal(HawkOctetsStream& rhsOS)
			{
				rhsOS << m_iTime;
				return rhsOS;
			};

			virtual HawkOctetsStream& Unmarshal(HawkOctetsStream& rhsOS)
			{
				rhsOS >> m_iTime;
				return rhsOS;
			};

		public:
			UInt32 m_iTime;
		};
		
		class Sys_SessionBreak : public HawkProtocol
		{
		public:
			Sys_SessionBreak(UInt32 iSid = 0) : HawkProtocol(SYS_SESSION_BREAK)
			{
				m_iSid = iSid;
			};

			virtual HawkProtocol* Clone() const
			{
				return new Sys_SessionBreak;
			};

			virtual HawkOctetsStream& Marshal(HawkOctetsStream& rhsOS)
			{
				rhsOS << m_iSid;
				return rhsOS;
			};

			virtual HawkOctetsStream& Unmarshal(HawkOctetsStream& rhsOS)
			{
				rhsOS >> m_iSid;
				return rhsOS;
			};

		public:
			UInt32 m_iSid;
		};
		
		class Sys_CloseSession : public HawkProtocol
		{
		public:
			Sys_CloseSession(UInt32 iSid = 0) : HawkProtocol(SYS_CLOSE_SESSION)
			{
				m_iSid = iSid;
			};

			virtual HawkProtocol* Clone() const
			{
				return new Sys_CloseSession;
			};

			virtual HawkOctetsStream& Marshal(HawkOctetsStream& rhsOS)
			{
				rhsOS << m_iSid;
				return rhsOS;
			};

			virtual HawkOctetsStream& Unmarshal(HawkOctetsStream& rhsOS)
			{
				rhsOS >> m_iSid;
				return rhsOS;
			};

		public:
			UInt32 m_iSid;
		};
		
		class Sys_LogMsg : public HawkProtocol
		{
		public:
			Sys_LogMsg(Int32 iLogId = 0, Int32 iType = 0, const UString& sKey = UString(), const UString& sMsg = UString()) : HawkProtocol(SYS_LOG_MSG)
			{
				m_iLogId = iLogId;
				m_iType	 = iType;
				m_sKey   = sKey;
				m_sMsg	 = sMsg;
			};

			virtual HawkProtocol* Clone() const
			{
				return new Sys_LogMsg;
			};

			virtual HawkOctetsStream& Marshal(HawkOctetsStream& rhsOS)
			{
				rhsOS << m_iLogId << m_iType << m_sKey << m_sMsg;
				return rhsOS;
			};

			virtual HawkOctetsStream& Unmarshal(HawkOctetsStream& rhsOS)
			{
				rhsOS >> m_iLogId >> m_iType >> m_sKey >> m_sMsg;
				return rhsOS;
			};

		public:
			Int32	m_iLogId;
			Int32	m_iType;
			UString m_sKey;
			UString m_sMsg;
		};

		class Sys_ProfReq : public HawkProtocol
		{
		public:
			Sys_ProfReq() : HawkProtocol(SYS_PROF_REQ)
			{
			};

			virtual HawkProtocol* Clone() const
			{
				return new Sys_ProfReq;
			};

			virtual HawkOctetsStream& Marshal(HawkOctetsStream& rhsOS)
			{
				return rhsOS;
			};

			virtual HawkOctetsStream& Unmarshal(HawkOctetsStream& rhsOS)
			{
				return rhsOS;
			};
		};

		class Sys_ProfInfo : public HawkProtocol
		{
		public:
			Sys_ProfInfo(UInt32 iTimeStamp = 0, UInt32 iCpuCount = 0, UInt64 iTotalMem = 0, UInt32 iCpuUsage = 0, UInt32 iMemUsage = 0, UInt32 iConnect = 0, UInt64 iRecvProto = 0, UInt64 iRecvSize = 0, UInt64 iSendProto = 0, UInt64 iSendSize = 0) : HawkProtocol(SYS_PROF_INFO)
			{
				m_iTimeStamp = iTimeStamp;
				m_iCpuCount  = iCpuCount;
				m_iTotalMem	 = iTotalMem;
				m_iCpuUsage	 = iCpuUsage;
				m_iMemUsage	 = iMemUsage;
				m_iConnect	 = iConnect;
				m_iRecvProto = iRecvProto;
				m_iRecvSize	 = iRecvSize;
				m_iSendProto = iSendProto;
				m_iSendSize	 = iSendSize;
			};

			virtual HawkProtocol* Clone() const
			{
				return new Sys_ProfInfo;
			};

			virtual HawkOctetsStream& Marshal(HawkOctetsStream& rhsOS)
			{
				rhsOS << m_iTimeStamp << m_iCpuCount << m_iTotalMem << m_iCpuUsage << m_iMemUsage << m_iConnect << m_iRecvProto << m_iRecvSize << m_iSendProto << m_iSendSize;
				return rhsOS;
			};

			virtual HawkOctetsStream& Unmarshal(HawkOctetsStream& rhsOS)
			{
				rhsOS >> m_iTimeStamp >> m_iCpuCount >> m_iTotalMem >> m_iCpuUsage >> m_iMemUsage >> m_iConnect >> m_iRecvProto >> m_iRecvSize >> m_iSendProto >> m_iSendSize;
				return rhsOS;
			};

		public:
			UInt32  m_iTimeStamp;
			UInt32	m_iCpuCount;
			UInt64  m_iTotalMem;
			UInt32	m_iCpuUsage;
			UInt64	m_iMemUsage;
			UInt32	m_iConnect;
			UInt64	m_iRecvProto;
			UInt64	m_iRecvSize;
			UInt64	m_iSendProto;
			UInt64	m_iSendSize;
		};

		class Sys_RefuseConn : public HawkProtocol
		{
		public:
			Sys_RefuseConn(Int32 iErrCode = 0) : HawkProtocol(SYS_REFUSE_CONN)
			{
				m_iErrCode = iErrCode;
			};

			virtual HawkProtocol* Clone() const
			{
				return new Sys_RefuseConn;
			};

			virtual HawkOctetsStream& Marshal(HawkOctetsStream& rhsOS)
			{
				return rhsOS << m_iErrCode;
			};

			virtual HawkOctetsStream& Unmarshal(HawkOctetsStream& rhsOS)
			{
				return rhsOS >> m_iErrCode;
			};

		public:
			Int32  m_iErrCode;
		};

		class Sys_CltPing : public HawkProtocol
		{
		public:
			Sys_CltPing() : HawkProtocol(SYS_CLT_PING)
			{
			};

			virtual HawkProtocol* Clone() const
			{
				return new Sys_CltPing;
			};

			virtual HawkOctetsStream& Marshal(HawkOctetsStream& rhsOS)
			{
				return rhsOS;
			};

			virtual HawkOctetsStream& Unmarshal(HawkOctetsStream& rhsOS)
			{
				return rhsOS;
			};
		};

		class Sys_SvrPong : public HawkProtocol
		{
		public:
			Sys_SvrPong(UInt32 iTime = 0) : HawkProtocol(SYS_SVR_PONG)
			{
				m_iTime = iTime;
			};

			virtual HawkProtocol* Clone() const
			{
				return new Sys_SvrPong;
			};

			virtual HawkOctetsStream& Marshal(HawkOctetsStream& rhsOS)
			{
				return rhsOS << m_iTime;
			};

			virtual HawkOctetsStream& Unmarshal(HawkOctetsStream& rhsOS)
			{
				return rhsOS >> m_iTime;
			};

		public:
			UInt32  m_iTime;
		};
	};	
}
#endif

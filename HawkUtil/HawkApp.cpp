#include "HawkApp.h"
#include "HawkRand.h"
#include "HawkScope.h"
#include "HawkException.h"
#include "HawkLoggerManager.h"

namespace Hawk
{
	HawkApp* g_App = 0;

	class ProtoTask : public HawkTask
	{
	public:
		ProtoTask(const XID& sXid = XID(0,0), SID iSid = 0, Protocol* pProto = 0) : m_sXid(sXid), m_iSid(0), m_pProto(pProto)
		{
		};

		virtual ~ProtoTask()
		{
			P_ProtocolManager->ReleaseProto(m_pProto);
			m_pProto = 0;
		}

	public:
		virtual PVoid  Run(void* pArgs = 0)
		{
			if(m_pProto && g_App)
				g_App->DispatchProto(m_sXid, m_iSid, m_pProto);

			return 0;
		}

	protected:
		XID	      m_sXid;
		SID		  m_iSid;
		Protocol* m_pProto;
	};

	class MsgTask : public HawkTask
	{
	public:
		MsgTask(const XID& sXid = XID(0,0), HawkMsg* pMsg = 0) : m_sXid(sXid), m_pMsg(pMsg)
		{
		};

		virtual ~MsgTask()
		{
			P_MsgManager->FreeMsg(m_pMsg);
			m_pMsg = 0;
		}

	public:
		virtual PVoid  Run(void* pArgs = 0)
		{
			if(m_pMsg && g_App)
				g_App->DispatchMsg(m_sXid, m_pMsg);

			return 0;
		}

	protected:
		XID	      m_sXid;
		HawkMsg*  m_pMsg;
	};

	//////////////////////////////////////////////////////////////////////////

	HawkApp::SafeObj::SafeObj(XID sXid) : ObjMan::SafeObj(g_App->GetObjMan(sXid.Type), sXid)
	{
	}

	HawkApp::SafeObj::~SafeObj()
	{
	}

	HawkApp::HawkApp()
	{
		HawkAssert(g_App == 0);		
		m_pThreadPool = 0;
		m_bRunning	  = false;		
		m_pSidXidLock = new HawkSpinLock;
		g_App         = this;
	}

	HawkApp::~HawkApp()
	{
		Stop();		

		if (m_pThreadPool)
			m_pThreadPool->Close();

		HAWK_RELEASE(m_pThreadPool);
		HAWK_RELEASE(m_pSidXidLock);
		g_App = 0;

		ObjManMap::iterator it = m_mObjMan.begin();
		for (;it!=m_mObjMan.end();it++)
		{
			ObjMan* pMan = it->second;
			HAWK_RELEASE(pMan);
		}
		m_mObjMan.clear();
		m_mSidXid.clear();
	}

	HawkApp::ObjMan* HawkApp::CreateObjMan(UInt32 iType)
	{
		ObjMan* pObjMan = GetObjMan(iType);
		if (!pObjMan)
		{
			pObjMan = new ObjMan;
			m_mObjMan[iType] = pObjMan;
		}
		return pObjMan;
	}

	HawkApp::ObjMan* HawkApp::GetObjMan(UInt32 iType)
	{
		ObjManMap::const_iterator it = m_mObjMan.find(iType);
		if (it != m_mObjMan.end())
		{
			return (ObjMan*)it->second;
		}
		return 0;
	}

	HawkAppObj* HawkApp::CreateObj(const XID& sXid, SID iSid)
	{
		if(!sXid.IsValid())
			return 0;		

		ObjMan* pObjMan = GetObjMan(sXid.Type);
		HawkAssert(pObjMan);

		HawkAppObj* pObj = g_App->CreateObj(sXid);
		if (pObj)
		{
			if(pObjMan->AllocObject(sXid,pObj))	
			{
				if(iSid) 
					BindSidXid(iSid,sXid);	

				return pObj;
			}

			HAWK_RELEASE(pObj);
			return 0;
		}

		HawkAssert(false && "Create Obj Null.");
		return 0;
	}

	Bool  HawkApp::DeleteObj(const XID& sXid)
	{
		if (sXid.IsValid())
		{
			ObjMan* pObjMan = GetObjMan(sXid.Type);
			HawkAssert(pObjMan);

			return pObjMan->FreeObject(sXid);
		}
		return true;
	}

	XID HawkApp::GetXidBySid(SID iSid)
	{
		HawkAutoSpinLock(lock, m_pSidXidLock);
		SidXidMap::iterator it = m_mSidXid.find(iSid);
		if (it != m_mSidXid.end())
			return it->second;
		
		return XID(0,0);
	}

	void HawkApp::UnbindSidXid(SID iSid)
	{
		if(iSid) 
		{
			HawkAutoSpinLock(lock, m_pSidXidLock);
			SidXidMap::iterator it = m_mSidXid.find(iSid);
			if (it != m_mSidXid.end())
				m_mSidXid.erase(it);
		}
	}

	void HawkApp::BindSidXid(SID iSid,const XID& sXid)
	{
		if (iSid && sXid.IsValid())
		{
			HawkAutoSpinLock(lock, m_pSidXidLock);
			m_mSidXid[iSid] = sXid;
		}
	}

	Bool HawkApp::Init(Int32 iThread)
	{
		if (!m_pThreadPool)
		{
			m_pThreadPool = new HawkThreadPool;
			m_pThreadPool->InitPool(iThread, false);
			return true;
		}
		
		return false;
	}

	Bool HawkApp::Run()
	{
		if (!m_bRunning)
		{
			m_bRunning = true;

			if(!m_pThreadPool->Start())
				return false;		

			for (Int32 i=0;i<m_pThreadPool->GetThreadNum();i++)
			{
				HawkFmtPrint("AppThread: %d", m_pThreadPool->GetThreadId(i));
			}			

			return true;
		}
		return false;
	}

	Bool HawkApp::Stop()
	{
		if (m_bRunning)
		{
			m_bRunning = false;

			if (m_pThreadPool)
				m_pThreadPool->Close();
		}
		return true;
	}

	Bool HawkApp::IsRunning() const
	{
		return m_bRunning;
	}

	Int32 HawkApp::GetThreadNum() const
	{
		if (m_pThreadPool)
			return m_pThreadPool->GetThreadNum();
		
		return 0;
	}

	Bool HawkApp::SendProtocol(SID iSid, Protocol* pProto)
	{
		return true;
	}

	Bool HawkApp::PostProtocol(SID iSid, Protocol* pProto)
	{
		if (m_bRunning && iSid && pProto)
		{
			XID sXid     = GetXidBySid(iSid);
			Int32 iCount = GetThreadNum();
			HawkAssert(iCount > 0);
			Int32 iIdx   = sXid.Id % iCount;

			ProtoTask* pTask = new ProtoTask(sXid, iSid, pProto);
			HawkScope::ObjPtr scope(pTask);
			return PostAppTask(pTask, iIdx);
		}

		P_ProtocolManager->ReleaseProto(pProto);
		return true;
	}

	Bool HawkApp::PostMsg(HawkMsg* pMsg)
	{
		if (m_bRunning && pMsg)
		{
			Int32 iCount = GetThreadNum();
			HawkAssert(iCount > 0);
			Int32 iIdx = pMsg->Target.Id % iCount;

			MsgTask* pTask = new MsgTask(pMsg->Target, pMsg);
			HawkScope::ObjPtr scope(pTask);
			return PostAppTask(pTask, iIdx);
		}

		P_MsgManager->FreeMsg(pMsg);
		return false;
	}

	Bool HawkApp::PostMsg(const XID& sXid, HawkMsg* pMsg)
	{
		HawkMsg* pRealMsg = (HawkMsg*)pMsg;
		if (pRealMsg && sXid.IsValid())
		{
			pRealMsg->Target = sXid;
			return PostMsg(pRealMsg);
		}
		
		P_MsgManager->FreeMsg(pRealMsg);
		return false;
	}

	Bool HawkApp::PostMsg(const XIDVector& vXID, HawkMsg* pMsg)
	{
		if (pMsg && vXID.size())
		{
			for(Size_t i=0;i<vXID.size();i++)
			{
				HawkMsg* pRealMsg = P_MsgManager->GetMsg(pMsg->Msg);
				if (pRealMsg)
				{
					pRealMsg->Target = vXID[i];
					pRealMsg->Source = pMsg->Source;
					pRealMsg->CopyParams(pMsg->Params);
					PostMsg(pRealMsg);
				}
			}
		}

		P_MsgManager->FreeMsg(pMsg);
		return true;
	}

	Bool HawkApp::PostAppTask(HawkTask* pTask, Int32 iThreadIdx)
	{
		if(m_bRunning && m_pThreadPool)
		{
			if(iThreadIdx < 0)
			{
				A_Exception(m_pThreadPool->GetThreadNum() > 0);
				iThreadIdx = HawkRand::RandInt(0, m_pThreadPool->GetThreadNum()-1);
			}

			return m_pThreadPool->AddTask(pTask, iThreadIdx);
		}
		return false;
	}

	Bool HawkApp::DispatchProto(const XID& sXid, SID iSid, Protocol* pProto)
	{
		if(sXid.IsValid() && pProto)
		{
			SafeObj obj(sXid);
			if (obj.IsObjValid())
				return obj->OnProtocol(iSid, pProto);		
		}	
		return false;
	}

	Bool HawkApp::DispatchMsg(const XID& sXid, HawkMsg* pMsg)
	{
		if(sXid.IsValid() && pMsg && pMsg->Msg > 0 && pMsg->Target.IsValid())
		{
			SafeObj obj(pMsg->Target);
			if (obj.IsObjValid())
				return obj->OnMessage(*pMsg);		
		}
		return false;
	}
}

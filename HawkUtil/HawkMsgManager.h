#ifndef HAWK_MSGMANAGER_H
#define HAWK_MSGMANAGER_H

#include "HawkMsg.h"
#include "HawkManagerBase.h"

namespace Hawk
{
	/************************************************************************/
	/* ��Ϣ������,�ڴ������ͷ���Ϣ֮ǰ,����ע�����ҵ���Ϣ����               */
	/************************************************************************/
	class HawkMsgManager : public HawkManagerBase
	{
	protected:
		//����
		HawkMsgManager();

		//����
		virtual ~HawkMsgManager();

		//��������������
		HAKW_SINGLETON_DECL(MsgManager);

		//ע���б�
		typedef map<Int32, Int32> MsgRegMap;

	public:
		//ע����Ϣ����
		virtual Bool	 Register(Int32 iMsg);	

		//��ȡ��Ϣ
		virtual HawkMsg* GetMsg(Int32 iMsg);

		//�ͷ���Ϣ
		virtual Bool     FreeMsg(HawkMsg* pMsg);

		//��ȡע��Э��ID�б�
		virtual UInt32	 GetRegMsgIds(vector<Int32>& vMsgIds);

	protected:		
		//ע�������б�
		MsgRegMap m_mReg;
	};

	#define P_MsgManager  HawkMsgManager::GetInstance()
}
#endif

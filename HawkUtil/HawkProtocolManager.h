#ifndef HAWK_PROTOCOLMANAGER_H
#define HAWK_PROTOCOLMANAGER_H

#include "HawkManagerBase.h"
#include "HawkProtocol.h"
#include "HawkSysProtocol.h"

namespace Hawk
{
	/************************************************************************/
	/* Э���������װ                                                       */
	/************************************************************************/
	class UTIL_API HawkProtocolManager : public HawkManagerBase
	{
	protected:
		//����
		HawkProtocolManager();

		//����
		virtual ~HawkProtocolManager();

		//��������������
		HAKW_SINGLETON_DECL(ProtocolManager);

	public:
		//Э��ע���
		typedef map<ProtoType, Protocol*>  ProtocolMap;

		//�Զ��ͷŷ�װ
		class UTIL_API Scope : public HawkRefCounter
		{
		public:
			Scope(HawkProtocol* pProto = 0);

			virtual ~Scope();

		protected:
			HawkProtocol*  m_pProto;
		};

	public:
		//����Э�����
		virtual HawkProtocol*  CreateProtocol(ProtoType iType);

		//����Э������
		virtual HawkProtocol*  Decode(HawkOctetsStream& rhsOS);

		//ע��Э������
		virtual Bool		   Register(ProtoType iType, HawkProtocol* pProto);		

		//�ж�Э���Ƿ�ע��
		virtual Bool		   CheckProtocolLegal(ProtoType iType);

		//�ͷ�Э�����
		virtual Bool		   ReleaseProto(HawkProtocol*& pProto);

		//���������ļ�����Э�����
		virtual Bool		   GenProtocols(const AString& sCfgFile, const AString& sHawkHead = "HawkUtil.h");

		//ֱ�ӽ���Э������(��������DecodeStream����)
		virtual Bool		   IsAutoDecode() const;

		//�����Զ��������
		virtual void		   SetAutoDecode(Bool bAuto = true);

	public:
		//�Ƿ��пɶ�Э���ͷ
		virtual Bool		   CheckDecodeProtocol(const HawkOctetsStream& xOS, UInt32* pBodySize = 0);

		//��ȡЭ��ͷ�ֽڴ�С
		virtual UInt32		   GetProtoHeaderSize() const;

		//дЭ���ͷ
		virtual Bool		   ReadProtocolHeader(HawkOctetsStream& xOS, ProtoType& iType, ProtoSize& iSize, ProtoCrc& iCrc);

		//��Э���ͷ
		virtual Bool		   WriteProtocolHeader(HawkOctetsStream& xOS,ProtoType iType, ProtoSize iSize, ProtoCrc iCrc);

		//��ȡע��Э��ID�б�
		virtual UInt32		   GetRegProtoIds(vector<ProtoType>& vProtoIds);

		//ע��ϵͳ�ڲ�Э��
		virtual Bool		   RegSysProtocol();

	protected:		
		//ע���Э���
		ProtocolMap		 m_mRegister;
		//�ͷ��Զ�Э�����
		Bool			 m_bAutoDecode;
		//Э���ͷ�ֽڴ�С
		UInt32			 m_iProtoSize;
	};

	#define P_ProtocolManager  HawkProtocolManager::GetInstance()

	//����ϵͳ����֧��Э���
	#define REGISTER_PROTO(protocol_class)\
	{\
		protocol_class* _ptr_ = new protocol_class;\
		P_ProtocolManager->Register(_ptr_->GetType(),_ptr_);\
		HAWK_RELEASE(_ptr_);\
	}
}
#endif

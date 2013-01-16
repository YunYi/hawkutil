#ifndef HAWK_SESSIONPROXY_H
#define HAWK_SESSIONPROXY_H

#include "HawkSocket.h"
#include "HawkProtocol.h"

namespace Hawk
{
	//�Ự�¼��ص�����
	class UTIL_API HawkSessionProxy : public HawkRefCounter
	{
	public:
		//����
		HawkSessionProxy();

		//����
		virtual ~HawkSessionProxy();

	public:
		//��ʼ���ص�����
		virtual Bool Init(SID iSid);

		//��¡�ص�����(�̳еı���ʵ�ִ˷���)
		virtual HawkSessionProxy* Clone() const;

	public:
		//����˽���Socket����(����True���������ӣ�����ֱ�ӹر�)
		virtual Bool OnAccept(const HawkSocket& sSocket, const SocketAddr& sAddr);

		//�Ự����
		virtual Bool OnStart();		

		//���ݿɶ�, ����false�������ж�����
		virtual Bool OnRead();				

		//Э�����, ����false��������Э���������
		virtual Bool OnDecode(OctetsStream* pOS);

		//�ỰЭ��
		virtual Bool OnProtocol(HawkProtocol* pProto, const SocketAddr* pAddr = 0);

		//���ݿ�д, ����false��������д����
		virtual Bool OnWrite();	

		//��Ϣ֪ͨ
		virtual Bool OnNotify(UInt32 iMsg, UInt32 iArgs);

		//�Ự�ر�
		virtual Bool OnClose();

	public:
		//��ȡ�ض������֪ͨ(����
		virtual Bool OnRdRedirect(Size_t iSize);

		//д���ض������֪ͨ
		virtual Bool OnWrRedirect(Size_t iSize);

		//��ȡ��ȡ�ض���Octets(����true��ʾ���ض���,��֮��Ȼ)
		virtual Bool GetRdRedirectOS(OctetsStream*& pOS, Size_t& iSize);

		//��ȡд���ض���Octets(����true��ʾ���ض���,��֮��Ȼ)
		virtual Bool GetWrRedirectOS(OctetsStream*& pOS, Size_t& iSize);

	protected:
		//�ỰID
		SID  m_iSid;
	};
}
#endif

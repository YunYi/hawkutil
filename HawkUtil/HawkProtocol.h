#ifndef HAWK_PROTOCOL_H
#define HAWK_PROTOCOL_H

#include "HawkMarshalData.h"
#include "HawkSocket.h"

namespace Hawk
{
	/************************************************************************/
	/* Э���ʽ���������װ                                                 */
	/************************************************************************/
	class UTIL_API HawkProtocol : public HawkMarshal
	{
	public:
		//Э�鹹��
		HawkProtocol(ProtoType iType = 0);

		//Э������
		virtual ~HawkProtocol();		

	public:
		//��¡Э��(ʵ��Э�����ʵ��)
		virtual HawkProtocol* Clone() const = 0;

	public:		
		//Э����: Type + Size + Crc + Data
		virtual Bool  Encode(HawkOctetsStream& rhsOS);

		//Э����
		virtual Bool  Decode(HawkOctetsStream& rhsOS);		

		//����Э��
		virtual Bool  Send(SID iSid, const SocketAddr* pAddr = 0);

		//��ȡ�Խ�Buffer
		virtual Bool  GetDecodeOS(OctetsStream*& pOS);
		
	public:
		//��ȡЭ������
		ProtoType	  GetType() const;

		//��ȡ�ֽ���
		ProtoSize     GetSize() const;

		//��ȡЭ������У��CRC
		ProtoCrc	  GetCrc() const;		

	protected:
		//�Խ���,Ĭ������»��Խ���
		virtual Bool  DecodeSelf();

	protected:
		//����
		ProtoType	 m_iType;
		//�ֽ���
		ProtoSize	 m_iSize;
		//У����
		ProtoCrc	 m_iCrc;
		//���ݴ洢��
		OctetsStream m_sDecode;
	};

	//Э�����ͼ�㶨��
	typedef HawkProtocol Protocol;
}
#endif

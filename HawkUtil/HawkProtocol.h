#ifndef HAWK_PROTOCOL_H
#define HAWK_PROTOCOL_H

#include "HawkMarshalData.h"
#include "HawkSocket.h"

namespace Hawk
{
	/************************************************************************/
	/* 协议格式定义操作封装                                                 */
	/************************************************************************/
	class UTIL_API HawkProtocol : public HawkMarshal
	{
	public:
		//协议构造
		HawkProtocol(ProtoType iType = 0);

		//协议析构
		virtual ~HawkProtocol();		

	public:
		//克隆协议(实际协议必须实现)
		virtual HawkProtocol* Clone() const = 0;

	public:		
		//协议打包: Type + Size + Crc + Data
		virtual Bool  Encode(HawkOctetsStream& rhsOS);

		//协议解包
		virtual Bool  Decode(HawkOctetsStream& rhsOS);		

		//发送协议
		virtual Bool  Send(SID iSid, const SocketAddr* pAddr = 0);

		//获取自解Buffer
		virtual Bool  GetDecodeOS(OctetsStream*& pOS);
		
	public:
		//获取协议类型
		ProtoType	  GetType() const;

		//获取字节数
		ProtoSize     GetSize() const;

		//获取协议数据校验CRC
		ProtoCrc	  GetCrc() const;		

	protected:
		//自解析,默认情况下会自解析
		virtual Bool  DecodeSelf();

	protected:
		//类型
		ProtoType	 m_iType;
		//字节数
		ProtoSize	 m_iSize;
		//校验码
		ProtoCrc	 m_iCrc;
		//数据存储器
		OctetsStream m_sDecode;
	};

	//协议类型简便定义
	typedef HawkProtocol Protocol;
}
#endif

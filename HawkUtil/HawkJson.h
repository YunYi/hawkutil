#ifndef HAWK_JSON_H
#define HAWK_JSON_H

#include "HawkDiskFile.h"

namespace Hawk
{
	/************************************************************************/
	/* JSON��ʽ���ݽ���(UTF8����)											*/
	/************************************************************************/
	class UTIL_API HawkJson : public HawkRefCounter
	{
	public:
		//����
		HawkJson();

		//����
		virtual ~HawkJson();

	public:
		//���Ͷ���
		enum
		{
			JSON_NONE = 0,
			JSON_STRING,
			JSON_OBJECT,
			JSON_ARRAY,
		};

		//�������ͺͶ������Ͷ���
		struct  JsonVal;
		typedef vector<JsonVal*>		JsonArr;
		typedef map<AString, JsonVal*>	JsonObj;

		//ֵ
		struct JsonVal
		{
			UInt8	 Type;
			AString	 Str;
			JsonObj  Obj;
			JsonArr	 Arr;
		};

	public:
		//��������
		virtual Bool   Parse(const Char* pData, Int32 iSize);

		//�������
		virtual Bool   Clear();

		//��ȡ������
		const JsonObj* GetRoot() const ;

	protected:
		//�ͷ�JsonVal����
		virtual Bool   FreeJsonVal(JsonVal* pVal);

	protected:
		//������
		JsonObj  m_sRoot;
	};
}
#endif

#ifndef HAWK_JSON_H
#define HAWK_JSON_H

#include "HawkDiskFile.h"

namespace Hawk
{
	/************************************************************************/
	/* JSON格式数据解析(UTF8编码)											*/
	/************************************************************************/
	class UTIL_API HawkJson : public HawkRefCounter
	{
	public:
		//构造
		HawkJson();

		//析构
		virtual ~HawkJson();

	public:
		//类型定义
		enum
		{
			JSON_NONE = 0,
			JSON_STRING,
			JSON_OBJECT,
			JSON_ARRAY,
		};

		//数组类型和对象类型定义
		struct  JsonVal;
		typedef vector<JsonVal*>		JsonArr;
		typedef map<AString, JsonVal*>	JsonObj;

		//值
		struct JsonVal
		{
			UInt8	 Type;
			AString	 Str;
			JsonObj  Obj;
			JsonArr	 Arr;
		};

	public:
		//解析数据
		virtual Bool   Parse(const Char* pData, Int32 iSize);

		//清除数据
		virtual Bool   Clear();

		//获取根对象
		const JsonObj* GetRoot() const ;

	protected:
		//释放JsonVal对象
		virtual Bool   FreeJsonVal(JsonVal* pVal);

	protected:
		//根对象
		JsonObj  m_sRoot;
	};
}
#endif

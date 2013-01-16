#include "HawkJson.h"
#include "HawkStringUtil.h"
#include "json/json.h"

namespace Hawk
{
	//½âÎöJsonÖµ
	Bool  ParseJsonVal(const Json::Value* pJsonVal, HawkJson::JsonVal* pVal)
	{
		if (!pJsonVal || !pVal)
			return false;

		if (pJsonVal->isBool())
		{
			pVal->Type = HawkJson::JSON_STRING;
			pVal->Str  = pJsonVal->asBool()? "true":"false";
		}
		else if (pJsonVal->isInt())
		{
			pVal->Type = HawkJson::JSON_STRING;
			pVal->Str  = HawkStringUtil::IntToString<AString>(pJsonVal->asInt());
		}
		else if (pJsonVal->isUInt())
		{
			pVal->Type = HawkJson::JSON_STRING;
			pVal->Str  = HawkStringUtil::UIntToString<AString>(pJsonVal->asUInt());
		}
		else if (pJsonVal->isDouble())
		{
			pVal->Type = HawkJson::JSON_STRING;
			pVal->Str  = HawkStringUtil::DoubleToString<AString>(pJsonVal->asDouble());
		}
		else if (pJsonVal->isString())
		{
			pVal->Type = HawkJson::JSON_STRING;
			pVal->Str  = pJsonVal->asString();
		}
		else if (pJsonVal->isArray())
		{
			pVal->Type = HawkJson::JSON_ARRAY;
			for (Size_t i=0;i<pJsonVal->size();i++)
			{
				const Json::Value& sJsonVal = pJsonVal->get(i, Json::Value::null);
				if (sJsonVal.isNull())
					return false;

				HawkJson::JsonVal* pSubVal = new HawkJson::JsonVal;
				if (ParseJsonVal(&sJsonVal, pSubVal))
				{
					pVal->Arr.push_back(pSubVal);
				}
				else
				{
					HAWK_DELETE(pSubVal);
				}
			}
		}
		else if (pJsonVal->isObject())
		{
			pVal->Type = HawkJson::JSON_OBJECT;
			for(Json::Value::iterator it = pJsonVal->begin(); it !=  pJsonVal->end(); it++)
			{
				const Json::Value& sJsonKey = it.key();
				if (sJsonKey.isNull())
					return false;

				AString sKey = sJsonKey.asString();
				const Json::Value& sJsonVal = pJsonVal->get(sKey.c_str(),Json::Value::null);
				if (sJsonVal.isNull())
					return false;

				HawkJson::JsonVal* pSubVal = new HawkJson::JsonVal;
				if (ParseJsonVal(&sJsonVal, pSubVal))
				{
					pVal->Obj[sKey] = pSubVal;
				}
				else
				{
					HAWK_DELETE(pSubVal);
					return false;
				}
			}
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////////

	HawkJson::HawkJson()
	{
	}

	HawkJson::~HawkJson()
	{
		Clear();
	}

	Bool HawkJson::Parse(const Char* pData, Int32 iSize)
	{
		Json::Reader jReader;
		Json::Value  sRoot;
		if (jReader.parse(pData, sRoot,false))
		{
			for(Json::Value::iterator it = sRoot.begin(); it != sRoot.end(); it++)
			{
				const Json::Value& sJsonKey = it.key();
				if (sJsonKey.isNull())
					return false;

				AString sKey = sJsonKey.asString();
				const Json::Value& sJsonVal = sRoot[sKey.c_str()];
				if (sJsonVal.isNull())
					return false;

				JsonVal* pVal = new JsonVal;
				if (ParseJsonVal(&sJsonVal, pVal))
				{
					m_sRoot[sKey] = pVal;
				}
				else
				{
					HAWK_DELETE(pVal);
					return false;
				}
			}
			return true;
		}
		return false;
	}

	Bool HawkJson::Clear()
	{
		JsonObj::iterator it = m_sRoot.begin();
		for (;it!=m_sRoot.end();it++)
		{
			FreeJsonVal(it->second);
		}
		m_sRoot.clear();
		return true;
	}

	Bool HawkJson::FreeJsonVal(JsonVal* pVal)
	{
		if (pVal->Type == JSON_OBJECT)
		{
			JsonObj::iterator it = m_sRoot.begin();
			for (;it!=m_sRoot.end();it++)
			{
				FreeJsonVal(it->second);
			}
			pVal->Obj.clear();
		}
		else if (pVal->Type == JSON_ARRAY && pVal->Arr.size())
		{
			for (Size_t i=0;i<pVal->Arr.size();i++)
			{
				FreeJsonVal(pVal->Arr[i]);
			}
			pVal->Arr.clear();
		}		
		HAWK_DELETE(pVal);
		return true;
	}

	const HawkJson::JsonObj* HawkJson::GetRoot() const
	{
		return &m_sRoot;
	}
}

#include "HawkXmlFile.h"
#include "HawkStringUtil.h"
#include "HawkDiskFile.h"
#include "HawkScope.h"
#include "tinyxml.h"

namespace Hawk
{
	//非递归模式解析避免文件节点过大递归导致堆栈溢出
	template <class StringType, class CharType> 
		Bool ParseTemplate(HawkFile::CodeType eCodeType, HawkXmlDocument<StringType>& xDoc, const CharType* pData, Int64 iSize)
	{
		TiXmlDocument xmlDoc;
		TiXmlEncoding iEncoding = TIXML_DEFAULT_ENCODING;
		if (eCodeType != HawkFile::CODE_ASCII)
			iEncoding  = TIXML_ENCODING_UTF8;

		xmlDoc.Parse((const Char*)pData, 0, iEncoding);
		if (xmlDoc.Error() || !xmlDoc.RootElement())
			return false;

		TiXmlNode* pNode = xmlDoc.RootElement();

		HawkXmlElement<StringType>* pXmlParent = 0;
		HawkXmlElement<StringType>* pXmlNode   = xDoc.GetRoot();
		StringType sTmp;

		while (pNode && pXmlNode)
		{
			if(pNode->Type() == TiXmlNode::TINYXML_ELEMENT)
			{
				TiXmlElement *pElemNode = pNode->ToElement();
				sTmp = (CharType*)pElemNode->Value();

				pXmlNode->Tag    = sTmp;
				pXmlNode->Parent = pXmlParent;

				if (pXmlParent)
					pXmlNode->Level = pXmlParent->Level + 1;
				else
					pXmlNode->Level = 0;

				TiXmlAttribute* pAttribute = pElemNode->FirstAttribute();
				while (pAttribute)
				{
					StringType sKey   = (CharType*)pAttribute->Name();
					StringType sValue = (CharType*)pAttribute->Value();
					if (sKey.size())
					{
						pXmlNode->AddAttribute(HawkXmlAttribute<StringType>(sKey,sValue));
					}
					pAttribute = pAttribute->Next();
				}
			}
			else if (pNode->Type() == TiXmlNode::TINYXML_TEXT)
			{
				sTmp = (CharType*)pNode->Value();
				pXmlNode->Value = sTmp;
			}

			TiXmlNode* pTmpNode = pNode;
			pNode = pNode->FirstChild();

			if (pNode)
			{
				if (pNode->Type() == TiXmlNode::TINYXML_ELEMENT || pNode->Type() == TiXmlNode::TINYXML_TEXT)
				{
					HawkXmlElement<StringType>* pNextChild = pXmlNode->AddChildren(StringType());
					pXmlParent = pXmlNode;
					pXmlNode   = pNextChild;
				}
				else
				{
					pXmlParent = pXmlNode;
				}
			}
			else
			{
				pNode = pTmpNode->NextSibling();
				if (pNode)
				{
					if (!pXmlParent)
						break;

					if (pNode->Type() == TiXmlNode::TINYXML_ELEMENT || pNode->Type() == TiXmlNode::TINYXML_TEXT)
					{
						pXmlNode = pXmlParent->AddChildren(StringType());							
					}						
				}
				else
				{
					pNode = pTmpNode->Parent();

					if(!pXmlParent)
						break;

					pXmlParent = pXmlParent->Parent;

					while (pNode)
					{
						pTmpNode = pNode->NextSibling();
						if (pTmpNode)
						{
							pNode = pTmpNode;
							if(!pXmlParent)
								break;

							if (pTmpNode->Type() == TiXmlNode::TINYXML_ELEMENT || pTmpNode->Type() == TiXmlNode::TINYXML_TEXT)
							{
								pXmlNode = pXmlParent->AddChildren(StringType());
							}

							break;
						}
						else
						{
							pNode = pNode->Parent();
							if(!pXmlParent)
								break;

							pXmlParent = pXmlParent->Parent;
						}
					}

					if (!pNode || !pXmlParent)
						break;
				}
			}
		}
		return true;
	}

	//////////////////////////////////////////////////////////////////////////

	HawkXmlFile::HawkXmlFile()
	{
	}

	HawkXmlFile::~HawkXmlFile()
	{
	}

	Bool HawkXmlFile::ParseFile(const AString& sFile, PVoid pDoc)
	{
		HawkDiskFile xFile;
		if (!xFile.Open(sFile))
			return false;

		Int64 iSize = xFile.GetFileSize() + 8;
		Char* pData = (Char*)HawkMalloc((Size_t)iSize);
		HawkScope::MallocPtr scope(pData);
		memset(pData, 0, (Size_t)iSize);
		iSize = xFile.Read(pData, iSize);
		xFile.Close();

		return ParseData(pData, iSize, pDoc);
	}

	Bool HawkXmlFile::ParseData(const void* pData, Int64 iSize, PVoid pDoc)
	{
		if (!pData || iSize <=0)
			return false;

		HawkFile::CodeType eCodeType = HawkFile::GetCodeType(pData);
		A_Exception(eCodeType != HawkFile::CODE_UNICODE && "HawkXmlFile Cannot Parser Unicode.");

		Char* pDataPtr  = (Char*)pData;
		Int64 iDataSize = iSize;

		if (eCodeType == HawkFile::CODE_ASCII)
		{
			AXmlDocument* pXmlDoc = (AXmlDocument*)pDoc;
			return ParseTemplate<AString, Char>(HawkFile::CODE_ASCII, *pXmlDoc, pDataPtr, iDataSize);
		}
		else if (eCodeType == HawkFile::CODE_UTF8)
		{
			pDataPtr  += 3;
			iDataSize -= 3;
			UXmlDocument* pXmlDoc = (UXmlDocument*)pDoc;
			return ParseTemplate<UString, UChar>(HawkFile::CODE_UTF8, *pXmlDoc, (const UChar*)pDataPtr, iDataSize);
		}		
		
		return false;
	}
}

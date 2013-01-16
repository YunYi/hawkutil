#include "HawkProtocolManager.h"
#include "HawkLoggerManager.h"
#include "HawkOSOperator.h"
#include "HawkXmlFile.h"
#include "HawkScope.h"

/*
<!-- ===========================协议格式定义===========================	-->
<!-- //xml special words:(&, &amp;),(<, &lt;),(>, &gt;),(", &quot;)		-->
<!-- Name: 变量名称, Type: 变量类型, Default: 构造默认值				-->
*/

//////////////////////////////////////////////////////////////////////////
//1M
#define MAX_BUFFER		1048576

#define MARSHALDATA_CLASS	"class %s : public MarshalData\r\n\
{\r\n\
public:\r\n\
	%s\r\n\
\r\n\
	%s\r\n\
\r\n\
	%s\r\n\
\r\n\
	%s\r\n\
\r\n\
	%s\r\n\
\r\n\
	%s\r\n%s\
};\r\n\r\n"

#define PROTOCOL_CLASS	"class %s : public HawkProtocol\r\n\
{\r\n\
public:\r\n\
	%s\r\n\
\r\n\
	%s\r\n\
\r\n\
	%s\r\n\
\r\n\
	%s\r\n%s\
};\r\n\r\n"

enum
{
	TYPE_UNKNOWN_CLASS = 0,
	TYPE_MARSHAL_CLASS,
	TYPE_PROTOCOL_CLASS,
};

//////////////////////////////////////////////////////////////////////////

namespace Hawk
{
	Char*   g_Buffer		  = 0;
	AString g_ProtocolIdFile  = "";
	AString g_ProtocolRegFile = "";
	AStringVector	 g_ProtocolVec;
	map<Int32,Int32> g_ProtocolMap;
	map<Int32,Int32> g_MacroMap;

	Bool IsValueType(const AString& sType)
	{
		return sType == "Int8" ||
			sType == "UInt8" ||
			sType == "Int16" ||
			sType == "UInt16" ||
			sType == "Int32" ||
			sType == "UInt32" ||
			sType == "Int64" || 
			sType == "UInt64" ||
			sType == "Float" ||
			sType == "Double" ||
			sType == "Bool" ||
			sType == "Char" ||
			sType == "UChar" ||
			sType == "Utf8" ||
			sType == "WChar";
	};

	Bool IsVectorType(const AString& sType)
	{
		return sType == "Int8" ||
			sType == "UInt8" ||
			sType == "Int16" ||
			sType == "UInt16" ||
			sType == "Int32" ||
			sType == "UInt32" ||
			sType == "Int64" || 
			sType == "UInt64" ||
			sType == "Float" ||
			sType == "Double" ||
			sType == "Bool" ||
			sType == "Char" ||
			sType == "UChar" ||
			sType == "Utf8" ||
			sType == "WChar";
	};

	struct GenVarData
	{
		AString Name;
		AString Type;
		AString Default;

		GenVarData()
		{
			Name = "";
			Type = "";
			Default = "";
		};

		Bool IsValid() const
		{
			return Name.size() && Type.size();
		}

		AString  GetMemberFmt()
		{
			AString sValue = Type;
			sValue += " ";
			sValue += Name;
			sValue += ";";
			return sValue;
		}

		AString GetConstructFmt()
		{
			if (Type.size() == 0)
			{
				if (Name.find("m_i") == 0)
				{
					Type = "Int32";
				}
				else if (Name.find("m_f") == 0)
				{
					Type = "Float";
				}
				else if (Name.find("m_db") == 0)
				{
					Type = "Double";
				}
				else if (Name.find("m_b") == 0)
				{
					Type = "Bool";
				}
				else if (Name.find("m_s") == 0)
				{
					Type = "AString";
				}
			}

			AString sConstructName = AString("_") + Name;
			if(Name.find("m_") != AString::npos)
				sConstructName = Name.substr(Name.find("m_")+2 ,Name.size()-2);

			AString sConstructType = Type;
			if (!IsValueType(Type))
				sConstructType = AString("const ") + Type + AString("&");

			if (Default.size() == 0)
			{
				if (IsValueType(Type))
					Default = "0";
				else
					Default = Type + "()";
			}

			AString sValue = sConstructType;
			sValue += " ";
			sValue += sConstructName;
			if (Default.size())
			{
				sValue += " = ";
				sValue += Default;
			}
			return sValue;
		}

		AString GetInitFmt()
		{
			AString sConstructName = AString("_") + Name;
			if(Name.find("m_") != AString::npos)
				sConstructName = Name.substr(Name.find("m_")+2 ,Name.size()-2);
			
			AString sValue = Name;
			sValue += "(";
			sValue += sConstructName;
			sValue += ")";
			return sValue;
		}

		AString GetCopyFmt()
		{
			AString sValue = Name;
			sValue += "(rhs.";
			sValue += Name;
			sValue += ")";
			return sValue;
		}

		AString GetSetValFunc()
		{
			AString sValue = Name;
			sValue += " = ";
			sValue += "rhs.";
			sValue += Name;
			sValue += ";";
			return sValue;
		}
	};

	struct GenClassData
	{
		Int32   ClassType;
		AString ClassName;
		AString ClassDesc;		
		Int32	Identify;		
		AString	Macro;

		AString ConstructFunc;
		AString CopyFunc;
		AString CloneFunc;
		AString OperatorSelfFunc;
		AString MarshalFunc;
		AString UnmarshalFunc;		
		AString MemberDefine;
		vector<GenVarData> VarData;

		GenClassData()
		{
			ClassType		    = 0;
			ClassName			= "";
			ClassDesc			= "";
			Identify			= 0;
			Macro				= "";
			ConstructFunc		= "";
			CopyFunc			= "";
			CloneFunc			= "";
			OperatorSelfFunc	= "";
			MarshalFunc			= "";
			UnmarshalFunc		= "";
			MemberDefine		= "";
		}

		Bool  BuildConstructFunc()
		{
			ConstructFunc = "";
			ConstructFunc += ClassName;
			ConstructFunc += "(";
			for (Size_t i=0;i<VarData.size();i++)
			{
				GenVarData& xVar = VarData[i];
				if (i != 0)
					ConstructFunc += ", ";
				ConstructFunc += xVar.GetConstructFmt();
			}

			ConstructFunc += ")";
			if (ClassType != TYPE_MARSHAL_CLASS || VarData.size())
				ConstructFunc += " : ";

			if (ClassType == TYPE_PROTOCOL_CLASS)
			{
				ConstructFunc += "HawkProtocol(";
				ConstructFunc += g_ProtocolIdFile;
				ConstructFunc += "::";
				ConstructFunc += Macro;
				ConstructFunc += ")";
				if(VarData.size())
					ConstructFunc += ", ";
			}

			for (Size_t i=0;i<VarData.size();i++)
			{
				GenVarData& xVar = VarData[i];
				if (i != 0)
					ConstructFunc += ", ";
				ConstructFunc += xVar.GetInitFmt();
			}

			ConstructFunc += "\r\n\t{\r\n";
			ConstructFunc += "\t};";
			return true;
		}

		Bool  BuildCopyFunc()
		{
			CopyFunc = "";
			if(ClassType == TYPE_MARSHAL_CLASS)
			{
				CopyFunc += ClassName;
				CopyFunc += "(const ";
				CopyFunc += ClassName;
				CopyFunc += "& rhs)";
				if (VarData.size())
					CopyFunc += " : ";

				for (Size_t i=0;i<VarData.size();i++)
				{
					GenVarData& xVar = VarData[i];
					if (i != 0)
						CopyFunc += ", ";
					CopyFunc += xVar.GetCopyFmt();
				}
				CopyFunc += "\r\n\t{\r\n\t};";
			}			
			return true;
		}

		Bool  BuildCloneFunc()
		{
			AString sFmtValue = "";
			sFmtValue += "virtual ";

			if(ClassType == TYPE_MARSHAL_CLASS)
			{
				sFmtValue += "MarshalData* Clone() const\r\n\t{\r\n\t\treturn new ";
			}
			else if (ClassType == TYPE_PROTOCOL_CLASS)
			{
				sFmtValue += "HawkProtocol* Clone() const\r\n\t{\r\n\t\treturn new ";
			}

			sFmtValue += ClassName;
			sFmtValue += ";\r\n\t};";

			CloneFunc = sFmtValue;
			return CloneFunc.size() != 0;
		}

		Bool  BuildOperatorSelfFunc()
		{
			OperatorSelfFunc = "";
			if (ClassType == TYPE_MARSHAL_CLASS)
			{
				OperatorSelfFunc = "virtual ";
				OperatorSelfFunc += ClassName;
				OperatorSelfFunc += "& operator = (const ";
				OperatorSelfFunc += ClassName;
				OperatorSelfFunc += "& rhs)\r\n\t{\r\n\t\tif(this != &rhs)\r\n\t\t{\r\n";

				for (Size_t i=0;i<VarData.size();i++)
				{
					GenVarData& xVar = VarData[i];
					OperatorSelfFunc += "\t\t\t";
					OperatorSelfFunc += xVar.GetSetValFunc();
					OperatorSelfFunc += "\r\n";
				}
				
				OperatorSelfFunc += "\t\t}\r\n\t\treturn *this;\r\n\t};";
			}
			return true;
		}

		Bool  BuildMarshalFunc()
		{
			MarshalFunc = "";
			if (ClassType == TYPE_MARSHAL_CLASS || ClassType == TYPE_PROTOCOL_CLASS)
			{
				MarshalFunc = "virtual HawkOctetsStream& Marshal(HawkOctetsStream& rhsOS)\r\n\t{\r\n";
				if (VarData.size())
				{
					MarshalFunc += "\t\trhsOS";
					for (Size_t i=0;i<VarData.size();i++)
					{
						MarshalFunc += " << ";
						MarshalFunc += VarData[i].Name;
					}
					MarshalFunc += ";\r\n";
				}
				MarshalFunc += "\t\treturn rhsOS;\r\n\t};";
			}
			return true;
		}

		Bool  BuildUnmarshalFunc()
		{
			UnmarshalFunc = "";
			if (ClassType == TYPE_MARSHAL_CLASS || ClassType == TYPE_PROTOCOL_CLASS)
			{
				UnmarshalFunc = "virtual HawkOctetsStream& Unmarshal(HawkOctetsStream& rhsOS)\r\n\t{\r\n";
				if (VarData.size())
				{
					UnmarshalFunc += "\t\trhsOS";
					for (Size_t i=0;i<VarData.size();i++)
					{
						UnmarshalFunc += " >> ";
						UnmarshalFunc += VarData[i].Name;
					}
					UnmarshalFunc += ";\r\n";
				}
				UnmarshalFunc += "\t\treturn rhsOS;\r\n\t};";
			}
			return true;
		}

		Bool  BuildClassMember()
		{
			MemberDefine = "";
			if (VarData.size())
			{
				MemberDefine += "\r\npublic:\r\n";
				for (Size_t i=0;i<VarData.size();i++)
				{
					GenVarData& xVar = VarData[i];
					MemberDefine += "\t";
					MemberDefine += xVar.GetMemberFmt();
					MemberDefine += "\r\n";
				}
			}
			return true;
		}

		Bool  BuildFunc()
		{
			BuildConstructFunc();
			BuildCopyFunc();
			BuildCloneFunc();
			BuildOperatorSelfFunc();
			BuildMarshalFunc();
			BuildUnmarshalFunc();
			BuildClassMember();
			return true;
		}

		AString  FormatString()
		{
			if (!BuildFunc()) return "";

			AString sFmtValue = "";
			memset(g_Buffer,0,MAX_BUFFER);
			
			if (ClassDesc.size())
			{
				memset(g_Buffer,0,MAX_BUFFER);
				sprintf(g_Buffer,"//%s\r\n",ClassDesc.c_str());
				sFmtValue += g_Buffer;				
			}

			if (ClassType == TYPE_MARSHAL_CLASS)
			{
				memset(g_Buffer,0,MAX_BUFFER);
				sprintf(g_Buffer,MARSHALDATA_CLASS,ClassName.c_str(),
					ConstructFunc.c_str(),
					CopyFunc.c_str(),
					CloneFunc.c_str(),
					OperatorSelfFunc.c_str(),
					MarshalFunc.c_str(),
					UnmarshalFunc.c_str(),
					MemberDefine.c_str());

				sFmtValue += g_Buffer;
			}
			else if (ClassType == TYPE_PROTOCOL_CLASS)
			{
				memset(g_Buffer,0,MAX_BUFFER);
				sprintf(g_Buffer,PROTOCOL_CLASS,ClassName.c_str(),
					ConstructFunc.c_str(),
					CloneFunc.c_str(),
					MarshalFunc.c_str(),
					UnmarshalFunc.c_str(),
					MemberDefine.c_str());

				sFmtValue += g_Buffer;
			}
		
			return sFmtValue;
		}
	};
	typedef vector<GenClassData> ClassVector;

	struct GenFileData
	{
		AString		Name;
		AString		Include;
		ClassVector Classes;

		GenFileData()
		{
			Name	= "";
			Classes.clear();
		}

		AString  FormatString()
		{			
			AString sFmtValue   = "";
			if (Include.size())
			{
				memset(g_Buffer,0,MAX_BUFFER);
				sprintf(g_Buffer,"#include \"%s\"\r\n",Include.c_str());
				sFmtValue += g_Buffer;
			}

			for (Size_t i=0;i<Classes.size();i++)
			{
				sFmtValue += Classes[i].FormatString();
			}

			HawkPrint(sFmtValue);
			return sFmtValue;
		}
	};

	struct MacroDefine 
	{
		AString MacroName;
		AString MacroValue;
		AString MacroDesc;

		MacroDefine()
		{
			MacroName = "";
			MacroValue = "";
			MacroDesc = "";
		}

		Bool IsValid()
		{
			return MacroName.size() && MacroValue.size();
		}

		AString  FormatString()
		{			
			AString sFmtValue   = "";
			
			memset(g_Buffer,0,MAX_BUFFER);

			if(MacroDesc.size())
				sprintf(g_Buffer,"\t\t//%s\r\n\t\t%s = %s,",MacroDesc.c_str(),MacroName.c_str(),MacroValue.c_str());
			else
				sprintf(g_Buffer,"\t\t%s = %s,",MacroName.c_str(),MacroValue.c_str());

			sFmtValue = g_Buffer;

			return sFmtValue;
		}
	};
	typedef vector<MacroDefine> MacroVector;

	struct GenSpaceData
	{
		AString		Space;
		AString		Desc;
		MacroVector	Macros;

		GenSpaceData()
		{
			Space = "";
			Desc  = "";
			Macros.clear();
		}

		Bool  IsValid()
		{
			return Space.size() != 0;
		}

		AString  FormatString()
		{			
			AString sFmtValue   = "";

			memset(g_Buffer,0,MAX_BUFFER);
			if (Desc.size())
				sprintf(g_Buffer,"//%s\r\nnamespace %s\r\n{\r\n\tenum\r\n\t{\r\n",Desc.c_str(),Space.c_str());
			else
				sprintf(g_Buffer,"namespace %s\r\n{\r\n\tenum\r\n\t{\r\n",Space.c_str());
			
			sFmtValue += g_Buffer;

			for (Size_t i=0;i<Macros.size();i++)
			{
				if(i != 0)
					sFmtValue += "\r\n";
				sFmtValue += Macros[i].FormatString();
			}

			sFmtValue += "\r\n\t};\r\n};\r\n\r\n";

			HawkPrint(sFmtValue);
			return sFmtValue;
		}
	};
	typedef vector<GenSpaceData> SpaceVector;

	struct GenMacroData
	{
		AString		Name;
		SpaceVector Spaces;

		GenMacroData()
		{
			Name = "";
			Spaces.clear();
		}

		AString  FormatString()
		{			
			AString sFmtValue   = "";

			for (Size_t i=0;i<Spaces.size();i++)
			{
				if(i != 0)
					sFmtValue += "\r\n";
				sFmtValue += Spaces[i].FormatString();
			}

			return sFmtValue;
		}
	};

	Bool ParseElement(AXmlElement* pElement,GenFileData& xGenData,MacroVector& vMacro)
	{
		if (pElement && pElement->GetAttribute("Name"))
		{
			GenClassData xClassData;

			if(pElement->GetAttribute("Name"))
				xClassData.ClassName = pElement->GetAttribute("Name")->StringValue();

			if(pElement->GetAttribute("Id"))
				xClassData.Identify = pElement->GetAttribute("Id")->IntValue();			

			if(xClassData.Identify)
			{
				if(g_ProtocolMap.find(xClassData.Identify) == g_ProtocolMap.end())
				{
					g_ProtocolMap[xClassData.Identify] = xClassData.Identify;
				}
				else
				{
					HawkAssert(false && "Protocol Id Repeat.");
					HawkFmtPrint("协议ID重复: %d",xClassData.Identify);
					HawkOSOperator::OSSleep(10000);
				}
			}			

			if(pElement->GetAttribute("Macro"))
				xClassData.Macro = pElement->GetAttribute("Macro")->StringValue();	

			if (pElement->GetAttribute("Desc"))
				xClassData.ClassDesc = pElement->GetAttribute("Desc")->StringValue();

			if (xClassData.Identify && xClassData.Macro.size())
			{
				MacroDefine xMacro;
				xMacro.MacroName = xClassData.Macro;
				xMacro.MacroValue= HawkStringUtil::IntToString<AString>(xClassData.Identify);
				xMacro.MacroDesc = xClassData.ClassDesc;

				if (xMacro.IsValid())
					vMacro.push_back(xMacro);
			}

			if (pElement->GetTag() == "Marshal")
				xClassData.ClassType = TYPE_MARSHAL_CLASS;
			else if (pElement->GetTag() == "Protocol")
				xClassData.ClassType = TYPE_PROTOCOL_CLASS;

			if (xClassData.ClassType == TYPE_PROTOCOL_CLASS)
			{
				g_ProtocolVec.push_back(xClassData.ClassName);
			}

			Int32 iVarCnt = pElement->GetChildrenNum();
			for (Int32 i=0;i<iVarCnt;i++)
			{
				AXmlElement* pVar = pElement->GetChildren(i);
				if (pVar && pVar->GetTag() == "Var")
				{
					GenVarData xVar;

					if (pVar->GetAttribute("Name"))
						xVar.Name = pVar->GetAttribute("Name")->StringValue();

					if (pVar->GetAttribute("Type"))
						xVar.Type = pVar->GetAttribute("Type")->StringValue();

					if (pVar->GetAttribute("Default"))
						xVar.Default = pVar->GetAttribute("Default")->StringValue();

					if (xVar.IsValid())
						xClassData.VarData.push_back(xVar);
				}				
			}

			xGenData.Classes.push_back(xClassData);
			return true;
		}
		return false;
	}

	Bool ParseIdSpace(AXmlElement* pElement,GenMacroData& xMacroData)
	{
		if (pElement && pElement->GetAttribute("Space"))
		{
			GenSpaceData xSpace;

			if(pElement->GetAttribute("Space"))
				xSpace.Space = pElement->GetAttribute("Space")->StringValue();

			if(pElement->GetAttribute("Desc"))
				xSpace.Desc = pElement->GetAttribute("Desc")->StringValue();

			Int32 iIdCnt = pElement->GetChildrenNum();
			for (Int32 i=0;i<iIdCnt;i++)
			{
				AXmlElement* pId = pElement->GetChildren(i);
				if (pId && pId->GetTag() == "Id")
				{
					MacroDefine xMacro;

					if (pId->GetAttribute("Macro"))
						xMacro.MacroName = pId->GetAttribute("Macro")->StringValue();

					if (pId->GetAttribute("Value"))
						xMacro.MacroValue = pId->GetAttribute("Value")->StringValue();

					if (pId->GetAttribute("Desc"))
						xMacro.MacroDesc = pId->GetAttribute("Desc")->StringValue();

					if (xMacro.IsValid())
					{
						Int32 iMacro = HawkStringUtil::StringToInt<AString>(xMacro.MacroValue);
						if (g_MacroMap.find(iMacro) != g_MacroMap.end())
						{
							HawkAssert(false && "Macro Id Repeat.");
							HawkFmtPrint("宏定义ID重复: %d",iMacro);
							HawkOSOperator::OSSleep(10000);
						}
						else
						{
							xSpace.Macros.push_back(xMacro);
						}	
					}
				}
			}

			if (xSpace.IsValid())
				xMacroData.Spaces.push_back(xSpace);

			return true;
		}
		return false;
	}

	Bool HawkProtocolManager::GenProtocols(const AString& sCfgFile, const AString& sHawkHead)
	{
		g_Buffer = (Char*)HawkMalloc(MAX_BUFFER);
		HawkScope::MallocPtr scope(g_Buffer);

		AString sXmlContent = "";

		HawkDiskFile protoXml;
		if (protoXml.Open(sCfgFile))
		{
			Int32 iSize = (Int32)protoXml.GetFileSize();
			Utf8* pData = (Utf8*)HawkMalloc(iSize + 1);
			memset(pData,0,iSize+1);
			protoXml.Read(pData,iSize);
			sXmlContent = HawkStringUtil::ToString(pData);
			HawkFree(pData);
		}
		else
		{
			return false;
		}

		HawkXmlFile  xml;
		AXmlDocument doc;
		if (sXmlContent.size() && xml.Open<AString>(sXmlContent.c_str(),sXmlContent.size(),doc))
		{
			AXmlElement* pRoot = doc.GetRoot();
			if (pRoot && pRoot->GetTag() == "ProtocolGen")
			{				
				if (pRoot->GetAttribute("ProtocolIdFile"))
					g_ProtocolIdFile = pRoot->GetAttribute("ProtocolIdFile")->StringValue();
				
				if (pRoot->GetAttribute("ProtocolRegFile"))
					g_ProtocolRegFile = pRoot->GetAttribute("ProtocolRegFile")->StringValue();

				MacroVector vProtocolMacro;

				Int32 iGenCount = pRoot->GetChildrenNum();
				for (Int32 i=0;i<iGenCount;i++)
				{
					AXmlElement* pGenFile = pRoot->GetChildren(i);
					if (pGenFile && pGenFile->GetTag() == "GenFile" && pGenFile->GetAttribute("Name"))
					{
						GenFileData		xGenData;
						GenMacroData	xMacroData;
						
						AString sName	= pGenFile->GetAttribute("Name")->StringValue();
						xGenData.Name	= sName;
						xMacroData.Name = sName;
						AString sFile	= sName + ".h";
						
						HawkDiskFile xFile;
						if (xFile.Open(sFile,HawkFile::OPEN_WRITE))
						{							
							AString sContent = "";

							if (pGenFile->GetAttribute("Include"))
							{
								AString sInclude = pGenFile->GetAttribute("Include")->StringValue();
								
								AStringVector vInclude;
								HawkStringUtil::Split<AString>(sInclude,vInclude,",");

								memset(g_Buffer,0,MAX_BUFFER);
								AString sUpCaseName = HawkStringUtil::UpCase<AString>(sName);
								sprintf(g_Buffer,"#ifndef __%s_H__\r\n#define __%s_H__\r\n\r\n",sUpCaseName.c_str(),sUpCaseName.c_str());

								sContent += g_Buffer;

								for (Size_t k=0;k<vInclude.size();k++)
								{
									memset(g_Buffer,0,MAX_BUFFER);
									sprintf(g_Buffer,"#include \"%s.h\"\r\n",vInclude[k].c_str());
									sContent += g_Buffer;
								}

								if(vInclude.size())
									sContent += "\r\n";

								xFile.Write(sContent.c_str(),sContent.size());
							}
							else
							{
								memset(g_Buffer,0,MAX_BUFFER);
								AString sUpCaseName = HawkStringUtil::UpCase<AString>(sName);
								sprintf(g_Buffer,"#ifndef __%s_H__\r\n#define __%s_H__\r\n\r\n#include \"%s\"\r\n\r\n",sUpCaseName.c_str(),sUpCaseName.c_str(),sHawkHead.c_str());

								sContent += g_Buffer;
								xFile.Write(sContent.c_str(),sContent.size());
							}

							Int32 iChildCnt = pGenFile->GetChildrenNum();
							for (Int32 j=0;j<iChildCnt;j++)
							{
								AXmlElement* pElement = pGenFile->GetChildren(j);
								xGenData.Classes.clear();
								xMacroData.Spaces.clear();

								if(pElement->GetTag() == "IdSpace")
								{
									ParseIdSpace(pElement,xMacroData);
									sContent = xMacroData.FormatString();
									xFile.Write(sContent.c_str(),sContent.size());
								}
								else
								{
									ParseElement(pElement,xGenData,vProtocolMacro);
									sContent = xGenData.FormatString();
									xFile.Write(sContent.c_str(),sContent.size());
								}								
							}
							sContent = "#endif\r\n";
							xFile.Write(sContent.c_str(),sContent.size());
							xFile.Close();
						}
					}
				}

				if (vProtocolMacro.size() && g_ProtocolIdFile.size())
				{
					AString sFile = g_ProtocolIdFile + ".h";

					HawkDiskFile xFile;
					if (xFile.Open(sFile,HawkFile::OPEN_WRITE))
					{
						memset(g_Buffer,0,MAX_BUFFER);
						sprintf(g_Buffer,"#ifndef __PROTOCOLID_H__\r\n#define __PROTOCOLID_H__\r\n\r\n#include \"%s\"\r\n\r\n",sHawkHead.c_str());
						AString sContent = g_Buffer;
						xFile.Write(sContent.c_str(),sContent.size());

						AString sFmtValue = "";
						sFmtValue += "namespace ";
						sFmtValue += g_ProtocolIdFile;
						sFmtValue += "\r\n{\r\n\tenum\r\n\t{\r\n";

						for (Size_t i=0;i<vProtocolMacro.size();i++)
						{
							if(i!=0)
								sFmtValue += "\r\n";
							sFmtValue += vProtocolMacro[i].FormatString();
						}
						sFmtValue += "\r\n\t};\r\n}\r\n#endif\r\n";
						xFile.Write(sFmtValue.c_str(),sFmtValue.size());
						xFile.Close();
					}
				}
			}			

			if (g_ProtocolVec.size() && g_ProtocolRegFile.size())
			{
				HawkDiskFile sRegFile;
				if (sRegFile.Open(g_ProtocolRegFile,HawkFile::OPEN_WRITE))
				{
					for (Size_t i=0;i<g_ProtocolVec.size();i++)
					{
						Char sRegInfo[1024] = {0};
						sprintf(sRegInfo,"REGISTER_PROTO(%s);\r\n",g_ProtocolVec[i].c_str());
						sRegFile.Write(sRegInfo,strlen((sRegInfo)));
					}
					sRegFile.Close();
				}
			}

			return true;
		}
		return false;
	}
}

#include "HawkMemoryFile.h"
#include "HawkDiskFile.h"
#include "HawkMath.h"

namespace Hawk
{
	HawkMemoryFile::HawkMemoryFile()
	{
		m_iFilePos = 0;
		m_pData    = 0;
		m_bExtra   = false;
	}

	HawkMemoryFile::~HawkMemoryFile()
	{
		Close();
	}

	Bool HawkMemoryFile::Open(Int64 iSize, OpenType eOpen)
	{
		Close();
	
		m_eOpenType  = eOpen;
		m_iFileSize  = iSize;
		m_bExtra     = false;

		if(iSize > 0)
		{
			m_pData = (Char*)HawkMalloc((Size_t)m_iFileSize);
			memset(m_pData, 0, (Size_t)m_iFileSize);
		}

		return true;
	}

	Bool HawkMemoryFile::Open(void* pData, Int64 iSize, Bool bExtra, OpenType eOpen)
	{
		Close();

		m_eOpenType = eOpen;
		m_iFileSize = iSize;
		m_bExtra	= bExtra;

		if (!m_bExtra)
		{
			m_pData = (Char*)HawkMalloc((Size_t)m_iFileSize);
			memcpy(m_pData, pData, (Size_t)m_iFileSize);
		}
		else
		{
			m_pData	= pData;
		}		

		return true;
	}

	Bool HawkMemoryFile::Open(const AString& sFile)
	{
		Close();

		m_eOpenType = OPEN_READ;
		m_bExtra    = false;

		HawkDiskFile df;
		if (df.Open(sFile))
		{
			m_iFileSize = df.GetFileSize();

			m_pData = (Char*)HawkMalloc((Size_t)m_iFileSize);
			memset(m_pData, 0, (Size_t)m_iFileSize);

			df.Read(m_pData, m_iFileSize);
			return true;
		}
		return false;
	}

	Bool HawkMemoryFile::Close()
	{
		m_iFilePos  = 0;
		m_iFileSize = 0;

		if (!m_bExtra)
		{
			HawkFree(m_pData);
			m_pData = 0;
		}
		else
		{
			m_pData = 0;
		}

		return true;
	}

	Int64 HawkMemoryFile::Read(void* pData, Int64 iSize)
	{
		if (!pData || iSize <= 0 || (m_eOpenType != OPEN_READ && m_eOpenType != OPEN_RW))
			return 0;
		
		iSize = HawkMath::Min<Int64>(m_iFileSize - m_iFilePos, iSize);
		
		memcpy(pData, (Char*)m_pData + m_iFilePos, (Size_t)iSize);

		m_iFilePos += iSize;

		return iSize;
	}

	Int64 HawkMemoryFile::Write(const void* pData, Int64 iSize, Bool bFlush)
	{
		if (!pData || iSize <= 0 || (m_eOpenType != OPEN_WRITE && m_eOpenType != OPEN_RW && m_eOpenType != OPEN_APPEND) )
			return 0;

		if (m_iFilePos + iSize > m_iFileSize) 
		{
			Int64 iTmp = m_iFileSize + iSize;
			void* pTmp = HawkMalloc((Size_t)iTmp);
			memset(pTmp, 0, (Size_t)iTmp);
			
			if (m_pData)
			{
				memcpy(pTmp, m_pData, (Size_t)m_iFileSize);
				HawkFree(m_pData);
				m_pData = 0;
			}
			
			m_pData = pTmp;
			m_iFileSize = m_iFilePos + iSize;
		}

		memcpy((Char*)m_pData+m_iFilePos, pData, (Size_t)iSize);
		m_iFilePos  += iSize;
		return iSize;
	}

	Int64 HawkMemoryFile::Tell()
	{
		return m_iFilePos;
	}

	Int64 HawkMemoryFile::Seek(Int64 iOffset, SeekPos ePos)
	{
		Int64 iRealPos = m_iFilePos;
		Int64 iOldPos  = m_iFilePos;
		if (ePos == POS_BEGIN)
		{
			iRealPos = iOffset;
		}
		else if (ePos == POS_CURRENT)
		{
			iRealPos = m_iFilePos + iOffset;
		}
		else
		{
			iRealPos = m_iFileSize + iOffset;
		}

		if(iRealPos > m_iFileSize)
		{
			if((m_eOpenType == OPEN_WRITE || m_eOpenType == OPEN_RW || m_eOpenType == OPEN_APPEND))
			{		
				void* pData = HawkMalloc((Size_t)iRealPos);
				if (pData)
				{
					memset(pData, 0, (Size_t)iRealPos);
					if(m_pData)
					{
						memcpy(pData, m_pData, (Size_t)m_iFileSize);

						HawkFree(m_pData);
						m_pData = 0;
					}
				}
				
				m_iFileSize = iRealPos;
				m_iFilePos  = iRealPos;				
				m_pData     = pData;
			}
			else
			{
				m_iFilePos = m_iFileSize;
			}
		}
		else
		{
			m_iFilePos = iRealPos;
		}

		return m_iFilePos - iOldPos;
	}

	Bool HawkMemoryFile::Chsize(Int64 iSize)
	{		
		if((m_eOpenType == OPEN_WRITE || m_eOpenType == OPEN_RW || m_eOpenType == OPEN_APPEND))
		{
			void* pData = HawkMalloc((Size_t)iSize);
			if (pData)
			{
				memset(pData, 0, (Size_t)iSize);
				if(m_pData)
				{
					Int64 iTmp = HawkMath::Min<Int64>(iSize, m_iFileSize);
					memcpy(pData, m_pData, (Size_t)iTmp);
					
					HawkFree(m_pData);
					m_pData = 0;
				}
			}			

			m_iFileSize = iSize;
			m_iFilePos  = 0;				
			m_pData     = pData;
			return true;
		}
		return false;
	}

	Bool HawkMemoryFile::SaveToDisk(const AString& sFile)
	{
		HawkDiskFile df;
		if (m_pData && m_iFileSize > 0 && df.Open(sFile, OPEN_WRITE))
		{
			Int64 iSize  = df.Write(m_pData, m_iFileSize);
			return iSize == m_iFileSize;
		}
		return false;
	}
}

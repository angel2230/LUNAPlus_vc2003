#include "stdafx.h"
#include "cIMEex.h"
#include "./interface/cFont.h"
#include "./interface/cEditbox.h"

// 070202 LYW --- Include header file.
#include "../[CC]Header/GameResourceManager.h"

cIMEex::cIMEex()
{
	m_wFontIdx	= 0;

	m_pStrBuf	= NULL;
	m_InsertPos.nLine	= 0;
	m_InsertPos.nIndex	= 0;
	
	m_nCurTotalLen		= 0;
	m_nCurTotalLine		= 1;

	m_nMaxTotalLen		= 128;	
	m_nMaxTotalLine		= 1;

	m_bCompositing		= FALSE;
	m_bMultiLine		= FALSE;

	m_bEnterAllow		= TRUE;
	
	const DISPLAY_INFO& dispInfo = GAMERESRCMNGR->GetResolution();
	m_nLimitWidth		= dispInfo.dwWidth ; 
	// 070202 LYW --- End.

	m_pParentEdit		= NULL;

	SetValidCheckMethod( VCM_DEFAULT );

	// 091126 ShinJS --- CheckMethod 처리로 인하여 기본문자를 설정한다.
	ZeroMemory( m_CheckMethodBaseStr, MAX_LINE );
	m_CheckMethodBaseStr[0] = '0';
}


cIMEex::~cIMEex()
{

}


void cIMEex::Init( int nMaxBufSize, int nPixelWidth, BOOL bMultiLine, int nMaxLine )
{
	m_nMaxTotalLen	= nMaxBufSize;

	m_nLimitWidth	= nPixelWidth;
	m_bMultiLine	= bMultiLine;

	if( m_bMultiLine )
	{
		m_nMaxTotalLine	= nMaxLine;
	}
	else
	{
		m_nMaxTotalLine	= 1;
	}


	m_bChanged		= FALSE;

	m_pStrBuf		= new char[nMaxBufSize+1];
	ZeroMemory( m_pStrBuf, nMaxBufSize+1 );
	ZeroMemory(
		m_pLineDesc,
		sizeof(m_pLineDesc));

	SetValidCheckMethod( VCM_DEFAULT );
}


void cIMEex::Release()
{
	if( m_pStrBuf )
	{
		delete[] m_pStrBuf;
		m_pStrBuf = NULL;
	}
}


BOOL cIMEex::Paste( char* str )
{
	if( *str == 0 ) return FALSE;

	char buf[3];

	while( *str )
	{
		if( str[0] == 13 && str[1] == 10 )
		{
			if( Enter() == FALSE )			return FALSE;
			str += 2;
		}
		else if( IsDBCSLeadByte(*str) )
		{
			buf[0] = str[0];
			buf[1] = str[1];
			buf[2] = 0;
			str += 2;

			if( IsValidChar( (unsigned char*)buf ) )
			if( Insert( buf ) == FALSE )
				return FALSE;
		}
		else
		{
			buf[0] = str[0];
			buf[1] = 0;
			++str;

			if( IsValidChar( (unsigned char*)buf ) )
			if( Insert( buf ) == FALSE )
				return FALSE;
		}
	}

	return TRUE;
}


BOOL cIMEex::Insert( char* str )	//오직 한글자(1~2byte)
{
	m_bChanged = TRUE;

	if( m_bCompositing )
		DeleteBefore();

	int nLen			= strlen( str );
	if( nLen == 0 ) return FALSE;

	if( nLen > 2 )	//Paste로만 가능
	{
		str[2] = 0;
		nLen = 2;
	}
		
	if( m_nCurTotalLen + nLen > m_nMaxTotalLen )
		return FALSE;

	int nInsertIndex	= GetInsertIndex();
	char* p				= GetInsertBufPtr();

	memmove( p + nLen, p, m_nCurTotalLen - nInsertIndex );
	m_nCurTotalLen		+= nLen;
	memcpy( p, str, nLen );

	m_InsertPos.nIndex	+= nLen;
	m_pLineDesc[m_InsertPos.nLine].nLen += nLen;

	for( int i = m_InsertPos.nLine + 1 ; i < m_nCurTotalLine ; ++ i )
	{
		m_pLineDesc[i].nStartIndex += nLen;
	}

	if( !Wrap( m_InsertPos.nLine  ) )
	{
		DeleteBefore();
		return FALSE;
	}

	return TRUE;
}


BOOL cIMEex::Wrap( int nStartLine )
{
	if( m_bMultiLine == FALSE ) return TRUE;

	LONG lExtent = CFONT_OBJ->GetTextExtentEx( m_wFontIdx,
							m_pStrBuf + m_pLineDesc[nStartLine].nStartIndex,
							m_pLineDesc[nStartLine].nLen );

	int nWrapNum = 0;
	int nStartIndex = m_pLineDesc[nStartLine].nStartIndex;
	int nWrapIndex = m_pLineDesc[nStartLine].nLen;

	char* p;
	int nMoveNum;

////////////////////////

	if( lExtent > m_nLimitWidth )
	{
//		if( m_bMultiLine == FALSE || m_nCurTotalLine >= m_nMaxTotalLine )
		if( m_bMultiLine == FALSE || m_nCurTotalLine >= m_nMaxTotalLine )
			return FALSE;	//쨌징횉횓 횉횘쩌철 쩐첩쨈횢.
		
		while( lExtent > m_nLimitWidth )		
		{
			p		 = m_pStrBuf + nStartIndex + nWrapIndex;
//			nMoveNum = IsDBCSLeadByte( *(p-1) ) ? 2 : 1;
			nMoveNum = IsDBCSLeadByte( *CharPrev( m_pStrBuf, p ) ) ? 2 : 1;

			nWrapNum += nMoveNum;
			nWrapIndex -= nMoveNum;
			
			if( m_InsertPos.nIndex > nWrapIndex )
			{
				m_InsertPos.nIndex = nWrapNum;
				++m_InsertPos.nLine;
			}
			
			lExtent = CFONT_OBJ->GetTextExtentEx( m_wFontIdx, 
				m_pStrBuf + m_pLineDesc[nStartLine].nStartIndex,
				nWrapIndex );
		}
		
		if( m_pLineDesc[nStartLine].nEndKind != EK_WRAP )
		{
			
			for( int i = m_nCurTotalLine ; i > nStartLine ; --i )
			{
				m_pLineDesc[i] = m_pLineDesc[i-1];
			}
			
			m_pLineDesc[nStartLine].nLen -= nWrapNum;
			m_pLineDesc[nStartLine].nEndKind = EK_WRAP;
			
			m_pLineDesc[nStartLine+1].nStartIndex = nStartIndex + nWrapIndex ;
			m_pLineDesc[nStartLine+1].nLen = nWrapNum;
			
			m_nCurTotalLine++;

			return TRUE;
		}
		else
		{			
			m_pLineDesc[nStartLine].nLen -= nWrapNum;
			
			m_pLineDesc[nStartLine+1].nStartIndex -= nWrapNum;
			m_pLineDesc[nStartLine+1].nLen += nWrapNum;
			
			return Wrap( nStartLine + 1 );
			
		}
	}

////////////////////////

	else
	{
		if( m_pLineDesc[nStartLine].nEndKind != EK_WRAP )
			return TRUE;	//쨌징횉횓횉횘 횉횎쩔채쩐첩쨈횢

		while( 1 )	
		{
			if( m_pLineDesc[nStartLine+1].nLen <= -nWrapNum ) 
				break;
			
			p		 = m_pStrBuf + nStartIndex + nWrapIndex;
			nMoveNum = IsDBCSLeadByte( *p ) ? 2 : 1;
			
			nWrapNum -= nMoveNum;
			nWrapIndex += nMoveNum;
			
			lExtent = CFONT_OBJ->GetTextExtentEx( m_wFontIdx, 
				m_pStrBuf + m_pLineDesc[nStartLine].nStartIndex,
				nWrapIndex );

			if( lExtent > m_nLimitWidth )
			{
				nWrapNum += nMoveNum;
				nWrapIndex -= nMoveNum;
				break;
			}
		}

		if( nWrapNum == 0 ) return TRUE;

		m_pLineDesc[nStartLine].nLen -= nWrapNum;
		
		m_pLineDesc[nStartLine+1].nStartIndex -= nWrapNum;
		m_pLineDesc[nStartLine+1].nLen += nWrapNum;

		if( m_pLineDesc[nStartLine+1].nLen <=0 )
		{
			--m_nCurTotalLine;

			m_pLineDesc[nStartLine].nEndKind = m_pLineDesc[nStartLine+1].nEndKind;

			for( int i = nStartLine+1 ; i < m_nCurTotalLine ; ++i )
			{
				m_pLineDesc[i] = m_pLineDesc[i+1];
			}

			return TRUE;
		}
		
		return Wrap( nStartLine + 1 );
	}
}



BOOL cIMEex::Enter()
{
	if( m_bEnterAllow == FALSE ) return FALSE;

	m_bChanged = TRUE;

	if( m_bMultiLine == FALSE || m_nMaxTotalLine <= m_nCurTotalLine )	return FALSE;

	int nBreakLen	= m_pLineDesc[m_InsertPos.nLine].nLen - m_InsertPos.nIndex;
	int nStartIndex = m_pLineDesc[m_InsertPos.nLine].nStartIndex + m_InsertPos.nIndex;

	m_pLineDesc[m_InsertPos.nLine].nLen		= m_InsertPos.nIndex;
	m_pLineDesc[m_InsertPos.nLine].nEndKind = EK_ENTER;

	m_InsertPos.nIndex = 0;
	++m_InsertPos.nLine;

	for( int i = m_nCurTotalLine ; i >= m_InsertPos.nLine ; --i )
	{
		m_pLineDesc[i] = m_pLineDesc[i-1];
	}

	m_pLineDesc[m_InsertPos.nLine].nLen			= nBreakLen;
	m_pLineDesc[m_InsertPos.nLine].nStartIndex	= nStartIndex;

	++m_nCurTotalLine;

	return TRUE;
}


BOOL cIMEex::CaretMoveLeft()
{
	m_bChanged = TRUE;
	int nInsertIndex	= GetInsertIndex();
	if( nInsertIndex <= 0 ) return FALSE;

	if( m_InsertPos.nIndex <= 0 )	
	{
		--m_InsertPos.nLine;
		m_InsertPos.nIndex = m_pLineDesc[m_InsertPos.nLine].nLen;
		if( m_pLineDesc[m_InsertPos.nLine].nEndKind == EK_WRAP )
			CaretMoveLeft();
	}
	else
	{
		char* p				= GetInsertBufPtr();
//		int nMoveNum		= IsDBCSLeadByte( *(p-1) ) ? 2 : 1;
		int nMoveNum		= IsDBCSLeadByte( *CharPrev( m_pStrBuf, p ) ) ? 2 : 1;
		m_InsertPos.nIndex -= nMoveNum;
	}

	return TRUE;
}


BOOL cIMEex::CaretMoveRight()
{
	m_bChanged = TRUE;
	int nInsertIndex	= GetInsertIndex();
	if( nInsertIndex >= m_nCurTotalLen ) return FALSE;

	if( m_InsertPos.nIndex >= m_pLineDesc[m_InsertPos.nLine].nLen )
	{
		++m_InsertPos.nLine;
		m_InsertPos.nIndex = 0;
		if( m_pLineDesc[m_InsertPos.nLine].nEndKind == EK_WRAP )
			CaretMoveRight();
	}
	else
	{
		char* p				= GetInsertBufPtr();
		int nMoveNum		= IsDBCSLeadByte( *p ) ? 2 : 1;
		m_InsertPos.nIndex += nMoveNum;
	}

	return TRUE;
}



BOOL cIMEex::CaretMoveUp()
{
	m_bChanged = TRUE;
	if( m_InsertPos.nLine <= 0 ) return FALSE;

	--m_InsertPos.nLine;

	if( m_InsertPos.nIndex >= m_pLineDesc[m_InsertPos.nLine].nLen )
	{
		m_InsertPos.nIndex = m_pLineDesc[m_InsertPos.nLine].nLen;
	}
	else if( m_InsertPos.nIndex != 0 )
	{
		char* p = GetInsertBufPtr();
//		if( IsDBCSLeadByte( *p ) && IsDBCSLeadByte( *(p-1) ) ) //횉횗짹횤횁횩째짙
		if( p != CharNext( CharPrev( m_pStrBuf, p ) ) )
		{
			--m_InsertPos.nIndex;
		}
	}

	return TRUE;
}


BOOL cIMEex::CaretMoveDown()
{
	m_bChanged = TRUE;
	if( m_InsertPos.nLine >= m_nCurTotalLine - 1 ) return FALSE;
	
	++m_InsertPos.nLine;

	if( m_InsertPos.nIndex >= m_pLineDesc[m_InsertPos.nLine].nLen )
	{
		m_InsertPos.nIndex = m_pLineDesc[m_InsertPos.nLine].nLen;
	}
	else if( m_InsertPos.nIndex != 0 )
	{
		char* p = GetInsertBufPtr();
//		if( IsDBCSLeadByte( *p ) && IsDBCSLeadByte( *(p-1) ) ) //횉횗짹횤횁횩째짙
		if( p != CharNext( CharPrev( m_pStrBuf, p ) ) )
		{
			--m_InsertPos.nIndex;
		}
	}

	return TRUE;
}


BOOL cIMEex::CaretMoveHome()
{
	m_bChanged = TRUE;	
	m_InsertPos.nIndex = 0;
	return TRUE;
}


BOOL cIMEex::CaretMoveEnd()
{
	m_bChanged = TRUE;
	m_InsertPos.nIndex = m_pLineDesc[m_InsertPos.nLine].nLen;
	return TRUE;
}

BOOL cIMEex::CaretMoveFirst()
{
	m_bChanged = TRUE;	//for scroll and so on
	m_InsertPos.nIndex	= 0;
	m_InsertPos.nLine	= 0;
	return TRUE;
}


BOOL cIMEex::DeleteAfter()
{
	m_bChanged = TRUE;
	int nInsertIndex	= GetInsertIndex();
	if( m_InsertPos.nLine >= m_nCurTotalLine - 1 && nInsertIndex >= m_nCurTotalLen ) return FALSE;

	char* p				= GetInsertBufPtr();
	int nDeleteNum		= IsDBCSLeadByte( *p ) ? 2 : 1;

	if( m_InsertPos.nIndex >= m_pLineDesc[m_InsertPos.nLine].nLen )
	{
		
		if( m_pLineDesc[m_InsertPos.nLine+1].nLen == 0 )
		{
			--m_nCurTotalLine;
			for( int i = m_InsertPos.nLine+1 ; i < m_nCurTotalLine ; ++i )
			{
				m_pLineDesc[i] = m_pLineDesc[i+1];
			}

			m_pLineDesc[m_InsertPos.nLine].nEndKind = m_pLineDesc[m_InsertPos.nLine+1].nEndKind;
		}
		else
		{
			if( m_pLineDesc[m_InsertPos.nLine].nEndKind == EK_WRAP )
			{
				
				m_InsertPos.nIndex = 0;
				++m_InsertPos.nLine;
				return DeleteAfter();
			}

			m_pLineDesc[m_InsertPos.nLine].nEndKind = EK_WRAP;
		}
	}
	else
	{
		memmove( p, p + nDeleteNum, m_nCurTotalLen - nInsertIndex - nDeleteNum );
		m_nCurTotalLen -= nDeleteNum;
		ZeroMemory( m_pStrBuf + m_nCurTotalLen, nDeleteNum );

		m_pLineDesc[m_InsertPos.nLine].nLen -= nDeleteNum;	
		for( int i = m_InsertPos.nLine + 1 ; i < m_nCurTotalLine ; ++i )
		{
			m_pLineDesc[i].nStartIndex -= nDeleteNum;
		}

		if( m_InsertPos.nLine > 0 && m_pLineDesc[m_InsertPos.nLine].nLen == 0 )
		{
			if( m_pLineDesc[m_InsertPos.nLine-1].nEndKind == EK_WRAP )
			{
				m_pLineDesc[m_InsertPos.nLine-1].nEndKind = m_pLineDesc[m_InsertPos.nLine].nEndKind;
				
				--m_nCurTotalLine;
				--m_InsertPos.nLine;
				m_InsertPos.nIndex = m_pLineDesc[m_InsertPos.nLine].nLen;

				for( int i = m_InsertPos.nLine + 1 ; i < m_nCurTotalLine ; ++i )
				{
					m_pLineDesc[i] = m_pLineDesc[i+1];
				}
			}
		}
	}

	return Wrap( m_InsertPos.nLine );	
}


BOOL cIMEex::DeleteBefore()
{
	m_bChanged = TRUE;
	int nInsertIndex	= GetInsertIndex();
	if( nInsertIndex <= 0 ) return FALSE;
	
	char* p				= GetInsertBufPtr();
//	int nDeleteNum		= IsDBCSLeadByte( *(p-1) ) ? 2 : 1;		
														
	int nDeleteNum		= IsDBCSLeadByte( *CharPrev( m_pStrBuf, p ) ) ? 2 : 1;

	if( m_InsertPos.nIndex <= 0 )
	{
		--m_InsertPos.nLine;
		m_InsertPos.nIndex = m_pLineDesc[m_InsertPos.nLine].nLen;

		if( m_pLineDesc[m_InsertPos.nLine+1].nLen == 0 )
		{
			--m_nCurTotalLine;
			for( int i = m_InsertPos.nLine+1 ; i < m_nCurTotalLine ; ++i )
			{
				m_pLineDesc[i] = m_pLineDesc[i+1];
			}

			m_pLineDesc[m_InsertPos.nLine].nEndKind = m_pLineDesc[m_InsertPos.nLine+1].nEndKind;

		}
		else
		{
			if( m_pLineDesc[m_InsertPos.nLine].nEndKind == EK_WRAP )
			{
				return DeleteBefore();
			}

			m_pLineDesc[m_InsertPos.nLine].nEndKind = EK_WRAP;
		}
	}
	else
	{
		memmove( p - nDeleteNum, p, m_nCurTotalLen - nInsertIndex );

		m_nCurTotalLen		-= nDeleteNum;
		ZeroMemory( m_pStrBuf + m_nCurTotalLen, nDeleteNum );

		m_InsertPos.nIndex	-= nDeleteNum;
		m_pLineDesc[m_InsertPos.nLine].nLen -= nDeleteNum;

		for( int i = m_InsertPos.nLine + 1 ; i < m_nCurTotalLine ; ++i )
		{
			m_pLineDesc[i].nStartIndex -= nDeleteNum;
		}

		if( m_InsertPos.nLine > 0 && m_pLineDesc[m_InsertPos.nLine].nLen == 0 )
		{
			if( m_pLineDesc[m_InsertPos.nLine-1].nEndKind == EK_WRAP )
			{
				m_pLineDesc[m_InsertPos.nLine-1].nEndKind = m_pLineDesc[m_InsertPos.nLine].nEndKind;
				
				--m_nCurTotalLine;
				--m_InsertPos.nLine;
				m_InsertPos.nIndex = m_pLineDesc[m_InsertPos.nLine].nLen;

				for( int i = m_InsertPos.nLine + 1 ; i < m_nCurTotalLine ; ++i )
				{
					m_pLineDesc[i] = m_pLineDesc[i+1];
				}
			}
		}
	}

	return Wrap( m_InsertPos.nLine );
}


BOOL cIMEex::DeleteAllText()
{
	ZeroMemory( m_pStrBuf, m_nCurTotalLen );
	ZeroMemory( m_pLineDesc, m_nCurTotalLine * sizeof(sLineDesc) );

	m_InsertPos.nIndex	= 0;
	m_InsertPos.nLine	= 0;

	m_nCurTotalLine = 1;
	m_nCurTotalLen = 0;
	
	m_bCompositing = FALSE;

	return TRUE;
}


BOOL cIMEex::GetLineText( int nLine, char* strText, int* pStrLen )
{
	if( nLine >= m_nCurTotalLine ) return FALSE;

	strncpy( strText, m_pStrBuf + m_pLineDesc[nLine].nStartIndex, m_pLineDesc[nLine].nLen );
	strText[m_pLineDesc[nLine].nLen] = 0;

	if( pStrLen )
		*pStrLen = m_pLineDesc[nLine].nLen;
	
	return TRUE;
}




void cIMEex::SetScriptText( const char* str )
{
	DeleteAllText();

	char strInsert[3];

	while( *str )
	{
		switch( *str )
		{
		case TEXT_DELIMITER:
			{
				++str;
				switch( *str )
				{
				case TEXT_NEWLINECHAR:
					{
						Enter();
					}
					break;
				}
			}
			break;

		default:
			{
				if( IsDBCSLeadByte(*str) )
				{
					strInsert[0] = *str;
					strInsert[1] = *(++str);
					strInsert[2] = 0;
				}
				else
				{
					strInsert[0] = *str;
					strInsert[1] = 0;
				}

				Insert( strInsert );
			}
			break;
		}

		++str;
	}

	
}


void cIMEex::GetScriptText( char* str )
{
	*str = 0; //쩐짼쨌쨔짹창째짧 횁짚쨍짰
	for( int i = 0 ; i < m_nCurTotalLine ; ++i )
	{
		strncat( str, m_pStrBuf + m_pLineDesc[i].nStartIndex, m_pLineDesc[i].nLen );
		if( m_pLineDesc[i].nEndKind == EK_ENTER ) 
			strcat( str, "^n" );
	}
}


void cIMEex::SetValidCheckMethod( int nMethodNum )
{
	m_nValidCheckMethod = nMethodNum;
}



BOOL cIMEex::IsValidChar( unsigned char* str )
{
	if( *str == 0 ) return TRUE;
	
	BOOL bValid = FALSE;

	switch( m_nValidCheckMethod )
	{

	case VCM_SPACE:
		{
			if( *str == ' ' )
				break; 
		}
	case VCM_DBPARAM:
	case VCM_ID:
	case VCM_PASSWORD:
		{
			if( *str == '\'' )
				break;
		}
///
	case VCM_DEFAULT:
		{
			if( (*str >= 32 && *str < 127) || IsDBCSLeadByte(*str) )	//쩔쨉쨔짰, 횉횗짹횤..횉횗?횣 횈짱쩌철쨔짰?횣
			{
				bValid = TRUE;
				break;
			}
			
		}
	case VCM_CHARNAME:		
		{
#ifdef _TW_LOCAL_
			///////////
			if( IsDBCSLeadByte(*str) )	
			{
				bValid = TRUE;
				break;
			}
			///////////
#else
			if( str[0] >= 0xa1 && str[0] <= 0xac && str[1] >= 0x80 && str[1] <= 0xfe )
			{
				if( str[0] == 0xa4 || ( str[1] > 0x80 && str[1] < 0xa1 ) )
				{
					bValid = TRUE;
					break;
				}
//				if( str[0] == 0xa4 )	
//				{
//					bValid = TRUE;
//					break;
//				}
				break;
			}

			if( str[0] >= 0xb0 && str[0] <=0xc8 && str[1] >= 0xa1 && str[1] <= 0xfe )//0xB0A1~0xC8FE
			{
				bValid = TRUE;
				break;
			}
			if( str[0] >= 0x81 && str[0] <= 0xc6 && str[1] >= 0x41 && str[1] <= 0xfe )
			{
				bValid = TRUE;
				break;
			}
#endif
		}

//	case VCM_ID:
//	case VCM_PASSWORD:
		{
		
			if( ( *str >= 'A' && *str <= 'Z' ) || ( *str >= 'a' && *str <= 'z' ) )
			{
				bValid = TRUE;
				break;
			}

#ifdef _TL_LOCAL_
			if( *str >= 0xa1 && *str <= 0xd9 )
			{
				bValid = TRUE;
				break;
			}
			else if( *str >= 0xE0 && *str <= 0xEC )
			{
				bValid = TRUE;
				break;
			}
#endif
		}

	// 080403 LYW --- cIMEex : Add a enum type to use number without comma.
	case VCM_NORMAL_NUMBER :
	case VCM_NUMBER:
		{
			if( *str >= '0' && *str <= '9' )
			{
				bValid = TRUE;
				break;
			}
		}		
	}

	return bValid;
}

BOOL cIMEex::SetLimitLine( int nMaxLine )
{
	if( nMaxLine < m_nCurTotalLine )
		return FALSE;

	m_nMaxTotalLine = nMaxLine;
	return TRUE;
}

void cIMEex::CheckAfterChange()
{
	if( m_nValidCheckMethod == VCM_NUMBER )
	{
		// 100607 ONS 수량입력시 FOCUS가 생기면 새로 입력을 받는다.
		char buf[64] = {0};
		if( m_bIsFirstInput ) 
		{
			const char* const pBuf = RemoveComma(m_pStrBuf);
			int nLen = strlen(pBuf);
			buf[0] = pBuf[max(0, nLen - 1)];
			m_bIsFirstInput = FALSE;
		}
		else
		{
			strcpy( buf, RemoveComma(m_pStrBuf) );
		}
		
		int len = strlen(buf);

		if( len == 0 )
		{
			strcpy( buf, m_CheckMethodBaseStr );
		}
		else
		{
			if( len > 1 && *buf == '0' )
			{
				memmove( buf, buf+1, strlen(buf) );
			}

			AddComma( buf );
		}

		SetScriptText( buf );
	}
	// 080403 LYW --- cIMEex : Add a enum type to use number without comma.
	else if( m_nValidCheckMethod == VCM_NORMAL_NUMBER )
 	{
		char buf[64] = {0};

		if( m_bIsFirstInput ) 
		{
			int nLen = strlen(m_pStrBuf);
			buf[0] = m_pStrBuf[max(0, nLen - 1)];
			m_bIsFirstInput = FALSE;
		}
		else
		{
 			strcpy( buf, m_pStrBuf );
		}

 		int len = strlen(buf);
 
 		if( len == 0 )
 		{
 			strcpy( buf, m_CheckMethodBaseStr );
 		}
 		else
 		{
 			if( len > 1 && *buf == '0' )
 			{
 				memmove( buf, buf+1, strlen(buf) );
 			}
 		}
 
 		SetScriptText( buf );
 	}
}
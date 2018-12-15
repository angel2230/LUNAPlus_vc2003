// ServerTable.h: interface for the CServerTable class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERVERTABLE_H__C42B519F_767C_4291_B2DA_5F077251C100__INCLUDED_)
#define AFX_SERVERTABLE_H__C42B519F_767C_4291_B2DA_5F077251C100__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CServerTable  : public CYHHashTable<SERVERINFO>
{
	DWORD m_MaxServerConnectionIndex;
	int m_nDisableAgent[8];
	
public:
	CServerTable();
	virtual ~CServerTable();
	void Init(DWORD dwBucketNum);
	void Release();

	void SetDisableAgent( int nAgentNum )
	{
		if( nAgentNum >= 0 && nAgentNum < 8 )
			m_nDisableAgent[nAgentNum] = TRUE;
	}

	//---- Á¤º¸ Ã³¸® ¸â¹ö(ÇØ½¬·Î Ã³¸®)
	SERVERINFO * FindServer(WORD Port);
	SERVERINFO* FindServerForConnectionIndex(DWORD dwConnectionIndex);
	void AddServer(SERVERINFO * info, WORD Port);
	SERVERINFO * RemoveServer(DWORD dwConnectionIndex);			// key°¡ ¾Æ´Ô & ¸®ÅÏµÇ´Â SERVERINFO(Èü ¸Þ¸ð¸®)Áö¿ö¾ß ÇÔ
	SERVERINFO * RemoveServer(WORD wKey);
	void RemoveAllServer();										// HASH¿¡ ¹°·ÁÀÖ´Â SERVERINFO(Èü¸Þ¸ð¸®)±îÁö ´Ù Áö¿ò

	SERVERINFO* GetNextServer();
	SERVERINFO* GetNextDistServer();
	SERVERINFO* GetNextMapServer();
	SERVERINFO* GetNextAgentServer();
	
	void AddSelfServer(SERVERINFO * info);
	void AddMSServer(SERVERINFO * info);
	//---- Á¤º¸ ¾ò´Â ¸â¹ö
	SERVERINFO * GetSelfServer(){ return m_pSelfServerInfo; }
	SERVERINFO * GetMSServer(){ return m_pMSServerInfo; }

	
	SERVERINFO* GetServer(WORD ServerKind, WORD ServerNum);
	WORD GetServerPort(WORD ServerKind, WORD ServerNum);
	WORD GetServerNum(WORD ServerPort);
	DWORD GetMaxServerConnectionIndex() const { return m_MaxServerConnectionIndex; }
	void SetMaxServerConnectionIndex(DWORD val) { m_MaxServerConnectionIndex=val; }	
	SERVERINFO* GetFastServer(WORD ServerKind);
	BOOL GetFastServer(WORD ServerKind, char* pOutIP,WORD* pOutPort);

protected:
	SERVERINFO * m_pSelfServerInfo;
	SERVERINFO * m_pMSServerInfo;
};

//extern char * SERVER_KIND_STR[];
extern CServerTable * g_pServerTable;

#endif // !defined(AFX_SERVERTABLE_H__C42B519F_767C_4291_B2DA_5F077251C100__INCLUDED_)

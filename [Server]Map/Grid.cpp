// Grid.cpp: implementation of the CGrid class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Grid.h"
#include "PackedData.h"
#include "Network.h"
#include "CharMove.h"
#include "Object.h"
#include "Player.h"
// 080616 LUJ, 합정이 함정 소유주를 타겟으로 잡지 않도록 처리
#include "Trap.h"

// 081027 LYW --- GridSystem : 펫 매니져 호출.
#include "./PetManager.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGrid::CGrid()
{
	m_CharacterHashTable.Initialize(10);
}

CGrid::~CGrid()
{
	m_CharacterHashTable.RemoveAll();
}

void CGrid::AddObject(CObject* pObject)
{
	CObject* pPreObject = m_CharacterHashTable.GetData(pObject->GetID());
	if(pPreObject)
	{
		char temp[256];
		sprintf(temp,"PreObj %d,  %s,   %d", pPreObject->GetID(),pPreObject->GetObjectName(),pPreObject);
		ASSERTMSG(0,temp);
		sprintf(temp,"NewObj %d,  %s,   %d", pObject->GetID(),pObject->GetObjectName(),pObject);
		ASSERTMSG(0,temp);
		return;
	}
	m_CharacterHashTable.Add(pObject,pObject->GetID());
}

void CGrid::RemoveObject(CObject* pObject)
{
	ASSERT(m_CharacterHashTable.GetData(pObject->GetID()) != NULL);
	m_CharacterHashTable.Remove(pObject->GetID());
}

void CGrid::AddNeighborGrid(CGrid* pGrid)
{
	m_NeighborGrid.AddTail(pGrid);
}

void CGrid::AddGridToPackedData(CPackedData* pPackedData)
{
	POS pos = m_NeighborGrid.GetFirstPos();
	CGrid* pGrid;
	
	AddObjectToPacket(0,pPackedData);
	while( (pGrid = m_NeighborGrid.GetNextPos(pos)) != NULL)
	{
		pGrid->AddObjectToPacket(0,pPackedData);
	}
}

void CGrid::AddGridToPackedDataWithoutOne(DWORD dwID,CPackedData* pPackedData)
{
	POS pos = m_NeighborGrid.GetFirstPos();
	CGrid* pGrid;
	
	AddObjectToPacket(dwID,pPackedData);
	while( (pGrid = m_NeighborGrid.GetNextPos(pos)) != NULL)
	{
		pGrid->AddObjectToPacket(dwID,pPackedData);
	}
}

void CGrid::AddObjectToPacket(DWORD ExceptID,CPackedData* pPackedData)
{
	m_CharacterHashTable.SetPositionHead();
	CObject* pObject;
	while( (pObject = (CObject*)m_CharacterHashTable.GetData() ) != NULL)
	{
		if(pObject->GetObjectKind() != eObjectKind_Player)
			continue;
		if(ExceptID == pObject->GetID())
			continue;
		
		pPackedData->AddObject(pObject);
		//pObject->SendMsg(pPackedData->GetMsg(),pPackedData->GetMsgLen());
	}
}

void CGrid::AddGridToPackedDataExceptLastGrid(DWORD ExceptID,CGrid* pLastGrid,CPackedData* pPackedData)
{
	if(pLastGrid == NULL)
	{
		AddGridToPackedDataWithoutOne(ExceptID,pPackedData);
		return;
	}
	
	POS pos = m_NeighborGrid.GetFirstPos();
	CGrid* pGrid;
	
	if(!pLastGrid->IsNeighborGrid(this))
		AddObjectToPacket(ExceptID,pPackedData);
	
	while( (pGrid = m_NeighborGrid.GetNextPos(pos) ) != NULL)
	{
		if(!pLastGrid->IsNeighborGrid(pGrid))
			pGrid->AddObjectToPacket(ExceptID,pPackedData);
	}
}

BOOL CGrid::IsNeighborGrid(CGrid* pGrid)
{
	if(pGrid == this)
		return TRUE;
	
	CGrid* pNeighborGrid;
	POS pos = m_NeighborGrid.GetFirstPos();
	while( (pNeighborGrid = m_NeighborGrid.GetNextPos(pos) ) != NULL)
	{
		if(pNeighborGrid == pGrid)
			return TRUE;
	}
	return FALSE;
}

void CGrid::SendCurrentCharacterMsg(CObject* pNewObject)
{
	m_CharacterHashTable.SetPositionHead();
	
	while(CObject* const pObject = m_CharacterHashTable.GetData())
	{
		if(pNewObject->GetID() == pObject->GetID())
			continue;

		MSGBASE* message = 0;
		const DWORD messageLength = pObject->SetAddMsg(
			pNewObject->GetID(),
			FALSE,
			message);
		g_Network.Send2Server(
			pNewObject->GetAgentNum(),
			LPTSTR(message),
			messageLength);
	}	
}

void CGrid::SendCurrentObjectInfo(CObject* pNewObject,CGrid* pLastGrid)
{
	/////////////////////////////////////////////////////////////////////////
	// 06. 04. 그리드 수정
	// 이 함수는 추가된 오브젝트가 소속된 그리드의 오브젝트들이
	// 추가된 오브젝트에게 연결된 Agent에 메세지를 보내는 부분인데
	// 플레이어 이외의 오브젝트는 연결된 Agent가 없으므로 들어올 필요가 없다.
	if(pNewObject->GetObjectKind() != eObjectKind_Player)
		return;
	//////////////////////////////////////////////////////////////////////////

	POS pos = m_NeighborGrid.GetFirstPos();
	CGrid* pGrid;
	
	if(pLastGrid == NULL)
	{
		SendCurrentCharacterMsg(pNewObject);
		while( (pGrid = m_NeighborGrid.GetNextPos(pos)) != NULL)
		{
			pGrid->SendCurrentCharacterMsg(pNewObject);
		}
	}
	else
	{
		if(!pLastGrid->IsNeighborGrid(this))
		{
			SendCurrentCharacterMsg(pNewObject);
		}
		while( (pGrid = m_NeighborGrid.GetNextPos(pos)) != NULL)
		{
			if(!pLastGrid->IsNeighborGrid(pGrid))
			{
				pGrid->SendCurrentCharacterMsg(pNewObject);
			}
		}
	}
}


void CGrid::SendRemoveCharacterMsg(CObject* pLeavedObject)
{
	m_CharacterHashTable.SetPositionHead();
	CObject* pObject;
	while( (pObject = (CObject*)m_CharacterHashTable.GetData()) != NULL)
	{
		if(pLeavedObject->GetID() == pObject->GetID())
			continue;

		MSGBASE* message = 0;
		const DWORD messageLength = pObject->SetRemoveMsg(
			pLeavedObject->GetID(),
			message);
		g_Network.Send2Server(
			pLeavedObject->GetAgentNum(),
			LPTSTR(message),
			messageLength);
	}	
}

void CGrid::SendRemoveObjectInfo(CObject* pLeavedObject,CGrid* pCurGrid)
{
	
	/////////////////////////////////////////////////////////////////////////
	// 06. 04. 그리드 수정
	// 이 함수는 삭제된 오브젝트가 소속된 그리드의 오브젝트들이
	// 삭제된 오브젝트에게 연결된 Agent에 메세지를 보내는 부분인데
	// 플레이어 이외의 오브젝트는 연결된 Agent가 없으므로 들어올 필요가 없다.
	if(pLeavedObject->GetObjectKind() != eObjectKind_Player )
		return;
	/////////////////////////////////////////////////////////////////////////

	POS pos = m_NeighborGrid.GetFirstPos();
	CGrid* pGrid;
	
	if(pCurGrid == NULL)
	{
		SendRemoveCharacterMsg(pLeavedObject);
	//	SendRemoveItemMsg(pLeavedObject);
		while( (pGrid = m_NeighborGrid.GetNextPos(pos)) != NULL)
		{
			pGrid->SendRemoveCharacterMsg(pLeavedObject);
	//		pGrid->SendRemoveItemMsg(pLeavedObject);
		}
	}
	else
	{
		if(!pCurGrid->IsNeighborGrid(this))
		{
			SendRemoveCharacterMsg(pLeavedObject);
	//		SendRemoveItemMsg(pLeavedObject);
		}
		
		while( (pGrid = m_NeighborGrid.GetNextPos(pos)) != NULL)
		{
			if(!pCurGrid->IsNeighborGrid(pGrid))
			{
				pGrid->SendRemoveCharacterMsg(pLeavedObject);
	//			pGrid->SendRemoveItemMsg(pLeavedObject);
			}
		}
	}
}

void CGrid::FindPlayerInRange(CObject* pFinder,VECTOR3* pPos,float Range,float& MinRange,CObject** ppObject)
{
	MinRange = Range;
	*ppObject = NULL;
	float dist;
	
	m_CharacterHashTable.SetPositionHead();
	
	while(CObject* pObject = m_CharacterHashTable.GetData())
	{
		if(pObject->GetObjectKind() != eObjectKind_Player)
			continue;
		
		if(pObject->GetState() == eObjectState_Die || pObject->GetState() == eObjectState_Immortal)
			continue;

		/// 투명상태인 사람 패스(GM)
		if( !((CPlayer*)pObject)->IsVisible() )
			continue;
		
		dist = CalcDistanceXZ(pPos,CCharMove::GetPosition(pObject));
		if(dist < MinRange)
		{
			if(pFinder != pObject)
			{
				MinRange = dist;
				*ppObject = pObject;
			}
		}
	}
}

CObject* CGrid::FindPlayerInRange(CObject* pFinder,VECTOR3* pPos,float Range)
{
	POS pos = m_NeighborGrid.GetFirstPos();
	CGrid* pGrid;
	CObject* pObject;
	CObject* pClosestObject;
	float dist;
	float mindist;
	
	FindPlayerInRange(pFinder,pPos,Range,mindist,&pClosestObject);
	while( (pGrid = m_NeighborGrid.GetNextPos(pos) ) != NULL)
	{
		pGrid->FindPlayerInRange(pFinder,pPos,Range,dist,&pObject);
		if(dist < mindist)
		{
			mindist = dist;
			pClosestObject = pObject;
		}
	}
	
	return pClosestObject;
}

void CGrid::FindFirstPlayerInRange(CObject* pFinder,VECTOR3* pPos,float Range,CObject** ppObject)
{
	*ppObject = NULL;
	float dist;
	
	m_CharacterHashTable.SetPositionHead();

	while(CObject* pObject = m_CharacterHashTable.GetData())
	{
		if( pFinder == pObject ) continue;

		if(pObject->GetObjectKind() != eObjectKind_Player)
			continue;
		
		if(pObject->GetState() == eObjectState_Die || pObject->GetState() == eObjectState_Immortal)
			continue;

		/// 투명상태인 사람 패스(GM)
		if( !((CPlayer*)pObject)->IsVisible() )
			continue;
		
		dist = CalcDistanceXZ(pPos,CCharMove::GetPosition(pObject));
		if(dist < Range)
		{
			*ppObject = pObject;
			return;
		}
	}
}

CObject* CGrid::FindFirstPlayerInRange(CObject* pFinder,VECTOR3* pPos,float Range)
{
	POS pos = m_NeighborGrid.GetFirstPos();
	CGrid* pGrid;
	CObject* pObject;
	
	FindFirstPlayerInRange(pFinder,pPos,Range,&pObject);
	if(pObject)
		return pObject;

	while( (pGrid = m_NeighborGrid.GetNextPos(pos)) != NULL)
	{
		pGrid->FindFirstPlayerInRange(pFinder,pPos,Range,&pObject);
		if(pObject)
			return pObject;
	}
	
	return NULL;

}



void CGrid::FindMonsterInRange(CObject* pFinder,VECTOR3* pPos,float Range,float& MinRange,CObject** ppObject)
{
	MinRange = Range;
	*ppObject = NULL;
	float dist;
	
	m_CharacterHashTable.SetPositionHead();
	CObject* pObject;
	while( (pObject = (CObject*)m_CharacterHashTable.GetData()) != NULL)
	{
		if( pFinder == pObject ) continue;

		if(FALSE == (pObject->GetObjectKind() & eObjectKind_Monster))
			continue;
		
		if(pObject->GetState() == eObjectState_Die || pObject->GetState() == eObjectState_Immortal)
			continue;
		
		dist = CalcDistanceXZ(pPos,CCharMove::GetPosition(pObject));
		if(dist < MinRange)
		{
//			if(pFinder != pObject)
			{
				MinRange = dist;
				*ppObject = pObject;
			}
		}
	}
}

CObject* CGrid::FindMonsterInRange(CObject* pFinder,VECTOR3* pPos,float Range)
{
	POS pos = m_NeighborGrid.GetFirstPos();
	CGrid* pGrid;
	CObject* pObject;
	CObject* pClosestObject;
	float dist;
	float mindist;
	
	FindMonsterInRange(pFinder,pPos,Range,mindist,&pClosestObject);
	while( (pGrid = m_NeighborGrid.GetNextPos(pos)) != NULL)
	{
		pGrid->FindMonsterInRange(pFinder,pPos,Range,dist,&pObject);
		if(dist < mindist)
		{
			mindist = dist;
			pClosestObject = pObject;
		}
	}
	
	return pClosestObject;
}

void CGrid::FindFirstMonsterInRange(CObject* pFinder,VECTOR3* pPos,float Range,CObject** ppObject)
{
	*ppObject = NULL;
	float dist;
	
	m_CharacterHashTable.SetPositionHead();
	CObject* pObject;
	while( (pObject = (CObject*)m_CharacterHashTable.GetData()) != NULL)
	{
		if( pFinder == pObject ) continue;

		if(FALSE == (pObject->GetObjectKind() & eObjectKind_Monster))
			continue;
		
		if(pObject->GetState() == eObjectState_Die || pObject->GetState() == eObjectState_Immortal)
			continue;
		
		dist = CalcDistanceXZ(pPos,CCharMove::GetPosition(pObject));
		if(dist < Range)
		{
//			if(pFinder != pObject)
			{
				*ppObject = pObject;
				return;
			}
		}
	}
}

CObject* CGrid::FindFirstMonsterInRange(CObject* pFinder,VECTOR3* pPos,float Range)
{
	POS pos = m_NeighborGrid.GetFirstPos();
	CGrid* pGrid;
	CObject* pObject;
	
	FindFirstMonsterInRange(pFinder,pPos,Range,&pObject);
	if(pObject)
		return pObject;

	while( (pGrid = m_NeighborGrid.GetNextPos(pos) ) != NULL)
	{
		pGrid->FindFirstMonsterInRange(pFinder,pPos,Range,&pObject);
		if(pObject)
			return pObject;
	}
	
	return NULL;

}

WORD CGrid::FindMonsterGroupInRange(CObject* pFinder,VECTOR3* pPos, DWORD* pObjectList)
{
	WORD wFindNum = 0;

	m_CharacterHashTable.SetPositionHead();
	CObject* pObject;
	while( (pObject = (CObject*)m_CharacterHashTable.GetData()) != NULL)
	{
		if(wFindNum >= MAX_FIND_MONSTERNUM)
			break;

		if( pFinder == pObject ) continue;

		if(FALSE == (pObject->GetObjectKind() & eObjectKind_Monster))
			continue;

		if(pObject->GetGroupAlias() != pFinder->GetGroupAlias())
			continue;

		if(pObject->GetState() == eObjectState_Die || pObject->GetState() == eObjectState_Immortal)
			continue;

		pObjectList[wFindNum] = pObject->GetID();
		wFindNum++;
	}

	return wFindNum;
}

WORD CGrid::FindMonsterGroupInRange(DWORD* pFriendsList, CObject* pFinder,VECTOR3* pPos)
{
	POS pos = m_NeighborGrid.GetFirstPos();
	CGrid* pGrid;
	WORD wStartEmptyNum = 0;
	DWORD objectList[MAX_FIND_MONSTERNUM] = {0,};
	WORD wFindNum = FindMonsterGroupInRange(pFinder, pPos, &objectList[0]);

	for(WORD i=0; i<wFindNum; ++i)
	{
		pFriendsList[wStartEmptyNum] = objectList[i];
		wStartEmptyNum++;
	}

	while( (pGrid = m_NeighborGrid.GetNextPos(pos) ) != NULL)
	{
		if(wStartEmptyNum >= MAX_FIND_MONSTERNUM)
			return wStartEmptyNum;

		ZeroMemory(objectList, sizeof(objectList));

		wFindNum = pGrid->FindMonsterGroupInRange(pFinder, pPos, &objectList[0]);

		for(WORD i=0; i<wFindNum; i++)
		{
			if(wStartEmptyNum >= MAX_FIND_MONSTERNUM)
				return wStartEmptyNum;

			pFriendsList[wStartEmptyNum] = objectList[i];
			wStartEmptyNum++;
		}
	}

	return wStartEmptyNum;
}

CObject* CGrid::FindPlayerRandom()
{
	POS pos = m_NeighborGrid.GetFirstPos();
	CGrid* pGrid;
	CObject* pObject[9];
	CObject* temp;
	WORD count = 0;

	/// 우선 자기 그리드 내에서 플레이어를 고른다.
	FindPlayerRandom( &temp );
	if( temp )
	{
		pObject[count++] = temp;
	}

	/// 각 그리드에서 플레이어를 골라온다.
	while( ( pGrid = m_NeighborGrid.GetNextPos( pos ) ) != NULL)
	{
		temp = NULL;
		pGrid->FindPlayerRandom( &temp );
		if( temp )
		{
			pObject[count++] = temp;

			if(count == 9)
				break;
		}
	}
	
	/// 선택된 플레이어가 1인 이상일때
	if(count > 0)
	{
		/// 리턴할 플레이어를 고른다
		WORD select = WORD( rand() % count);

		return pObject[select];
	}

	/// 선택된 플레이어가 없으면 NULL 리턴
	return NULL;
}

void CGrid::FindPlayerRandom( CObject** ppObject )
{
	/// 전체 객체수
	DWORD num = m_CharacterHashTable.GetDataNum();
	
	/// 객체수가 0이면 NULL 리턴
	if(num == 0)
	{
		*ppObject = NULL;
		return;
	}

	/// 탐색을 시작할 기준점
	DWORD muster = 0;
	if( num > 1 )
		muster = rand() % ( num - 1 );

	/// 카운트용 변수
	DWORD count = 0;

	m_CharacterHashTable.SetPositionHead();
	CObject* pObject = NULL;
	while( ( pObject = ( CObject* )m_CharacterHashTable.GetData() ) != NULL)
	{
		/// 카운트가 기준점을 지난 지점부터 검색한다.
		if( count < muster )
		{
			count++;
			continue;
		}

		/// 플레이어가 아니면 패스
		if( pObject->GetObjectKind() != eObjectKind_Player )
			continue;

		/// 죽은 사람이나 무적 상태는 패스
		if( pObject->GetState() == eObjectState_Die || pObject->GetState() == eObjectState_Immortal )
			continue;

		/// 투명상태인 사람 패스(GM)
		if( !((CPlayer*)pObject)->IsVisible() )
			continue;

		/// 최초 발견된 플레이어를 리턴한다.
		*ppObject = pObject;
		return;
	}

	/// 만약 플레이어를 못찾았을 경우
	/// 처음부터 기준점까지 다시 검색한다.
	m_CharacterHashTable.SetPositionHead();
	pObject = NULL;
	count = 0;
	while( (pObject = ( CObject* )m_CharacterHashTable.GetData()) != NULL)
	{
		/// 기준점이 되면 검색 종료한다
		if( count >= muster )
		{
			break;
		}
		
		/// 카운트 증가
		count++;

		/// 플레이어가 아니면 패스
		if( pObject->GetObjectKind() != eObjectKind_Player )
			continue;

		/// 죽은 사람이나 무적 상태는 패스
		if( pObject->GetState() == eObjectState_Die || pObject->GetState() == eObjectState_Immortal )
			continue;

		/// 최초 발견된 플레이어를 리턴한다.
		*ppObject = pObject;
		return;
	}

	/// 그래도 못찾았다면 NULL 리턴
	*ppObject = NULL;
	return;
}
//////////////////////////////////////////////////////////////////////////
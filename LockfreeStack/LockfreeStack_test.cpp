#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <time.h>
#include <stdlib.h>

#include "lib\/Library.h"
#include "MemoryPool.h"
#include "LockFreeStack.h"
#include "LockfreeStack_test.h"

/*

 �׽�Ʈ ����
 - ���� ������ ������ ���� ������ ������ ��ġ Ȯ��
 - �����͸� �־��ٰ� ���� �ڿ� �� �����͸� �ٸ��̰� �޸𸮸� ����ϴ��� Ȯ�� (2������ �������� Ȯ��)

struct st_TEST_DATA
{
	volatile LONG64	lData;
	volatile LONG64	lCount;
};

//�� ����ü�� �����͸� �ٷ�� ���� ���� ����.

//CLockfreeStack<st_TEST_DATA *> g_Stack();


*/

CLockfreeStack<st_TEST_DATA *> g_Stack;

LONG64 lPushTPS = 0;
LONG64 lPopTPS = 0;

LONG64 lPushCounter = 0;
LONG64 lPopCounter = 0;

unsigned __stdcall StackThread(void *pParam);

void main()
{
	HANDLE hThread[dfTHREAD_MAX];
	DWORD dwThreadID;

	LOG_SET(LOG::FILE, LOG::LEVEL_DEBUG);

	for (int iCnt = 0; iCnt < dfTHREAD_MAX; iCnt++)
	{
		hThread[iCnt] = (HANDLE)_beginthreadex(
			NULL,
			0,
			StackThread,
			(LPVOID)1000,
			0,
			(unsigned int *)&dwThreadID
			);
	}

	while (1)
	{
		lPushTPS = lPushCounter;
		lPopTPS = lPopCounter;

		lPushCounter = 0;
		lPopCounter = 0;

		wprintf(L"------------------------------------------------\n");
		wprintf(L"Push TPS : %d\n", lPushTPS);
		wprintf(L"Pop TPS : %d\n", lPopTPS);
		wprintf(L"------------------------------------------------\n\n");

		Sleep(999);
	}
}

/*------------------------------------------------------------------*/
// 0. �� �����忡�� st_QUEUE_DATA �����͸� ���� ��ġ (10000��) ����		
// 0. ������ ����(Ȯ��)
// 1. iData = 0x0000000055555555 ����
// 1. lCount = 0 ����
// 2. ���ÿ� ����

// 3. �ణ���  Sleep (0 ~ 3)
// 4. ���� ���� ������ �� ��ŭ ���� 
// 4. - �̶� �����°� ���� ���� �������� ����, �ٸ� �����尡 ���� �������� ���� ����
// 5. ���� ��ü �����Ͱ� �ʱⰪ�� �´��� Ȯ��. (�����͸� ���� ����ϴ��� Ȯ��)
// 6. ���� ��ü �����Ϳ� ���� lCount Interlock + 1
// 6. ���� ��ü �����Ϳ� ���� iData Interlock + 1
// 7. �ణ���
// 8. + 1 �� �����Ͱ� ��ȿ���� Ȯ�� (���� �����͸� ���� ����ϴ��� Ȯ��)
// 9. ������ �ʱ�ȭ (0x0000000055555555, 0)
// 10. ���� �� ��ŭ ���ÿ� �ٽ� ����
//  3�� ���� �ݺ�.
/*------------------------------------------------------------------*/
unsigned __stdcall StackThread(void *pParam)
{
	int iCnt;
	st_TEST_DATA *pData;
	st_TEST_DATA *pDataArray[dfTHREAD_ALLOC];

	// 0. ������ ����(Ȯ��)
	// 1. iData = 0x0000000055555555 ����
	// 1. lCount = 0 ����
	for (iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
	{
		pDataArray[iCnt] = new st_TEST_DATA;
		pDataArray[iCnt]->lData = 0x0000000055555555;
		pDataArray[iCnt]->lCount = 0;
	}

	// 2. ���ÿ� ����
	for (iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
	{
		g_Stack.Push(pDataArray[iCnt]);
		InterlockedIncrement64(&lPushCounter);
	}

	while (1){
		// 3. �ణ���  Sleep (0 ~ 3)
		Sleep(1);

		// 4. ���� ���� ������ �� ��ŭ ���� 
		// 4. - �̶� �����°� ���� ���� �������� ����, �ٸ� �����尡 ���� �������� ���� ����
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			g_Stack.Pop(&pData);
			InterlockedIncrement64(&lPopCounter);

			pDataArray[iCnt] = pData;
		}

		// 5. ���� ��ü �����Ͱ� �ʱⰪ�� �´��� Ȯ��. (�����͸� ���� ����ϴ��� Ȯ��)
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			if ((pDataArray[iCnt]->lData != 0x0000000055555555) || (pDataArray[iCnt]->lCount != 0))
				LOG(L"LockfreeStack", LOG::LEVEL_DEBUG, L"Pop  ] pDataArray[%04d] is using in stack\n", iCnt);

		}

		// 6. ���� ��ü �����Ϳ� ���� lCount Interlock + 1
		// 6. ���� ��ü �����Ϳ� ���� iData Interlock + 1
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			InterlockedIncrement64(&pDataArray[iCnt]->lCount);
			InterlockedIncrement64(&pDataArray[iCnt]->lData);
		}

		// 7. �ణ���
		Sleep(1);

		// 8. + 1 �� �����Ͱ� ��ȿ���� Ȯ�� (���� �����͸� ���� ����ϴ��� Ȯ��)
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			if ((pDataArray[iCnt]->lCount != 1) || (pDataArray[iCnt]->lData != 0x0000000055555556))
				LOG(L"LockfreeStack", LOG::LEVEL_DEBUG, L"Plus ] pDataArray[%04d] is using in another stack\n", iCnt);
		}

		// 9. ������ �ʱ�ȭ (0x0000000055555555, 0)
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			pDataArray[iCnt]->lData = 0x0000000055555555;
			pDataArray[iCnt]->lCount = 0;
		}

		// 10. ���� �� ��ŭ ���ÿ� �ٽ� ����
		for (int iCnt = 0; iCnt < dfTHREAD_ALLOC; iCnt++)
		{
			g_Stack.Push(pDataArray[iCnt]);
			InterlockedIncrement64(&lPushCounter);
		}
	}
}
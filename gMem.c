//====================================================================
//====================================================================
/*****************************�����ڴ����******************************
//����-------  ���ڶ�̬���빫���ڴ�
//����-------  �˹���
//����ʱ��--20130604
//�޸�: ( �˹���) 20131213   --V2(���������ͷŵ��ڴ棬���û��ջ���)
//�޸�: ( �˹���) 20131213   --V3(�����ڴ������������ڣ�
								     �������ͷŵ�Ч�����53.00%(V2-59037,V3-27755))
//�޸�: ( �˹���) 20140612   --V4(���ӽṹ�����ϵ�ָ��,�ͷ��ڴ��ٶȴ��������)
//�޸�: ( �˹���) 20150504   --V5(���ӽṹ�����ϵ�ָ�����,�ͷ��ڴ��ٶȴ��������)
											�ۺ��������ͷŵ�Ч�����36.06%(V3-87243,V5-55786),V4���V3��������)
******************************************************************************/
#include "comm.h"

//-------------------�ڴ������ṹ----------------------------------------
typedef struct _DfFlagGuideRAM
{ //---��pNext�����м��λλ�ñ���Խ�����-----------------------------   
	u16 	uSize; 						//���ε�ַ���ڱ�ǩ,0x0000 ���ͷ�		
	u16 	uLast; 						//��һ��㣬=NULLʱ�ռ��Ѿ��ͷ�
	struct _DfFlagGuideRAM  *pNext;
	char 	MsgData[4];  //�����С,һ���������Ҫ����С�ռ�
} DfFlagGuideRAM;
//-------------------�ڴ����ָ�붨��----------------------------------------
static char* pAppRamGuideHead;		//--��ʼ��ַ---
static char* pAppRamGuideEnd;		//--�յ��ַ---
//-------------------���ٴ�����������----------------------------------------
static DfFlagGuideRAM *pRamQuick;	//���ٴ�����ַ��¼��
//====================================================================
//����:     �˹���	---
//����:    ���ڳ�ʼ���ڴ�ʹ�õ�����
//��������:pGuideHead������������׵�ַ,RamLen,����ʹ�õ��ڴ��С
//�������:��
//����ʱ��:  	20130604
//---------------------------------------------------------------
void InitGuideRAM(void* pGuideHead,unsigned RamLen)
{
//	TRACE("InitGuideRAM Sta[%x],Len[%d]\r\n",pGuideHead,RamLen);
	pAppRamGuideHead=(char*)pGuideHead;
	pAppRamGuideEnd=pAppRamGuideHead+RamLen;
	//-------------���ٴ���------------
	pRamQuick=(DfFlagGuideRAM *)pAppRamGuideHead;
	pRamQuick->pNext=NULL;
	pRamQuick->uSize=0;
	pRamQuick->uLast=0;	//��һ��ָ����
}
//====================================================================
//����:    ��������̶��ڴ湦�ܣ�
//��������:silen ��Ҫ�����ڴ��С
//�������:������������׵�ַ������ʧ��ʱ����Ϊ�յ�ַ
//����:     �˹���	---	
//����ʱ��:  	20140612	-V4
//---------------------------------------------------------------
void *gmalloc(unsigned int silen)
{
	unsigned int iLen;
	char *pRet;
	DfFlagGuideRAM *pGuideRAM,*pNext;//register 
	//------���볤������4������(����ָ��ƫ��)--------------
	iLen=sizeof(char*)-1;
	silen =(silen+iLen)&(~iLen); 
	//-----�������-------
	//-----�����������-------
	while(pRamQuick->uSize)
		pRamQuick=pRamQuick->pNext;
	pGuideRAM=pRamQuick;
	//-------------------------------------------------------
	while(pGuideRAM->pNext)	//pGuideRAM->pNext!=NULL
	{	
		if(!pGuideRAM->uSize)//pGuideRAM->size == 0
		{//--------���մ���------
			iLen=(char*)pGuideRAM->pNext - pGuideRAM->MsgData;	//��������
			if(iLen >= silen)
			{
				pGuideRAM->uSize=1;
				pRet= pGuideRAM->MsgData;
				if((iLen-silen) >= (sizeof(DfFlagGuideRAM)))
				{//---���ڵĿռ䣬��������һ�����ݿռ���--------
					pNext=pGuideRAM->pNext;
					pGuideRAM->pNext=(DfFlagGuideRAM *)&pRet[silen];
					pGuideRAM = pGuideRAM->pNext;
					pGuideRAM->pNext = pNext;
					pGuideRAM->uLast = pNext->uLast;
					pGuideRAM->uSize = 0;
					pNext->uLast=(u16)((char*)pGuideRAM-pAppRamGuideHead);
				}
				return (void*)pRet;
			}
		}
		pGuideRAM = pGuideRAM->pNext;
	}
	if((pGuideRAM->MsgData +silen) >= pAppRamGuideEnd)
	{
//		UI_ShowInfo("AppPE RAM OVER");
//		FreeCheckRAM();
		return NULL;
	}
	//--------New Ram-----
	pRet=pGuideRAM->MsgData;
	pGuideRAM->pNext=(DfFlagGuideRAM *)&pRet[silen];
	pGuideRAM->uSize=1;
	pNext =pGuideRAM->pNext;//---ָ����һ��
	pNext->pNext=NULL;	//��һ���ƿ�
	pNext->uSize=0;//=1;
	pNext->uLast=(u16)((char*)pGuideRAM-pAppRamGuideHead);	//��ֵ��һ������ָ��
	return (void*)pRet;
}
//====================================================================
//����:    �����ͷ�֮ǰ������ڴ�
//��������:pfree������������׵�ַ�������ͷŶ�Ӧ�õ�����
//�������:��
//����:     �˹���	---	
//����ʱ��:  	20140612	-V4
//---------------------------------------------------------------
void gfree(void *pfree)
{
	DfFlagGuideRAM *pGuideRAM,*pLast;//register 
	//---------�������ָ�����Ч��---------------------------------------
	if((char*)pfree <pAppRamGuideHead || (char*)pfree >pAppRamGuideEnd) return;
	pGuideRAM=(DfFlagGuideRAM *)(((char*)pfree)-(sizeof(DfFlagGuideRAM)-sizeof(pGuideRAM->MsgData)));
	//---------���ָ�������ȷ�ԣ�����������---------------------------------------
	//if(pGuideRAM->uLast > (u16)(pAppRamGuideEnd-pAppRamGuideHead)) return;
	if((char*)pGuideRAM->pNext<pAppRamGuideHead||(char*)pGuideRAM->pNext>pAppRamGuideEnd) return;
	//--(pGuideRAM->pNext->pPrev==NULL||pGuideRAM->pNext->pNext==NULL)------------
	pGuideRAM->uSize=0x00;	//��ָ�����ձ��

	if(!pGuideRAM->pNext->uSize)
	{//------ɾ������(���)------------
		pGuideRAM->pNext=pGuideRAM->pNext->pNext;
		if(pGuideRAM->pNext) 
			pGuideRAM->pNext->uLast=(u16)((char*)pGuideRAM-pAppRamGuideHead);
	}
	pLast=(DfFlagGuideRAM *)(pAppRamGuideHead+pGuideRAM->uLast);
	if(!pLast->uSize)//(pGuideRAM->pLast->pLast==NULL)
	{//------ɾ������(��ǰ)------------
		pLast->pNext=pGuideRAM->pNext;
		if(pLast->pNext) 
			pLast->pNext->uLast=(u16)((char*)pLast-pAppRamGuideHead);
		pGuideRAM=pLast;
	}
	if(pRamQuick > pGuideRAM) pRamQuick= pGuideRAM;	//�µĿ�����ڣ���һ������ʹ����￪ʼ	
//	TRACE("find OVER_[%x]\r\n",pfree);
//	UI_ShowInfo("find OVER");
}
//====================================================================
//����:    ������ʾ����ʹ�þ���(��Ҫ���ڵ���)
//��������:��
//�������:��
//����:     �˹���	---	
//����ʱ��:  	20130604
//---------------------------------------------------------------
extern void API_Trace(   u32 tLevel,char* pFmt,...);	

void gMemCheck(void)
{
	u16 MaxNum=0,ShowLen=0;
	char Showmsg[512];
	DfFlagGuideRAM *pGuideRAM;
	pGuideRAM=(DfFlagGuideRAM *)pAppRamGuideHead;
	Showmsg[ShowLen++]='[';
	while(pGuideRAM->pNext!=NULL)
	{
		MaxNum++;
		if(pGuideRAM->uSize==0)
			Showmsg[ShowLen++]='-';
		else Showmsg[ShowLen++]='8';
		pGuideRAM = pGuideRAM->pNext;
	}
	Showmsg[ShowLen++]=']';
	Showmsg[ShowLen++]=0x00;
	API_Trace(DBG_APP_INFO,"%s-MaxAdder[%d]-MaxNum[%d]",Showmsg,(char*)pGuideRAM-pAppRamGuideHead,MaxNum);
}



//====================================================================
/*****************************�����ڴ����******************************
//����-------  ���ڶ�̬���빫���ڴ�
//����-------  �˹���
//����ʱ��--20130604
//�޸�: ( �˹���) 20131213   --V2(���������ͷŵ��ڴ棬���û��ջ���)
//�޸�: ( �˹���) 20131213   --V3(�����ڴ������������ڣ�
								     �������ͷŵ�Ч�����53.00%(V2-59037,V3-27755))
******************************************************************************/
//-------------------�ڴ�����ṹ-(ע���ڴ�Գ�)---------------------------------------
typedef struct _DfFlagGuideRAM
{ //---��pNext�����м��λλ�ñ���Խ�����-----------------------------   
//	u16 	Flag; 	//��ʱ���ã�
	unsigned long size; 							//b������4���ֽ�
	struct _DfFlagGuideRAM  *pNext;
	char 	MsgData[4];  //�����С
} DfFlagGuideRAM;
//-------------------�ڴ����ָ�붨��----------------------------------------
static char* pAppRamGuideHead;		//--��ʼ��ַ---
static char* pAppRamGuideEnd;		//--�յ��ַ---
//-------------------���ٴ����������----------------------------------------
static DfFlagGuideRAM *pRamQuick;	//���ٴ����ַ��¼��
//====================================================================
//����:     �˹���	---
//����:    ���ڳ�ʼ���ڴ�ʹ�õ�����
//��������:pGuideHead������������׵�ַ(������4����������ַ),RamLen,����ʹ�õ��ڴ��С
//�������:��
//����ʱ��:  	20130604
//---------------------------------------------------------------
void InitGuideRAM(void* pGuideHead,unsigned RamLen)
{
//	TRACE("InitGuideRAM Sta[%x],Len[%d]\r\n",pGuideHead,RamLen);
	pAppRamGuideHead=(char*)pGuideHead;
	pAppRamGuideEnd=(pAppRamGuideHead+RamLen);
	//-------------���ٴ���------------
	pRamQuick=(DfFlagGuideRAM *)pAppRamGuideHead;
	pRamQuick->pNext=NULL;
	pRamQuick->size =0;	
}
//====================================================================
//����:    ��������̶��ڴ湦�ܣ�
//��������:silen ��Ҫ�����ڴ��С
//�������:������������׵�ַ������ʧ��ʱ����Ϊ�յ�ַ
//����:     �˹���	---	
//����ʱ��:  	20130604
//---------------------------------------------------------------
void *gmalloc(unsigned silen)
{
	unsigned iLen;
	char *pRet;
	DfFlagGuideRAM *pGuideRAM,*pNext;
	//------���볤������4������(����ָ��ƫ��)--------------
//	if(!silen) return NULL;	//��������ݳ��Ȳ���Ϊ0
	iLen=sizeof(char*)-1;
	silen =(silen+iLen)&(~iLen); 
	//-----�����������-------
	while(pRamQuick->size)
		pRamQuick=pRamQuick->pNext;
	
	pGuideRAM=pRamQuick;
	//-------------------------------------------------------
	while(pGuideRAM->pNext)	//pGuideRAM->pNext!=NULL
	{
		if(!pGuideRAM->size)//pGuideRAM->size == 0
		{//--------���մ���------
			iLen=(char*)pGuideRAM->pNext - pGuideRAM->MsgData;	//��������
			if(iLen >= silen)
			{
				pGuideRAM->size = silen;
				pRet= pGuideRAM->MsgData;
				if((iLen-silen) > (sizeof(DfFlagGuideRAM)-sizeof(pGuideRAM->MsgData)))
				{//---���ڵĿռ䣬��������һ�����ݿռ���--------
					pNext=pGuideRAM->pNext;
					pGuideRAM->pNext=(DfFlagGuideRAM *)(pGuideRAM->MsgData+silen);
					pGuideRAM = pGuideRAM->pNext;
					pGuideRAM->pNext = pNext;
					pGuideRAM->size =0;
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
	pGuideRAM->size = silen;
	pGuideRAM->pNext=(DfFlagGuideRAM *)&pRet[silen];
	pGuideRAM =pGuideRAM->pNext;//---ָ����һ��
	pGuideRAM->pNext=NULL;	//��һ���ƿ�	
	pGuideRAM->size = 0;
	return (void*)pRet;
}
//====================================================================
//����:    �����ͷ�֮ǰ������ڴ�
//��������:pfree������������׵�ַ�������ͷŶ�Ӧ�õ�����
//�������:��
//����:     �˹���	---	
//����ʱ��:  	20130604	-V1
//---------------------------------------------------------------
void gfree(void *pfree)
{
	DfFlagGuideRAM *pGuideRAM,*pRamPrev;
	if(pfree == NULL) return;			
	pGuideRAM=(DfFlagGuideRAM *)pAppRamGuideHead;
	pRamPrev = NULL;
	while(pGuideRAM->pNext)//pGuideRAM->pNext!=NULL
	{
		if(pGuideRAM->MsgData ==(char*)pfree)
		{
			pGuideRAM->size=0;	//------������ݱ��------------
			if(pGuideRAM->pNext->size==0)
			{//------ɾ������(���)------------
				pGuideRAM->pNext=pGuideRAM->pNext->pNext;
			}
			if((pRamPrev != NULL) &&(pRamPrev->size ==0))
			{//------ɾ������(��ǰ)------------
				pRamPrev->pNext = pGuideRAM->pNext;
				pGuideRAM=pRamPrev;
			}
			//-----������һ������������--------------
			if(pRamQuick>pGuideRAM)
				pRamQuick=pGuideRAM;
			return;
		}
		pRamPrev = pGuideRAM;
		pGuideRAM = pGuideRAM->pNext;
	}
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
int FreeCheckRAM(char *pStrout,int *fNum)
{
	int MaxNum=0,flag=0,gErr=0;
//	char OutStr[20];
	char *pEndRam;
	DfFlagGuideRAM *pGuideRAM;
	pGuideRAM=(DfFlagGuideRAM *)pAppRamGuideHead;
	pEndRam=pAppRamGuideHead;
	*fNum=0;
	*pStrout++ = 'V';
	*pStrout++ = '3';
	*pStrout++ = ':';
	*pStrout++ ='[';
	while(pGuideRAM->pNext)
	{
		MaxNum++;
		if(pGuideRAM->size)
		{
			*pStrout++ ='8';
			pEndRam=(char*)pGuideRAM;
			(*fNum)++;
			flag=0;
		}
		else 
		{
			*pStrout++ ='_';
			if(flag) gErr++;
			else flag=1;
		}
		pGuideRAM = pGuideRAM->pNext;
	}
	*pStrout++ =']';
	API_sprintf(pStrout,"---[%d]>MaxNum[%d],EndRAM[%d],Err[%d]\r\n",fNum,MaxNum,pEndRam-pAppRamGuideHead,gErr);
	return gErr;
}



//====================================================================
/*****************************�����ڴ����******************************
//����-------  ���ڶ�̬���빫���ڴ�
//����-------  �˹���
//����ʱ��--20130604
//�޸�: ( �˹���) 20131213   --V2(���������ͷŵ��ڴ棬���û��ջ���)
//�޸�: ( �˹���) 20131213   --V3(�����ڴ������������ڣ�
								     �������ͷŵ�Ч�����53.00%(V2-59037,V3-27755))
//�޸�: ( �˹���) 20140612   --V4(���ӽṹ�����ϵ�ָ��,�ͷ��ڴ��ٶȴ��������)
******************************************************************************/

//-------------------�ڴ������ṹ----------------------------------------
typedef struct _DfFlagGuideRAM
{ //---��pNext�����м��λλ�ñ���Խ�����-----------------------------   
//	u16 	Flag; 	//��ʱ���ã�
	struct _DfFlagGuideRAM  *pPrev;	//��һ��㣬=NULLʱ�ռ��Ѿ��ͷ�
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
//��������:pGuideHead������������׵�ַ(������4����������ַ),RamLen,����ʹ�õ��ڴ��С
//�������:��
//����ʱ��:  	20130604
//---------------------------------------------------------------
void InitGuideRAM(char* pGuideHead,unsigned RamLen)
{
//	TRACE("InitGuideRAM Sta[%x],Len[%d]\r\n",pGuideHead,RamLen);
	pAppRamGuideHead=pGuideHead;//(pGuideHead+3)&(~3);
	pAppRamGuideEnd=(pGuideHead+RamLen);
	//-------------���ٴ���------------
	pRamQuick=(DfFlagGuideRAM *)pAppRamGuideHead;
	pRamQuick->pNext=NULL;
	pRamQuick->pPrev=pRamQuick;	//��һ��ָ����
}
//====================================================================
//����:    ��������̶��ڴ湦�ܣ�
//��������:silen ��Ҫ�����ڴ��С
//�������:������������׵�ַ������ʧ��ʱ����Ϊ�յ�ַ
//����:     �˹���	---	
//����ʱ��:  	20140612	-V4
//---------------------------------------------------------------
void *ApplyGuideRAM(unsigned silen)
{
	unsigned iLen;
	char *pRet;
	DfFlagGuideRAM *pGuideRAM,*pLink;
	//------���볤������4������(����ָ��ƫ��)--------------
//	if(!silen) return NULL;	//��������ݳ��Ȳ���Ϊ0
	iLen=sizeof(char*)-1;
	silen =(silen+iLen)&(~iLen); 
	//-----�������-------	
	pGuideRAM=pRamQuick;
	//--��պø���ֵ-------
	pRamQuick=NULL;	
	//-------------------------------------------------------
	while(pGuideRAM->pNext)	//pGuideRAM->pNext!=NULL
	{		
		if(!pGuideRAM->pPrev)//pGuideRAM->size == 0
		{//--------���մ���------
			iLen=(char*)pGuideRAM->pNext - pGuideRAM->MsgData;	//��������
			if(iLen >= silen)
			{
				//------��������������һ���һ��ָ��---
				pLink=(DfFlagGuideRAM *)pAppRamGuideHead;
				if(pLink!=pGuideRAM)
				{
					while(pLink->pNext!=pGuideRAM)
						pLink=pLink->pNext;
				}
				//--------------------------------------------------
				pGuideRAM->pPrev = pLink;	//�ָ�����ָ�룬��ַ��������
				pRet= pGuideRAM->MsgData;	//���뵽�ĵ�ַָ��
				if((iLen-silen) >= sizeof(DfFlagGuideRAM))
				{//---���ڵĿռ䣬��������һ�����ݿռ���--------
					pLink=pGuideRAM->pNext;	//������ָ�룬�Ա㸳ֵ����һ������ָ
					pGuideRAM->pNext=(DfFlagGuideRAM *)(pGuideRAM->MsgData+silen);	//���¸�ֵ��ָ�룬�Ա����һ���µĽ��
					//-----�½�㸳ֵ------
					//pGuideRAM->pNext->pPrev = NULL; //��ֵ��һ������ָ��
					//pGuideRAM->pNext->pNext = pLink;//��ֵ����һ������ָ,������ָ��
					//pLink->pPrev = pGuideRAM->pNext; //��ֵ��һ������ָ��
					pGuideRAM = pGuideRAM->pNext;
					pGuideRAM->pNext = pLink;
					pGuideRAM->pPrev = NULL;
					pLink->pPrev = pGuideRAM; 
					if(!pRamQuick) 
						pRamQuick= pGuideRAM;	//�µĿ�����ڣ���һ������ʹ����￪ʼ
				}
				else 
				{
					if(!pRamQuick) 
						pRamQuick= pGuideRAM->pNext;	//�µĿ�����ڣ���һ������ʹ����￪ʼ
				}
				return (void*)pRet;
			}
			if(!pRamQuick) pRamQuick= pGuideRAM;	//�µĿ�����ڣ���һ������ʹ����￪ʼ				
		}
		pGuideRAM = pGuideRAM->pNext;
	}
	if((pGuideRAM->MsgData +silen) >= pAppRamGuideEnd)
	{
//		UI_ShowInfo("AppPE RAM OVER");
//		FreeCheckRAM();
		if(!pRamQuick) pRamQuick= pGuideRAM;	//�µĿ�����ڣ���һ������ʹ����￪ʼ				
		return NULL;
	}
	//--------New Ram-----
	pRet=pGuideRAM->MsgData;
	pGuideRAM->pNext=(DfFlagGuideRAM *)&pRet[silen];
	pGuideRAM->pNext->pPrev=pGuideRAM;	//��ֵ��һ������ָ��
	pGuideRAM =pGuideRAM->pNext;//---ָ����һ��
	pGuideRAM->pNext=NULL;	//��һ���ƿ�
	if(!pRamQuick) pRamQuick= pGuideRAM;	//�µĿ�����ڣ���һ������ʹ����￪ʼ				
	return (void*)pRet;
}
//====================================================================
//����:    �����ͷ�֮ǰ������ڴ�
//��������:pfree������������׵�ַ�������ͷŶ�Ӧ�õ�����
//�������:��
//����:     �˹���	---	
//����ʱ��:  	20140612	-V4
//---------------------------------------------------------------
void FreeGuideRAM(void *pfree)
{
	DfFlagGuideRAM *pGuideRAM,*pLink;
	//---------�������ָ�����Ч����---------------------------------------
	if((char*)pfree <pAppRamGuideHead || (char*)pfree >pAppRamGuideEnd) return;
	pGuideRAM=(DfFlagGuideRAM *)(((char*)pfree)-(sizeof(pGuideRAM->pNext)+sizeof(pGuideRAM->pPrev)));
	//---------���ָ�������ȷ�ԣ�����������---------------------------------------
	if(pGuideRAM->pNext<(DfFlagGuideRAM *)pAppRamGuideHead||pGuideRAM->pNext>(DfFlagGuideRAM *)pAppRamGuideEnd) return;
	if(pGuideRAM->pPrev<(DfFlagGuideRAM *)pAppRamGuideHead||pGuideRAM->pPrev> pGuideRAM->pNext) return;
	//--(pGuideRAM->pNext->pPrev==NULL||pGuideRAM->pNext->pNext==NULL)------------
	if((!pGuideRAM->pNext->pPrev)||(!pGuideRAM->pNext->pNext))
	{//------ɾ������(���)------------
		pGuideRAM->pNext=pGuideRAM->pNext->pNext;
		if(pGuideRAM->pNext) pGuideRAM->pNext->pPrev=pGuideRAM;
	}
	if(!pGuideRAM->pPrev->pPrev)//(pGuideRAM->pPrev->pPrev==NULL)
	{//------ɾ������(��ǰ)------------
		pGuideRAM=pGuideRAM->pPrev;
		pGuideRAM->pNext=pGuideRAM->pNext->pNext;

		if(pGuideRAM->pNext) pGuideRAM->pNext->pPrev=pGuideRAM;
		else 
		{
			//------��������������һ���һ��ָ��---
			pLink=(DfFlagGuideRAM *)pAppRamGuideHead;
			if(pLink!=pGuideRAM)
			{
				while(pLink->pNext!=pGuideRAM)
					pLink=pLink->pNext;
			}
			pGuideRAM->pPrev=pLink;
		}
	}
	
	if(pGuideRAM->pNext) pGuideRAM->pPrev=NULL;	//�������һ����Ҫɾ����ָ��
	
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
/*
u16 FreeCheckRAM(char *pStrout)
{
	u16 MaxNum=0,fNum=0;
//	char OutStr[20];
	DfFlagGuideRAM *pGuideRAM;
	pGuideRAM=(DfFlagGuideRAM *)pAppRamGuideHead;
	*pStrout++ ='[';
	while(pGuideRAM->pNext!=NULL)
	{
		MaxNum++;
		if(pGuideRAM->pPrev==NULL)
			*pStrout++ ='_';
		else {*pStrout++ ='8'; fNum++;}
		pGuideRAM = pGuideRAM->pNext;
	}
	*pStrout++ =']';
	sprintf(pStrout,"---[%d]>MaxNum[%d],EndRAM[%d]\r\n",fNum,MaxNum,(char*)pGuideRAM-pAppRamGuideHead);
	return fNum;
}
*/

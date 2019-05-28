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
//�޸�: ( �˹���) 20161108   --V6(����Ԥ���ڴ�ռ�������ڴ�ռ�Ԥ����������)
******************************************************************************/
#ifndef KTYPE
typedef unsigned short  		u16;
#endif
#ifndef NULL
#define	NULL				(0)
#endif

//==========�����ڴ����ͱ��==============
#define	NULL_MEMORY					(0)
#define	NORMAL_MEMORY				(1)
#define	PREAUTH_MEMORY				(2)

//==============�ⲿ����======================
extern int APP_ShowMsg(char* pTitle,char* pMsg,int tTimeOutMS);
extern int 	API_sprintf(char* str, const char* format, ...);
static void UI_ShowMemMsg(char* pTitle,char* pMsg,int Mpar)
{
	char showErr[24];
	API_sprintf(showErr,pMsg,Mpar);
	APP_ShowMsg(pTitle,showErr,10000);
}

//-------------------�ڴ������ṹ----------------------------------------
typedef struct _DfFlagGuideRAM
{ //---��pNext�����м��λλ�ñ���Խ�����-----------------------------   
	unsigned 	uSize:8; 						//���ε�ַ���ڱ�ǩ,0x0000 ���ͷ�		
	unsigned	uLast:24;						//��һ��㣬=NULLʱ�ռ��Ѿ��ͷ�
	struct _DfFlagGuideRAM  *pNext;
	char 	MsgData[0];  //�����С,һ���������Ҫ����С�ռ�
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
	pAppRamGuideHead=(char*)pGuideHead;//(pGuideHead+3)&(~3);
	pAppRamGuideEnd=pAppRamGuideHead+RamLen;
	//-------------���ٴ���------------
	pRamQuick=(DfFlagGuideRAM *)pAppRamGuideHead;
	pRamQuick->pNext=(DfFlagGuideRAM *)pAppRamGuideEnd;
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
void *gMalloc(unsigned silen)
{
	unsigned int iLen;
	DfFlagGuideRAM *pGuideRAM;//register 
	//------���볤������4������(����ָ��ƫ��)--------------
	iLen=sizeof(char*)-1;
	silen =(silen+iLen)&(~iLen); 
	//-----�������-------
	//-----�����������-------
	while(pRamQuick->uSize!=NULL_MEMORY)
		pRamQuick=pRamQuick->pNext;
	pGuideRAM=pRamQuick;
	//-------------------------------------------------------
	while((char*)pGuideRAM != pAppRamGuideEnd)	//pGuideRAM->pNext!=NULL
	{	
		if(pGuideRAM->uSize==NULL_MEMORY)//pGuideRAM->size == 0
		{//--------���մ���------
			iLen=(char*)pGuideRAM->pNext - pGuideRAM->MsgData;	//��������
			if(iLen >= silen)
			{
				pGuideRAM->uSize=NORMAL_MEMORY;
				if(iLen > (silen+sizeof(DfFlagGuideRAM)))
				{//---���ڵĿռ䣬��������һ�����ݿռ���--------
					DfFlagGuideRAM *pNext;
					pNext=pGuideRAM->pNext;
					pGuideRAM->pNext=(DfFlagGuideRAM *)(pGuideRAM->MsgData+silen);
					if(((char*)pNext) != pAppRamGuideEnd)
					{
						pNext->uLast=((char*)pNext- (char*)pGuideRAM->pNext);
					}
					pGuideRAM->pNext->uLast =((char*)pGuideRAM->pNext - (char*)pGuideRAM);
					pGuideRAM->pNext->pNext = pNext;
					pGuideRAM->pNext->uSize = NULL_MEMORY;
				}
				return (void*)pGuideRAM->MsgData;
			}
		}
		pGuideRAM = pGuideRAM->pNext;
	}
	UI_ShowMemMsg((char*)"�ڴ�����",(char*)"����%d�ռ䲻��",silen);
//	FreeCheckRAM();
	return NULL;
}
//====================================================================
//����:    �����ͷ�֮ǰ������ڴ�
//��������:pfree������������׵�ַ�������ͷŶ�Ӧ�õ�����
//�������:��
//����:     �˹���	---	
//����ʱ��:  	20140612	-V4
//---------------------------------------------------------------
void gFree(void *pfree)
{
	DfFlagGuideRAM *pGuideRAM;//,*pLast;//register 
	//---------�������ָ�����Ч��---------------------------------------
	if((char*)pfree <pAppRamGuideHead || (char*)pfree >pAppRamGuideEnd) return;
	pGuideRAM=(DfFlagGuideRAM *)(((char*)pfree)-(sizeof(DfFlagGuideRAM)));
	//---------���ָ�������ȷ�ԣ�����������---------------------------------------
	//if(pGuideRAM->uLast > (u16)(pAppRamGuideEnd-pAppRamGuideHead)) return;
	if((char*)pGuideRAM->pNext<pAppRamGuideHead||(char*)pGuideRAM->pNext>pAppRamGuideEnd) return;
	//--(pGuideRAM->pNext->pPrev==NULL||pGuideRAM->pNext->pNext==NULL)------------
	if(pGuideRAM->uSize==NULL_MEMORY)
	{
		UI_ShowMemMsg((char*)"�ڴ��ͷ�",(char*)"�ظ��ͷ�%xͬһ�ռ�",(int)pfree);
		return;
	}
	//------�ͷŽڵ�ռ�---------------------------
	{
		pGuideRAM->uSize=NULL_MEMORY;	//��ָ�����ձ��
		if((char*)pGuideRAM->pNext != pAppRamGuideEnd)
		{
			if(pGuideRAM->pNext->uSize==NULL_MEMORY)
			{//------ɾ������(���)------------
				pGuideRAM->pNext=pGuideRAM->pNext->pNext;
				if((char*)pGuideRAM->pNext != pAppRamGuideEnd) 
					pGuideRAM->pNext->uLast=((char*)pGuideRAM->pNext - (char*)pGuideRAM);
			}
		}
		//if(pGuideRAM->uLast)
		{
			DfFlagGuideRAM *pLast;
			pLast=(DfFlagGuideRAM *)((char*)pGuideRAM - pGuideRAM->uLast);
			if(pLast->uSize==NULL_MEMORY)//(pGuideRAM->pLast->pLast==NULL)
			{//------ɾ������(��ǰ)------------
				pLast->pNext=pGuideRAM->pNext;
				if((char*)pLast->pNext != pAppRamGuideEnd) 
					pLast->pNext->uLast=((char*)pLast->pNext - (char*)pLast);
				pGuideRAM=pLast;
			}
		}
		if(pRamQuick > pGuideRAM) pRamQuick= pGuideRAM;	//�µĿ�����ڣ���һ������ʹ����￪ʼ	
	}
//	TRACE("find OVER_[%x]\r\n",pfree);
//	UI_ShowInfo("find OVER");
}
//====================================================================
//����:    ����Ԥ��һ�������ڴ�ռ䣬
//��������:sPreLen ��ҪԤ���ڴ��С(0Ϊϵͳ���ռ�)
//�������:������������׵�ַ������ʧ��ʱ����Ϊ�յ�ַ��sPreLenԤ�ڿռ����ֵ
//����:     �˹���	---	
//����ʱ��:  	20161108	-V6
//---------------------------------------------------------------
void *gPralloc(unsigned sPreLen,unsigned *OutSize)
{
	DfFlagGuideRAM *pGuideRAM;//register 
	//-----�����������-------
	while(pRamQuick->uSize)
		pRamQuick=pRamQuick->pNext;
	pGuideRAM=pRamQuick;
	//-------------------------------------------------------
	if(sPreLen)
	{
		void *pRet;
		unsigned Len = sizeof(char*)-1;
		sPreLen =(sPreLen+Len)&(~Len); 
		while((char*)pGuideRAM != pAppRamGuideEnd)	
		{	
			if(pGuideRAM->uSize==NULL_MEMORY)
			{
				Len=((char*)pGuideRAM->pNext - pGuideRAM->MsgData);
				if(Len >= sPreLen)
				{
					pGuideRAM->uSize=PREAUTH_MEMORY;
					pRet=(void*)pGuideRAM->MsgData;
					*OutSize = sPreLen;
					//-------------���沿�ֿ��п���-------------------------------
					if(Len > (sPreLen+sizeof(DfFlagGuideRAM)))
					{//---���ڵĿռ䣬��������һ�����ݿռ���--------
						DfFlagGuideRAM *pNext;
						pNext=pGuideRAM->pNext;
						pGuideRAM->pNext=(DfFlagGuideRAM *)(pGuideRAM->MsgData+sPreLen);
						pGuideRAM=pGuideRAM->pNext;
						pGuideRAM->uLast =(sPreLen+sizeof(DfFlagGuideRAM));
						pGuideRAM->pNext = pNext;
						pGuideRAM->uSize = NULL_MEMORY;
						if(((char*)pNext) != pAppRamGuideEnd)
						{
							pNext->uLast=((char*)pNext- (char*)pGuideRAM);
						}
					}
					return pRet;
				}
			}
			pGuideRAM = pGuideRAM->pNext;
		}
	}
	else
	{
		while((char*)pGuideRAM->pNext != pAppRamGuideEnd)	
		{	
			pGuideRAM = pGuideRAM->pNext;
		}
		if(pGuideRAM->uSize==NULL_MEMORY)
		{
			pGuideRAM->uSize=PREAUTH_MEMORY;
			*OutSize=(char*)pGuideRAM->pNext -pGuideRAM->MsgData;
			return (void*)pGuideRAM->MsgData;
		}
	}
	UI_ShowMemMsg((char*)"�ڴ�����",(char*)"Ԥ��%d�ռ䲻��",sPreLen);
//	FreeCheckRAM();
	return NULL;
}
//====================================================================
//����:    ��������ڴ�ռ��Ԥ�ڣ�
//��������:pMemory��Ԥ�ڵ��ڴ��ַ   silen ��ҪԤ���ڴ��С(0�ͷ�Ԥ��)
//�������:���ؽ��(0��ʾ�ɹ�)
//����:     �˹���	---	
//����ʱ��:  	20161108	-V4
//---------------------------------------------------------------
void* gRealloc(void *pMemory,unsigned silen)
{
	DfFlagGuideRAM *pGuideRAM;//register 
	//---------�������ָ�����Ч��---------------------------------------
	if((char*)pMemory <pAppRamGuideHead || (char*)pMemory >pAppRamGuideEnd) return 1;
	pGuideRAM=(DfFlagGuideRAM *)(((char*)pMemory)-(sizeof(DfFlagGuideRAM)));
	//---------���ָ�������ȷ�ԣ�����������---------------------------------------
	if((char*)pGuideRAM->pNext<pAppRamGuideHead||(char*)pGuideRAM->pNext>pAppRamGuideEnd) return 2;
	//--(pGuideRAM->pNext->pPrev==NULL||pGuideRAM->pNext->pNext==NULL)------------
	if(pGuideRAM->uSize==PREAUTH_MEMORY)
	{
		if(silen)
		{
			unsigned Len;
			Len=sizeof(char*)-1;
			silen =(silen+Len)&(~Len); 
			Len=(char*)pGuideRAM->pNext - pGuideRAM->MsgData;	//��������
			if(Len < silen)
			{
				pGuideRAM->uSize=NULL_MEMORY;
				return 3;
			}
			pGuideRAM->uSize=NORMAL_MEMORY;
			if(Len > (silen+sizeof(DfFlagGuideRAM)))
			{//---���ڵĿռ䣬��������һ�����ݿռ���--------
				DfFlagGuideRAM *pNext;
				pNext=pGuideRAM->pNext;
				pGuideRAM->pNext=(DfFlagGuideRAM *)(pGuideRAM->MsgData+silen);
				pGuideRAM=pGuideRAM->pNext;
				pGuideRAM->uLast =(silen+sizeof(DfFlagGuideRAM));
				pGuideRAM->pNext = pNext;
				pGuideRAM->uSize = NULL_MEMORY;
				if(((char*)pNext) != pAppRamGuideEnd)
				{
					if(pNext->uSize==NULL_MEMORY)
					{//------ɾ������(���)------------
						pGuideRAM->pNext=pNext->pNext;
						if((char*)pGuideRAM->pNext != pAppRamGuideEnd) 
							pGuideRAM->pNext->uLast=((char*)pGuideRAM->pNext - (char*)pGuideRAM);
					}	
					else
					{
						pNext->uLast=((char*)pNext- (char*)pGuideRAM);
					}
				}
			}
		}
		else
		{//-----Ԥ��ȡ��-----�ͷſռ�----
			DfFlagGuideRAM *pLast;
			pGuideRAM->uSize=NULL_MEMORY;	//��ָ�����ձ��
			if((char*)pGuideRAM->pNext != pAppRamGuideEnd)
			{
				if(pGuideRAM->pNext->uSize==NULL_MEMORY)
				{//------ɾ������(���)------------
					pGuideRAM->pNext=pGuideRAM->pNext->pNext;
					if((char*)pGuideRAM->pNext != pAppRamGuideEnd) 
						pGuideRAM->pNext->uLast=((char*)pGuideRAM->pNext - (char*)pGuideRAM);
				}
			}
			//if(pGuideRAM->uLast)
			{
				pLast=(DfFlagGuideRAM *)((char*)pGuideRAM - pGuideRAM->uLast);
				if(pLast->uSize==NULL_MEMORY)//(pGuideRAM->pLast->pLast==NULL)
				{//------ɾ������(��ǰ)------------
					pLast->pNext=pGuideRAM->pNext;
					if((char*)pLast->pNext != pAppRamGuideEnd) 
						pLast->pNext->uLast=((char*)pLast->pNext - (char*)pLast);
					pGuideRAM=pLast;
				}
			}
			if(pRamQuick > pGuideRAM) pRamQuick= pGuideRAM;	//�µĿ�����ڣ���һ������ʹ����￪ʼ	
		}
		return pMemory;
	}
	return NULL;
}

//====================================================================
//����:    ������ʾ����ʹ�þ���(��Ҫ���ڵ���)
//��������:��
//�������:��
//����:     �˹���	---	
//����ʱ��:  	20130604
//---------------------------------------------------------------
int gCheckAllocStatus(char *pStrout,int *fNum)
{
	int MaxNum=0,flag=0,gErr=0;
//	char OutStr[20];
	char *pEndRam;
	DfFlagGuideRAM *pGuideRAM;
	pGuideRAM=(DfFlagGuideRAM *)pAppRamGuideHead;
	pEndRam=pAppRamGuideHead;
	*fNum=0;
	*pStrout++ ='V';
	*pStrout++ ='6';
	*pStrout++ ='=';
	*pStrout++ ='[';
	while((char*)pGuideRAM != pAppRamGuideEnd)
	{
		MaxNum++;
		if(pGuideRAM->uSize==NULL_MEMORY)
		{
			*pStrout++ ='_';
			if(flag) gErr++;
			else flag=1;
		}
		else 
		{
			if(pGuideRAM->uSize==NORMAL_MEMORY)
				*pStrout++ ='8';
			else *pStrout++ ='6';
			pEndRam=(char*)pGuideRAM;
			(*fNum)++;
			flag=0;
		}
		pGuideRAM = pGuideRAM->pNext;
	}
	*pStrout++ =']';
	API_sprintf(pStrout,"---[%d]>MaxNum[%d],EndRAM[%d],Err[%d][%d]\r\n",fNum,MaxNum,pEndRam-pAppRamGuideHead,gErr,sizeof(DfFlagGuideRAM));
	return gErr;
}



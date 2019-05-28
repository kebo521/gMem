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
//�޸�: ( �˹���) 20161108   --V6(����Ԥ���ڴ�ռ�������ڴ�ռ�Ԥ���������ܣ�ʹ��Ч�ʲ��䣬���ʲ���)
//�޸�: ( �˹���) 20161118   --V7(����������й������б�(��С����)����һ�������ڴ�ʹ��Ч�ʣ�
									�ۺ��������ͷŵ�Ч�����29.1%(256:V6-7077,V7-5010),V6���V5��ͬ)
******************************************************************************/
#ifndef NULL
#define	NULL				(0)
#endif
#include "gMem.h"

//-------------------�ڴ�����ṹ----------------------------------------
typedef struct _DfFlagGuideRAM
{ //---��pNext�����м��λλ�ñ���Խ�����----------------------------- 
	struct _DfFlagGuideRAM  *pNfree;			//�����:��һ�����нڵ�(���һ���ڵ�ΪEND,ΪNULL��ʾռ��)
	struct _DfFlagGuideRAM	*pLast;				//�����:��һ���(��һ���ڵ�ΪNULL)
	struct _DfFlagGuideRAM  *pNext;				//�����:��һ���(���һ���ڵ�ΪEND)
	char 	MsgData[0];  //�����С,һ���������Ҫ����С�ռ�
} DfFlagGuideRAM;
//-------------------�ڴ����ָ�붨��----------------------------------------
static DfFlagGuideRAM *pAppRamGuideHead;		//--��ʼ��ַ---
static DfFlagGuideRAM *pAppRamGuideEnd;			//--�յ��ַ---
//-------------------���ٴ����������----------------------------------------
static DfFlagGuideRAM *pRamQuick;	//���ٴ��������е�ַ���
//-------------------������С��Ԫ---------------------------------
static unsigned uAppRamParticleSize;
//----------------�쳣������Ϣ��ʾ-----------
static PFUNC_SHOW_MSG pFunAbnormalMsg=NULL;
void gLoadFunAllocShowMsg(PFUNC_SHOW_MSG pFun)
{
	pFunAbnormalMsg=pFun;
}
#define SHOW_MEM_ALLOC_MSG(pTitle,pMsg,Mpar)		{if(pFunAbnormalMsg) (*pFunAbnormalMsg)(pTitle,pMsg,Mpar);}
//====================================================================
//����:     �˹���	---
//����:    ���½ڵ��������������б�
//��������:pGuideHead���м���Ľڵ�
//�������:��
//����ʱ��:  	20161118
//---------------------------------------------------------------
static void gIncreaseEmptyNode(DfFlagGuideRAM *pGuideRAM)
{
	unsigned RamLen;
	DfFlagGuideRAM *pNext,*pLastfree;
	if(pRamQuick == pAppRamGuideEnd)
	{
		pGuideRAM->pNfree =pRamQuick;
		pRamQuick=pGuideRAM;
		return;
	}
	pNext=pRamQuick;
	pLastfree=NULL;
	RamLen= (char*)pGuideRAM->pNext - pGuideRAM->MsgData;
	//---������С����ռ�ֲ�----
	while(1)
	{
		if(RamLen <= (unsigned)((char*)pNext->pNext - pNext->MsgData))
		{
			pGuideRAM->pNfree = pNext;
			if(pLastfree)
			{
				pLastfree->pNfree=pGuideRAM;
			}
			else
			{
				pRamQuick=pGuideRAM;
			}
			return;
		}
		if(pNext->pNfree == pAppRamGuideEnd)
		{
			pGuideRAM->pNfree =pAppRamGuideEnd;
			pNext->pNfree=pGuideRAM;
			return;
		}
		pLastfree=pNext;
		pNext=pNext->pNfree;
	}	
}
//====================================================================
//����:     �˹���	---
//����:    �������������б��Ƴ��ڵ�
//��������:pGuideHead�����Ƴ��Ľڵ�
//�������:��
//����ʱ��:  	20161118
//---------------------------------------------------------------
static void gDeleteEmptyNode(DfFlagGuideRAM *pGuideRAM)
{
	DfFlagGuideRAM *pNext,*pLastfree;
	pNext=pRamQuick;
	pLastfree=NULL;
	while(pNext != pGuideRAM)
	{
		pLastfree=pNext;
		pNext=pNext->pNfree;
	}	
	if(pLastfree)
	{
		pLastfree->pNfree=pGuideRAM->pNfree;
	}
	else
	{
		pRamQuick=pGuideRAM->pNfree;
	}
	pGuideRAM->pNfree=NULL;
}
//====================================================================
//����:     �˹���	---
//����:    �ͷ������ڵ㣬�������ڿսڵ�
//��������:pGuideRAM�����ͷŵĽڵ�
//�������:��
//����ʱ��:  	20161121
//---------------------------------------------------------------
static void gFreePhysicalNode(DfFlagGuideRAM *pGuideRAM)
{
	DfFlagGuideRAM *pNext=pGuideRAM->pNext;
	//------�ͷŽڵ�ռ�---------------------------
	if(pNext != pAppRamGuideEnd)
	{
		if(pNext->pNfree)
		{//------ɾ������(���)------------
			//------ɾ�����й���-------------
			gDeleteEmptyNode(pNext);
			pNext=pNext->pNext;
			//------ɾ��������------------
			pGuideRAM->pNext= pNext;
			if(pNext != pAppRamGuideEnd) 
				pNext->pLast=pGuideRAM;
		}
	}
	if((char*)pGuideRAM->pLast != NULL)
	{
		DfFlagGuideRAM *pLast=pGuideRAM->pLast;
		if(pLast->pNfree)//(pGuideRAM->pLast->pLast==NULL)
		{//------ɾ������(��ǰ)------------
			//------ɾ�����й���-------------
			gDeleteEmptyNode(pLast);
			//------ɾ��������------------
			pLast->pNext=pNext;
			if(pNext != pAppRamGuideEnd) 
				pNext->pLast=pLast;
			pGuideRAM=pLast;
		}
	}
	gIncreaseEmptyNode(pGuideRAM);
}

//====================================================================
//����:     �˹���	---
//����:    �ָ�ռ�
//��������:pGuideRAM��Ҫ�ָ�Ŀռ�ڵ㣬silen Ҫ�ָ�Ĵ�С
//�������:��
//����ʱ��:  	20170217
//---------------------------------------------------------------
void gMemSplitSpace(DfFlagGuideRAM *pGuideRAM,unsigned silen)
{
	DfFlagGuideRAM *pNext;
	pNext=pGuideRAM->pNext;
	pGuideRAM->pNext=(DfFlagGuideRAM *)(pGuideRAM->MsgData+silen);
	pGuideRAM->pNext->pLast = pGuideRAM;
	pGuideRAM->pNext->pNext = pNext;
	if(pNext != pAppRamGuideEnd)
	{
		if(pNext->pNfree)
		{
			//------ɾ�����й���-------------
			gDeleteEmptyNode(pNext);
			//------ɾ������(���)------------
			pGuideRAM->pNext->pNext =pNext->pNext;
			if(pNext->pNext != pAppRamGuideEnd) 
				pNext->pNext->pLast=pGuideRAM->pNext;
		}	
		else
		{
			pNext->pLast=pGuideRAM->pNext;
		}
	}
	gIncreaseEmptyNode(pGuideRAM->pNext);
}
//====================================================================
//����:     �˹���	---
//����:    ���ڳ�ʼ���ڴ�ʹ�õ�����
//��������:mem_addr������������׵�ַ,mem_size,����ʹ�õ��ڴ��С,particle_size ��С���뵥Ԫ,ȡֵΪ2^(2+n)
//�������:��
//����ʱ��:  	20130604
//---------------------------------------------------------------
int gMemAllocInit(void* mem_addr,unsigned mem_size,unsigned particle_size)
{
	if(((unsigned)mem_addr)&(sizeof(char*)-1)) 
	{
		pAppRamGuideEnd=NULL;
		pRamQuick=NULL;
		SHOW_MEM_ALLOC_MSG("�ڴ�����ʼ��","�ڴ�[%x]����Ҫ��",(int)mem_addr);
		return -1;
	}
	pAppRamGuideHead=(DfFlagGuideRAM *)mem_addr;//(mem_addr+3)&(~3);
	pAppRamGuideEnd=(DfFlagGuideRAM *)((char*)mem_addr+mem_size);
	//-------------���ٴ���------------
	pRamQuick=pAppRamGuideHead;
	pRamQuick->pNext=pAppRamGuideEnd;
	pRamQuick->pNfree=pAppRamGuideEnd;//����������Ŀ���
	pRamQuick->pLast=NULL;	//��һ��ָ���
	//------------��С��Ԫ------------------
	uAppRamParticleSize=4-1;
	{
		unsigned bit3,i,Max;
		for(bit3=0x00000004,Max=0;Max<32;Max++)
			if((bit3<<Max) >= mem_size) break;
		for(bit3=0x00000004,i=0;i<Max;i++)
			if((bit3<<i) >= particle_size) break;
		if(i == Max) return 1;
			particle_size = (bit3<<i);
		if(particle_size >= mem_size) return 2;
	}
	uAppRamParticleSize = particle_size-1;
	return 0;
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
	unsigned iLen;
	DfFlagGuideRAM *pGuideRAM,*pLastfree;//register 
	//------���볤������4������(����ָ��ƫ��)--------------
	silen =(silen+uAppRamParticleSize)&(~uAppRamParticleSize); 
	//-----�������-------
	pGuideRAM=pRamQuick;
	pLastfree=NULL;
	while(pGuideRAM != pAppRamGuideEnd)
	{
		iLen=((char*)pGuideRAM->pNext - pGuideRAM->MsgData);
		if(iLen >= silen)
		{
			//------ɾ�����й���=gDeleteEmptyNode--------
			if(pLastfree) 
				pLastfree->pNfree=pGuideRAM->pNfree;
			else //if(pRamQuick==pGuideRAM) 
				pRamQuick=pGuideRAM->pNfree;
			pGuideRAM->pNfree=NULL;	//����ָ��Ϊ�գ��ǿ��У���ʾռ�á�
			//--------����Ƿ�����ٷָ�--------------
			if(iLen > (silen+sizeof(DfFlagGuideRAM)))
			{//---���ڵĿռ䣬��������һ�����ݿռ���--------
				DfFlagGuideRAM *pNext;
				pNext=pGuideRAM->pNext;
				pGuideRAM->pNext=(DfFlagGuideRAM *)(pGuideRAM->MsgData+silen);
				pGuideRAM->pNext->pLast = pGuideRAM;
				pGuideRAM->pNext->pNext = pNext;
				if(pNext != pAppRamGuideEnd)
				{
					pNext->pLast=pGuideRAM->pNext;
				}
				gIncreaseEmptyNode(pGuideRAM->pNext);
			}
			return (void*)pGuideRAM->MsgData;
		}
		pLastfree=pGuideRAM;
		pGuideRAM=pGuideRAM->pNfree;
	}
	SHOW_MEM_ALLOC_MSG((char*)"�ڴ�����",(char*)"����%d�ռ䲻��",silen);
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
	DfFlagGuideRAM *pGuideRAM,*pNext;//,*pLast;//register 
	//---------�������ָ�����Ч��---------------------------------------
	if((DfFlagGuideRAM *)pfree <pAppRamGuideHead || (DfFlagGuideRAM *)pfree >pAppRamGuideEnd) return;
	pGuideRAM=(DfFlagGuideRAM *)(((char*)pfree)-(sizeof(DfFlagGuideRAM)));
	//---------���ָ�������ȷ�ԣ�����������---------------------------------------
	if(pGuideRAM->pNext<pAppRamGuideHead||pGuideRAM->pNext>pAppRamGuideEnd) return;
	//---------����Ѿ��ͷ�ֱ�ӷ���---------
	if(pGuideRAM->pNfree)
	{
		SHOW_MEM_ALLOC_MSG((char*)"�ڴ��ͷ�",(char*)"�ظ��ͷ�%xͬһ�ռ�",(int)pfree);
		return;
	}
	pNext=pGuideRAM->pNext;
	//------�ͷŽڵ�ռ�---------------------------
	if(pNext != pAppRamGuideEnd)
	{
		if(pNext->pNfree)
		{//------ɾ������(���)------------
			//------ɾ�����й���-------------
			gDeleteEmptyNode(pNext);
			pNext=pNext->pNext;
			//------ɾ��������------------
			pGuideRAM->pNext=pNext;
			if(pNext != pAppRamGuideEnd) 
				pNext->pLast=pGuideRAM;
		}
	}
	if(pGuideRAM->pLast != NULL)
	{
		DfFlagGuideRAM *pLast=pGuideRAM->pLast;
		if(pLast->pNfree)//(pGuideRAM->pLast->pLast==NULL)
		{//------ɾ������(��ǰ)------------
			//------ɾ�����й���-------------
			gDeleteEmptyNode(pLast);
			//------ɾ��������------------
			pLast->pNext=pNext;
			if(pNext != pAppRamGuideEnd) 
				pNext->pLast= pLast;
			pGuideRAM=pLast;
		}
	}
	gIncreaseEmptyNode(pGuideRAM);
//	TRACE("find OVER_[%x]\r\n",pfree);
//	UI_ShowInfo("find OVER");
}
//====================================================================
//����:    ����Ԥ��һ�������ڴ�ռ䣬
//��������:sPreLen ��ҪԤ���ڴ��С(0Ϊϵͳ���ռ�)
//�������:������������׵�ַ������ʧ��ʱ����Ϊ�յ�ַ��OutSizeԤ�ڿռ����ֵ
//����:     �˹���	---	
//����ʱ��:  	20161108	-V6
//---------------------------------------------------------------
void *gPralloc(unsigned sPreLen,unsigned *OutSize)
{
	unsigned iLen;
	DfFlagGuideRAM *pGuideRAM,*pLastfree;//register 
	if(pRamQuick == pAppRamGuideEnd)
	{
		SHOW_MEM_ALLOC_MSG((char*)"�ڴ�Ԥ��",(char*)"�޿��пռ�[%d]",sPreLen);
		return NULL;
	}
	//-----�������-------
	pGuideRAM=pRamQuick;
	pLastfree=NULL;
	if(sPreLen)
	{
		//-���볤������4������(����ָ��ƫ��)-
		sPreLen =(sPreLen+uAppRamParticleSize)&(~uAppRamParticleSize); 
		while(pGuideRAM != pAppRamGuideEnd)
		{
			iLen=((char*)pGuideRAM->pNext - pGuideRAM->MsgData);
			if(iLen >= sPreLen)
			{
				//---ɾ�����й���=gDeleteEmptyNode--------
				if(pLastfree) 
					pLastfree->pNfree=pGuideRAM->pNfree;
				else //if(pRamQuick==pGuideRAM) 
					pRamQuick=pGuideRAM->pNfree;
				pGuideRAM->pNfree=NULL;	//����ָ��Ϊ�գ��ǿ��У���ʾռ�á�
				//--------����Ƿ�����ٷָ�--------------
				if(iLen > (sPreLen+sizeof(DfFlagGuideRAM)))
				{//---���ڵĿռ䣬��������һ�����ݿռ���--------
					DfFlagGuideRAM *pNext;
					pNext=pGuideRAM->pNext;
					pGuideRAM->pNext=(DfFlagGuideRAM *)(pGuideRAM->MsgData+sPreLen);
					pGuideRAM->pNext->pLast = pGuideRAM;
					pGuideRAM->pNext->pNext = pNext;
					if(pNext != pAppRamGuideEnd)
					{
						pNext->pLast=pGuideRAM->pNext;
					}
					gIncreaseEmptyNode(pGuideRAM->pNext);
					iLen = sPreLen;
				}
				*OutSize=iLen;
				return (void*)pGuideRAM->MsgData; //�践�صĿռ��ַ;
			}
			pLastfree=pGuideRAM;
			pGuideRAM=pGuideRAM->pNfree;
		}
	}
	else
	{
		while(pGuideRAM->pNfree != pAppRamGuideEnd)
		{
			pLastfree=pGuideRAM;
			pGuideRAM=pGuideRAM->pNfree;
		}
		//---ɾ�����й���=gDeleteEmptyNode--------
		if(pLastfree) 
			pLastfree->pNfree=pGuideRAM->pNfree;
		else //if(pRamQuick==pGuideRAM) 
			pRamQuick=pGuideRAM->pNfree;
		//----���ÿ���״̬-----------
		pGuideRAM->pNfree=NULL; //����ָ��Ϊ��(�ǿ���)��ʾռ�á�
		*OutSize=((char*)pGuideRAM->pNext - pGuideRAM->MsgData);
		return (void*)pGuideRAM->MsgData;
	}
	SHOW_MEM_ALLOC_MSG((char*)"�ڴ�Ԥ��",(char*)"����%d�ռ䲻��",sPreLen);
	return NULL;
}

//====================================================================
//����:    ��������ڴ�ռ��Ԥ�ڣ�
//��������:pMemory��Ԥ�ڵ��ڴ��ַ   silen ��ҪԤ���ڴ��С(0�ͷ�Ԥ��)
//�������:���ؽ��(0��ʾ�ɹ�)
//����:     �˹���	---	
//����ʱ��:  	20161108	-V6
//---------------------------------------------------------------
void* gRealloc(void *pMemory,unsigned silen)
{
	DfFlagGuideRAM *pGuideRAM;//register 
	//---------�������ָ�����Ч��---------------------------------------
	if((DfFlagGuideRAM *)pMemory < pAppRamGuideHead || (DfFlagGuideRAM *)pMemory >pAppRamGuideEnd)
	{
		SHOW_MEM_ALLOC_MSG((char*)"�ڴ����·���",(char*)"�ռ�[%x]��Ȩ����",(int)pMemory);
		return NULL;
	}
	pGuideRAM=(DfFlagGuideRAM *)(((char*)pMemory)-(sizeof(DfFlagGuideRAM)));
	//---------���ָ�������ȷ�ԣ�����������---------------------------------------
	if(pGuideRAM->pNext<pAppRamGuideHead||pGuideRAM->pNext>pAppRamGuideEnd)
	{
		SHOW_MEM_ALLOC_MSG((char*)"�ڴ����·���",(char*)"�ռ�[%x]������Ч",(int)pMemory);
		return NULL;
	}
	//---------����Ѿ��ͷ�ֱ�ӷ���---------
	if(pGuideRAM->pNfree)
	{
		SHOW_MEM_ALLOC_MSG((char*)"�ڴ����·���",(char*)"�ռ�[%x]�Ѿ��ͷ�",(int)pMemory);
		return NULL;
	}
	
	if(silen)
	{
		unsigned Len;
		silen =(silen+uAppRamParticleSize)&(~uAppRamParticleSize); 
		Len=(char*)pGuideRAM->pNext - pGuideRAM->MsgData;	//��������
		if(Len < silen)
		{//----���пռ䲻��-----
			if(pGuideRAM->pNext != pAppRamGuideEnd)
			{
				if(pGuideRAM->pNext->pNfree)
				{//�����һ���ռ䣬�Ƿ�Ϊ��
					unsigned Len2=(char*)pGuideRAM->pNext->pNext - pGuideRAM->MsgData;
					if(Len2 >= silen)	//�ռ乻
					{
						gDeleteEmptyNode(pGuideRAM->pNext);
						pGuideRAM->pNext=pGuideRAM->pNext->pNext;
						if(pGuideRAM->pNext != pAppRamGuideEnd)
							pGuideRAM->pNext->pLast=pGuideRAM;
						if(Len2 > (silen+sizeof(DfFlagGuideRAM)))
						{
							gMemSplitSpace(pGuideRAM,silen);
						}
						return pMemory; //���ı��ַ
					}
				}
			}
			//-----���пռ�����ܺϲ��ռ��޷�����-----
			{
				unsigned *p1,*p2,Max;
				p1=(unsigned *)gMalloc(silen);
				if (p1)
				{
					p2 = (unsigned *)pGuideRAM->MsgData;
					//-----�ָ�ԭ������----------
					Max = Len / sizeof(unsigned);
					while (Max--) p1[Max] = p2[Max];					
				}	
				//-----Ԥ��ȡ��-----�ͷſռ�----
				gFreePhysicalNode(pGuideRAM);
				return p1; //�ı��ַ��������������ڷ���
			}
		}
		else 
		{
			if(Len > (silen+sizeof(DfFlagGuideRAM)))
			{//---���ڵĿռ䣬��������һ�����ݿռ���--------
				gMemSplitSpace(pGuideRAM,silen);
			}
			return pMemory; //���ı��ַ
		}
	}
	//-----Ԥ��ȡ��-----�ͷſռ�----
	gFreePhysicalNode(pGuideRAM);
	return NULL;
}

  


//====================================================================
//����:    ������ʾ����ʹ�����(��Ҫ���ڵ���)
//��������:��
//�������:��
//����:     �˹���	---	
//����ʱ��:  	20130604
//---------------------------------------------------------------
int gCheckAllocStatus(char *pStrout,int *pNum,int *pMaxNum,int *UseLen)
{
	int MaxNum=0,flag=0,gErr=0,uNum=0;
//	char OutStr[20];
	char *pEndRam;
	DfFlagGuideRAM *pGuideRAM;
	pGuideRAM=pAppRamGuideHead;
	pEndRam=(char*)pAppRamGuideHead;
	if(pStrout)
	{
		*pStrout++ ='V';
		*pStrout++ ='7';
		*pStrout++ =':';
		*pStrout++ ='[';
	}
	while(pGuideRAM != pAppRamGuideEnd)
	{
		MaxNum++;
		if(pGuideRAM->pNfree)
		{
			if(pStrout) *pStrout++ ='_';
			if(flag) gErr++;
			else flag=1;
		}
		else 
		{
			if(pStrout) *pStrout++ ='8';
			pEndRam=(char*)pGuideRAM;
			uNum++;
			flag=0;
		}
		pGuideRAM = pGuideRAM->pNext;
	}
	if(pNum) *pNum=uNum;
	if(pMaxNum) *pMaxNum=MaxNum;
	if(UseLen) *UseLen=pEndRam-(char*)pAppRamGuideHead;
	
	if(pStrout)
	{
		*pStrout++ =']';
		*pStrout++ ='\0';
		//-----ת������ָ��
		pStrout = (char*)(((unsigned)pStrout+(sizeof(unsigned*)-1))&(~(sizeof(unsigned*)-1)));
		pEndRam=pStrout;
		pStrout += sizeof(unsigned*);
		//----------------------------------------------------------------
		uNum= MaxNum-uNum;
		MaxNum=0;
		pGuideRAM=pRamQuick;
		while(pGuideRAM != pAppRamGuideEnd)
		{
			MaxNum++;
			*((unsigned *)pStrout)=(char*)pGuideRAM->pNext-pGuideRAM->MsgData;
			pStrout += sizeof(unsigned*);
			pGuideRAM = pGuideRAM->pNfree;
		}
		*((unsigned *)pEndRam)=MaxNum;
		return uNum-MaxNum;
	}
	return gErr;
}




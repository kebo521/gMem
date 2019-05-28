//====================================================================
//====================================================================
/*****************************公共内存管理******************************
//功能-------  用于动态申请公共内存
//作者-------  邓国祖
//创作时间--20130604
//修改: ( 邓国祖) 20131213   --V2(重新整合释放的内存，起用回收机制)
//修改: ( 邓国祖) 20131213   --V3(增加内存申请快速切入口，
								     申请与释放的效率提高53.00%(V2-59037,V3-27755))
//修改: ( 邓国祖) 20140612   --V4(增加结构体向上的指针,释放内存速度大幅度提升)
//修改: ( 邓国祖) 20150504   --V5(增加结构体向上的指针参数,释放内存速度大幅度提升)
											综合申请与释放的效率提高36.06%(V3-87243,V5-55786),V4相比V3提升不大)
//修改: ( 邓国祖) 20161108   --V6(增加预授内存空间与完成内存空间预授两个功能，使用效率不变，速率不变)
//修改: ( 邓国祖) 20161118   --V7(增加虚拟空闲关连排列表(从小到大)，进一步提升内存使用效率，
									综合申请与释放的效率提高29.1%(256:V6-7077,V7-5010),V6相比V5相同)
******************************************************************************/
#ifndef NULL
#define	NULL				(0)
#endif
#include "gMem.h"

//-------------------内存链表结构----------------------------------------
typedef struct _DfFlagGuideRAM
{ //---将pNext放在中间的位位置避免越界风险----------------------------- 
	struct _DfFlagGuideRAM  *pNfree;			//虚拟层:下一个空闲节点(最后一个节点为END,为NULL表示占用)
	struct _DfFlagGuideRAM	*pLast;				//物理层:上一结点(第一个节点为NULL)
	struct _DfFlagGuideRAM  *pNext;				//物理层:下一结点(最后一个节点为END)
	char 	MsgData[0];  //任意大小,一个结点所需要的最小空间
} DfFlagGuideRAM;
//-------------------内存初如指针定义----------------------------------------
static DfFlagGuideRAM *pAppRamGuideHead;		//--起始地址---
static DfFlagGuideRAM *pAppRamGuideEnd;			//--终点地址---
//-------------------加速处理变量定义----------------------------------------
static DfFlagGuideRAM *pRamQuick;	//快速处虚拟排列地址入口
//-------------------分配最小单元---------------------------------
static unsigned uAppRamParticleSize;
//----------------异常功能信息显示-----------
static PFUNC_SHOW_MSG pFunAbnormalMsg=NULL;
void gLoadFunAllocShowMsg(PFUNC_SHOW_MSG pFun)
{
	pFunAbnormalMsg=pFun;
}
#define SHOW_MEM_ALLOC_MSG(pTitle,pMsg,Mpar)		{if(pFunAbnormalMsg) (*pFunAbnormalMsg)(pTitle,pMsg,Mpar);}
//====================================================================
//作者:     邓国祖	---
//功能:    将新节点加入虚拟关连排列表
//输入数据:pGuideHead需有加入的节点
//输出数据:无
//创作时间:  	20161118
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
	//---排序由小到大空间分布----
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
//作者:     邓国祖	---
//功能:    从虚拟层关连排列表移除节点
//输入数据:pGuideHead需有移除的节点
//输出数据:无
//创作时间:  	20161118
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
//作者:     邓国祖	---
//功能:    释放物理层节点，连接相邻空节点
//输入数据:pGuideRAM需有释放的节点
//输出数据:无
//创作时间:  	20161121
//---------------------------------------------------------------
static void gFreePhysicalNode(DfFlagGuideRAM *pGuideRAM)
{
	DfFlagGuideRAM *pNext=pGuideRAM->pNext;
	//------释放节点空间---------------------------
	if(pNext != pAppRamGuideEnd)
	{
		if(pNext->pNfree)
		{//------删除链表(向后)------------
			//------删除空闲关联-------------
			gDeleteEmptyNode(pNext);
			pNext=pNext->pNext;
			//------删除后链表------------
			pGuideRAM->pNext= pNext;
			if(pNext != pAppRamGuideEnd) 
				pNext->pLast=pGuideRAM;
		}
	}
	if((char*)pGuideRAM->pLast != NULL)
	{
		DfFlagGuideRAM *pLast=pGuideRAM->pLast;
		if(pLast->pNfree)//(pGuideRAM->pLast->pLast==NULL)
		{//------删除链表(向前)------------
			//------删除空闲关联-------------
			gDeleteEmptyNode(pLast);
			//------删除本链表------------
			pLast->pNext=pNext;
			if(pNext != pAppRamGuideEnd) 
				pNext->pLast=pLast;
			pGuideRAM=pLast;
		}
	}
	gIncreaseEmptyNode(pGuideRAM);
}

//====================================================================
//作者:     邓国祖	---
//功能:    分割空间
//输入数据:pGuideRAM需要分割的空间节点，silen 要分割的大小
//输出数据:无
//创作时间:  	20170217
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
			//------删除空闲关联-------------
			gDeleteEmptyNode(pNext);
			//------删除链表(向后)------------
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
//作者:     邓国祖	---
//功能:    用于初始化内存使用的区域
//输入数据:mem_addr所申请区域的首地址,mem_size,可以使用的内存大小,particle_size 最小申请单元,取值为2^(2+n)
//输出数据:无
//创作时间:  	20130604
//---------------------------------------------------------------
int gMemAllocInit(void* mem_addr,unsigned mem_size,unsigned particle_size)
{
	if(((unsigned)mem_addr)&(sizeof(char*)-1)) 
	{
		pAppRamGuideEnd=NULL;
		pRamQuick=NULL;
		SHOW_MEM_ALLOC_MSG("内存分配初始化","内存[%x]不合要求",(int)mem_addr);
		return -1;
	}
	pAppRamGuideHead=(DfFlagGuideRAM *)mem_addr;//(mem_addr+3)&(~3);
	pAppRamGuideEnd=(DfFlagGuideRAM *)((char*)mem_addr+mem_size);
	//-------------快速处理------------
	pRamQuick=pAppRamGuideHead;
	pRamQuick->pNext=pAppRamGuideEnd;
	pRamQuick->pNfree=pAppRamGuideEnd;//本身就是最大的空闲
	pRamQuick->pLast=NULL;	//第一条指向空
	//------------最小单元------------------
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
//功能:    用于申请固定内存功能，
//输入数据:silen 需要申请内存大小
//输出数据:所申请区域的首地址，申请失败时返回为空地址
//作者:     邓国祖	---	
//创作时间:  	20140612	-V4
//---------------------------------------------------------------
void *gMalloc(unsigned silen)
{
	unsigned iLen;
	DfFlagGuideRAM *pGuideRAM,*pLastfree;//register 
	//------申请长度整成4整数倍(避免指针偏移)--------------
	silen =(silen+uAppRamParticleSize)&(~uAppRamParticleSize); 
	//-----最快的入口-------
	pGuideRAM=pRamQuick;
	pLastfree=NULL;
	while(pGuideRAM != pAppRamGuideEnd)
	{
		iLen=((char*)pGuideRAM->pNext - pGuideRAM->MsgData);
		if(iLen >= silen)
		{
			//------删除空闲关联=gDeleteEmptyNode--------
			if(pLastfree) 
				pLastfree->pNfree=pGuideRAM->pNfree;
			else //if(pRamQuick==pGuideRAM) 
				pRamQuick=pGuideRAM->pNfree;
			pGuideRAM->pNfree=NULL;	//空闲指针为空，非空闲，表示占用。
			//--------检查是否可以再分割--------------
			if(iLen > (silen+sizeof(DfFlagGuideRAM)))
			{//---多于的空间，用于增加一个数据空间结点--------
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
	SHOW_MEM_ALLOC_MSG((char*)"内存申请",(char*)"申请%d空间不足",silen);
//	FreeCheckRAM();
	return NULL;
}

//====================================================================
//功能:    用于释放之前申请的内存
//输入数据:pfree所申请区域的首地址，用于释放对应用的区域。
//输出数据:无
//作者:     邓国祖	---	
//创作时间:  	20140612	-V4
//---------------------------------------------------------------
void gFree(void *pfree)
{
	DfFlagGuideRAM *pGuideRAM,*pNext;//,*pLast;//register 
	//---------检查输入指针的有效性---------------------------------------
	if((DfFlagGuideRAM *)pfree <pAppRamGuideHead || (DfFlagGuideRAM *)pfree >pAppRamGuideEnd) return;
	pGuideRAM=(DfFlagGuideRAM *)(((char*)pfree)-(sizeof(DfFlagGuideRAM)));
	//---------检查指针结点的正确性，避免错误干扰---------------------------------------
	if(pGuideRAM->pNext<pAppRamGuideHead||pGuideRAM->pNext>pAppRamGuideEnd) return;
	//---------如果已经释放直接返回---------
	if(pGuideRAM->pNfree)
	{
		SHOW_MEM_ALLOC_MSG((char*)"内存释放",(char*)"重复释放%x同一空间",(int)pfree);
		return;
	}
	pNext=pGuideRAM->pNext;
	//------释放节点空间---------------------------
	if(pNext != pAppRamGuideEnd)
	{
		if(pNext->pNfree)
		{//------删除链表(向后)------------
			//------删除空闲关联-------------
			gDeleteEmptyNode(pNext);
			pNext=pNext->pNext;
			//------删除后链表------------
			pGuideRAM->pNext=pNext;
			if(pNext != pAppRamGuideEnd) 
				pNext->pLast=pGuideRAM;
		}
	}
	if(pGuideRAM->pLast != NULL)
	{
		DfFlagGuideRAM *pLast=pGuideRAM->pLast;
		if(pLast->pNfree)//(pGuideRAM->pLast->pLast==NULL)
		{//------删除链表(向前)------------
			//------删除空闲关联-------------
			gDeleteEmptyNode(pLast);
			//------删除本链表------------
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
//功能:    用于预授一定量的内存空间，
//输入数据:sPreLen 需要预授内存大小(0为系统最大空间)
//输出数据:所申请区域的首地址，申请失败时返回为空地址，OutSize预授空间最大值
//作者:     邓国祖	---	
//创作时间:  	20161108	-V6
//---------------------------------------------------------------
void *gPralloc(unsigned sPreLen,unsigned *OutSize)
{
	unsigned iLen;
	DfFlagGuideRAM *pGuideRAM,*pLastfree;//register 
	if(pRamQuick == pAppRamGuideEnd)
	{
		SHOW_MEM_ALLOC_MSG((char*)"内存预授",(char*)"无空闲空间[%d]",sPreLen);
		return NULL;
	}
	//-----最快的入口-------
	pGuideRAM=pRamQuick;
	pLastfree=NULL;
	if(sPreLen)
	{
		//-申请长度整成4整数倍(避免指针偏移)-
		sPreLen =(sPreLen+uAppRamParticleSize)&(~uAppRamParticleSize); 
		while(pGuideRAM != pAppRamGuideEnd)
		{
			iLen=((char*)pGuideRAM->pNext - pGuideRAM->MsgData);
			if(iLen >= sPreLen)
			{
				//---删除空闲关联=gDeleteEmptyNode--------
				if(pLastfree) 
					pLastfree->pNfree=pGuideRAM->pNfree;
				else //if(pRamQuick==pGuideRAM) 
					pRamQuick=pGuideRAM->pNfree;
				pGuideRAM->pNfree=NULL;	//空闲指针为空，非空闲，表示占用。
				//--------检查是否可以再分割--------------
				if(iLen > (sPreLen+sizeof(DfFlagGuideRAM)))
				{//---多于的空间，用于增加一个数据空间结点--------
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
				return (void*)pGuideRAM->MsgData; //需返回的空间地址;
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
		//---删除空闲关联=gDeleteEmptyNode--------
		if(pLastfree) 
			pLastfree->pNfree=pGuideRAM->pNfree;
		else //if(pRamQuick==pGuideRAM) 
			pRamQuick=pGuideRAM->pNfree;
		//----设置空闲状态-----------
		pGuideRAM->pNfree=NULL; //空闲指针为空(非空闲)表示占用。
		*OutSize=((char*)pGuideRAM->pNext - pGuideRAM->MsgData);
		return (void*)pGuideRAM->MsgData;
	}
	SHOW_MEM_ALLOC_MSG((char*)"内存预授",(char*)"申请%d空间不足",sPreLen);
	return NULL;
}

//====================================================================
//功能:    用于完成内存空间的预授，
//输入数据:pMemory所预授的内存地址   silen 需要预授内存大小(0释放预授)
//输出数据:返回结果(0表示成功)
//作者:     邓国祖	---	
//创作时间:  	20161108	-V6
//---------------------------------------------------------------
void* gRealloc(void *pMemory,unsigned silen)
{
	DfFlagGuideRAM *pGuideRAM;//register 
	//---------检查输入指针的有效性---------------------------------------
	if((DfFlagGuideRAM *)pMemory < pAppRamGuideHead || (DfFlagGuideRAM *)pMemory >pAppRamGuideEnd)
	{
		SHOW_MEM_ALLOC_MSG((char*)"内存重新分配",(char*)"空间[%x]无权操作",(int)pMemory);
		return NULL;
	}
	pGuideRAM=(DfFlagGuideRAM *)(((char*)pMemory)-(sizeof(DfFlagGuideRAM)));
	//---------检查指针结点的正确性，避免错误干扰---------------------------------------
	if(pGuideRAM->pNext<pAppRamGuideHead||pGuideRAM->pNext>pAppRamGuideEnd)
	{
		SHOW_MEM_ALLOC_MSG((char*)"内存重新分配",(char*)"空间[%x]操作无效",(int)pMemory);
		return NULL;
	}
	//---------如果已经释放直接返回---------
	if(pGuideRAM->pNfree)
	{
		SHOW_MEM_ALLOC_MSG((char*)"内存重新分配",(char*)"空间[%x]已经释放",(int)pMemory);
		return NULL;
	}
	
	if(silen)
	{
		unsigned Len;
		silen =(silen+uAppRamParticleSize)&(~uAppRamParticleSize); 
		Len=(char*)pGuideRAM->pNext - pGuideRAM->MsgData;	//计算容量
		if(Len < silen)
		{//----现有空间不足-----
			if(pGuideRAM->pNext != pAppRamGuideEnd)
			{
				if(pGuideRAM->pNext->pNfree)
				{//检查下一个空间，是否为空
					unsigned Len2=(char*)pGuideRAM->pNext->pNext - pGuideRAM->MsgData;
					if(Len2 >= silen)	//空间够
					{
						gDeleteEmptyNode(pGuideRAM->pNext);
						pGuideRAM->pNext=pGuideRAM->pNext->pNext;
						if(pGuideRAM->pNext != pAppRamGuideEnd)
							pGuideRAM->pNext->pLast=pGuideRAM;
						if(Len2 > (silen+sizeof(DfFlagGuideRAM)))
						{
							gMemSplitSpace(pGuideRAM,silen);
						}
						return pMemory; //不改变地址
					}
				}
			}
			//-----现有空间与可能合并空间无法满足-----
			{
				unsigned *p1,*p2,Max;
				p1=(unsigned *)gMalloc(silen);
				if (p1)
				{
					p2 = (unsigned *)pGuideRAM->MsgData;
					//-----恢复原来数据----------
					Max = Len / sizeof(unsigned);
					while (Max--) p1[Max] = p2[Max];					
				}	
				//-----预授取消-----释放空间----
				gFreePhysicalNode(pGuideRAM);
				return p1; //改变地址，但这种情况存在风险
			}
		}
		else 
		{
			if(Len > (silen+sizeof(DfFlagGuideRAM)))
			{//---多于的空间，用于增加一个数据空间结点--------
				gMemSplitSpace(pGuideRAM,silen);
			}
			return pMemory; //不改变地址
		}
	}
	//-----预授取消-----释放空间----
	gFreePhysicalNode(pGuideRAM);
	return NULL;
}

  


//====================================================================
//功能:    用于显示内在使用情况(主要用于调试)
//输入数据:无
//输出数据:无
//作者:     邓国祖	---	
//创作时间:  	20130604
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
		//-----转成整形指针
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




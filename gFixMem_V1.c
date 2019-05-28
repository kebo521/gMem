//====================================================================
//====================================================================
/*****************************定长公共内存管理******************************
//功能-------  用于定长动态申请公共内存
//作者-------  邓国祖
//创作时间--20190402
******************************************************************************/
#ifndef NULL
#define	NULL				(0)
#endif
#include "gMem.h"

//-------------------内存链表结构----------------------------------------
typedef struct _DfFlagGuideRAM
{ //---将pNext放在中间的位位置避免越界风险----------------------------- 
	struct _DfFlagGuideRAM  *pNfree;			//虚拟层:下一个空闲节点(最后一个节点为END,为NULL表示占用)
} DfFlagGuideRAM;
//-------------------内存初如指针定义----------------------------------------
static DfFlagGuideRAM *pAppRamGuideHead;		//--起始地址---
static DfFlagGuideRAM *pAppRamGuideEnd;			//--终点地址---
//-------------------加速处理变量定义----------------------------------------
static DfFlagGuideRAM *pRamQuick=NULL;	//快速处虚拟排列地址入口
//----------------异常功能信息显示-----------
static PFUNC_SHOW_MSG pFunAbnormalMsg=NULL;
void gLoadFunAllocShowMsg(PFUNC_SHOW_MSG pFun)
{
	pFunAbnormalMsg=pFun;
}
#define SHOW_MEM_ALLOC_MSG(pTitle,pMsg,Mpar)		{if(pFunAbnormalMsg) (*pFunAbnormalMsg)(pTitle,pMsg,Mpar);}
//====================================================================
//作者:     邓国祖	---
//功能:    用于初始化内存使用的区域
//输入数据:mem_addr所申请区域的首地址,mem_size,可以使用的内存大小,particle_size 最小申请单元,取值为2^(2+n)
//输出数据:无
//创作时间:  	20130604
//---------------------------------------------------------------
int gMemAllocInit(void* mem_addr,unsigned mem_size,unsigned particle_size)
{
	unsigned i,num;
	DfFlagGuideRAM *pNext;
	if(((unsigned)mem_addr)&(sizeof(char*)-1)) 
	{
		SHOW_MEM_ALLOC_MSG("内存分配初始化","内存[%x]不合要求",(int)mem_addr);
		return -1;
	}
	particle_size = (particle_size+3)&(~3);
	if(mem_size < particle_size)
	{
		SHOW_MEM_ALLOC_MSG("内存分配初始化","错误[m,%d < p,%d]",(int)mem_size,particle_size);
		return 0;
	}
	pAppRamGuideHead=(DfFlagGuideRAM *)mem_addr;		//(mem_addr+3)&(~3);
	pAppRamGuideEnd=(char*)pAppRamGuideHead + mem_size;			//--终点地址---
	//------------最小单元------------------
	i=particle_size;
	num=0;
	pNext=pAppRamGuideHead;
	while(1)
	{
		pNext->pNfree = (DfFlagGuideRAM *)((char*)pNext + i);
		num++;
		i += particle_size;
		if(i > mem_size) break;
		pNext = pNext->pNfree;
	}
	pNext->pNfree=NULL;
	//-------------快速处理------------
	pRamQuick= pAppRamGuideHead;
	return num;
}

//====================================================================
//功能:    用于申请固定内存功能，
//输入数据:无
//输出数据:所申请区域的首地址，大小为particle_size
//---------------------------------------------------------------
void *gfixMalloc(void)
{
	register DfFlagGuideRAM *pGuideRAM=pRamQuick;
	//-----最快的入口-------
	if(pGuideRAM==NULL)
	{
		SHOW_MEM_ALLOC_MSG("内存申请","空间已用完%x",pGuideRAM);
		return pGuideRAM;
	}
	pRamQuick=pGuideRAM->pNfree;
	return pGuideRAM;
}

//====================================================================
//功能:    用于释放之前申请的内存
//输入数据:gfixFree所申请区域的首地址，用于释放对应用的区域。
//输出数据:无
//---------------------------------------------------------------
void gfixFree(void *pfree)
{
	register DfFlagGuideRAM *pGuideRAM=(DfFlagGuideRAM *)pfree;
	//---------检查输入指针的有效性---------------------------------------
	if(pGuideRAM <pAppRamGuideHead || pGuideRAM >=pAppRamGuideEnd) 
	{
		SHOW_MEM_ALLOC_MSG("内存释放","内存[%x]非法",pfree);
		return;
	}
	pGuideRAM->pNfree = pRamQuick;
	pRamQuick=pGuideRAM;
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
	int MaxNum=0;
	DfFlagGuideRAM *pGuideRAM;
	pGuideRAM=pRamQuick;
	if(pStrout)
	{
		*pStrout++ ='V';
		*pStrout++ ='F';
		*pStrout++ =':';
		*pStrout++ ='[';
	}
	while(pGuideRAM != NULL)
	{
		MaxNum++;
		if(pStrout) *pStrout++ ='_';
		pGuideRAM = pGuideRAM->pNfree;
	}
	
	if(pStrout)
	{
		*pStrout++ =']';
		*pStrout++ ='\0';
	}
	return MaxNum;
}




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
******************************************************************************/
#ifndef KTYPE
typedef unsigned short  		u16;
#endif
#ifndef NULL
#define	NULL				(0)
#endif
extern int APP_ShowMsg(char* pTitle,char* pMsg,int tTimeOutMS);
extern int API_sprintf(char* str, const char* format, ...);
extern int API_strlen(const char*);

//-------------------内存链表结构----------------------------------------
typedef struct _DfFlagGuideRAM
{ //---将pNext放在中间的位位置避免越界风险-----------------------------   
	u16 	uSize; 						//本段地址存在标签,0x0000 已释放		
	u16 	uLast; 						//上一结点，=NULL时空间已经释放
	struct _DfFlagGuideRAM  *pNext;
	char 	MsgData[4];  //任意大小,一个结点所需要的最小空间
} DfFlagGuideRAM;
//-------------------内存初如指针定义----------------------------------------
static char* pAppRamGuideHead;		//--起始地址---
static char* pAppRamGuideEnd;		//--终点地址---
//-------------------加速处理变量定义----------------------------------------
static DfFlagGuideRAM *pRamQuick;	//快速处理地址记录器
//====================================================================
//作者:     邓国祖	---
//功能:    用于初始化内存使用的区域
//输入数据:pGuideHead所申请区域的首地址,RamLen,可以使用的内存大小
//输出数据:无
//创作时间:  	20130604
//---------------------------------------------------------------
void InitGuideRAM(void* pGuideHead,unsigned RamLen)
{
//	TRACE("InitGuideRAM Sta[%x],Len[%d]\r\n",pGuideHead,RamLen);
	pAppRamGuideHead=(char*)pGuideHead;//(pGuideHead+3)&(~3);
	pAppRamGuideEnd=pAppRamGuideHead+RamLen;
	//-------------快速处理------------
	pRamQuick=(DfFlagGuideRAM *)pAppRamGuideHead;
	pRamQuick->pNext=NULL;
	pRamQuick->uSize=0;
	pRamQuick->uLast=0;	//第一条指向本身
}
//====================================================================
//功能:    用于申请固定内存功能，
//输入数据:silen 需要申请内存大小
//输出数据:所申请区域的首地址，申请失败时返回为空地址
//作者:     邓国祖	---	
//创作时间:  	20140612	-V4
//---------------------------------------------------------------
void *gMalloc(unsigned int silen)
{
	unsigned int iLen;
	char *pRet;
	DfFlagGuideRAM *pGuideRAM,*pNext;//register 
	//------申请长度整成4整数倍(避免指针偏移)--------------
	iLen=sizeof(char*)-1;
	silen =(silen+iLen)&(~iLen); 
	//-----最快的入口-------
	//-----计算最快的入口-------
	while(pRamQuick->uSize)
		pRamQuick=pRamQuick->pNext;
	pGuideRAM=pRamQuick;
	//-------------------------------------------------------
	while(pGuideRAM->pNext)	//pGuideRAM->pNext!=NULL
	{	
		if(!pGuideRAM->uSize)//pGuideRAM->size == 0
		{//--------回收处理------
			iLen=(char*)pGuideRAM->pNext - pGuideRAM->MsgData;	//计算容量
			if(iLen >= silen)
			{
				pGuideRAM->uSize=1;
				pRet= pGuideRAM->MsgData;
				if((iLen-silen) >= (sizeof(DfFlagGuideRAM)))
				{//---多于的空间，用于增加一个数据空间结点--------
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
		APP_ShowMsg((char*)"内存申请",(char*)"空间不足",10000);
//		FreeCheckRAM();
		return NULL;
	}
	//--------New Ram-----
	pRet=pGuideRAM->MsgData;
	pGuideRAM->pNext=(DfFlagGuideRAM *)&pRet[silen];
	pGuideRAM->uSize=1;
	pNext =pGuideRAM->pNext;//---指向下一条
	pNext->pNext=NULL;	//下一条制空
	pNext->uSize=0;//=1;
	pNext->uLast=(u16)((char*)pGuideRAM-pAppRamGuideHead);	//赋值下一条向上指针
	return (void*)pRet;
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
	DfFlagGuideRAM *pGuideRAM,*pLast;//register 
	//---------检查输入指针的有效性---------------------------------------
	if((char*)pfree <pAppRamGuideHead || (char*)pfree >pAppRamGuideEnd) return;
	pGuideRAM=(DfFlagGuideRAM *)(((char*)pfree)-(sizeof(DfFlagGuideRAM)-sizeof(pGuideRAM->MsgData)));
	//---------检查指针结点的正确性，避免错误干扰---------------------------------------
	//if(pGuideRAM->uLast > (u16)(pAppRamGuideEnd-pAppRamGuideHead)) return;
	if((char*)pGuideRAM->pNext<pAppRamGuideHead||(char*)pGuideRAM->pNext>pAppRamGuideEnd) return;
	//--(pGuideRAM->pNext->pPrev==NULL||pGuideRAM->pNext->pNext==NULL)------------
	if(pGuideRAM->uSize==0x00)
	{
		APP_ShowMsg((char*)"内存释放",(char*)"重复释放同一空间",3000);
		return;
	}
	pGuideRAM->uSize=0x00;	//对指针做空标记
	if(!pGuideRAM->pNext->uSize)
	{//------删除链表(向后)------------
		pGuideRAM->pNext=pGuideRAM->pNext->pNext;
		if(pGuideRAM->pNext) 
			pGuideRAM->pNext->uLast=(u16)((char*)pGuideRAM-pAppRamGuideHead);
	}
	pLast=(DfFlagGuideRAM *)(pAppRamGuideHead+pGuideRAM->uLast);
	if(!pLast->uSize)//(pGuideRAM->pLast->pLast==NULL)
	{//------删除链表(向前)------------
		pLast->pNext=pGuideRAM->pNext;
		if(pLast->pNext) 
			pLast->pNext->uLast=(u16)((char*)pLast-pAppRamGuideHead);
		pGuideRAM=pLast;
	}
	if(pRamQuick > pGuideRAM) pRamQuick= pGuideRAM;	//新的快速入口，下一次申请就从这里开始	
//	TRACE("find OVER_[%x]\r\n",pfree);
//	UI_ShowInfo("find OVER");
}
//====================================================================
//功能:    用于显示内在使用睛况(主要用于调试)
//输入数据:无
//输出数据:无
//作者:     邓国祖	---	
//创作时间:  	20130604
//---------------------------------------------------------------
int gCheckAllocStatus(char *pStrout,int *fNum)
{
	int MaxNum=0,flag=0,gErr=0;
//	char OutStr[20];
	char *pEndRam;
	DfFlagGuideRAM *pGuideRAM;
	pGuideRAM=(DfFlagGuideRAM *)pAppRamGuideHead;
	pEndRam=pAppRamGuideHead;
	*fNum = 0;
	if(pStrout)
	{
		*pStrout++ ='V';
		*pStrout++ ='5';
		*pStrout++ =':';
		*pStrout++ ='[';
	}
	while(pGuideRAM->pNext)
	{
		MaxNum++;
		if(pGuideRAM->uSize)
		{
			if(pStrout) *pStrout++ ='8';
			pEndRam=(char*)pGuideRAM;
			(*fNum)++;
			flag=0;
		}
		else 
		{
			if(pStrout) *pStrout++ ='_';
			if(flag) gErr++;
			else flag=1;
		}
		pGuideRAM = pGuideRAM->pNext;
	}
	if(pStrout)
	{
		*pStrout++ =']';
		API_sprintf(pStrout,"---[%d]>MaxNum[%d],EndRAM[%d],Err[%d]\r\n",fNum,MaxNum,pEndRam-pAppRamGuideHead,gErr);
	}
	return gErr;
}





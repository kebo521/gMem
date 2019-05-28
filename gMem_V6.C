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
//修改: ( 邓国祖) 20161108   --V6(增加预授内存空间与完成内存空间预授两个功能)
******************************************************************************/
#ifndef KTYPE
typedef unsigned short  		u16;
#endif
#ifndef NULL
#define	NULL				(0)
#endif

//==========定义内存类型标记==============
#define	NULL_MEMORY					(0)
#define	NORMAL_MEMORY				(1)
#define	PREAUTH_MEMORY				(2)

//==============外部引用======================
extern int APP_ShowMsg(char* pTitle,char* pMsg,int tTimeOutMS);
extern int 	API_sprintf(char* str, const char* format, ...);
static void UI_ShowMemMsg(char* pTitle,char* pMsg,int Mpar)
{
	char showErr[24];
	API_sprintf(showErr,pMsg,Mpar);
	APP_ShowMsg(pTitle,showErr,10000);
}

//-------------------内存链表结构----------------------------------------
typedef struct _DfFlagGuideRAM
{ //---将pNext放在中间的位位置避免越界风险-----------------------------   
	unsigned 	uSize:8; 						//本段地址存在标签,0x0000 已释放		
	unsigned	uLast:24;						//上一结点，=NULL时空间已经释放
	struct _DfFlagGuideRAM  *pNext;
	char 	MsgData[0];  //任意大小,一个结点所需要的最小空间
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
	pRamQuick->pNext=(DfFlagGuideRAM *)pAppRamGuideEnd;
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
void *gMalloc(unsigned silen)
{
	unsigned int iLen;
	DfFlagGuideRAM *pGuideRAM;//register 
	//------申请长度整成4整数倍(避免指针偏移)--------------
	iLen=sizeof(char*)-1;
	silen =(silen+iLen)&(~iLen); 
	//-----最快的入口-------
	//-----计算最快的入口-------
	while(pRamQuick->uSize!=NULL_MEMORY)
		pRamQuick=pRamQuick->pNext;
	pGuideRAM=pRamQuick;
	//-------------------------------------------------------
	while((char*)pGuideRAM != pAppRamGuideEnd)	//pGuideRAM->pNext!=NULL
	{	
		if(pGuideRAM->uSize==NULL_MEMORY)//pGuideRAM->size == 0
		{//--------回收处理------
			iLen=(char*)pGuideRAM->pNext - pGuideRAM->MsgData;	//计算容量
			if(iLen >= silen)
			{
				pGuideRAM->uSize=NORMAL_MEMORY;
				if(iLen > (silen+sizeof(DfFlagGuideRAM)))
				{//---多于的空间，用于增加一个数据空间结点--------
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
	UI_ShowMemMsg((char*)"内存申请",(char*)"申请%d空间不足",silen);
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
	DfFlagGuideRAM *pGuideRAM;//,*pLast;//register 
	//---------检查输入指针的有效性---------------------------------------
	if((char*)pfree <pAppRamGuideHead || (char*)pfree >pAppRamGuideEnd) return;
	pGuideRAM=(DfFlagGuideRAM *)(((char*)pfree)-(sizeof(DfFlagGuideRAM)));
	//---------检查指针结点的正确性，避免错误干扰---------------------------------------
	//if(pGuideRAM->uLast > (u16)(pAppRamGuideEnd-pAppRamGuideHead)) return;
	if((char*)pGuideRAM->pNext<pAppRamGuideHead||(char*)pGuideRAM->pNext>pAppRamGuideEnd) return;
	//--(pGuideRAM->pNext->pPrev==NULL||pGuideRAM->pNext->pNext==NULL)------------
	if(pGuideRAM->uSize==NULL_MEMORY)
	{
		UI_ShowMemMsg((char*)"内存释放",(char*)"重复释放%x同一空间",(int)pfree);
		return;
	}
	//------释放节点空间---------------------------
	{
		pGuideRAM->uSize=NULL_MEMORY;	//对指针做空标记
		if((char*)pGuideRAM->pNext != pAppRamGuideEnd)
		{
			if(pGuideRAM->pNext->uSize==NULL_MEMORY)
			{//------删除链表(向后)------------
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
			{//------删除链表(向前)------------
				pLast->pNext=pGuideRAM->pNext;
				if((char*)pLast->pNext != pAppRamGuideEnd) 
					pLast->pNext->uLast=((char*)pLast->pNext - (char*)pLast);
				pGuideRAM=pLast;
			}
		}
		if(pRamQuick > pGuideRAM) pRamQuick= pGuideRAM;	//新的快速入口，下一次申请就从这里开始	
	}
//	TRACE("find OVER_[%x]\r\n",pfree);
//	UI_ShowInfo("find OVER");
}
//====================================================================
//功能:    用于预授一定量的内存空间，
//输入数据:sPreLen 需要预授内存大小(0为系统最大空间)
//输出数据:所申请区域的首地址，申请失败时返回为空地址，sPreLen预授空间最大值
//作者:     邓国祖	---	
//创作时间:  	20161108	-V6
//---------------------------------------------------------------
void *gPralloc(unsigned sPreLen,unsigned *OutSize)
{
	DfFlagGuideRAM *pGuideRAM;//register 
	//-----计算最快的入口-------
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
					//-------------下面部分可有可无-------------------------------
					if(Len > (sPreLen+sizeof(DfFlagGuideRAM)))
					{//---多于的空间，用于增加一个数据空间结点--------
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
	UI_ShowMemMsg((char*)"内存申请",(char*)"预授%d空间不足",sPreLen);
//	FreeCheckRAM();
	return NULL;
}
//====================================================================
//功能:    用于完成内存空间的预授，
//输入数据:pMemory所预授的内存地址   silen 需要预授内存大小(0释放预授)
//输出数据:返回结果(0表示成功)
//作者:     邓国祖	---	
//创作时间:  	20161108	-V4
//---------------------------------------------------------------
void* gRealloc(void *pMemory,unsigned silen)
{
	DfFlagGuideRAM *pGuideRAM;//register 
	//---------检查输入指针的有效性---------------------------------------
	if((char*)pMemory <pAppRamGuideHead || (char*)pMemory >pAppRamGuideEnd) return 1;
	pGuideRAM=(DfFlagGuideRAM *)(((char*)pMemory)-(sizeof(DfFlagGuideRAM)));
	//---------检查指针结点的正确性，避免错误干扰---------------------------------------
	if((char*)pGuideRAM->pNext<pAppRamGuideHead||(char*)pGuideRAM->pNext>pAppRamGuideEnd) return 2;
	//--(pGuideRAM->pNext->pPrev==NULL||pGuideRAM->pNext->pNext==NULL)------------
	if(pGuideRAM->uSize==PREAUTH_MEMORY)
	{
		if(silen)
		{
			unsigned Len;
			Len=sizeof(char*)-1;
			silen =(silen+Len)&(~Len); 
			Len=(char*)pGuideRAM->pNext - pGuideRAM->MsgData;	//计算容量
			if(Len < silen)
			{
				pGuideRAM->uSize=NULL_MEMORY;
				return 3;
			}
			pGuideRAM->uSize=NORMAL_MEMORY;
			if(Len > (silen+sizeof(DfFlagGuideRAM)))
			{//---多于的空间，用于增加一个数据空间结点--------
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
					{//------删除链表(向后)------------
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
		{//-----预授取消-----释放空间----
			DfFlagGuideRAM *pLast;
			pGuideRAM->uSize=NULL_MEMORY;	//对指针做空标记
			if((char*)pGuideRAM->pNext != pAppRamGuideEnd)
			{
				if(pGuideRAM->pNext->uSize==NULL_MEMORY)
				{//------删除链表(向后)------------
					pGuideRAM->pNext=pGuideRAM->pNext->pNext;
					if((char*)pGuideRAM->pNext != pAppRamGuideEnd) 
						pGuideRAM->pNext->uLast=((char*)pGuideRAM->pNext - (char*)pGuideRAM);
				}
			}
			//if(pGuideRAM->uLast)
			{
				pLast=(DfFlagGuideRAM *)((char*)pGuideRAM - pGuideRAM->uLast);
				if(pLast->uSize==NULL_MEMORY)//(pGuideRAM->pLast->pLast==NULL)
				{//------删除链表(向前)------------
					pLast->pNext=pGuideRAM->pNext;
					if((char*)pLast->pNext != pAppRamGuideEnd) 
						pLast->pNext->uLast=((char*)pLast->pNext - (char*)pLast);
					pGuideRAM=pLast;
				}
			}
			if(pRamQuick > pGuideRAM) pRamQuick= pGuideRAM;	//新的快速入口，下一次申请就从这里开始	
		}
		return pMemory;
	}
	return NULL;
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




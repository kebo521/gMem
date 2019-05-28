//====================================================================
/*****************************公共内存管理******************************
//功能-------  用于动态申请公共内存
//作者-------  邓国祖
//创作时间--20130604
//修改: ( 邓国祖) 20131213   --V2(重新整合释放的内存，起用回收机制)
//修改: ( 邓国祖) 20131213   --V3(增加内存申请快速切入口，
								     申请与释放的效率提高53.00%(V2-59037,V3-27755))
******************************************************************************/
//-------------------内存链表结构-(注意内存对称)---------------------------------------
typedef struct _DfFlagGuideRAM
{ //---将pNext放在中间的位位置避免越界风险-----------------------------   
//	u16 	Flag; 	//暂时不用，
	unsigned long size; 							//b必须是4个字节
	struct _DfFlagGuideRAM  *pNext;
	char 	MsgData[4];  //任意大小
} DfFlagGuideRAM;
//-------------------内存初如指针定义----------------------------------------
static char* pAppRamGuideHead;		//--起始地址---
static char* pAppRamGuideEnd;		//--终点地址---
//-------------------加速处理变量定义----------------------------------------
static DfFlagGuideRAM *pRamQuick;	//快速处理地址记录器
//====================================================================
//作者:     邓国祖	---
//功能:    用于初始化内存使用的区域
//输入数据:pGuideHead所申请区域的首地址(必须是4的整数倍地址),RamLen,可以使用的内存大小
//输出数据:无
//创作时间:  	20130604
//---------------------------------------------------------------
void InitGuideRAM(void* pGuideHead,unsigned RamLen)
{
//	TRACE("InitGuideRAM Sta[%x],Len[%d]\r\n",pGuideHead,RamLen);
	pAppRamGuideHead=(char*)pGuideHead;
	pAppRamGuideEnd=(pAppRamGuideHead+RamLen);
	//-------------快速处理------------
	pRamQuick=(DfFlagGuideRAM *)pAppRamGuideHead;
	pRamQuick->pNext=NULL;
	pRamQuick->size =0;	
}
//====================================================================
//功能:    用于申请固定内存功能，
//输入数据:silen 需要申请内存大小
//输出数据:所申请区域的首地址，申请失败时返回为空地址
//作者:     邓国祖	---	
//创作时间:  	20130604
//---------------------------------------------------------------
void *gmalloc(unsigned silen)
{
	unsigned iLen;
	char *pRet;
	DfFlagGuideRAM *pGuideRAM,*pNext;
	//------申请长度整成4整数倍(避免指针偏移)--------------
//	if(!silen) return NULL;	//申请的数据长度不能为0
	iLen=sizeof(char*)-1;
	silen =(silen+iLen)&(~iLen); 
	//-----计算最快的入口-------
	while(pRamQuick->size)
		pRamQuick=pRamQuick->pNext;
	
	pGuideRAM=pRamQuick;
	//-------------------------------------------------------
	while(pGuideRAM->pNext)	//pGuideRAM->pNext!=NULL
	{
		if(!pGuideRAM->size)//pGuideRAM->size == 0
		{//--------回收处理------
			iLen=(char*)pGuideRAM->pNext - pGuideRAM->MsgData;	//计算容量
			if(iLen >= silen)
			{
				pGuideRAM->size = silen;
				pRet= pGuideRAM->MsgData;
				if((iLen-silen) > (sizeof(DfFlagGuideRAM)-sizeof(pGuideRAM->MsgData)))
				{//---多于的空间，用于增加一个数据空间结点--------
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
	pGuideRAM =pGuideRAM->pNext;//---指向下一条
	pGuideRAM->pNext=NULL;	//下一条制空	
	pGuideRAM->size = 0;
	return (void*)pRet;
}
//====================================================================
//功能:    用于释放之前申请的内存
//输入数据:pfree所申请区域的首地址，用于释放对应用的区域。
//输出数据:无
//作者:     邓国祖	---	
//创作时间:  	20130604	-V1
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
			pGuideRAM->size=0;	//------清空数据标记------------
			if(pGuideRAM->pNext->size==0)
			{//------删除链表(向后)------------
				pGuideRAM->pNext=pGuideRAM->pNext->pNext;
			}
			if((pRamPrev != NULL) &&(pRamPrev->size ==0))
			{//------删除链表(向前)------------
				pRamPrev->pNext = pGuideRAM->pNext;
				pGuideRAM=pRamPrev;
			}
			//-----推算下一次申请最快入口--------------
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
//功能:    用于显示内在使用睛况(主要用于调试)
//输入数据:无
//输出数据:无
//作者:     邓国祖	---	
//创作时间:  	20130604
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



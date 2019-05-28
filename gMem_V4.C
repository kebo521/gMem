//====================================================================
/*****************************公共内存管理******************************
//功能-------  用于动态申请公共内存
//作者-------  邓国祖
//创作时间--20130604
//修改: ( 邓国祖) 20131213   --V2(重新整合释放的内存，起用回收机制)
//修改: ( 邓国祖) 20131213   --V3(增加内存申请快速切入口，
								     申请与释放的效率提高53.00%(V2-59037,V3-27755))
//修改: ( 邓国祖) 20140612   --V4(增加结构体向上的指针,释放内存速度大幅度提升)
******************************************************************************/

//-------------------内存链表结构----------------------------------------
typedef struct _DfFlagGuideRAM
{ //---将pNext放在中间的位位置避免越界风险-----------------------------   
//	u16 	Flag; 	//暂时不用，
	struct _DfFlagGuideRAM  *pPrev;	//上一结点，=NULL时空间已经释放
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
//输入数据:pGuideHead所申请区域的首地址(必须是4的整数倍地址),RamLen,可以使用的内存大小
//输出数据:无
//创作时间:  	20130604
//---------------------------------------------------------------
void InitGuideRAM(char* pGuideHead,unsigned RamLen)
{
//	TRACE("InitGuideRAM Sta[%x],Len[%d]\r\n",pGuideHead,RamLen);
	pAppRamGuideHead=pGuideHead;//(pGuideHead+3)&(~3);
	pAppRamGuideEnd=(pGuideHead+RamLen);
	//-------------快速处理------------
	pRamQuick=(DfFlagGuideRAM *)pAppRamGuideHead;
	pRamQuick->pNext=NULL;
	pRamQuick->pPrev=pRamQuick;	//第一条指向本身
}
//====================================================================
//功能:    用于申请固定内存功能，
//输入数据:silen 需要申请内存大小
//输出数据:所申请区域的首地址，申请失败时返回为空地址
//作者:     邓国祖	---	
//创作时间:  	20140612	-V4
//---------------------------------------------------------------
void *ApplyGuideRAM(unsigned silen)
{
	unsigned iLen;
	char *pRet;
	DfFlagGuideRAM *pGuideRAM,*pLink;
	//------申请长度整成4整数倍(避免指针偏移)--------------
//	if(!silen) return NULL;	//申请的数据长度不能为0
	iLen=sizeof(char*)-1;
	silen =(silen+iLen)&(~iLen); 
	//-----最快的入口-------	
	pGuideRAM=pRamQuick;
	//--清空好赋新值-------
	pRamQuick=NULL;	
	//-------------------------------------------------------
	while(pGuideRAM->pNext)	//pGuideRAM->pNext!=NULL
	{		
		if(!pGuideRAM->pPrev)//pGuideRAM->size == 0
		{//--------回收处理------
			iLen=(char*)pGuideRAM->pNext - pGuideRAM->MsgData;	//计算容量
			if(iLen >= silen)
			{
				//------遇到特殊情况，找回上一条指针---
				pLink=(DfFlagGuideRAM *)pAppRamGuideHead;
				if(pLink!=pGuideRAM)
				{
					while(pLink->pNext!=pGuideRAM)
						pLink=pLink->pNext;
				}
				//--------------------------------------------------
				pGuideRAM->pPrev = pLink;	//恢复向上指针，地址重新起用
				pRet= pGuideRAM->MsgData;	//申请到的地址指针
				if((iLen-silen) >= sizeof(DfFlagGuideRAM))
				{//---多于的空间，用于增加一个数据空间结点--------
					pLink=pGuideRAM->pNext;	//保存下指针，以便赋值给下一条的下指
					pGuideRAM->pNext=(DfFlagGuideRAM *)(pGuideRAM->MsgData+silen);	//重新赋值下指针，以便插入一个新的结点
					//-----新结点赋值------
					//pGuideRAM->pNext->pPrev = NULL; //赋值下一条向上指针
					//pGuideRAM->pNext->pNext = pLink;//赋值给下一条的下指,保存下指针
					//pLink->pPrev = pGuideRAM->pNext; //赋值下一条向上指针
					pGuideRAM = pGuideRAM->pNext;
					pGuideRAM->pNext = pLink;
					pGuideRAM->pPrev = NULL;
					pLink->pPrev = pGuideRAM; 
					if(!pRamQuick) 
						pRamQuick= pGuideRAM;	//新的快速入口，下一次申请就从这里开始
				}
				else 
				{
					if(!pRamQuick) 
						pRamQuick= pGuideRAM->pNext;	//新的快速入口，下一次申请就从这里开始
				}
				return (void*)pRet;
			}
			if(!pRamQuick) pRamQuick= pGuideRAM;	//新的快速入口，下一次申请就从这里开始				
		}
		pGuideRAM = pGuideRAM->pNext;
	}
	if((pGuideRAM->MsgData +silen) >= pAppRamGuideEnd)
	{
//		UI_ShowInfo("AppPE RAM OVER");
//		FreeCheckRAM();
		if(!pRamQuick) pRamQuick= pGuideRAM;	//新的快速入口，下一次申请就从这里开始				
		return NULL;
	}
	//--------New Ram-----
	pRet=pGuideRAM->MsgData;
	pGuideRAM->pNext=(DfFlagGuideRAM *)&pRet[silen];
	pGuideRAM->pNext->pPrev=pGuideRAM;	//赋值下一条向上指针
	pGuideRAM =pGuideRAM->pNext;//---指向下一条
	pGuideRAM->pNext=NULL;	//下一条制空
	if(!pRamQuick) pRamQuick= pGuideRAM;	//新的快速入口，下一次申请就从这里开始				
	return (void*)pRet;
}
//====================================================================
//功能:    用于释放之前申请的内存
//输入数据:pfree所申请区域的首地址，用于释放对应用的区域。
//输出数据:无
//作者:     邓国祖	---	
//创作时间:  	20140612	-V4
//---------------------------------------------------------------
void FreeGuideRAM(void *pfree)
{
	DfFlagGuideRAM *pGuideRAM,*pLink;
	//---------检查输入指针的有效果性---------------------------------------
	if((char*)pfree <pAppRamGuideHead || (char*)pfree >pAppRamGuideEnd) return;
	pGuideRAM=(DfFlagGuideRAM *)(((char*)pfree)-(sizeof(pGuideRAM->pNext)+sizeof(pGuideRAM->pPrev)));
	//---------检查指针结点的正确性，避免错误干扰---------------------------------------
	if(pGuideRAM->pNext<(DfFlagGuideRAM *)pAppRamGuideHead||pGuideRAM->pNext>(DfFlagGuideRAM *)pAppRamGuideEnd) return;
	if(pGuideRAM->pPrev<(DfFlagGuideRAM *)pAppRamGuideHead||pGuideRAM->pPrev> pGuideRAM->pNext) return;
	//--(pGuideRAM->pNext->pPrev==NULL||pGuideRAM->pNext->pNext==NULL)------------
	if((!pGuideRAM->pNext->pPrev)||(!pGuideRAM->pNext->pNext))
	{//------删除链表(向后)------------
		pGuideRAM->pNext=pGuideRAM->pNext->pNext;
		if(pGuideRAM->pNext) pGuideRAM->pNext->pPrev=pGuideRAM;
	}
	if(!pGuideRAM->pPrev->pPrev)//(pGuideRAM->pPrev->pPrev==NULL)
	{//------删除链表(向前)------------
		pGuideRAM=pGuideRAM->pPrev;
		pGuideRAM->pNext=pGuideRAM->pNext->pNext;

		if(pGuideRAM->pNext) pGuideRAM->pNext->pPrev=pGuideRAM;
		else 
		{
			//------遇到特殊情况，找回上一条指针---
			pLink=(DfFlagGuideRAM *)pAppRamGuideHead;
			if(pLink!=pGuideRAM)
			{
				while(pLink->pNext!=pGuideRAM)
					pLink=pLink->pNext;
			}
			pGuideRAM->pPrev=pLink;
		}
	}
	
	if(pGuideRAM->pNext) pGuideRAM->pPrev=NULL;	//不是最后一条不要删除上指针
	
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


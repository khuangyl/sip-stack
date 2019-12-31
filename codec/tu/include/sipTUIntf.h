#ifndef _SIP_TU_INTF_H
#define _SIP_TU_INTF_H


#include "osList.h"
#include "sipMsgRequest.h"
#include "sipMsgFirstLine.h" 



typedef enum {
	SIP_TU_MSG_TYPE_MESSAGE,
    SIP_TU_MSG_TYPE_TRANSACTION_ERROR,
} sipTUMsgType_e;


typedef struct sipTUMsg {
    sipMsgType_e sipMsgType;
    sipMsgBuf_t* pSipMsgBuf;		//contains raw sip buffer
	void* pTransId;
	void* pTUId;
#if 0
	osListElement_t* pTransHashId;	//transaction layer will pass this id to TU everytime sending a message to TU
    osListElement_t* pTUHashId;		//for a session, if transaction layer gets this Id from TU layer before, transaction layer will pass up to TU in the following messages.  This value is 0 if sipTrans does not have TU hashId value
#endif
} sipTUMsg_t;

typedef osStatus_e (*sipTUAppOnSipMsg_h)(sipTUMsgType_e msgType, sipTUMsg_t* pSipTUMsg);


//if sipTrans calls this function and get error response, it shall remove the transaction.  TU shall return OS_STATUS_OK even if it returns error response.  TU shall only return !OS_STATUS_OK if it can not habdle the SIP MESSAGE properly, like could not decode, memory error, etc. 
osStatus_e sipTU_onMsg(sipTUMsgType_e msgType, sipTUMsg_t* pMsg);
void sipTU_attach(sipTUAppOnSipMsg_h appOnSipMsg);

#endif

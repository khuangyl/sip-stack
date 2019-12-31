#include "osTimer.h"
#include "osHash.h"

#include "sipConfig.h"
#include "sipTransaction.h"
#include "sipMsgRequest.h"
#include "sipTransMgr.h"
#include "sipTransaction.h"
#include "sipTU.h"
#include "sipTransport.h"


static osStatus_e sipTransNISEnterState(sipTransState_e newState, sipTransMsgType_e msgType, sipTransaction_t* pTrans);
static osStatus_e sipTransNISStateNone_onMsg(sipTransMsgType_e msgType, void* pMsg, uint64_t timerId);
static osStatus_e sipTransNISStateTrying_onMsg(sipTransMsgType_e msgType, void* pMsg, uint64_t timerId);
static osStatus_e sipTransNISStateProceeding_onMsg(sipTransMsgType_e msgType, void* pMsg, uint64_t timerId);
static osStatus_e sipTransNISStateCompleted_onMsg(sipTransMsgType_e msgType, void* pMsg, uint64_t timerId);


osStatus_e sipTransNoInviteServer_onMsg(sipTransMsgType_e msgType, void* pMsg, uint64_t timerId)
{
	osStatus_e status = OS_STATUS_OK;

    if(!pMsg)
    {
        logError("null pointer, pMsg.");
		status = OS_ERROR_NULL_POINTER;
        goto EXIT;
    }

    sipTransaction_t* pTrans = pMsg;
    if(msgType == SIP_TRANS_MSG_TYPE_PEER || msgType == SIP_TRANS_MSG_TYPE_TU)
    {
        pTrans = ((sipTransMsg2SM_t*)pMsg)->pTrans;
    }

	if(!pTrans)
	{
		logError("null pointer, pTrans, for msgType (%d).", msgType);
		status = OS_ERROR_INVALID_VALUE;
		goto EXIT;
	}
		
    switch(pTrans->state)
	{
		case SIP_TRANS_STATE_NONE:
			status = sipTransNISStateNone_onMsg(msgType, pMsg, timerId);
			break;
        case SIP_TRANS_STATE_TRYING:
            status = sipTransNISStateTrying_onMsg(msgType, pMsg, timerId);
            break;
        case SIP_TRANS_STATE_PROCEEDING:
            status = sipTransNISStateProceeding_onMsg(msgType, pMsg, timerId);
            break;
        case SIP_TRANS_STATE_COMPLETED:
            status = sipTransNISStateCompleted_onMsg(msgType, pMsg, timerId);
            break;
        case SIP_TRANS_STATE_TERMINATED:
        default:
            logError("received a timeout event that has invalid transaction state (%d),", (sipTransaction_t*)pTrans->state);
			status = OS_ERROR_INVALID_VALUE;
            break;
    }

EXIT:
	return status;
}
	

osStatus_e sipTransNISStateNone_onMsg(sipTransMsgType_e msgType, void* pMsg, uint64_t timerId)
{
	osStatus_e status = OS_STATUS_OK;

    if(!pMsg)
    {
        logError("null pointer, pMsg.");
        status = OS_ERROR_NULL_POINTER;
        goto EXIT;
    }

	if(msgType != SIP_TRANS_MSG_TYPE_PEER)
	{
        logError("receive unexpected msgType (%d) in NISStateNone.", msgType);
        status = OS_ERROR_INVALID_VALUE;
		goto EXIT;
    }

	sipTransaction_t* pTrans = ((sipTransMsg2SM_t*) pMsg)->pTrans;
    if(!pTrans)
    {
        logError("null pointer, pTrans, for msgType (%d).", msgType);
        status = OS_ERROR_INVALID_VALUE;
        goto EXIT;
    }

//	sipTU_onMsg(msgType, pTrans);

	sipTransNISEnterState(SIP_TRANS_STATE_TRYING, msgType, pTrans);

EXIT:
	return status;
}


osStatus_e sipTransNISStateTrying_onMsg(sipTransMsgType_e msgType, void* pMsg, uint64_t timerId)
{
    osStatus_e status = OS_STATUS_OK;

    if(!pMsg)
    {
        logError("null pointer, pTrans.");
        status = OS_ERROR_NULL_POINTER;
        goto EXIT;
    }

	if(msgType != SIP_TRANS_MSG_TYPE_TU)
    {
        logError("receive unexpected msgType (%d) in NISStateTrying.", msgType);
        status = OS_ERROR_INVALID_VALUE;
		goto EXIT;
    }

	sipTransMsg2SM_t* pTU = pMsg;
    sipTransaction_t* pTrans = pTU->pTrans;
    if(!pTrans)
    {
        logError("null pointer, pTrans, for msgType (%d).", msgType);
        status = OS_ERROR_INVALID_VALUE;
        goto EXIT;
    }
 
	if(pTU->sipMsgType != SIP_TRANS_SIPMSG_RESPONSE)
	{
		logError("received unexpected request message from TU in NISStateTrying, response message is expected.");
        status = OS_ERROR_INVALID_VALUE;
        goto EXIT;
    }

	//pTrans->pResp shall be NULL before
	pTrans->pResp = osMem_ref(pTU->pSipMsg);

    sipTransport_send(SIP_TRANS_SIPMSG_RESPONSE, pTrans);

	sipResponse_e rspCode = pTU->pTransInfo->rspCode;
	if(rspCode >= 100 && rspCode <=199)
	{
        sipTransNISEnterState(SIP_TRANS_STATE_PROCEEDING, msgType, pTrans);
	}
    else if(rspCode >= 200 && rspCode <= 699)
    {
        sipTransNISEnterState(SIP_TRANS_STATE_COMPLETED, msgType, pTrans);
	}

EXIT:
	return status;
}


osStatus_e sipTransNISStateProceeding_onMsg(sipTransMsgType_e msgType, void* pMsg, uint64_t timerId)
{
    osStatus_e status = OS_STATUS_OK;

    if(!pMsg)
    {
        logError("null pointer, pTrans.");
        status = OS_ERROR_NULL_POINTER;
        goto EXIT;
    }

	switch (msgType)
	{
		case SIP_TRANS_MSG_TYPE_TU:
		{
			sipTransMsg2SM_t* pTU = pMsg;
    		if(pTU->sipMsgType != SIP_TRANS_SIPMSG_RESPONSE)
    		{
        		logError("received unexpected request message from TU in NISStateProceeding, response message is expected.");
        		status = OS_ERROR_INVALID_VALUE;
        		goto EXIT;
    		}

            sipTransaction_t* pTrans = pTU->pTrans;
		    if(!pTrans)
    		{
        		logError("null pointer, pTrans, for msgType (%d).", msgType);
        		status = OS_ERROR_INVALID_VALUE;
        		goto EXIT;
    		}

			osMem_deref(pTrans->pResp);
			pTrans->pResp = osMem_ref(pTU->pSipMsg);
            sipTransport_send(SIP_TRANS_SIPMSG_RESPONSE, pTrans);

    		sipResponse_e rspCode = pTU->pTransInfo->rspCode;
    		if(rspCode >= 200 && rspCode <= 699)
    		{
				sipTransNISEnterState(SIP_TRANS_STATE_COMPLETED, msgType, pTrans);
			}
			break;
 		}
		case SIP_TRANS_MSG_TYPE_PEER:
		{
            sipTransMsg2SM_t* pPeer = pMsg;
		    sipTransaction_t* pTrans = pPeer->pTrans;
            if(!pTrans)
            {
                logError("null pointer, pTrans, for msgType (%d).", msgType);
                status = OS_ERROR_INVALID_VALUE;
                goto EXIT;
            }

        	if(pPeer->sipMsgType != SIP_TRANS_SIPMSG_REQUEST)
        	{
            	logError("received unexpected response message from peer in NISStateProcessing, request message is expected.");
            	status = OS_ERROR_INVALID_VALUE;
            	goto EXIT;
			}		

			//resend the response
			sipTransport_send(SIP_TRANS_SIPMSG_RESPONSE, pTrans);
			break;
		}
		case SIP_TRANS_MSG_TYPE_TX_FAILED:
		{
			sipTransaction_t* pTrans = pMsg;
            if(!pTrans)
            {
                logError("null pointer, pTrans, for msgType (%d).", msgType);
                status = OS_ERROR_INVALID_VALUE;
                goto EXIT;
            }

			sipTransNISEnterState(SIP_TRANS_STATE_TERMINATED, msgType, pTrans);
			break;
		}
		case SIP_TRANS_MSG_TYPE_TIMEOUT:
		default:
			logError("received unexpected mesType (%d) in sipTransNISStateProceeding, ignore.", msgType);
			status = OS_ERROR_INVALID_VALUE;
			goto EXIT;
			break;	
	}


EXIT:
	return status;
}


osStatus_e sipTransNISStateCompleted_onMsg(sipTransMsgType_e msgType, void* pMsg, uint64_t timerId)
{
    osStatus_e status = OS_STATUS_OK;

    if(!pMsg)
    {
        logError("null pointer, pTrans.");
        status = OS_ERROR_NULL_POINTER;
        goto EXIT;
    }

    switch (msgType)
    {
        case SIP_TRANS_MSG_TYPE_TU:
		{
            sipTransMsg2SM_t* pTU = pMsg;
            if(pTU->sipMsgType != SIP_TRANS_SIPMSG_RESPONSE)
            {
                logError("received unexpected request message from TU in NISStateCompleted, response message is expected.");
                status = OS_ERROR_INVALID_VALUE;
                goto EXIT;
            }

            sipTransaction_t* pTrans = pTU->pTrans;
            if(!pTrans)
            {
                logError("null pointer, pTrans, for msgType (%d).", msgType);
                status = OS_ERROR_INVALID_VALUE;
                goto EXIT;
            }

			logInfo("received a response from TU in NISStateCompleted, ignore.");
			break;
		}
        case SIP_TRANS_MSG_TYPE_PEER:
		{
            sipTransMsg2SM_t* pPeer = pMsg;
            sipTransaction_t* pTrans = pPeer->pTrans;
            if(!pTrans)
            {
                logError("null pointer, pTrans, for msgType (%d).", msgType);
                status = OS_ERROR_INVALID_VALUE;
                goto EXIT;
            }

            if(pPeer->sipMsgType != SIP_TRANS_SIPMSG_REQUEST)
            {
                logError("received unexpected response message from peer in NISStateCompleted, request message is expected.");
                status = OS_ERROR_INVALID_VALUE;
                goto EXIT;
            }

            //resend the response
            sipTransport_send(SIP_TRANS_SIPMSG_RESPONSE, pTrans);
            break;
		}
        case SIP_TRANS_MSG_TYPE_TX_FAILED:
            sipTransNISEnterState(SIP_TRANS_STATE_TERMINATED, msgType, (sipTransaction_t*)pMsg);
            break;
        case SIP_TRANS_MSG_TYPE_TIMEOUT:
		{
            sipTransaction_t* pTrans = pMsg;
            if(!pTrans)
            {
                logError("null pointer, pTrans, for msgType (%d).", msgType);
                status = OS_ERROR_INVALID_VALUE;
                goto EXIT;
            }

			if(pTrans->sipTransNISTimer.timerIdJ != timerId)
			{
            	logError("received a timerId (%d) that is not equal to timerJ (%d), ignore.", timerId, pTrans->sipTransNISTimer.timerIdJ);
            	status = OS_ERROR_INVALID_VALUE;
            	goto EXIT;
			}

			sipTransNISEnterState(SIP_TRANS_STATE_TERMINATED, msgType, pTrans);
            break;
		}
		default:
			logError("received a unexpected msgType (%d), ignore.", msgType);
			break;
    }

EXIT:
    return status;
}


osStatus_e sipTransNISEnterState(sipTransState_e newState, sipTransMsgType_e msgType, sipTransaction_t* pTrans)
{
	osStatus_e status = OS_STATUS_OK;

	switch (newState)
	{
		case SIP_TRANS_STATE_TRYING:
            sipTU_onMsg(msgType, pTrans);
			pTrans->state = newState;
			break;
		case SIP_TRANS_STATE_PROCEEDING:
			pTrans->state = newState;
			break;
		case SIP_TRANS_STATE_COMPLETED:
			pTrans->state = newState;
			if(pTrans->isUDP)
			{
				sipTransStartTimer(SIP_TIMER_J, pTrans);
			}
			else
			{
				sipTransNISEnterState(SIP_TRANS_STATE_TERMINATED, SIP_TRANS_MSG_TYPE_TIMEOUT, pTrans);
			}
			break;
		case SIP_TRANS_STATE_TERMINATED:
			switch(pTrans->state)
			{
				case SIP_TRANS_STATE_TRYING:
					//something wrong, notify UT, treat the error as a transport error
					sipTU_onMsg(SIP_TRANS_MSG_TYPE_TX_FAILED, pTrans);
				case SIP_TRANS_STATE_PROCEEDING:
					//transport error
					sipTU_onMsg(SIP_TRANS_MSG_TYPE_TX_FAILED, pTrans);
					break;
				case SIP_TRANS_STATE_COMPLETED:
					if(msgType == SIP_TRANS_MSG_TYPE_TX_FAILED)
					{
						//transport error
	                    sipTU_onMsg(SIP_TRANS_MSG_TYPE_TX_FAILED, pTrans);
					}
					break;
				default:
					logError("unexpected NIS state for NIS (%d).", pTrans->state);
					status = OS_ERROR_INVALID_VALUE;
					break;
			}

			pTrans->state = newState;
	
			//clean up the transaction.
            if(pTrans->sipTransNISTimer.timerIdJ != 0)
            {
                osStopTimer(pTrans->sipTransNISTimer.timerIdJ);
                pTrans->sipTransNISTimer.timerIdJ = 0;
            }

			osHash_deleteElement(pTrans->pTransHashLE);
			osHashData_t* pHashData = pTrans->pTransHashLE->data;
			osMem_deref((sipTransaction_t*)pHashData->pData);
			free(pTrans->pTransHashLE);

			break;
		default:
			logError("received unexpected newState (%d).", newState);
            status = OS_ERROR_INVALID_VALUE;
			break;
	}

EXIT:
	return status;
}	


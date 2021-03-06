/********************************************************
 * Copyright (C) 2019,2020, Sean Dai
 *
 * @file scscfIfc.c
 * implement 3GPP 29.228 IFC parsing
 *
 * an example ifc
 * <SharedIFCSet>
 *     <SharedIFCSetID>xx</SharedIFCSetID>
 *     <InitialFilterCriteria>
 *        <Priority>0</Priority>
 *        <TriggerPoint>
 *        </TriggerPoint>
 *        <ApplicationServer>
 *        </ApplicationServer>
 *    </InitialFilterCriteria>
 *    <InitialFilterCriteria>
 *    </InitialFilterCriteria>
 * </SharedIFCSet>
 * <SharedIFCSet>
 * ...
 * </SharedIFCSet>
 *
 * CNF == true: (a or b or c) and (d or e)
 * CNF == false: (a and b and c) or (d and e)
 ********************************************************/


#include "osMBuf.h"
#include "osConfig.h"

#include "sipMsgFirstLine.h"
#include "sipHeaderData.h"
#include "sipHeader.h"

#include "scscfIfc.h"
#include "scscfRegistrar.h"



typedef enum {
	SCSCF_IFC_DEFAULT_HANDLING_SESS_CONTINUED = 0,
	SCSCF_IFC_DEFAULT_HANDLING_SESS_TERMINATED = 1,
}scscfIfcDefaultHandling_e;


typedef enum {
	SCSCF_IFC_SPT_TYPE_METHOD,
	SCSCF_IFC_SPT_TYPE_REQUEST_URI,
	SCSCF_IFC_SPT_TYPE_SESSION_CASE,
	SCSCF_IFC_SPT_TYPE_HEADER,
} scscfIfcSptType_e;


typedef struct {
	sipHdrName_e hdrName;
	osPointerLen_t hdrContent;
} ifcSptHdr_t;

typedef struct {
	scscfIfcSptType_e sptType;
	bool isConditionNegated;
	union {
		osPointerLen_t reqUri;
		sipRequest_e method;
		ifcSptHdr_t header;
		scscfIfcSessCase_e sessCase;
	};
} scscfIfcSptInfo_t;


typedef struct {
	osList_t sptGrp;	//each element contains a scscfIfcSptInfo_t.  all element in the sptGrp has the same group value
} scscfIfcSptGrp_t;
		
typedef struct {
    uint32_t sIfcGrpId; 	//a sifc group id, multiple sIfc may share one group Id.  This is NOT part of IFC
	bool conditionTypeCNF;
	uint32_t priority;
	osList_t sptGrpList;	//each entry contains a scscfIfcSptGrp_t
	osPointerLen_t asName;
	bool isDefaultSessContinued;	//default handling
} scscfIfc_t;	


#if 0
typedef struct {
	uint32_t sIfcGrpId;	//a sifc group id, multiple sIfc may share one group Id
	scscfIfc_t sIfc;		
} scscfIfcInfo_t;
#endif

typedef enum {
	SCSCF_IFC_SPT,
	SCSCF_IFC_Group,
	SCSCF_IFC_Method,
	SCSCF_IFC_Header,
	SCSCF_IFC_Content,
	SCSCF_IFC_Priority,
	SCSCF_IFC_SIPHeader,
	SCSCF_IFC_RequestURI,
	SCSCF_IFC_ServerName,
	SCSCF_IFC_SessionCase,
    SCSCF_IFC_TriggerPoint,
	SCSCF_IFC_SharedIFCSetID,
	SCSCF_IFC_DefaultHandling,
	SCSCF_IFC_ConditionNegated,
	SCSCF_IFC_ConditionTypeCNF,	
	SCSCF_IFC_InitialFilterCriteria,
    SCSCF_IFC_XML_MAX_DATA_NAME_NUM,
} scscfIfc_xmlDataName_e;


osXmlData_t scscfIfc_xmlData[SCSCF_IFC_XML_MAX_DATA_NAME_NUM] = {
    {SCSCF_IFC_SPT,		{"SPT", sizeof("SPT")-1}, OS_XML_DATA_TYPE_COMPLEX, true},
    {SCSCF_IFC_Group,	{"Group", sizeof("Group")-1}, OS_XML_DATA_TYPE_SIMPLE, false},
    {SCSCF_IFC_Method,	{"Method", sizeof("Method")-1}, OS_XML_DATA_TYPE_SIMPLE, false},
    {SCSCF_IFC_Header,	{"Header", sizeof("Header")-1}, OS_XML_DATA_TYPE_SIMPLE, false},
    {SCSCF_IFC_Content,	{"Content", sizeof("Content")-1}, OS_XML_DATA_TYPE_SIMPLE, false},
    {SCSCF_IFC_Priority,	{"Priority", sizeof("Priority")-1}, OS_XML_DATA_TYPE_SIMPLE, false},
    {SCSCF_IFC_SIPHeader,	{"SIPHeader", sizeof("SIPHeader")-1}, OS_XML_DATA_TYPE_COMPLEX, true},
    {SCSCF_IFC_RequestURI,	{"RequestURI", sizeof("RequestURI")-1}, OS_XML_DATA_TYPE_SIMPLE, false},
    {SCSCF_IFC_ServerName,	{"ServerName", sizeof("ServerName")-1}, OS_XML_DATA_TYPE_SIMPLE, false},
    {SCSCF_IFC_SessionCase,	{"SessionCase", sizeof("SessionCase")-1}, OS_XML_DATA_TYPE_SIMPLE, false},
    {SCSCF_IFC_TriggerPoint,	{"TriggerPoint", sizeof("TriggerPoint")-1}, OS_XML_DATA_TYPE_COMPLEX, true},
	{SCSCF_IFC_SharedIFCSetID,	{"SharedIFCSetID", sizeof("SharedIFCSetID")-1}, OS_XML_DATA_TYPE_SIMPLE, false},
    {SCSCF_IFC_DefaultHandling,	{"DefaultHandling", sizeof("DefaultHandling")-1}, OS_XML_DATA_TYPE_SIMPLE, false},
    {SCSCF_IFC_ConditionNegated,	{"ConditionNegated", sizeof("ConditionNegated")-1}, OS_XML_DATA_TYPE_SIMPLE, false},
    {SCSCF_IFC_ConditionTypeCNF,	{"ConditionTypeCNF", sizeof("ConditionTypeCNF")-1}, OS_XML_DATA_TYPE_SIMPLE, false},
    {SCSCF_IFC_InitialFilterCriteria, {"InitialFilterCriteria", sizeof("InitialFilterCriteria")-1}, OS_XML_DATA_TYPE_COMPLEX, true},
}; 


static void scscfIfc_parseSIfcCB(osXmlData_t* pXmlValue, void* nsInfo, void* appData);
static bool scscfIfcSptGrpIsMatch(scscfIfcSptGrp_t* pSptGrp, bool isOr, sipMsgDecodedRawHdr_t* pReqDecodedRaw, scscfIfcEvent_t* pIfcEvent);
static bool scscfIfc_isIdMatch(sIfcIdList_t* pSIfcIdList, uint32_t sIfcGrpId);
static bool scscfIfc_sortPriority(osListElement_t *le1, osListElement_t *le2, void *arg);


static osList_t gSIfcSet;	//each entry contains a scscfIfc_t
static osMBuf_t* gSIfcXmlBuf = NULL;	//shall be freed/replaced when ifcXml changes



osStatus_e scscfIfc_init(char* sIfcFileFolder, char* sIfcXsdFileName, char* sIfcXmlFileName)
{
    osStatus_e status = OS_STATUS_OK;
	osMBuf_t* sIfcXsdBuf = NULL;

    if(!sIfcXsdFileName || !sIfcXmlFileName)
    {
        logError("null pointer, sIfcXsdFileName=%p, sIfcXmlFileName=%p.", sIfcXsdFileName, sIfcXmlFileName);
        status = OS_ERROR_NULL_POINTER;
        goto EXIT;
    }

    char sIfcXmlFile[OS_MAX_FILE_NAME_SIZE], sIfcXsdFile[OS_MAX_FILE_NAME_SIZE];

    if(snprintf(sIfcXmlFile, OS_MAX_FILE_NAME_SIZE, "%s/%s", sIfcFileFolder ? sIfcFileFolder : ".", sIfcXmlFileName) >= OS_MAX_FILE_NAME_SIZE)
    {
        logError("sIfcXmlFile name is truncated.");
        status = OS_ERROR_INVALID_VALUE;
    }

	int xsdFileNameSize = snprintf(sIfcXsdFile, OS_MAX_FILE_NAME_SIZE, "%s/%s", sIfcFileFolder ? sIfcFileFolder : ".", sIfcXsdFileName);
    if(xsdFileNameSize >= OS_MAX_FILE_NAME_SIZE)
    {
        logError("sIfcXsdFile name is truncated.");
        status = OS_ERROR_INVALID_VALUE;
    }

    //8000 is the initial mBuf size.  If the reading needs more than 8000, the function will realloc new memory
    gSIfcXmlBuf = osMBuf_readFile(sIfcXmlFile, 8000);
    if(!gSIfcXmlBuf)
    {
        logError("read gSIfcXmlBuf fails, sIfcXmlFile=%s", sIfcXmlFile);
        status = OS_ERROR_INVALID_VALUE;
        goto EXIT;
    }

    sIfcXsdBuf = osMBuf_readFile(sIfcXsdFile, 8000);
    if(!sIfcXsdBuf)
    {
        logError("read sIfcXsdBuf fails, sIfcXsdFile=%s", sIfcXsdFile);
        status = OS_ERROR_INVALID_VALUE;
        goto EXIT;
    }

	osPointerLen_t xsdName = {sIfcXsdFile, xsdFileNameSize};
    osXmlDataCallbackInfo_t cbInfo = {true, false, false, scscfIfc_parseSIfcCB, &gSIfcSet, scscfIfc_xmlData, SCSCF_IFC_XML_MAX_DATA_NAME_NUM};
	status = osXml_parse(gSIfcXmlBuf, sIfcXsdBuf, &xsdName, &cbInfo);

    //osXml_getElemValue(&xsdName, NULL, xmlMBuf, false, &cbInfo);

EXIT:
	if(status != OS_STATUS_OK)
	{
		osfree(gSIfcXmlBuf);
	}

	//needs to free sIfcXsdBuf after the parsing is done cause it is useless in the application layer.  xml module may refer it if it wants to keep it.
	//shall not free gSIfcXmlBuf, as some data structure (osPL_t) in the callback will refer it.  As long as th application wants to use the data structure 
	//in th callback, it shall keep it.
	osfree(sIfcXsdBuf);
	return status;
}



osPointerLen_t* scscfIfc_getNextAS(osListElement_t** ppLastSIfc, sIfcIdList_t* pSIfcIdList, sipMsgDecodedRawHdr_t* pReqDecodedRaw, scscfIfcEvent_t* pIfcEvent)
{
	osPointerLen_t* pAs = NULL;
	if(!ppLastSIfc || !pSIfcIdList)
	{
		logError("null pointer, ppLastSIfc=%p, pSIfcIdList=%p.", ppLastSIfc, pSIfcIdList);
		goto EXIT;
	}

	osListElement_t* pLE = *ppLastSIfc ? (*ppLastSIfc)->next : gSIfcSet.head;
	while(pLE)
	{
		*ppLastSIfc = pLE;

		scscfIfc_t* pIfc = pLE->data;
		if(!scscfIfc_isIdMatch(pSIfcIdList, pIfc->sIfcGrpId))
		{
			pLE = pLE->next;
			continue;
		}

		//if last As failed, and the default handling is terminated, stop here.
		if(!pIfcEvent->isLastAsOK && !pIfc->isDefaultSessContinued)
		{
			goto EXIT;
		}

		osListElement_t* pSptGrpLE = pIfc->sptGrpList.head;
		while(pSptGrpLE)
		{
			if(pIfc->conditionTypeCNF)
			{
				if(!scscfIfcSptGrpIsMatch(pSptGrpLE->data, true, pReqDecodedRaw, pIfcEvent))
				{
					break;
				}
			}
			else
			{
				if(scscfIfcSptGrpIsMatch(pSptGrpLE->data, false, pReqDecodedRaw, pIfcEvent))
				{
					pAs = &pIfc->asName;
					goto EXIT;
				}
			}

			pSptGrpLE = pSptGrpLE->next;
		}

		if(pIfc->conditionTypeCNF && !pSptGrpLE)
		{
			pAs = &pIfc->asName;
			goto EXIT;
		}

		pLE = pLE->next;
	}

EXIT:
	return pAs;
}


static bool scscfIfcSptGrpIsMatch(scscfIfcSptGrp_t* pSptGrp, bool isOr, sipMsgDecodedRawHdr_t* pReqDecodedRaw, scscfIfcEvent_t* pIfcEvent)
{
	bool isMatch = true;

	osListElement_t* pLE = pSptGrp->sptGrp.head;
	if(isOr)
	{
		while(pLE)
		{
			scscfIfcSptInfo_t* pSpt = pLE->data;
			switch(pSpt->sptType)
			{
				case SCSCF_IFC_SPT_TYPE_METHOD:
					if((pSpt->method == pIfcEvent->sipMethod && !pSpt->isConditionNegated) || (pSpt->method != pIfcEvent->sipMethod && pSpt->isConditionNegated))
					{
						isMatch = true;
						goto EXIT;
					}
					break;			
				case SCSCF_IFC_SPT_TYPE_REQUEST_URI:
				case SCSCF_IFC_SPT_TYPE_SESSION_CASE:
					if((pSpt->sessCase == pIfcEvent->sessCase && !pSpt->isConditionNegated) || (pSpt->sessCase != pIfcEvent->sessCase && pSpt->isConditionNegated))
					{
						isMatch = true;
                        goto EXIT;
                    }
					break;
				case SCSCF_IFC_SPT_TYPE_HEADER:
				default:
					break;
			}

			pLE = pLE->next;
		}
		isMatch = false;
	}
	else
	{
        while(pLE)
        {
            scscfIfcSptInfo_t* pSpt = pLE->data;
            switch(pSpt->sptType)
            {
                case SCSCF_IFC_SPT_TYPE_METHOD:
                    if((pSpt->method == pIfcEvent->sipMethod && pSpt->isConditionNegated) || (pSpt->method != pIfcEvent->sipMethod && !pSpt->isConditionNegated))
                    {
                        isMatch = false;
                        goto EXIT;
                    }
                    break;
                case SCSCF_IFC_SPT_TYPE_REQUEST_URI:
                case SCSCF_IFC_SPT_TYPE_SESSION_CASE:
                    if((pSpt->sessCase == pIfcEvent->sessCase && pSpt->isConditionNegated) || (pSpt->sessCase != pIfcEvent->sessCase && !pSpt->isConditionNegated))
                    {
                        isMatch = false;
                        goto EXIT;
                    }
                    break;
                case SCSCF_IFC_SPT_TYPE_HEADER:
                default:
                    break;
            }

            pLE = pLE->next;
        }
        isMatch = true;
    }

EXIT:
	return isMatch;
}
		

static void scscfIfc_parseSIfcCB(osXmlData_t* pXmlValue, void* nsInfo, void* appData)
{
    if(!pXmlValue)
    {
        logError("null pointer, pXmlValue.");
        return;
    }

	osList_t* pSIfcSet = appData;

	static __thread uint32_t fsIfcGrpId = 0; 
    static __thread scscfIfc_t* fpIfc = NULL;
	static __thread scscfIfcSptGrp_t* fpCurSptGrp = NULL;
	static __thread scscfIfcSptInfo_t* fpCurSpt = NULL;
	static __thread ifcSptHdr_t* fpCurHdr = NULL;
	static __thread int fCurGrp = -1;

    switch(pXmlValue->eDataName)
    {
		case SCSCF_IFC_SharedIFCSetID:
			fsIfcGrpId = pXmlValue->xmlInt;
			break;	
		case SCSCF_IFC_InitialFilterCriteria:
			if(pXmlValue->isEOT)
			{
				fpIfc->sIfcGrpId = fsIfcGrpId;
				osList_append(pSIfcSet, fpIfc);
				fpIfc = NULL;
				return;
			}
			if(fpIfc)
			{
				logError("starts to parse a new ifc, but the previous ifc has not closed.");
				return;
			}
			fpIfc = oszalloc(sizeof(scscfIfc_t), NULL);
			fpIfc->isDefaultSessContinued = true;
			break;
		case SCSCF_IFC_Priority:
			if(!fpIfc)
			{
				logError("fpIfc is NULL for SCSCF_IFC_Priority.");
				return;
			}
			fpIfc->priority = pXmlValue->xmlInt;
			break;
		case SCSCF_IFC_ConditionTypeCNF:
			if(!fpIfc)
            {
                logError("fpIfc is NULL for SCSCF_IFC_ConditionTypeCNF.");
                return;
            }
			fpIfc->conditionTypeCNF = pXmlValue->xmlInt;
			break;
		case SCSCF_IFC_SPT:
			if(pXmlValue->isEOT)
			{
				osList_append(&fpCurSptGrp->sptGrp, fpCurSpt);
				fpCurSpt = NULL;
			}
			else
			{
				fpCurSpt = osmalloc(sizeof(scscfIfcSptInfo_t), NULL);
			}
			break;
		case SCSCF_IFC_ConditionNegated:
			fpCurSpt->isConditionNegated = pXmlValue->xmlInt > 0 ? true : false;			
			break;
		case SCSCF_IFC_Group:
			//assume all SPTs with the same group value are continuous
			if(fCurGrp != pXmlValue->xmlInt)
			{
				if(fpCurSptGrp)
				{
					osList_append(&fpIfc->sptGrpList, fpCurSptGrp);
				}
				fpCurSptGrp = oszalloc(sizeof(scscfIfcSptGrp_t), NULL);
				fCurGrp = pXmlValue->xmlInt;
			}
			break;
		case SCSCF_IFC_TriggerPoint:
            if(pXmlValue->isEOT)
            {
				if(fpCurSptGrp)
				{
					osList_append(&fpIfc->sptGrpList, fpCurSptGrp);
				}
			}
			break;
		case SCSCF_IFC_Method:
			fpCurSpt->sptType = SCSCF_IFC_SPT_TYPE_METHOD;
			fpCurSpt->method =  sipMsg_method2Code(&pXmlValue->xmlStr);
			break;
		case SCSCF_IFC_SIPHeader:
			if(pXmlValue->isEOT)
			{
				fpCurHdr = NULL;
			}
			else
			{
				fpCurSpt->sptType = SCSCF_IFC_SPT_TYPE_HEADER;
				fpCurHdr = &fpCurSpt->header;
			}
			break;
		case SCSCF_IFC_Header:
			fpCurHdr->hdrName = sipHdr_getNameCode(&pXmlValue->xmlStr);
			break;
		case SCSCF_IFC_Content:
			fpCurHdr->hdrContent = pXmlValue->xmlStr;
			break;
		case SCSCF_IFC_RequestURI:
			fpCurSpt->sptType = SCSCF_IFC_SPT_TYPE_REQUEST_URI;
			fpCurSpt->reqUri = pXmlValue->xmlStr;
			break;
		case SCSCF_IFC_SessionCase:
			fpCurSpt->sptType = SCSCF_IFC_SPT_TYPE_SESSION_CASE;
			fpCurSpt->sessCase = pXmlValue->xmlInt >= SCSCF_IFC_SESS_CASE_ORIGINATING && pXmlValue->xmlInt <= SCSCF_IFC_SESS_CASE_ORIG_CDIV ? pXmlValue->xmlInt : SCSCF_IFC_SESS_CASE_INVALID;
			break;
		case SCSCF_IFC_ServerName:
			fpIfc->asName = pXmlValue->xmlStr;
			break;
		case SCSCF_IFC_DefaultHandling:
			fpIfc->isDefaultSessContinued = pXmlValue->xmlInt == SCSCF_IFC_DEFAULT_HANDLING_SESS_CONTINUED ? true : false;
			break;
		default:
			mlogInfo(LM_CSCF, "pXmlValue->eDataName(%d) is not processed.", pXmlValue->eDataName);
			break;
	}

	//sort ifc based on priority
	osList_sort(&gSIfcSet, scscfIfc_sortPriority, NULL);
 
	return;
}



scscfIfcRegType_e scscfIfc_mapSar2IfcRegType(scscfRegSarRegType_e sarRegType)
{
	switch(sarRegType)
	{
    	case SCSCF_REG_SAR_RE_REGISTER:
			return SCSCF_IFC_REG_TYPE_RE_REG;
			break;
    	case SCSCF_REG_SAR_DE_REGISTER:
			return SCSCF_IFC_REG_TYPE_DE_REG;
			break;
        case SCSCF_REG_SAR_REGISTER:
    	case SCSCF_REG_SAR_UN_REGISTER:
        case SCSCF_REG_SAR_INVALID:
		default:
			break;
	}

	return SCSCF_IFC_REG_TYPE_INITIAL_REG;
}


static bool scscfIfc_isIdMatch(sIfcIdList_t* pSIfcIdList, uint32_t sIfcGrpId)
{
	for(int i=0; i<pSIfcIdList->sIfcIdNum; i++)
	{
		if(pSIfcIdList->sIfcId[i] == sIfcGrpId)
		{
			return true;
		}
	}

	return false;
}


static bool scscfIfc_sortPriority(osListElement_t *le1, osListElement_t *le2, void *arg)
{
    bool isSwitch = false;
	scscfIfc_t* pIfc1 = le1->data;
	scscfIfc_t* pIfc2 = le2->data;

    if(pIfc1->priority > pIfc2->priority)
    {
        isSwitch = true;
        goto EXIT;
    }

EXIT:
    return isSwitch;
}


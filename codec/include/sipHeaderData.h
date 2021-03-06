/********************************************************
 * Copyright (C) 2019,2020, Sean Dai
 *
 * @file sipHeaderData.h
 ********************************************************/

#ifndef _SIP_HEADER_DATA_H
#define _SIP_HEADER_DATA_H


typedef enum sipHdrName {
	SIP_HDR_NONE,
    SIP_HDR_ACCEPT,
    SIP_HDR_ACCEPT_CONTACT,
    SIP_HDR_ACCEPT_ENCODING,
    SIP_HDR_ACCEPT_LANGUAGE,
    SIP_HDR_ACCEPT_RESOURCE_PRIORITY,
    SIP_HDR_ALERT_INFO,
    SIP_HDR_ALLOW,
    SIP_HDR_ALLOW_EVENTS,
    SIP_HDR_ANSWER_MODE,
    SIP_HDR_AUTHENTICATION_INFO,
    SIP_HDR_AUTHORIZATION,
    SIP_HDR_CALL_ID,
    SIP_HDR_CALL_INFO,
    SIP_HDR_CONTACT,
    SIP_HDR_CONTENT_DISPOSITION,
    SIP_HDR_CONTENT_ENCODING,
    SIP_HDR_CONTENT_LANGUAGE,
    SIP_HDR_CONTENT_LENGTH,
    SIP_HDR_CONTENT_TYPE,
    SIP_HDR_CONTRIBUTION_ID,
    SIP_HDR_CONVERSATION_ID,
    SIP_HDR_CSEQ,
    SIP_HDR_DATE,
    SIP_HDR_ENCRYPTION,
    SIP_HDR_ERROR_INFO,
    SIP_HDR_EVENT,
    SIP_HDR_EXPIRES,
    SIP_HDR_FLOW_TIMER,
    SIP_HDR_FROM,
    SIP_HDR_HIDE,
    SIP_HDR_HISTORY_INFO,
    SIP_HDR_IDENTITY,
    SIP_HDR_IDENTITY_INFO,
    SIP_HDR_IN_REPLY_TO,
    SIP_HDR_JOIN,
    SIP_HDR_MAX_BREADTH,
    SIP_HDR_MAX_FORWARDS,
    SIP_HDR_MIME_VERSION,
    SIP_HDR_MIN_EXPIRES,
    SIP_HDR_MIN_SE,
    SIP_HDR_ORGANIZATION,
    SIP_HDR_P_ACCESS_NETWORK_INFO,
    SIP_HDR_P_ANSWER_STATE,
    SIP_HDR_P_ASSERTED_IDENTITY,
    SIP_HDR_P_ASSOCIATED_URI,
    SIP_HDR_P_CALLED_PARTY_ID,
    SIP_HDR_P_CHARGING_FUNCTION_ADDRESSES,
    SIP_HDR_P_CHARGING_VECTOR,
    SIP_HDR_P_DCS_TRACE_PARTY_ID,
    SIP_HDR_P_DCS_OSPS,
    SIP_HDR_P_DCS_BILLING_INFO,
    SIP_HDR_P_DCS_LAES,
    SIP_HDR_P_DCS_REDIRECT,
    SIP_HDR_P_EARLY_MEDIA,
    SIP_HDR_P_MEDIA_AUTHORIZATION,
    SIP_HDR_P_PREFERRED_IDENTITY,
    SIP_HDR_P_PROFILE_KEY,
    SIP_HDR_P_REFUSED_URI_LIST,
    SIP_HDR_P_SERVED_USER,
    SIP_HDR_P_USER_DATABASE,
    SIP_HDR_P_VISITED_NETWORK_ID,
    SIP_HDR_PATH,
    SIP_HDR_PERMISSION_MISSING,
    SIP_HDR_PRIORITY,
    SIP_HDR_PRIV_ANSWER_MODE,
    SIP_HDR_PRIVACY,
    SIP_HDR_PROXY_AUTHENTICATE,
    SIP_HDR_PROXY_AUTHORIZATION,
    SIP_HDR_PROXY_REQUIRE,
    SIP_HDR_RACK,
    SIP_HDR_REASON,
    SIP_HDR_RECORD_ROUTE,
    SIP_HDR_REFER_SUB,
    SIP_HDR_REFER_TO,
    SIP_HDR_REFERRED_BY,
    SIP_HDR_REJECT_CONTACT,
    SIP_HDR_REPLACES,
    SIP_HDR_REPLY_TO,
    SIP_HDR_REQUEST_DISPOSITION,
    SIP_HDR_REQUIRE,
    SIP_HDR_RESOURCE_PRIORITY,
    SIP_HDR_RESPONSE_KEY,
    SIP_HDR_RETRY_AFTER,
    SIP_HDR_ROUTE,
    SIP_HDR_RSEQ,
    SIP_HDR_SECURITY_CLIENT,
    SIP_HDR_SECURITY_SERVER,
    SIP_HDR_SECURITY_VERIFY,
    SIP_HDR_SERVER,
    SIP_HDR_SERVICE_ROUTE,
    SIP_HDR_SESSION_EXPIRES,
    SIP_HDR_SIP_ETAG,
    SIP_HDR_SIP_IF_MATCH,
    SIP_HDR_SUBJECT,
    SIP_HDR_SUBSCRIPTION_STATE,
    SIP_HDR_SUPPORTED,
    SIP_HDR_TARGET_DIALOG,
    SIP_HDR_TIMESTAMP,
    SIP_HDR_TO,
    SIP_HDR_TRIGGER_CONSENT,
    SIP_HDR_UNSUPPORTED,
    SIP_HDR_USER_AGENT,
    SIP_HDR_VIA,
    SIP_HDR_WARNING,
    SIP_HDR_WWW_AUTHENTICATE,
	SIP_HDR_X,
	SIP_HDR_COUNT,
} sipHdrName_e;



#endif

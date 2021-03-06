/**
 * @file MA.c
 *
 * @brief MangementAgent
 *
 * Copyright (C) 2016. SK Telecom, All Rights Reserved.
 * Written 2016, by SK Telecom
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "MA.h"
#include "SRA.h"
#include "SMA.h"

#include "oneM2M.h"
#include "Configuration.h"
#ifdef ONEM2M_V1_12
#include "oneM2M_V1_12.h"
#else
#include "oneM2M_V1.h"
#endif

#include "SKTtpDebug.h"
#include "cmsis_os.h"

#ifdef ONEM2M_V1_12
#define TOPIC_SUBSCRIBE_REQ                 "/oneM2M/req/%s/%s"
#define TOPIC_SUBSCRIBE_RES                 "/oneM2M/resp/%s/%s"
#define TOPIC_SUBSCRIBE_SIZE                2
#define TOPIC_PUBLISH                       "/oneM2M/req/%s/%s"

#define TO_AE                               "%s/%s"
#define TO_CONTAINER                        "%s/%s/%s"

#define NAME_NODE                           "nod-middleware"
#define NAME_REMOTECSE                      "csr-middleware"
#define NAME_CONTAINER                      "cnt-sensor01"
#define NAME_MGMTCMD                        "mgc-reset"
#define NAME_LOCATIONPOLICY                 "lcp-middleware"
#define NAME_ACCESSCONTROLPOLICY            "acp-middleware"
#else
#define TO_REMOTECSE                        "%s/remoteCSE-%s"
#define TO_NODE                             "%s/node-%s"
#define TO_CONTAINER                        "%s/remoteCSE-%s/container-%s"
#define TO_CONTENTINSTANCE                  "%s/remoteCSE-%s/container-%s/contentInstance-%s"
#define TO_AREANWKINFO                      "%s/node-%s/areaNwkInfo-%s"
#define TO_AE                               "%s/remoteCSE-%s/AE-%s"
#define TO_LOCATIONPOLICY                   "%s/locationPolicy-%s"
#define TO_MGMTCMD                          "%s/mgmtCmd-%s"
#define TO_MGMTCMDRESULT                    "%s/mgmtCmd-%s/execInstance-%s"

#define TOPIC_SUBSCRIBE_REQ                 "/oneM2M/req/+/%s"
#define TOPIC_SUBSCRIBE_RES                 "/oneM2M/resp/%s/+"
#define TOPIC_SUBSCRIBE_SIZE                2
#define TOPIC_PUBLISH                       "/oneM2M/req/%s/ThingPlug"

#define NAME_CONTAINER                      "%s_container_01"

#define NAME_AREANWKINFO                    "%s_areaNwkInfo_01"
#define NAME_LOCATIONPOLICY                 "%s_locationPolicy_01"
#define NAME_AE                             "%s_AE_01"
#define NAME_POA                            "MQTT|%s"

#define NAME_MGMTCMD                        "%s_%s"
#endif

#define CMT_DEVRESET						"DevReset"
#define CMT_REPPERCHANGE					"RepPerChange"
#define CMT_REPIMMEDIATE					"RepImmediate"
#define CMT_TAKEPHOTO						"TakePhoto"
#define CMT_LEDCONTROL						"LEDControl"

#ifdef ONEM2M_V1_12
static char mAEID[128] = "";
static char mNodeLink[23] = "";
#else
static char mDeviceKey[128] = "";
static char mNodeLink[23] = "";
static char mHostCSELink[23] = "";
static char mRemoteCSEResourceName[128] = "";
static char mContentInstanceResourceName[128] = "";
static char mContainerResourceName[128] = "";
#endif

static enum PROCESS_STEP
{
    PROCESS_NODE_CREATE = 0,
    PROCESS_REMOTECSE_CREATE,
    PROCESS_AE_CREATE,    
    PROCESS_CONTAINER_CREATE,
    PROCESS_MGMTCMD_CREATE,
    PROCESS_CONTENTINSTANCE_CREATE,

    PROCESS_END
} mStep;


int CreateNode() {
	mStep = PROCESS_NODE_CREATE;
	int rc = -1;
#ifdef ONEM2M_V1_12
	rc = tp_v1_12_RegisterDevice(node, APP_AEID, ONEM2M_TO, ONEM2M_RI, NAME_NODE, "node_01", NULL, NULL, NULL);
#else
	char mga[128] = "";	
	snprintf(mga, sizeof(mga), NAME_POA, ONEM2M_NODEID);
	rc = tpRegisterDevice(node, ONEM2M_NODEID, ONEM2M_TO, ONEM2M_RI, mga, NULL, NULL, NULL, NULL, NULL);
#endif
	return rc;
}


int CreateRemoteCSE() {
	mStep = PROCESS_REMOTECSE_CREATE;
	int rc = -1;
	char to[512] = "";
	memcpy(to, ONEM2M_TO, strlen(ONEM2M_TO));	
#ifdef ONEM2M_V1_12
#else
	char poa[128] = ""; 	
	snprintf(poa, sizeof(poa), NAME_POA, ONEM2M_NODEID);
	rc = tpRegisterDevice(remoteCSE, ONEM2M_NODEID, ONEM2M_TO, ONEM2M_RI, NULL, ONEM2M_NODEID, "3", ONEM2M_PASSCODE, poa, mNodeLink);
#endif
	return rc;
}


int CreateAE() {
	mStep = PROCESS_AE_CREATE;
	int rc = -1;
#ifdef ONEM2M_V1_12
	char poa[128] = "";
	snprintf(poa, sizeof(poa), "mqtt:///oneM2M/req/%s/%s", ONEM2M_SERVICENAME, ONEM2M_AE_RESOURCENAME);
	rc = tp_v1_12_RegisterDevice(AE, "S", ONEM2M_TO, ONEM2M_RI, ONEM2M_AE_RESOURCENAME, NULL, NULL, poa, "middleware");
#else
#endif
	return rc;
}


int CreateContainer() {
	mStep = PROCESS_CONTAINER_CREATE;
	int rc = -1;
	char to[512] = "";
#ifdef ONEM2M_V1_12
	snprintf(to, sizeof(to), TO_AE, ONEM2M_TO, ONEM2M_AE_RESOURCENAME);
	rc = tp_v1_12_RegisterContainer(mAEID, to, ONEM2M_RI, NAME_CONTAINER);
#else
	snprintf(to, sizeof(to), TO_REMOTECSE, ONEM2M_TO, mRemoteCSEResourceName);
	char nm[128] = "";
	snprintf(nm, sizeof(nm), NAME_CONTAINER, ONEM2M_NODEID);
	rc = tpRegisterContainer(ONEM2M_NODEID, to, ONEM2M_RI, nm, mDeviceKey, "con");
#endif
	return rc;
}


int CreateMgmtCmd(char* cmt) {
	mStep = PROCESS_MGMTCMD_CREATE;
	int rc = -1;
#ifdef ONEM2M_V1_12
	rc = tp_v1_12_RegisterMgmtCmd(APP_AEID, ONEM2M_TO, ONEM2M_RI, NAME_MGMTCMD, "1", mNodeLink);
#else
	char nm[128] = "";
	snprintf(nm, sizeof(nm), NAME_MGMTCMD, ONEM2M_NODEID, cmt);
	rc = tpRegisterMgmtCmd(ONEM2M_NODEID, ONEM2M_TO, ONEM2M_RI, nm, mDeviceKey, cmt, "false", mNodeLink, ONEM2M_NODEID);
#endif
	return rc;
}


int CreateContentInstance() {
	mStep = PROCESS_CONTENTINSTANCE_CREATE;
	int rc = -1;
	char to[512] = "";
	char* cnf = "Lora/Sensor";
#ifdef ONEM2M_V1_12
	raw2tlv time;
	memset(&time, 0, sizeof(time));
 	SRADataConvert(IOT_GET_TIME_TLV, (void*) &time);
	tp_v1_12_AddData(time.tlv, strlen(time.tlv));
	free(time.tlv);
	char* sensorType = "temperature";
	char *output = NULL;
	SMAGetData(sensorType, &output);
	raw2tlv data;
	data.type = sensorType;
	data.value = output;
	SRADataConvert( IOT_RAW_TO_TLV, (void*) &data);
	tp_v1_12_AddData(data.tlv, strlen(data.tlv));
	free(data.tlv);
	free(output);
	
	sensorType = "humidity";
	SMAGetData(sensorType, &output);
	data.type = sensorType;
	data.value = output;
	SRADataConvert( IOT_RAW_TO_TLV, (void*) &data);
	tp_v1_12_AddData(data.tlv, strlen(data.tlv));
	free(data.tlv);
	free(output);

	sensorType = "light";
	SMAGetData(sensorType, &output);
	data.type = sensorType;
	data.value = output;
	SRADataConvert( IOT_RAW_TO_TLV, (void*) &data);
	tp_v1_12_AddData(data.tlv, strlen(data.tlv));
	free(data.tlv);
	free(output);

	sensorType = "motion";
	SMAGetData(sensorType, &output);
	data.type = sensorType;
	data.value = output;
	SRADataConvert( IOT_RAW_TO_TLV, (void*) &data);
	tp_v1_12_AddData(data.tlv, strlen(data.tlv));
	free(data.tlv);
	free(output);

	snprintf(to, sizeof(to), TO_CONTAINER, ONEM2M_TO, ONEM2M_AE_RESOURCENAME, NAME_CONTAINER);
	rc = tp_v1_12_Report(mAEID, to, ONEM2M_RI, cnf, NULL, 1);
#else
	raw2tlv time;
	memset(&time, 0, sizeof(time));
 	SRADataConvert(IOT_GET_TIME_TLV, (void*) &time);
	tpAddData(time.tlv, strlen(time.tlv));
	free(time.tlv);
	char* sensorType = "temperature";
	char *output = NULL;
	SMAGetData(sensorType, &output);
	raw2tlv data;
	data.type = sensorType;
	data.value = output;
	SRADataConvert( IOT_RAW_TO_TLV, (void*) &data);
	tpAddData(data.tlv, strlen(data.tlv));
	free(data.tlv);
	free(output);
	
	sensorType = "humidity";
	SMAGetData(sensorType, &output);
	data.type = sensorType;
	data.value = output;
	SRADataConvert( IOT_RAW_TO_TLV, (void*) &data);
	tpAddData(data.tlv, strlen(data.tlv));
	free(data.tlv);
	free(output);

	sensorType = "light";
	SMAGetData(sensorType, &output);
	data.type = sensorType;
	data.value = output;
	SRADataConvert( IOT_RAW_TO_TLV, (void*) &data);
	tpAddData(data.tlv, strlen(data.tlv));
	free(data.tlv);
	free(output);

	sensorType = "motion";
	SMAGetData(sensorType, &output);
	data.type = sensorType;
	data.value = output;
	SRADataConvert( IOT_RAW_TO_TLV, (void*) &data);
	tpAddData(data.tlv, strlen(data.tlv));
	free(data.tlv);
	free(output);

	snprintf(to, sizeof(to), TO_CONTAINER, ONEM2M_TO, mRemoteCSEResourceName, mContainerResourceName);
 	rc = tpReport(ONEM2M_NODEID, to, ONEM2M_RI, mDeviceKey, cnf, NULL, 1);
#endif
	return rc;
}


static int SimpleXmlParser(char* payload, char* name, char* value, int isPC) {
    int rc = 0;
    char start[10];
    char end[10];
    char* pl = payload;
    memset(start, 0, sizeof(start));
    memset(end, 0, sizeof(end));

    if(isPC) {
        pl = strstr(payload, "<pc>");
    }

    snprintf(start, sizeof(start), "<%s>", name);
    snprintf(end, sizeof(end), "</%s>", name);
    char* s = strstr(pl, start) + strlen(start);
    char* e = strstr(pl, end);

    if(s && e) {
        memcpy(value, s, e-s);
        SKTDebugPrint(LOG_LEVEL_INFO, "[%s : %s]", name, value);
    }    
    return rc;
}


static void SimpleJsonParser(char* jsonObject, char* value) {
	char* loc = jsonObject;
	loc = strstr(jsonObject, ":");
	if(!loc) return;
	loc = strstr(loc, "\"");
	if(!loc) return;
	loc += 1;
	char* e = strstr(loc, "\"");
	if(loc && e) {
		memcpy(value, loc, e-loc);
		SKTDebugPrint(LOG_LEVEL_INFO, "[cmd : %s]", value);
	}
}


static char* strnstr(char *s, char *text, size_t slen) {
	char c, sc;
	size_t len;

	if ((c = *text++) != '\0') {
		len = strlen(text);
		do {
			do {
				if ((sc = *s++) == '\0' || slen-- < 1)
					return (NULL);
			} while (sc != c);
			if (len > slen)
				return (NULL);
		} while (strncmp(s, text, len) != 0);
		s--;
	}
	return ((char *)s);
}


static char* IsCMD(char* topic) {
	return strnstr(topic, "req", 12);
}


static void ProcessCMD(char* payload, int payloadLen) {
	SKTDebugPrint(LOG_LEVEL_INFO, "ProcessCMD payload : %.*s", payloadLen, payload);
	char cmt[128] = "";
	char exra[128] = "";
	char resourceid[23] = "";
	char value[10] = "";
	SimpleXmlParser(payload, ATTR_CMT, cmt, 1);
	SimpleXmlParser(payload, ATTR_EXRA, exra, 1);
	SimpleXmlParser(payload, ATTR_RI, resourceid, 1);
	SimpleJsonParser(exra, value);
	if(strcmp(cmt, CMT_REPPERCHANGE) == 0) {
		SKTDebugPrint(LOG_LEVEL_INFO, "cmt RepPerChange"); 
	} else if(strcmp(cmt, CMT_TAKEPHOTO) == 0) {
		SKTDebugPrint(LOG_LEVEL_INFO, "cmt TakePhoto");
	} else if(strcmp(cmt, CMT_DEVRESET) == 0) {
		SKTDebugPrint(LOG_LEVEL_INFO, "cmt DevReset");
	} else if(strcmp(cmt, CMT_LEDCONTROL) == 0) {
		SKTDebugPrint(LOG_LEVEL_INFO, "cmt LEDControl");
	} else if(strcmp(cmt, CMT_REPIMMEDIATE) == 0) {
		SKTDebugPrint(LOG_LEVEL_INFO, "cmt RepImmediate");
	}
}


void MQTTConnected(int result) {
    SKTDebugPrint(LOG_LEVEL_INFO, "MQTTConnected result : %d", result);    
}

void MQTTSubscribed(int result) {
    SKTDebugPrint(LOG_LEVEL_INFO, "MQTTSubscribed result : %d", result);
	CreateNode();
}

void MQTTDisconnected(int result) {
    SKTDebugPrint(LOG_LEVEL_INFO, "MQTTDisconnected result : %d", result);
}

void MQTTConnectionLost(char* cause) {
    SKTDebugPrint(LOG_LEVEL_INFO, "MQTTConnectionLost result : %s", cause);
}

void MQTTMessageDelivered(int token) {
    SKTDebugPrint(LOG_LEVEL_INFO, "MQTTMessageDelivered token : %d, step : %d", token, mStep);
}

void MQTTMessageArrived(char* topic, char* msg, int msgLen) {
    SKTDebugPrint(LOG_LEVEL_INFO, "MQTTMessageArrived topic : %s, step : %d", topic, mStep);

	if(msg == NULL || msgLen < 1) {
		return;
	}

	if(IsCMD(topic)) {
		ProcessCMD(msg, msgLen);
		return;
	}
	
	char* payload = (char*)calloc(msgLen + 1, sizeof(char));
    memcpy(payload, msg, msgLen);
    SKTDebugPrint(LOG_LEVEL_INFO, "payload : %s", payload);
    char rsc[10] = "";
    SimpleXmlParser(payload, ATTR_RSC, rsc, 0);
    char rsm[128] = "";
    SimpleXmlParser(payload, ATTR_RSM, rsm, 0);

#ifdef ONEM2M_V1_12
    switch(mStep) {
		case PROCESS_NODE_CREATE:
            SimpleXmlParser(payload, ATTR_RI, mNodeLink, 1);
            if(strlen(mNodeLink) > 0) {
				CreateAE();
            }
            break;
       case PROCESS_AE_CREATE:
            SimpleXmlParser(payload, ATTR_AEI, mAEID, 1);
            if(strlen(mAEID) > 0) {
				CreateContainer();
            }
            break;
        case PROCESS_CONTAINER_CREATE:
            CreateMgmtCmd(CMT_LEDCONTROL);
            break;
	    case PROCESS_MGMTCMD_CREATE:
            CreateContentInstance();
            break;
	    case PROCESS_CONTENTINSTANCE_CREATE:
	    	mStep = PROCESS_END;
	    	break;
        default:
            break;
    }
#else
    switch(mStep) {
        case PROCESS_NODE_CREATE:
            SimpleXmlParser(payload, ATTR_RI, mNodeLink, 1);
            SimpleXmlParser(payload, ATTR_HCL, mHostCSELink, 1);
            if(strlen(mNodeLink) > 0) {
                CreateRemoteCSE();
            }
            break;            
        case PROCESS_REMOTECSE_CREATE:
            SimpleXmlParser(payload, ATTR_DKEY, mDeviceKey, 0);
            SimpleXmlParser(payload, ATTR_RN, mRemoteCSEResourceName, 0);
            if(strlen(mDeviceKey) > 0 && strlen(mRemoteCSEResourceName) > 0) {
				CreateContainer();
            }
            break;
		case PROCESS_CONTAINER_CREATE:
			SimpleXmlParser(payload, ATTR_RN, mContainerResourceName, 1);
			if(strlen(mContainerResourceName) > 0) {
				CreateContentInstance();
			}
			break;
		case PROCESS_CONTENTINSTANCE_CREATE:
			SimpleXmlParser(payload, ATTR_RN, mContentInstanceResourceName, 1);
			CreateMgmtCmd(CMT_DEVRESET);
			CreateMgmtCmd(CMT_REPPERCHANGE);
			CreateMgmtCmd(CMT_REPIMMEDIATE);
			CreateMgmtCmd(CMT_TAKEPHOTO);
			CreateMgmtCmd(CMT_LEDCONTROL);			
			break;
		case PROCESS_MGMTCMD_CREATE: ;
			char rn[128] = "";
			SimpleXmlParser(payload, ATTR_RN, rn, 1);
			break;			
        default:
            break;
    }
#endif
	SKTDebugPrint(LOG_LEVEL_INFO, "=============================================================================");
	free(payload);
}

/**
 * @brief main
 * @param[in] argc
 * @param[in] argv
 */
int MARun() {
    SKTDebugInit(True, LOG_LEVEL_INFO, stdout);

	SKTDebugPrint(LOG_LEVEL_INFO, "");
	SKTDebugPrint(LOG_LEVEL_INFO, "==========================================================================");
	SKTDebugPrint(LOG_LEVEL_INFO, "*** MARun example ***");
	SKTDebugPrint(LOG_LEVEL_INFO, "==========================================================================");

	int rc = 0;

    // set callbacks    
    rc = tpMQTTSetCallbacks(MQTTConnected, MQTTSubscribed, MQTTDisconnected, MQTTConnectionLost, MQTTMessageDelivered, MQTTMessageArrived);
	if(rc != 0) {
		SKTDebugPrint(LOG_LEVEL_DEBUG, "#### tpMQTTSetCallbacks failed. rc = %d", rc);
		goto exit;
	}
    SKTDebugPrint(LOG_LEVEL_INFO, "tpMQTTSetCallbacks result : %d", rc);

    // create
    char subscribeTopic[2][128];
    char publishTopic[128] = "";
    memset(subscribeTopic, 0, sizeof(subscribeTopic));
#ifdef ONEM2M_V1_12
    sprintf(subscribeTopic[0], TOPIC_SUBSCRIBE_REQ, ONEM2M_SERVICENAME, ONEM2M_AE_RESOURCENAME);
    sprintf(subscribeTopic[1], TOPIC_SUBSCRIBE_RES, ONEM2M_AE_RESOURCENAME, ONEM2M_SERVICENAME);
    sprintf(publishTopic, TOPIC_PUBLISH, ONEM2M_AE_RESOURCENAME, ONEM2M_SERVICENAME);
#else
    sprintf(subscribeTopic[0], TOPIC_SUBSCRIBE_REQ, ONEM2M_NODEID);
    sprintf(subscribeTopic[1], TOPIC_SUBSCRIBE_RES, ONEM2M_NODEID);
    sprintf(publishTopic, TOPIC_PUBLISH, ONEM2M_NODEID);
#endif
    char* st[] = {subscribeTopic[0], subscribeTopic[1]};

	int port = (!MQTT_ENABLE_SERVER_CERT_AUTH ? MQTT_PORT : MQTT_SECURE_PORT);
	rc = tpMQTTCreate(MQTT_HOST, port, MQTT_KEEP_ALIVE, ACCOUNT_USER, ACCOUNT_PASSWORD, MQTT_ENABLE_SERVER_CERT_AUTH, st, TOPIC_SUBSCRIBE_SIZE, publishTopic);
    SKTDebugPrint(LOG_LEVEL_INFO, "tpSDKCreate result : %d", rc);
	if(rc == 0) {
		while (mStep < PROCESS_END) {
			if(tpMQTTYield(1000) != 0)	// wait for seconds
			{
				osDelay(500);
			}
		}
		tpMQTTDisconnect();
	}

exit:
    tpMQTTDestory();
    return 0;
}



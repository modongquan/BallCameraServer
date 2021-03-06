#include <string.h>
#include <stdio.h>
#include <cstdlib>

#include "EventParser.h"
#include "cJSON.h"

#define EVENT_INFO_MAX_SIZE 64
#define COORDINATES_STR_MAX_SIZE    512

const char device_id[EVENT_INFO_MAX_SIZE] = {"deviceid"};
const char event_type[EVENT_INFO_MAX_SIZE] = {"eventtype"};
const char alarm_type[EVENT_INFO_MAX_SIZE] = {"alarmtype"};
const char sub_alarm_type[EVENT_INFO_MAX_SIZE] = {"subalarmtype"};
const char desc[EVENT_INFO_MAX_SIZE] = {"desc"};
const char alarm_time[EVENT_INFO_MAX_SIZE] = {"alarmtime"};
const char frame_id[EVENT_INFO_MAX_SIZE] = {"frameid"};

const char* StrCut(const char *pSrc, char *pDes, char c)
{
    if(!pSrc || !pDes) return nullptr;

    while(*pSrc != c)
    {
        *pDes++ = *pSrc++;
        if(*pSrc == '\0') return nullptr;
    }
    *pDes = '\0';

    return pSrc;
}

int32_t ParseDeviceId(const char *strEvent, char *pDevId)
{
    if(!strEvent || !pDevId) return -1;

    const char *pDevIdStart = strstr(strEvent, device_id);
    if(!pDevIdStart) return -1;

    pDevIdStart += (strlen(device_id) + 3);
    while(*pDevIdStart != '\"')
    {
        *pDevId++ = *pDevIdStart++;
    }
    *pDevId = '\0';

    return 0;
}

int32_t ParseEventType(const char *strEvent, int32_t *pEventType)
{
    if(!strEvent || !pEventType) return -1;

    const char *pEventTypeStart = strstr(strEvent, event_type);
    if(!pEventTypeStart) return -1;


    char EventType[EVENT_INFO_MAX_SIZE];
    uint32_t i = 0;

    pEventTypeStart += (strlen(event_type) + 3);
    while (*pEventTypeStart != '\"')
    {
        EventType[i++] = *pEventTypeStart++;
        if(i >= EVENT_INFO_MAX_SIZE)
        {
            printf("%s : event type too long \n", __FUNCTION__);
            fflush(stdout);
            return -1;
        }
    }
    EventType[i] = '\0';

    *pEventType = std::atoi(EventType);

    return 0;
}

int32_t ParseAlarmType(const char *strEvent, int32_t *pAlarmType)
{
    if(!strEvent || !pAlarmType) return -1;

    const char *pAlarmTypeStart = strstr(strEvent, alarm_type);
    if(!pAlarmTypeStart) return -1;

    char AlarmType[EVENT_INFO_MAX_SIZE];
    uint32_t i = 0;

    pAlarmTypeStart += (strlen(alarm_type) + 3);
    while (*pAlarmTypeStart != '\"')
    {
        AlarmType[i++] = *pAlarmTypeStart++;
        if(i >= EVENT_INFO_MAX_SIZE)
        {
            printf("%s : event type too long \n", __FUNCTION__);
            fflush(stdout);
            return -1;
        }
    }
    AlarmType[i] = '\0';

    *pAlarmType = std::atoi(AlarmType);

    return 0;
}

int32_t ParseSubAlarmType(const char *strEvent, int32_t *pSubAlarmType)
{
    if(!strEvent || !pSubAlarmType) return -1;

    const char *pSubAlarmTypeStart = strstr(strEvent, sub_alarm_type);
    if(!pSubAlarmTypeStart) return -1;

    char SubAlarmType[EVENT_INFO_MAX_SIZE];
    uint32_t i = 0;

    pSubAlarmTypeStart += (strlen(sub_alarm_type) + 3);
    while (*pSubAlarmTypeStart != '\"')
    {
        SubAlarmType[i++] = *pSubAlarmTypeStart++;
        if(i >= EVENT_INFO_MAX_SIZE)
        {
            printf("%s : event type too long \n", __FUNCTION__);
            fflush(stdout);
            return -1;
        }
    }
    SubAlarmType[i] = '\0';

    *pSubAlarmType = std::atoi(SubAlarmType);

    return 0;
}

int32_t ParseCoordinates(const char *strEvent, StruDefCoordinate *pCoordinates, uint32_t *pNums)
{
    if(!strEvent || !pCoordinates || !pNums) return -1;

    const char *pDescStart = strstr(strEvent, desc);
    if(!pDescStart) return -1;
    const char *pDescEnd = strstr(pDescStart, ",");
    if(!pDescEnd) return -1;

    char strNumber[EVENT_INFO_MAX_SIZE];

    const char *pCoordinatesStart = nullptr;
    const char *pCoordinatesEnd = pDescStart + 5;
    uint32_t idx = 0;

    while(pCoordinatesStart < pDescEnd)
    {
        pCoordinatesStart = strstr(pCoordinatesEnd, "{");
        if(!pCoordinatesStart) break;

        pCoordinatesEnd = strstr(pCoordinatesStart, "}");
        if(!pCoordinatesEnd) break;

        const char *pLeftTopX = pCoordinatesStart + 2;
        pLeftTopX = StrCut(pLeftTopX, strNumber, '|');
        if(!pLeftTopX) break;
        pCoordinates[idx].left_top_x = std::atoi(strNumber);

        const char *pLeftTopY = pLeftTopX + 1;
        pLeftTopY = StrCut(pLeftTopY, strNumber, ')');
        if(!pLeftTopY) break;
        pCoordinates[idx].left_top_y = std::atoi(strNumber);

        const char *pRightBottomX = pLeftTopY + 3;
        pRightBottomX = StrCut(pRightBottomX, strNumber, '|');
        if(!pRightBottomX) break;
        pCoordinates[idx].right_bottom_x = std::atoi(strNumber);

        const char *pRightBottomY = pRightBottomX + 1;
        pRightBottomY = StrCut(pRightBottomY, strNumber, ')');
        if(!pRightBottomY) break;
        pCoordinates[idx].right_bottom_y = std::atoi(strNumber);

        idx++;
    }

    if(!idx) return -1;
    *pNums = idx;

    return 0;
}

int32_t ParseCoordinateFromUdp(const char *pData, StruDefCoordinate *pCoordinates, uint32_t *pNums)
{
    if(!pData || !pCoordinates || !pNums) return -1;

    cJSON *pParseArray = cJSON_Parse(pData);
    if(!pParseArray)
    {
        printf("parse cjson fail\n");
        fflush(stdout);
        return -1;
    }

    int size = cJSON_GetArraySize(pParseArray);
    for(int i = 0; i < size;i ++)
    {
        cJSON *pParseObj = cJSON_GetArrayItem(pParseArray, i);
        if(!pParseObj)
        {
            return -1;
        }

        cJSON *pParseType = cJSON_GetObjectItem(pParseObj, "type");
        if(!pParseType)
        {
            printf("do not find type\n");
            fflush(stdout);
            return -1;
        }

        cJSON *pParseRegion = cJSON_GetObjectItem(pParseObj, "region");
        if(!pParseRegion)
        {
            printf("do not find coodinate\n");
            fflush(stdout);
            return -1;
        }
        cJSON *pParseC1 = cJSON_GetObjectItem(pParseRegion, "c1");
        if(!pParseC1)
        {
            printf("do not find c1\n");
            fflush(stdout);
            return -1;
        }
        cJSON *pParseR1 = cJSON_GetObjectItem(pParseRegion, "r1");
        if(!pParseR1)
        {
            printf("do not find r1\n");
            fflush(stdout);
            return -1;
        }
        cJSON *pParseC2 = cJSON_GetObjectItem(pParseRegion, "c2");
        if(!pParseC2)
        {
            printf("do not find c2\n");
            fflush(stdout);
            return -1;
        }
        cJSON *pParseR2 = cJSON_GetObjectItem(pParseRegion, "r2");
        if(!pParseR2)
        {
            printf("do not find r2\n");
            fflush(stdout);
            return -1;
        }
        printf("type = %s, c1 = %d, r1 = %d, c2 = %d, r2 = %d\n", pParseType->valuestring,
               pParseC1->valueint, pParseR1->valueint, pParseC2->valueint, pParseR2->valueint);
        fflush(stdout);

        pCoordinates[i].type = atoi(pParseType->valuestring);
        pCoordinates[i].left_top_x = pParseC1->valueint;
        pCoordinates[i].left_top_y = pParseR1->valueint;
        pCoordinates[i].right_bottom_x = pParseC2->valueint;
        pCoordinates[i].right_bottom_y = pParseR2->valueint;
    }
    *pNums = size;

    cJSON_Delete(pParseArray);
    return 0;
}

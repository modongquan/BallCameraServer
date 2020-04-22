#ifndef EVENTPARSER_H
#define EVENTPARSER_H

#include <stdint.h>

typedef struct{
    int32_t left_top_x;
    int32_t left_top_y;
    int32_t right_bottom_x;
    int32_t right_bottom_y;
    uint8_t type;
}StruDefCoordinate;

int32_t ParseDeviceId(const char *strEvent, char *pDevId);
int32_t ParseEventType(const char *strEvent, int32_t *pEventType);
int32_t ParseAlarmType(const char *strEvent, int32_t *pAlarmType);
int32_t ParseSubAlarmType(const char *strEvent, int32_t *pSubAlarmType);
int32_t ParseCoordinates(const char *strEvent, StruDefCoordinate *pCoordinates, uint32_t *pNums);
int32_t ParseCoordinateFromUdp(const char *pData, StruDefCoordinate *pCoordinates, uint32_t *pNums);

#endif // EVENTPARSER_H

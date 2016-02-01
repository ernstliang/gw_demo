#ifndef _COMMON_H_
#define _COMMON_H_

typedef unsigned char		uint8;
typedef unsigned short		uint16;

//#include "TXSDKCommonDef.h"

#define TRUE	1
#define FALSE	0

#define BUILD_UINT16(loByte, hiByte)	((uint16)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))

#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)

/*** Generic Status Return Values ***/
#define SUCCESS                   0x00
#define FAILURE                   0x01
#define INVALIDPARAMETER          0x02
#define INVALID_TASK              0x03
#define MSG_BUFFER_NOT_AVAIL      0x04
#define INVALID_POINTER       0x05
#define INVALID_EVENT_ID          0x06
#define INVALID_INTERRUPT_ID      0x07
#define NO_TIMER_AVAIL            0x08
#define NV_ITEM_UNINIT            0x09
#define NV_OPER_FAILED            0x0A
#define INVALID_MEM_SIZE          0x0B
#define NV_BAD_ITEM_LEN           0x0C

#define SYS_EVENT_MSG               0x8000  // A message is waiting event

#define SERIAL_MSG		0x01

#define GW_SDK_CMD			0x04
#define GW_SDK_RM_DEV	0x05

#endif /* _COMMON_H_ */
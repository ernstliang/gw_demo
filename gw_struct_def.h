#ifndef _GW_STRUCT_DEF_H_
#define _GW_STRUCT_DEF_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "common.h"
#include "osal.h"

typedef struct
{
	event_hdr_t hdr;
	uint8 *msg;
} serialdata_t;

typedef struct
{
	unsigned int device_id;		// device id
	unsigned char net_type;
} device_t;

typedef struct  
{
	event_hdr_t hdr;
	unsigned int device_id;
	unsigned char* data;	// recv data from QQ (JSON format)
	unsigned short data_len;
} gw_sdk_cmd_t;

typedef struct
{
	event_hdr_t hdr;
	unsigned int device_id;
} gw_sdk_rm_dev_t;

typedef struct
{
	uint16 nwkAddr;
	unsigned long long extAddr;
} zb_mac_to_addr_t;


#ifdef __cplusplus
}
#endif

#endif /* _GW_STRUCT_DEF_H_ */
#ifndef _GW_DEVICE_H_
#define _GW_DEVICE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "common.h"
#include "gw_struct_def.h"
#include <string.h>

	/************************************************************************/
	/* TYPEDEFS				                                                                     */
	/************************************************************************/
	typedef struct
	{
		void *next;
		unsigned int	device_id;		// device id, unique identification of the equipment
		unsigned char net_type;		// sub-device network type (zigbee, ble ... )
		unsigned char mark;			// device info completed degree
	} device_hdr_t;

	//typedef struct
	//{
	//	uint8 ep_id;				// device endpoint id
	//	uint8 stauts;				// device endpoint data status (0: not get 1: ok)
	//	uint16 profid;
	//	uint16 deviceId;
	//	uint8 deviceVer;
	//	uint16 cluster_in_cnt;	// count of cluster in
	//	uint16 *clusters_in;	// clusters access in
	//	uint16 cluster_out_cnt;	// count of cluster out
	//	uint16 *clusters_out;	// clusters output
	//} ep_info_t;

	typedef struct
	{
		unsigned long long extAddr;	// mac address, unique identification of the sub-device
		uint16 nwkAddr;		// short address in zigbee network
		uint8 endpoint;			// endpoint for qqiot
	} zb_device_t;

	typedef void* device_list_t;

	// device info completed degree
#define DEVICE_INFO_NONE				0x00

	// zb device param
#define ZB_DEVICE_INFO_NWKADDR			0x01
#define ZB_DEVICE_INFO_EXTADDR				0x02
#define ZB_DEVICE_INFO_ENDPOINT			0x04
#define ZB_DEVICE_INFO_COMPLETE			(ZB_DEVICE_INFO_NWKADDR | ZB_DEVICE_INFO_EXTADDR | ZB_DEVICE_INFO_ENDPOINT)

	/************************************************************************/
	/* MACROS				                                                                     */
	/************************************************************************/
#define DEVICE_ID(device_ptr)		((device_hdr_t *)(device_ptr) - 1)->device_id

#define DEVICE_NET_TYPE(device_ptr)	((device_hdr_t *)(device_ptr) - 1)->net_type

#define DEVICE_NEXT(device_ptr)	((device_hdr_t *)(device_ptr) - 1)->next

#define DEVICE_MARK(device_ptr) ((device_hdr_t *)(device_ptr) - 1)->mark

	/************************************************************************/
	/* COMMON FUNCTIONS			                                                 */
	/************************************************************************/
	uint8*	device_allocate(uint16 len);
	uint8		device_deallocate(uint8 *device_ptr);

	// device mgr
	void		device_mgr_init();
	void		device_mgr_uninit();
	void		device_list_clean();

	void		device_enqueue(device_list_t *q_ptr, void *device_ptr);
	void*		device_dequeue(device_list_t *q_ptr);
	void		rm_device(unsigned int device_id);
	uint8*	device_extract(unsigned int device_id);

	uint8*	get_device_ptr(unsigned int device_id);
	uint8		get_device(unsigned int device_id, device_t *info);
	uint8		get_device_nettype(unsigned int device_id, unsigned int * net_type);
	

	/************************************************************************/
	/* ZIGBEE DEVICE FUNCTIONS													       */
	/************************************************************************/
	unsigned int	zb_add_device(unsigned long long extAddr, uint16 shortAddr);
	uint8*	zb_get_device_ptr(uint16 nwkAddr);
	uint8		zb_get_device(uint16 nwkAddr, device_t *info);
	uint8		zb_get_device_info(unsigned int device_id, uint16 *nwkAddr, uint8 *ep, unsigned long long *extAddr);
	uint8		zb_set_endpoint(uint16 nwkAddr, uint8 endpoint);
	uint8		zb_is_device_info_ok(uint16 nwkAddr);

#ifdef __cplusplus
}
#endif

#endif
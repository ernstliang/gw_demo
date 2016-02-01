#include <stdio.h>
#include <stdlib.h>
#include "gw_device_mgr.h"
#include "TXGWSDK.h"
#include "gw_struct_def.h"
#include "gw_sn_to_dev.h"

// device list head
device_list_t device_list_head;

//pthread_mutex_t mutex_device;	// mutex for device_list_head

uint8* device_allocate( uint16 len )
{
	if (len == 0)
		return NULL;

	device_hdr_t *hdr;
	hdr = (device_hdr_t *)malloc(len + sizeof(device_hdr_t));
	if (hdr)
	{
		hdr->next = NULL;
		hdr->device_id = 0;
		hdr->net_type = 0;
		hdr->mark = DEVICE_INFO_NONE;
		return ((uint8 *)(hdr + 1));
	}
	else
		return NULL;
}

uint8 device_deallocate( uint8 *device_ptr )
{
	uint8 *x;

	if (device_ptr == NULL)
		return (INVALID_POINTER);

	printf("device_deallocate %u\n", DEVICE_ID(device_ptr));

	x = (uint8 *)((uint8*)device_ptr - sizeof(device_hdr_t));
	free(x);

	return (SUCCESS);
}


//////////////////////////////////////////////////////////////////////////

// this must be call before use device info
void device_mgr_init()
{
	// Initialize the device queue
	device_list_head = NULL;

	load_m2di_in_file();
	// Initialize the mutex
	//pthread_mutex_init(&mutex_device, NULL);
}

void device_mgr_uninit()
{
	// 保存设备列表并清理内存退出
	device_list_clean();
	clear_m2di_list();

	//pthread_mutex_destroy(&mutex_device);
}

/************************************************************************/
/* DEVICE LIST			                                                                     */
/************************************************************************/

void device_list_clean()
{
	void * device_ptr = device_dequeue(&device_list_head);
	while (device_ptr)
	{
		device_deallocate((uint8*)device_ptr);
		device_ptr = device_dequeue(&device_list_head);
	}
}

void device_enqueue( device_list_t *q_ptr, void *device_ptr )
{
	void *list;

	//pthread_mutex_lock(&mutex_device);

	DEVICE_NEXT(device_ptr) = NULL;
	if (*q_ptr == NULL)
	{
		*q_ptr = device_ptr;
	}
	else
	{
		for (list = *q_ptr; DEVICE_NEXT(list) != NULL; list = DEVICE_NEXT(list));

		DEVICE_NEXT(list) = device_ptr;
	}

	//pthread_mutex_unlock(&mutex_device);
}

void* device_dequeue( device_list_t *q_ptr )
{
	void *device_ptr = NULL;

	//pthread_mutex_lock(&mutex_device);

	if (*q_ptr != NULL)
	{
		// Dequeue device
		device_ptr = *q_ptr;
		*q_ptr = DEVICE_NEXT(device_ptr);
		DEVICE_NEXT(device_ptr) = NULL;
	}

	//pthread_mutex_unlock(&mutex_device);

	return device_ptr;
}

unsigned int zb_add_device( unsigned long long extAddr, uint16 shortAddr )
{
	// 添加之前先判断mac对应的device id是否已分配
	unsigned int device_id = add_mac_to_device_id(extAddr);
	if (device_id != 0)
	{
		// 判读device_id对应的设备是否已在缓存中
		uint8 * dev_ptr = get_device_ptr(device_id);
		if (dev_ptr != NULL && ((DEVICE_MARK(dev_ptr) == ZB_DEVICE_INFO_COMPLETE))) // 已存在则直接返回0
		{
			printf("Device %u already existed! mark[%02X]\n", device_id, DEVICE_MARK(dev_ptr));
			return 0;
		}

		// 2016-01-14 更新子设备信息
		if (dev_ptr && (DEVICE_NET_TYPE(dev_ptr) == gw_end_device_network_type_zigbee))
		{
			DEVICE_MARK(dev_ptr) |= (ZB_DEVICE_INFO_NWKADDR | ZB_DEVICE_INFO_EXTADDR);

			zb_device_t * zb_dev_ptr = (zb_device_t *)dev_ptr;
			zb_dev_ptr->nwkAddr = shortAddr;
			zb_dev_ptr->extAddr = extAddr;

			return device_id;
			//rm_device(device_id);
		}
	}

	zb_device_t *device_ptr = (zb_device_t *)device_allocate(sizeof(zb_device_t));
	if (device_ptr)
	{
		memset(device_ptr, 0, sizeof(zb_device_t));
		DEVICE_ID(device_ptr) = device_id;
		DEVICE_NET_TYPE(device_ptr) = gw_end_device_network_type_zigbee;
		DEVICE_NEXT(device_ptr) = NULL;
		DEVICE_MARK(device_ptr) |= (ZB_DEVICE_INFO_NWKADDR | ZB_DEVICE_INFO_EXTADDR);

		device_ptr->nwkAddr = shortAddr;
		device_ptr->extAddr = extAddr;

		device_enqueue(&device_list_head, device_ptr);
	}
	return device_id;
}

void rm_device( unsigned int device_id )
{
	uint8* device_ptr = device_extract(device_id);
	if (device_ptr)
	{
		device_deallocate(device_ptr);
	}
}

uint8* get_device_ptr( unsigned int device_id )
{
	device_hdr_t *listHdr;
	device_hdr_t *foundHdr = NULL;

	//pthread_mutex_lock(&mutex_device);

	listHdr = (device_hdr_t *)device_list_head;

	while (listHdr != NULL)
	{
		if ((listHdr - 1)->device_id == device_id)
		{
			foundHdr = listHdr;
			break;
		}
		listHdr = (device_hdr_t *)DEVICE_NEXT(listHdr);
	}

	//pthread_mutex_unlock(&mutex_device);

	return ((uint8 *)foundHdr);
}

uint8* zb_get_device_ptr( uint16 nwkAddr )
{
	device_hdr_t *listHdr;
	device_hdr_t *foundHdr = NULL;

	//pthread_mutex_lock(&mutex_device);

	listHdr = (device_hdr_t *)device_list_head;

	while (listHdr != NULL)
	{
		if (DEVICE_NET_TYPE(listHdr) == gw_end_device_network_type_zigbee && ((zb_device_t *)listHdr)->nwkAddr == nwkAddr)
		{
			foundHdr = listHdr;
			break;
		}
		listHdr = (device_hdr_t *)DEVICE_NEXT(listHdr);
	}

	//pthread_mutex_unlock(&mutex_device);

	return ((uint8 *)foundHdr);
}

uint8 get_device_nettype(unsigned int device_id, unsigned int * net_type)
{
	uint8 * dev_ptr = get_device_ptr(device_id);
	if (!dev_ptr)
		return 0;

	*net_type = DEVICE_NET_TYPE(dev_ptr);
	
	return 1;
}

uint8 zb_get_device_info(unsigned int device_id, uint16 *nwkAddr, uint8 *ep, unsigned long long *extAddr)
{
	uint8 * dev_ptr = get_device_ptr(device_id);
	if (dev_ptr && DEVICE_NET_TYPE(dev_ptr) == gw_end_device_network_type_zigbee)
	{
		zb_device_t *zb_device = (zb_device_t *)dev_ptr;

		*nwkAddr = zb_device->nwkAddr;
		*ep = zb_device->endpoint;
		*extAddr = zb_device->extAddr;
		return 1;
	}
	return 0;
}

// 提取device_id对应设备的信息
uint8* device_extract( unsigned int device_id )
{
	device_hdr_t *listHdr;
	device_hdr_t *foundHdr = NULL;

	//pthread_mutex_lock(&mutex_device);

	listHdr = (device_hdr_t *)device_list_head;

	while (listHdr != NULL)
	{
		if ((listHdr - 1)->device_id == device_id)
		{
			foundHdr = listHdr;
			listHdr = (device_hdr_t *)DEVICE_NEXT(listHdr);
			DEVICE_NEXT(foundHdr) = NULL;

			// extract is the first item of device_list_head, need change device_list_head to next item
			if (foundHdr == device_list_head)
				device_list_head = listHdr;
			break;
		}
		listHdr = (device_hdr_t *)DEVICE_NEXT(listHdr);
	}

	//pthread_mutex_unlock(&mutex_device);

	return ((uint8 *)foundHdr);
}

uint8 get_device( unsigned int device_id, device_t *info )
{
	if (!info) return 0;

	uint8 *dev_ptr = get_device_ptr(device_id);
	if (!dev_ptr)
		return 0;

	info->device_id = DEVICE_ID(dev_ptr);
	info->net_type = DEVICE_NET_TYPE(dev_ptr);
	return 1;
}

uint8 zb_get_device( uint16 nwkAddr, device_t *info )
{
	if (!info) return 0;

	uint8 * dev_ptr = zb_get_device_ptr(nwkAddr);
	if (!dev_ptr)
		return 0;

	info->device_id = DEVICE_ID(dev_ptr);
	info->net_type = DEVICE_NET_TYPE(dev_ptr);
	return 1;
}

uint8 zb_is_device_info_ok( uint16 nwkAddr )
{
	uint8 * dev_ptr = zb_get_device_ptr(nwkAddr);
	if (!dev_ptr)
		return 0;

	if (DEVICE_MARK(dev_ptr) == ZB_DEVICE_INFO_COMPLETE)
		return 1;

	return 0;
}

uint8 zb_set_endpoint( uint16 nwkAddr, uint8 endpoint )
{
	uint8 * device_ptr = zb_get_device_ptr(nwkAddr);
	if (device_ptr)
	{
		DEVICE_MARK(device_ptr) |= ZB_DEVICE_INFO_ENDPOINT;
		((zb_device_t *)device_ptr)->endpoint = endpoint;
		return 1;
	}
	return 0;
}
#include "gw_sn_to_dev.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

// mac to device_id
mac_to_id_q_t m2di_qHead;

unsigned int g_device_id = 1000;

#define M2DI_FILE	"m2di_list"

/************************************************************************/
/* MAC ADDR TO DEVICE_ID                                                         */
/************************************************************************/

unsigned int assign_device_id( unsigned long long extAddr )
{
	// TODO: save mac address to device_id list
	return ++g_device_id;
}

unsigned int	add_mac_to_device_id(unsigned long long extAddr)
{
	// 判断mac地址是否已分配device_id
	unsigned int device_id = get_device_id_by_mac(extAddr);
	if (device_id != 0)
		return device_id;

	device_id = assign_device_id(extAddr);

	add_m2di(device_id, extAddr);

	save_m2di_to_file(device_id, extAddr);

	return device_id;
}

unsigned int get_device_id_by_mac(unsigned long long extAddr)
{
	unsigned int device_id = 0;
	sn_to_id_t *list;
	sn_to_id_t *found = NULL;

	list = (sn_to_id_t *)m2di_qHead;

	while (list != NULL)
	{
		if (list->extAddr == extAddr)
		{
			found = list;
			break;
		}
		list = (sn_to_id_t*)list->next;
	}
	if (found)
	{
		device_id = found->device_id;
	}
	return device_id;
}

void m2di_enqueue( mac_to_id_q_t* q_ptr, sn_to_id_t *m2di_ptr )
{
	if (NULL == m2di_ptr) return;

	sn_to_id_t *list;

	m2di_ptr->next = NULL;
	if (*q_ptr == NULL)
	{
		*q_ptr = m2di_ptr;
	}
	else
	{
		for (list = *q_ptr; list->next != NULL; list = (sn_to_id_t *)list->next);

		list->next = m2di_ptr;
	}
}

void* m2di_dequeue(mac_to_id_q_t* q_ptr)
{
	void * m2di_ptr = NULL;
	if (*q_ptr != NULL)
	{
		m2di_ptr = *q_ptr;
		*q_ptr = M2DI_NEXT(m2di_ptr);
		M2DI_NEXT(m2di_ptr) = NULL;
	}
	return m2di_ptr;
}

void add_m2di(unsigned int device_id, unsigned long long extAddr)
{
	if (g_device_id < device_id)
		g_device_id = device_id;

	printf("add_m2di, device_id: %u, extAddr: %llX\n", device_id, extAddr);
	sn_to_id_t *m2di = (sn_to_id_t *)malloc(sizeof(sn_to_id_t));
	m2di->device_id = device_id;
	m2di->extAddr = extAddr;
	m2di->next = NULL;

	m2di_enqueue(&m2di_qHead, m2di);
}

void clear_m2di_list()
{
	sn_to_id_t * m2di_ptr = NULL;
	while ((m2di_ptr = (sn_to_id_t *)m2di_dequeue(&m2di_qHead)) != NULL)
	{
		free(m2di_ptr);
	}
	m2di_qHead = NULL;
}

void save_m2di_to_file( unsigned int device_id, unsigned long long extAddr )
{
	FILE * fd = fopen(M2DI_FILE, "ab+");
	if (fd)
	{
		int nlen = 4 + 8 + 2;
		unsigned char *data = (unsigned char*)malloc(nlen * sizeof(unsigned char));
		memcpy(&data[0], "[", 1);
		memcpy(&data[1], &extAddr, 8);
		memcpy(&data[9], &device_id, 4);
		memcpy(&data[13], "]", 1);
		fwrite(data, sizeof(unsigned char), nlen, fd);

		free(data);
		fclose(fd);
	}
}

void load_m2di_in_file()
{
	FILE * fd = fopen(M2DI_FILE, "rb");
	if (fd)
	{
		fseek(fd, 0L, SEEK_END);
		long len = ftell(fd);
		fseek(fd, 0L, SEEK_SET);

		char * buf = (char *)malloc(len * sizeof(char));
		if (fread(buf, sizeof(char), len, fd) == len)
		{
			// 处理数据
			int index = 0;
			while (index < len && buf[index] == '[')
			{
				index += 1;
				unsigned long long extAddr = 0;
				unsigned int device_id = 0;
				memcpy(&extAddr, &buf[index], 8);
				index += 8;
				memcpy(&device_id, &buf[index], 4);
				add_m2di(device_id, extAddr);
				index += 4;
				index += 1;
			}
		}

		free(buf);
		fclose(fd);
	}
}
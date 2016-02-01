#include <stdio.h>
#include <stdlib.h>

#include "osal.h"
#include "gw_struct_def.h"
#include "osal_task.h"
#include <pthread.h>
#include "utils.h"


osal_msg_q_t osal_qHead;

pthread_mutex_t mutex_event;
pthread_mutex_t mutex_msg;

static uint8 activeTaskID = TASK_NO_TASK;

extern int g_stop;

uint8 * osal_msg_allocate(uint16 len)
{
	if (len == 0)
		return NULL;

	//printf("osal_msg_allocate \n");
	osal_msg_hdr_t *hdr;
	hdr = (osal_msg_hdr_t *)malloc(len + sizeof(osal_msg_hdr_t));
	if (hdr)
	{
		hdr->next = NULL;
		hdr->len = len;
		hdr->dest_id = TASK_NO_TASK;
		return ((uint8*)(hdr + 1));
	}
	else
		return NULL;
}

uint8 osal_msg_deallocate(uint8 * msg_ptr)
{
	uint8 *x;

	if (msg_ptr == NULL)
		return (INVALID_POINTER);

	if (OSAL_MSG_ID(msg_ptr) != TASK_NO_TASK)
		return (MSG_BUFFER_NOT_AVAIL);

	//printf("osal_msg_deallocate\n");
	x = (uint8 *)((uint8 *)msg_ptr - sizeof(osal_msg_hdr_t));

	free(x);

	return (SUCCESS);
}

uint8 osal_msg_send( uint8 task_id, uint8 *msg_ptr )
{
	if (msg_ptr == NULL)
		return (INVALID_POINTER);

	if (task_id >= tasksCnt)
	{
		osal_msg_deallocate(msg_ptr);
		return (INVALID_TASK);
	}

	if (OSAL_MSG_NEXT(msg_ptr) != NULL ||
		OSAL_MSG_ID(msg_ptr) != TASK_NO_TASK)
	{
		osal_msg_deallocate(msg_ptr);
		return (INVALID_POINTER);
	}

	OSAL_MSG_ID(msg_ptr) = task_id;

	// queue message
	osal_msg_enqueue(&osal_qHead, msg_ptr);

	// Signal the task that a message is waiting
	osal_set_event(task_id, SYS_EVENT_MSG);

	return (SUCCESS);
}

uint8* osal_msg_receive( uint8 task_id )
{
	osal_msg_hdr_t *listHdr;
	osal_msg_hdr_t *prevHdr = NULL;
	osal_msg_hdr_t *foundHdr = NULL;

	pthread_mutex_lock(&mutex_msg);
	
	// Point to the top of the queue
	listHdr = (osal_msg_hdr_t *)osal_qHead;

	// Look through the queue for a message that belongs to the asking task
	while (listHdr != NULL)
	{
		if ((listHdr - 1)->dest_id == task_id)
		{
			if (foundHdr == NULL)
			{
				foundHdr = listHdr;
			}
			else
			{
				break;
			}
		}
		if (foundHdr == NULL)
		{
			prevHdr = listHdr;
		}
		listHdr = (osal_msg_hdr_t *)OSAL_MSG_NEXT(listHdr);
	}

	// Is there mor than one?
	if (listHdr != NULL)
	{
		osal_set_event(task_id, SYS_EVENT_MSG);
	}
	else
	{
		osal_clear_event(task_id, SYS_EVENT_MSG);
	}

	if (foundHdr != NULL)
	{
		osal_msg_extract(&osal_qHead, foundHdr, prevHdr);
	}

	pthread_mutex_unlock(&mutex_msg);
	
	return ((uint8 *)foundHdr);
}

void osal_msg_enqueue( osal_msg_q_t *q_ptr, void *msg_ptr )
{
	void *list;

	pthread_mutex_lock(&mutex_msg);

	OSAL_MSG_NEXT(msg_ptr) = NULL;
	if (*q_ptr == NULL)
	{
		*q_ptr = msg_ptr;
	}
	else
	{
		// Find end of queue
		for (list = *q_ptr; OSAL_MSG_NEXT(list) != NULL; list = OSAL_MSG_NEXT(list));

		// Add message to end of queue
		OSAL_MSG_NEXT(list) = msg_ptr;
	}

	pthread_mutex_unlock(&mutex_msg);
}

void* osal_msg_dequeue( osal_msg_q_t *q_ptr )
{
	void *msg_ptr = NULL;
	pthread_mutex_lock(&mutex_msg);

	if (*q_ptr != NULL)
	{
		// Dequeue message
		msg_ptr = *q_ptr;
		*q_ptr = OSAL_MSG_NEXT(msg_ptr);
		OSAL_MSG_NEXT(msg_ptr) = NULL;
		OSAL_MSG_ID(msg_ptr) = TASK_NO_TASK;
	}

	pthread_mutex_unlock(&mutex_msg);

	return msg_ptr;
}

// Warning: this function need lock outside
void osal_msg_extract( osal_msg_q_t *q_ptr, void *msg_ptr, void *prev_ptr )
{
	if (msg_ptr == *q_ptr)
	{
		*q_ptr = OSAL_MSG_NEXT(msg_ptr);
	}
	else
	{
		OSAL_MSG_NEXT(prev_ptr) = OSAL_MSG_NEXT(msg_ptr);
	}
	OSAL_MSG_NEXT(msg_ptr) = NULL;
	OSAL_MSG_ID(msg_ptr) = TASK_NO_TASK;
}

uint8 osal_set_event( uint8 task_id, uint16 events )
{
	if (task_id < tasksCnt)
	{
		pthread_mutex_lock(&mutex_event);
		tasksEvents[task_id] |= events;
		pthread_mutex_unlock(&mutex_event);
		return (SUCCESS);
	}
	else
	{
		return (INVALID_TASK);
	}
}

uint8 osal_clear_event( uint8 task_id, uint16 events )
{
	if (task_id < tasksCnt)
	{
		pthread_mutex_lock(&mutex_event);
		tasksEvents[task_id] &= ~events;
		pthread_mutex_unlock(&mutex_event);
		return (SUCCESS);
	}
	else
	{
		return (INVALID_TASK);
	}
}

uint8	 osal_init_system()
{
	// Initialize the message queue
	osal_qHead = NULL;

	//
	osalInitTasks();

	// Initialize the mutex
	pthread_mutex_init(&mutex_event, NULL);
	pthread_mutex_init(&mutex_msg, NULL);

	return 1;
}

void osal_start_system()
{
	for (;;)
	{
		osal_run_system();

		if (g_stop) // stop process
			break;

		gw_sleep(100);
	}

	// release mutex
	pthread_mutex_destroy(&mutex_event);
	pthread_mutex_destroy(&mutex_msg);
}

void osal_run_system()
{
	uint8 idx = 0;
	do 
	{
		if (tasksEvents[idx])
			break;
	} while (++idx < tasksCnt);

	if (idx < tasksCnt)
	{
		uint16 events;

		pthread_mutex_lock(&mutex_event);
		events = tasksEvents[idx];
		tasksEvents[idx] = 0;
		pthread_mutex_unlock(&mutex_event);
		
		activeTaskID = idx;
		events = (tasksArr[idx])(idx, events);
		activeTaskID = TASK_NO_TASK;

		pthread_mutex_lock(&mutex_event);
		tasksEvents[idx] |= events;
		pthread_mutex_unlock(&mutex_event);
	}
}
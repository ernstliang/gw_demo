#ifndef _OSAL_H_
#define _OSAL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "common.h"

/************************************************************************/
/* TYPEDEFS				                                                                     */
/************************************************************************/
typedef struct
{
	void * next;
	uint16 len;
	uint8 dest_id;
} osal_msg_hdr_t;

typedef struct
{
	uint8 event;
	uint8 status;
} event_hdr_t;

typedef void * osal_msg_q_t;

/************************************************************************/
/* MACROS				                                                                     */
/************************************************************************/
#define OSAL_MSG_NEXT(msg_ptr)      (((osal_msg_hdr_t *)msg_ptr) - 1)->next

#define OSAL_MSG_Q_INIT(q_ptr)      *(q_ptr) = NULL

#define OSAL_MSG_Q_EMPTY(q_ptr)     (*(q_ptr) == NULL)

#define OSAL_MSG_Q_HEAD(q_ptr)      (*(q_ptr))

#define OSAL_MSG_LEN(msg_ptr)      ((osal_msg_hdr_t *) (msg_ptr) - 1)->len

#define OSAL_MSG_ID(msg_ptr)      ((osal_msg_hdr_t *) (msg_ptr) - 1)->dest_id

/************************************************************************/
/*	FUNCTIONS			                                                                     */
/************************************************************************/
	uint8*	osal_msg_allocate(uint16 len);
	uint8		osal_msg_deallocate(uint8 * msg_ptr);

	uint8		osal_msg_send(uint8 task_id, uint8 *msg_ptr);
	uint8*	osal_msg_receive(uint8 task_id);

	void		osal_msg_enqueue(osal_msg_q_t *q_ptr, void *msg_ptr);
	void*		osal_msg_dequeue(osal_msg_q_t *q_ptr);

	void		osal_msg_extract(osal_msg_q_t *q_ptr, void *msg_ptr, void *prev_ptr);

	uint8		osal_set_event(uint8 task_id, uint16 events);
	uint8		osal_clear_event(uint8 task_id, uint16 events);

	uint8		osal_init_system();
	void		osal_start_system();
	void		osal_run_system();

#ifdef __cplusplus
}
#endif

#endif /* _OSAL_H_ */
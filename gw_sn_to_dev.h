#ifndef _GW_SN_TO_DEV_H_
#define _GW_SN_TO_DEV_H_

/************************************************************************/
/*  维护sn对应deviceid的表		                                                         */
/************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct
	{
		void* next;
		unsigned int device_id;
		unsigned long long extAddr;
	} sn_to_id_t;

	typedef sn_to_id_t* mac_to_id_q_t;

#define M2DI_NEXT(m2di_ptr) ((sn_to_id_t*)m2di_ptr)->next

	unsigned int	assign_device_id(unsigned long long extAddr);
	unsigned int	add_mac_to_device_id(unsigned long long extAddr);
	unsigned int get_device_id_by_mac(unsigned long long extAddr);
	void		m2di_enqueue(mac_to_id_q_t* q_ptr, sn_to_id_t *m2di_ptr);
	void *	m2di_dequeue(mac_to_id_q_t* q_ptr);
	void		add_m2di(unsigned int device_id, unsigned long long extAddr);
	void		clear_m2di_list();

	void		save_m2di_to_file(unsigned int device_id, unsigned long long extAddr);
	void		load_m2di_in_file();

#ifdef __cplusplus
}
#endif

#endif /* _GW_SN_TO_DEV_H_ */
#ifndef _UTILS_H_
#define _UTILS_H_

#ifdef __cplusplus
extern "C"
{
#endif
	int gw_readfile(char *pPath, char *pBuffer, int nInSize, int *pSizeUsed);

	void gw_sleep(long millsecond);

	// duplicate
	unsigned char *	data_dup(const unsigned char * data);
	unsigned char *	data_dup_len(const unsigned char * data, unsigned int len);
	void		data_free(unsigned char * data);

	// file operation
	int	is_file_exist(const char * file_path);

#ifdef __cplusplus
}
#endif

#endif /* _UTILS_H_ */
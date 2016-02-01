#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

int gw_readfile(char *pPath, char *pBuffer, int nInSize, int *pSizeUsed)
{
	if (!pPath || !pBuffer || !pSizeUsed)
	{
		return -1;
	}

	int uLen = 0;
	FILE * file = fopen(pPath, "rb");
	if (!file)
	{
		return -1;
	}

	do
	{
		fseek(file, 0L, SEEK_END);
		uLen = ftell(file);
		fseek(file, 0L, SEEK_SET);

		if (0 == uLen || uLen > nInSize)
		{
			printf("invalide file or buffer size is too small...\n");
			break;
		}

		*pSizeUsed = fread(pBuffer, 1, uLen, file);

		fclose(file);
		return 0;

	}while(0);

	fclose(file);
	return -1;
}

void gw_sleep( long millsecond )
{
	struct timespec ts;
	ts.tv_sec = millsecond / 1000;
	ts.tv_nsec = (millsecond % 1000) * 1000000;
	nanosleep(&ts, 0);
}

unsigned char * data_dup( const unsigned char * data )
{
	if (!data) return NULL;

	unsigned char * dup_data;
	size_t size = strlen(data);
	
	dup_data = (unsigned char *)malloc(size * sizeof(unsigned char));
	memcpy(dup_data, data, size);

	return dup_data;
}

unsigned char * data_dup_len(const unsigned char * data, unsigned int len)
{
	if (!data) return NULL;

	if (len <= 0)
	{
		return data_dup(data);
	}
	else
	{
		unsigned char * dup_data;
		dup_data = (unsigned char *)malloc(len * sizeof(unsigned char));
		memcpy(dup_data, data, len);

		return dup_data;
	}
}

void data_free( unsigned char * data )
{
	if (data)
		free(data);
}

int is_file_exist( const char * file_path )
{
	if (file_path == NULL)
		return -1;

	if (access(file_path, F_OK) == 0)
		return 0;
	return -1;
}
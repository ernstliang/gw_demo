#ifndef _UART_API_H_
#define _UART_API_H_

#ifdef __cplusplus
extern "C"
{
#endif

	// uart init
	void set_speed(int fd, int speed);
	int set_parity(int fd, int databits, int stopbits, int parity);
	int open_serial_port(char * dev);

	//int tread(int fd, void* buf, int len, unsigned int timeout);
	//int treadn(int fd, void* buf, int len, unsigned int timeout);

#ifdef __cplusplus
}
#endif

#endif /* _UART_API_H_ */
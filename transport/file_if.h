int tc_init(unsigned short port); //open/close telnet server
int tc_write(unsigned char* data, int len); //send sata to connecting
int tc_read(unsigned char* data); //poll server for accept and receive
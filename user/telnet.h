int tn_init(unsigned short port); //open/close telnet server
int tn_write(unsigned char* data, int len); //send sata to connecting
int tn_read(unsigned char* data); //poll server for accept and receive
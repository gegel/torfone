//platform-depend serial device functions
int InitPort(char* dev);
int ReadPort(char* buf, int maxlen);
int WritePort(char* buf, int len);
void ClosePort(void);

//helpers
unsigned int ser_crc32(unsigned char const *p, unsigned int len);

//interface
short  ser_open(char* dev);
void ser_close(void);
short ser_send(unsigned char* data);
short ser_read(unsigned char* data);

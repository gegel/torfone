#ifdef __cplusplus
extern "C" {
#endif

int InitPort(char* dev);
int ReadPort(char* buf, int maxlen);
int WritePort(char* buf, int len);
void ClosePort(void);

#ifdef __cplusplus
}
#endif

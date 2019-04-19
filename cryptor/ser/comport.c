//FOR WIN32 ONLY!
#ifdef _WIN32

#include <stdio.h>
#include <conio.h>
#include <string.h>

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>



    HANDLE file=INVALID_HANDLE_VALUE;
    DWORD read=0;
    volatile DWORD opened=0;

int InitPort(char* dev)
{

    COMMTIMEOUTS timeouts;
    DCB port;
    char port_name[128] = "\\\\.\\COM2";

    opened=0;

 if((!dev)||(!dev[0]))
 {
  if(file!=INVALID_HANDLE_VALUE) CloseHandle(file);
  file=INVALID_HANDLE_VALUE;
 }

 
 sprintf(port_name, "\\\\.\\%s", dev);
 //sprintf(port_name, "\\\\.\\COM%d", mport);
 //sprintf(port_name, "COM%c", mport);
 // open the comm port.
    file = CreateFile(port_name,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if ( INVALID_HANDLE_VALUE == file) {
        return -1;
    }

     // get the current DCB, and adjust a few bits to our liking.
    memset(&port, 0, sizeof(port));
    port.DCBlength = sizeof(port);
    if ( !GetCommState(file, &port))
        {CloseHandle(file);
        file=INVALID_HANDLE_VALUE;
        return -2;}
    if (!BuildCommDCB("baud=115200 parity=n data=8 stop=1", &port))
        {CloseHandle(file);
        file=INVALID_HANDLE_VALUE;
        return -3;}
    if (!SetCommState(file, &port))
        {
         CloseHandle(file);
         file=INVALID_HANDLE_VALUE;
         return -4;
        }

    // set short timeouts on the comm port.
    timeouts.ReadIntervalTimeout = MAXDWORD; //50;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0; //1000;
    timeouts.WriteTotalTimeoutMultiplier = 0; //1;
    timeouts.WriteTotalTimeoutConstant = 0; //1;
    if (!SetCommTimeouts(file, &timeouts))
        {CloseHandle(file);
        file=INVALID_HANDLE_VALUE;
        return -5;}
        opened=1;
    return 0;
}


int ReadPort(char* buf, int maxlen)
{
  read=0;
  if(!opened) return -2;
  if(file==INVALID_HANDLE_VALUE) return -1;
  // check for data on port and display it on screen.
  ReadFile(file, buf, maxlen, &read, NULL);
  return read;
}

int WritePort(char* buf, int len)
{
 DWORD written;
 if(!opened) return -2;
 if(file==INVALID_HANDLE_VALUE) return -1;
 written=0;
 WriteFile(file, buf, len, &written, NULL);
 return written;
}


void ClosePort(void)
{
 opened=0;
 if(file!=INVALID_HANDLE_VALUE) CloseHandle(file);
 file=INVALID_HANDLE_VALUE;
}


#endif //#ifdef _WIN32

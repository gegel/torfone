#ifndef _WIN32

#include <stdio.h>
#include <string.h>
//#include <errno.h>
//#include <sys/time.h>

#include <stdint.h>
#include <stdarg.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>

#include <stdlib.h>
//#include <time.h>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <netinet/tcp.h>
//#include <arpa/inet.h>
//#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <fcntl.h>
//#include <ifaddrs.h>

 #define BAUDRATE 115200  //used baudrate

 int uart=0;    //uart descriptor
 int opened=0;  //safe-thread flag of port is ready to use
 
 
 //close port
void ClosePort(void)
{
 if(uart)
 {
  opened=0;  //clear safe thread flag
  close (uart); //close port
  uart=0; //clear descriptor
 }
}
 
 //open port by serial device name
 int InitPort(char* dev)
 {
  struct termios options ;
  speed_t myBaud ;
  int     status, fd ;
  int baud = BAUDRATE; //set fixed baudrate
    
  ClosePort();  //close old port 
  
  //use boud rate predefined constant by speed:
  switch (baud)
  {
    case     50:	myBaud =     B50 ; break ;
    case     75:	myBaud =     B75 ; break ;
    case    110:	myBaud =    B110 ; break ;
    case    134:	myBaud =    B134 ; break ;
    case    150:	myBaud =    B150 ; break ;
    case    200:	myBaud =    B200 ; break ;
    case    300:	myBaud =    B300 ; break ;
    case    600:	myBaud =    B600 ; break ;
    case   1200:	myBaud =   B1200 ; break ;
    case   1800:	myBaud =   B1800 ; break ;
    case   2400:	myBaud =   B2400 ; break ;
    case   4800:	myBaud =   B4800 ; break ;
    case   9600:	myBaud =   B9600 ; break ;
    case  19200:	myBaud =  B19200 ; break ;
    case  38400:	myBaud =  B38400 ; break ;
    case  57600:	myBaud =  B57600 ; break ;
    case 115200:	myBaud = B115200 ; break ;
    case 230400:	myBaud = B230400 ; break ;

    default:
      return -2 ;
  }

  //try open uart
  if ((fd = open (dev, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK)) == -1)
    return -1 ;
  //set uart device as read/writeable
  fcntl (fd, F_SETFL, O_RDWR) ;

// Get and modify current uart options:
  tcgetattr (fd, &options) ;  //get current

    cfmakeraw   (&options) ; //check
    cfsetispeed (&options, myBaud) ; //set baudrate RX
    cfsetospeed (&options, myBaud) ; //set baudrate TX

	//set other uart options
    options.c_cflag |= (CLOCAL | CREAD) ;
    options.c_cflag &= ~PARENB ;
    options.c_cflag &= ~CSTOPB ;
    options.c_cflag &= ~CSIZE ;
    options.c_cflag |= CS8 ;
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG) ;
    options.c_oflag &= ~OPOST ;

    options.c_cc [VMIN]  =   0 ;
    options.c_cc [VTIME] =   0 ;	// No delay

	//apply options
  tcsetattr (fd, TCSANOW | TCSAFLUSH, &options) ;

  //get HW flow status
  ioctl (fd, TIOCMGET, &status);
//apply new
  status |= TIOCM_DTR ;
  status |= TIOCM_RTS ;
//set DTR flow status
  ioctl (fd, TIOCMSET, &status);
 //deleay for setup uart before first using
  usleep (10000) ; // 10mS
 // psleep (10) ;	// 10mS
  uart=fd; //set port descriptor
  opened=1; //set safe thread flag: ready for use
  return 0;

 }



//read up to maxlen bytes from serial
int ReadPort(char* buf, int maxlen)
{
 int result=0;
 if(opened && uart) //check port is ready
 {
  if(ioctl (uart, FIONREAD, &result) == -1) return 0; //check data is available
  if(result<=0) return 0;
  if(result>=maxlen) result=maxlen; //read only specified bytes or less
  if (read (uart, (unsigned char*)buf, maxlen) != result) result=0; //try read
  result=maxlen; //bytes actually read
 }
 return result;
}

//write len bytes to serial
int WritePort(char* buf, int len)
{
  if(opened && uart) write (uart, (unsigned char*)buf, len) ;
  return len;
}

//flush serial
void FlushPort(void)
{
 if(opened && uart) tcflush (uart, TCIOFLUSH) ;
}













#endif //#ifndef _WIN32


#include <string.h>
#include "ser.h"

//port descriptor
unsigned char ser_in[48];
unsigned char ser_out[48];
short ser_len=0;
short ser_sync=-1;

unsigned int ser_crc32(unsigned char const *p, unsigned int len)
{
  #define CRCPOLY_LE 0xedb88320
  
  unsigned int i;
  unsigned int crc4=0xffffffff;
  while(len--) {
    crc4^=*p++;
    for(i=0; i<8; i++)
      crc4=(crc4&1) ? (crc4>>1)^CRCPOLY_LE : (crc4>>1);
    }
  crc4^=0xffffffff;
  return crc4;
}

//open serial device
short  ser_open(char* dev)
{
 short i;

 for(i=0;i<8;i++) ser_out[i]=0x88+i*0x11; //set output header
 dev[32]=0; //terminate string
 i=InitPort(dev); //open serial device
 return (short)i; //return result
}

//close serial device
void ser_close(void)
{
 ClosePort();
}

//send 36 byte payload in 48 bytes packet
short ser_send(unsigned char* data)
{
 unsigned int crc;
 short i;
 //check descriptor
 memcpy(ser_out+12, data, 36);  //copy data to output buffer
 crc=ser_crc32(ser_out+12, 36); //compute crc
 ser_out[8]=crc&0xFF;  //copy crc to output buffer
 ser_out[9]=(crc>>8)&0xFF;
 ser_out[10]=(crc>>16)&0xFF;
 ser_out[11]=(crc>>24)&0xFF;
 i=(short)WritePort((char*)ser_out, 48);  //send output buffer to serial device
 return i; //return result
}

//read packet from serial device, return 36 bytes payload or 0
//set async mode if boundary failed
short ser_read(unsigned char* data)
{
 unsigned int crc;
 unsigned int u;
 int len;

 if(ser_sync<0) //sync mode
 {
  len=ReadPort((char*)ser_in+ser_len, 4-ser_len); //read up to 4 bytes header
  if(len<=0) return 0;  //no data readed
  ser_len+=len; //total bytes in buffer
  if(ser_len>=4) //check we have 4 bytes
  {
   u=ser_in[0];
   u<<=8;
   u|=ser_in[1];
   u<<=8;
   u|=ser_in[2];
   u<<=8;
   u|=ser_in[3];
   if(u==0x8899AABB) ser_sync=0;
   if(u==0x99AABBCC) ser_sync=1;
   if(u==0x99AABBCC) ser_sync=2;
   if(u==0x99AABBCC) ser_sync=3;
   if(ser_sync<0) ser_len=0; else ser_len=ser_sync+4; //clear or set shift
  }
 }
 else //work mode
 {
  len=ReadPort((char*)ser_in+ser_len, 48-ser_len); //read up to 48 bytes packet
  if(len<=0) return 0; //no data was read
  ser_len+=len; //total bytes in buffer
  if(ser_len>=48)  //check we have a packet
  {
   ser_len=0; //clear
   if(ser_in[7]!=0xFF) ser_sync=-1;  //check boundary, set async
   else  //boundary OK
   {
    ser_len=0; //clear
    //get crc from received packet
    crc=ser_in[11];
    crc<<=8;
    crc|=ser_in[10];
    crc<<=8;
    crc|=ser_in[9];
    crc<<=8;
    crc|=ser_in[8];
    if(crc!=ser_crc32(ser_in+12, 36)) return 0; //check crc, ingnore wrong
    memcpy(data, ser_in+12, 36); //output packet's payload
    return 36; //return payload lengs
   } //sync ok
  } //if(ser_len>=48)
 }  //work mode
 return 0;
}
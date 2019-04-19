// Contact: <torfone@ukr.net>
// Author: Van Gegel
//
// THIS IS A FREE SOFTWARE
//
// This software is released under GNU LGPL:
//
// * LGPL 3.0 <http://www.gnu.org/licenses/lgpl.html>
//
// You're free to copy, distribute and make commercial use
// of this software under the following conditions:
//
// * You have to cite the author (and copyright owner): Van Gegel
// * You have to provide a link to the author's Homepage: <http://torfone.org/>
//
///////////////////////////////////////////////

   int soundinit(char* device_in, char* device_out);
   int soundgrab(char *buf, int len);
   int soundplay(int len, unsigned char *buf);
   void soundterm(void);
   int getdelay(void);
   int getchunksize(void);
   int getbufsize(void);
   void soundflush(void);
   int soundrec(int on);
   void sounddev(unsigned char dev);

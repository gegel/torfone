///////////////////////////////////////////////
//
// **************************
//
// Project/Software name: X-Phone
// Author: "Van Gegel" <gegelcopy@ukr.net>
//
// THIS IS A FREE SOFTWARE  AND FOR TEST ONLY!!!
// Please do not use it in the case of life and death
// This software is released under GNU LGPL:
//
// * LGPL 3.0 <http://www.gnu.org/licenses/lgpl.html>
//
// You’re free to copy, distribute and make commercial use
// of this software under the following conditions:
//
// * You have to cite the author (and copyright owner): Van Gegel
// * You have to provide a link to the author’s Homepage: <http://torfone.org>
//
///////////////////////////////////////////////


int getdelay(void);
int getchunksize(void);
int getbufsize(void);
int soundinit(char* device_in, char* device_out);
void soundterm(void);
void sound_open_file_descriptors(int *audio_io, int *audio_ctl);
int soundplay(int len, unsigned char *buf);
void soundplayvol(int value);
void soundrecgain(int value);
void sounddest(int where);
int soundgrab(char *buf, int len);
void soundflush(void);
int soundrec(int on);
void sounddev(unsigned char dev);


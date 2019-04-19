
#include "scalarmult.h"
#include "ecc.h"


const unsigned char sqrtminusone[32]={0xB0,0xA0,0x0E,0x4A,0x27,0x1B,0xEE,0xC4,0x78,0xE4,0x2F,0xAD,0x06,0x18,0x43,0x2F,0xA7,0xD7,0xFB,0x3D,0x99,0x00,0x4D,0x2B,0x0B,0xDF,0xC1,0x4F,0x80,0x24,0x83,0x2B};

// Return 1 if v > lim. Return 0 otherwise.
unsigned char gt_than (const unsigned char v[32], const unsigned char lim[32])
{
	unsigned char equal = 1;
	unsigned char gt = 0;
        short i;

	for (i = 31; i >= 0; --i) {
		gt |= ((lim[i] - v[i]) >> 8) & equal;
		equal &= ((lim[i] ^ v[i]) - 1) >> 8;
	}
	return gt;
}


//clear point
void set0(int32_t *dst)
{
    unsigned idx;
    
    for (idx = 0; idx < 10; ++idx)
    dst[idx] = 0;
}

//Square x, n times
void sqrn (int32_t* y, const int32_t* x, int n)
{
    int32_t tmp[10];

    if (n&1)
    {
     sqr(y,x);
     n--;
    }
    else
    {
     sqr(tmp,x);
     sqr(y,tmp);
     n-=2;
    }

    for (; n; n-=2)
    {
     sqr(tmp,y);
     sqr(y,tmp);
    }
}

short elligator_gen(unsigned char *r, unsigned char* x)
{
   int32_t L0[10], L1[10], L2[10], L3[10], Lx[10], La[10], Lb[10];
   short res;

   from_bytes(Lx, r); //Lx = point will be represente

   set0(La); La[0]=486662;  //La = A
   add (La, Lx, La); //La =  Lx + A
   mul(Lb, La, Lx); //Lb = Lx*(Lx+A)
   add (Lb, Lb, Lb);//Lb = 2*Lx(Lx+A)
   set0(L0); //L1=0
   sub(Lb, L0, Lb); //Lb = -2*Lx(Lx+A)

   //try inverse square root from Lb
   sqr (L0, Lb);
   mul (L1, L0, Lb);
   sqr (L0, L1);
   mul (L1, L0, Lb);
   sqrn(L0, L1, 3);
   mul (L2, L0, L1);
   sqrn(L0, L2, 6);
   mul (L1, L2, L0);
   sqr (L2, L1);
   mul (L0, L2, Lb);
   sqrn(L2, L0, 12);
   mul (L0, L2, L1);
   sqrn(L2, L0, 25);
   mul (L3, L2, L0);
   sqrn(L2, L3, 25);
   mul (L1, L2, L0);
   sqrn(L2, L1, 50);
   mul (L0, L2, L3);
   sqrn(L2, L0, 125);
   mul (L3, L2, L0);
   sqrn(L2, L3, 2);
   mul (L0, L2, Lb);   //inv square root of Lb in L0

   //check is sqrt
   sqr (L2, L0); //1/Lb in L2
   mul (L3, L2, Lb); //1 in L3
   //L3 can be 1, -1 or sqrt(-1) or -sqrt(-1), only 1 or -1 are valid points
   to_bytes((uint8_t*)L2, L3);
   res = 1 &  (    ( (uint16_t*)L2 )[1] ^ ( (uint16_t*)L2)[2]     );
   if(res) return 0; //not sqrt: cann't be represent as elligator, try again


  //check is -1
  res=1 & ( (uint16_t*)L2)[2]; //check for minus 1
  from_bytes(L2, sqrtminusone);
  mul (L1, L0, L2); //multiple result with sqrt(-1)
  cswap(L0, L1, res);

  //random choice v coordinate
  mul (La, La, L0); //compute points with positive and negative v coordinates
  mul (Lb, Lx, L0);
  res=x[0]&1; //use random byte to choice v coordinate
  cswap(La, Lb, res);  //candidate in La

  //choice less number fit to 254 bits
  set0(L0); //L1=0
  sub(Lb, L0, La); //Lb = -La
  to_bytes(r, La);
  res=1&((r[31]>>7)|(r[31]>>6));
  cswap(La, Lb, res);

  //output representation with 2 MSB at random
  to_bytes(r, La);
  r[31]&=0x3F;
  r[31]|=(0xC0&(x[0]<<5)); //set random

  return 1;
}


//Hash2Point
//r is 32 bytes hash
//point in Montgomery format
void r2p (unsigned char* p, const unsigned char* r)
{
    int32_t L0[10], L1[10], L2[10], L3[10], Lx[10], Ly[10];
    short res;

    //clear two MSB
    for(res=0;res<8;res++) ((unsigned int*)L1)[res]=((unsigned int*)r)[res];
    ((unsigned char*)L1)[31]&=0x3F;
    from_bytes(L0, (unsigned char*)L1); //L0=r   

    //compute point candidate x=-A/(2r^2+1)
    set0(L1); L1[0]=1;  //L1=1
    sqr (Lx, L0);   //Lx=r^2
    add (Lx, Lx, Lx); //Lx=2r^2
    add (Lx, Lx, L1); //Lx=2r^2+1
    invert(Lx, Lx); //Lx=1/(2r^2+1)
    set0(L1); L1[0]=486662;  //L1=A
    mul(Lx, Lx, L1); //Lx=A/(2r^2+1)
    set0(L1); //L1=0
    sub(Lx, L1, Lx); //Lx=-A/(2r^2+1)
    //point candidate x is in Lx

    //y^2 = x^3 + Ax^2 + x
    sqr (L2, Lx);
    mul (L3, L2, Lx);
    add (L1, L3, Lx);
    set0(Ly); Ly[0]=486662;  //Ly=A
    mul(L2, L2, Ly);
    add (Ly, L1, L2);

    //try inverse square root from y^2
    sqr (L0, Ly);
    mul (L1, L0, Ly);
    sqr (L0, L1);
    mul (L1, L0, Ly);
    sqrn(L0, L1, 3);
    mul (L2, L0, L1);
    sqrn(L0, L2, 6);
    mul (L1, L2, L0);
    sqr (L2, L1);
    mul (L0, L2, Ly);
    sqrn(L2, L0, 12);
    mul (L0, L2, L1);
    sqrn(L2, L0, 25);
    mul (L3, L2, L0);
    sqrn(L2, L3, 25);
    mul (L1, L2, L0);
    sqrn(L2, L1, 50);
    mul (L0, L2, L3);
    sqrn(L2, L0, 125);
    mul (L3, L2, L0);
    sqrn(L2, L3, 2);
    mul (L0, L2, Ly);
    sqr (L2, L0);
    mul (L3, L2, Ly);

    //can be 1, -1 or sqrt(-1) or -sqrt(-1), only 1 or -1 are valid points
    to_bytes((uint8_t*)L2, L3);
    res = 1 &  (    ( (uint16_t*)L2 )[1] ^ ( (uint16_t*)L2)[2]     );

    //select x or -x-A
    set0(L1); //L1=0
    sub(L0, L1, Lx);  //L0=-x
    cswap(L0, Lx, res); //Lx is x or -x
    set0(L0);
    set0(L1); L1[0]=486662;  //Ly=A
    cswap(L1, L0, res);  //L0 is 0 or A
    sub (Lx, Lx, L0);  //Lx is x or -x-A
    to_bytes(p, Lx); //output point
}

void scalarmultbase(unsigned char *p, const unsigned char *n)
{
 unsigned char bp[32]={0,};
 bp[0]=9;
 scalarmult(p, n, bp);
}

//compute point p and it's representative r by scalar x
void p2r(unsigned char *r, unsigned char* x)
{


 short i;
 
  x[1]--;
  for(i=0;i<255;i++)
  {
   x[1]++; //modify scalar while select point can be represented as elligator
   scalarmultbase(r, x); //compute point from scalar  (x not modified)
   if(elligator_gen(r, x)) break;
  }
}


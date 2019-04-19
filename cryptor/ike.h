#define EVENT_LEN 4 //len of event notification packet
#define IKE_LEN 36  //len of IKE packet remotely send/receive
#define IKE_END 16  //input counter value for go to work mode 


//helpers
void cr_memclr( void *v, short n ); //safe clear memory
unsigned char ike_test(void); //test validity of crypto primitives
void ike_rand(unsigned char* data, short len); //generate random
unsigned char ike_cmp(unsigned char* keya, unsigned char* keyb); //compare public keys

//init
void ike_create(void); //create IKE
void ike_reset(void);  //reset IKE
unsigned char ike_test(void); //test crypro
//interface
short ike_rep(unsigned char* data); //request for representation
short ike_obf(unsigned char* data); //set obfuscation in pre-IKE
short ike_run_originator(unsigned char* data); //run IKE by originator
short ike_run_acceptor(unsigned char* data);  //run IKE by acceptor
short ike_r(unsigned char* data); //IKE loop
//internal IKE procedures
short ike_2(unsigned char* data);
short ike_3(unsigned char* data);
short ike_4(unsigned char* data);
short ike_5(unsigned char* data);
short ike_6(unsigned char* data);
short ike_7(unsigned char* data);
short ike_8(unsigned char* data);
short ike_9(unsigned char* data);
short ike_10(unsigned char* data);
short ike_11(unsigned char* data);
short ike_12(unsigned char* data);
short ike_13(unsigned char* data);
short ike_14(unsigned char* data);
short ike_15(unsigned char* data);
//symmetric crypto in work mode
short encrypt(unsigned char* pkt);
short decrypt(unsigned char* pkt);


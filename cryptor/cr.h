



typedef struct {
 unsigned char sid[16]; //random seed
 unsigned char A[32]; //our pub
 unsigned char a[32]; //test only

 unsigned char x[32]; //efemeral secret
 unsigned char Y[32]; //their y
 unsigned char S[32]; //DH secret
 unsigned char B[32]; //their pup
 unsigned char T[32]; //tag secret
 unsigned char C[32]; //commitment
 unsigned char U[32]; //ID protecting secret
 unsigned int cnt_in; //counter of incoming packets
 unsigned int cnt_out; //counter of outgoing packets
 unsigned short sas;   //session authenticator
 unsigned short keystep; //counter of key send and speke steps
 unsigned short speke;  //speke flag
 unsigned char ans;
}ct_data;


//extern ct_data ct;

short cr_rst(unsigned char* data);  //local reset from CR and UI
short cr_term(unsigned char* data); //remote reset event from TR


short cr_event_net(unsigned char* data);
short cr_event_connect(unsigned char* data);
short cr_event_accept(unsigned char* data);
short cr_event_run(unsigned char* data);
short cr_event_apply(unsigned char* data);
short cr_event_note(unsigned char* data);
short cr_event_direct(unsigned char* data);
short cr_event_ROK(unsigned char* data);


short cr_st_rnd(unsigned char* data);
short cr_st_reqQ(unsigned char* data);
short cr_st_Q(unsigned char* data);
short cr_st_A(unsigned char* data);
short cr_st_B(unsigned char* data);
short cr_st_T(unsigned char* data);
short cr_st_R(unsigned char* data);

short cr_ui_pth(unsigned char* data);
short cr_ui_adr(unsigned char* data);
short cr_ui_mute(unsigned char* data);
short cr_ui_direct(unsigned char* data);
short cr_ui_psw(unsigned char* data);
short cr_ui_call(unsigned char* data);
short cr_ui_tcpinit(unsigned char* data);
short cr_au(unsigned char* data);


short process_tr(unsigned char* data);
short process_st(unsigned char* data);
short process_ui(unsigned char* data);

short ct_create(unsigned char* data);
short cr_loop(void);

short cr_process(unsigned char* data, short inlen);

 short cr_set_key(unsigned char* data);
  short cr_add_key(unsigned char* data);
  short cr_ans(unsigned char* data);
 unsigned int cr_crc32(unsigned char const *p, unsigned int len);




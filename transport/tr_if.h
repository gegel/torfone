//called from cr_if for set type of usage TR module of this torfine
short tr_set_ser(unsigned char* data);
//called by other if for PASS data to TR and modules connected to TR (possible UI)
void tr_send_to_tr(unsigned char* data, short len);
//call from main loop for processing TR data
short tr_loop(void);


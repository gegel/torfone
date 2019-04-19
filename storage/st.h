#include "shake.h"

typedef struct {
//data of Storage module 
 unsigned char book_psw[16]; //password for adress fields access
 unsigned char contact[128]; //temporary contact data
 unsigned char address[64];  //current adress field
 unsigned char secret[32];   //our secret key
 unsigned char stseed[16]; //internal PRNG seed
 unsigned short current_field; //current contact
 unsigned short last_field;    //last not-empty contact
 //sha3_ctx_t st_shake;          //shake context
 } st_data;


//-----------------helpers------------------------
void st_rand(unsigned char* data, short len); //get random value
void st_encrypt(unsigned char* data);  //encrypt contact too book
short st_decrypt(unsigned char* data); //decrypt contact from book
//---------------interface-----------------------
short st_process(unsigned char* st_buf, short len); //process data sended to Storage module
//--------------internal commands---------------
//init
short st_storage_init(unsigned char* data); //ST_PTH -> CR_RND
short st_set_password(unsigned char* data);  //ST_PSW -> CR_PUB
short st_get_next_name(unsigned char* data); //SR_REQ -> UI_LST
short st_reset(unsigned char* data);    //ST_RST ->CR_RST
//book search
short st_get_addr_by_name(unsigned char* data); //ST_GETADR -> UI_ADR
short st_search_by_name(unsigned char* data); //ST_GETKEY -> CR_KEY (input name, output key/zero)
short st_search_by_key(unsigned char* data); //ST_GETNAME ->UI_NAME (input key, output name/empty)
//book edit
short st_set_address(unsigned char* data);  //ST_ADR -> UI_ADROK
short st_save_contact(unsigned char* data); //ST_SAVE -> UI_SAVEOK
short st_new_contact(unsigned char* data); //ST_NEW -> UI_NEWOK
short st_copy_contact(unsigned char* data); //ST_COPY -> UI_COPYOK
short st_delete_contact(unsigned char* data); //ST_DEL -> UI_DELOK
short st_save_key(unsigned char* data);      //ST_ADD ->  UI_KEY
//use prbate key
short st_get_sec(unsigned char* data);   //ST_SEC -> CR_SEC
//crc
unsigned int st_crc32(unsigned char const *p, unsigned int len);
unsigned char st_test(void);














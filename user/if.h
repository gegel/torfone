

#ifdef __cplusplus
extern "C" {
#endif


//==================Data packet general types================
#define TYPE_MASK 0xE0
#define TYPE_TRCMD 0x10

#define TYPE_CR 0x80
#define TYPE_UI 0xC0
#define TYPE_ST 0xA0
#define TYPE_TR 0xE0


//======================Transport===========================

//output event types output by TR and processed in CR
#define EVENT_ACCEPT_TCP    0xF0 //accept new incoming from WAN
#define EVENT_ACCEPT_TOR    0x1F0 //accept new incoming from Tor

#define EVENT_READY_OUT     0xF1 //outgoing connecting ready
#define EVENT_READY_TCP     0x1F1 //incoming TCP connecting ready
#define EVENT_READY_TOR     0x2F1 //incoming TOR connecting ready
#define EVENT_CLOSE_OUT     0x3F1 //close outgoing connecting
#define EVENT_CLOSE_IN      0x4F1 //close incoming connecting

#define EVENT_TERMINATED    0xF2 //connecting terminated

#define EVENT_DIRECT_REQ    0xF3 //direct connecting allowed by remote
#define EVENT_DIRECT_DENY   0x1F3 //direct connecting deny by remote
#define EVENT_DIRECT_ACTIVE 0x2F3 //direct connecting active

#define EVENT_INIT_OK       0xF4  //transport initialization is OK
#define EVENT_INIT_FAIL     0x1F4  //network fail

#define EVENT_CALL_TCP      0xF5  //outgoing call over TCP in progress
#define EVENT_CALL_TOR      0x1F5  //outgoing call over Tor in progress

#define EVENT_BUSY          0xF6  //unwanted incoming call

#define EVENT_DOUBLING_IN   0xF7  //incoming doubling connecting
#define EVENT_DOUBLING_OUT  0x1F7  //outgoing doubling connecting

#define EVENT_SEED_OK       0xF8 //seeding OK

#define EVENT_SEC           0xF9   //secret applied
#define EVENT_SET           0x1F9  //secret set but not aplied yet
#define EVENT_REQ           0x2F9  //remote request for secret

#define EVENT_OBF_OUT       0xFA   //request of representation (originator side)
#define EVENT_OBF_IN        0x1FA   //request of representation (acceptor side)

//remote data outputs by TR and processed in CR
#define REMOTE_R             0xFB //data from remote: their representation
#define REMOTE_A             0xFC //data from remote: reserv
#define REMOTE_Q             0xFD //data from remote: data for IKE
#define REMOTE_K             0xFE //data from remote: data for key pass init
#define REMOTE_S             0xFF //data from remote: data for SPEKE init


//Command processed in TR
#define TR_INIT 0xE0   //bind transport module
#define TR_SET 0xE1    //set/reset call
#define TR_DIR 0xE2    //set/reset direct mode
#define TR_SEC 0xE3    //set coomon secret
#define TR_SEED 0xE4   //set seed
#define TR_RST  0xE5   //reset call
#define TR_MUTE 0xE6  //mute voice queue
#define TR_REP  0xE7  //our representation
#define TR_OBF  0xE8  //shared keymaterial for obfuscation

//Date send to remote by TR
#define TR_A 0xEC   //data for remote: reserv
#define TR_Q 0xED   //data for remote: IKE
#define TR_K 0xEE   //data for remote: key pass init
#define TR_S 0xEF   //data for remote: data for SPEKE init


//Channel types
#define CHANNEL_DEF 0 //set channel type is default
#define CHANNEL_UDP 1  //set channel type is UDP
#define CHANNEL_TCP 2  //set channel type is TCP
#define CHANNEL_TOR 3  //set channel type is TOR


//==========================Cryptor===============================

//Packets processed in Cryptor module
#define CR_PTH   0x80 //serial device+pin or book file path from UI
#define CR_RND   0x81 //seed PRNG from ST
#define CR_PSW   0x82 //book password from UI
#define CR_PUB   0x83 //our public key from ST
#define CR_KEY   0x84 //their public key from ST
#define CR_SEC   0x85 //Secret Tag from ST
#define CR_SPEKE 0x86 //speke common password from UI
#define CR_REQK  0x87 //key adress for output from UI
#define CR_ADDK  0x88 //key allow for saving from UI
#define CR_MUTE  0x89 //set audio mode from UI
#define CR_RST   0x8A //reset from UI
#define CR_AUD   0x8C //audio devices from UI

//Lens of data answered by Cryptor
#define CR_RND_LEN 36   //rand
#define CR_PUB_LEN 36   //our pubA
#define CR_SEC_LEN 36   //mult result
#define CR_KEY_LEN 36   //key by name
#define UI_NAME_LEN 36  //name by key
#define UI_ADR_LEN 36   //address by name
#define UI_LST_LEN 36   //next name
#define UI_ADROK_LEN 36 //current name
#define UI_KEY_LEN 36   //new name
#define UI_SAVEOK_LEN 36 //no answer

//Authentication masks
#define CR_AU_FAIL 0xFE
#define CR_DH_FAIL 0xFF


//==========================Storage=====================================

//commands processed in ST
#define ST_PTH    0xA0 //set book path[32], return rand CR_RND
#define ST_PSW    0xA1 //set storage password[32], return pubA[32] CR_PUB
#define ST_SEC    0xA2 //put sec[32], return res[32]  CR_SEC
#define ST_REQ    0xA3 //request nex book iteam [0], return name[16] UI_LIST
#define ST_GETADR  0xA4//request adress by name[16], return address[32] UI_ADR
#define ST_GETKEY  0xA5//request key by name[16], return key[32]   CR_KEY
#define ST_GETNAME 0xA6//return name by key[32], return name[16]   UI_NAME
#define ST_ADR   0xA7//set current adress field[32], return [1] UI_ADROK
#define ST_SAVE  0xA8//save current contact with name[16] to name+16[16] returns [1] UI_SAVEOK
#define ST_ADD   0xA9//save received key[32], return [16] UI_KEY
#define ST_RST  0xAA//reset storage[0], return UI_RST
#define ST_NEW  0xAB //create new contact by name[16], return UI_NEWOK
#define ST_DEL 0xAC  //delete contact by name[16], return UI_DELOK
#define ST_COPY 0xAD //create copy of contact name[16] to name+16[16], return UI_COPYOK

//len of data returnd by Storage  
#define ST_PTH_LEN 36  //path[32]
#define ST_PSW_LEN 36 //psw[32]
#define ST_SEC_LEN 36 //Y[32]
#define ST_REQ_LEN 4  //req next
#define ST_GETADR_LEN 20 //name[16]
#define ST_GETKEY_LEN 20 //name[16]
#define ST_GETNAME_LEN 36 //B[32]
#define ST_ADR_LEN 36 //adr[32]
#define ST_SAVE_LEN 20 //name[16]
#define ST_ADD_LEN 36 //key[32]
#define ST_RST_LEN 4 //rst

//Error codes of storage
#define STORAGE_OK 0
#define STORAGE_TRNG_ERR 1
#define STORAGE_UNINIT 2
#define STORAGE_NOBOOK 3
#define STORAGE_NOSECRET 4
#define STORAGE_NOTFIND 5
#define STORAGE_RDFAIL 6
#define STORAGE_MACFAIL 7
#define STORAGE_NONAME 8
#define STORAGE_WRFAIL 9
#define STORAGE_NOSPACE 10
#define STORAGE_DELETE 11
#define STORAGE_EXIST 12
#define STORAGE_UPDATE 13
#define STORAGE_CREATE 14
#define STORAGE_NOTAG 15
#define STORAGE_MYSELF 16
#define STORAGE_COPY 17


//==========================GUI==================================

//commands processed in UI
#define UI_INI     0xC0 //request for password
#define UI_FGP     0xC1 //set key fingerprint field, return [0]
#define UI_TCP     0xC2 //tcp state [1], return [0]
#define UI_LST     0xC3 //set next name[16] in list, return [0]

#define UI_ADR     0xC4 //set adr[32] to adr field, return [0]
#define UI_ADROK   0xC5 //answer: adress field is setted, return [0]
#define UI_NEWOK   0xC6 //create new contact
#define UI_SAVEOK  0xC7 //answer: contact saved, return [0]

#define UI_REQ     0xC8 //request key for output
#define UI_KEY     0xC9 //fingerprint of added key[16]
#define UI_SPREQ   0xCA //remote SPEKE request
#define UI_SPEKE   0xCB //set speke result [1], return [0]

#define UI_OUT     0xCC //notification of outgoing call handshake
#define UI_INC     0xCD //notification of incoming call handshake
#define UI_NAME    0xCE //set name[16] to name edit, return [0]
#define UI_CALL    0xCF //au result of outgoing call

#define UI_DBL     0xD0 //autentification of outgoing double call handshake
#define UI_DIR     0xD1 //direct mode change notification
#define UI_RST     0xD2 //reset
#define UI_COPYOK  0xD4 //report of create copy of contact
#define UI_DELOK   0xD5 //report of delete contact
#define UI_ACCEPT  0xD6 //accept call//try to connect
#define UI_ANSWER  0xD7 //callee answer for caller

//GUI buttons
#define UI_BUTTON_MENU 0
#define UI_BUTTON_DIRECT 1
#define UI_BUTTON_CALL 2
#define UI_BUTTON_CANCEL 3
#define UI_BUTTON_LIST 4
#define UI_BUTTON_SAVE 5

//GUI colors
#define UI_COLOR_BW 0
#define UI_COLOR_B 1
#define UI_COLOR_G 2
#define UI_COLOR_R 3




//======================Interface=========================

//interface called from GUI thread modules for get data
short if_getgui(unsigned char* data);
//interface called from GUI thread modules for send data
void if_setgui(unsigned char* data, short len);


//called from main module on start
int au_create(char* audioin, char* audioout);
short ct_create(unsigned char* data);
void tr_create(unsigned char* data);


//called from main loop for run modules
short ui_loop(void); //process incoming UI data
short cr_loop(void);
short st_loop(void);
short tr_loop(void);
short au_loop(void);






#ifdef __cplusplus
}
#endif

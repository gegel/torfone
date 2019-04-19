
#ifndef UI_H
 #define UI_H


#ifdef __cplusplus
extern "C" {
#endif




//configuration structure
typedef struct {
 char myadr[32];
 char tcp[8];
 char tor[8];
 char stun[32];
 char com[32];
 char mike[16];
 char spk[16];
 unsigned char wan;
 } ui_conf;

 
 void bk_init_path(char* path);                      //init path to book folder
 
//user notifications (interface UI -> GUI)

void gui_rdconf(void* cf); //get configuration from file
void gui_setinfo(char* text, unsigned char color); //set info text
void gui_setfgp(char* text, unsigned char color); //set fgp text
void gui_setsas(char* text, unsigned char color); //set sas text
void gui_setname(char* text, unsigned char color); //set name field
void gui_setaddr(char* text, unsigned char color); //set address field

void gui_setnew(unsigned char result); //set result of creating contact
void gui_setchange(unsigned char result); //set rersult of changing contact
void gui_setspeke(unsigned char result); //set result of speke
void gui_setkey(unsigned char result); //set result of key receiving
void gui_setdir(unsigned char result); //set result of direct mode
void gui_setdbl(unsigned char result); //set result of dobling mode

void gui_addlist(char* name);
void gui_dellist(char* name);

//void gui_setstate(unsigned char state);

void gui_setiddle(void); //set iddle state
void gui_setaccept(unsigned char in, unsigned char tor);  //set state after accepting (pre-IKE)
void gui_setcall(unsigned char in, unsigned char tor);  //set work state (in call)
void gui_setterm(void); //set call termination state



//user commands (interface GUI -> UI)
void ui_doinit(char* password);  //create UI
void ui_dotimer(void); //process data sended to UI in context of GUI thread
void ui_donew(char* newname);  //create new contact
void ui_dospeke(char* secret); //run SPEKE
void ui_doaddr(char* addr);  //set current address
void ui_dolist(char* mask); //request list output by mask
void ui_dochange(char* name, char* addr);  //change current contact
void ui_docall(char* name, char* addr); //outgoing call
void ui_domute(unsigned char on); //mute/talk
void ui_dodirect(unsigned char on); //allow/deny direct p2p mode
void ui_docancel(void); //terminate call
void ui_doselect(char* name);  //select contact
void ui_dokey(char* name);  //send key to remote
void ui_dokeyrcvd(unsigned char on); //aloow receiving keys

void ui_setcommand(unsigned char cmd); //add command for GUI to pipe
unsigned char ui_getcommand(char* par); //extract command for gui from pipe
void ui_setconfig(void* cfg); //set GUI configuration to UI 
void tf_loop(void); //application main loop 
#ifdef __cplusplus
}
#endif





//incoming data processing
//init
void ud_getpsw(unsigned char* data);
void ud_setfgp(unsigned char* data);
void ud_setnet(unsigned char* data);
void ud_addlst(unsigned char* data);
//contact managment
void ud_setadr(unsigned char* data);
void ud_addnew(unsigned char* data);
void ud_addadr(unsigned char* data);
void ud_setupd(unsigned char* data);
void ud_setcpy(unsigned char* data);
void ud_setdel(unsigned char* data);
//key send
void ud_setkey(unsigned char* data);
void ud_getkey(unsigned char* data);
//speke
void ud_getsec(unsigned char* data);
void ud_setsec(unsigned char* data);
//call
void ud_setacc(unsigned char* data);
void ud_setout(unsigned char* data);
void ud_setinc(unsigned char* data);
void ud_setname(unsigned char* data);
void ud_setcall(unsigned char* data);
//in call
void ud_setans(unsigned char* data);
void ud_setdbl(unsigned char* data);
void ud_setdir(unsigned char* data);
void ud_setrst(unsigned char* data);

 typedef struct {
 unsigned char sid[32]; //PRNG sid
 unsigned char buf[68]; //inbuffer for packet
 unsigned char out[68]; //output buffer
 char name[16]; //name of selected contact
 char addr[32]; //address of selected contact
 unsigned char iscall;  //flag of connecting active
 unsigned char istalk;  //flag of call setup compleet
 unsigned char iskey; //flag of allow key receiving
 unsigned char isset; //flag of contact was set
 unsigned char in;
 unsigned char tor;
 } ui_data;



 //configuration structure
typedef struct {
 char list[16];  //gui list entry
 char del[16];   //gui list delete entry
 char name[16];  //gui name field
 char addr[32];  //gui addr field
 char fgp[16];   //gui fgp label
 char sas[8];    //gui sas label
 char info[32];  //gui info label
 char note[32]; //gui notification
 unsigned char cmd[16]; //ui-to-gui commands pipe
 unsigned char in; //in command pointer
 unsigned char out; //out command pinter
 } gui_data;



#define SLEEP_IDDLE 100 //time in mS for suspent Torfone thread while not actibe in iddle
#define SLEEP_TIME 1  //time in mS for suspent Torfone thread while not actibe in call
#define SLEEP_DELAY 100 //count of loops of Torfone thread before set not active state
#define SLEEP_CMD 50 //count of loops of Torfone thread after GUI command

 //GUI edit colors
 #define GUI_COLOR_W 0  //black/white
 #define GUI_COLOR_B 1  //blue
 #define GUI_COLOR_G 2  //green
 #define GUI_COLOR_R 3  //red
//---------Top Buttons--------
//GUI SAVE and NEW icons
 #define GUI_SAVE_NO 0
 #define GUI_SAVE_OK 2
 #define GUI_SAVE_REQ 1
 #define GUI_SAVE_FAIL 3
//GUI key icons
 #define GUI_KEY_NO 0
 #define GUI_KEY_REQ 1
 #define GUI_KEY_OK 2
 #define GUI_KEY_FAIL 3
 //GUI speke icons
 #define GUI_SPEKE_NO 0
 #define GUI_SPEKE_REQ 1
 #define GUI_SPEKE_OK 2
 #define GUI_SPEKE_FAIL 3
//---------Bottom buttons----------
 //GUI direct icons
 #define GUI_DIR_DENY 0
 #define GUI_DIR_REQ 1
 #define GUI_DIR_OK 2
 #define GUI_DIR_ALLOW 3
 //GUI double icons
 #define GUI_TERM 0
 #define GUI_DBL_IN  1
 #define GUI_DBL_BOTH 2
 #define GUI_DBL_OUT 3



 //========================================================
 //                 GUI data definnitions
 //========================================================


 //set text
 #define GUI_NAME_TEXT 1
 #define GUI_ADDR_TEXT 2
 #define GUI_FGP_TEXT 3
 #define GUI_SAS_TEXT 4
 #define GUI_INFO_TEXT 5
 #define GUI_LIST_ADD 6
 #define GUI_LIST_DEL 7

 //set text color
 #define GUI_NAME_W  8
 #define GUI_NAME_G  9
 #define GUI_ADDR_W  10
 #define GUI_ADDR_G  11
 #define GUI_FGP_W   12
 #define GUI_FGP_G   13

 #define GUI_INFO_W  16
 #define GUI_INFO_B  17
 #define GUI_INFO_G  18
 #define GUI_INFO_R  19

 #define GUI_BTNNEW_W 20
 #define GUI_BTNNEW_B 21
 #define GUI_BTNNEW_G 22
 #define GUI_BTNNEW_R 23

 //bunnon new/speke color
 #define GUI_BTNSPK_W 24
 #define GUI_BTNSPK_B 25
 #define GUI_BTNSPK_G 26
 #define GUI_BTNSPK_R 27

 //button change color
 #define GUI_BTNCHG_W  28
 #define GUI_BTNCHG_B  29
 #define GUI_BTNCHG_G  30
 #define GUI_BTNCHG_R  31

 //button key color
 #define GUI_BTNKEY_W  32
 #define GUI_BTNKEY_B  33
 #define GUI_BTNKEY_G  34
 #define GUI_BTNKEY_R  35

 //button call/mute color
 #define GUI_BTNCLL_W  36
 #define GUI_BTNCLL_B  37
 #define GUI_BTNCLL_G  38
 #define GUI_BTNCLL_R  39

 //button list/direct color
 #define GUI_BTNDIR_W  40
 #define GUI_BTNDIR_B  41
 #define GUI_BTNDIR_G  42
 #define GUI_BTNDIR_R  43

 //button menu/cancel color
 #define GUI_BTNTRM_W  44
 #define GUI_BTNTRM_B  45
 #define GUI_BTNTRM_G  46
 #define GUI_BTNTRM_R  47

 //set states
 #define GUI_STATE_IDDLE 48
 #define GUI_STATE_IKE   49
 #define GUI_STATE_CALL  50
 #define GUI_STATE_TERM  51
 #define GUI_NOTE_TEXT  52
 #define GUI_NOTE_ANS  53

 #endif //UI_H




//called from GUI therad
void if_setgui(unsigned char* data, short len); //send data to Torfone
short if_getgui(unsigned char* data); //check having and get data from Torfone

//called from Torfone thread
void ui_send_to_ui(unsigned char* data, short len); //send data to GUI
short ui_loop(void); //process incoming UI data

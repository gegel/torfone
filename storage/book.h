//Platform-depend procedures
void bk_set_pws(unsigned char* p);                  //set obfuscation secret
void bk_init_path(char* path);                      //init path to book folder
int bk_set_path(char* path);                        //set name of book file
void bk_mask(unsigned char* data);                  //mask/unmask book data
int bk_read_field(int field, unsigned char* data);  //read 128-bytes field
int bk_save_field(int field, unsigned char* data);  //save 128-bytes field
int bk_delete_field(int field);                     //erise 128-bytes field
int bk_search_last(void);                           //search last non-empty field
int bk_search_empty(void);                          //search first empty field
int bk_search_by_name(char* name);                  //search field by name
int bk_search_by_key(unsigned char* key);           //search field by key
int bk_load_secret(unsigned char* data);            //load private key from file



//gcrecent.h
//v1.04	

#ifndef gcrecent_h
#define gcrecent_h



#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>

#include <string>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

using namespace std;


//#define _LARGE_FILES
#define _FILE_OFFSET_BITS 64			//large file handling



//windows code
#ifdef compile_for_windows
#include <windows.h>
#include <process.h>
#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>
#include <conio.h>
#include <shlobj.h>
#include <objbase.h>
#endif


//linux code
#ifndef compile_for_windows
#include <dirent.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <langinfo.h>
#include <pthread.h>
#endif


#include "GCProfile.h"






class gcrecent
{
private:
int max_entries;
vector<string>vstr;


public:



public:
gcrecent( int num );

int count( );
void clear_all( );
int add_entry( string sadd, bool dont_repeat, bool make_first );
bool get_entry ( string &sget, int idx );
int find_match( string &sfind );

bool make_first_if_a_match( string &sfirst );

bool save_settings( string ini_fname, string section, string key );
int load_settings( string ini_fname, string section, string key, string def, bool dont_repeat, bool make_first, int max );
};



#endif

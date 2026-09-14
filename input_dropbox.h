/*
Copyright (C) 2025 BrerDawg

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


//input_dropbox.h
//v1.07

#ifndef input_dropbox_h
#define input_dropbox_h

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501         //need this for GetFilesizeEx
#endif

#define _FILE_OFFSET_BITS 64	    //large file handling, must be before all #include...

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <string>
#include <vector>
#include <ctype.h>
#include <wchar.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <algorithm>

#define __STDC_FORMAT_MACROS			//needed this with windows/msys/mingw for: PRIu64
#include <inttypes.h>



//---------------- 
//linux code
#ifndef compile_for_windows
#define _FILE_OFFSET_BITS 64			//large file handling
//#define _LARGE_FILES
#include <dirent.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <syslog.h>		//MakeIniPathFilename(..) needs this

#endif
//----------------

//---------------- 
//windows code
#ifdef compile_for_windows
#include <windows.h>
#include <process.h>
#include <winnls.h>
#include <share.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>
#include <conio.h>
#define WC_ERR_INVALID_CHARS 0x0080		//missing from gcc's winnls.h
#endif
//---------------- 


#include <FL/Fl.H>
//#include <FL/Fl_Window.H>
//#include <FL/Fl_Box.H>
#include <FL/Enumerations.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Input_Choice.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Menu_Window.H>
#include <FL/Fl_Overlay_Window.H>

#include "GCProfile.h"



//linux code
#ifndef compile_for_windows
#define LLU "llu"
#endif


//windows code
#ifdef compile_for_windows
#define LLU "I64u"
#endif




using namespace std;


class input_dropbox_popup;





class input_dropbox : public Fl_Window 									// !!!!!!!! does not support a label, use an external Fl_Box for labelling

{
private:
bool ctrl_key;
bool shift_key;

int mousex, mousey;
int mousewheel;



//bool right_but_drag;
//bool left_button, right_button;

//int grab_right_drag_x, grab_right_drag_y;
//double grab_right_drag_val;


public:
Fl_Input* fi_text;														//add a callback to this obj, e.g:
																		//idb_search->fi_text->callback( (void*)cb_idb_search_fi_text, (void*)0 );
																		//idb_search->fi_text->when( FL_WHEN_ENTER_KEY | FL_WHEN_NOT_CHANGED );
Fl_Button *bt_dropdown;
input_dropbox_popup* dropdown;
int posx, posy;
int normal_hei;

bool expand;
int expand_wid, expand_hei;
int textfont, textsize;
int col_text_r, col_text_g, col_text_b;
int col_text_hov_r, col_text_hov_g, col_text_hov_b;
int col_hov_r, col_hov_g, col_hov_b;
int col_bkgd_r, col_bkgd_g, col_bkgd_b;

int hover_idx;

int line_hei;
int scroll_line;
int dropbox_posy;
int dropbox_line0_posy;

vector<int>vposx;
vector<int>vposy;

//bool inside;
vector<string> vstr;

int page_up_step;
int page_down_step;

bool can_delete;
bool ignore_blank_entries;
bool ingore_duplicate;													//v1.04
bool ignore_case;														//v1.04 when searching 
bool sort_when_adding;													//v1.04
bool sort_descending;													//v1.04
bool bring_last_selection_to_top;										//v1.04
bool allow_dropdown_to_float_y;											//v1.05, allows the dropdown to float upward if its expanded bottom edge is below parent's bottom edge

void (*p_callback)( void *obj_in, void *args_in ); 	//this callback is called when a dropdown is selected (and not when editing 'fi_text' obj, add a seperate callback for 'fi_text', as shown above )
void *cb_obj;															//e.g: idb_search->set_callback( cb_idb_search, (void *)idb_search, (void *)0 );
void *cb_args;



private:
void draw();
int handle(int);


public:
input_dropbox(int x,int y,int w, int h,const char *label=0);
void expand_adj( bool expand );
void expand_max( int maxx, int maxy );
void add( string ss );
void clear();
int get_hover_idx( int xx, int yy );
const char* value();
void value( string ss );
void select_entry( unsigned int idx );
bool erase_entry( unsigned int idx );

void scroll_into_view( unsigned int idx );
void set_callback( void (*p_cb)( void*, void* ), void *obj_in, void *args_in );
void add_at_head( string ss );											//v1.03
void trim_entry_count_to( unsigned int cnt, bool b_from_head );			//v1.03
int search( string ss, bool ignorecase );								//v1.04
void sort( bool bdesending );											//v1.04
int set_with_c_esc_str( string ss, bool clear_first );					//v1.06
void get_as_c_esc_str( string &ss );									//v1.06


/*
void (*left_click_p_cb)( My_Input_Wheel *wdj, void *args );
void *left_click_cb_args;

void (*left_double_click_p_cb)( My_Input_Wheel *wdj, void *args );
void *left_double_click_cb_args;


void (*right_click_p_cb)( My_Input_Wheel *wdj, void *args );
void *right_click_cb_args;

void (*right_double_click_p_cb)( My_Input_Wheel *wdj, void *args );
void *right_double_click_cb_args;


void (*mousewheel_p_cb)( My_Input_Wheel *wdj, void *args, int delta );
void *mousewheel_cb_args;


void (*left_click_release_p_cb)( My_Input_Wheel *wdj, void *args );
void *left_click_release_cb_args;

void (*right_click_release_p_cb)( My_Input_Wheel *wdj, void *args );
void *right_click_release_cb_args;
*/

};








class input_dropbox_popup : public Fl_Menu_Window  						//don't use this class directly, its only for 'class input_dropbox'
{
private:


public:
input_dropbox* wdg_owner;
bool mouse_move_scrolling;												//v1.04



private:
void draw();
int handle(int);


public:
input_dropbox_popup( int x,int y,int w, int h,const char *label=0 );
void set_owner( input_dropbox* ow );


};


#endif

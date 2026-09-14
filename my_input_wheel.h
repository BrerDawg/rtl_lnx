/*
Copyright (C) 2022 BrerDawg

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


//my_input_wheel.h
//v1.12

#ifndef my_input_wheel_h
#define my_input_wheel_h

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

//#define compile_for_windows	0	//!!!!!!!!!!! uncomment for a dos/windows compile

//#define compile_for_windows		//!!!!!!!!!!! uncomment for a dos/windows compile

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


struct st_miw_step_wheel_boundary_tag
{
int left;
int right;
};





//--------
class My_Input_Wheel : public Fl_Input 
{
private:
bool ctrl_key;
bool shift_key;
void (*p_callback)( void *obj_in, void *args_in ); 
void *cb_obj;
void *cb_args;
int mousex, mousey;
int mousewheel;
bool right_but_drag;
bool left_button, right_button;

int grab_right_drag_x, grab_right_drag_y;
double grab_right_drag_val;


public:
string s_printf_format;
bool force_integer;                     		//set this only if you use '%d' in printf_format and s_printf_format

bool b_take_focus_on_inside;
bool b_allow_wheel_inc;
bool b_invert_wheel;							//if set, turning mousewheel towards user takes value up
double std_wheel_step;
double ctrl_wheel_step;
double shift_wheel_step;
double ctrl_shift_wheel_step;

st_miw_step_wheel_boundary_tag step_wheel_boundary[4];	//loaded with 4 bounded sections across control's x dimension
bool b_step_wheel_side_left;					//these are overruled if shift or ctrl keys are down
bool b_step_wheel_side_left_center;
bool b_step_wheel_side_right_center;
bool b_step_wheel_side_right;

float step_wheel_side_left;
float step_wheel_side_left_center;
float step_wheel_side_right_center;
float step_wheel_side_right;

int step_wheel_boundary_idx;					//index to 'step_wheel_boundary[]' is governed by mouse x pos within gui control


bool b_use_limit_max;
bool b_use_limit_min;
double limit_max;
double limit_min;
string slabel;
string stooltip;
int id, id2, id3;
bool paired_obj;								//this is set when 'My_Input_Wheel_Packable' use this obj as a child

bool allow_right_but_drag;
double right_drag_x_val_change_factor;			//e.g: 'limit_max' / 200.0f, plus is right, use minus val to invert dir (200 pixel mouse move would hit 'lim_max', this is dependant on the cur val gabbed on initial mouse butt down )
double right_drag_y_val_change_factor;			//e.g: 'limit_max' / 200.0f, plus is up, use minus val to invert dir
bool inside;
bool focused;
Fl_Color col_bkg, col_bkg_focus, col_bkg_hover, col_bkg_hover_focus, col_bkg_modified, col_bkg_hover_modified;

bool b_show_modified;							//set this to show if modified but 'enter key' not yet been pressed
bool b_modified;								//set if keydown has happened and still to have the 'enter key' pressed
int keycode_last;								//used for 'keydown_p_cb'  'keyup_p_cb'


private:
int handle(int);

public:
My_Input_Wheel(int x,int y,int w, int h,const char *label=0);
const char* value();
void value( string ss );
void value( string *ss );
void enforce_limits();
void enforce_limits_var( double &dd );
void enforce_limits( const char* printf_format );
void set_value_from_double( double dd );
void set_value_from_double( double dd,  const char* printf_format );
void set_value_from_str( string ss );
void set_value_from_str( string ss,  const char* printf_format );

double get_value_as_double();
void set_left_click_cb( void (*p_cb)( My_Input_Wheel *, void* ), void *args );
void set_left_double_click_cb( void (*p_cb)( My_Input_Wheel *, void* ), void *args );
void set_right_click_cb( void (*p_cb)( My_Input_Wheel *, void* ), void *args );
void set_right_double_click_cb( void (*p_cb)( My_Input_Wheel *, void* ), void *args );
void set_mousewheel_cb( void (*p_cb)( My_Input_Wheel *, void*, int ), void *args );
void set_callback( void (*p_cb)( void*, void* ), void *obj_in, void *args_in );
void set_left_click_release_cb( void (*p_cb)( My_Input_Wheel *, void* ), void *args );
void set_right_click_release_cb( void (*p_cb)( My_Input_Wheel *, void* ), void *args );
void set_keydown_cb( void (*p_cb)( My_Input_Wheel *, void* ), void *args );
void set_keyup_cb( void (*p_cb)( My_Input_Wheel *, void* ), void *args );
void mousewheel_external( int step_wheel_boundary_idx_in, int dir, bool ctrl_key0, bool shift_key0 );
void step_wheel_boundary_build();
int step_wheel_boundary_find_idx( int mx, int my );
void col_update();
void value_text_only( string ss );

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



void (*keydown_p_cb)( My_Input_Wheel *wdj, void *args );
void *keydown_cb_args;


void (*keyup_p_cb)( My_Input_Wheel *wdj, void *args );
void *keyup_cb_args;

};

//--------




//--------
class My_Input_Wheel_Packable_Box : public Fl_Box
{
private:
bool ctrl_key;
bool shift_key;
int mousex, mousey;
bool left_button;
bool right_button;
bool middle_button;
bool border;
void (*p_callback)( void* obj, void *args ); 
void *cb_obj;
void *cb_args;
double ctrl_val;
int mousewheel;
bool allow_mousewheel_adj;

public:
bool show_slider;
float slider_val;
Fl_Color col_bkg, col_bkg_focus, col_bkg_hover, col_border, col_border_hover, col_pointer;
bool b_use_limit_min;
bool b_use_limit_max;
double limit_max;
double limit_min;
int id, id2, id3;
int step_wheel_boundary_idx;

public:
My_Input_Wheel_Packable_Box(int x,int y,int w, int h,const char *label=0);
void set_callback( void (*p_cb)( void*, void* ), void *obj_in, void *args_in );
void set_val( double val );

private:
void draw();
int handle( int e );

};


//contains in a group a 'My_Input_Wheel' obj and a Fl_Box for label and optional slider, 
//as they are in a group internal, when this obj is added to a Fl_Scroll/Fl_Pack both are within parent's boundaries,
//use contained class: *miw to get/set double val
//see EXAMPLE code in header file

//My_Input_Wheel_Packable *miwp=new My_Input_Wheel_Packable(10, 15, 100, 35, o1.label.c_str() );
//miwp->labelsize( 9 );
//miwp->textsize( 9 );
//miwp->slider_enable( 1 );
//miwp->use_limit_min( 1 );
//miwp->use_limit_max( 1 );
//miwp->limit_min( o1.fmin );
//miwp->limit_max( o1.fmax );
//miwp->align(FL_ALIGN_BOTTOM);
//miwp->miw->std_wheel_step = ( o1.fmax - o1.fmin ) / 1000;
//miwp->miw->ctrl_wheel_step =  (  o1.fmax -  o1.fmin ) /100;
//miwp->miw->shift_wheel_step = (  o1.fmax - o1.fmin ) /10;
//miwp->miw->set_value_from_double( o1.fval );				
//miwp->miw->color( col_grn2 );											//show i/p ports with diff col
//miwp->set_id(i);														//additional value for callback, useful for matrix arrays of ctrls
//miwp->set_id2(j);
//miwp->set_callback( cb_plugin_ctrl, (void*)miwp, (void*)trk );

class My_Input_Wheel_Packable : public Fl_Group
{
private:
bool ctrl_key;
bool shift_key;
bool first_draw;
int txtsize;
void (*p_callback)( void* obj, void *args ); 
void *cb_obj;
void *cb_args;


//Fl_Group *grp;
public:
My_Input_Wheel_Packable_Box *bx_label;
My_Input_Wheel *miw;
bool b_use_limit_min;
bool b_use_limit_max;
double dlimit_min;
double dlimit_max;
int id, id2, id3;
bool inside;
bool focused;
bool b_last_change_was_by_dragging;										//v1.12


public:
void init();
const char* label();
void label( const char *sz );
void copy_label( string ss );
void textsize( int ii );
void use_limit_min( bool enable );
void use_limit_max( bool enable );
void limit_min( double limit );
void limit_max( double limit );
void set_callback( void (*p_cb)( void*, void* ), void* obj, void *args_in );
void set_id( int id_in );
void set_id2( int id_in );
void set_id3( int id_in );
void slider_enable( bool enable);
void set_val_internal_miw( double val );
void set_val_internal_flbox( double val );

private:
int handle(int);
void draw();

public:
My_Input_Wheel_Packable(int x,int y,int w, int h,const char *label=0);

};
//--------


#endif

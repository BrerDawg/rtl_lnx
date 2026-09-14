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

//rotary_knob_code.h

//---- v1.01


#ifndef rotary_knob_code_h
#define rotary_knob_code_h

#include "math.h"

#include <FL/Fl.H>
//#include <FL/Fl_Window.H>
//#include <FL/Fl_Box.H>
#include <FL/Enumerations.H>
#include <FL/fl_draw.H>
#include "GCProfile.h"

using namespace std;




class cl_rotary_knob : public Fl_Widget
{
private:

void (*p_mouse_button_changed_cb)( void* obj, void *args ); 
void *cb_mouse_button_changed_obj;
void *cb_mouse_button_changed_args;


void (*p_mouse_event_cb)( void* obj, void *args ); 
void *cb_mouse_event_obj;
void *cb_mouse_event_args;

bool b_do_std_callback_on_button_down;

mystr m1_dt;

public:
int id;
int id2;
bool left_button;
bool middle_button;
bool right_button;
int mousewheel;
int mousewheel_cnt;
int mousewheel_cnt_prev;

bool has_focus;

int knob_w, knob_h;

float dimple_detents_per_degree;										//how many degrees dimple to rotate after a single mousewheel event
float dimple_theta;														//in radian,  0 is east most, pi/2 is north, pi is west most, 3/4*twopi is southmost

Fl_Color col_bkgd, col_outline, col_border, col_multiplier_text, col_dimple, col_dimple2;

bool show_dimple;
int dimple_direction;													//1 is clockwise (increasing 'dimple_theta'), -1 is anticlockwise (increasing 'dimple_theta')
float dimple_x, dimple_y, dimple_w, dimple_h;
bool dimple_flicker;													//used to alter dimple intensity to show mousewheel is being spun, handy if dimple theta changes to the same value after a spin

bool show_bkgd;
bool show_border;

bool show_user_multiplier_text;
int user_multiplier_fonttype;
int user_multiplier_fontsize;
float user_multiplier;													//factors the spin value, change this in your own mouse callback

float local_pi;
float local_twopi;

float dimple_arc_ratio;													//0.0 means dimple is at center of knob, 1.0 means dimple sits or knob outline

#define cn_spinrate_factor_max 16
bool b_enable_spinrate_multiplier;
int spinrate_multiplier_cnt;											//the num of time/multipliers in arrays 'spinrate_time[]' and 'spinrate_multiplier[]'
float spinrate_time[cn_spinrate_factor_max];							//highest times must be first
float spinrate_multiplier[cn_spinrate_factor_max];						//factor to scale mousewheel's change

float rotate_change_amount;												//this will be scaled by 'b_enable_spinrate_multiplier'


private:
void draw();
int handle( int );

public:
cl_rotary_knob( int x, int y, int w, int h, const char *label );	
~cl_rotary_knob();
void set_mouse_button_changed_callback( void (*p_cb)( void*, void* ), void *obj_in, void *args_in );
void set_mouse_event_callback( void (*p_cb)( void*, void* ), void *obj_in, void *args_in );
void set_theta_radians( float theta );
void set_theta_degrees( float theta );
float get_theta_radians();
float get_theta_degrees();

//void set_mouse_event_callback( void (*p_cb)( void*, void* ), void *obj_in, void *args_in );

};





#endif

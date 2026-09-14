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

//rotary_knob_code.cpp

//---- v1.01    2025-mar-28        

#include "rotary_knob_code.h"




cl_rotary_knob::cl_rotary_knob( int xx, int yy, int ww, int hh, const char *label ) : Fl_Widget( xx, yy, ww, hh, label )
{


//p_mouse_button_changed_cb = 0;
//p_mouse_event_cb = 0;

id = 0;
id2 = 0;
left_button = middle_button = right_button = mousewheel = mousewheel_cnt = mousewheel_cnt_prev = 0;
dimple_flicker = 0;


has_focus = 0;

p_mouse_button_changed_cb = 0;
p_mouse_event_cb = 0;

b_do_std_callback_on_button_down = 0;


rotate_change_amount = 0;

m1_dt.time_start( m1_dt.ns_tim_start );


col_bkgd = parent()->color();

col_outline = fl_rgb_color( 0, 0, 0 );

col_border = fl_rgb_color( 50, 50, 50 );
col_multiplier_text = fl_rgb_color( 0, 0, 0 );
col_dimple = fl_rgb_color( 50, 50, 50 );
col_dimple2 = fl_rgb_color( 120, 120, 120 );

knob_w = w() - 5;
knob_h = h() - 5;

show_dimple = 1;
dimple_direction = 1;
dimple_x = w()/2;
dimple_y = h()/2;

dimple_w = 5;
dimple_h = 5;

dimple_detents_per_degree = 10;

dimple_theta = 0;
dimple_arc_ratio = 0.6;

show_bkgd = 1;
show_border = 1;

show_user_multiplier_text = 1;
user_multiplier_fonttype = 4;
user_multiplier_fontsize = 9;
user_multiplier = 1.0f;

local_pi = M_PI;
local_twopi = 2.0*M_PI;



//---
b_enable_spinrate_multiplier = 1;
spinrate_multiplier_cnt = 5;

spinrate_time[0] = 0.050;												//mousewheel dt change <= to this time: will use below matching 'spinrate_multiplier[]'
spinrate_multiplier[0] = 2.0;

spinrate_time[1] = 0.015;
spinrate_multiplier[1] = 4.0;

spinrate_time[2] = 0.003;												//medium spin rate
spinrate_multiplier[2] = 6.0;

spinrate_time[3] = 0.001;
spinrate_multiplier[3] = 10.0;

spinrate_time[4] = 0.0005;												//fast spin rate
spinrate_multiplier[4] = 20.0;
//---

}





cl_rotary_knob::~cl_rotary_knob()
{
	
}





void cl_rotary_knob::draw()
{
string s1;

float wx = x() + 1;
float wy = y() + 1;

float ww = w() - 2;
float wh = h() - 2;


if( show_bkgd )
	{
	fl_color( col_bkgd );
	fl_rectf( nearbyint(wx), nearbyint(wy), nearbyint(ww), nearbyint(wh) );
	} 

if( show_border )
	{
	fl_color( col_border );
	fl_rect(  nearbyint(wx), nearbyint(wy), nearbyint(ww), nearbyint(wh) );
	}


if( show_user_multiplier_text )
	{
	int iF = fl_font();
	int iS = fl_size();
	
	fl_font( user_multiplier_fonttype, user_multiplier_fontsize );
	fl_color( col_multiplier_text );
	
	strpf( s1, "%d", (int)user_multiplier );
	if( user_multiplier >= 1000 ) strpf( s1, "%dK", (int)user_multiplier/1000 );
	if( user_multiplier >= 1000000 ) strpf( s1, "%dM", (int)user_multiplier/1000000 );
	
	fl_draw( s1.c_str(), nearbyint(wx - 0), nearbyint(wy + wh/2 - 6), ww, 12, FL_ALIGN_INSIDE|FL_ALIGN_CENTER );

	fl_font( iF, iS );
	}

//outline
fl_color( col_outline );
fl_arc( nearbyint(wx + ww/2 - knob_w/2), nearbyint(wy + wh/2 - knob_h/2), nearbyint(knob_w), nearbyint(knob_h), 0, 360 );

//dimple_theta = 3.0f/4*local_twopi;


//dimple
if( show_dimple )
	{
	float dimp_rotx = dimple_arc_ratio * (ww/2) * cosf( dimple_theta ) + dimple_arc_ratio * (wh/2*0) * sinf( dimple_theta );
	float dimp_roty = dimple_arc_ratio *(ww/2) * sinf( dimple_theta ) + dimple_arc_ratio * (wh/2*0) * cosf( dimple_theta );



	if( dimple_flicker ) fl_color( col_dimple2 );
	else fl_color( col_dimple );

	float dimp_left = wx + ww/2 - dimple_w/2 + dimp_rotx;
	float dimp_top = wy + wh/2 - dimple_h/2 - dimp_roty;

	fl_pie( nearbyint(dimp_left), nearbyint(dimp_top), nearbyint(dimple_w), nearbyint(dimple_h), 0, 360 );
	}


//draw focus
if( has_focus ) 
	{
//	fl_color( parent()->color() );
	fl_color( 0, 0, 0 );

	fl_line_style( FL_DOT );
	//fl_arc( x(), y(), w(), h(), 0, 360 );
	fl_rect( x(), y(), w(), h() );

	fl_line_style( FL_SOLID );
	}


fl_color( 0, 0, 0 );
}








void cl_rotary_knob::set_theta_radians( float theta )
{
while( theta >= local_twopi )
	{
	theta -= local_twopi;
	}

while( theta <= -local_twopi )
	{
	theta += local_twopi;
	}
	
dimple_theta = theta;

}





void cl_rotary_knob::set_theta_degrees( float theta )
{
theta *= local_pi/180.0f;

while( theta >= local_twopi )
	{
	theta -= local_twopi;
	}

while( theta <= -local_twopi )
	{
	theta += local_twopi;
	}
	
dimple_theta = theta;
}









float cl_rotary_knob::get_theta_radians()
{
return dimple_theta;

}


float cl_rotary_knob::get_theta_degrees()
{
return dimple_theta*180.0/local_pi;
}




int cl_rotary_knob::handle( int e )
{

bool need_redraw = 0;
bool dont_pass_on = 0;

bool b_do_event_cb = 0;

//last_event = e;

//to trigger 'p_mouse_event_cb()'
if ( ( e == FL_ENTER ) || ( e == FL_LEAVE ) || ( e == FL_FOCUS ) || ( e == FL_UNFOCUS ) || ( e == FL_PUSH ) || ( e == FL_RELEASE ) || ( e == FL_KEYDOWN ) || ( e == FL_KEYUP ) || ( e == FL_MOUSEWHEEL ) )
    {
	b_do_event_cb = 1;
	}


if( b_do_event_cb )
	{
	if( p_mouse_event_cb ) p_mouse_event_cb( cb_mouse_event_obj, cb_mouse_event_args );
	}


if ( e == FL_ENTER )		//v1.06, was 'e & FL_ENTER'
    {
    dont_pass_on = 1;		//need this for tooltip
    }


if ( e & FL_LEAVE )			//see v1.03 mods, this code does nothing now
    {
//    dont_pass_on = 1;		//need this for tooltip
    }


//return Fl_Widget::handle(e);

if ( e == FL_FOCUS )
    {
    has_focus = 1;		    //need this for focus rectangle to be drawn
    dont_pass_on = 1;
    need_redraw = 1;
    }


if ( e == FL_UNFOCUS )
    {
    has_focus = 0;		    //need this for focus rectangle to be drawn
    dont_pass_on = 1;
    need_redraw = 1;
    }




if ( e == FL_PUSH )
	{
	if( Fl::event_button() == 1 ) left_button = 1;
	if( Fl::event_button() == 2 ) middle_button = 1;
	if( Fl::event_button() == 3 ) right_button = 1;
 
    take_focus();
    has_focus = 1;

	if( p_mouse_button_changed_cb ) p_mouse_button_changed_cb( cb_mouse_button_changed_obj, cb_mouse_button_changed_args );
	
	if(b_do_std_callback_on_button_down) do_callback();
    
    dont_pass_on = 1;
    need_redraw = 1;
	}


if ( e == FL_RELEASE )
	{
	if( Fl::event_button() == 1 ) left_button = 0;
	if( Fl::event_button() == 2 ) middle_button = 0;
	if( Fl::event_button() == 3 ) right_button = 0;
 
	if( p_mouse_button_changed_cb ) p_mouse_button_changed_cb( cb_mouse_button_changed_obj, cb_mouse_button_changed_args );
    dont_pass_on = 1;
    need_redraw = 1;
	}

if ( e == FL_KEYDOWN )
	{
    int key = Fl::event_key();
	
//	if( key == ' ' ) 
//		{
//		do_callback();
//		dont_pass_on = 1;
//		}
    }


if ( e == FL_MOUSEWHEEL )
	{

	float dt = m1_dt.time_passed( m1_dt.ns_tim_start );
	m1_dt.time_start( m1_dt.ns_tim_start );

	if( dt < 0.0f ) dt = 0.6f;
//printf("cl_rotary_knob::handle() - dt %f\n", dt );
	float factor = 1.0f;												//make a factor proportional to mousewheel spin rate

	if( b_enable_spinrate_multiplier )
		{
		for( int i = 0; i < spinrate_multiplier_cnt; i++ )				//larger times are checked first	
			{
			if( dt <= spinrate_time[i] ) factor = spinrate_multiplier[i];
			}
		}

	
	
	mousewheel_cnt_prev = mousewheel_cnt;
	mousewheel = Fl::event_dy();
	mousewheel_cnt += mousewheel;
	
	if( mousewheel_cnt_prev != mousewheel_cnt ) dimple_flicker = !dimple_flicker;
	

	float rotate_dir = 1;
	if( mousewheel < 0 ) rotate_dir = -1;
	
	rotate_change_amount = rotate_dir * factor * user_multiplier;		//apply factors
	
	float dimple_rot = rotate_dir * local_twopi * (dimple_detents_per_degree/360.0f * factor);

//	float dimple_theta_old = dimple_theta;

//	float dimple_rot = rotate_dir * local_twopi*0.05;
	dimple_theta += dimple_rot * (-dimple_direction);

//	if( dimple_theta == dimple_theta_old ) dimple_theta += dimple_theta_old + local_twopi*0.05;
	
    do_callback();

	need_redraw = 1;
    dont_pass_on = 0;
	}


if( need_redraw )
	{
	redraw();
	}

	
if( dont_pass_on ) return 1;

return Fl_Widget::handle(e);
}








//e.g: use public var 'left_button' to determine state
void cl_rotary_knob::set_mouse_button_changed_callback( void (*p_cb)( void*, void* ), void *obj_in, void *args_in )
{
p_mouse_button_changed_cb = p_cb;
cb_mouse_button_changed_obj = obj_in;
cb_mouse_button_changed_args = args_in;
}





//could be used to handle a mouse enter/leave event
void cl_rotary_knob::set_mouse_event_callback( void (*p_cb)( void*, void* ), void *obj_in, void *args_in )
{
p_mouse_event_cb = p_cb;
cb_mouse_event_obj = obj_in;
cb_mouse_event_args = args_in;
}


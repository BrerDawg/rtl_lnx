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


//my_input_wheel.cpp

//v1.01   2019-may-26		//
//v1.02   2019-jun-23		//for 'My_Input_Wheel', added right butt dragging to adj val, see 'allow_right_but_drag' etc

//v1.03   10-mar-2020		//fixed limit max bug when mouse wheeling, 'My_Input_Wheel_Packable::use_limit_max()'
							//added 'My_Input_Wheel::mousewheel_external()', called by 'My_Input_Wheel_Packable_Box::handle()', so 
							//the crude 'slider fl_box' has mousewheel adj of vals, also option to disable with bool 'allow_mousewheel_adj'
//v1.04   25-mar-2020		//added 'id3'
//v1.05	  06-jul-2020		//merged missing v1.02 features for dragging vals (ex tr808), see 'allow_right_but_drag' etc
//v1.06	  23-jan-2022		//improved FL_ENTER handling, so mousewheel only works when 'inside'
							//added to 'My_Input_Wheel_Packable_Box' colour vars: 'col_bkg_hover' and  'col_border_hover'

//v1.07	  29-nov-2022		//added 'ctrl_shift_wheel_step', added: 'const char* My_Input_Wheel::value()', added: 'btake_focus_on_inside' and 'col_bkg_hover_focus'
							//added 'My_Input_Wheel_Packable::label()',  'My_Input_Wheel_Packable::copy_label()'

//v1.08	  16-dec-2022		//added further wheel inc/dec features, 4 'mousex' bounded sections, see e.g:  'b_step_wheel_side_left', 'step_wheel_side_left' .... 'b_step_wheel_side_right', 'step_wheel_side_right'
//v1.09	  14-feb-2023		//added 'b_show_modified' colour feature, also added 'col_update()' for internal use
//v1.10	  23-dec-2023		//added 'set_keydown_cb()', 'set_keyup_cb()', for the callbacks use 'keycode_last' to find what key was changed
//v1.11	  25-mar-2025		//added 'My_Input_Wheel::value_text_only()'
//v1.12	  13-sep-2026		//added to 'My_Input_Wheel_Packable_Box'  flag 'b_last_change_was_by_dragging'


#include "my_input_wheel.h"








//----------------------------------------------------------



//!!note: if you have 's_printf_format' = "%d",  make sure you set 'force_integer'

//v1.01			03-dec-2014				//
//v1.02			19-dec-2016				//fixed windows handling, search for: s_printf_format.find( "lf") != string::npos
//v1.03			26-mar-2018				//added 'b_invert_wheel' 
//v1.04			19-apr-2018				//added callbacks for left/right clicks/double clicks/wheel
//v1.05			15-sep-2018				//added 'id' and 'id2' vars to help in identifying multiple instances of these objs
//v1.06			30-sep-2018				//added FL_LEAVE to call parent:  Fl::focus( this->parent() );
//v1.07			21-oct-2018				//added 'set_callback()', which if set will override the default 'do_callback()', this ties in with 'My_Input_Wheel_Packable'
										//that uses this obj as a child
//v1.08			04-jan-2019				//added: set_left_click_release_cb(), set_right_click_release_cb()
//v1.09			06-jul-2020				//merged missing v1.02 features for dragging vals (ex tr808), see 'allow_right_but_drag' etc
	
My_Input_Wheel::My_Input_Wheel( int x, int y, int w, int h, const char *label ) : Fl_Input( x, y, w, h, label )
{
s_printf_format = "%g";                     //set default printf format for display
force_integer = 0;                          //set this only if you use '%d' in printf_format and s_printf_format

left_click_p_cb = 0;
left_double_click_p_cb = 0;

right_click_p_cb = 0;
right_double_click_p_cb = 0;

left_click_release_p_cb = 0;
right_click_release_p_cb = 0;


mousewheel_p_cb = 0;

keydown_p_cb = 0;
keyup_p_cb = 0;

keycode_last = 0;


paired_obj = 0;
p_callback = 0;

id = 0;
id2 = 0;
id3 = 0;

ctrl_key = 0;
shift_key = 0;
b_allow_wheel_inc = 1;
b_invert_wheel = 0;							//if set, turning mousewheel towards user takes value up
std_wheel_step = 1;
ctrl_wheel_step = 1;
shift_wheel_step = 1;
ctrl_shift_wheel_step = 1;


b_step_wheel_side_left = 0;
b_step_wheel_side_left_center = 0;
b_step_wheel_side_right_center = 0;
b_step_wheel_side_right = 0;

step_wheel_side_left = 0.0f;
step_wheel_side_left_center = 0.0f;
step_wheel_side_right_center = 0.0f;
step_wheel_side_right = 0.0f;

step_wheel_boundary_idx = -1;



b_use_limit_max = 0;
b_use_limit_min = 0;
limit_max = 1e99;
limit_min = -1e99;

value( "0" );

left_button = right_button = 0;
allow_right_but_drag = 0;
right_but_drag = 0;
right_drag_x_val_change_factor = 0.00;
right_drag_y_val_change_factor = 0.001;
inside = 0;
focused = 0;

col_bkg = fl_rgb_color( 220, 255, 220 );
col_bkg_hover = fl_rgb_color( 200, 255, 200 );
col_bkg_focus = fl_rgb_color( 255, 230, 230 );
col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
col_bkg_modified = fl_rgb_color( 255, 180, 180 );
col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );

b_take_focus_on_inside = 0;



step_wheel_boundary_build();

b_show_modified = 1;
b_modified = 0;

col_update();

}






const char* My_Input_Wheel::value()
{
return Fl_Input::value();
}








void My_Input_Wheel::step_wheel_boundary_build()
{
step_wheel_boundary[0].left = x();
step_wheel_boundary[0].right = x() + w()/4 - 1;

step_wheel_boundary[1].left = x() + w()/4;
step_wheel_boundary[1].right = x() + w()/2 - 1;

step_wheel_boundary[2].left = x() + w()/2;
step_wheel_boundary[2].right = x() + 3*w()/4 - 1;

step_wheel_boundary[3].left = x() + 3*w()/4;
step_wheel_boundary[3].right = x() + w();
}




//returns idx if coord is in a boundary, else -1
int My_Input_Wheel::step_wheel_boundary_find_idx( int mx, int my )
{
if( ( mx >= step_wheel_boundary[ 0 ].left ) && ( mx <= step_wheel_boundary[ 0 ].right ) ) return 0;
if( ( mx >= step_wheel_boundary[ 1 ].left ) && ( mx <= step_wheel_boundary[ 1 ].right ) ) return 1;
if( ( mx >= step_wheel_boundary[ 2 ].left ) && ( mx <= step_wheel_boundary[ 2 ].right ) ) return 2;
if( ( mx >= step_wheel_boundary[ 3 ].left ) && ( mx <= step_wheel_boundary[ 3 ].right ) ) return 3;

return -1;
}





void My_Input_Wheel::value( string ss )
{
Fl_Input::value( ss.c_str() );

enforce_limits();

do_callback();
}




//this only enter text and does not convert to a double, hand to enter numbers via a keypad which could use '.' decimal point
void My_Input_Wheel::value_text_only( string ss )
{
Fl_Input::value( ss.c_str() );

//enforce_limits();

//do_callback();
}



void My_Input_Wheel::value( string *ss )
{
*ss = Fl_Input::value();
}







void My_Input_Wheel::set_left_click_cb( void (*p_cb)( My_Input_Wheel*, void* ), void *args )
{
left_click_p_cb = p_cb;
left_click_cb_args = args;
}


void My_Input_Wheel::set_left_double_click_cb( void (*p_cb)( My_Input_Wheel*, void* ), void *args )
{
left_double_click_p_cb = p_cb;
left_double_click_cb_args = args;
}



void My_Input_Wheel::set_right_click_cb( void (*p_cb)( My_Input_Wheel*, void* ), void *args )
{
right_click_p_cb = p_cb;
right_click_cb_args = args;
}


void My_Input_Wheel::set_right_double_click_cb( void (*p_cb)( My_Input_Wheel*, void* ), void *args )
{
right_double_click_p_cb = p_cb;
right_double_click_cb_args = args;
}




void My_Input_Wheel::set_mousewheel_cb( void (*p_cb)( My_Input_Wheel*, void*, int ), void *args )
{
mousewheel_p_cb = p_cb;
mousewheel_cb_args = args;
}



void My_Input_Wheel::set_left_click_release_cb( void (*p_cb)( My_Input_Wheel*, void* ), void *args )
{
left_click_release_p_cb = p_cb;
left_click_release_cb_args = args;
}



void My_Input_Wheel::set_right_click_release_cb( void (*p_cb)( My_Input_Wheel*, void* ), void *args )
{
right_click_release_p_cb = p_cb;
right_click_release_cb_args = args;
}



//get key code from 'keycode_last'
void My_Input_Wheel::set_keydown_cb( void (*p_cb)( My_Input_Wheel*, void* ), void *args )
{
keydown_p_cb = p_cb;
keydown_cb_args = args;
}


//get key code from 'keycode_last'
void My_Input_Wheel::set_keyup_cb( void (*p_cb)( My_Input_Wheel*, void* ), void *args )
{
keyup_p_cb = p_cb;
keyup_cb_args = args;
}




//check if supplied var is past limits, enforce limits to it if so
void My_Input_Wheel::enforce_limits_var( double &dd )
{
//printf("My_Input_Wheel::enforce_limits_var() - this %09x\n", this );
if( b_use_limit_max ) if( dd > limit_max ) dd = limit_max;
if( b_use_limit_min ) if( dd < limit_min ) dd = limit_min;
}






//check if displayed value is past limits, enforce limits if so and redisplay
void My_Input_Wheel::enforce_limits()
{
double dd;

string s1 = Fl_Input::value();

sscanf( s1.c_str(), "%lf", &dd );

if( b_use_limit_max ) if( dd > limit_max ) dd = limit_max;
if( b_use_limit_min ) if( dd < limit_min ) dd = limit_min;

if( !force_integer )                							//use appropriate var type
	{
	//linux code
	#ifndef compile_for_windows
		strpf( s1, s_printf_format.c_str(), dd );
	#endif

	//windows code
	#ifdef compile_for_windows
	if( s_printf_format.find( "lf") != string::npos )			//is there a %lf format?
		{	
		strpf( s1, s_printf_format.c_str(), (long double)dd );
		}
	else{
		strpf( s1, s_printf_format.c_str(), (double) dd );
		}
	#endif
	}
else{
	strpf( s1, s_printf_format.c_str(), (int)dd );
	}
	
Fl_Input::value( s1.c_str() );
}






//check if displayed value is past limits, enforce limits if so and redisplay
//you must specify an alternate printf format for display as default 's_printf_format' is not used
void My_Input_Wheel::enforce_limits( const char* printf_format )
{
double dd;

string s1 = Fl_Input::value();

sscanf( s1.c_str(), "%lf", &dd );

if( b_use_limit_max ) if( dd > limit_max ) dd = limit_max;
if( b_use_limit_min ) if( dd < limit_min ) dd = limit_min;

if( !force_integer )                							//use appropriate var type
	{
	//linux code
	#ifndef compile_for_windows
		strpf( s1, s_printf_format.c_str(), dd );
	#endif

	//windows code
	#ifdef compile_for_windows
	if( s_printf_format.find( "lf") != string::npos )			//is there a %lf format?
		{	
		strpf( s1, s_printf_format.c_str(), (long double)dd );
		}
	else{
		strpf( s1, s_printf_format.c_str(), (double) dd );
		}
	#endif
	}
else{
	strpf( s1, s_printf_format.c_str(), (int)dd );
	}

Fl_Input::value( s1.c_str() );
}






void My_Input_Wheel::set_value_from_double( double dd )
{
string s1;

enforce_limits_var( dd );

if( !force_integer )                							//use appropriate var type
	{
	//linux code
	#ifndef compile_for_windows
		strpf( s1, s_printf_format.c_str(), dd );
	#endif

	//windows code
	#ifdef compile_for_windows
	if( s_printf_format.find( "lf") != string::npos )			//is there a %lf format?
		{	
		strpf( s1, s_printf_format.c_str(), (long double)dd );
		}
	else{
		strpf( s1, s_printf_format.c_str(), (double) dd );
		}
	#endif
	}
else{
	strpf( s1, s_printf_format.c_str(), (int)dd );
	}

if( paired_obj ) ((My_Input_Wheel_Packable*)parent())->set_val_internal_miw( dd );		//let parent know, so it can adj paired wdj

Fl_Input::value( s1.c_str() );
}









//user specifies display format
void My_Input_Wheel::set_value_from_double( double dd,  const char* printf_format )
{
string s1;

enforce_limits_var( dd );

if( !force_integer )                							//use appropriate var type
	{
	//linux code
	#ifndef compile_for_windows
		strpf( s1, s_printf_format.c_str(), dd );
	#endif

	//windows code
	#ifdef compile_for_windows
	if( s_printf_format.find( "lf") != string::npos )			//is there a %lf format?
		{	
		strpf( s1, s_printf_format.c_str(), (long double)dd );
		}
	else{
		strpf( s1, s_printf_format.c_str(), (double) dd );
		}
	#endif
	}
else{
	strpf( s1, s_printf_format.c_str(), (int)dd );
	}

if( paired_obj ) ((My_Input_Wheel_Packable*)parent())->set_val_internal_miw( dd );		//let parent know, so it can adj paired wdj

Fl_Input::value( s1.c_str() );
}









void My_Input_Wheel::set_value_from_str( string ss )
{
value( ss );
enforce_limits( s_printf_format.c_str() );
}





//user specifies display format
void My_Input_Wheel::set_value_from_str( string ss,  const char* printf_format )
{
value( ss );
enforce_limits( printf_format );
}









double My_Input_Wheel::get_value_as_double()
{
double dd;
string s1 = Fl_Input::value();

sscanf( s1.c_str(), "%lf", &dd );

return dd;
}







void My_Input_Wheel::set_callback( void (*p_cb)( void*, void* ), void *obj_in, void *args_in )
{
p_callback = p_cb;
cb_obj = obj_in;
cb_args = args_in;
}







//used by 'My_Input_Wheel_Packable'
//'mx, my' are global mouse coords
void My_Input_Wheel::mousewheel_external( int step_wheel_boundary_idx_in, int dir, bool ctrl_key0, bool shift_key0 )
{
//mousewheel = dir;

//printf("mousewheel_external= %d\n", dir );


if( b_invert_wheel ) dir = -dir;


bool b_key_down = 0;

if( ctrl_key0 || shift_key0 ) b_key_down = 1;

if( b_allow_wheel_inc )
	{
	int b_done = 0;
	if( ( step_wheel_boundary_idx_in >= 0 ) && ( !b_key_down ) )
		{
		if( ( step_wheel_boundary_idx_in == 0 ) && b_step_wheel_side_left )	
			{
			double dd = get_value_as_double();
			
			dd += step_wheel_side_left * dir;			
			set_value_from_double( dd, s_printf_format.c_str() );
			
			b_done = 1;
			}

		if( ( step_wheel_boundary_idx_in == 1 ) && b_step_wheel_side_left_center )	
			{
			double dd = get_value_as_double();
			
			dd += step_wheel_side_left_center * dir;			
			set_value_from_double( dd, s_printf_format.c_str() );
			
			b_done = 1;
			}

		if( ( step_wheel_boundary_idx_in == 2 ) && b_step_wheel_side_right_center )	
			{
			double dd = get_value_as_double();
			
			dd += step_wheel_side_right_center * dir;			
			set_value_from_double( dd, s_printf_format.c_str() );
			
			b_done = 1;
			}

		if( ( step_wheel_boundary_idx_in == 3 ) && b_step_wheel_side_right )	
			{
			double dd = get_value_as_double();
			
			dd += step_wheel_side_right * dir;			
			set_value_from_double( dd, s_printf_format.c_str() );
			
			b_done = 1;
			}
		}



	if( !b_done )			//do regular std/ctrl/shift wheel adj
		{
		double dd;
	//		s1 = Fl_Input::value();
		
		dd = get_value_as_double();
	//		sscanf( s1.c_str(), "%lf", &dd );

	//printf(" dd= %g\n", dd );
		double step = std_wheel_step;
		
		if( (ctrl_key0) && (!shift_key0) ) step = ctrl_wheel_step;
		if( (!ctrl_key0) && (shift_key0) ) step = shift_wheel_step;
		if( (ctrl_key0) && (shift_key0) ) step = ctrl_shift_wheel_step;

//		if( b_invert_wheel ) mousewheel = -mousewheel;

//		if( mousewheel < 0 ) dd += step * dir;
//		if( mousewheel > 0 ) dd -= step * dir;

		dd += step * dir;
 
		set_value_from_double( dd, s_printf_format.c_str() );
		b_done = 1;
		}

	if( b_done )
		{
		if( !p_callback )
			{
			do_callback();															//use fltk callback?
			}
		else{
			p_callback( cb_obj, cb_args);
			}

		if( paired_obj ) 
			{
			double dv = get_value_as_double();
			((My_Input_Wheel_Packable*)parent())->set_val_internal_miw( dv );		//let parent know, so it can adj paired wdj
			}
		}
	}

if( mousewheel_p_cb ) mousewheel_p_cb( this, mousewheel_cb_args, Fl::event_dy() );
}






//set required col
void My_Input_Wheel::col_update()
{
if( (!inside) && (!focused) && (!b_modified) ) color( col_bkg );
	
if( (!inside) && focused && (!b_modified) ) color( col_bkg_focus );
if( (inside) && (!focused) && (!b_modified) ) color( col_bkg_hover );
if( (inside) && focused && (!b_modified) ) color( col_bkg_hover_focus );

if( (!inside) && focused && b_modified ) color( col_bkg_modified );
if( (inside) && (!focused) && b_modified ) color( col_bkg_hover_modified );
if( (inside) && focused && b_modified ) color( col_bkg_hover_modified );
}




int My_Input_Wheel::handle( int e )
{
string s1;
int len;
char *szTmp;
bool need_redraw = 0;
bool dont_pass_on = 0;
int mousewheel;


if ( e == FL_ENTER )
	{
	inside = 1;
	if( b_take_focus_on_inside )
		{
		take_focus();
		}
	
//	if( (!inside) && (!focused) && (!b_modified) ) color( col_bkg );
	
//	if( (!inside) && focused && (!b_modified) ) color( col_bkg_focus );
//	if( (inside) && (!focused) && (!b_modified) ) color( col_bkg_hover );
//	if( (inside) && focused && (!b_modified) ) color( col_bkg_hover_focus );

//	if( (!inside) && focused && b_modified ) color( col_bkg_modified );
//	if( (inside) && (!focused) && b_modified ) color( col_bkg_hover_modified );
//	if( (inside) && focused && b_modified ) color( col_bkg_hover_modified );

	col_update();

	if( paired_obj ) 
		{
		((My_Input_Wheel_Packable*)parent())->inside = 1;				//let parent know
		}

//	printf( "My_Input_Wheel::handle() - FL_ENTER id %d %d %d\n", id, id2, id3 );
//	if( Fl::event_button() == 1 )											//left click
//		{
//		}
//    take_focus();
//    dont_pass_on = 1;
	need_redraw = 1;
//    dont_pass_on = 1;													//don't do this, so I beam cursor changes back to an arrow
    }



if ( e == FL_LEAVE )
	{
	inside = 0;
	step_wheel_boundary_idx = -1;
	

	col_update();

//	if( (!inside) && (!focused) && (!b_modified) ) color( col_bkg );
	
//	if( (!inside) && focused && (!b_modified) ) color( col_bkg_focus );
//	if( (inside) && (!focused) && (!b_modified) ) color( col_bkg_hover );
//	if( (inside) && focused && (!b_modified) ) color( col_bkg_hover_focus );

//	if( (!inside) && focused && b_modified ) color( col_bkg_modified );
//	if( (inside) && (!focused) && b_modified ) color( col_bkg_hover_modified );
//	if( (inside) && focused && b_modified ) color( col_bkg_hover_modified );


	if( paired_obj ) 
		{
		((My_Input_Wheel_Packable*)parent())->inside = 0;				//let parent know
		}
//	printf( "My_Input_Wheel::handle() - FL_LEAVE id %d %d %d\n", id, id2, id3 );
	
//	take_focus();
//if( pref_main_wnd_auto_take_focus ) Fl::focus( this );
//	Fl::focus( this->parent() );

	need_redraw = 1;
//	dont_pass_on = 1;													//don't do this, so I beam cursor changes back to an arrow
	}


if ( e == FL_FOCUS )
	{
	focused = 1;
	col_update();

//	printf( "My_Input_Wheel::handle() - FL_FOCUS id %d %d %d\n", id, id2, id3 );
	need_redraw = 1;
	dont_pass_on = 1;													//don't do this, so I beam cursor changes back to an arrow
	}

if ( e == FL_UNFOCUS )
	{
	focused = 0;
	col_update();
//	printf( "My_Input_Wheel::handle() - FL_UNFOCUS id %d %d %d\n", id, id2, id3 );
	need_redraw = 1;
	dont_pass_on = 1;													//don't do this, so I beam cursor changes back to an arrow
	}

if ( e & FL_MOVE )	
	{
	mousex = Fl::event_x();
	mousey = Fl::event_y();
	
	step_wheel_boundary_idx = step_wheel_boundary_find_idx( mousex, mousey );
//	printf("My_Input_Wheel::handle() - step_wheel_boundary_idx %d\n", step_wheel_boundary_idx );

	
	bool changed = 0;
	if( ( allow_right_but_drag ) && ( right_button ) )
		{
		int dx = mousex - grab_right_drag_x;
		int dy = mousey - grab_right_drag_y;
		
		int drag_dead_zone = 5;
		if( ( fabs( dx ) > drag_dead_zone  ) )
			{
			double dir = 1;
			if( dx < 0.0 ) dir = -1;

			right_but_drag = 1;
//			printf("dragging dx %f  grab_right_drag_val: %f\n", dir * ( fabs(dx) - drag_dead_zone ), grab_right_drag_val);
			
			float ff = grab_right_drag_val + dir * right_drag_x_val_change_factor * ( fabs(dx) - drag_dead_zone );
			set_value_from_double( ff );
			changed = 1;
			}

		if( ( fabs( dy ) > drag_dead_zone ) )
			{
			double dir = 1;
			if( dy > 0.0 ) dir = -1;

			right_but_drag = 1;
//			printf("dragging dy %f  grab_right_drag_val: %f\n", dir * ( fabs(dy) - drag_dead_zone ), grab_right_drag_val);
			
			float ff = grab_right_drag_val + dir * right_drag_y_val_change_factor * ( fabs(dy) - drag_dead_zone );
			set_value_from_double( ff );
			changed = 1;
			}


		if( changed )
			{
			if( !p_callback ) do_callback();														//use fltk callback?
			else p_callback( cb_obj, cb_args);
			}
		}
	need_redraw = 1;
    dont_pass_on = 0;
	}







if ( e == FL_PUSH )	
	{
	int ii = Fl::event_clicks();

	if( Fl::event_button() == 1 )											//left click
		{
		left_button = 1;
		if( ii == 2 )
			{
			if( left_double_click_p_cb != 0 ) left_double_click_p_cb( this, left_double_click_cb_args );
			}
		else{
			if( left_click_p_cb != 0 ) left_click_p_cb( this, left_click_cb_args );
			}
		}

	if( Fl::event_button() == 3 )											//right click
		{
		right_button = 1;
		grab_right_drag_x = mousex;
		grab_right_drag_y = mousey;
		grab_right_drag_val = get_value_as_double();

		if( ii == 2 )
			{
			if( right_double_click_p_cb != 0 ) right_double_click_p_cb( this, right_double_click_cb_args );
			}
		else{
			if( right_click_p_cb != 0 ) right_click_p_cb( this, right_click_cb_args );
			}
		}
	need_redraw = 1;
    dont_pass_on = 0;
	}



if ( e == FL_RELEASE )	
	{
	if( Fl::event_button() == 1 )
		{
		left_button = 0;
		if( left_click_release_p_cb != 0 ) left_click_release_p_cb( this, left_click_release_cb_args );
		}

	if( Fl::event_button() == 3 )
		{
		right_button = 0;
		right_but_drag = 0;
		if( right_click_release_p_cb != 0 ) right_click_release_p_cb( this, right_click_release_cb_args );
		}
	need_redraw = 1;
    dont_pass_on = 0;
	}



if ( e == FL_MOUSEWHEEL )
	{
	if( inside )
		{
		mousewheel = Fl::event_dy();
		
		int dir = 1;
		if ( mousewheel < 0 ) dir = -1;

		need_redraw = 1;
		dont_pass_on = 1;

		mousewheel_external( step_wheel_boundary_idx, dir, ctrl_key, shift_key );

/*
		if( b_allow_wheel_inc )
			{
				
	
			double dd;
	//		s1 = Fl_Input::value();
			
			dd = get_value_as_double();
	//		sscanf( s1.c_str(), "%lf", &dd );

	//printf(" dd= %g\n", dd );
			double step = std_wheel_step;
			
			if( (ctrl_key) && (!shift_key) ) step = ctrl_wheel_step;
			if( (!ctrl_key) && (shift_key) ) step = shift_wheel_step;
			if( (ctrl_key) && (shift_key) ) step = ctrl_shift_wheel_step;
		 
			if( b_invert_wheel ) mousewheel = -mousewheel;

			if( mousewheel < 0 ) dd += step;
			if( mousewheel > 0 ) dd -= step;

			set_value_from_double( dd, s_printf_format.c_str() );
			
			if( !p_callback )
				{
				do_callback();														//use fltk callback?
				}
			else{
				p_callback( cb_obj, cb_args);
				}

			if( paired_obj ) 
				{
				double dv = get_value_as_double();
				((My_Input_Wheel_Packable*)parent())->set_val_internal_miw( dv );		//let parent know, so it can adj paired wdj
				}
			}

		if( mousewheel_p_cb ) mousewheel_p_cb( this, mousewheel_cb_args, Fl::event_dy() );
*/
		}
	}





if ( ( e == FL_KEYDOWN ) || ( e == FL_SHORTCUT ) )
	{
	int key = Fl::event_key();
	keycode_last = key;
	
	if( ( key == FL_Control_L ) || ( key == FL_Control_R ) ) ctrl_key = 1;
	if( ( key == FL_Shift_L ) || ( key == FL_Shift_R ) ) shift_key = 1;

	if( b_show_modified && !(key == FL_Control_L) && !(key == FL_Control_R) && !(key == FL_Shift_L) && !(key == FL_Shift_R) ) b_modified = 1;
	

	if( keydown_p_cb != 0 )  keydown_p_cb( this, keydown_cb_args );

	if( key == FL_Enter )												//is it CR ?
		{
		b_modified = 0;
		
        enforce_limits( s_printf_format.c_str() );

		if( !p_callback )
			{
			do_callback();												//use fltk callback?
			}
		else{
			p_callback( cb_obj, cb_args);
			}
					
		if( paired_obj ) 
			{
			double dv = get_value_as_double();
			((My_Input_Wheel_Packable*)parent())->set_val_internal_miw( dv );		//let parent know, so it can adj paired wdj
			}	
		dont_pass_on = 1;
		}

	col_update();

	need_redraw = 1;
	}



if ( e == FL_KEYUP )												//key release?
	{
	int key = Fl::event_key();
	keycode_last = key;
	
	if( keyup_p_cb != 0 )  keyup_p_cb( this, keyup_cb_args );

	if( ( key == FL_Control_L ) || ( key == FL_Control_R ) ) ctrl_key = 0;
	if( ( key == FL_Shift_L ) || (  key == FL_Shift_R ) ) shift_key = 0;

	need_redraw = 1;
    dont_pass_on = 1;
	}





if ( need_redraw ) redraw();

if( dont_pass_on ) return 1;

return Fl_Input::handle(e);
}


//----------------------------------------------------------








//----------------------------------------------------------

My_Input_Wheel_Packable_Box::My_Input_Wheel_Packable_Box( int x, int y, int w, int h, const char *label ) : Fl_Box( x, y, w, h, "" )
{
ctrl_key = 0;
shift_key = 0;
show_slider = 1;
ctrl_val = 0;
slider_val = 0;
p_callback = 0;
left_button = 0;
border = 1;
col_bkg = FL_GRAY;//fl_rgb_color( 180, 180, 180 );
col_bkg_focus = fl_rgb_color( 120, 100, 100 );
col_pointer = fl_rgb_color( 255, 100, 100 );
col_border = fl_rgb_color( 0, 0, 0 );

col_bkg_hover = fl_rgb_color( 200, 200, 200 );
col_border_hover = fl_rgb_color( 80, 80, 80 );

allow_mousewheel_adj = 1;

step_wheel_boundary_idx = -1;

}



void My_Input_Wheel_Packable_Box::set_callback( void (*p_cb)( void*, void* ), void *obj_in, void *args_in )
{
p_callback = p_cb;
cb_obj = obj_in;
cb_args = args_in;
}



void My_Input_Wheel_Packable_Box::set_val( double val )
{
if( val < limit_min ) val = limit_min;
if( val > limit_max ) val = limit_max;
slider_val = val;

ctrl_val = (val - limit_min)  / (limit_max - limit_min);
redraw();
}




void My_Input_Wheel_Packable_Box::draw()
{
My_Input_Wheel_Packable *oparent = ((My_Input_Wheel_Packable*)parent());


if( oparent->inside ) fl_color( col_bkg_hover );
else fl_color( col_bkg );
fl_rectf( x(), y(), w(), h() );

if( border )
	{
	if( oparent->inside ) fl_color( col_border_hover );
	else fl_color( col_border );
	fl_rect( x(), y(), w(), h() );
	}

if( show_slider )
	{
	fl_color( col_pointer );

	int slidr_x =  ctrl_val * w();
	int slidr_y = 1;

	if( slidr_x <=0 ) slidr_x = 1;
	if( slidr_x >= w() ) slidr_x = w()-3;

	fl_rectf(  x() + slidr_x , y() + slidr_y, 2 , h() - 2 );		//draw slider pointer
	}


//if( oparent->inside ) fl_rect( x() + 3, y() + 3, w() - 6, h() - 6 );

Fl_Box::draw();														//draw label

}




int My_Input_Wheel_Packable_Box::handle( int e )
{
bool need_redraw = 0;
bool dont_pass_on = 0;

My_Input_Wheel_Packable *oparent = ((My_Input_Wheel_Packable*)parent());

if ( e == FL_ENTER ) 
	{
//	printf( "My_Input_Wheel_Packable_Box::handle() - FL_ENTER id %d %d %d\n", oparent->id, oparent->id2, oparent->id3 );
	oparent->inside = 1;
	need_redraw = 1;
	dont_pass_on = 1;						//need this for tooltip
//	Fl::focus( this );
//	take_focus();
	}

if ( e == FL_LEAVE ) 
	{
//	printf( "My_Input_Wheel_Packable_Box::handle() - FL_LEAVE id %d %d %d\n", oparent->id, oparent->id2, oparent->id3 );
	oparent->inside = 0;
	oparent->b_last_change_was_by_dragging = 1;						//v1.12
	dont_pass_on = 1;		//need this for tooltip
	need_redraw = 1;
	}

if( e == FL_FOCUS ) 
	{
	oparent->focused = 1;
	
//	printf( "My_Input_Wheel_Packable_Box::handle() - FL_FOCUS id %d %d %d\n", oparent->id, oparent->id2, oparent->id3 );
	need_redraw = 1;
	dont_pass_on = 1;						//need this for tooltip
	}

if( e == FL_UNFOCUS ) 
	{
	oparent->focused = 0;
//	printf( "My_Input_Wheel_Packable_Box::handle() - FL_UNFOCUS id %d %d %d\n", oparent->id, oparent->id2, oparent->id3 );
	need_redraw = 1;
	dont_pass_on = 1;						//need this for tooltip
	}

//printf("left_button: %d %d %d, '%s'\n",left_button,e, (int)FL_MOVE, label() );
if ( e & FL_MOVE )
	{
	mousex = Fl::event_x();
	mousey = Fl::event_y();

	if( left_button ) oparent->b_last_change_was_by_dragging = 0;		//v1.12

	step_wheel_boundary_idx = oparent->miw->step_wheel_boundary_find_idx( mousex, mousey );
//	printf("My_Input_Wheel_Packable::handle() - step_wheel_boundary_idx %d\n", step_wheel_boundary_idx );
	
	if( ( show_slider ) & ( left_button ) )
		{
		double range = limit_max - limit_min;		
		ctrl_val = (mousex - x()) / (double)w();			//min is 0.0, max is 1.0
		if( ctrl_val < 0.0 ) ctrl_val = 0;
		if( ctrl_val > 1.0 ) ctrl_val = 1.0;
		slider_val = limit_min + range * ctrl_val;	
//printf("ctrl_val %f slider_val %f\n", ctrl_val, slider_val ); 
		if( p_callback ) p_callback( cb_obj, cb_args );
		((My_Input_Wheel_Packable*)parent())->set_val_internal_flbox( slider_val );		//let parent know, so it can adj paired wdj

		oparent->b_last_change_was_by_dragging = 1;						//v1.12
		}

	need_redraw = 1;
	dont_pass_on = 1;
	}


if ( e == FL_PUSH )
	{
	if( Fl::event_button() == 1 )
		{
		left_button = 1;
		if( ( show_slider ) & ( left_button ) )
			{
			double range = limit_max - limit_min;		
			ctrl_val = (mousex - x()) / (double)w();			//min is 0.0, max is 1.0
			if( ctrl_val < 0.0 ) ctrl_val = 0;
			if( ctrl_val > 1.0 ) ctrl_val = 1.0;
			slider_val = limit_min + ctrl_val * range;	
			if( p_callback ) p_callback( cb_obj, cb_args );
			((My_Input_Wheel_Packable*)parent())->set_val_internal_flbox( slider_val );	//let parent know, so it can adj paired wdj
			}
		}
	
	need_redraw = 1;
    dont_pass_on = 1;
	}



if ( e == FL_RELEASE )
	{
	if( Fl::event_button() == 1 )
		{
		left_button = 0;
		}
	need_redraw = 1;
    dont_pass_on = 1;
	}





if ( ( e == FL_KEYDOWN ) || ( e == FL_SHORTCUT ) )					//key pressed?
	{
	int key = Fl::event_key();
	
	if( ( key == FL_Control_L ) || (  key == FL_Control_R ) ) ctrl_key = 1;
	if( ( key == FL_Shift_L ) || (  key == FL_Shift_R ) ) shift_key = 1;
	
	need_redraw = 1;
    dont_pass_on = 0;
	}


if ( e == FL_KEYUP )												//key release?
	{
	int key = Fl::event_key();
	
	if( ( key == FL_Control_L ) || ( key == FL_Control_R ) ) ctrl_key = 0;
	if( ( key == FL_Shift_L ) || (  key == FL_Shift_R ) ) shift_key = 0;

	need_redraw = 1;
    dont_pass_on = 0;
	}


if ( e == FL_MOUSEWHEEL )
	{
	mousewheel = Fl::event_dy();
	

	if( allow_mousewheel_adj ) 
		{
		if( ((My_Input_Wheel_Packable*)parent())->inside )
			{
			oparent->miw->mousewheel_external( step_wheel_boundary_idx, mousewheel, ctrl_key, shift_key );
			}
		}

	need_redraw = 1;
	dont_pass_on = 1;

/*
	if( b_allow_wheel_inc )
		{
		double dd;
//		s1 = Fl_Input::value();
        
        dd = get_value_as_double();
//		sscanf( s1.c_str(), "%lf", &dd );

//printf(" dd= %g\n", dd );
        double step = std_wheel_step;
        
        if( ctrl_key ) step = ctrl_wheel_step;
        if( shift_key ) step = shift_wheel_step;
     
		if( b_invert_wheel ) mousewheel = -mousewheel;

		if( mousewheel < 0 ) dd += step;
		if( mousewheel > 0 ) dd -= step;

        set_value_from_double( dd, s_printf_format.c_str() );
        
		if( !p_callback )
			{
			do_callback();														//use fltk callback?
			}
		else{
			p_callback( cb_obj, cb_args);
			}

		if( paired_obj ) 
			{
			double dv = get_value_as_double();
			((My_Input_Wheel_Packable*)parent())->set_val_internal_miw( dv );		//let parent know, so it can adj paired wdj
			}
		}

	if( mousewheel_p_cb ) mousewheel_p_cb( this, mousewheel_cb_args, Fl::event_dy() );
*/
	}






if ( need_redraw ) redraw();

if( dont_pass_on ) return 1;

return Fl_Box::handle( e) ;
}






//the 'My_Input_Wheel', 'fl_box' label and its optional slider are all internal to wdg, so when added to a Fl_Scroll/Fl_Pack both are within parent's boundaries
//'set_callback()' will also set callabacks for 'My_Input_Wheel' and optional 'fl_box' label/slider
//see
My_Input_Wheel_Packable::My_Input_Wheel_Packable( int x, int y, int w, int h, const char *label ) : Fl_Group( x, y, w, h, "" )
{
first_draw = 1;
p_callback = 0;

miw = new My_Input_Wheel( x, y, w, h, "" );						//height adj'd in init()
bx_label = new My_Input_Wheel_Packable_Box( x, y, w, h, "");	//height adj'd in init(), label/slider
bx_label->copy_label( label );

miw->paired_obj = 1;
end();

txtsize = 14;

inside = 0;
focused = 0;
b_last_change_was_by_dragging = 0;
}



const char* My_Input_Wheel_Packable::label()
{
return Fl_Group::label();
}


void My_Input_Wheel_Packable::label( const char *sz )
{
bx_label->copy_label( sz );
init();
}




void My_Input_Wheel_Packable::copy_label( string ss )
{
bx_label->copy_label( ss.c_str() );
init();
}


void My_Input_Wheel_Packable::textsize( int ii )
{
txtsize = ii;
}


void My_Input_Wheel_Packable::init()
{
int label_wid = fl_width( label() );

int algn = align();

if( algn == FL_ALIGN_LEFT )							//label pos?
	{
	int mid = w() / 2;
	miw->resize( x() + mid, y(), w() - mid, h());
	bx_label->resize( x(), y() , w() - mid, h() );
	}

if( algn == FL_ALIGN_RIGHT )
	{
	int mid = w() / 2;
	miw->resize( x(), y(), w() - mid, h());
	bx_label->resize( x() + mid, y() , w() - mid, h() );
	}

if( algn == FL_ALIGN_TOP )
	{
	int mid = h() / 2;
	miw->resize( x(), y() + mid, w(), h() - mid );
	bx_label->resize( x(), y(), w(), h() - mid );
	}


if( algn == FL_ALIGN_BOTTOM )
	{
	int mid = h() / 2;
	miw->resize( x(), y(), w(), h() - mid );
	bx_label->resize( x(), y() + mid, w(), h() - mid );
	}


bx_label->labelsize( labelsize() );				//use text details from parent
bx_label->labeltype( labeltype() );

miw->textsize( txtsize );
miw->labeltype( labeltype() );

//fl_draw_box( FL_DOWN_BOX, x(), y(), w(), h(), FL_BLACK  );

}


void My_Input_Wheel_Packable::set_id( int id_in )
{
id = id_in;
miw->id = id_in;
bx_label->id = id_in;

}


void My_Input_Wheel_Packable::set_id2( int id_in )
{
id2 = id_in;
miw->id2 = id_in;
bx_label->id2 = id_in;
}



void My_Input_Wheel_Packable::set_id3( int id_in )
{
id3 = id_in;
miw->id3 = id_in;
bx_label->id3 = id_in;
}


void My_Input_Wheel_Packable::slider_enable( bool enable )
{
bx_label->show_slider = enable;
}


void My_Input_Wheel_Packable::use_limit_min( bool enable )
{
b_use_limit_min = enable;
miw->b_use_limit_min = enable;
bx_label->b_use_limit_min = enable;
}


void My_Input_Wheel_Packable::use_limit_max( bool enable )
{
b_use_limit_max = enable;
miw->b_use_limit_max = enable;						//v1.02
bx_label->b_use_limit_max = enable;
}


void My_Input_Wheel_Packable::limit_min( double limit )
{
dlimit_min = limit;
miw->limit_min = limit;
bx_label->limit_min = limit;
}



void My_Input_Wheel_Packable::limit_max( double limit )
{
dlimit_max = limit;
miw->limit_max = limit;
bx_label->limit_max = limit;
}

void My_Input_Wheel_Packable::set_val_internal_miw( double val )
{
bx_label->set_val( val );
}


void My_Input_Wheel_Packable::set_val_internal_flbox( double val )
{
miw->set_value_from_double( val );
}




void My_Input_Wheel_Packable::set_callback( void (*p_cb)( void*, void* ), void *obj_in, void *args_in )
{
p_callback = p_cb;
cb_obj = obj_in;
cb_args = args_in;

miw->set_callback( p_cb, obj_in, args_in );
bx_label->set_callback( p_cb, obj_in, args_in );				//label/slider
}



void My_Input_Wheel_Packable::draw()
{

if( first_draw )
	{
	init();
	first_draw = 0;
	}

Fl_Group::draw();
//fl_rect( x(), y(), w(), h() );

}









int My_Input_Wheel_Packable::handle( int e )
{
string s1;
bool need_redraw = 0;
bool dont_pass_on = 0;



if ( e == FL_ENTER )
	{
//	printf( "My_Input_Wheel_Packable::handle() - FL_ENTER id %d %d %d\n", id, id2, id3 );
	inside = 1;
//	if( Fl::event_button() == 1 )											//left click
//		{
//		}
//    take_focus();
//    dont_pass_on = 1;
	need_redraw = 1;
//    dont_pass_on = 1;													//don't do this, so I beam cursor changes back to an arrow
    }


if ( e == FL_LEAVE )
	{
//	printf( "My_Input_Wheel_Packable::handle() - FL_LEAVE id %d %d %d\n", id, id2, id3 );
	inside = 0;
//	if( Fl::event_button() == 1 )											//left click
//		{
//		}
//    take_focus();
//    dont_pass_on = 1;
	need_redraw = 1;
//    dont_pass_on = 1;													//don't do this, so I beam cursor changes back to an arrow
    }





if ( need_redraw ) redraw();

if( dont_pass_on ) return 1;

return Fl_Group::handle( e) ;
}

//----------------------------------------------------------

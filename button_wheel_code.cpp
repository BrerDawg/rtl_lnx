/*
Copyright (C) 2026 BrerDawg

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

//button_wheel_code.cpp



//=====> REM, set 'when(FL_WHEN_CHANGED)' to correcly capture the state of mouse buttons, you will receive 2 callbacks for 
//one gui button press cycle, just check the mouse button flags to only process the pair of callbacks once,
//if you don't set 'when()' you will only receive callback when gui button is released and the mouse button flags will have been set to zero, so you
//can't tell which mouse button was pressed



//v1.01	---- 11-feb-2026                //
//v1.02	---- 02-may-2026                //added mouse button state vars, 'id0' , 'id1' , 'sname'
										//SET 'when(FL_WHEN_CHANGED)' to correcly capture the state of mouse buttons as mentioned above



#include "button_wheel_code.h"



cl_button_wheel::cl_button_wheel(int xx, int yy, int ww, int hh, const char *label) : Fl_Button( xx, yy, ww, hh, label )
{
mousewheel = 0;
b_invert_mousewheel = 0;
b_mousewheel_changed = 0;

left_button = middle_button = right_button = 0;
}





int cl_button_wheel::handle( int e )
{
string s1;
bool need_redraw = 0;
bool dont_pass_on = 0;


if ( e == FL_ENTER )
	{

	need_redraw = 0;
	dont_pass_on = 0;
	}


//if ( e == FL_PUSH )
//	{
//	b_mousewheel_changed = 0;											//clear this so callback knows what event caused callback

//	need_redraw = 1;
//	dont_pass_on = 0;
//	}


//if ( e == FL_RELEASE )
//	{
//	b_mousewheel_changed = 0;											//clear this so callback knows what event caused callback

//	need_redraw = 1;
//	dont_pass_on = 0;
//	}


if ( e == FL_PUSH )
	{
	b_mousewheel_changed = 0;											//clear this so callback knows what event caused callback

	if( Fl::event_button() == 1 ) left_button = 1;
	if( Fl::event_button() == 2 ) middle_button = 1;
	if( Fl::event_button() == 3 ) right_button = 1;
 
//    take_focus();
//    has_focus = 1;

//	if( p_mouse_button_changed_cb ) p_mouse_button_changed_cb( cb_mouse_button_changed_obj, cb_mouse_button_changed_args );
//    do_callback();
    
    dont_pass_on = 0;
    need_redraw = 1;
	}


if ( e == FL_RELEASE )
	{
	b_mousewheel_changed = 0;											//clear this so callback knows what event caused callback
	
	if( Fl::event_button() == 1 ) left_button = 0;
	if( Fl::event_button() == 2 ) middle_button = 0;
	if( Fl::event_button() == 3 ) right_button = 0;
 
//	if( p_mouse_button_changed_cb ) p_mouse_button_changed_cb( cb_mouse_button_changed_obj, cb_mouse_button_changed_args );

//    do_callback();

    dont_pass_on = 0;
    need_redraw = 1;
	}












if ( e == FL_MOUSEWHEEL )
	{
	b_mousewheel_changed = 1;											//set this so callback knows what event caused callback
	
	mousewheel = Fl::event_dy();
	
	if( b_invert_mousewheel ) mousewheel = -mousewheel;
	
//	printf( "cl_button_wheel::handle() - mousewheel: %d\n", mousewheel );

	do_callback();
	
	need_redraw = 1;
    dont_pass_on = 0;
	}


if ( need_redraw ) redraw();

if( dont_pass_on ) return 1;

return Fl_Button::handle(e);

}

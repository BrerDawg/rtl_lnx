/*
Copyright (C) 2025 BrerDawg

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
hint
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

//vert_meter.cpp
//v1.01 mgraph	 	2025-03-02		//



#include "vert_meter.h"



//------------------------------------------------------------------------
//v1.01		24-sep-2018
//1.0 gives max upward delfection
//1 or channels implemented, adj by eye: 'gap', 'barwid', 'gapcntr' 'peak_hold_countx'
vert_meter::vert_meter( int xx, int yy, int wid, int hei, const char *label=0 ) : Fl_Box( xx, yy, wid, hei, label )
{
channels = 2;	
gap_vert = 1;									//set by eye
gap_horiz = 2;
barwid = 5;
gapcntr = 1;

lev75_0 = 0.75;
lev75_1 = 0.75;

lev0 = 0;
lev1 = 0;

col_bkg = fl_rgb_color( 70, 70, 70 );

col_low = fl_rgb_color( 80, 220, 80 );			//green
col_high = fl_rgb_color( 240, 110, 50 );		//red

col_low_peak = fl_rgb_color( 80, 255, 80 );
col_high_peak = fl_rgb_color( 240, 180, 50 );

show_peak0 = 1;
show_peak1 = 1;

peak0 = 0;
peak1 = 0;

peak_hold_dec0 = 0;
peak_hold_dec1 = 0;
peak_hold_count0 = 20;
peak_hold_count1 = 20;

}



void vert_meter::set_levels( float f0, float f1 )
{
lev0 = f0;
lev1 = f1;

if( lev0 > 1.0 ) lev0 = 1.0;
if( lev1 > 1.0 ) lev1 = 1.0;

if( lev0 < 0.0 ) lev0 = 0.0;
if( lev1 < 0.0 ) lev1 = 0.0;

bool bredraw = 0;

if( lev0 != last_lev0 ) bredraw = 1;
if( lev1 != last_lev1 ) bredraw = 1;

last_lev0 = lev0;
last_lev1 = lev1;

if( peak_hold_dec0 != 0.0 ) bredraw = 1;
if( peak_hold_dec1 != 0.0 ) bredraw = 1;

if( bredraw ) redraw();
}


void vert_meter::draw()
{
Fl_Box::draw();

int wid = w()/2;

int hei = h()-2*gap_vert;


fl_rectf( x(), y(), w(), h(), col_bkg );

int lev_hei;



//ch0
if( lev0 > lev75_0 )
	{
	lev_hei = hei * (1.0f - lev0);
	
	fl_color( col_high );
	fl_rectf( x() + gap_horiz, y() + gap_vert + lev_hei, barwid, hei - lev_hei );					//red bar first, if any
	}

	
if( lev0 > lev75_0 ) lev_hei = hei * (1.0f - lev75_0);
else lev_hei = hei * (1.0f - lev0);

fl_color( col_low );
fl_rectf( x() + gap_horiz, y() + gap_vert + lev_hei, barwid, hei - lev_hei );					//green last to cover most part of red
	

//peak ch0
if( show_peak0 )
	{
	if( lev0 > peak0 ) {peak0 = lev0; peak_hold_dec0 = peak_hold_count0; }
	peak_hold_dec0--;
	if( peak_hold_dec0 <= 0 ) { peak_hold_dec0 = 0; peak0 = lev0; }

	lev_hei = hei * (1.0f - peak0);

	fl_color( col_low_peak );
	if( peak0 > lev75_0 ) fl_color( col_high_peak );
	fl_rectf( x() + gap_horiz, y() + gap_vert + lev_hei - 1, barwid, 2 );							//red bar first, if any
	}


//ch1
if( channels == 2 )
	{
	
	if( lev1 > lev75_1 )
		{
		lev_hei = hei * (1.0f - lev1);

		fl_color( col_high );
		fl_rectf( x() + w()/2 + gapcntr, y() + gap_vert + lev_hei, barwid, hei - lev_hei );		//red bar first, if any
		}

	
	if( lev1 > lev75_1 ) lev_hei = hei * (1.0f - lev75_1);
	else lev_hei = hei * (1.0f - lev1);

	fl_color( col_low );
	fl_rectf( x() + w()/2 + gapcntr, y() + gap_vert + lev_hei, barwid, hei - lev_hei );				//green last to cover most part of red
		

//peak ch1
	if( show_peak1 )
		{
		if( lev1 > peak1 ) {peak1 = lev1; peak_hold_dec1 = peak_hold_count1; }
		peak_hold_dec1--;
		if( peak_hold_dec1 <= 0 ) { peak_hold_dec1 = 0; peak1 = lev1; }

		lev_hei = hei * (1.0f - peak1);
		
		fl_color( col_low_peak );
		if( peak1 > lev75_1 ) fl_color( col_high_peak );
		fl_rectf(  x() + w()/2 + gapcntr, y() + gap_vert - 1 + lev_hei, barwid, 2 );
		}
	}
}





int vert_meter::handle( int e )
{
bool need_redraw = 0;
bool dont_pass_on = 0;

return Fl_Box::handle(e);			// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1

if ( e == FL_PUSH )	
	{
	need_redraw = 1;
    dont_pass_on = 0;
	}


if ( ( e == FL_KEYDOWN ) || ( e == FL_SHORTCUT ) )					//key pressed?
	{
	int key = Fl::event_key();
	
//	if( ( key == FL_Control_L ) || (  key == FL_Control_R ) ) ctrl_key = 1;
//	if( ( key == FL_Shift_L ) || (  key == FL_Shift_R ) ) shift_key = 1;
	
	need_redraw = 1;
    dont_pass_on = 0;
	}


if ( e == FL_MOUSEWHEEL )
	{
//	mousewheel = Fl::event_dy();
	need_redraw = 1;
    dont_pass_on = 0;
	}

if ( need_redraw ) redraw();

if( dont_pass_on ) return 1;

return Fl_Box::handle(e);
}
//------------------------------------------------------------------------


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


//vert_meter.h
//v1.01




#ifndef vert_meter_h
#define vert_meter_h




#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <locale.h>
#include <string>
#include <vector>
#include <wchar.h>
#include <math.h>
#include <algorithm>
#include <mutex>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Enumerations.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_JPEG_Image.H>
#include <FL/Fl_File_Input.H>
#include <FL/Fl_Input_Choice.H>

#include "GCProfile.h"





class vert_meter : public Fl_Box
{
private:										//private var
float lev0, lev1;
int peak_hold_dec0;
int peak_hold_dec1;
float peak0;
float peak1;
float last_lev0;
float last_lev1;

public:
Fl_Color col_bkg;
Fl_Color col_low;
Fl_Color col_high;

Fl_Color col_low_peak;
Fl_Color col_high_peak;

int channels;
int gap_horiz;
int gap_vert;
int barwid;
int gapcntr;
bool show_peak0;
bool show_peak1;
float lev75_0;									//level for 75% (green to red point)
float lev75_1;
int peak_hold_count0;							//number of draw calls before peak is zeroed
int peak_hold_count1;

public:											//public var
vert_meter( int xx, int yy, int wid, int hei, const char *label );
void set_levels( float f0, float f1 );

private:										//private var
void draw();
int handle( int e );
};





#endif

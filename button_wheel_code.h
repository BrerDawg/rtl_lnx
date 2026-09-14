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



//button_wheel_code.h
//v1.02

#ifndef button_wheel_code_h
#define button_wheel_code_h

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <locale.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <algorithm>




#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/fl_ask.H>
#include <FL/Enumerations.H>

#include "GCProfile.h"


using namespace std;



class cl_button_wheel : public Fl_Button
{
private:


public:
int id0, id1;															//useful if there are a matrix of buttons
string sname;															//can be used to hold string that is not displayed

int mousewheel;
bool b_invert_mousewheel;
bool b_mousewheel_changed;												//use this to tell what caused the callback
bool left_button;
bool middle_button;
bool right_button;


private:
int handle(int);

public:
cl_button_wheel(int x,int y,int w, int h,const char *label=0);

	
};



#endif

/*
Copyright (C) 2023 BrerDawg

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




//a_canvas_demo.h
//v1.01





#ifndef a_canvas_demo_h
#define a_canvas_demo_h


#define _FILE_OFFSET_BITS 64			//large file handling, must be before all #include...
//#define _LARGE_FILES

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <locale.h>
#include <string>
#include <vector>
#include <wchar.h>
#include <algorithm>
#include <math.h>

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

#include "globals.h"
#include "GCProfile.h"
#include "aa_canvas.h"


//linux code
#ifndef compile_for_windows

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
//#include <X11/Xaw/Form.h>							//64bit
//#include <X11/Xaw/Command.h>						//64bit

#include <dirent.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <syslog.h>		//MakeIniPathFilename(..) needs this
#endif


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



#define cn_bytes_per_pixel 3


using namespace std;







class cl_a_canvas_demo : public Fl_Double_Window
{
private:

public:											//public functions
bool ctrl_key, shift_key;
unsigned int bkgd_ww;
unsigned int bkgd_hh;
unsigned int dwnsmpl_wid;
unsigned int dwnsmpl_hei; 

unsigned int layer0;
unsigned int layer1;
unsigned int layer2;
unsigned int layer3;
unsigned int bytes_per_pixel;
float scale_factor;
int kern_size;
Fl_Box *bx_image0;
Fl_Box *bx_image1;
Fl_RGB_Image *bmp0;
Fl_RGB_Image *bmp1;
unsigned char *bf0;
unsigned char *bf1;
st_aa_canvas_polygon_tag poly0, poly1, poly2;
float theta_hr_hand;

mystr mtimer;
float timer_avg;
int timer_avg_cnt;

int idbg0, idbg1;
int idbg2, idbg3;

public:											//public functions
cl_a_canvas_demo( int xx, int yy, int wid, int hei, const char *label );
~cl_a_canvas_demo();

void tick( float dt );

private:
int handle(int e);

};











#endif

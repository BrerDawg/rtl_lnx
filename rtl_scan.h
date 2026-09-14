/*
Copyright (C) 2010-2026 BrerDawg, et. al.

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

//rtl_scan.h
//v1.01		15-02-14			


#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <locale.h>
#include <string>
#include <vector>
#include <wchar.h>
#include <sys/file.h>												//refer 'check_instance_try_lock()'

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



#include "globals.h"

#include "pref.h"
#include "GCProfile.h"
#include "GCLed.h"
#include "gclog.h"
#include "gcthrd.h"
#include "gcpipe.h"
#include "rtl_graph.h"
#include "rtlobj.h"

#include "gc_rtaudio.h"
#include "rt_code.h"
#include "demod_code.h"
//#include "elliptical_iir_code.h"
#include "fm_demode_code.h"
//#include "freqz_code.h"
//#include "iir_sos_code.h"
#include "halfband_poly_optimised_float.h"


//linux code
#ifndef compile_for_windows

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>

#define _FILE_OFFSET_BITS 64			//large file handling
//#define _LARGE_FILES
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


using namespace std;

#define cnFontEditor 4
#define cnSizeEditor 12

#define cnGap 2
#define cnCslHei 270

#define cnsLogName "log.txt"						//log filename
#define cnsLogSwapName "log_swap.txt"	//log swap file for culling older entries at begining of log file

#define cn_max_tune_history 100

#define cn_timer_period 0.1f









class mywnd : public Fl_Double_Window
{
private:										//private var
int *buf;
int ctrl_key;
int left_button;
string dropped_str;

int mousewheel;
Fl_Box *bx_image;
Fl_JPEG_Image *jpg;


public:											//public var
int a_public_var;

public:											//public functions
mywnd( int xx, int yy, int wid, int hei, const char *label );
~mywnd();
void init();

private:										//private functions
void draw();
int handle( int );
void setcolref( colref col );

};









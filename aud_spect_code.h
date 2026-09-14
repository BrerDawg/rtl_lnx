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


//aud_spect_code.h
//v1.01



#ifndef aud_spect_code_h
#define aud_spect_code_h

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <locale.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <algorithm>
#include <atomic>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/fl_ask.H>
#include <FL/Enumerations.H>
#include "GCProfile.h"
#include "mgraph.h"
#include "filter_code.h"
#include "rtl_graph.h"



using namespace std;



#define cn_filters_max 32


#define cn_aud_spect_notch_max 2

struct st_aud_notch_gui_ctrls_tag										//holds one aud spectrum notch filter, dynamically alloc by user clicking aud spect graph
{
bool active;
unsigned int filt_idx;													//index in 'st_filt[]'

Fl_Group *gp;
Fl_Round_Button* bt_sel;
Fl_Check_Button* ck_en;
My_Input_Wheel *miw_fc;
My_Input_Wheel *miw_Q;

};




/*
struct st_audio_spect_callback_items_tag								//holds indexes for callback processing
{
unsigned int filt_idx;													//index into 'st_filt[]'
unsigned int gui_idx;													//index into 'st_aud_notch_gui_ctrls[]'
}
*/










#define cn_spect_audio_avg_cnt 8										//avg over this many frames
#define cn_spect_audio_avg_bf_max 16384

struct st_audio_spect_avg_tag
{
float bf_spect_ch0[ cn_spect_audio_avg_cnt ][ cn_spect_audio_avg_bf_max ];
float bf_avg_ch0[ cn_spect_audio_avg_bf_max ];							//this is the average across 'num_bf_in_use'
int spect_size;															//size of spec store in 'bf_spect_ch0[n][]', must be less than 'st_notch_audio_bf_max' 
int cur_wr_bf;
int	num_bf_in_use;														//number of buffers being written

float z0 = 0;															//dc blocker delay
float z1 = 0;


};










class cl_aud_spect_wnd : public Fl_Double_Window
{
private:

public:
mgraph *gph;
int notch_sel_idx;
bool b_Q_adj;
float gainy;
float posy;

public:
cl_aud_spect_wnd( int xx, int yy, int ww, int hh, const char *label );
void plot( unsigned int srate_in, vector<float> &vf0 );
void update_gph_user_obj( int srate_in );

private:

};


#endif

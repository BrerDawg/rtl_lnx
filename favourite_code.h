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

//favourite_code.h
//v1.01


#ifndef favourite_code_h
#define favourite_code_h



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
#include <FL/Fl_Table_Row.H>

#include "globals.h"
#include "rtl_graph.h"
#include "GCProfile.h"
#include "GCLed.h"
#include "my_input_wheel.h"
//#include "rtl_graph.h"
#include "gc_input_multiline.h"


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

#define cn_header_posy 15

#define cn_favourite_row_max 256
#define cn_favourite_box_header_max 25





enum en_header_sort_order_tag
{
en_hso_none,
en_hso_increasing,							//down arrow
en_hso_decreasing,							//up arrow
en_hso_mixed,								//double arrow, sort governed by another column

};





enum en_header_param_tag
{
en_hpm_active,
en_hpm_sname,
en_hpm_scomment0,
en_hpm_sgroup,
en_hpm_freq,
en_hpm_freq_center,
en_hpm_demod_type,
en_hpm_b_use_dwn_aa,
en_hpm_dwn_srate,
en_hpm_b_use_iffreq,
en_hpm_if_freq,
en_hpm_b_bw_bpass,
en_hpm_iffreq_bw_low,
en_hpm_iffreq_bw_high,
en_hpm_iffreq_bw_taps,
en_hpm_dev_gain,
en_hpm_b_agc,
en_hpm_audio_gain,
en_hpm_deemph,
en_hpm_sdate,
en_hpm_stime,
en_hpm_bias_t,
en_hpm_direct_sampling,
en_hpm_iq_gain,
en_hpm_sdev_manufacturer,
//en_hpm_sname_long,
//en_hpm_scomment1,
//en_hpm_sworld_coord,
//en_hpm_dev_srate,
//en_hpm_dev_ppm,
//en_hpm_sdev_name,
//en_hpm_dev_prod,
//en_hpm_dev_id,
};





enum en_fav_row_ctrl_id_tag			//NOTE THIS forms the lower 16 bits (column) and 'row_id' forms the higher 16 bits, see 'cb_fav_row_combo()'
{
en_frci_ld_active,
en_frci_fi_name,
en_frci_fi_comment0,
en_frci_fi_group,
en_frci_fi_freq,
en_frci_fi_freq_center,
en_frci_fi_demod_type,
en_frci_ld_b_use_dwn_aa,
en_frci_fi_dwn_srate,
en_frci_ld_b_use_iffreq,
en_frci_fi_if_freq,
en_frci_ld_b_bw_bpass,
en_frci_fi_iffreq_bw_low,
en_frci_fi_iffreq_bw_high,
en_frci_fi_iffreq_bw_taps,
en_frci_fi_dev_gain,
en_frci_ld_b_agc,
en_frci_fi_audio_gain,
en_frci_ld_deemph,
en_frci_fi_date,
en_frci_fi_time,
en_frci_ld_bias_t,
en_frci_ld_direct_sampling,
en_frci_fi_iq_gain,
en_frci_fi_dev_manufacturer,

};



struct st_favourite_header_dim_tag
{
int x, y, w, h;
int label_offs_x;
};





/*
class cl_fi_input_click : public Fl_Input
{
public:
int id0;

bool ctrl_key, shift_key;
bool left_button;
bool right_button;
bool middle_button;
int mousex, mousey;
int mousewheel;

public:
cl_fi_input_click( int xx, int yy, int wid, int hei, const char *label );
~cl_fi_input_click();


private:
int handle( int e );
};
*/










class cl_box_header : public Fl_Box
{
private:
bool ctrl_key, shift_key;
bool left_button;
bool right_button;
bool middle_button;
int mousex, mousey;
int mousewheel;
int menu_hei;
int label_offs_x;

public:
int id0;
en_header_param_tag param;
bool b_sort_order_dimmed;
bool b_sort_order_hidden;
en_header_sort_order_tag sort_order;
string stooltip;

private:
int handle( int e );
void draw();




public:
cl_box_header( int xx, int yy, int wid, int hei, const char *label, int label_offs_x_in );
~cl_box_header();

};




class cl_fav_text_input : public Fl_Input
{
private:


public:
bool ctrl_key, shift_key;
bool left_button;
bool right_button;
bool middle_button;
int mousex, mousey;
int mousewheel;
int id0;
int id1;
bool text_changed;
int sel;
bool b_type_comment;										//user comment editbox, this control will respond to a mouse click and open up a bigger editbox	


private:
int handle( int e );
void draw();


public:
cl_fav_text_input( int xx, int yy, int wid, int hei, const char *label );
~cl_fav_text_input();


};







enum en_favourite_status_tag		//keep BINARY to allow oring/anding
{
en_fvs_active = 0x1,
en_fvs_disabled = 0x2,
en_fvs_hidden = 0x4,
en_fvs_sel = 0x8,
en_fvs_highlight = 0x10,
en_fvs_marked = 0x20,				//see 'marked_idx'
en_fvs_flagged = 0x40,
en_fvs_flashing = 0x80,
en_fvs_font_bold = 0x100,
en_fvs_font_italic = 0x200,
en_fvs_font_underline = 0x400,
};







struct st_favourite_freq_tag
{
en_favourite_status_tag status;

string sdev_name;
string sdev_manufacturer;
string sdev_prod;
string sdev_serial;

int marked_idx;

int tuned_count;						//number of times tuned to

int64_t freq;															//freq_center + freq_sub_tune
int64_t freq_center;

//int freq_offset;

bool b_dwn_aa;
int i_dwn_srate;

bool b_use_iffreq;

int64_t if_freq;

bool b_bw_bpass;
int64_t iffreq_bw_low;
int64_t iffreq_bw_high;
int iffreq_bw_taps;


en_demodulator_type_tag demod_type;

int dev_srate;
float dev_gain;
int dev_ppm;
int dev_direct_sampling;
bool dev_offset_tuning;
bool dev_bias_t;

bool b_agc;
float audio_gain;
int deemph;

int reception_quality;					//0-->10
float last_tuned_signal_lev;


int font_type;
int font_size;
int colbkg_r, colbkg_g, colbkg_b;
int col_r, col_g, col_b;				//text color
int colsel_r, colsel_g, colsel_b;		//selected colour

string sgroup;
string sname;
string sname_long;
//string scomment;
string stext_flashing;
string sworld_coord;					//e.g lat/lgtde
string sdate;
string stime;

float iq_gain;

string scomment0;
};






struct st_favourite_row_ctrl_tag
{
GCLed *ld_active;
cl_fav_text_input *fi_name;
cl_fav_text_input *fi_comment0;
cl_fav_text_input *fi_group;
cl_fav_text_input *fi_freq;
cl_fav_text_input *fi_freq_center;				
cl_fav_text_input *fi_demod_type;
cl_fav_text_input *fi_dev_manufacturer;
GCLed *ld_b_use_iffreq;
GCLed *ld_b_dwn_aa;
cl_fav_text_input *fi_dwn_srate;
cl_fav_text_input *fi_if_freq;
GCLed *ld_b_bw_bpass;
cl_fav_text_input *fi_iffreq_bw_low;
cl_fav_text_input *fi_iffreq_bw_high;
cl_fav_text_input *fi_iffreq_bw_taps;
cl_fav_text_input *fi_dev_gain;
GCLed *ld_b_agc;
cl_fav_text_input *fi_audio_gain;
GCLed *ld_deemph;
cl_fav_text_input *fi_date;
cl_fav_text_input *fi_time;
GCLed *ld_bias_t;
GCLed *ld_direct_sampling;
cl_fav_text_input *fi_iq_gain;
};














class cl_favourite_row : public Fl_Group
{
private:


public:
int id0;
int id1;
st_favourite_header_dim_tag hdr_dim[cn_favourite_row_max];				//used to position top headings

st_favourite_row_ctrl_tag ctrl;


public:
cl_favourite_row( int xx, int yy, int ww, int hh, int id_in0, int id_in1, const char *label );
~cl_favourite_row();

};







class cl_favourite_wnd : public Fl_Double_Window
{
private:
bool ctrl_key, shift_key;
bool left_button;
bool right_button;
bool middle_button;
int mousex, mousey;
int mousewheel;
int menu_hei;

public:
Fl_Menu_Bar *menu_fav;
Fl_Window *wnd_header;
int st_fav_ctrl_dim_tag[cn_favourite_box_header_max];
int last_header_offsx;
float tick_secs_passed;
float header_align_time_period;
float header_align_time_count;
int flag_to_tune_using_fav_idx;											//a >=0 idx value here is a prompt for 'wnd_rtl_graph' to make a fav tuning
int last_wid;
int last_hei;
int row_sel;


cl_favourite_row* grp_row[cn_favourite_row_max];
cl_box_header* bx_hdr[cn_favourite_box_header_max];						//for headings
//Fl_Group *fl_scrol_grp;
Fl_Scroll *fl_scrol;




private:
int handle( int e );




public:
cl_favourite_wnd( int xx, int yy, int wid, int hei, const char *label );
~cl_favourite_wnd();
void header_align( bool force );
void tick( float dt );
void clear();
void set_via_vector( vector<st_favourite_freq_tag> vfav, char szdemod[16][32], bool sanitise );
void load_vector_from_gui_ctrls( vector<st_favourite_freq_tag>&vv, bool sanitise );
void set_leds_column0( int do_nothing_if_state_is, bool new_state );
void sort_order_change( int idx, int order );
void sort_move_blanks_to_end_for_param_sname();
void sort_move_blanks_to_end_for_param_sgroup();
void sort_move_blanks_to_end_for_param( en_header_param_tag pp );
void set_sel_row( unsigned int row );
void store_to_sel_fav( st_favourite_freq_tag o );
int store_to_free_slot( st_favourite_freq_tag o );
void sanitise_using_limit( vector<st_favourite_freq_tag> &vv );
int find_free_slot( vector<st_favourite_freq_tag>vv );
//void resize(int x, int y, int w, int h);

};







#endif

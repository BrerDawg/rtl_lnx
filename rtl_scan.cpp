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


//rtl_scan.cpp					//refer to rtl_graph.cpp for history of mods
//v1.01		15-02-14			//
//v1.02		24-Nov-2022			//upgraded to use 64 bit 'librtlsdr' (-lrtlsdr) library obtained via synaptic, moded to uses rtaudio instead of '/dev/dsp'

//!!!!!!!!!  19-sep-2016 -- with tahrpup605 use '-lusb-1.0' and you'll need to free up terratec from dvb driver using: rmmod dvb_usb_rtl28xxu !!!!!!!!

//mingw needs library -lmsvcp60 for mcrtomb type calls:
//from Makefile.win that dev-c++ uses, these params were used to compile and link:
//LIBS =  -L"C:/Dev-Cpp/lib" -lmsvcp60 -lfltk_images -lfltk_jpeg -mwindows -lfltk -lole32 -luuid -lcomctl32 -lwsock32 -lm 
//INCS =  -I"C:/Dev-Cpp/include" 
//CXXINCS =  -I"C:/Dev-Cpp/lib/gcc/mingw32/3.4.2/include"  -I"C:/Dev-Cpp/include/c++/3.4.2/backward"  -I"C:/Dev-Cpp/include/c++/3.4.2/mingw32"  -I"C:/Dev-Cpp/include/c++/3.4.2"  -I"C:/Dev-Cpp/include" 
//CXXFLAGS = $(CXXINCS)  
//CFLAGS = $(INCS) -DWIN32 -mms-bitfields 

//#define compile_for_windows	//!!!!!!!!!!! uncomment for a dos/windows compile


//NOTE: use the flag: 'b_use_synthesis_dont_use_rtl_dev'   ------->				flag TO SKIP probing a PHYSICAL rtl device, see also cmdline flag: 'no_rtl=1''       <-------

#include "rtl_scan.h"
#include "icon_rtl_32x.xpm"
#include "icon_ntc_32x.xpm"
#include "icon_prb_32x.xpm"
#include "icon_fav_32x.xpm"



//asm("int3");						//useful to trigger debugger



extern bool b_use_synthesis_dont_use_rtl_dev;



//global objs
Fl_Window *wndMain;
Fl_Light_Button *btToday;
Fl_Button *ckMutex;
Fl_Input *fi_status;
GCLed *led1;
Fl_RGB_Image *img_icon_ntc;
Fl_RGB_Image *img_icon_prb;
Fl_RGB_Image *img_icon_fav;

vector<string>vlog;
gcthrd *thrd1 = 0;
gcthrd *thrd2 = 0;

extern gcrtl rtl;							//rtl tuner dongle

//gcthrd *thrd_pipe = 0;					//used by the gcpipe test code

//gcpipe *pipe1 = 0;


PrefWnd* pref_wnd = 0;
PrefWnd* pref_wnd2=0;
PrefWnd* font_pref_wnd=0;
Fl_Text_Buffer *tb_csl = 0;
Fl_Text_Editor *te_csl = 0;
rtl_graph_wnd *wnd_rtl_graph = 0;

//menu
Fl_Menu_Bar* meMain;


//global vars
string csIniFilename;
int iBorderWidth;
int iBorderHeight;
int gi0,gi1,gi2,gi3,gi4;
double gd0,gd1,gd2,gd3,gd4;
string app_path;						//path this app resides on
string dir_seperator="\\";				//assume dos\windows folder directory seperator
string sglobal;
string sg_col1, sg_col2;
int gi;
//int mode;
int use_mutex;
int font_num = cnFontEditor;
int font_size = cnSizeEditor;
//static Fl_Font extra_font;
unsigned long long int ns_tim_start1;	//a ns timer start val
double perf_period;						//used for windows timings in timing_start( ), timing_stop( )
int ilast_device_index = 0;
string fav_editor;
int imode = en_mode_stop;
bool b_first_timer_call = 0;
int start_up_state = 0;


int iAppExistDontRun = 1;	//if set, app probably exist and user doesn't want to run another
							//set it to assume we don't want to run another incase user closes
							//wnd without pressing either 'Run Anyway' or 'Don't Run, Exit' buttons
int check_instance_fd = -1;


#ifndef compile_for_windows
//Display *gDisp;
#endif

extern int aud_op_srate;
extern int downsample_srate;
extern int fm_decimator_srate;


int rtl_scan_count;
int rtl_scan_cur_count;
int rtl_scan_cur_freq;
int rtl_scan_freq_step;
extern vector<st_spect_tag> vspect;
extern vector<st_spect_tag> vspect_displayble;
extern vector<st_spect_tag> vspect_gph;
extern vector<filter_code::st_cplex_tag> vcplex_gph;
extern int i_fftw_trig_plan_create_state;



//function prototypes
void LoadSettings(string csIniFilename);
void SaveSettings(string csIniFilename);
//int CheckInstanceExists(string csAppName);
void open_editor( string fname );
void cb_bt_gcthrd_stop(Fl_Widget *, void *);
void cb_bt_thread_stop(Fl_Widget *, void *);
void cb_bt_close_gcpipe( Fl_Widget *, void * );
int RunShell( string sin );
//void extern call_rtl_fm_kill();
extern void cb_bt_freq_start( Fl_Widget *, void * );
extern void cb_bt_freq_stop( Fl_Widget *, void * );
void cb_bt_freq_single( Fl_Widget *, void * );
extern void do_scan();
extern void buf_allocate();
extern void buf_free();
extern void stop_audio();
extern void stop_threads();
void tops_thread_clear_file();
void filter_adjust_fir_bw_lower_upper();
//extern bool create_hilbert_fir_filters();
//extern void create_lsb_usb_iq();


//callbacks
void cb_wndmain(Fl_Widget*, void* v);
//void cb_btAbout(Fl_Widget *, void *);
//void cb_btOpen(Fl_Widget *, void *);
void cb_btSave(Fl_Widget *, void *);
void cb_btQuit(Fl_Widget *, void *);
void cb_btTest(Fl_Widget *, void *);
void cb_timer1(void *);
void cb_pref(Fl_Widget*w, void* v);
void cb_pref2(Fl_Widget*w, void* v);
void cb_font_pref(Fl_Widget*w, void* v);
void cb_open(Fl_Widget *, void *);
void cb_bt_open_gcpipe( Fl_Widget *, void * );
void cb_bt_close_gcpipe( Fl_Widget *, void * );

extern void plot_gph01();
extern void init_fftw_arrays();
extern void set_synth_mode_and_menu_state( bool bstate );
extern void filter_test_sweep();

extern int pref_taper_windowing_for_fft;
extern int pref_zero_padding_for_fft;
extern int pref_zero_padding_for_fft_cnt;
extern int pref_wfall_non_linear_gain;
extern int pref_freq_mousewheel_low_digit_zero_cnt;
extern int pref_freq_left_click_drag_low_digit_zero_cnt;
extern int pref_freq_right_click_drag_low_digit_zero_cnt;

extern int pref_show_dc_for_fft_graphs;

//extern int pref_show_graph_onboard_tuner_freq;
extern int pref_show_graph_hz_units;
extern int pref_show_graph_hz_eng;

extern double pref_db_ref_A;
extern double pref_db_ref_B;
extern double pref_db_ref_C;
extern string pref_db_ref_suffix;
extern bool pref_ask_preset_overwrite;

extern string pref_scol_gph0_trace_col;
extern int col_gph0_trace_col_r;
extern int col_gph0_trace_col_g;
extern int col_gph0_trace_col_b;


extern string pref_scol_xaxis_text_freq;
extern int col_xaxis_text_freq_r;
extern int col_xaxis_text_freq_g;
extern int col_xaxis_text_freq_b;


extern string s_tops_thread_stats;
extern int tops_thread_stats_cnt;
extern float tops_thread_stats_avg_sum;
extern float tops_thread_stats_avg;

extern bool b_dbg_no_graph_update;
extern bool b_need_filter_rebuild;
extern bool b_need_h_zoom_recalc;


extern st_fftw_plans_tag st_fftw[ cn_fftw_plan_siz ];
extern void fftw_clear_all_structs();
extern bool fftw_build_if_req( unsigned int idx, bool b_destroy_only, string sname_in, en_fftw_plans_type_tag typ, unsigned int size );
extern void fftw_destroy_all_structs();

extern void filter_create_new();
extern st_filter_sdr_tag st_filt[ cn_filters_max ];



enum
{
enNone,
enStop,
enRun
};



struct sThreadTag
{
int iKillThread;
int iThreadFinished;
};


sThreadTag sThrd1;


//linux code
#ifndef compile_for_windows
void* Thread1 (void* pParams);
pthread_t thread1_id;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
#endif

//windows code
#ifdef compile_for_windows
DWORD WINAPI DosThread(void* lpData);
HANDLE h_mutex1; 
#endif



rtaud rta;																//v1.02 rtaudio
st_rtaud_arg_tag st_rta_arg;											//v1.02 this is used in audio proc callback to work out chan in/out counts
float time_per_sample;													//v1.02 
bool audio_started = 0;													//v1.02

extern bool threads_rtl_started;
extern bool threads_started;
int audio_source = 0;													//v1.02 
extern bool start_audio();
extern void start_threads_rtl();
extern void start_threads();
mystr tim;
mystr tim0;																//just for timing callback periods
mystr tim1; 


vector<string> vtunehist2;												//tune history
extern bool rtl_get_graph();
extern bool b_listen;
float gph_scaley = 1.0f;
int gph_loc_sel = 0;													//refer: 'menu_graph_loc_sel'
int gph_loc_sel_tmp = 0;
int gph_samples_per_sec = 0;
													
extern double g_dev_bw;
extern unsigned int framecnt;

//extern char szdev_name[256];
//extern char szdev_manufact[256];
//extern char szdev_product[256];
//extern char szdev_serial[256];

extern string s_dev_name;
extern string s_dev_manufact;
extern string s_dev_product;
extern string s_dev_serial;


extern bool b_plot_gph1;
extern bool b_plot_gph2;

extern filter_code::st_iir usr_rtl_srate_lpf_iir_I0;
extern filter_code::st_iir usr_rtl_srate_lpf_iir_Q0;


extern filter_code::st_iir agc_iir0;

extern filter_code::st_iir usr_hpf_iir0_I0;
extern filter_code::st_iir usr_hpf_iir0_Q0;

extern filter_code::st_iir usr_lpf_iir0_I0;
extern filter_code::st_iir usr_lpf_iir0_Q0;

extern filter_code::st_iir usr_lpf_iir1_I0;
extern filter_code::st_iir usr_lpf_iir1_Q0;

extern filter_code::st_iir usr_lpf_iir2_I0;
extern filter_code::st_iir usr_lpf_iir2_Q0;


extern int g_user_iir_lpf0;
extern int g_user_iir_lpf1;
extern int g_user_iir_lpf2;

//extern filter_code::st_iir usr_notch_iir0;
//extern int g_user_iir_notch_freq0;
//extern float g_user_iir_notch_Q_0;


extern filter_code::st_iir usr_demod_hpf_iir_I0;
extern filter_code::st_iir usr_demod_hpf_iir_Q0;

extern filter_code::st_iir usr_demod_hpf_iir_I1;
extern filter_code::st_iir usr_demod_hpf_iir_Q1;


extern filter_code::st_iir wfall_iir0;



extern int g_bw_taps;

extern filter_code::st_fir usr_demod_bpf_fir_I0;
extern filter_code::st_fir usr_demod_bpf_fir_Q0;
extern filter_code::st_fir ssb_fir_lsb_I0;
extern filter_code::st_fir ssb_fir_lsb_Q0;
extern filter_code::st_fir ssb_fir_usb;

extern filter_code::st_fir fir_hilbert_45_plus;
extern filter_code::st_fir fir_hilbert_45_minus;

extern iir_sos::st_iir_sos_tag fm_stereo_dwnsmpl_aa;

extern halfband_poly_optimised_float::st_hbpo_tag fm_stereo_decimator;
extern filter_code::st_fir fm_ster_fir_15k_lpf0;
extern filter_code::st_fir fm_ster_fir_15k_lpf1;
extern filter_code::st_fir fm_ster_fir_23_53k_bpf;

extern filter_code::st_iir fm_ster_iir_19k_notch;


//--------------------- Main Menu --------------------------
Fl_Menu_Item menuitems[] =
{
	{ "&File",              0, 0, 0, FL_SUBMENU },
//		{ "&Open", FL_CTRL + 'o'	, (Fl_Callback *)cb_btOpen, 0 },
		{ "&Open...", FL_CTRL + 'o'	, (Fl_Callback *)cb_open, 0 },
		{ "&Save...", FL_CTRL + 's'	, (Fl_Callback *)cb_btSave, 0, FL_MENU_DIVIDER },
		{ "E&xit", FL_CTRL + 'q'	, (Fl_Callback *)cb_btQuit, 0 },
		{ 0 },

	{ "&Edit", 0, 0, 0, FL_SUBMENU },
		{ "&Preferences..",  0, (Fl_Callback *)cb_pref},
		{ "&Preferences2..",  0, (Fl_Callback *)cb_pref2},
		{ "&Font Pref..",  0, (Fl_Callback *)cb_font_pref},	
		{ 0 },

	{ "&Help", 0, 0, 0, FL_SUBMENU },
//		{ "&About", 0				, (Fl_Callback *)cb_btAbout, 0 },
		{ 0 },


	{ 0 }
};
//------------------need_redraw----------------------------------------
































void cb_dir_open( Fl_Widget *w, void* v )
{
string s1, s2;
//linux code
#ifndef compile_for_windows
s1 = "/mnt/home/PuppyLinux/MyPrj/Skeleton Unicode fltk";
#endif

//windows code
#ifdef compile_for_windows
s1 = "c:\\gc\\MyPrj\\Skeleton Unicode fltk";
#endif


}
























void clear_csl()
{
if ( tb_csl ==0 ) return;
int len = tb_csl->length();
if( len > 0 ) tb_csl->replace(0,len,"");		//clear text buf if anything there
}




void outcsl(string s)
{

if ( te_csl ==0 ) return;

te_csl->insert( s.c_str() );
te_csl->show_insert_position();

}








void cols_build()
{
string s1;

int rr;
int gg;
int bb;
	
	{
	s1 = pref_scol_gph0_trace_col;
	sscanf( s1.c_str(), "%d,%d,%d", &rr, &gg, &bb );	
		
//	printf("cols_build() - '%s'   rr %d %d %d\n", s1.c_str(), rr, gg, bb );
		
	col_gph0_trace_col_r = rr;
	col_gph0_trace_col_g = gg;
	col_gph0_trace_col_b = bb;
	}

	{
	s1 = pref_scol_xaxis_text_freq;
	sscanf( s1.c_str(), "%d,%d,%d", &rr, &gg, &bb );	
		
//	printf("cols_build() - '%s'   rr %d %d %d\n", s1.c_str(), rr, gg, bb );
		
	col_xaxis_text_freq_r = rr;
	col_xaxis_text_freq_g = gg;
	col_xaxis_text_freq_b = bb;
	}





}




#define cn_pref_scol_gph0_trace_col 1000
#define cn_pref_scol_xaxis_text_freq 1001


//this is the callback that is called by buttons that specified is in - 
//definition ....pref_wnd->sc.cb=(void*)cb_user;
//'*o' is the PrefWnd* ptr 
//'row' is the row the control lives on, 0 is first row
//'ctrl' is the number of the controlon that row, 0 is first control
void cb_user(void *o, int row,int ctrl)
{
printf("\ncb_user() - Ping by Control on Row=%d at control count Ctrl=%d on this row.\n", row, ctrl );

PrefWnd *w = (PrefWnd *) o;


b_need_h_zoom_recalc = 1;

unsigned int id = w->ctrl_list[ row ][ ctrl ].uniq_id;


if( id == cn_pref_scol_gph0_trace_col )
	{
	GCCol *oc = (GCCol *) w->ctrl_ptrs[ row ][ ctrl ];
	
	pref_scol_gph0_trace_col = *w->ctrl_list[ row ][ ctrl ].sretval;
	
	cols_build();
	}


if( id == cn_pref_scol_xaxis_text_freq )
	{
	GCCol *oc = (GCCol *) w->ctrl_ptrs[ row ][ ctrl ];
	
	pref_scol_xaxis_text_freq = *w->ctrl_list[ row ][ ctrl ].sretval;
	
	cols_build();
	}

//i_fftw_trig_plan_create_state = 0; 		//start the transition state going which will create require fftw plans
//sync_wr_rd_pointer = 1;					//trigger a wr/rd pointer reset

}






void cb_pref2(Fl_Widget*w, void* v)
{
if(pref_wnd2) pref_wnd2->Show(1);
}




void make_pref2_wnd()
{
sControl sc;

if(pref_wnd2==0)
	{
	pref_wnd2 = new PrefWnd(wndMain->x()+20,wndMain->y()+20,700,330,"Preferences2","Settings","PrefWnd2Pos");
	}
else{
	pref_wnd2->Show(1);
	return;
	}


// -- dont need to do the below manual default load as "ClearToDefCtrl()" does this for you --
/*
pref_wnd2->sc.type=cnNone;						//blank gap from top of window
pref_wnd2->sc.x=105;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=150;
pref_wnd2->sc.h=20;
pref_wnd2->sc.label="";
pref_wnd2->sc.label_type=FL_ALIGN_CENTER;
pref_wnd2->sc.tooltip="";						//tool tip
pref_wnd2->sc.options="";	//menu button drop down options
pref_wnd2->sc.labelfont=-1;					//-1 means use fltk default
pref_wnd2->sc.labelsize=-1;					//-1 means use fltk default
pref_wnd2->sc.textfont=fl_font();
pref_wnd2->sc.textsize=fl_size();
pref_wnd2->sc.section="";
pref_wnd2->sc.key="";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="";							//default to use if ini value not avail
pref_wnd2->sc.iretval=(int*)-1;					//address of int to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb=0;								//address of a callback if any, 0 means none


pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values
pref_wnd2->AddControl();
pref_wnd2->CreateRow(10);			//specify optional row height
*/

pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnMenuChoicePref;
pref_wnd2->sc.x=120;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=150;
pref_wnd2->sc.h=20;
pref_wnd2->sc.label="Initial Execution:";
pref_wnd2->sc.label_type=FL_ALIGN_CENTER;
pref_wnd2->sc.tooltip="";						//tool tip
pref_wnd2->sc.options="&None,&Step into main(...),&Run";	//menu button drop down options
pref_wnd2->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd2->sc.labelsize=-1;						//-1 means use fltk default
pref_wnd2->sc.textfont=-1;//fl_font();
pref_wnd2->sc.textsize=-1;//fl_size();
pref_wnd2->sc.section="MyPref";
pref_wnd2->sc.key="InitExec";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="0";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&gi0;						//address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb=0;								//address of a callback if any, 0 means none
pref_wnd2->AddControl();

pref_wnd2->CreateRow(25);			//specify optional row height




pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnCheckPref;
pref_wnd2->sc.x=0;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=300;
pref_wnd2->sc.h=20;
pref_wnd2->sc.label="Bring Watch Window to Front at each BreakPoint";
pref_wnd2->sc.label_type=FL_ALIGN_CENTER;
pref_wnd2->sc.tooltip="This is My Tool Tip";						//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd2->sc.labelsize=-1;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=12;
pref_wnd2->sc.section="MyPref";
pref_wnd2->sc.key="WatchToFront";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="0";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&gi1;						//address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb=0;								//address of a callback if any, 0 means none
pref_wnd2->AddControl();

pref_wnd2->CreateRow(25);					//specify optional row height


pref_wnd->ClearToDefCtrl();					//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnCheckPref;
pref_wnd2->sc.x=0;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=300;
pref_wnd2->sc.h=20;
pref_wnd2->sc.label="Prompt to Reload, if Executable Datestamp Changes";
pref_wnd2->sc.label_type=FL_ALIGN_CENTER;
pref_wnd2->sc.tooltip="";						//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd2->sc.labelsize=-1;						//-1 means use fltk default
pref_wnd2->sc.textfont=-1;
pref_wnd2->sc.textsize=-1;
pref_wnd2->sc.section="MyPref";
pref_wnd2->sc.key="DateStampReload";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="0";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&gi2;						//address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb=0;								//address of a callback if any, 0 means none
pref_wnd2->AddControl();

pref_wnd2->CreateRow(25);						//specify optional row height


pref_wnd->ClearToDefCtrl();					//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnInputIntPref;
pref_wnd2->sc.x=190;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=150;
pref_wnd2->sc.h=20;
pref_wnd2->sc.label="Enter a Integer Num:";
pref_wnd2->sc.label_type=FL_ALIGN_CENTER;
pref_wnd2->sc.tooltip="";						//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd2->sc.labelsize=-1;						//-1 means use fltk default
pref_wnd2->sc.textfont=-1;
pref_wnd2->sc.textsize=-1;
pref_wnd2->sc.section="MyPref";
pref_wnd2->sc.key="Integer1";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="0";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&gi3;						//address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb=0;								//address of a callback if any, 0 means none
pref_wnd2->AddControl();

pref_wnd2->CreateRow(25);						//specify optional row height


pref_wnd->ClearToDefCtrl();					//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnInputDoublePref;
pref_wnd2->sc.x=200;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=150;
pref_wnd2->sc.h=20;
pref_wnd2->sc.label="Enter a Floating Point Num:";
pref_wnd2->sc.label_type=FL_ALIGN_CENTER;
pref_wnd2->sc.tooltip="";						//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd2->sc.labelsize=-1;						//-1 means use fltk default
pref_wnd2->sc.textfont=-1;
pref_wnd2->sc.textsize=-1;
pref_wnd2->sc.section="MyPref";
pref_wnd2->sc.key="Float1";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="0";							//default to use if ini value not avail
pref_wnd2->sc.iretval=(int*)-1;					//address of int to be modified, -1 means none
pref_wnd2->sc.dretval=&gd0;						//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb=0;								//address of a callback if any, 0 means none
pref_wnd2->AddControl();

pref_wnd2->CreateRow(25);						//specify optional row height


pref_wnd->ClearToDefCtrl();					//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnInputHexPref;
pref_wnd2->sc.x=190;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=150;
pref_wnd2->sc.h=20;
pref_wnd2->sc.label="Enter a Hex Num:";
pref_wnd2->sc.label_type=FL_ALIGN_CENTER;
pref_wnd2->sc.tooltip="";						//tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd2->sc.labelsize=-1;						//-1 means use fltk default
pref_wnd2->sc.textfont=-1;
pref_wnd2->sc.textsize=-1;
pref_wnd2->sc.section="MyPref";
pref_wnd2->sc.key="Hex1";
pref_wnd2->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd2->sc.def="0";							//default to use if ini value not avail
pref_wnd2->sc.iretval=&gi4;						//address of int to be modified, -1 means none
pref_wnd2->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
pref_wnd2->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb=0;								//address of a callback if any, 0 means none
pref_wnd2->AddControl();

pref_wnd2->CreateRow(25);						//specify optional row height



pref_wnd2->ClearToDefCtrl();					//this will clear sc struct to safe default values

pref_wnd2->sc.type=cnInputPref;
pref_wnd2->sc.x=100;
pref_wnd2->sc.y=0;
pref_wnd2->sc.w=570;
pref_wnd2->sc.h=25;
pref_wnd2->sc.label="FavEditor:";
pref_wnd2->sc.label_type=FL_ALIGN_CENTER;
pref_wnd2->sc.tooltip="Enter Appname of your favorite text editor"; //tool tip
pref_wnd2->sc.options="";						//menu button drop down options
pref_wnd2->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd2->sc.labelsize=-1;						//-1 means use fltk default
pref_wnd2->sc.textfont=4;
pref_wnd2->sc.textsize=12;
pref_wnd2->sc.section="Pref";
pref_wnd2->sc.key="FavEditor";
pref_wnd2->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd2->sc.def="";							//default to use if ini value not avail
pref_wnd2->sc.iretval=(int*)-1;			       	//address of int to be modified, -1 means none
pref_wnd2->sc.sretval=&fav_editor;				//address of string to be modified, -1 means none
pref_wnd2->sc.cb=cb_user;						//address of a callback if any, 0 means none
pref_wnd2->AddControl();

pref_wnd2->CreateRow( 25 );							//specify optional row height





pref_wnd2->End();								//do end for all windows

}










void cb_pref(Fl_Widget*w, void* v)
{
if(pref_wnd) pref_wnd->Show(1);
}





void make_pref_wnd()
{
string s;
sControl sc;

if(pref_wnd==0)
	{
	pref_wnd = new PrefWnd(wndMain->x()+20,wndMain->y()+20,960,350,"Preferences","Settings","PrefWnd1Pos");
	}
else{
	pref_wnd->Show(0);
	return;
	}

//pref_wnd->pck->begin();

/*
pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values


pref_wnd->sc.type=cnStaticTextPref;
pref_wnd->sc.x=0;
pref_wnd->sc.y=0;
pref_wnd->sc.w=60;
pref_wnd->sc.h=20;
pref_wnd->sc.label="IP No:";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="Enter an IP number, e.g. 10.27.25.131";		//tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;					//-1 means use fltk default
pref_wnd->sc.labelsize=-1;					//-1 means use fltk default
pref_wnd->sc.textfont=4;
pref_wnd->sc.textsize=12;
pref_wnd->sc.section="";						//ini file Section
pref_wnd->sc.key="";							//ini file Key
pref_wnd->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd->sc.def="";							//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)-1;					//address of int to be modified, -1 means none
pref_wnd->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd->sc.cb=0;								//address of a callback if any, 0 means none
pref_wnd->AddControl();


pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd->sc.type=cnInputPref;
pref_wnd->sc.x=60;
pref_wnd->sc.y=0;
pref_wnd->sc.w=180;
pref_wnd->sc.h=20;
pref_wnd->sc.label="";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="";						//tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=-1;						//-1 means use fltk default
pref_wnd->sc.textfont=4;
pref_wnd->sc.textsize=12;
pref_wnd->sc.section="Pref";
pref_wnd->sc.key="IP";
pref_wnd->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd->sc.def="0.0.0.0.0";					//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)-1;					//address of int to be modified, -1 means none
pref_wnd->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd->sc.cb=cb_user;						//address of a callback if any, 0 means none
pref_wnd->AddControl();
*/

pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd->sc.type=cnCheckPref;
pref_wnd->sc.x=200;
pref_wnd->sc.y=0;
pref_wnd->sc.w=20;
pref_wnd->sc.h=20;
pref_wnd->sc.label="Taper windowing enable";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="apply a tapering window function to received data blocks, will minimize spectral leakage in fft operations, reduces spectra fluctuations (for display purposes only)";						//tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=10;						//-1 means use fltk default
pref_wnd->sc.textfont=4;
pref_wnd->sc.textsize=12;
pref_wnd->sc.section="Settings";
pref_wnd->sc.key="pref_taper_windowing_for_fft";
pref_wnd->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd->sc.def="1";							//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)&pref_taper_windowing_for_fft;					//address of int to be modified, -1 means none
pref_wnd->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd->sc.cb=cb_user;						//address of a callback if any, 0 means none
pref_wnd->sc.dynamic = 1;
pref_wnd->AddControl();

pref_wnd->CreateRow();							//specify optional row height









pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd->sc.type=cnCheckPref;
pref_wnd->sc.x=200;
pref_wnd->sc.y=0;
pref_wnd->sc.w=20;
pref_wnd->sc.h=20;
pref_wnd->sc.label="Zero padding enable";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="zero stuff received data to increase the number of spectral bins in fft operations (helps improve display of spectra when graphs are zoomed in, will increase cpu usage)";						//tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=10;						//-1 means use fltk default
pref_wnd->sc.textfont=4;
pref_wnd->sc.textsize=12;
pref_wnd->sc.section="Settings";
pref_wnd->sc.key="pref_zero_padding_for_fft";
pref_wnd->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd->sc.def="0";							//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)&pref_zero_padding_for_fft;					//address of int to be modified, -1 means none
pref_wnd->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd->sc.cb=cb_user;						//address of a callback if any, 0 means none
pref_wnd->sc.dynamic = 1;
pref_wnd->AddControl();





pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd->sc.type=cnInputIntPref;
pref_wnd->sc.x=400;
pref_wnd->sc.y=0;
pref_wnd->sc.w=20;
pref_wnd->sc.h=20;
pref_wnd->sc.label="Zero padding block count";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="e.g. if set to 3, will add 3 additional blocks of zeros to each received data block before fft is performed (helps increase fft bin freq resolution on display, higher value will increase cpu usage)";						//tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=10;						//-1 means use fltk default
pref_wnd->sc.textfont=4;
pref_wnd->sc.textsize=9;
pref_wnd->sc.section="Settings";
pref_wnd->sc.key="pref_zero_padding_for_fft_cnt";
pref_wnd->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd->sc.def="1";							//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)&pref_zero_padding_for_fft_cnt;					//address of int to be modified, -1 means none
pref_wnd->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd->sc.cb=cb_user;						//address of a callback if any, 0 means none
pref_wnd->sc.dynamic = 1;
pref_wnd->AddControl();

pref_wnd->CreateRow();							//specify optional row height




pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd->sc.type=cnCheckPref;
pref_wnd->sc.x=200;
pref_wnd->sc.y=0;
pref_wnd->sc.w=20;
pref_wnd->sc.h=20;
pref_wnd->sc.label="Waterfall non linear gain";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="helps bring up detail in the noise by increasing gain of low level signals";						//tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=10;						//-1 means use fltk default
pref_wnd->sc.textfont=4;
pref_wnd->sc.textsize=12;
pref_wnd->sc.section="Settings";
pref_wnd->sc.key="pref_wfall_non_linear_gain";
pref_wnd->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd->sc.def="1";							//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)&pref_wfall_non_linear_gain;					//address of int to be modified, -1 means none
pref_wnd->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd->sc.cb=cb_user;						//address of a callback if any, 0 means none
pref_wnd->sc.dynamic = 1;
pref_wnd->AddControl();

pref_wnd->CreateRow();							//specify optional row height




/*

pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd->sc.type=cnCheckPref;
pref_wnd->sc.x=200;
pref_wnd->sc.y=0;
pref_wnd->sc.w=20;
pref_wnd->sc.h=20;
pref_wnd->sc.label="show graph onboard tuner freq";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="show onboard tuner freq at center of graph";						//tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=10;						//-1 means use fltk default
pref_wnd->sc.textfont=4;
pref_wnd->sc.textsize=12;
pref_wnd->sc.section="Settings";
pref_wnd->sc.key="pref_show_graph_onboard_tuner_freq";
pref_wnd->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd->sc.def="1";							//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)&pref_show_graph_onboard_tuner_freq;					//address of int to be modified, -1 means none
pref_wnd->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd->sc.cb=cb_user;						//address of a callback if any, 0 means none
pref_wnd->sc.dynamic = 1;
pref_wnd->AddControl();

pref_wnd->CreateRow();							//specify optional row height
*/





pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd->sc.type=cnCheckPref;
pref_wnd->sc.x=200;
pref_wnd->sc.y=0;
pref_wnd->sc.w=20;
pref_wnd->sc.h=20;
pref_wnd->sc.label="show graph freq in Hz";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="show freq in units of Hz on graph, e.g: 12340000 Hz";						//tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=10;						//-1 means use fltk default
pref_wnd->sc.textfont=4;
pref_wnd->sc.textsize=12;
pref_wnd->sc.section="Settings";
pref_wnd->sc.key="pref_show_graph_hz_units";
pref_wnd->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd->sc.def="1";							//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)&pref_show_graph_hz_units;					//address of int to be modified, -1 means none
pref_wnd->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd->sc.cb=cb_user;						//address of a callback if any, 0 means none
pref_wnd->sc.dynamic = 1;
pref_wnd->AddControl();

pref_wnd->CreateRow();							//specify optional row height







pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd->sc.type=cnCheckPref;
pref_wnd->sc.x=200;
pref_wnd->sc.y=0;
pref_wnd->sc.w=20;
pref_wnd->sc.h=20;
pref_wnd->sc.label="show graph freq Hz in eng format";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="show freq Hz in engineering format, e.g: 1.234 MHz";						//tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=10;						//-1 means use fltk default
pref_wnd->sc.textfont=4;
pref_wnd->sc.textsize=12;
pref_wnd->sc.section="Settings";
pref_wnd->sc.key="pref_show_graph_hz_eng";
pref_wnd->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd->sc.def="1";							//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)&pref_show_graph_hz_eng;					//address of int to be modified, -1 means none
pref_wnd->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd->sc.cb=cb_user;						//address of a callback if any, 0 means none
pref_wnd->sc.dynamic = 1;
pref_wnd->AddControl();

pref_wnd->CreateRow();							//specify optional row height









pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd->sc.type=cnInputDoublePref;
pref_wnd->sc.x=250;
pref_wnd->sc.y=0;
pref_wnd->sc.w=35;
pref_wnd->sc.h=18;
pref_wnd->sc.label="dB ref values for selected sample's y val,       A:";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="enter dB ref values to use, e.g: A=20, B=1, C=0 will show 6.02 dB when select sample's y value is 2.0:\n\nA * log10( y / B ) + C";						//tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=10;						//-1 means use fltk default
pref_wnd->sc.textfont=4;
pref_wnd->sc.textsize=9;
pref_wnd->sc.section="Settings";
pref_wnd->sc.key="pref_db_ref_A";
pref_wnd->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd->sc.def="20.0";							//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)-1;					//address of int to be modified, -1 means none
pref_wnd->sc.dretval=(double*)&pref_db_ref_A;		//address of double to be modified, -1 means none
pref_wnd->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd->sc.cb=cb_user;						//address of a callback if any, 0 means none
pref_wnd->sc.dynamic = 1;
pref_wnd->AddControl();




pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd->sc.type=cnInputDoublePref;
pref_wnd->sc.x=350;
pref_wnd->sc.y=0;
pref_wnd->sc.w=35;
pref_wnd->sc.h=18;
pref_wnd->sc.label="B:";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="enter dB ref values to use, e.g: A=20, B=1, C=0 will show 6.02 dB when select sample's y value is 2.0:\n\nA * log10( y / B ) + C";						//tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=10;						//-1 means use fltk default
pref_wnd->sc.textfont=4;
pref_wnd->sc.textsize=9;
pref_wnd->sc.section="Settings";
pref_wnd->sc.key="pref_db_ref_B";
pref_wnd->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd->sc.def="1.0";							//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)-1;					//address of int to be modified, -1 means none
pref_wnd->sc.dretval=(double*)&pref_db_ref_B;		//address of double to be modified, -1 means none
pref_wnd->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd->sc.cb=cb_user;						//address of a callback if any, 0 means none
pref_wnd->sc.dynamic = 1;
pref_wnd->AddControl();






pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd->sc.type=cnInputDoublePref;
pref_wnd->sc.x=450;
pref_wnd->sc.y=0;
pref_wnd->sc.w=35;
pref_wnd->sc.h=18;
pref_wnd->sc.label="C:";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="enter dB ref values to use, e.g: A=20, B=1, C=0 will show 6.02 dB when select sample's y value is 2.0:\n\nA * log10( y / B ) + C";						//tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=10;						//-1 means use fltk default
pref_wnd->sc.textfont=4;
pref_wnd->sc.textsize=9;
pref_wnd->sc.section="Settings";
pref_wnd->sc.key="pref_db_ref_C";
pref_wnd->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd->sc.def="0.0";							//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)-1;					//address of int to be modified, -1 means none
pref_wnd->sc.dretval=(double*)&pref_db_ref_C;		//address of double to be modified, -1 means none
pref_wnd->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd->sc.cb=cb_user;						//address of a callback if any, 0 means none
pref_wnd->sc.dynamic = 1;
pref_wnd->AddControl();





pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd->sc.type=cnInputPref;
pref_wnd->sc.x=600;
pref_wnd->sc.y=0;
pref_wnd->sc.w=35;
pref_wnd->sc.h=18;
pref_wnd->sc.label="dB suffix text";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="enter dB suffix text, e.g. dB, dBFS";						//tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=10;						//-1 means use fltk default
pref_wnd->sc.textfont=4;
pref_wnd->sc.textsize=9;
pref_wnd->sc.section="Settings";
pref_wnd->sc.key="pref_db_ref_suffix";
pref_wnd->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd->sc.def="dB";							//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)-1;					//address of int to be modified, -1 means none
pref_wnd->sc.dretval=(double*)-1;			//address of double to be modified, -1 means none
pref_wnd->sc.sretval=(string*)&pref_db_ref_suffix;				//address of string to be modified, -1 means none
pref_wnd->sc.cb=cb_user;						//address of a callback if any, 0 means none
pref_wnd->sc.dynamic = 1;
pref_wnd->AddControl();






pref_wnd->CreateRow();							//specify optional row height






pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd->sc.type=cnInputIntPref;
pref_wnd->sc.x=250;
pref_wnd->sc.y=0;
pref_wnd->sc.w=35;
pref_wnd->sc.h=18;
pref_wnd->sc.label="Freq digit zeroing, right drag - onboard tuner";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="quantizes right click drag freq tweeks when over inbedded graph (onboard tuner's freq).\nE.g: '2' would zero last two digit, so dragging would adjust in 100Hz steps";						//tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=10;						//-1 means use fltk default
pref_wnd->sc.textfont=4;
pref_wnd->sc.textsize=9;
pref_wnd->sc.section="Settings";
pref_wnd->sc.key="pref_freq_right_click_drag_low_digit_zero_cnt";
pref_wnd->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd->sc.def="3";							//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)&pref_freq_right_click_drag_low_digit_zero_cnt;					//address of int to be modified, -1 means none
pref_wnd->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd->sc.cb=cb_user;						//address of a callback if any, 0 means none
pref_wnd->sc.dynamic = 1;
pref_wnd->AddControl();

pref_wnd->CreateRow();							//specify optional row height






pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd->sc.type=cnInputIntPref;
pref_wnd->sc.x=250;
pref_wnd->sc.y=0;
pref_wnd->sc.w=35;
pref_wnd->sc.h=18;
pref_wnd->sc.label="Freq digit zeroing, left drag - sub tuner";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="quantizes left click drag freq tweeks when over inbedded graph (sub tuner's freq).\nE.g: '2' would zero last two digit, so dragging would adjust in 100Hz steps";						//tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=10;						//-1 means use fltk default
pref_wnd->sc.textfont=4;
pref_wnd->sc.textsize=9;
pref_wnd->sc.section="Settings";
pref_wnd->sc.key="pref_freq_left_click_drag_low_digit_zero_cnt";
pref_wnd->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd->sc.def="2";							//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)&pref_freq_left_click_drag_low_digit_zero_cnt;					//address of int to be modified, -1 means none
pref_wnd->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd->sc.cb=cb_user;						//address of a callback if any, 0 means none
pref_wnd->sc.dynamic = 1;
pref_wnd->AddControl();

pref_wnd->CreateRow();							//specify optional row height

						



pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd->sc.type=cnInputIntPref;
pref_wnd->sc.x=250;
pref_wnd->sc.y=0;
pref_wnd->sc.w=35;
pref_wnd->sc.h=18;
pref_wnd->sc.label="Freq tune digit zeroing, mousewheel - sub tuner";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="quantizes mousewheel freq tweeks when over inbedded graph.\nE.g: '2' would zero last two digit, so mousewheel would adjust in 100Hz steps,\nthis quantization is further modified by where mouse sits on graph,\ntop of graph gives 1Hz tweeks,\nmiddle and bottom of graph give courser tweeks)";						//tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=10;						//-1 means use fltk default
pref_wnd->sc.textfont=4;
pref_wnd->sc.textsize=9;
pref_wnd->sc.section="Settings";
pref_wnd->sc.key="pref_freq_mousewheel_low_digit_zero_cnt";
pref_wnd->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd->sc.def="1";							//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)&pref_freq_mousewheel_low_digit_zero_cnt;					//address of int to be modified, -1 means none
pref_wnd->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd->sc.cb=cb_user;						//address of a callback if any, 0 means none
pref_wnd->sc.dynamic = 1;
pref_wnd->AddControl();

pref_wnd->CreateRow();							//specify optional row height




						
						
						
						
						
pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd->sc.type=cnCheckPref;
pref_wnd->sc.x=200;
pref_wnd->sc.y=0;
pref_wnd->sc.w=35;
pref_wnd->sc.h=18;
pref_wnd->sc.label="Show dc bin when plotting fft graphs";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="show dc bin when plotting fft graphs, if unticked the dc bin is forced to zero";							//tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=10;						//-1 means use fltk default
pref_wnd->sc.textfont=4;
pref_wnd->sc.textsize=9;
pref_wnd->sc.section="Settings";
pref_wnd->sc.key="pref_show_dc_for_fft_graphs";
pref_wnd->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd->sc.def="0";							//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)&pref_show_dc_for_fft_graphs;					//address of int to be modified, -1 means none
pref_wnd->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd->sc.cb=cb_user;						//address of a callback if any, 0 means none
pref_wnd->sc.dynamic = 1;
pref_wnd->AddControl();

pref_wnd->CreateRow();							//specify optional row height
						


					
pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd->sc.type=cnCheckPref;
pref_wnd->sc.x=200;
pref_wnd->sc.y=0;
pref_wnd->sc.w=35;
pref_wnd->sc.h=18;
pref_wnd->sc.label="Ask preset overwrite";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="ask if you want to overwrite a preset when right clicking it";							//tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=10;						//-1 means use fltk default
pref_wnd->sc.textfont=4;
pref_wnd->sc.textsize=9;
pref_wnd->sc.section="Settings";
pref_wnd->sc.key="pref_ask_preset_overwrite";
pref_wnd->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd->sc.def="1";							//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)&pref_ask_preset_overwrite;					//address of int to be modified, -1 means none
pref_wnd->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd->sc.cb=cb_user;						//address of a callback if any, 0 means none
pref_wnd->sc.dynamic = 1;
pref_wnd->AddControl();

pref_wnd->CreateRow();							//specify optional row height
						
				
				
				

pref_wnd->sc.type=cnGCColColour;
pref_wnd->sc.x=180;
pref_wnd->sc.y=2;
pref_wnd->sc.w=84;
pref_wnd->sc.h=20;
pref_wnd->sc.label="inset graph trace colour";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="set r,g,b colour value"; //tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=10;						//-1 means use fltk default
pref_wnd->sc.textfont=4;
pref_wnd->sc.textsize=9;
pref_wnd->sc.section="Settings";
pref_wnd->sc.key="pref_scol_gph0_trace_col";
pref_wnd->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd->sc.def="224,255,212";						//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)-1;			       	//address of int to be modified, -1 means none
pref_wnd->sc.sretval=&pref_scol_gph0_trace_col;				//address of string to be modified, -1 means none
pref_wnd->sc.cb = cb_user;						//address of a callback if any, 0 means none
pref_wnd->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd->sc.uniq_id = cn_pref_scol_gph0_trace_col;                   //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd->AddControl();

pref_wnd->CreateRow( 25 );					//specify optional row height


		


pref_wnd->sc.type=cnGCColColour;
pref_wnd->sc.x=180;
pref_wnd->sc.y=2;
pref_wnd->sc.w=84;
pref_wnd->sc.h=20;
pref_wnd->sc.label="x-axis freq scale text colour";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="set r,g,b colour value"; //tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=10;						//-1 means use fltk default
pref_wnd->sc.textfont=4;
pref_wnd->sc.textsize=9;
pref_wnd->sc.section="Settings";
pref_wnd->sc.key="pref_scol_xaxis_text_freq";
pref_wnd->sc.keypostfix=-1;					//ini file Key post fix
pref_wnd->sc.def="80,170,255";						//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)-1;			       	//address of int to be modified, -1 means none
pref_wnd->sc.sretval=&pref_scol_xaxis_text_freq;				//address of string to be modified, -1 means none
pref_wnd->sc.cb = cb_user;						//address of a callback if any, 0 means none
pref_wnd->sc.dynamic = 1;						//allow immediate dynamic change of user var
pref_wnd->sc.uniq_id = cn_pref_scol_xaxis_text_freq;                   //allows identification of an actual control, rather that using its row and column values, don't use 0xffffffff
pref_wnd->AddControl();

pref_wnd->CreateRow( 25 );					//specify optional row height





/*
pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values


pref_wnd->sc.type=cnRoundButtonPref;
pref_wnd->sc.x=310;
pref_wnd->sc.y=0;
pref_wnd->sc.w=60;
pref_wnd->sc.h=20;
pref_wnd->sc.label="Cycle";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="";						//tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=-1;						//-1 means use fltk default
pref_wnd->sc.textfont=4;
pref_wnd->sc.textsize=12;
pref_wnd->sc.section="Pref";
pref_wnd->sc.key="Round";
pref_wnd->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd->sc.def="0";							//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)-1;					//address of int to be modified, -1 means none
pref_wnd->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd->sc.cb=cb_user;						//address of a callback if any, 0 means none
pref_wnd->AddControl();


pref_wnd->sc.type=cnMenuChoicePref;
pref_wnd->sc.x=550;
pref_wnd->sc.y=0;
pref_wnd->sc.w=150;
pref_wnd->sc.h=20;
pref_wnd->sc.label="Initial Execution:";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="";						//tool tip
pref_wnd->sc.options="&None,&Step into main(...),&Run";	//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=-1;						//-1 means use fltk default
pref_wnd->sc.textfont=-1;
pref_wnd->sc.textsize=-1;
pref_wnd->sc.section="Pref";
pref_wnd->sc.key="MenuCh";
pref_wnd->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd->sc.def="0";							//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)-1;					//address of int to be modified, -1 means none
pref_wnd->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd->sc.cb=cb_user;						//address of a callback if any, 0 means none
pref_wnd->AddControl();


pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd->sc.type=cnButtonPref;
pref_wnd->sc.x=770;
pref_wnd->sc.y=1;
pref_wnd->sc.w=45;
pref_wnd->sc.h=17;
pref_wnd->sc.label="Ping";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="";						//tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=12;						//-1 means use fltk default
pref_wnd->sc.textfont=-1;
pref_wnd->sc.textsize=-1;
pref_wnd->sc.section="";
pref_wnd->sc.key="";
pref_wnd->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd->sc.def="0";							//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)-1;					//address of int to be modified, -1 means none
pref_wnd->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd->sc.cb=cb_user;						//address of a callback if any, 0 means none
pref_wnd->AddControl();


pref_wnd->ClearToDefCtrl();			//this will clear sc struct to safe default values

pref_wnd->sc.type=cnButtonPref;
pref_wnd->sc.x=820;
pref_wnd->sc.y=1;
pref_wnd->sc.w=45;
pref_wnd->sc.h=17;
pref_wnd->sc.label="Ping2";
pref_wnd->sc.label_type = FL_NORMAL_LABEL;      //label effects such a FL_EMBOSSED_LABEL, FL_ENGRAVED_LABEL, FL_SHADOW_LABEL
pref_wnd->sc.label_align = FL_ALIGN_LEFT;
pref_wnd->sc.tooltip="";						//tool tip
pref_wnd->sc.options="";						//menu button drop down options
pref_wnd->sc.labelfont=-1;						//-1 means use fltk default
pref_wnd->sc.labelsize=12;						//-1 means use fltk default
pref_wnd->sc.textfont=-1;
pref_wnd->sc.textsize=-1;
pref_wnd->sc.section="";
pref_wnd->sc.key="";
pref_wnd->sc.keypostfix=-1;						//ini file Key post fix
pref_wnd->sc.def="0";							//default to use if ini value not avail
pref_wnd->sc.iretval=(int*)-1;					//address of int to be modified, -1 means none
pref_wnd->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
pref_wnd->sc.cb=cb_user;						//address of a callback if any, 0 means none
pref_wnd->AddControl();

pref_wnd->CreateRow();							//specify optional row height
*/

pref_wnd->End();						//do end for all windows

}














//----------------------------------------------------------------------------------

//this is the callback that is called by buttons that specified is in - 
//definition ....pref_wnd->sc.cb=(void*)cb_user;
//'*o' is the PrefWnd* ptr 
//'row' is the row the control lives on, 0 is first row
//'ctrl' is the number of the controlon that row, 0 is first control
void cb_user2(void *o, int row,int ctrl)
{


printf("\nPing by Control on Row=%d at control count Ctrl=%d on this row.\n",row,ctrl);
}




void cb_font_pref(Fl_Widget*w, void* v)
{
if(font_pref_wnd)
	{
	font_pref_wnd->hide();
	font_pref_wnd->Show(0);
	}
}








void make_font_pref_wnd()
{
sControl sc;


if( font_pref_wnd == 0 )
	{
	font_pref_wnd = new PrefWnd(wndMain->x()+20,wndMain->y()+20,400,115,"Font Preference","Settings","FontPrefWndPos");
	}
else{
	font_pref_wnd->Show(0);
	return;
	}

PrefWnd* o=font_pref_wnd;



string fnames;
string fsizes;



int maxfont = Fl::set_fonts("*");
string s;

for (int i = 0; i < maxfont; i++)
	{
    int t;
    const char *name = Fl::get_font_name((Fl_Font)i,&t);
//    printf("%d: %s\n",i,name);
    strpf(s,"%02d: %s",i,name);
    fnames+=s;
    fnames+=",";
	}


for (int i = 0; i <= 72; i++)
	{
	string s;
	
	strpf(s,"%d",i);
	fsizes+=s;
	fsizes+=",";
	}






// -- dont need to do the below manual default load as "ClearToDefCtrl()" does this for you --

o->ClearToDefCtrl();			//this will clear sc struct to safe default values
o->AddControl();
o->CreateRow(10);				//specify optional row height


o->ClearToDefCtrl();			//this will clear sc struct to safe default values

o->sc.type=cnMenuChoicePref;
o->sc.x=105;
o->sc.y=0;
o->sc.w=250;
o->sc.h=25;
o->sc.label="Font Type:";
o->sc.label_type=FL_ALIGN_CENTER;
o->sc.tooltip="";						//tool tip
o->sc.options=fnames;	//menu button drop down options
o->sc.labelfont=-1;						//-1 means use fltk default
o->sc.labelsize=-1;						//-1 means use fltk default
o->sc.textfont=-1;
o->sc.textsize=-1;
o->sc.section="Settings";
o->sc.key="FontTypeEditor";
o->sc.keypostfix=-1;					//ini file Key post fix
o->sc.def="0";							//default to use if ini value not avail
o->sc.iretval=&font_num;				//address of int to be modified, -1 means none
o->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
o->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
o->sc.cb=cb_user2;						//address of a callback if any, 0 means none
o->sc.dynamic=1;						//allow immediate dynamic change of user var
o->AddControl();

o->CreateRow(30);						//specify optional row height

o->ClearToDefCtrl();					//this will clear sc struct to safe default values

o->sc.type=cnMenuChoicePref;
o->sc.x=105;
o->sc.y=0;
o->sc.w=50;
o->sc.h=25;
o->sc.label="Font Size";
o->sc.label_type=FL_ALIGN_CENTER;
o->sc.tooltip="This is My Tool Tip";	//tool tip
o->sc.options=fsizes;					//menu button drop down options
o->sc.labelfont=-1;						//-1 means use fltk default
o->sc.labelsize=-1;						//-1 means use fltk default
o->sc.textfont=-1;
o->sc.textsize=-1;
o->sc.section="Settings";
o->sc.key="FontSizeEditor";
o->sc.keypostfix=-1;					//ini file Key post fix
o->sc.def="12";							//default to use if ini value not avail
o->sc.iretval=&font_size;			//address of int to be modified, -1 means none
o->sc.dretval=(double*)-1;				//address of double to be modified, -1 means none
o->sc.sretval=(string*)-1;				//address of string to be modified, -1 means none
o->sc.dynamic=1;						//allow immediate dynamic change of user var
o->sc.cb=cb_user2;						//address of a callback if any, 0 means none
o->AddControl();

o->CreateRow(30);						//specify optional row height



o->End();								//do end for all windows

}








//----------------------------------------------------------------------------------












void cb_btTest2(Fl_Widget *, void *)
{
cb_pref2(0,0);
}





void cb_btTest(Fl_Widget *, void *)
{
cb_pref(0,0);
}




void cb_bt_clear(Fl_Widget *, void *)
{
clear_csl();
}










void cb_bt_thread_start(Fl_Widget *, void *)
{
string s1;

if ( sThrd1.iThreadFinished == 0 )
	{
	strpf(s1, "\nThread1 still running...\n" );
	printf( "%s", s1.c_str() );
	outcsl( s1);
	return;
	}

sglobal = "Hello";
use_mutex = ckMutex->value();
cb_bt_clear( 0, 0 );

sThrd1.iKillThread = 0;

//linux code
//start thread
#ifndef compile_for_windows
pthread_create ( &thread1_id, NULL, &Thread1, (void*) &sglobal );		//start thread
#endif



//windows code
//start thread
#ifdef compile_for_windows
CreateThread(NULL, 0, 		// default values
&DosThread, 				// function to use
0, 							// data to pass to the function
0, NULL); 					//Start thread, we don't need a thread ID
#endif

}








void cb_bt_thread_stop(Fl_Widget *, void *)
{
string s1;

if ( sThrd1.iThreadFinished == 1 )
	{
	strpf(s1, "\nThread1 not running...\n" );
	printf( "%s", s1.c_str() );
	outcsl( s1);

	}

sThrd1.iKillThread = 1;

}



void cb_ck_mutex(Fl_Widget *, void *)
{
sThrd1.iKillThread = 1;
}








void cb_btRunAnyway(Fl_Widget *w, void* v)
{
Fl_Widget* wnd=(Fl_Widget*)w;
Fl_Window *win;
win=(Fl_Window*)wnd->parent();

iAppExistDontRun = 0;
win->~Fl_Window();
}






void cb_btDontRunExit(Fl_Widget* w, void* v)
{
Fl_Widget* wnd=(Fl_Widget*)w;
Fl_Window *win;
win=(Fl_Window*)wnd->parent();

win->~Fl_Window();
}









//linux code
/*
 
#ifndef compile_for_windows 
//gets its ID, -- fixed memory leak using XFetchName (used XFree) 01-10-10
int FindWindowID(string csName,Display *display,Window &wid)
{
Window root, parent, *children;
unsigned int numWindows = 0;
int iRet=0;

//*display = XOpenDisplay(NULL);
//gDisp = XOpenDisplay(NULL);

//if(cnShowFindResults) printf("\nDispIn %x\n",display);

XQueryTree(gDisp, RootWindow(gDisp,0), &root, &parent, &children, &numWindows);

int i = 0;
for(i=0; i < numWindows ; i++)
	{
//	char *name;
	Window root2, parent2, *children2;
//	XFetchName(display, children[i], &name);

	
	unsigned int numWindows2 = 0;

//	if(cnShowFindResults) if(name) printf("Window name: %s\n", name);

	XQueryTree(display, children[i], &root2, &parent2, &children2, &numWindows2);
	for(int j=0; j < numWindows2 ; j++)
		{
		char *name;
		XFetchName(display, children2[j], &name);

		
//		unsigned int numWindows2 = 0;
//		Window root2, parent2, *children2;
//		XQueryTree(display, RootWindow(display,0), children[i], &parent2, &children2, &numWindows2);
		 
		if(name) 
			{
//		if(cnShowFindResults) printf("    Window2 name: %s  Id=%x\n", name2,children2[j]);

			if(strcmp(csName.c_str(),name)==0)
				{
//				if(cnShowFindResults) printf("Found It................\n");
//				XMoveWindow(display, children2[j], -100, -100);
//				XMoveWindow(display, children2[j], -100, -100);
//				XMoveWindow(display, children2[j], -100, -100);
//				XResizeWindow(display, children2[j], 1088, 612+22);
//				XMoveWindow(*display, children2[j], 1100, 22);
				wid=children2[j];
//				gw=children2[j];
				iRet=1;
//				return 0;
//				if(iRet)
//					{
//					printf("\n\nTrying to Move %x  %x\n\n",gDisp, gw);
//					XMoveWindow(gDisp, gw, 700, 22);
//					return 1;
//					}
				}
			XFree(name);
			}
		}
	 if(children2) XFree(children2);
	}

if(children) XFree(children);
return  iRet;
}

#endif




void BringWindowToFront(string csTitle)
{

//linux code
#ifndef compile_for_windows
Window wid;
if(FindWindowID(csTitle,gDisp,wid))
	{
	XUnmapWindow(gDisp, wid);
	XMapWindow(gDisp, wid);
	XFlush(gDisp);
	}
#endif


//windows code
#ifdef compile_for_windows
HWND hWnd;
//csAppName.LoadString(IDS_APPNAME);

hWnd = FindWindow( 0, cnsAppName );

if( hWnd )
	{
	::BringWindowToTop( hWnd );
//	::SetForegroundWindow( hWnd );
//	::PostMessage(hWnd,WM_MAXIMIZE,0,0);
	::ShowWindow( hWnd, SW_RESTORE );
	}
#endif

}
*/






//linux code
#ifndef compile_for_windows 


/*
//test if window with csAppName already exists, if so create inital main window with
//two options to either run app, or to exit.
//if no window with same name exists returns 0
//if 'exit' option chosen, exit(0) is called and no return happens
//if 'run anyway' option is chosen, returns 1
int CheckInstanceExists(string csAppName)
{
string csTmp;

gDisp = XOpenDisplay(NULL);

Window wid;


if(FindWindowID(csAppName,gDisp,wid))		//a window with same name exists?
	{
	BringWindowToFront( csAppName );

	XCloseDisplay(gDisp);		//added this to see if valgrind showed improvement - it didn't

	Fl_Window *wndInitial = new Fl_Window(50,50,330,90);
	wndInitial->label("Possible Instance Already Running");
	
	Fl_Box *bxHeading = new Fl_Box(10,10,200, 15, "Another Window With Same Name Was Found.");
	bxHeading->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	strpf(csTmp,"App Name: '%s'",csAppName.c_str()); 
	Fl_Box *bxAppName = new Fl_Box(10,30,200, 15,csTmp.c_str());
	bxAppName->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	Fl_Button *btRunAnyway = new Fl_Button(25,55,130,25,"Run App Anyway");
	btRunAnyway->labelsize(12);
	btRunAnyway->callback(cb_btRunAnyway,0);

	Fl_Button *btDontRunExit = new Fl_Button(btRunAnyway->x()+btRunAnyway->w()+15,55,130,25,"Don't Run App, Exit");
	btDontRunExit->labelsize(12);
	btDontRunExit->callback(cb_btDontRunExit,0);

	wndInitial->end();
	wndInitial->show();
	
	Fl::run();

	return iAppExistDontRun;
	}
else iAppExistDontRun=0;

XCloseDisplay(gDisp);		//added this to see if valgrind showed improvement - it didn't

return iAppExistDontRun;

}

*/






//------------------------------
//returns 1 if file was able to be locked
//returns 0 if file was already locked (possibly by another running instance)

//NOTE you should run this periodically incase the file lock was held by a previous running instance, when that instance closes, this app will gain file lock
//NOTE is called by 'check_instance_exists()' on startup AND periodically by 'cb_timer1()'
int check_instance_try_lock( int fd )
{
if( flock(fd, LOCK_EX | LOCK_NB) == 0) return 1;
return 0;
}





//linux instance checking code which does not req X11
//uses an opened file (/tmp/s_app_name.lock") and 'flock()' to determine if an instance is running,
//returns 0, if no other instance seems to be running i.e: if a '/tmp/s_app_name.lock' file is not locked by another instance (will return 0 if a lock file problem was detected)
//if 'Don't Run App, Exit' option chosen by user in dlg, exit(0) is called and no return happens
//returns 1, if another instance was running and user chose 'run anyway' option
//NOTE requirement for 'cb_timer1()' to call 'check_instance_try_lock()' periodically, refer 'check_instance_try_lock()'
int check_instance_exists(string s_app_name)
{
bool vb = 1;

string s1, csTmp;

strpf( s1, "/tmp/%s.lock", s_app_name.c_str() );						//build a lock file fname

if(vb)printf( "check_instance_exists() - check if file locked: '%s'\n", s_app_name.c_str() ); 


check_instance_fd = open( s1.c_str() , O_CREAT | O_RDWR, 0666);
if ( check_instance_fd < 0 )
	{
	if(vb)printf( "check_instance_exists() - failed to open/create file '%s'\n", s_app_name.c_str() );
	iAppExistDontRun = 0;							//allow user to create another instance as file lock creation failed for some reason
	return iAppExistDontRun;											
	}

if( check_instance_try_lock( check_instance_fd ) )
	{
	if(vb)printf("check_instance_exists() - no other instance found\n");
	iAppExistDontRun = 0;
	return iAppExistDontRun;		
	}
else{
	if(vb)printf("check_instance_exists() - another instance found\n");
	
	Fl_Window *wndInitial = new Fl_Window( 50, 50, 380, 90 );
	wndInitial->label("Possible instance already running");
	
	strpf( s1, "There aappears to be another instance of this app.\nDetermined via locked file: '%s'", s1.c_str() );
	Fl_Box *bxHeading = new Fl_Box(10,20,200, 15, s1.c_str() );
	bxHeading->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

//	strpf(csTmp,"App Name: '%s'",s_app_name.c_str()); 
//	Fl_Box *bxAppName = new Fl_Box(10,30,200, 15,csTmp.c_str());
//	bxAppName->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	Fl_Button *btRunAnyway = new Fl_Button(95,55,130,25,"Run App Anyway");
	btRunAnyway->labelsize(12);
	btRunAnyway->callback(cb_btRunAnyway,0);

	Fl_Button *btDontRunExit = new Fl_Button(btRunAnyway->x()+btRunAnyway->w()+15,55,130,25,"Don't Run App, Exit");
	btDontRunExit->labelsize(12);
	btDontRunExit->callback(cb_btDontRunExit,0);

	wndInitial->end();
	wndInitial->show();
	int iAppRet=Fl::run();
	}
	
return iAppExistDontRun;
}
//-----------------------------

#endif









//windows code
#ifdef compile_for_windows 

//test if window with csAppName already exists, if so create inital main window with
//two options to either run app, or to exit.
//if no window with same name exists returns 0
//if 'exit' option chosen, exit(0) is called and no return happens
//if 'run anyway' option is chosen, returns 1
int CheckInstanceExists( string csAppName )
{
string csTmp;

HWND hWnd;
//csAppName.LoadString(IDS_APPNAME);

hWnd = FindWindow( 0, csAppName.c_str() );

if( hWnd )
	{
	BringWindowToFront( csAppName );
//	::BringWindowToTop( hWnd );
//::SetForegroundWindow( hWnd );
//::PostMessage(hWnd,WM_MAXIMIZE,0,0);
//	::ShowWindow( hWnd, SW_RESTORE );
Sleep(1000);

	Fl_Window *wndInitial = new Fl_Window(50,50,330,90);
	wndInitial->label("Possible Instance Already Running");
	
	Fl_Box *bxHeading = new Fl_Box(10,10,200, 15, "Another Window With Same Name Was Found.");
	bxHeading->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	strpf(csTmp,"App Name: '%s'",csAppName.c_str()); 
	Fl_Box *bxAppName = new Fl_Box(10,30,200, 15,csTmp.c_str());
	bxAppName->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

	Fl_Button *btRunAnyway = new Fl_Button(25,55,130,25,"Run App Anyway");
	btRunAnyway->labelsize(12);
	btRunAnyway->callback(cb_btRunAnyway,0);

	Fl_Button *btDontRunExit = new Fl_Button(btRunAnyway->x()+btRunAnyway->w()+15,55,130,25,"Don't Run App, Exit");
	btDontRunExit->labelsize(12);
	btDontRunExit->callback(cb_btDontRunExit,0);

	wndInitial->end();
	wndInitial->show();
	wndInitial->hide();
	wndInitial->show();
	Fl::run();

	return iAppExistDontRun;
	}
else iAppExistDontRun = 0;

return iAppExistDontRun;
}

#endif















/*
//find this apps path and prefix it to supplied filename
void MakeIniPathFilename(string csFilename,string &csPathFilename)
{

//linux code
#ifndef compile_for_windows

//get the actual path this app lives in
#define MAXPATHLEN 1025   // make this larger if you need to

int length;
char fullpath[MAXPATHLEN];

// /proc/self is a symbolic link to the process-ID subdir
// of /proc, e.g. /proc/4323 when the pid of the process
// of this program is 4323.
//
// Inside /proc/<pid> there is a symbolic link to the
// executable that is running as this <pid>.  This symbolic
// link is called "exe".
//
// So if we read the path where the symlink /proc/self/exe
// points to we have the full path of the executable.


length = readlink("/proc/self/exe", fullpath, sizeof(fullpath));
 
// Catch some errors:
if (length < 0)
	{
 
	syslog(LOG_ALERT,"Error resolving symlink /proc/self/exe.\n");
	fprintf(stderr, "Error resolving symlink /proc/self/exe.\n");
	exit(0);
	}

if (length >= MAXPATHLEN)
	{
	syslog(LOG_ALERT, "Path too long. Truncated.\n");
	fprintf(stderr, "Path too long. Truncated.\n");
	exit(0);
	}

//I don't know why, but the string this readlink() function 
//returns is appended with a '@'

fullpath[length] = '\0';      // Strip '@' off the end


//printf("Full path is: %s\n", fullpath);
//syslog(LOG_ALERT,"Full path is: %s\n", fullpath);

string csTmp;

csTmp=fullpath;
size_t found=csTmp.rfind("/");
if (found!=string::npos) csPathFilename=csTmp.substr(0,found);
//syslog(LOG_ALERT,"Path only is: %s\n", csPathFilename.c_str());

csPathFilename+='/';
csPathFilename+=csFilename;	
#endif



//windows code
#ifdef compile_for_windows 
csTmp = GetCommandLine();

size_t found = csTmp.rfind( dir_seperator );
if ( found != string::npos ) csPathFilename  =csTmp.substr( 0, found );

csPathFilename += dir_seperator;
csPathFilename+=csFilename;	


csPathFilename = csFilename;
#endif

}

*/






//extract command line details from windud

//from GCCmdLine::GetAppName()
//eg: prog.com											//no path
//eg. "c:\dos\edit prog.com"							//with path\prog in quotes
//eg. "c:\dos\edit prog.com" c:\dos\junk.txt			//with path\prog in quotes and path\file
//eg. c\dos\edit.com /p /c		(as in screen-savers)	//path\prog and params no quotes

void get_cmd_line( string cmdline, string &path, string &appname, vector<string> &vparams )
{
string stmp;
char ch;

path = "";
appname = "";
vparams.clear();
bool in_str = 0;
bool in_quote = 0;
bool beyond_app_name = 0;
//bool app_name_in_quotes = 0;

//cmdline = "c:/dos/edit prog.com";
//cmdline = "\"c:/dos/edit prog.com\" hello 123";
//cmdline = "c:/dos/edit.com hello 123";

int len =  cmdline.length();

if( len == 0 ) return;

for( int i = 0; i < len; i++ )
	{
	ch = cmdline[ i ];
	
	if( ch == '\"' )									//quote?
		{
		if( in_quote )
			{
			in_quote = 0;								//if here found closing quote
			goto got_param;
			}
		else{
			in_quote = 1;
			}
		}
	else{
		if( ch == ' ' )									//space?
			{
			if( !in_quote )				//if not in quote and space must be end of param
				{
				if( in_str ) goto got_param;
				}
			}
		else{
			in_str = 1;
			}

		if( in_str ) stmp += ch;
		}

	continue;

	got_param:

	in_str = 0;
	if ( beyond_app_name == 0 )					//store where approp
		{
		path = stmp;
		beyond_app_name = 1;
		}
	else{
		//store if not just a space
		if( stmp.compare( " " ) != 0 ) vparams.push_back( stmp );
		}

	stmp = "";
	}


//if here end of params reached, store where approp
if ( beyond_app_name == 0 )
	{
	path = stmp;
	}
else{
	vparams.push_back( stmp );
	}




appname = path;

len = path.length();
if( len == 0 ) return;

int pos = path.rfind( dir_seperator );

if( pos == string::npos )					//no directory path found?
	{
	path = "";	
	return;
	}

if( ( pos + 1 ) < len ) appname = path.substr( pos + 1,  pos + 1 - len );	//extract appname
path = path.substr( 0,  pos );								//extract path


//windows code
#ifdef compile_for_windows 
#endif

}









//find this apps path and prefix it to supplied filename
void get_app_path( string &path_out )
{
string s1, path;
mystr m1;


//linux code
#ifndef compile_for_windows

//get the actual path this app lives in
#define MAXPATHLEN 1025   // make this larger if you need to

int length;
char fullpath[MAXPATHLEN];

// /proc/self is a symbolic link to the process-ID subdir
// of /proc, e.g. /proc/4323 when the pid of the process
// of this program is 4323.
//
// Inside /proc/<pid> there is a symbolic link to the
// executable that is running as this <pid>.  This symbolic
// link is called "exe".
//
// So if we read the path where the symlink /proc/self/exe
// points to we have the full path of the executable.


length = readlink("/proc/self/exe", fullpath, sizeof(fullpath));
 
// Catch some errors:
if (length < 0)
	{
	syslog(LOG_ALERT,"Error resolving symlink /proc/self/exe.\n");
	fprintf(stderr, "Error resolving symlink /proc/self/exe.\n");
	exit(0);
	}

if (length >= MAXPATHLEN)
	{
	syslog(LOG_ALERT, "Path too long. Truncated.\n");
	fprintf(stderr, "Path too long. Truncated.\n");
	exit(0);
	}

//I don't know why, but the string this readlink() function 
//returns is appended with a '@'

fullpath[length] = '\0';      // Strip '@' off the end


//printf("Full path is: %s\n", fullpath);
//syslog(LOG_ALERT,"Full path is: %s\n", fullpath);

path = fullpath;
size_t found = path.rfind( "/" );
if ( found != string::npos ) path_out = path.substr( 0, found );
//syslog(LOG_ALERT,"Path only is: %s\n", csPathFilename.c_str());

#endif



//windows code
#ifdef compile_for_windows 
UINT i,uiLen;                    //eg. c\dos\edit.com /p /c		(as in screen-savers)
bool bQuotes;
string csCmdLineStr;


//----------------------------
//from GCCmdLine::GetAppName()
//eg: prog.com											//no path
//eg. "c:\dos\edit prog.com"							//with path\prog in quotes
//eg. "c:\dos\edit prog.com" c:\dos\junk.txt			//with path\prog in quotes and path\file
//eg. c\dos\edit.com /p /c		(as in screen-savers)	//path\prog and params no quotes
csCmdLineStr = GetCommandLine();

//csCmdLineStr = "skeleton.exe abc";
//printf("csCmdLineStr= '%s'\n", csCmdLineStr.c_str() );


string appname;
vector<string> vparams;
get_cmd_line( csCmdLineStr, path_out, appname, vparams  );

#endif


printf( "csPathFilename= %s\n", path_out.c_str() );

}




















void LoadSettings(string csIniFilename)
{
string csTmp, s1, s2;
int x,y,cx,cy;

GCProfile p(csIniFilename);
x=p.GetPrivateProfileLONG("Settings","WinX",100);
y=p.GetPrivateProfileLONG("Settings","WinY",100);
cx=p.GetPrivateProfileLONG("Settings","WinCX",750);
cy=p.GetPrivateProfileLONG("Settings","WinCY",550);

wndMain->position( x , y );	
wndMain->size( cx , cy );	


ilast_device_index = p.GetPrivateProfileLONG( "Settings", "last_device_index",0) ;



double dd = p.GetPrivateProfileDOUBLE( "Settings", "gph_scaley", 1.0 );
if( dd > 100 ) dd = 100;
if( dd < 0 ) dd = 0;
gph_scaley = dd;



if(pref_wnd!=0) pref_wnd->Load(p);
if(pref_wnd2!=0) pref_wnd2->Load(p);
if(font_pref_wnd!=0) font_pref_wnd->Load(p);

cols_build();
}














void SaveSettings( string csIniFilename )
{
string s1;

GCProfile p( csIniFilename );

//wndMain->resize();

//remove window border offset when saving winow pos settings
p.WritePrivateProfileLONG("Settings","WinX",wndMain->x()-iBorderWidth);
p.WritePrivateProfileLONG("Settings","WinY",wndMain->y()-iBorderHeight);
p.WritePrivateProfileLONG("Settings","WinCX",wndMain->w());
p.WritePrivateProfileLONG("Settings","WinCY",wndMain->h());

p.WritePrivateProfileLONG( "Settings", "last_device_index", ilast_device_index );

p.WritePrivateProfileDOUBLE( "Settings", "gph_scaley", gph_scaley );


if(pref_wnd!=0) pref_wnd->Save(p);
//if(pref_wnd2!=0) pref_wnd2->Save(p);
//if(font_pref_wnd!=0) font_pref_wnd->Save(p);


}




/*
// call this after designing the filter
inline void test_impulse_response(cl_halfband_decimator &dec)
{
mystr m1;
string s1, st;

    std::vector<filter_code::st_cplex_tag> vin(512);
    std::vector<filter_code::st_cplex_tag> vout;

    // create unit impulse
//    vin[0] = filter_code::st_cplex_tag{1.0f, 0.0f};
    for (size_t i = 0; i < vin.size(); i++)
		{
        vin[i] = filter_code::st_cplex_tag{0.0f, 0.0f};
		if( i == 128 ) vin[i] = filter_code::st_cplex_tag{1.0f, 0.0f};
		}

    // process impulse through decimator
    dec.process_block(vin, vout);

	
    // print output like Python
    std::cout << "\nImpulse Response (vout):\n";
    for (size_t i = 0; i < vout.size(); i++)
    {

	strpf( s1, "%.8f\n", vout[i].real );
	st += s1;
	

        std::cout << " " << i << ": "
                  << vout[i].real << "\n";
    }
    
    m1 = st;
	m1.writefile( "zzzhalfband_poly.txt" );
}
*/






bool filters_init = 1;

//see also  'filters_create()'  'filter_iir_rebuild()' 'filter_fir_rebuild()' 'filters_destroy()' 'filter_iir0_adjust()'   'filter_iir1_adjust()'  
void filters_create()
{
printf( "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC\n" );
printf( "filters_create()\n" );

if( filters_init )
	{
	filters_init = 0;

	int filt_idx = en_ftid_iir_notch0_aud;
	st_filt[ filt_idx ].fc0 = 400;
	st_filt[ filt_idx ].Q = 6;

	filt_idx = en_ftid_iir_notch1_aud;
	st_filt[ filt_idx ].fc0 = 1800;
	st_filt[ filt_idx ].Q = 6;
	
	filt_idx = en_ftid_iir_fmst_19k_pilot_notch;
	st_filt[ filt_idx ].fc0 = 19e3;
	st_filt[ filt_idx ].Q = 20;
	
	filt_idx = en_ftid_fir_fmst_15k_lpf0;
	st_filt[ filt_idx ].fc0 = 15e3;
	st_filt[ filt_idx ].Q = 0;											//not used
	
	filt_idx = en_ftid_fir_fmst_15k_lpf1;
	st_filt[ filt_idx ].fc0 = 15e3;
	st_filt[ filt_idx ].Q = 0;											//not used

	filt_idx = en_ftid_fir_fmst_23_53k_bpf;
	st_filt[ filt_idx ].fc0 = 20000;
	st_filt[ filt_idx ].fc1 = 55000;
	st_filt[ filt_idx ].Q = 0;											//not used
	}


filter_create_new();




float sampl_rate = downsample_srate;

vector<float>vcef;
fm_stereo_dwnsmpl_aa.fetch_inbuilt_coeffs( 0, vcef );
if( !fm_stereo_dwnsmpl_aa.create( 5, vcef ) )
	{
	printf( "filters_create() - failed to create 'fm_stereo_dwnsmpl_aa', coeff count: %d\n", (int)vcef.size() );
	
	}
else{
	printf( "filters_create() - created 'fm_stereo_dwnsmpl_aa', coeff count: %d\n", (int)vcef.size() );
	}





//--------
unsigned int odd_tap_cnt = 127;

filter_code::make_halfband_coeffs_float( odd_tap_cnt, vcef );

if( 0 )
	{
	printf("----\n" );
	for( int i = 0; i < vcef.size(); i++ )
		{
		printf( "filters_create() - vcef[%d] %f\n", i, vcef[i] );
		}

	printf("----\n" );
getchar();
	}

fm_stereo_decimator.load_coeffs( vcef );
//--------









//----	
iir_init( agc_iir0 );

filter_code::en_filter_pass_type_tag filt_type = filter_code::fpt_lowpass;
double fc1 = 1;
double Q = 1.0;
double db_gain = 0;
vector<double> vcoeff;

calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, sampl_rate, vcoeff );
if( !create_iir_filter_from_coeffs( agc_iir0, vcoeff ) )
	{
	printf( "filters_create() - failed at 'create_iir_filter_from_coeffs( agc_iir0, vcoeff )'\n" );
	}
else{
	printf( "filters_create() - 'agc_iir0' coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}

//----








/*
//------------------
//----
iir_init( usr_rtl_srate_lpf_iir_I0 );

filt_type = filter_code::fpt_lowpass;
fc1 = 10000;
Q = 1.0;
db_gain = 0;

calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, g_dev_bw, vcoeff );
if( !create_iir_filter_from_coeffs( usr_rtl_srate_lpf_iir_I0, vcoeff ) )
	{
	printf( "filters_create() - failed at 'usr_rtl_srate_lpf_iir_I0( usr_iq_lpf_iir_I0, vcoeff )'\n" );
	}
else{
	printf( "filters_create() - 'usr_rtl_srate_lpf_iir_I0'  srate %d  coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", (int)g_dev_bw, vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----



//----
iir_init( usr_rtl_srate_lpf_iir_Q0 );

//filt_type = filter_code::fpt_lowpass;
//fc1 = 10000;
//Q = 1.0;
//db_gain = 0;


//calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, g_dev_bw, vcoeff );
if( !create_iir_filter_from_coeffs( usr_rtl_srate_lpf_iir_Q0, vcoeff ) )
	{
	printf( "filters_create() - failed at 'create_iir_filter_from_coeffs( usr_rtl_srate_lpf_iir_Q0, vcoeff )'\n" );
	}
else{
	printf( "filters_create() - 'usr_rtl_srate_lpf_iir_Q0'  srate %d coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", (int)g_dev_bw, vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----
//------------------

//printf( "filters_create() - getchar()\n" );
//getchar();

*/






//----
iir_init( usr_hpf_iir0_I0 );

filt_type = filter_code::fpt_highpass;
fc1 = 80;
Q = 1.0;
db_gain = 0;

usr_hpf_iir0_I0.fc = fc1;												//store just for ref not used in filter calcs
usr_hpf_iir0_I0.q = 1.0;
usr_hpf_iir0_I0.db_gain = 1.0;

calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, sampl_rate, vcoeff );
if( !create_iir_filter_from_coeffs( usr_hpf_iir0_I0, vcoeff ) )
	{
	printf( "filters_create() - failed at 'create_iir_filter_from_coeffs( usr_hpf_iir0_I0, vcoeff )'\n" );
	}
else{
	printf( "filters_create() - 'usr_hpf_iir0_I0' coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----



//----
iir_init( usr_hpf_iir0_Q0 );

filt_type = filter_code::fpt_highpass;
fc1 = 80;
Q = 1.0;
db_gain = 0;


calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, sampl_rate, vcoeff );
if( !create_iir_filter_from_coeffs( usr_hpf_iir0_Q0, vcoeff ) )
	{
	printf( "filters_create() - failed at 'create_iir_filter_from_coeffs( usr_hpf_iir0_Q0, vcoeff )'\n" );
	}
else{
	printf( "filters_create() - 'usr_hpf_iir0_Q0' coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----









//----
iir_init( usr_lpf_iir0_I0 );

filt_type = filter_code::fpt_lowpass;
fc1 = g_user_iir_lpf0;
Q = 1.0;
db_gain = 0;

usr_lpf_iir0_I0.fc = fc1;												//store just for ref not used in filter calcs
usr_lpf_iir0_I0.q = 1.0;
usr_lpf_iir0_I0.db_gain = 0;

calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, sampl_rate, vcoeff );
if( !create_iir_filter_from_coeffs( usr_lpf_iir0_I0, vcoeff ) )
	{
	printf( "filters_create() - failed at 'create_iir_filter_from_coeffs( usr_lpf_iir0_I0, vcoeff )'\n" );
	}
else{
	printf( "filters_create() - 'usr_lpf_iir0_I0' coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----


//----
iir_init( usr_lpf_iir0_Q0 );

filt_type = filter_code::fpt_lowpass;
fc1 = g_user_iir_lpf0;
Q = 1.0;
db_gain = 0;

calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, sampl_rate, vcoeff );
if( !create_iir_filter_from_coeffs( usr_lpf_iir0_Q0, vcoeff ) )
	{
	printf( "filters_create() - failed at 'create_iir_filter_from_coeffs( usr_lpf_iir0_Q0, vcoeff )'\n" );
	}
else{
	printf( "filters_create() - 'usr_lpf_iir0_Q0' coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----








//----
iir_init( usr_lpf_iir1_I0 );

filt_type = filter_code::fpt_lowpass;
fc1 = g_user_iir_lpf1;
Q = 1.0;
db_gain = 0;

calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, sampl_rate, vcoeff );
if( !create_iir_filter_from_coeffs( usr_lpf_iir1_I0, vcoeff ) )
	{
	printf( "filters_create() - failed at 'create_iir_filter_from_coeffs( usr_lpf_iir1_I0, vcoeff )'\n" );
	}
else{
	printf( "filters_create() - 'usr_lpf_iir1_I0' coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----



//----
iir_init( usr_lpf_iir1_Q0 );

filt_type = filter_code::fpt_lowpass;
fc1 = g_user_iir_lpf1;
Q = 1.0;
db_gain = 0;

calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, sampl_rate, vcoeff );
if( !create_iir_filter_from_coeffs( usr_lpf_iir1_Q0, vcoeff ) )
	{
	printf( "filters_create() - failed at 'create_iir_filter_from_coeffs( usr_lpf_iir1_Q0, vcoeff )'\n" );
	}
else{
	printf( "filters_create() - 'usr_lpf_iir1_Q0' coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----










//----
iir_init( usr_lpf_iir2_I0 );

filt_type = filter_code::fpt_lowpass;
fc1 = g_user_iir_lpf2;
Q = 1.0;
db_gain = 0;

calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, sampl_rate, vcoeff );
if( !create_iir_filter_from_coeffs( usr_lpf_iir2_I0, vcoeff ) )
	{
	printf( "filters_create() - failed at 'create_iir_filter_from_coeffs( usr_lpf_iir2_I0, vcoeff )'\n" );
	}
else{
	printf( "filters_create() - 'usr_lpf_iir2_I0' coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----



//----
iir_init( usr_lpf_iir2_Q0 );

filt_type = filter_code::fpt_lowpass;
fc1 = g_user_iir_lpf2;
Q = 1.0;
db_gain = 0;

calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, sampl_rate, vcoeff );
if( !create_iir_filter_from_coeffs( usr_lpf_iir2_Q0, vcoeff ) )
	{
	printf( "filters_create() - failed at 'create_iir_filter_from_coeffs( usr_lpf_iir2_Q0, vcoeff )'\n" );
	}
else{
	printf( "filters_create() - 'usr_lpf_iir2_Q0' coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----




/*
//----
iir_init( usr_notch_iir0 );

filt_type = filter_code::fpt_notch;
fc1 = g_user_iir_notch_freq0;
Q = g_user_iir_notch_Q_0;
db_gain = 0;

calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, sampl_rate, vcoeff );
if( !create_iir_filter_from_coeffs( usr_notch_iir0, vcoeff ) )
	{
	printf( "filters_create() - failed at 'create_iir_filter_from_coeffs( usr_notch_iir0, vcoeff )'\n" );
	}
else{
	printf( "filters_create() - 'usr_notch_iir0' coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----

*/











/*
//----
iir_init( usr_demod_lpf_iir_Q0 );

filt_type = filter_code::fpt_lowpass;
fc1 = 2000;
Q = 1.0;
db_gain = 0;


calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, sampl_rate, vcoeff );
if( !create_iir_filter_from_coeffs( usr_demod_lpf_iir_Q0, vcoeff ) )
	{
	printf( "filters_create() - failed at 'create_iir_filter_from_coeffs( usr_demod_lpf_iir_Q0, vcoeff )'\n" );
	}
else{
	printf( "filters_create() - 'usr_demod_lpf_iir_Q0' coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----



//----
iir_init( usr_demod_lpf_iir_Q1 );

filt_type = filter_code::fpt_lowpass;
fc1 = 2000;
Q = 1.0;
db_gain = 0;


calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, sampl_rate, vcoeff );
if( !create_iir_filter_from_coeffs( usr_demod_lpf_iir_Q1, vcoeff ) )
	{
	printf( "filters_create() - failed at 'create_iir_filter_from_coeffs( usr_demod_lpf_iir_Q1, vcoeff )'\n" );
	}
else{
	printf( "filters_create() - 'usr_demod_lpf_iir_Q1' coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----
*/











//----
iir_init( usr_demod_hpf_iir_I0 );

filt_type = filter_code::fpt_highpass;
fc1 = 80;
Q = 1.0;
db_gain = 0;


calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, sampl_rate, vcoeff );
if( !create_iir_filter_from_coeffs( usr_demod_hpf_iir_I0, vcoeff ) )
	{
	printf( "filters_create() - failed at 'create_iir_filter_from_coeffs( usr_demod_hpf_iir_I0, vcoeff )'\n" );
	}
else{
	printf( "filters_create() - 'usr_demod_hpf_iir_I0' coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----



//----	
iir_init( usr_demod_hpf_iir_I1 );

filt_type = filter_code::fpt_highpass;
fc1 = 80;
Q = 1.0;
db_gain = 0;


calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, sampl_rate, vcoeff );
if( !create_iir_filter_from_coeffs( usr_demod_hpf_iir_I1, vcoeff ) )
	{
	printf( "filters_create() - failed at 'create_iir_filter_from_coeffs( usr_demod_hpf_iir_I1, vcoeff )'\n" );
	}
else{
	printf( "filters_create() - 'usr_demod_hpf_iir_I1' coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----	







//----	
iir_init( usr_demod_hpf_iir_Q0 );

filt_type = filter_code::fpt_highpass;
fc1 = 80;
Q = 1.0;
db_gain = 0;


calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, sampl_rate, vcoeff );
if( !create_iir_filter_from_coeffs( usr_demod_hpf_iir_Q0, vcoeff ) )
	{
	printf( "filters_create() - failed at 'create_iir_filter_from_coeffs( usr_demod_hpf_iir_Q0, vcoeff )'\n" );
	}
else{
	printf( "filters_create() - 'usr_demod_hpf_iir_Q0' coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----	



//----	
iir_init( usr_demod_hpf_iir_Q1 );

filt_type = filter_code::fpt_highpass;
fc1 = 80;
Q = 1.0;
db_gain = 0;


calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, sampl_rate, vcoeff );
if( !create_iir_filter_from_coeffs( usr_demod_hpf_iir_Q1, vcoeff ) )
	{
	printf( "filters_create() - failed at 'create_iir_filter_from_coeffs( usr_demod_hpf_iir_Q1, vcoeff )'\n" );
	}
else{
	printf( "filters_create() - 'usr_demod_hpf_iir_Q1' coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----	










//----
iir_init( wfall_iir0 );

filt_type = filter_code::fpt_lowpass;
fc1 = 20;
Q = 1.0;
db_gain = 0;

calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, sampl_rate, vcoeff );
if( !create_iir_filter_from_coeffs( wfall_iir0, vcoeff ) )
	{
	printf( "filters_create() - failed at 'create_iir_filter_from_coeffs( wfall_iir0, vcoeff )'\n" );
	}
else{
	printf( "filters_create() - 'wfall_iir0' coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----












//----	
filter_code::en_filter_window_type_tag wnd_type = filter_code::fwt_blackman_harris;
filter_code::en_filter_pass_type_tag filt_type2 = filter_code::fpt_bandpass;

unsigned int taps = g_bw_taps;
float fc01 = 80;
float fc02 = 3000;

usr_demod_bpf_fir_I0.verb = 1;
usr_demod_bpf_fir_I0.suser0 = "usr_demod_bpf_fir_I0";
usr_demod_bpf_fir_I0.user_id0 = 0;
usr_demod_bpf_fir_I0.created = 0;

filter_code::filter_fir_windowed( wnd_type, filt_type2, taps, fc01, fc02, sampl_rate, vcoeff );
filter_code::create_filter_from_coeffs( usr_demod_bpf_fir_I0, vcoeff );
filter_code::fir_init( usr_demod_bpf_fir_I0 );

usr_demod_bpf_fir_I0.bypass = 0;





usr_demod_bpf_fir_Q0.verb = 1;
usr_demod_bpf_fir_Q0.suser0 = "usr_demod_bpf_fir_Q0";
usr_demod_bpf_fir_Q0.user_id0 = 0;
usr_demod_bpf_fir_Q0.created = 0;


//filter_code::filter_fir_windowed( wnd_type, filt_type2, taps, fc01, fc02, sampl_rate, vcoeff );
filter_code::create_filter_from_coeffs( usr_demod_bpf_fir_Q0, vcoeff );
filter_code::fir_init( usr_demod_bpf_fir_Q0 );

usr_demod_bpf_fir_Q0.bypass = 0;
//----	








//----	
filt_type2 = filter_code::fpt_bandpass;

ssb_fir_lsb_I0.verb = 1;
ssb_fir_lsb_I0.suser0 = "ssb_fir_lsb_I0";
ssb_fir_lsb_I0.user_id0 = 0;
ssb_fir_lsb_I0.created = 0;


//fc01 = 80;
//fc02 = 3000;
//taps = fir_taps;

filter_code::filter_fir_windowed( wnd_type, filt_type2, taps, fc01, fc02, sampl_rate, vcoeff );
filter_code::create_filter_from_coeffs( ssb_fir_lsb_I0, vcoeff );
filter_code::fir_init( ssb_fir_lsb_I0 );
ssb_fir_lsb_I0.bypass = 0;





ssb_fir_lsb_Q0.verb = 1;
ssb_fir_lsb_Q0.suser0 = "ssb_fir_lsb_Q0";
ssb_fir_lsb_Q0.user_id0 = 0;
ssb_fir_lsb_Q0.created = 0;

//filter_code::filter_fir_windowed( wnd_type, filt_type, taps, fc01, fc02, sampl_rate, vcoeff );
filter_code::create_filter_from_coeffs( ssb_fir_lsb_Q0, vcoeff );
filter_code::fir_init( ssb_fir_lsb_Q0 );
ssb_fir_lsb_Q0.bypass = 0;

//printf( "filters_create() - press any key\n" );
//getchar();






//----
wnd_type = filter_code::fwt_kaiser;
filter_code::kaiser_beta = 6;
filt_type2 = filter_code::fpt_lowpass;
taps = 55;
fc01 = 15000;
fc02 = 0;


fm_ster_fir_15k_lpf0.verb = 1;
fm_ster_fir_15k_lpf0.suser0 = "fm_ster_fir_15k_lpf0";
fm_ster_fir_15k_lpf0.user_id0 = 0;
fm_ster_fir_15k_lpf0.created = 0;

filter_code::filter_fir_windowed( wnd_type, filt_type2, taps, fc01, fc02, fm_decimator_srate, vcoeff );
filter_code::create_filter_from_coeffs( fm_ster_fir_15k_lpf0, vcoeff );
filter_code::fir_init( fm_ster_fir_15k_lpf0 );
fm_ster_fir_15k_lpf0.bypass = 0;

//----	





//----
wnd_type = filter_code::fwt_kaiser;
filter_code::kaiser_beta = 6;
filt_type2 = filter_code::fpt_lowpass;
taps = 55;
fc01 = 15000;
fc02 = 0;


fm_ster_fir_15k_lpf1.verb = 1;
fm_ster_fir_15k_lpf1.suser0 = "fm_ster_fir_15k_lpf1";
fm_ster_fir_15k_lpf1.user_id0 = 0;
fm_ster_fir_15k_lpf1.created = 0;


filter_code::filter_fir_windowed( wnd_type, filt_type2, taps, fc01, fc02, fm_decimator_srate, vcoeff );
filter_code::create_filter_from_coeffs( fm_ster_fir_15k_lpf1, vcoeff );
filter_code::fir_init( fm_ster_fir_15k_lpf1 );
fm_ster_fir_15k_lpf1.bypass = 0;

//----	









//----	
wnd_type = filter_code::fwt_kaiser;
filter_code::kaiser_beta = 1;											//lower gives sharper transition, but causes more ripple in passband
filt_type2 = filter_code::fpt_bandpass;
taps = cn_fm_stereo_bpf_taps;
fc01 = 20000;
fc02 = 55000;


fm_ster_fir_23_53k_bpf.verb = 1;
fm_ster_fir_23_53k_bpf.suser0 = "fm_ster_fir_23_53k_bpf";
fm_ster_fir_23_53k_bpf.user_id0 = 0;
fm_ster_fir_23_53k_bpf.created = 0;


//fc01 = 80;
//fc02 = 3000;
//taps = fir_taps;

filter_code::filter_fir_windowed( wnd_type, filt_type2, taps, fc01, fc02, fm_decimator_srate, vcoeff );
filter_code::create_filter_from_coeffs( fm_ster_fir_23_53k_bpf, vcoeff );
filter_code::fir_init( fm_ster_fir_23_53k_bpf );
fm_ster_fir_23_53k_bpf.bypass = 0;

//----	



//fm_ster_15k_lpf0
//fm_ster_15k_lpf1
//fm_ster_23_53k_bpf



//----
iir_init( fm_ster_iir_19k_notch );

fm_ster_iir_19k_notch.suser0 = "fm_ster_iir_19k_notch";

filt_type = filter_code::fpt_notch;
fc1 = 19e3;
Q = 20.0;
db_gain = 0;


calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, fm_decimator_srate, vcoeff );
if( !create_iir_filter_from_coeffs( fm_ster_iir_19k_notch, vcoeff ) )
	{
	printf( "filters_create() - failed at 'create_iir_filter_from_coeffs( fm_ster_iir_19k_notch, vcoeff )'\n" );
	}
else{
	printf( "filters_create() - 'fm_ster_iir_19k_notch' coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----

}





//see also   'filters_create()' 'filter_iir_rebuild()' 'filter_fir_rebuild()' 'filters_destroy()' 'filter_iir0_adjust()'   'filter_iir1_adjust()'  
void filters_destroy()
{
printf( "filters_destroy()\n" );
filter_code::iir_delete_filter( agc_iir0 );								//these filters don't alloc mem, so these calls are not really required, just here for completeness
filter_code::iir_delete_filter( usr_hpf_iir0_I0 );
filter_code::iir_delete_filter( usr_hpf_iir0_Q0 );
filter_code::iir_delete_filter( usr_lpf_iir0_I0 );
filter_code::iir_delete_filter( usr_lpf_iir0_Q0 );
filter_code::iir_delete_filter( usr_demod_hpf_iir_I0 );
filter_code::iir_delete_filter( usr_demod_hpf_iir_Q0 );


filter_code::delete_filter( usr_demod_bpf_fir_I0 );						//this filter needs to be deleted as was created using mem alloc
filter_code::delete_filter( usr_demod_bpf_fir_Q0 );						//this filter needs to be deleted as was created using mem alloc



filter_code::delete_filter( ssb_fir_lsb_I0 );							//this filter needs to be deleted as was created using mem alloc
filter_code::delete_filter( ssb_fir_lsb_Q0 );							//this filter needs to be deleted as was created using mem alloc


filter_code::delete_filter( fm_ster_fir_15k_lpf0 );						//this filter needs to be deleted as was created using mem alloc
filter_code::delete_filter( fm_ster_fir_15k_lpf1 );						//this filter needs to be deleted as was created using mem alloc


filter_code::iir_delete_filter( fm_ster_iir_19k_notch );				//this filter does not use alloc mem, so this call is not really required, just here for completeness
filter_code::delete_filter( fm_ster_fir_23_53k_bpf );					//this filter needs to be deleted as was created using mem alloc



int ii = en_ftid_fir_fmst_15k_lpf0;
filter_code::delete_filter( st_filt[ii].fir[0] );
filter_code::delete_filter( st_filt[ii].fir[1] );

}







void DoQuit()
{
printf( "DoQuit()\n" );
mystr m1;

//call_rtl_fm_kill();
//cb_bt_freq_stop( 0, 0 );

//rtl.cancel_async();

if( wnd_rtl_graph != 0 ) wnd_rtl_graph->save_settings( csIniFilename );

SaveSettings(csIniFilename);

if(pref_wnd!=0) delete pref_wnd;
if(pref_wnd2!=0) delete pref_wnd2;

//if( wnd_rtl_graph != 0 ) delete wnd_rtl_graph;

stop_audio();
stop_threads();

//rtl.close();

if( wnd_rtl_graph != 0 ) wnd_rtl_graph->fftw_destroy_plans();

m1.delay_ms( 80 );

filters_destroy();

fftw_destroy_all_structs();

exit(0);
}









void cb_wndmain(Fl_Widget*, void* v)
{
Fl_Window* wnd = (Fl_Window*)v;

DoQuit();

}



extern int rec_play_iq_state;




void cb_wnd_rtl_graph(Fl_Widget*, void* v)
{
Fl_Window* wnd = (Fl_Window*)v;


if( ( rec_play_iq_state == 3 ) || ( rec_play_iq_state == 4 ) || ( rec_play_iq_state == 5 ) )
	{
	string s1;
	strpf( s1, "Can't quit, file record is running, stop this first." );
	fl_alert( s1.c_str(), 0 );
	return;
	}

DoQuit();
}


















void cb_btOpen(Fl_Widget *, void *)
{

//char *pPathName = fl_file_chooser("Open Record Schedule File?", "*",0);
//if (!pPathName) return;

//GCProfile p(csIniFilename);
//p.WritePrivateProfileStr("Settings","LastScheduleFile",pPathName);

}









void cb_open(Fl_Widget *, void *)
{
string s1;

s1 = csIniFilename;

char *pPathName = fl_file_chooser( "Open a File ?", "*", 0 );

//Fl_File_Chooser *fc = new Fl_File_Chooser( s1.c_str(), 0, FL_SINGLE, "Open a File" );
if (!pPathName) return;

LoadSettings(pPathName);

//fc->textfont( font_num );
//fc->textsize( font_size );


//fc->redraw();
//fc->show();
//fc->hide();
//fc->show();
//fc->fileName->show();
}








void cb_btSave(Fl_Widget *, void *)
{
	
char *pPathName = fl_file_chooser("Save Settings File ?", "*",0);
if (!pPathName) return;

SaveSettings( pPathName );

//GCProfile p(csIniFilename);
//p.WritePrivateProfileStr("Settings","LastScheduleFile",pPathName);

}







void cb_btQuit(Fl_Widget *, void *)
{
DoQuit();
}






void cb_bt_help( Fl_Widget *, void *)
{
cb_bt_clear ( 0, 0 );

outcsl( "--- Description of Thread operation with and without a Mutex Lock ---\n" );
outcsl( "A global str is loaded with text \"Hello\". Then a thread (Thread1) appends many asterisks to this global\n");
outcsl( "str via a loop, Thread1 will then erase every asterisk via a similar loop.\n" );
outcsl( "\nAn arbitrary timer callback (in another thread) shows you the current contents of the global\n" );
outcsl( "str repeatedly. If Thread1 is pre-empted by a task switch before all the asterisks are erased,\n" );
outcsl( "and the timer callback then executes, you will see some asterisks in the \"Hello\" text str.\n" );
outcsl( "\nThread1 can only be pre-empted by the timer callback if the Mutex checkbox is not enabled.\n" );
outcsl( "Likewise the timer callback can only be pre-empted by Thread1 if the Mutex checkbox is not enabled.\n" );
outcsl( "\nIf the Mutex checkbox is enabled, the interrupting thread (Thread1 or timer) will block at the\n" );
outcsl( "'pthread_mutex_lock()' call till the mutex is unlocked by which ever thread set it first. You will notice\n" );
outcsl( "when the Mutex checkbox is enabled and you press the StartThrd button, you will only see the text \"Hello\".\n" );
outcsl( "This demonstrates that only one thread had access to the global str and other thread was blocked, this\n" );
outcsl( "ensures the global str is mutually exclusive to only one thread at a time, thus avoiding possible\n" );
outcsl( "data inconsistencies and corruption.\n" );

tm tt;
string s1, s2, s3, s4, s5, s6, s7;
mystr m1;


m1.get_time_now( tt );
m1.make_time_str( tt, s1, s2, s3, s4 );
outcsl( "   Time is: " );
outcsl( s4 );

void make_date_str( struct tm tn, string &dow, string &dom, string &mon_num, string &mon_name, string &year, string &year_short );

m1.make_date_str( tt, s1, s2, s3, s4, s5, s6 );
strpf( s7, "%s-%s-%s, ", s2.c_str(), s3.c_str(), s6.c_str() );
outcsl( "   Date is: " );
outcsl( s7 );

strpf( s7, "   %s %s-%s-%s", s1.c_str(), s2.c_str(), s4.c_str(), s5.c_str() );
outcsl( s7 );

}





int cb_led1(Fl_Window *w, int e)
{

/*
GCLed *ledx;

ledx=(GCLed *)w;

int i=ledx->GetColIndex();
	if(i>=0)
		{
		i++;
		if(i>2) i=0;
		ledx->ChangeCol(i);
		}
*/
}








//keep edit obj lines within spec ranges
bool cull_console_lines_at_begining( int max_lines, int min_lines )
{

if( max_lines <= 0 ) return 0;
if( min_lines < 0 ) return 0;

if( min_lines >=  max_lines )  return 0;

int char_count = tb_csl->length();

int line_count = tb_csl->count_lines( 0, char_count );

if( line_count > max_lines )
	{
	int charpos_at_line = tb_csl->skip_lines( 0, line_count - min_lines );
	tb_csl->replace( 0, charpos_at_line, "" );
	
	}
	
return 1;
}









int device_cnt = -1;
string sdevice_list;
int device_index = -1;
bool bstart_synthesis = 0;

void ask_which_rtl_device()
{
string s1, s2;

if( ilast_device_index >= device_cnt ) ilast_device_index = device_cnt - 1;

strpf( s1, "Enter index of an avail device to open:\n%s", sdevice_list.c_str() );
strpf( s2, "%d", ilast_device_index );

char *sz = fl_input( s1.c_str(), s2.c_str() );

int ii = 1;
if( sz != 0 )
	{
	sscanf( sz, "%d", &ii );	

	if( ( ii < 0 ) || ( ii >= device_cnt ) )
		{
		strpf(s1, "Invalid index, should have been between: 0 --> %d, will exit...\n", device_cnt-1 );
		fl_alert( s1.c_str(), 0 );
		exit(0);
		}
	else{
		device_index = ii;
		ilast_device_index = ii;
		}
	}
else{
	strpf( s1, "Start I/Q synthesis to simulate an RTL dongle stream with test signals ?" );
	int ret = fl_choice( s1.c_str(),"No","Start Synthsis", 0 );
	if( ret == 1 )
		{
		bstart_synthesis = 1;
		}

//	strpf( s1, "Will exit...");
//	fl_alert( s1.c_str(), 0 );
//	exit(0);
	}
start_up_state = 3;
}










//clear stale text file that 'tops_thread.sh' script generates, refer 'tops_thread_stats()'
void tops_thread_clear_file()
{
string s1;
mystr m1;

printf( "tops_thread_clear_file() - clearing file: '%s'\n", cns_tops_thread_text_fname );

if( m1.writefile( cns_tops_thread_text_fname ) )
	{
	}
}








/* --- 'top' script, example dump into file: 'cns_tops_thread_text_fname' ---   see 'tops_thread.sh', cmd example run in a loop is: top -n 1 -b -H -p `pidof rtl_scan` > tops_thread.txt

top - 13:54:44 up 2 days, 17:09,  2 users,  load average: 2.05, 2.56, 1.64
Threads:   5 total,   1 running,   4 sleeping,   0 stopped,   0 zombie
%Cpu(s): 35.0 us,  0.0 sy,  0.0 ni, 65.0 id,  0.0 wa,  0.0 hi,  0.0 si,  0.0 st 
MiB Mem :  15399.2 total,    573.4 free,  10222.6 used,   5139.5 buff/cache     
MiB Swap:   4096.0 total,   2210.0 free,   1886.0 used.   5176.6 avail Mem 

    PID USER      PR  NI    VIRT    RES    SHR S  %CPU  %MEM     TIME+ COMMAND
2010623 me        20   0 1074932 228184  22760 S  60.0   1.4   4:24.05 rtl_scan
2010616 me        20   0 1074932 228184  22760 R  20.0   1.4   2:03.86 rtl_scan
2010535 me        20   0 1074932 228184  22760 S  10.0   1.4   0:59.88 rtl_scan
2010621 me        20   0 1074932 228184  22760 S   0.0   1.4   0:00.00 alsa-pi+
2010622 me       -84   0 1074932 228184  22760 S   0.0   1.4   0:00.33 data-lo+

*/


/* --- 'top' script, example dump into file: 'cns_tops_thread_text_fname' ---   see 'tops_thread.sh', cmd example run in a loop is: top -n 1 -b -H -p `pidof rtl_scan` > tops_thread.txt

top - 19:23:36 up 5 days, 23:32,  2 users,  load average: 0.80, 1.10, 1.03
Threads:   6 total,   0 running,   6 sleeping,   0 stopped,   0 zombie
%Cpu(s): 16.3 us,  4.7 sy,  0.0 ni, 79.1 id,  0.0 wa,  0.0 hi,  0.0 si,  0.0 st 
MiB Mem :  15399.2 total,    800.6 free,   6435.2 used,   6604.5 buff/cache     
MiB Swap:   4096.0 total,   4094.6 free,      1.4 used.   8964.0 avail Mem 

    PID USER      PR  NI    VIRT    RES    SHR S  %CPU  %MEM     TIME+ COMMAND
 110753 gc        20   0 1189616 264400  23968 S  20.0   1.7   0:38.16 RtlSdrL+
 110677 gc        20   0 1189616 264400  23968 S  10.0   1.7   0:39.60 RtlSdrL+
 110734 gc        20   0 1189616 264400  23968 S   0.0   1.7   0:00.00 libusb_+
 110746 gc        20   0 1189616 264400  23968 S   0.0   1.7   0:03.71 RtlSdrL+
 110751 gc        20   0 1189616 264400  23968 S   0.0   1.7   0:00.00 alsa-pi+
 110752 gc       -84   0 1189616 264400  23968 S   0.0   1.7   0:00.33 data-lo+
*/


//read the file: 'cns_tops_thread_text_fname', this is generated when a terminal is running script: 'tops_thread.sh', which loops linux 'top' command (make sure 'tops_thread.sh' is running from this app's directory)
void tops_thread_stats()
{
bool vb = 0;

string s1, s2;
mystr m1;


string fname = cns_tops_thread_text_fname;

if( m1.readfile( fname, 10000 ) )
	{
	vector<string> vstr1, vstr2, vstr20, vstr3;
	m1.LoadVectorStrings( vstr1, '\n' );

	if(vb)printf("tops_thread_stats() - read line count: %d, from file '%s'\n", (int)vstr1.size(), fname.c_str() );

	s2 += cnsAppName[0];										//only search for a partial 'cnsAppName' as 'tops' crops long named app names
	s2 += cnsAppName[1];
	s2 += cnsAppName[2];
	s2 += cnsAppName[3];
	s2 += cnsAppName[4];
	s2 += cnsAppName[5];
//	s2 = "RtlSdrL";

	for( int i = 0; i < vstr1.size(); i++ )
		{
		if( vstr1[i].size() > 40 )										//make sure there's a resonable len to line
			{
			m1 = vstr1[i];
			
			if( m1.cut_at_first_find( s1, s2, 0 ) )				//does text line have matching app name?
				{
				if(vb)printf("tops_thread_stats() - vstr1[%02d]  '%s'\n", i, s1.c_str() );
				
				m1 = s1;
				m1.FindReplace( s1, "  ", " ", 0);						//remove superfluous formatting, keep a single space as delimmiter

				m1 = s1;
				m1.FindReplace( s1, "  ", " ", 0);

				m1 = s1;
				m1.FindReplace( s1, "  ", " ", 0);

				m1 = s1;
				m1.FindReplace( s1, "  ", " ", 0);

				m1 = s1;
				m1.FindReplace( s1, "\t", " ", 0);

				m1 = s1;
				m1.FindReplace( s1, "\r", "", 0);
				
				m1 = s1;
				m1.FindReplace( s1, "\n", "", 0);


				m1.LoadVectorStrings( vstr2, ' ');						

				vstr20.clear();
				for( int j = 0; j < vstr2.size(); j++ )					//remove any confused empty columns, was noted when white space preceeding first column
					{
					if( vstr2[j].length() != 0 ) vstr20.push_back( vstr2[j] );
					}
				
				for( int j = 0; j < vstr20.size(); j++ )
					{
					if(vb)printf("tops_thread_stats() -      vstr20[%02d]  '%s'\n", j, vstr20[j].c_str() );
					
					if( j == 8 )										//at time of writing this code, linux 'top' cmd placed cpu utilisation in 9th column
						{
						vstr3.push_back( vstr20[j] );
						}
					}
				}
			}
		}

	if( vstr3.size() >= 1 )												//have a value to do an avg?
		{		
		float f0;
		sscanf( vstr3[0].c_str(), "%f", &f0 );
	
		if(vb)printf("vstr3[0]  '%s'\n", vstr3[0].c_str() );
		
		tops_thread_stats_avg_sum += f0;
		tops_thread_stats_avg = tops_thread_stats_avg_sum / tops_thread_stats_cnt;
		tops_thread_stats_cnt++;
		}

	if( vstr3.size() >= 2 )												//have a value to do an avg?
		{		
		float f0;
		sscanf( vstr3[1].c_str(), "%f", &f0 );
		if(vb)printf("vstr3[1]  '%s'\n", vstr3[1].c_str() );
		
		tops_thread_stats_avg_sum += f0;
		tops_thread_stats_avg = tops_thread_stats_avg_sum / tops_thread_stats_cnt;
		tops_thread_stats_cnt++;
		}

	if( vstr3.size() >= 3 )												//have a value to do an avg?
		{		
		float f0;
		sscanf( vstr3[2].c_str(), "%f", &f0 );
		if(vb)printf("vstr3[2]  '%s'\n", vstr3[2].c_str() );
		
		tops_thread_stats_avg_sum += f0;
		tops_thread_stats_avg = tops_thread_stats_avg_sum / tops_thread_stats_cnt;
		tops_thread_stats_cnt++;
		}

	if( vstr3.size() == 1 )
		{		
		strpf( s_tops_thread_stats, "%s   ??   ??     avg %.2f", vstr3[0].c_str(), tops_thread_stats_avg );
		}

	if( vstr3.size() == 2 )
		{		
		strpf( s_tops_thread_stats, "%s   %s   ??      avg %.2f", vstr3[0].c_str(), vstr3[1].c_str(), tops_thread_stats_avg  );
		}

	if( vstr3.size() == 3 )
		{		
		strpf( s_tops_thread_stats, "%s   %s   %s      avg %.2f", vstr3[0].c_str(), vstr3[1].c_str(), vstr3[2].c_str(), tops_thread_stats_avg  );
		}

	if( vstr3.size() == 4 )
		{		
		strpf( s_tops_thread_stats, "%s   %s   %s   %s avg %.2f", vstr3[0].c_str(), vstr3[1].c_str(), vstr3[2].c_str(), vstr3[3].c_str(), tops_thread_stats_avg  );
		}

	}
else{
	s_tops_thread_stats = "??   ??   ??        ";
	}
}









int tops_thread_cnt = 0;

float tim_10sec_cnt = 0;
//mystr m1_tmp;


int cb_timer1_cnt = 0;

void cb_timer1(void *)
{
string s1, s2;
mystr m1;

Fl::repeat_timeout( cn_timer_period, cb_timer1 );

//printf( "cb_timer1() - b_listen %d\n", b_listen );


if( !b_first_timer_call )
	{
	wnd_rtl_graph->fftw_create_plans();
	b_first_timer_call = 1;
	}


if( start_up_state == 3 )					//4th exec
	{
	start_up_state = 4;		//this is the last state, normal running begins
	
	if( device_index >= 0 )
		{
		if( ( !rtl.open( device_index ) ) || ( b_use_synthesis_dont_use_rtl_dev ) )
			{
			if( b_use_synthesis_dont_use_rtl_dev) printf( "cb_timer1() - skipping use of rtl dongle, can be due to flag on cmdline 'no_rtl=1', or boolean c code flag: 'b_use_synthesis_dont_use_rtl_dev=1' was set in code build.\n" );
			else printf( "cb_timer1() - failed to open rtl dongle index: %d\n", device_index );
			
			if( b_use_synthesis_dont_use_rtl_dev) strpf(s1, "Failed to open rtl dongle index: %d\n\n==> Will turn on IQ synthesis mode to simulate a dongle and provide test signals.", device_index );
			else strpf(s1, "Skipping use of rtl dongle, can be due to flag on cmdline 'no_rtl=1', or boolean c code flag: 'b_use_synthesis_dont_use_rtl_dev=1' was set in code build.\n\nWill turn on IQ synthesis mode to simulate a dongle and provide test signals." );
			
			fl_alert( s1.c_str(), 0 );
			
			wnd_rtl_graph->b_synth_iq = 1;
			set_synth_mode_and_menu_state( wnd_rtl_graph->b_synth_iq );

//			wnd_rtl_graph->fftw_adjust_plans();
			
//			fftw_access_gui_thrd( true );


			start_threads_rtl();
			start_audio();
			
			filters_create();
//			filter_adjust_fir_bw_lower_upper();

			}
		else{															//if here dongle opened
//			strncpy( szdev_name, rtl.get_device_name( device_index ), 256 );
			s_dev_name =  rtl.get_device_name();
			
//			rtl.get_get_device_usb_strings( device_index, szdev_manufact, szdev_product, szdev_serial );
			rtl.get_get_device_usb_strings( s_dev_manufact, s_dev_product, s_dev_serial );

			rtl.set_srate( g_dev_bw );

			int bf_size_rtl = 8 * 6553;
			
			int sr = aud_op_srate;


			float time_per_frame = framecnt * 1.0f/sr;

			printf("timer1() - srate %d, framecnt %d, time_per_time %f, g_rtl_bw %f\n", sr, framecnt, time_per_frame, g_dev_bw );
			
			rtl.set_buf_size( 8 * 65536 );

			start_threads_rtl();
			if( !start_audio() )
				{
				strpf(s1, "Failed to open audio device, start from console to see details (srate: %d, frame size: %d)", aud_op_srate, framecnt  );
				printf( "%s\n", s1.c_str() );
				fl_alert( s1.c_str(), 0 );
				}
			
			filters_create();

//b_listen = 1;

			tim.time_start( tim.ns_tim_start );

			wnd_rtl_graph->b_synth_iq = 0;
			set_synth_mode_and_menu_state( wnd_rtl_graph->b_synth_iq );
			
			}
		}
	else{
		start_threads_rtl();
		}
	
	filter_adjust_fir_bw_lower_upper();
	b_need_filter_rebuild = 1;
	
	cb_bt_freq_stop( 0, 0 );
	wnd_rtl_graph->freq_listen( 1, 1, 1, 1, "cb_timer1()" );
//	cb_bt_freq_single( 0, 0 );
	}


if( start_up_state == 1 )					//2nd exec
	{
	if( device_cnt == 0 )
		{
		if( b_use_synthesis_dont_use_rtl_dev) printf( "cb_timer1() - Skipping use of rtl dongle, can be due to flag on cmdline 'no_rtl=1', or boolean c code flag: 'b_use_synthesis_dont_use_rtl_dev=1' was set in code build\n" );
		else printf( "cb_timer1() - No rtl devices found.\n" );
		if( b_use_synthesis_dont_use_rtl_dev) strpf(s1, "Skipping use of rtl dongle, can be due to flag on cmdline 'no_rtl=1', or boolean c code flag: 'b_use_synthesis_dont_use_rtl_dev=1' was set in code build.\n\n==> Will turn on IQ synthesis mode to simulate a dongle and provide test signals.\n" );
		else strpf(s1, "No rtl devices found.\n\n==> Will turn on IQ synthesis mode to simulate a device and provide test signals.\n" );
		fl_alert( s1.c_str(), 0 );

		wnd_rtl_graph->b_synth_iq = 1;
		set_synth_mode_and_menu_state( wnd_rtl_graph->b_synth_iq );

//		wnd_rtl_graph->fftw_adjust_plans();
		
//		filter_adjust_fir_bw_lower_upper();
//		b_need_filter_rebuild = 1;

		start_threads_rtl();
		start_audio();
		filters_create();

		wnd_rtl_graph->ineed_graph_fit = 15*2;

		start_up_state = 4;					//this is the last state, normal running begins
		}
	else{
		start_up_state = 2;
		ask_which_rtl_device();
		if( bstart_synthesis )
			{
//			b_use_synthesis_dont_use_rtl_dev = 1;
			wnd_rtl_graph->b_synth_iq = 1;
			set_synth_mode_and_menu_state( wnd_rtl_graph->b_synth_iq );
			
			start_threads_rtl();
			start_audio();
			filters_create();

			start_up_state = 4;
			}

		}

	wndMain->hide();
	}


if( start_up_state == 0 )					//1st exec
	{
	start_up_state = 1;
	i_fftw_trig_plan_create_state = 0;									//show plans are to be created
	
	if( !b_use_synthesis_dont_use_rtl_dev ) device_cnt = rtl.get_device_count();
	else device_cnt = 0;


	printf("cb_timer1() - rtl capable device list:\n" );

	sdevice_list.clear();

	for( int i = 0; i < device_cnt; i++ )
		{
		string ss_dev_name;
		string ss_dev_manufact;
		string ss_dev_product;
		string ss_dev_serial;
		bool b_is_open = 0;
		
//	printf("xxxxxxxxxxxxxxxxxxhere0\n");		
		ss_dev_name = rtl.get_device_name_using_index( i );
//		strncpy( szdev_name, rtl.get_device_name( i ), 256 );
//		ss_dev_name = rtl.get_device_name();

//	printf("xxxxxxxxxxxxxxxxxxhere1\n");		
		rtl.get_get_device_usb_strings_using_index( i, ss_dev_manufact, ss_dev_product, ss_dev_serial );

//	printf("xxxxxxxxxxxxxxxxxxhere2\n");		

		if( !rtl.open( i ) ) 
			{
			b_is_open = 1;
			}
		else{
			rtl.close();
			}



		if( rtl.open_status() ) b_is_open = 1;

		if( b_is_open ) s2 = "busy";
		else s2 = "avail";
		
		strpf(s1, "Index %d (%s) --> '%s', Mfr: %s, Prod: %s, Serial: %s\n", i, s2.c_str(), ss_dev_name.c_str(), ss_dev_manufact.c_str(), ss_dev_product.c_str(), ss_dev_serial.c_str() );
		sdevice_list += s1;
		}
	printf( "%s", sdevice_list.c_str() );
	}


//printf( "timer1() - imode = %d\n", imode );


if( !wnd_rtl_graph ) return;




wnd_rtl_graph->tick( cn_timer_period );

//---
if( i_fftw_trig_plan_create_state == 1 )
	{
	i_fftw_trig_plan_create_state = 2;									//show plans are ready for use

//	printf( "cb_timer1() - xxxxxxxxxxxxxxxxxxxxx i_fftw_trig_plan_create_state %d, will create fftw plans\n", i_fftw_trig_plan_create_state );
	
//	wnd_rtl_graph->fftw_adjust_plans();
	
//	i_fftw_trig_plan_create_state = 2;		//plan create, fttw is avail
	}
//---




tops_thread_cnt++;
if( tops_thread_cnt > 10 )
	{
	tops_thread_cnt = 0;
	
//	printf( "cb_timer1() - xxxxxxxxxxxxxxxxxxxxx tops_thread_cnt became %d, calling 'tops_thread_stats()' \n", tops_thread_cnt );
	tops_thread_stats();	
	}




//this ensures other instances know that this app is running (by locking a file), refer 'check_instance_exists()'
tim_10sec_cnt -= cn_timer_period;
if( tim_10sec_cnt < 0 )
	{
	tim_10sec_cnt = 10;
	if( check_instance_try_lock( check_instance_fd ) )					//try and get file lock
		{
//		printf("cb_timer1() - the 'check instance exists' file lock was obtained by this app, new instances will know this instance is already running\n" );
		}


//float dt = m1_tmp.time_passed(m1_tmp.ns_tim_start );
//m1_tmp.time_start(m1_tmp.ns_tim_start );
//printf("cb_timer1() - m1_tmp %f\n", dt );
		
//		printf("cb_timer1() - the 'check instance exists' file lock was obtained by this app, new instances will know this instance is already running\n" );
	}



wnd_rtl_graph->update_controls();


if( !b_dbg_no_graph_update ) 
	{
	wnd_rtl_graph->update_prep_gph0();										//this is the main user graph with grid


//	if( ( b_plot_gph1 ) || ( b_plot_gph1 ) )
		{
	//	if( cb_timer1_cnt < 10 ) wnd_rtl_graph->plot_gph01();
		wnd_rtl_graph->plot_gph01();										//this is fast_mgraph 'gph1'
		b_plot_gph1 = 0;
		}
	}

if( start_up_state != 4 ) return;


if( ( imode == en_mode_scan_mode ) || (  imode == en_mode_scan_mode_single ) )
	{
	int idx;

	if( imode == en_mode_scan_mode_single )
		{
 		imode = en_mode_stop;
		return;
		}



	double xmin, xmax, ymin, ymax;

	wnd_rtl_graph->get_trace_min_max( xmin, xmax, ymin, ymax );
//	printf( "\n\nthreshold: y=%lg, x=%lg\n\n", ymax, xmax );

	double xx, yy;
	wnd_rtl_graph->get_trace_maxy( idx, xx, yy );

//	printf( "\n\nthreshold: idx= %d, y=%lg, x=%lg\n\n", idx, ymax, xmax );


	double db_max_threshold = wnd_rtl_graph->threshold;

	if( ymax >= db_max_threshold )
		{
//		cb_bt_freq_stop( 0, 0 );
		wnd_rtl_graph->set_selected_sample( 0, idx, 1 );
		}
	else{
		if( imode == en_mode_scan_mode ) do_scan();
		}


//	scan_start();

	}


cb_timer1_cnt++;
}












//linux code
#ifndef compile_for_windows

//execute shell cmd
int RunShell(string sin)
{

if ( sin.length() == 0 ) return 0;

//make command to cd working dir to app's dir and execute app (params in "", incase of spaces)
//strpf(csCmd,"cd \"%s\";\"%s\" \"%s\"",csPath.c_str(),sEntry[iEntryNum].csStartFunct.c_str(),csFile.c_str());

pid_t child_pid;

child_pid=fork();		//create a child process	

if(child_pid==-1)		//failed to fork?
	{
	printf("\nRunShell() failed to fork\n");
	return 0;
	}

if(child_pid!=0)		//parent fork? i.e. child pid is avail
	{
	int status;
	printf("\nwaitpid: %d, RunShell start\n",child_pid);	

	while(1)
		{
		waitpid(child_pid,&status,0);		//wait for return val from child so a zombie process is not left in system
		printf("\nwaitpid %d RunShell stop\n",child_pid);
		if(WIFEXITED(status)) break;		//confirm status returned shows the child terminated
		}	
	}
else{					//child fork (0) ?
//	printf("\nRunning Shell: %s\n",csCmd.c_str());
	printf("\nRunShell system cmd started: %s\n",sin.c_str());	
	system(sin.c_str());
	printf("\nRunShell system cmd finished \n");	
	exit(1);
	}
return 1;
}

#endif











//windows code
#ifdef compile_for_windows

//execute shell cmd as a process that can be monitored
int RunShell( string sin )
{
BOOL result;
wstring ws1;

if ( sin.length() == 0 ) return 0;


mystr m1 = sin;

m1.mbcstr_wcstr( ws1 );	//convert utf8 string to windows wchar string array


memset(&processInformation, 0, sizeof(processInformation));


STARTUPINFOW StartInfoW; 							// name structure
memset(&StartInfoW, 0, sizeof(StartInfoW));
StartInfoW.cb = sizeof(StartInfoW);

StartInfoW.wShowWindow = SW_HIDE;

result = CreateProcessW( NULL, (WCHAR*)ws1.c_str(), NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &StartInfoW, &processInformation);

if ( result == 0)
	{
	
	return 0;
	}

return 1;



//bkup_filelist_SEI[ 0 ].cbSize = sizeof( bkup_filelist_SEI[ 0 ] ); 
//bkup_filelist_SEI[ 0 ].lpVerb = "open"; 
//bkup_filelist_SEI[ 0 ].lpFile = sin.c_str(); 
//bkup_filelist_SEI[ 0 ].lpParameters= 0; 
//bkup_filelist_SEI[ 0 ].nShow = SW_HIDE; 
//bkup_filelist_SEI[ 0 ].fMask = SEE_MASK_NOCLOSEPROCESS; 

//ShellExecuteEx( &bkup_filelist_SEI[ 0 ] );     //execute batch file



//WCHAR cmd[] = L"cmd.exe /c pause";
//LPCWSTR dir = L"c:\\";
//STARTUPINFOW si = { 0 };
//si.cb = sizeof(si);
//PROCESS_INFORMATION pi;

//STARTUPINFO StartInfo; 							// name structure
//PROCESS_INFORMATION ProcInfo; 						// name structure
//memset(&ProcInfo, 0, sizeof(ProcInfo));				// Set up memory block
//memset(&StartInfo, 0 , sizeof(StartInfo)); 			// Set up memory block
//StartInfo.cb = sizeof(StartInfo); 					// Set structure size

//int res = CreateProcess( NULL, (char*)sin.c_str(), 0, 0, TRUE, 0, NULL, NULL, &StartInfo, &ProcInfo );

}

#endif









//open preference specified editor with supplied fname as parameter 
void open_editor( string fname )
{
string s1;

s1 = "\"";
s1 += fav_editor;
s1 += "\"";
s1 += " ";
s1 += "\"";
s1 += fname;
s1 += "\"";


//linux code
#ifndef compile_for_windows
RunShell( s1 );
#endif



//windows code
#ifdef compile_for_windows
wstring ws1;
mystr m1 = s1;

m1.mbcstr_wcstr( ws1 );	//convert utf8 string to windows wchar string array

//WCHAR cmd[] = L"cmd.exe /c pause";
//LPCWSTR dir = L"c:\\";
//STARTUPINFOW si = { 0 };
//si.cb = sizeof(si);
//PROCESS_INFORMATION pi;

STARTUPINFOW StartInfoW; 							// name structure
PROCESS_INFORMATION ProcInfo; 						// name structure
memset(&ProcInfo, 0, sizeof(ProcInfo));				// Set up memory block
memset(&StartInfoW, 0 , sizeof(StartInfoW)); 		// Set up memory block
StartInfoW.cb = sizeof(StartInfoW); 				// Set structure size

int res = CreateProcessW(NULL, (WCHAR*)ws1.c_str(), 0, 0, TRUE, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &StartInfoW, &ProcInfo );

#endif
}




















































































void show_usage()
{
string s1;
strpf( s1, "\n\nrtl_scan, %s, usage:\n\n", cns_version );
printf("%s" , s1.c_str() );

printf("cfg_fname=/home/you/rtl_scan.ini        specify path and filename to use when creating or reading config file.\n" );
printf("no_rtl=1                                don't probe for rtl devices, use synthesized rtl in software, tune to 0.0 MHz to hear synth carriers\n" );
printf("\n\ne.g: ./rtl_scan no_rtl=1\n");
printf("\n\n");
}




extern fast_mgraph gph1;
extern vector<float> vgph1_x;
extern vector<float> vgph1_y0;
extern vector<float> vgph1_y1;


/*
// call this after designing the filter
inline void test_impulse_response2(cl_halfband_decimator &dec)
{
mystr m1;
string s1, st;

    std::vector<filter_code::st_cplex_tag> vin(127);
    std::vector<filter_code::st_cplex_tag> vout;

    // create unit impulse
//    vin[0] = filter_code::st_cplex_tag{1.0f, 0.0f};
    for (size_t i = 0; i < vin.size(); i++)
		{
        vin[i] = filter_code::st_cplex_tag{0.0f, 0.0f};
		if( i == 0 ) vin[i] = filter_code::st_cplex_tag{1.0f, 0.0f};
		}

    // process impulse through decimator
    dec.process_block(vin, vout);

	
    // print output like Python
    std::cout << "\nImpulse Response (vout):\n";
    for (size_t i = 0; i < vout.size(); i++)
    {

	strpf( s1, "%.8f\n", vout[i].real );
	st += s1;
	

        std::cout << " " << i << ": "
                  << vout[i].real << "\n";
    }
    
    m1 = st;
	m1.writefile( "zzzhalfband_poly.txt" );
	
	vector<float> vgph1_x;
	vector<float> vgph1_y0;
	vector<float> vgph1_y1;

	vector<float> vimpulse;

	for( int i = 0; i < vout.size(); i++ )
		{
		vimpulse.push_back( vout[i].real );
		}

	vector<float> vcoeff;
	vector<float> vomega;
	vector<float> vamp;
	unsigned int points = 1024;

	freqz( vimpulse, points, vomega, vamp );
			
	for( int i = 0; i < vomega.size(); i++ )
		{
		vgph1_x.push_back( vomega[i] );
		vgph1_y0.push_back( vamp[i] );
		}


	s1 = " plot_gph01()";
	int gph_idx = 0;						//only one graph that has multiple traces
	gph1.copy_label( s1.c_str() );

	gph1.position( 60, 30 );
 	gph1.font_size( 9 );
	gph1.set_sig_dig( 2 );
	gph1.sample_rect_hints_distancex = 0;
	gph1.sample_rect_hints_distancey = 0;

//	gph1.scale_y( gph_idx, 1.0, 1.0, 1.0, 0.1 );
//	gph1.shift_y( gph_idx, 0.0, 0.0, -0.00, -0.0 );

	gph1.shift_y( gph_idx, 0, 0.0f, 0.0f, 0.0f);

//	gph1.yunits_perpxl[0] = -1;
	gph1.max_defl_y[ 0 ] = 20;

	gph1.plotxy_vfloat_1( vgph1_x, vgph1_y0 );

}
*/





/*
void test_plot_freqz()
{
printf("test_plot_freqz() - close to continue...\n");

fm_stereo_decimator.set_sample_rate( 48000 );
fm_stereo_decimator.design_filter(127);           						//halfband polyphase decimator

test_impulse_response2(fm_stereo_decimator);

int iAppRet=Fl::run();

}
*/

/*

void test_halfband_polyphase_delete()
{
printf("test_halfband_polyphase() - close to continue...\n");

halfband_poly_decimator::st_halfband_tag hb;

vector<float> vcoeff =
{
0,0,2,0,4,5,4,0,2,0,0
};

hb.vb = 1;

//hb.design_hb_poly_lpf( 240000, 11, vcoeff, 1, 1 );

hb.load_coeffs( vcoeff );

vector<float> vin;
vector<float> vout;

for( int i = 0; i < 40; i++ )
	{
	vin.push_back( (float)i );
	}

hb.process( vin, vout );

for( size_t i = 0; i < vout.size(); i++ )
	{
	printf( "y[%d] = %f\n", (int)i, vout[i] );
	}
}

*/


/*
void test_halfband_polyphase()
{
string s1;

vector<float> vin, vout;


vector<float> vcoeff =
{
-0.000676, 0.000000, 0.012719, -0.000000, -0.062740, 0.000000, 0.300939, 0.500000, 0.300939, 0.000000, -0.062740, -0.000000, 0.012719, 0.000000, -0.000676
};

#define cn_cnt 128
float bf[cn_cnt];

for( int i = 0; i < cn_cnt; i++ )
	{
	if( i == 20 ) vin.push_back ( 1.0f );
	else vin.push_back ( 0.0f );
	}

for( int i = 0; i < vin.size() - vcoeff.size(); i++ )
	{
	float sum = 0;
	for( int j = 0; j < vcoeff.size(); j++ )
		{
		sum += vin[i + j] * vcoeff[j];			//mac
		}
	vout.push_back( sum );
	}


	s1 = " plot_gph01()";
	int gph_idx = 0;						//only one graph that has multiple traces
	gph1.copy_label( s1.c_str() );

	gph1.position( 60, 30 );
 	gph1.font_size( 9 );
	gph1.set_sig_dig( 2 );
	gph1.sample_rect_hints_distancex = 0;
	gph1.sample_rect_hints_distancey = 0;

//	gph1.scale_y( gph_idx, 1.0, 1.0, 1.0, 0.1 );
//	gph1.shift_y( gph_idx, 0.0, 0.0, -0.00, -0.0 );

	gph1.shift_y( gph_idx, 0, 0.0f, 0.0f, 0.0f);

//	gph1.yunits_perpxl[0] = -1;
	gph1.max_defl_y[ 0 ] = 20;

	gph1.plot_vfloat_1( vout );

	gph1.fit_plot( 0 );

int iAppRet=Fl::run();
printf(" test_halfband_polyphase()\n" );
exit(0);
}
*/



/*
void test_kaiser_wnd()
{
string s1;

printf(" test_kaiser_wnd()\n" );

filter_code::en_filter_window_type_tag wnd_type = filter_code::fwt_kaiser;


vector<float> vwnd;

filter_code::window_function_float( wnd_type, 63, vwnd );

//filter_code::window_kaiser_float( 63, 6, vwnd );


printf("kaiser---------\n" );
for( int i = 0; i < vwnd.size(); i++ )
	{
	vgph1_x.push_back( i );
	vgph1_y0.push_back( vwnd[i] );
	printf("kaiser[%3d]: %f\n", i, vwnd[i] );
	
	}
printf("kaiser--------\n" );


	s1 = " plot_gph01()";
	int gph_idx = 0;						//only one graph that has multiple traces
	gph1.copy_label( s1.c_str() );

	gph1.position( 60, 30 );
 	gph1.font_size( 9 );
	gph1.set_sig_dig( 2 );
	gph1.sample_rect_hints_distancex = 0;
	gph1.sample_rect_hints_distancey = 0;

//	gph1.scale_y( gph_idx, 1.0, 1.0, 1.0, 0.1 );
//	gph1.shift_y( gph_idx, 0.0, 0.0, -0.00, -0.0 );

	gph1.shift_y( gph_idx, 0, 0.0f, 0.0f, 0.0f);

//	gph1.yunits_perpxl[0] = -1;
	gph1.max_defl_y[ 0 ] = 20;

	gph1.plotxy_vfloat_1( vgph1_x, vgph1_y0 );
	int iAppRet=Fl::run();

//getchar();
}
*/







int main(int argc, char **argv)
{
	
//test_kaiser_wnd();

//test_halfband_polyphase();
//getchar();

//test_plot_freqz();

//Fl::scheme("plastic");								//optional
bool vb = 0;
string s, s1, st, sequ, fname, dir_sep;
bool add_ini = 1;									//assume need to add ini extension	
mystr m1;

//Fl::set_font( FL_HELVETICA, "Helvetica bold");
//Fl::set_font( FL_COURIER, "");
//Fl::set_font( FL_COURIER, "Courier bold italic");

//fl_font( (Fl_Font) FL_COURIER, 12 );

fname = cnsAppName;							//assume ini file will have same name as app
dir_sep = "";								//assume no path specified, so no / or \ (dos\windows)


gph1.hide();								//hide this so  below 'check_instance_exists()' dlg button 'Don't Run App, Exit' will 
											//actually close the whole app as there is a 'Fl::run()' req in that function to show its dlg

//filter_test_sweep();
//getchar();


//test if another app instance is already running and ask if to still to run this instance -
// will return 1 if user presses 'Don't Run, Exit' button
if( check_instance_exists( cnsAppName ) ) return 0;

//linux code
#ifndef compile_for_windows
dir_seperator = "/";									//use unix folder directory seperator
#endif


dir_sep = dir_seperator;



//windows code
//attach a command line console, so printf works
#ifdef compile_for_windows
int hCrt;
FILE *hf;

AllocConsole();
hCrt = _open_osfhandle( (long) GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
hf = _fdopen( hCrt, "w" );
*stdout = *hf;
setvbuf( stdout, NULL, _IONBF, 0 );
#endif

get_app_path( app_path );				//get app's path


//path = app_path;


//windows code
#ifdef compile_for_windows
h_mutex1 = CreateMutex( NULL, FALSE, NULL );  // make a win mutex obj, not signalled ( not locked)
#endif




//handle command line params
printf("\n\n"cnsAppName"\n");
    printf("~~~~~~~~\n\n");


//printf("\nSpecify no switches to use config with app's filename app's dir\n");
//printf("\nSpecify '--cf ffff' to use config with filename 'ffff'\n");


//if( argc == 1 )
//	{
//	printf( "-- Using app's dir for config storage.\n" );
//	}

/*
if( argc == 3 )
	{
	if( strcmp( argv[ 1 ], "--cf" ) == 0 )
		{
		printf( "-- Using specified filename for config storage.\n" );
		fname = argv[ 2 ];
		app_path = "";
		dir_sep = "";								//no directory seperator
		add_ini = 0;								//user spec fname don't add ini ext
		}
	}
*/


//---- cmdline params ---
for( int i = 1; i < argc; i++ )
	{
	if(vb)printf("[%d] '%s'\n", i, argv[i] );
	st += argv[i];
	st += ",";
	}

m1 = st;
float fv;
int iv;
unsigned int ui;


if( m1.ExtractParamVal( "-help", sequ ) )
	{
	show_usage();
	exit(0);
	}

if( m1.ExtractParamVal( "--help", sequ ) )
	{
	show_usage();
	exit(0);
	}

/*
if( m1.ExtractParamVal_with_delimit( "verb=", ",", sequ ) )
	{
	if( sequ.compare( "on" ) == 0 )
		{
		verbose = 1;
		vb = verbose;	
		}
	if(vb)printf( "verb=%d \n", verbose );
	}
*/

if( m1.ExtractParamVal_with_delimit( "cfg_fname=", ",", sequ ) )
	{
	fname = sequ;

	if(vb)printf( "cfg_fname='%s' \n", sequ.c_str() );
	printf( "-- Using specified pathname for config storage.\n" );
	
	app_path = "";
	dir_sep = "";								//no directory seperator
	add_ini = 0;								//user spec fname don't add ini ext
	}


if( m1.ExtractParamVal_with_delimit( "no_rtl=", ",", sequ ) )
	{
	if( sequ.compare( "1" ) == 0 )
		{
		b_use_synthesis_dont_use_rtl_dev = 1;
		}
	if(vb)printf( "no_rtl=%d \n", b_use_synthesis_dont_use_rtl_dev );
	}

//----




csIniFilename = app_path + dir_sep + fname;			//make config file pathfilename
if( add_ini ) csIniFilename += ".ini";				//need ini if no user spcified fname





printf("\n\n-> Config pathfilename is:'%s', this will be used for config recall and saving.\n",csIniFilename.c_str());




fftw_clear_all_structs();

tops_thread_clear_file();



wndMain = new Fl_Window(50,50,780,550);
wndMain->label(cnsAppWndName);



//calc offset window border, for removal when saving settings
iBorderWidth=wndMain->x();
iBorderHeight=wndMain->y();
wndMain->border(1);
iBorderWidth=wndMain->x()-iBorderWidth;
iBorderHeight=wndMain->y()-iBorderHeight;

//menu bar
meMain = new Fl_Menu_Bar(0, 0, wndMain->w(), 25);
meMain->textsize(12);
meMain->copy(menuitems, wndMain);


Fl_Box *bx1 = new Fl_Box( 0, meMain->h(), 700, 150, "");
bx1->box( FL_BORDER_BOX );


//this box is used by resizable() function, it limits window resizing
//notice buttons that cross this box in a the x dimension are resized in that dimension on user resizing the main wnd
Fl_Box *bx2 = new Fl_Box( 0, meMain->h()+155, 95, 50, " Resizable(Box)");
bx2->box( FL_BORDER_BOX );

Fl_Button *btTest = new Fl_Button(50,50,55,20,"Test");
btTest->labelsize(12);
btTest->callback(cb_btTest,0);

Fl_Button *btTest2 = new Fl_Button(120,50,55,20,"Test2");
btTest2->labelsize(12);
btTest2->callback(cb_btTest2,0);



Fl_Button *btClear = new Fl_Button( 370,50,55,20,"Clear");
btClear->labelsize(12);
btClear->callback( cb_bt_clear, 0 );


Fl_Button *btHelp = new Fl_Button( 50,80,55,20,"Help");
btHelp->labelsize(12);
btHelp->callback( cb_bt_help, 0 );




//windows code
//attach a command line console, so printf works
#ifdef compile_for_windows
fi_gcpipe_app->value( "D:\\gc\\MinGW\\bin\\gdb.exe" );
#endif



//GCProfile p(csIniFilename);
//p.WritePrivateProfileStr("Settings","UTF-8", s1.c_str() );
//p.Save();
//p.GetPrivateProfileStr( "Settings", "UTF-8", "This is a default value", &s1 );



ckMutex = new Fl_Check_Button( 190,70,100,20,"Use Mutex");
ckMutex->labelsize(12);
ckMutex->callback( cb_ck_mutex, 0 );

fi_status = new Fl_Input( 190,95,100,20,"ThrdStatus:");
fi_status->labelsize(12);

led1 = new GCLed(300,100,10,10,"");
//led1->end();

led1->SetColIndex(0, 100, 0, 0);
led1->SetColIndex(1, 255,0, 0);
//led1->SetEventCallback( &cb_led1 );

//grp1->end();



tb_csl = new Fl_Text_Buffer;
te_csl = new Fl_Text_Editor( cnGap,  wndMain->h() - cnCslHei - cnGap, wndMain->w() - 2 * cnGap , cnCslHei );
te_csl->buffer( tb_csl );
te_csl->textsize(12);
te_csl->textfont(4);


buf_allocate();

wndMain->end();
wndMain->callback((Fl_Callback *)cb_wndmain, wndMain);

make_pref_wnd();
make_pref2_wnd();
make_font_pref_wnd();


wndMain->resizable( bx2 );	//note this must be before LoadSettings(), where the window is resized

LoadSettings(csIniFilename); 


//create_hilbert_fir_filters();
//create_lsb_usb_iq();


//---
Fl_Pixmap *pxm_icon_rtl_xpm = new Fl_Pixmap( icon_rtl_32x_xpm );
Fl_RGB_Image *img_icon_rtl = new Fl_RGB_Image( pxm_icon_rtl_xpm, Fl_Color(0) );

Fl_Pixmap *pxm_icon_ntc_xpm = new Fl_Pixmap( icon_ntc_32x_xpm );
img_icon_ntc = new Fl_RGB_Image( pxm_icon_ntc_xpm, Fl_Color(0) );


Fl_Pixmap *pxm_icon_prb_xpm = new Fl_Pixmap( icon_prb_32x_xpm );
img_icon_prb = new Fl_RGB_Image( pxm_icon_prb_xpm, Fl_Color(0) );
gph1.icon( img_icon_prb );

Fl_Pixmap *pxm_icon_fav_xpm = new Fl_Pixmap( icon_fav_32x_xpm );
img_icon_fav = new Fl_RGB_Image( pxm_icon_fav_xpm, Fl_Color(0) );

//---



wnd_rtl_graph = new rtl_graph_wnd( 30, 30, 1200, 900, cnsAppName );
wnd_rtl_graph->end();
wnd_rtl_graph->callback( (Fl_Callback *)cb_wnd_rtl_graph, wnd_rtl_graph );

wnd_rtl_graph->icon( img_icon_rtl );




//fi_unicode_greek->textfont( font_num );	//needed this after LoadSettings() so the ini font value is loaded via font_pref_wnd->Load(p);
//fi_unicode->textfont( font_num );		//needed this after LoadSettings() so the ini font value is loaded via font_pref_wnd->Load(p);


wndMain->show(argc, argv);
if( wnd_rtl_graph != 0 ) wnd_rtl_graph->load_settings( csIniFilename, 1 );
wnd_rtl_graph->show();

//fl_font( (Fl_Font) font_num, font_size );


st_spect_tag o;

o.freq = 1010;
o.ampl = -10;
vspect.push_back( o );

o.freq = 1020;
o.ampl = -10;
vspect.push_back( o );


o.freq = 1030;
o.ampl = -5;
vspect.push_back( o );


o.freq = 1040;
o.ampl = -2;
vspect.push_back( o );

o.freq = 1050;
o.ampl = -0;
vspect.push_back( o );

o.freq = 1060;
o.ampl = -3;
vspect.push_back( o );

o.freq = 1070;
o.ampl = -6;
vspect.push_back( o );


vspect.clear();

//call_rtl_fm_kill();
//rtl_scan( 88.0e6, 108.0e6, 5000, 5 );

//wnd_rtl_graph->update_graph( vspect );


init_fftw_arrays();

Fl::add_timeout( 0.5, cb_timer1 );		//update controls, post queued messages


 
int iAppRet=Fl::run();

close( check_instance_fd );

return iAppRet;

}














//start a timing timer with internal ns resolution
//the supplied var 'start' will hold an abitrary count in ns used by call time_passed()
void time_start( unsigned long long int &start )
{

//linux code
#ifndef compile_for_windows 

timespec ts_now;

clock_gettime( CLOCK_MONOTONIC, &ts_now );		//initial time read
start = ts_now.tv_nsec;							//get ns count
start += (double)ts_now.tv_sec * 1e9;					//make secs count a ns figure

#endif




//windows code
#ifdef compile_for_windows

LARGE_INTEGER li;
unsigned long long int big;

QueryPerformanceFrequency( &li );		//get performance counter's freq


big = 0;
big |= li.HighPart;
big = big << 32;
big |= li.LowPart;

perf_period = (double)1.0 / (double)big;		//derive performance counter's period

//cslpf("Freq = %I64u\n", big );
//cslpf("Period = %g\n", perf_period );

QueryPerformanceCounter( &li );

start = 0;
start |= li.HighPart;
start = start << 32;
start |= li.LowPart;

start = start * perf_period * 1e9;					//make performance counter a ns figure
#endif

}








//calc secs that have passed since call to time_start()
//calcs are maintained in ns internally and in supplied var 'start'
//returns time that has passed in seconds
double time_passed( unsigned long long int start )
{
unsigned long long int now, time_tot;

//linux code
#ifndef compile_for_windows 

timespec ts_now;

clock_gettime( CLOCK_MONOTONIC, &ts_now );		//read current time
now = ts_now.tv_nsec;							//get ns count
now += (double)ts_now.tv_sec * 1e9;				//make secs count a ns figure

time_tot = now - start;

#endif





//windows code
#ifdef compile_for_windows

LARGE_INTEGER li;

QueryPerformanceCounter( &li );

now = 0;
now |= li.HighPart;
now = now << 32;
now |= li.LowPart;

now = now * perf_period * 1e9;			//conv performance counter figure into ns

time_tot = now - start;
#endif


return (double)time_tot * 1e-9;			//conv internal ns delta to secs
}



















///linux code
#ifndef compile_for_windows

//---------------------- thread -------------------------
void* Thread1( void* arg )
{
string *s1 = ( string * ) arg;
timespec ts_old, ts_new;
bool need_mutex = use_mutex;


sThrd1.iThreadFinished = 0;

printf( "\nThread1 started, stopping after 10 secs ...\n" );


clock_gettime( CLOCK_MONOTONIC, &ts_old );				//initial time read

#define maxcount 25
for ( int z = 0; z < 100 ; z++ )
	{
	if( sThrd1.iKillThread == 1 ) goto finthread1;

	clock_gettime( CLOCK_MONOTONIC, &ts_new );			//get new time

	if ( ts_new.tv_sec >= ts_old.tv_sec + 7 ) break;	//finish thread after x secs
	
	timespec ts, tsret;				//don't hog processor in this for() loop
	ts.tv_sec = 0;					//this can hold seconds
//	ts.tv_nsec = 0x3b9ac9ff;		//0x3b9ac9ff = 999999999 nS or 0.999999999 secs
	ts.tv_nsec = 500000000;			//500000000 nS or 0.5 secs
//	nanosleep( &ts , &tsret );

	int endptr;
	for ( int i = 0; i < 1000000; i ++ )
		{
    	if( sThrd1.iKillThread == 1 ) goto finthread1;

		if (need_mutex )
			{
			//block other thread to gain mutually exclusive access to sglobal
			//i.e. cb_timer will be blocked if it calls pthread_mutex_lock(..)

			pthread_mutex_lock( &mutex1 );			//block other thread to gain mutually exclusive access to sglobal
			}

		for ( int k = 0; k < maxcount; k ++ )
			{
			sglobal += "*";
			}

		endptr = sglobal.length() - 1;
		for ( int j = 0; j < maxcount; j++ )
			{
			sglobal.erase( endptr );
			endptr--;
			}

		if (need_mutex ) 
			{
			pthread_mutex_unlock( &mutex1 );		//release mutex
			}
		}


//	printf( "Thread1 loop %02d: Passed arg is a string pointer: %s\n" , gi , s1->c_str() );
//	break;
	}

finthread1:

printf( "\nThread1 finished...\n" );

sThrd1.iThreadFinished = 1;
return  (void*) 1;	
}
//-------------------------------------------------------
#endif












//windows code
#ifdef compile_for_windows
//only use mutex locked logpf_thrd() calls
//---------------------- thread -------------------------
DWORD WINAPI DosThread(void* lpData)
{
bool need_mutex = use_mutex;
string s1;
double duration;

printf( "\nThread1 started, stopping after 10 secs ...\n" );

sThrd1.iThreadFinished = 0;

time_start( ns_tim_start1 );

#define maxcount 25
for ( int z = 0; z < 100 ; z++ )
	{
	if( sThrd1.iKillThread == 1 ) goto finthread1;

    Sleep( 500 );

	duration = time_passed( ns_tim_start1 );
	if ( duration >= 7 ) break;
	


	int endptr;
	for ( int i = 0; i < 1000000; i ++ )
		{
    	if( sThrd1.iKillThread == 1 ) goto finthread1;

		if (need_mutex )
			{
			//block other thread to gain mutually exclusive access to sglobal
			//i.e. cb_timer will be blocked if it calls pthread_mutex_lock(..)

			WaitForSingleObject( h_mutex1, INFINITE );		//block other thread to gain mutually exclusive access to sglobal
			}

		for ( int k = 0; k < maxcount; k ++ )
			{
			sglobal += "*";
			}

		endptr = sglobal.length() - 1;
		for ( int j = 0; j < maxcount; j++ )
			{
			sglobal.erase( endptr );
			endptr--;
			}

		if (need_mutex ) 
			{
			ReleaseMutex( h_mutex1 );						//release mutex
			}
		}


//	printf( "Thread1 loop %02d: Passed arg is a string pointer: %s\n" , gi , s1->c_str() );
//	break;
	}



finthread1:

sThrd1.iThreadFinished = 1;
printf( "\nThread1 finished...\n" );

return 1;

}
#endif
//-------------------------------------------------------


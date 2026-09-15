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

//rtl_graph.h
//v1.06


#ifndef rtl_graph_h
#define rtl_graph_h



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

#include <fftw3.h>
//#include <rfftw3.h>


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
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Round_Button.H>

#include "globals.h"
#include "pref.h"
#include "GCProfile.h"
#include "GCLed.h"
#include "gclog.h"
#include "gcthrd.h"
#include "gcpipe.h"
#include "rtlobj.h"
//#include "pcaudio.h"

#include "gc_rtaudio.h"
#include "rt_code.h"
#include "my_input_wheel.h"
#include "input_dropbox.h"
#include "mgraph.h"
#include "favourite_code.h"
#include "aa_canvas.h"
#include "bmp_code.h"
#include "audio_formats.h"
#include "audio_formats.h"
#include "gc_srateconv.h"
#include "filter_code.h"
#include "vert_meter.h"
#include "rotary_knob_code.h"
#include "demod_code.h"
//#include "elliptical_iir_code.h"
#include "button_wheel_code.h"
#include "fm_demode_code.h"
//#include "halfband_filter_code.h"
#include "iir_sos_code.h"
#include "aud_spect_code.h"

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


#define cns_build_date build_date
#define cns_led_preset_tooltip "recalls a stored preset which holds most params,\nright click to store current tuned settings"




#define cn_bytes_per_pixel 3


#define cn_max_tune_history 100
#define cn_max_scan_history 30
#define cn_spec_avg_slots_max 16
#define cn_vspec_avg_size_max 327680


#define cn_rtl_buf_size (65536*16)	//for rtl IQ buffers

#define cn_dev_bwidth_selections_max 48

//#define cn_carrier_avg_bf_size 16

#define cn_down_sample_factor_small 10			//eg: for wideband fm
#define cn_down_sample_factor_large 52			//eg: for am    e.g: 2496000 * ((1/48000)*2048) = 106496 ----> 106496/2048 = 52

#define cn_carrier_max_freq_dead_band 100e3;							//half of this value below center tuning, other half is above center tuning

#define cn_freq_memory_max 16
#define cn_preset_memory_max 16


#define cn_preset_memory_max_clm 50										//number of leds in a column of one 'Fl_Tabs' wdj							
#define cn_preset_memory_max2 (cn_preset_memory_max_clm*8)				//multiply by number of 'Fl_Tabs'


#define cn_filter_memory_max 5
#define cn_dwn_srate_memory_max 4

#define cn_dwn_srate_memory_max 4

#define cn_aa_dwnsrate_fc_max 1

#define cn_morse_dit_time 120e-3										
#define cn_morse_dah_time (cn_morse_dit_time*3)
#define cn_morse_letter_gap_time (cn_morse_dit_time*3)
#define cn_morse_word_gap_time (cn_morse_dit_time*7)

#define cn_morse_fade_time 5e-3											//time its takes a morse tone to fade up/down, helps stops aliasing by applying a slew rate limit

#define cn_morse_tone_freq 450


#define cn_clip_audio_limit_low   (0.01f)								//refer 'clip_level_audio'
#define cn_clip_audio_limit_high  (30.0f)

#define cn_tune_history_dropdown_max 512

#define cn_synth_voice_fname0 "zzzmorse_pause.wav"
#define cn_synth_voice_fname1 "zzzmorse_long.wav"
#define cn_synth_voice_fname2 "ttsmaker-we_unwilling.au"
#define cn_synth_voice_fname3 "ttsmaker-qsl.au"
#define cn_synth_voice_fname4 "zzzmorse_pause.wav"

//#define cns_hilbert_45_plus "hilbert_45_plus_127.txt"
//#define cns_hilbert_45_minus "hilbert_45_minus_127.txt"

#define cns_hilbert_plus45_127taps_24KHz "hilbert_plus45_127taps_24KHz.txt"
#define cns_hilbert_minus45_127taps_24KHz "hilbert_minus45_127taps_24KHz.txt"

//#define cns_hilbert_plus45_32taps_24KHz "hilbert_plus45_32taps_24KHz.txt"
//#define cns_hilbert_minus45_32taps_24KHz "hilbert_minus45_32taps_24KHz.txt"



#define cn_downsample_srate_min (4000)
#define cn_downsample_srate_max (480000)

#define cn_graph_mousewheel_vert_region_high 0.3f						//upper 1/3 of of graph, used for mouse tweek of freq at course level
#define cn_graph_mousewheel_vert_region_middle 0.75f					//middle 3rd of of graph, used for mouse tweek of freq at fine level
#define cn_graph_mousewheel_vert_region_verylow 0.95f					//very lower fraction of of graph, used for mouse tweek of hzoom


#define cn_pref_zero_padding_for_fft_cnt_min 1							//refer 'pref_zero_padding_for_fft_cnt'
#define cn_pref_zero_padding_for_fft_cnt_max 15

#define cn_aud_dim_time_default (2.0f)									//refer 'aud_dim_time'


/*

enum class fftw_state_tag : int											//provides thread safe access to fftw plans/arrays
{
ready = 0,        														//gui and audio thrd can use fftw plans/arrays
rebuild_req,      														//the gui thrd and audio thrd asks gui thrd to rebuild
draining,         														//audio thrd needs to stop using fftw plans/arrays, it will set state 'rebuilding' when clear of fftw
rebuilding        														//gui thrd can rebuild fftw plans/arrays via 'fftw_adjust_plans()', when its rebuilt it set 'ready' state
};

extern std::atomic<fftw_state_tag> fftw_state_atm{ fftw_state_tag::ready };	//provides thread safe access to fftw plans/arrays and allows a 'fftw_adjust_plans()' mechanisim


extern bool fftw_access_gui_thrd( bool bneed_fftw_adj );
extern inline bool fftw_access_audio_thrd( bool bneed_fftw_adj );
*/


struct st_filt_bode_tag													//filter transfer function
{
vector<double> vx;
vector<double> vy;
vector<int> vpx;														//pixel vals
vector<int> vpy;
};



//refer 'filters_create()'  'filter_iir_rebuild()'  'filters_destroy()'
enum en_filter_idx_tag													//index into 'st_filt[]'
{
en_ftid_iir_notch0_aud,													//audio notch filter, fc set by user click on the aud spectrum graph, Q is user adj
en_ftid_iir_notch1_aud,

en_ftid_iir_fmst_19k_pilot_notch,										//for fm stereo	
en_ftid_fir_fmst_15k_lpf0,												//for fm stereo
en_ftid_fir_fmst_15k_lpf1,												//for fm stereo
en_ftid_fir_fmst_23_53k_bpf,											//for fm stereo
};






enum en_filter_flag_tag													//boolean, 1: in_use,  2: fir  etc
{
en_fflg_zero = 0x0,														//zero value to OR with
en_fflg_alloc = 0x1,													//if not set the filter is available for allocation and is not configured
en_fflg_fir = 0x2,														//filter is an fir and not an iir
en_fflg_on = 0x4,														//filter is on
};



struct st_filter_sdr_tag
{
string sname;															//optional user string
int int0;																//optional user integer
int int1;																//optional user integer
en_filter_flag_tag flags;
unsigned int srate;
float db_gain;															//where applicable, such as for a peaking iir
float fc0;
float fc1;
float Q;
unsigned int taps;														//taps count used to calc fir coeffs																												
vector<double>vcef;												//holds either last calc'd iir 2nd order coeffs (a1,a2,b0,b1,b2), or last calc'd fir coeffs 

int active_idx;						//audio thrd controls this, it shows which filter the audio thrd is using, e.g: 'iir[ 0 ]' or 'iir[ 1 ]' (assuming filter struct represents an iir), audio thrd switches this when 'pending_idx' is not -1, thread safety mechanism
bool inactive_idx;					//index of inactive 'iir[]' ( or 'fir[]') filter which can be modified by gui thrd, gui thrd also toggles 'inactive_idx', allows ping ponging, thread saftey mechanism
std::atomic<int> pending_idx;		//'pending_idx' is set to 'inactive_idx' by gui thread when 'pending_idx' is equal to -1, audio thread flags it has grabbed the 'pending_idx' into 'active_idx' and is using 'active_idx', it does this by setting 'pending_idx' to -1, thread safety mechanism

filter_code::st_iir iir[2];												//iir filter struct (if this struct is NOT flagged as an 'en_fflg_fir' ), for ping ponging thread safe mechanism

filter_code::st_fir fir[2];												//fir filter struct (if this struct is IS flagged as an 'en_fflg_fir' ), for ping ponging thread safe mechanism

};





struct st_freq_pin
{
int px, py;																//pixel position
int freq_actual;
int freq_tune;
int freq_sub_tune;
bool sel;
};




enum en_go_flags_tag
{
en_gflg_vis = 0x1,														//must be boolean, 0x1: vis,  0x2: flashing,  0x4: text vis,  0x8: selectable
en_gflg_flashing = 0x2,
en_gflg_text_vis = 0x4,
en_gflg_selectable = 0x8,
en_gflg_follows_freq = 0x16,											//'x' coord is derived from and follows 'st_gph_obj_tag.freq_sub_tune'
};




enum en_gph_obj_type_tag
{
en_got_sel_rect,
en_got_freq_pin,
en_got_demod_btn,

};




struct st_gph_obj_tag													//holds a graph obj, e.g: pin marking a freq, or a clickable button rect such as AM
{
en_go_flags_tag flags;													//must be boolean, refer 'en_pin_flags_tag'
string stext;
en_mgraph_draw_obj_type shape;
int freq_actual;
int freq_tune;
int freq_sub_tune;

en_gph_obj_type_tag type;												//
int int0;																//could hold a value such as 'en_demodulator_type_tag',
int pin_idx;															//an index in 'vpin[]' to allow pin deletion


int descent;															//in pixels, 0 is top of graph

int font;
int font_size;
int line_thick;
en_mgraph_line_style line_style;
en_mgraph_text_justify justify;

//int radius;
int x, y;
int offsx, offsy;														//offset
int wid;
int hei;

int px1, py1;															//pixel positions in wdg, used to detect mouse hovering over this pin
int px2, py2;


int rr;
int gg;
int bb;

};




#define cn_fftw_plan_siz 8

enum en_fftw_plans_type_tag												//refer 'en_fftw_index_allocations_tag'  'st_fftw_plans_tag'
{																		//refer 'fftw_clear_struct()'  'fftw_build_if_req()' 'fftw_destroy_all_structs()'
en_fpt_fwd_real_to_cplx,
en_fpt_fwd_cplx_to_cplx,

en_fpt_rvs_cplx_to_real,
en_fpt_rvs_cplx_to_cplx,
en_fpt_illegal_type,
};



struct st_fftw_plans_tag												//refer 'en_fftw_index_allocations_tag'  'en_fftw_plans_type_tag'
{																		//refer 'fftw_clear_struct()'  'fftw_build_if_req()' 'fftw_destroy_all_structs()'
string sname;
bool built;
en_fftw_plans_type_tag type;
unsigned int size;

double *fftwir;
double *fftwor;
fftw_complex *fftwic;
fftw_complex *fftwoc;

fftw_plan fftwp;
};




//these last enumeration (which is an index int 'st_fftw[]') must be less that 'cn_fftw_plan_siz'
enum en_fftw_index_allocations_tag										//refer 'st_fftw_plans_tag'  'en_fftw_plans_type_tag'
{																		//refer 'fftw_clear_struct()'  'fftw_build_if_req()' 'fftw_destroy_all_structs()'
en_fia_gph0,															//refer 'update_prep_gph0_downsample()'
en_fia_plot_gph1,														//refer 'aud_spect_gui_thrd()'
en_fia_aud_spect_notch,														
};








struct st_ctrl_params_tag
{
double freq_tune;														//these need to be doubles else float rounding loses lower digit accuracy
double freq_sub_tune;
double freq_start;
double freq_stop;
double freq_step;
float freq_gain;
float dev_bandwidth;
float freq_reso;
float threshold;

bool b_dwn_aa;
int i_dwn_srate;

bool b_if_freq;
float bw_lower;
float bw_upper;
int bw_taps;

int interfreq;
bool b_bw_limit;
int tuning_offset;
float gain_iq;
int direct_sampling;
bool bias_t;
bool b_agc;
float audgain;
bool b_dcblk;

//int rtl_srate_filter;

bool b_clip_enable;
};



enum en_fftw_thread_id_tag												//used by the fftw plan allocation mechanism, ensures 2 thread don't use the same plan, refer: 'complex_fwd_fft_multi()'
{
en_ftid_gui,															//fltk gui thread (main thread)
//en_ftid_audio_proc,														//audio callback thread
//en_ftid_fft_disp_thrd,													//fft display thread
};





enum en_graph_loc_sel_tag												//MUST MATCH 'menu_graph_loc_sel'  'plot_using_vectors_eval()'  'st_plot_probe_tag'
{
en_gls_IQ_timedomain,
en_gls_IQ_spect,
en_gls_sub_tuner_spect,
en_gls_dwncnv_aa_spect,
en_gls_downcnv_timedomain,
en_gls_downcnv_spect,
en_gls_bandpass_fir_timedomain,
en_gls_bandpass_fir_spect,
//en_gls_IQ_spect_downcnv_filter_iffreq_spect_shift,
//en_gls_IQ_spect_noise_reduction,
//en_gls_timedomain,
en_gls_demod_aud_timedomain,
en_gls_demod_aud_dcblock_timedomain,
en_gls_demod_aud_spect,
en_gls_fm_demod_spect_mpx,
en_gls_fm_demod_mpx_halfband_decimator_spect,
en_gls_fm_demod_pilot_pll_err,
en_gls_fm_demod_pilot_rectified_peak,
en_gls_fm_demod_pilot_bounce,
en_gls_fm_demod_pll_38K_osc,

en_gls_fm_demod_pll_osc_compare,

en_gls_fm_demod_left_minus_right_bpf_spect,
en_gls_fm_demod_left_minus_right_timedomain,
en_gls_fm_demod_left_minus_right_spect,
en_gls_fm_ster_demux_aud_ch0_timedomain,
en_gls_fm_ster_demux_aud_ch1_timedomain,
en_gls_fm_ster_demux_aud_ch0_spect,
en_gls_fm_ster_demux_aud_ch1_spect,
//en_gls_timedomain_demod_downcnv,
en_gls_demod_agc_audio_peak_timedomain,
//en_gls_agc_filter,
en_gls_demod_agc_gain_ctrl_timedomain,
en_gls_demod_aud_clip_agc_timedomain,

en_gls_demod_aud_resampler_ch0_timedomain,
en_gls_demod_aud_resampler_ch1_timedomain,
en_gls_demod_aud_resampler_ch0_spect,
en_gls_demod_aud_resampler_ch1_spect,

en_gls_filtresp_antialias_iir,
en_gls_filtresp_demod_bpf_fir,
en_gls_filtresp_demod_lpf0_iir,
en_gls_filtresp_demod_hpf_iir,

en_gls_tdm_probe_vflt0,
en_gls_spect_probe_vflt0,												//fftw fwd 'real to complex'
en_gls_spect_probe_vcpx0,												//fftw fwd 'complex to complex'
};



struct st_plot_probe_tag
{
en_graph_loc_sel_tag which;
unsigned int srate_in;

vector<filter_code::st_cplex_tag> *vcpx0;
vector<filter_code::st_cplex_tag> *vcpx1;								//not used at present
vector<st_spect_tag> *vsp0;												//not used at present
vector<double> *vdble0;													//not used at present
vector<float> *vflt0;
vector<float> *vflt1;													//not used at present

};







struct st_plot_probe_atm_tag
{
en_graph_loc_sel_tag which;
unsigned int srate_in;

vector<filter_code::st_cplex_tag> vcpx0;
vector<filter_code::st_cplex_tag> vcpx1;								//not used at present
vector<st_spect_tag> vsp0;												//not used at present
vector<double> vdble0;													//not used at present
vector<float> vflt0;
vector<float> vflt1;													//not used at present

};





struct st_aud_spect_atm_tag
{
unsigned int srate_in;

vector<float> vflt0;
};






struct st_morse_seq_tag
{
float freq;
float dur;
float amp0;
float amp1;
};




struct st_morse_code_tag
{
string scode;															// 0: dit, 1: dah   e.g  'S' 0,0,0    'F' 0,0,1,0

};






struct st_dev_bwidth_tag 
{
int freq;	
};



struct st_carrier_max_tag
{
float lev;
int freq;
int graph_x_idx;
	
};



struct st_freq_mem_tag
{
int freq_tune;
int freq_sub_tune;
};



struct st_preset_tag
{
string sname;
int freq_tune;
int freq_sub_tune;
en_demodulator_type_tag demod_type;
bool b_dwn_aa;															//downsampler aa filter enable
int i_dwn_srate;
bool b_use_iffreq;
int if_freq;
bool b_bw_limit;
int iffreq_bw_low;
int iffreq_bw_high;
int iffreq_bw_taps;
float dev_gain;
bool b_bias_t;
int direct_sampling;
float iq_gain;
float aud_gain;
bool b_agc;
int i_deemph;															//refer: 'i_deemphasis'
bool b_dcblk;


string tooltip;															//not saved
};




struct st_filter_memory_tag
{
string sname;

bool b_bw_limit;
unsigned int iffreq_bw_taps;
unsigned int iffreq_bw_low;
unsigned int iffreq_bw_high;

};




struct st_dwn_srate_memory_tag
{
string sname;

unsigned int dwn_srate;

};





class My_Input_Choice : public Fl_Input_Choice 
{
private:


public:
bool b_allow_wheel_inc;


private:
int handle(int);

public:
My_Input_Choice(int x,int y,int w, int h,const char *label=0);

};









/*
class mgraph : public Fl_Box
{
private:

int woffx;
int woffy;
int sizex;
int sizey;
int mousex, mousey;
bool double_click_left;

int idx_maxx;
int idx_maxy;


bool show_axisx;
bool show_axisy;
//double val_y_min, val_y_max, val_x_min, val_x_max;

double ref_scalex;						//used to reference other traces wrt to 1st trace
double ref_d_midx;						//used to reference other traces wrt to 1st trace

vector<trace_tag>trce;

public:
int rect_size;
col_tag background;
//int selected_sample;
//int selected_pixel_rect_left;
//int selected_pixel_rect_right;
//int selected_pixel_rect_top;
//int selected_pixel_rect_bot;

int bkgd_border_left;					//use to size background rect
int bkgd_border_top;
int bkgd_border_right;
int bkgd_border_bottom;

int graticule_border_left;				//use to size graticule rect
int graticule_border_top;
int graticule_border_right;
int graticule_border_bottom;
int graticle_count_y;
int graticle_count_x;
int grat_pixels_x;

int grat_pixels_y;
bool cro_graticle;

public:
mgraph( int x, int y, int w, int h, const char *label = 0 );
void clear_traces();
void clear_trace( int idx );
void add_trace( trace_tag &tr );
void render();
void get_selected_value( int trc, double &xx, double &yy );
void get_trace_min_max( int trace, double &xmin, double &xmax, double &ymin, double &ymax );
void set_trace_visibility( int idx, bool visible );
bool get_selected_idx( int trc, int &sel_idx );
bool set_left_click_cb( int trc, void (*p_cb)( void* ), void *args_in );
void get_trace_maxx( int trc, int &idx, double &x );
void get_trace_maxy( int trc, int &idx, double &x );
void set_selected_sample( int trc, int sel_sample_idx, int issue_which_callback );

private:
void draw();
int handle( int );
bool get_mouse_vals( int trc, int mx, int my, double &x, double &y );


};


*/


struct st_filter_sweep_tag
{
int state;																//0: stopped,     1: primed for sweep start,    2: sweeping,     3: sweep finished

int filter_type;														//0: iir  1: fir
filter_code::st_iir *oiir;												//filter object
filter_code::st_fir *ofir;

float srate;
vector<float>vfreq_probe;												//list of freq to test
int freq_probe_idx;

float freq_start;
float freq_stop;
//float freq_step;
//float freq_cur;

float cycles_per_step;													//how many sine cycles (per freq step) to probe filter response with
vector<float>vfreq;														//freqs used during the sweep
vector<float>vampl;														//rectified/averaged amplitude o/p of filter during sweep steps

float osc_amp;
//float osc_theta0;														//current oscillator phase angle
//float osc_theta_inc;

};




struct st_scan_freq_tag
{
string start;
string stop;
};




struct st_wtrfall_col_tag
{
int r;
int g;
int b;
};





//class cl_waterfall : public Fl_Widget
class cl_waterfall : public Fl_Double_Window
{
private:
int rd, wr;

void (*mousemove_p_cb)( cl_waterfall *wdj, void *args );
void *mousemove_cb_args;

void (*left_click_p_cb)( cl_waterfall *wdj, void *args );
void *left_click_cb_args;

void (*mousewheel_p_cb)( cl_waterfall *wdj, void *args, int delta );
void *mousewheel_cb_args;

void (*keydown_p_cb)( cl_waterfall *wdj, void *args, int key );
void *keydown_cb_args;

public:
bool inside_control;
bool ctrl_key;
bool shift_key;
int mousex, mousey;
bool left_button;
bool right_button;
bool middle_button;
int key;

int freq_center;
int bwidth;
int iffreq;

int freq_disp0;
double disp_zoom_factor;

int sizex;
int sizey;
//unsigned char *pixbf0;													//this holds the plot lines drawn at 4 times the req size
unsigned char *pixbf1;													//trial for reducing unnecessary calcs

float lim_low, lim_high;
//float* fbuf0;															//ampl
float gain;
st_wtrfall_col_tag col_ramp[11];
Fl_Double_Window *win_wfall;
string sdate_stamp;

public:
cl_waterfall( int xx, int yy, int wid, int hei, const char *label );
~cl_waterfall();
void set_freq_details( int freq_center_in, int bwidth_in, int iffreq_in, double disp_zoom_factor_in );
void add_line( vector<st_spect_tag> &vsp );
void add_line_to_image( vector<float> &vf );
void make_colour_map();
void clear();

void set_mousemove_cb( void (*p_cb)( cl_waterfall*, void* ), void *args );
void set_left_click_cb( void (*p_cb)( cl_waterfall *, void* ), void *args );
void set_mousewheel_cb( void (*p_cb)( cl_waterfall*, void*, int ), void *args );
void set_keydown_cb( void (*p_cb)( cl_waterfall*, void* ), void *args );


private:
void draw();
void dissolve_ramp( float level, st_wtrfall_col_tag &col );
void dissolve_col( float fader, st_wtrfall_col_tag &col_a, st_wtrfall_col_tag &col_b, st_wtrfall_col_tag &col_fader );
int handle(int e);

};






class aa_wnd : public Fl_Double_Window
{
private:

public:
bool ctrl_key, shift_key;
bool left_button;
bool right_button;
bool middle_button;
int mousex, mousey;
int mousewheel;

aa_canvas *cnvs;
float scale_factor;
st_aa_canvas_polygon_tag poly0, poly1, poly2;
unsigned char *bf0;
unsigned char *bf1;
Fl_JPEG_Image *jpg;
Fl_Box *bx_image0;
Fl_Box *bx_image1;
Fl_RGB_Image *bmp0;
Fl_RGB_Image *bmp1;

Fl_Box *bx_info;

int control_key;

int dbg0;
int dbg1;
int dbg2;
int dbg3;
int dbg4;
int dbg5;

mystr mtimer;

int which_meter;

float aa_freq0, aa_freq1;
float aa_theta0, aa_theta1;
float aa_phse0, aa_phse1;
float aa_sin0;


public:
aa_wnd( int xx, int yy, int wid, int hei, const char *label );
void tick( float dt );


private:
void draw();
int handle( int e );

};








class rtl_graph_wnd : public Fl_Double_Window
{
private:										//private var
bool ctrl_key, shift_key;
bool left_button;
bool right_button;
bool middle_button;
int mousex, mousey;
int mousewheel;
int menu_hei;

vector<string> vtunehist;						//tune history
vector<st_scan_freq_tag> vscanhist;				//scan history

public:
Fl_Menu_Bar *menu_sdr;
bool b_gph0_shrink;
int gph0_posy;									//used to resize when menu is showing
int gph0_hei;

int wfall0_posy;
int wfall0_hei;


int demodul_mode;
double threshold;
double freq_reso;
//double bandwidth;
int last_tune_history_value;
int last_scan_history_start;
int last_scan_history_stop;
int iled_buf_rd_adj;






mgraph *gph0;
string s_gph0_sel_sample;

//GCLed *led_squelch;
GCLed *ld_aud_plus_db;
GCLed *ld_aud_minus_db;

st_dev_bwidth_tag st_dev_bwidth[cn_dev_bwidth_selections_max];

//Fl_Input *fi_freq_bandwidth;
Fl_Input *fi_freq_resolution;
My_Input_Choice *fi_freq_start;
My_Input_Choice *fi_freq_stop;
Fl_Input *fi_freq_step;

My_Input_Wheel *miw_freq_gain;
//Fl_Input *fi_freq_gain;
Fl_Box *bx_freq_gain;
Fl_Input *fi_freq_threshold;
Fl_Input *fi_freq_cur;
Fl_Button *bt_freq_start;
Fl_Button *bt_freq_stop;

Fl_Button *bt_freq_single;
//Fl_Button *bt_freq_tune;
//Fl_Button *bt_freq_tune_off;
Fl_Button *bt_freq_start_marker;
Fl_Button *bt_freq_stop_marker;
Fl_Choice *fi_demodul_mode;
Fl_Check_Button *ck_mono;

Fl_Check_Button *ck_user_dc_block_iq;


Fl_Check_Button *ck_user_hpf0;
My_Input_Wheel *miw_user_hpf0;


Fl_Check_Button *ck_user_dwn_aa;

Fl_Check_Button *ck_user_lpf0;
My_Input_Wheel *miw_user_lpf0;

Fl_Check_Button *ck_user_lpf1;
My_Input_Wheel *miw_user_lpf1;

Fl_Check_Button *ck_user_lpf2;
My_Input_Wheel *miw_user_lpf2;

My_Input_Wheel *miw_filt_lwr;
My_Input_Wheel *miw_filt_upr;

My_Input_Wheel *miw_filt_taps;

Fl_Check_Button *ck_if_freq;



//Fl_Input *fi_ifreq;

//Fl_Input *fi_freq_mouse;
Fl_Input *fi_ampl_mouse;

//Fl_Slider* fvs_vert;
//Fl_Slider* fvs_horiz;
Fl_Slider* fvs_gain;
//Fl_Slider* fvs_squelch;
Fl_Slider* fvs_play_pos;

//My_Input_Choice *fi_tune;
Fl_Input *fi_name;
Fl_Input *fi_group_name;
Fl_Box *bx_tune;
My_Input_Wheel_Packable* miwp_tune;
input_dropbox *idb_tune;
My_Input_Wheel *miw_if_freq;

My_Input_Wheel* miw_tuning_offset;
My_Input_Wheel* miw_ppm_offset_dev;
My_Input_Wheel* miw_gph_scaley;
My_Input_Wheel* miw_gph_disp_spect_zoom_factor;
My_Input_Wheel* miw_spect_avg;

My_Input_Wheel *miw_dev_dwnconv_srate;
My_Input_Wheel* miw_dev_bwidth;
Fl_Menu_Button *mb_dev_bwidth;

My_Input_Wheel* miw_freq_sub_tune;

My_Input_Wheel* miw_carrier_max_tune_threshold;
My_Input_Wheel* miw_gain_iq;

My_Input_Wheel* miw_dbg0;
My_Input_Wheel* miw_dbg1;
My_Input_Wheel* miw_dbg2;
My_Input_Wheel* miw_dbg3;
My_Input_Wheel* miw_dbg4;
My_Input_Wheel* miw_dbg5;

Fl_Box *bx_freq_start;
Fl_Box *bx_freq_stop;
Fl_Box *bx_dev_bwidth;
Fl_Box *bx_buf_rd_wr;
Fl_Box *bx_tops_thread;
Fl_Box *bx_ppm_offset;
Fl_Box *bx_carrier_lev;
Fl_Box *bx_carrier_max_tune_threshold_count;
Fl_Box *bx_dwnconv_factor;

Fl_Box *bx_signal_level;
Fl_Box *bx_freq_actual;

//Fl_Button *ck_show_demod;
Fl_Button *ck_bw_limit;

Fl_Choice *ch_graph_loc_sel;
Fl_Box* bx_led_preset_hov_name;
Fl_Box* bx_gph0_sel_sample_text;

cl_waterfall *wfall0;

GCLed* ld_direct_sampling;
GCLed* ld_bias_t;
GCLed* ld_offset_tuning;
GCLed* ld_buf_rd_adj;
GCLed* ld_clip;
GCLed* ld_agc;
GCLed* ld_carrier_max_tune;
GCLed* ld_rec_play_synth;
GCLed *ld_fm_19k_pilot;
GCLed *ld_fm_deemph;

Fl_Box *bx_direct_sampling;
Fl_Box *bx_offset_tuning;
Fl_Box *bx_samples_per_sec;

aa_wnd *wnd_aa;

GCLed *ld_freq_memory[cn_freq_memory_max];
st_freq_mem_tag freq_memory[cn_freq_memory_max];


GCLed *ld_preset_memory[cn_preset_memory_max];
st_preset_tag preset_memory[cn_preset_memory_max];
bool b_led_preset_memory_tooltip_show_tooltips;

GCLed *ld_preset_memory2[cn_preset_memory_max2];
st_preset_tag preset_memory2[cn_preset_memory_max2];



GCLed *ld_filter_memory[cn_filter_memory_max];
st_filter_memory_tag filter_memory[cn_filter_memory_max];



GCLed *ld_dwn_srate_memory[cn_dwn_srate_memory_max];
st_dwn_srate_memory_tag dwn_srate_memory[cn_dwn_srate_memory_max];

GCLed *ld_aa_dwncnv_fc[cn_aa_dwnsrate_fc_max];

#define cn_btw_band_max 6 
cl_button_wheel *btw_band[cn_btw_band_max];



int b_synth_iq;
en_modulation_type_tag synth_mod_type;									//type of modulation
int b_synth_noise_on_iq;
int b_synth_voice_on_iq0;
int b_synth_voice_on_iq1;
int b_synth_voice_on_iq2;
int b_synth_voice_on_iq3;
int b_synth_voice_on_iq4;
int b_synth_voice_on_iq5;
int b_synth_voice_on_iq6;
float synth_mod_1st_tone_ampl;
float synth_mod_1st_tone_freq;
float synth_mod_2nd_tone_ampl;
float synth_mod_2nd_tone_freq;

int morse_tone_freq;													//pitch of a morse dit/dah in Hz, see 'cn_morse_tone_freq' 
float morse_fade_time;													//time its takes a morse tone to fade up/down, helps stops aliasing by applying a slew rate limit, see 'cn_morse_fade_time'
float morse_dit_time;													//dur of a dit, dah will be 3 times this, see 'cn_morse_dit_time'
float morse_dah_time;													//dur of a dah
float morse_letter_gap_time;											//silence between letter
float morse_word_gap_time;												//silence between words
int morse_play_which;													//used during dongle synthesis, set < 0 to not play, see 'en_meni_synth0'
int morse_idx;
float morse_time_note;													//set this to dur of of the first 'vmorse[]' entry, it will count down to zero
float morse_time_tot;
bool morse_reset;														//set this to force reload selected morse seq
vector<st_morse_code_tag> vmorse_alpha;									//holds more char codes
int led_freq_mem_hov_idx;
int led_preset_hov_idx;
int led_preset_hov_idx2;

//vert_meter *agc_vmeter;
Fl_Slider *fvs_agc;

vert_meter *aud_vmeter;

Fl_Box *bx_keypad_number;
int ineed_graph_fit;													//load this with number of ticks to tigger a call to 'cb_bt_graph_fit()'
bool bneed_graph_fit_bring_for_front = 0;

string skeyin_freq;



private:
int handle( int e );



public:											//public functions
rtl_graph_wnd( int xx, int yy, int wid, int hei, const char *label );
~rtl_graph_wnd();
void load_settings( string ini_fname, bool b_load_last_favourite_settings );
void save_settings( string ini_fname );
void update_gph0( vector<st_spect_tag> &vspct );
void update_sel( int trc, vector<st_spect_tag> &vspct );
void get_trace_min_max( double &xmin, double &xmax, double &ymin, double &ymax );
void get_trace_maxy( int &idx, double &x, double &y );
void set_selected_sample( int trc, int sel_sample_idx, bool issue_callback );
void do_service();
//void update();
void freq_listen( bool history_add, bool bset_dev_gain, bool bset_dev_srate, bool bset_direct_sampling, string scaller );
void freq_listen_using_fav_idx( unsigned int idx );
void add_to_tune_history_skip_duplicates();
//void add_to_tune_history();
void add_to_scan_history();
void reload_scanhist();
void update_controls();
void update_tune_history();
void update_prep_gph0();
void update_prep_gph0_downsample();
void graph_adjust_graticule();
void plot_gph01();
void make_dev_bwidth_list();
void set_dev_bwidth( int freq );
int iq_count_calc( int &iq_count_downsample, int &iq_count_downsample_widefm );
void fftw_create_plans();
void fftw_adjust_plans();
void fftw_destroy_plans();

void carrier_max_sanitize( vector<st_carrier_max_tag> &vv );
void carrier_max_tune();
void carrier_max_exclusion( bool b_add, int freq );
int carrier_max_exclusion_find_idx( int freq );
bool load_favourite_list( string ini_fname );
bool save_favourite_list( string ini_fname );
void favourite_add( bool bstore_in_first_empty_slot );
void set_bias_t( bool bset );
void toggle_bias_t();
bool set_dev_direct_sampling( unsigned int state );
void led_freq_memory_turn_off();
void led_filter_memory_turn_off();
void led_dwn_srate_memory_turn_off();
void led_preset_memory_turn_off();
void led_preset_memory_turn_off2();
void led_preset_memory_tooltip_update( bool bshow_tooltips );
void led_preset_memory_tooltip_update2( bool bshow_tooltips );
void led_preset_memory_label_update2();


void tick( float dt );
void morse_build();
void morse_code_table_build();
void morse_code_add_symbol( vector<st_morse_seq_tag> &vm, char ch, bool b_add_char_silence, bool b_add_word_silence );
void morse_code_add_word( vector<st_morse_seq_tag> &vm, string s_word, bool b_add_word_silence );
void morse_code_add_text( vector<st_morse_seq_tag> &vm, string s_text, bool b_add_word_silence );
bool morse_render_to_audio_file( vector<st_morse_seq_tag> &vm, string fname, int srate_in );

bool do_common_key_processing( bool ctrl, bool shift, int key, int key_pass_b );

void update_gph0_user_obj();
void gph0_obj_build();
void gph0_obj_calc_coords();
int gph0_obj_find_hover();
bool pin_add( int px, int py, int freq_center, int freq_sub );
bool pin_delete( unsigned int idx );
int pin_select_inc_dec( int dir, bool adj_freq );
bool pin_select_by_idx( unsigned int idx, bool adj_freq, bool cntr_onbrd_zero_sub_tune );
void pin_deselect_all();
int pin_selected();
void pin_conform( int freq_cntr );
void centre_graph( int delay_cnt, int hzoom );
void gph0_edge_freqs( int shift_left_px, int shift_right_px, double &freq_left, double &freq_right );
void gph0_make_sel_sample_text();
int text_dim( string ss, int fonttype, int fontsize, int &height, int &descent );

//void carrier_max();

};



#endif

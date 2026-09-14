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


//rtl_graph.cpp
//v1.01		15-Feb-2014			//from previous puppylinux 32 bit network analyser version
//v1.02		12-dec-2022			//upgraded to use 64 bit 'librtlsdr' (-lrtlsdr) library obtained via synaptic, moded to use rtaudio instead of '/dev/dsp'
								//added waterfall display and further features, improved 'demod()' 
//v1.03		21-oct-2023			//improved 'aa_canvas::block_filter_scale_layer()' and 'block_filter_partial_scale_layer()' (not in use at present)
								//added freq/preset memory leds (right click led to store), see 'ld_freq_memory[]' and 'ld_preset_memory[]'
								//added 'b_synth_iq' etc option, and 'morse_play_which' etc option using 'vmorse0', which can also play a music tune
//v1.04		27-may-2025			//moded tune buttons '<<'  '>>' to step at half the amount, e.g: the freq at right most of graph will end up in middle of graph when '>>' is pressed
								//added 'b_use_synthesis_dont_use_rtl_dev' flag, see: 'rtl_scan.cpp'
								//added 'filters_create()' 'filters_destroy()'  'led_preset_memory_tooltip_update()' 'b_led_preset_memory_tooltip_show_tooltips'
								//added 'clip_audio_cnt' 'clip_level_audio'
								//added 'filter_demod_adjust()' filtering is now done in the time domain (after inverse fft), no longer using 'pass_spectrum_hard()' which was causing clicking
								//refer 'usr_demod_bpf_fir_I0' 'g_b_bw_limit'
								//added 'spect_avg()'
								//added 'ssb_fir_lsb_I0' etc (to be completed)
								//added calls to: 'idb_tune->add_at_head()' 'idb_tune->trim_entry_count_to()'  in 'freq_listen()'
								//added keypad for freq entry, refer: 'cb_bt_keypad()'
								//added class 'cl_rotary_knob'
								//grouped some controls
								//added to synth mode, modulation using an audio clip (from a file), it's set on a seperate carrier, refer: 'b_synth_voice_on_iq0' etc, 'bvoice_file' and 'cn_synth_voice_fname0' etc,
								//it will srate convert  'cn_synth_voice_fname0' etc audio clip as req to match audio proc srate, see: 'set_synth_mode_and_menu_state()'
								//also see 'vaudsynth0' etc used to hold a generated tone sweep and a two 2 tone warble
								//added 'tops_thread_stats()' etc to show linux 'top' cmd stats for threads this app uses, needs 'tops_thread.sh' script running in a terminal to generate file: 'cns_tops_thread_text_fname', this file must be generated in this app's directory
								//added further presets using tabbed widget, refer 'tb_preset2'

//v1.05		10-jan-2026			//improved waterfall, no longer recreates every pixel for whole display when a line is added, 
								//now just generates one line of pixels at top (after shifting old pixels down one line), refer: 'pixbf1 'add_line_to_image()'

//v1.06		06-jul-2026			//moved over to 'demod_iso()' to reduce cpu usage, added improved sharp antialias filter before downsampler, 
								//now downsampling to user selectable srate, e.g: 12KHz, selectivity filtering is performed at this rate, resamples demoded audio to system audio srate (e.g: 12K-->48KHz)
								//added rudimentary agc
								//added 'plot_load_vectors(..)' to graph various points in sdr chain in either a tdm or spectral plot, see 'en_graph_loc_sel_tag' which selects what type of plot and which vectors to use
								//added 'b_slow_load_read_voice_files' to turn off: voice file read, resample, hilbert firs, this allows speed up of start up, useful for development of code
								//added fm stereo demodulator, refer 'stereo_pll_process()'
								//added and migrated to 'fftw_clear_all_structs()' 'fftw_build_if_req()' 'fftw_destroy_all_structs()'
								//added audio notcher wnd refer 'wnd_aud_spect' 'cl_aud_spect_wnd' 


//how to stop any dvb modules gaining access to rtl usb dongles (unplug dongles first):
//1) unplug dongles
//2) add text file: '/etc/modprobe.d/blacklist-dvb_usb_rtl28xxu.conf'   (see file contents directly below)
//3) issue a 'rmmod' commands further below


//'blacklist' file: '/etc/modprobe.d/blacklist-dvb_usb_rtl28xxu.conf'

/*------- /etc/modprobe.d/blacklist-dvb_usb_rtl28xxu.conf -------
blacklist dvb_usb_rtl28xxu
blacklist rtl2832
blacklist rtl2830
-----------------------------------------------------------------*/


//sudo rmmod dvb_usb_rtl28xxu											<<<---- enable blacklist first, then do these (dongles must be unplugged)
//sudo rmmod rtl2832
//sudo rmmod rtl2830


//NOTE: use the flag: 'b_use_synthesis_dont_use_rtl_dev'   ------->		flag TO SKIP probing a PHYSICAL rtl device, see also cmdline flag: 'no_rtl=1''       <-------
																		//SEE ALSO 'b_fast_start_no_voice_files'

#include "rtl_graph.h"

bool b_use_synthesis_dont_use_rtl_dev = 0;								//set this TO SKIP using a PHYSICAL rtl device, see also cmdline flag: 'no_rtl=1'
																		//SEE ALSO 'b_fast_start_no_voice_files'


bool b_fast_start_no_voice_files = 0;									//set this to one to speed startup while debugging, avoids voice file reads, srate resampling and hilbert firs, all which are slow to process,
																		//further below are sin/cos synth signals which are used to replace each voice file process chain
																		//refer 'set_synth_mode_and_menu_state()'
																		//SEE ALSO 'b_use_synthesis_dont_use_rtl_dev'



extern rtl_graph_wnd *wnd_rtl_graph;
Fl_Window *synth_dlg = 0;
extern Fl_RGB_Image *img_icon_ntc;
extern Fl_RGB_Image *img_icon_prb;
extern Fl_RGB_Image *img_icon_fav;

extern string csIniFilename;
string slast_filename_ini = "";

cl_favourite_wnd* wnd_fav = 0;
vector<st_favourite_freq_tag>vfav;
cl_aud_spect_wnd* wnd_aud_spect;

audio_formats af0, af1;
st_audio_formats_tag saf0, saf1;

extern bool vb0;


extern int imode;
extern int rtl_scan_count;
extern int rtl_scan_cur_count;
extern int rtl_scan_cur_freq;
extern int rtl_scan_freq_step;
extern int iBorderWidth;
extern int iBorderHeight;

extern rtaud rta;														//v1.02
extern st_rtaud_arg_tag st_rta_arg;										//v1.02
extern float time_per_sample;											//v1.02 
extern bool audio_started;												//v1.02

bool threads_rtl_started = 0;
bool threads_started = 0;

extern mystr tim;
extern mystr tim0;
extern mystr tim1;

mystr m1_skeyin_freq_changed;											//used when user is keying in a freq while mouse is in gph0
mystr m1_skeyin_freq_changed1;
int i_skeyin_restore_cnt = 0;		//one shot timer to allow restore of current on-board tuner freq (into gui ctrl) if no number is entered

extern vector<string> vtunehist2;
extern int ilast_device_index;
extern float rnd();
extern int DoQuit();
extern void filters_create();

extern int start_up_state;

bool rtl_scan( float freq_start, float freq_stop, float freq_step, float gain, float bandwidth, float freq_reso, float threshold );
void rtl_listen( double freq );
void rtl_listen_stop();
void get_user_params_new( st_ctrl_params_tag &prm );
void filter_adjust_fir_bw_lower_upper();
bool rec_play_change_play_position( bool b_absolute_time, int itime );
void demodul_mode_set( unsigned int which );
bool preset_ask_overwrite();
void aud_dimmer();


extern void make_pref_wnd();													//PrefWnd* pref_wnd;
extern int RunShell( string sin );

vector<st_spect_tag> vspect;
vector<st_spect_tag> vspect_avg[cn_spec_avg_slots_max];
bool b_spect_average = 1;
int ispec_avg_idx_wr = 1;
int ispec_avg_idx_rd = 0;
int ispec_avg_wnd = cn_spec_avg_slots_max;					//number of previous samples to average over

vector<st_spect_tag> vspect_displayble;
vector<st_spect_tag> vspect_gph;
vector<filter_code::st_cplex_tag> vcplex_gph;							//this is for the graticule graph obj


gcrtl rtl;								//rtl tuner dongle
bool rtl_fm_running = 0;


void tune_rtl();
void call_rtl_fm();
void call_rtl_fm_kill();

//void start_audio( void* obj );
bool start_audio();						//v1.02
void stop_audio();
void filter1( vector <filter_code::st_cplex_tag> &vbuf,  vector <filter_code::st_cplex_tag> &vfilt );
bool create_hilbert_fir_filters();



bool my_verbose = 1;
bool b_listen = 0;						//will be set if rtl_thrd_cb is to put rtl obj in async mode


gcthrd *thrd_rtl = 0;

//gcthrd *thrd3 = 0;														//not used
//void thrd3_cb( void* args );

//gcthrd *thrd4 = 0;														//not used, for fft display
//void thrd4_cb( void* args );

void cb_bt_freq_start( Fl_Widget *, void * );
//void cb_bt_freq_tune( Fl_Widget *, void * );
//void freq_listen( bool history_add );
void cb_menu_combo( Fl_Widget *, void *);


void start_threads_rtl();
//void start_threads();
void stop_threads();
//static void cb_rtlsdr_callback( unsigned char *buf, uint32_t len, void *arg );
pthread_mutex_t mutex2;
std::mutex mutex3;


void spect_avg( unsigned int avg_wnd );


float frame_time;

unsigned int framecnt = 2048;
unsigned int fftw_size = 4096;
unsigned int fftw_size_demod = 8192;

unsigned int aud_op_srate = 48000;
double max_time_cb_audio_process_out;		//longest time between pcaudio callback calls

int downsample_srate = 12000;								//this is srate used for selectivity filter processing, it can be lower than 'aud_op_srate'
int downsample_srate_pending = downsample_srate;			//used with 'b_need_dwn_srate_change'
bool b_need_dwn_srate_change = 0;							//set 'downsample_srate_pending' with new srate before setting this

int fm_decimator_srate = 240000/2;										//halfband decimatoro/p srate

fftw_complex *fft_in_c0 = 0;						//complex pointers
fftw_complex *fft_out_c0 = 0;

fftw_plan fft_p_fwd_c0 = 0;
fftw_plan fft_p_rvs_c0 = 0;


unsigned int fftw_plan_countc;					//number of fttw complex plans, set in start_threads()
unsigned int fftw_plan_countr;					//number of fttw real plans, set in start_threads()

#define cn_fftw_size_demod_cnt 19				//see 'fftw_size_demod_0'

//unsigned int fftw_size_demod_0 = 49152;			//see start_threads(), see also 'cn_fftw_size_demod_cnt'  SEE 'fftw_adjust_plans()'
//unsigned int fftw_size_demod_1 = 32768;
unsigned int fftw_size_demod_0 = 159744;			//see start_threads(), see also 'cn_fftw_size_demod_cnt', SEE 'fftw_create_plans()' and 'fftw_adjust_plans()' they MODIFY the actual size, so this value is NOT the fftw plan size allocated for this idx
en_fftw_thread_id_tag fftw_thrd_id_0 = en_ftid_gui;					//which thread can use this plan, see also: 'ftw_plan_thrd_id_fwdc[]'

unsigned int fftw_size_demod_1 = 4992;				//SEE 'fftw_adjust_plans()' it MODIFIES the actual size, so this value is NOT the fftw plan size allocated for this idx
en_fftw_thread_id_tag fftw_thrd_id_1 = en_ftid_gui;

unsigned int fftw_size_demod_2 = 204800;			//SEE 'fftw_adjust_plans()' it MODIFIES the actual size, so this value is NOT the fftw plan size allocated for this idx
en_fftw_thread_id_tag fftw_thrd_id_2 = en_ftid_gui;

unsigned int fftw_size_demod_3 = 6400;				//SEE 'fftw_adjust_plans()' it MODIFIES the actual size, so this value is NOT the fftw plan size allocated for this idx
en_fftw_thread_id_tag fftw_thrd_id_3 = en_ftid_gui;

unsigned int fftw_size_demod_4 = 204800;			//SEE 'fftw_adjust_plans()' it MODIFIES the actual size, so this value is NOT the fftw plan size allocated for this idx
en_fftw_thread_id_tag fftw_thrd_id_4 = en_ftid_gui;

unsigned int fftw_size_demod_5 = 6400;				//SEE 'fftw_adjust_plans()' it MODIFIES the actual size, so this value is NOT the fftw plan size allocated for this idx
en_fftw_thread_id_tag fftw_thrd_id_5 = en_ftid_gui;

unsigned int fftw_size_demod_6 = 131072;
en_fftw_thread_id_tag fftw_thrd_id_6 = en_ftid_gui;	//SEE 'fftw_adjust_plans()' it MODIFIES the actual size, so this value is NOT the fftw plan size allocated for this idx

unsigned int fftw_size_demod_7 = 3276;
en_fftw_thread_id_tag fftw_thrd_id_7 = en_ftid_gui;

unsigned int fftw_size_demod_8 = 1024;
en_fftw_thread_id_tag fftw_thrd_id_8 = en_ftid_gui;

unsigned int fftw_size_demod_9 = 106496;
en_fftw_thread_id_tag fftw_thrd_id_9 = en_ftid_gui;

unsigned int fftw_size_demod_10 = 106496*5;
en_fftw_thread_id_tag fftw_thrd_id_10 = en_ftid_gui;

unsigned int fftw_size_demod_11 = 2048;
en_fftw_thread_id_tag fftw_thrd_id_11 = en_ftid_gui;

unsigned int fftw_size_demod_12 = 81920;
en_fftw_thread_id_tag fftw_thrd_id_12 = en_ftid_gui;

unsigned int fftw_size_demod_13 = 2048;
en_fftw_thread_id_tag fftw_thrd_id_13 = en_ftid_gui;

unsigned int fftw_size_demod_14 = 2048*6;
en_fftw_thread_id_tag fftw_thrd_id_14 = en_ftid_gui;

unsigned int fftw_size_demod_15 = 106496;
en_fftw_thread_id_tag fftw_thrd_id_15 = en_ftid_gui;

unsigned int fftw_size_demod_16 = 106496*2;								//refer 'thrd4_cb()'  ----> SEE 'fftw_adjust_plans()' it MODIFIES the actual size, so this value is NOT the fftw plan size allocated for this idx
en_fftw_thread_id_tag fftw_thrd_id_16 = en_ftid_gui;

unsigned int fftw_size_demod_17 = 4096;									//refer 'thrd4_cb()'  ----> SEE 'fftw_adjust_plans()' it MODIFIES the actual size, so this value is NOT the fftw plan size allocated for this idx
en_fftw_thread_id_tag fftw_thrd_id_17 = en_ftid_gui;

unsigned int fftw_size_demod_18 = 106496;								//refer 'thrd4_cb()'  ----> SEE 'fftw_adjust_plans()' it MODIFIES the actual size, so this value is NOT the fftw plan size allocated for this idx
en_fftw_thread_id_tag fftw_thrd_id_18 = en_ftid_gui;



unsigned int fftwsiz[ cn_fftw_size_demod_cnt ];							//loaded in start_threads(), SEE 'fftw_create_plans()' and 'fftw_adjust_plans()'
fftw_complex *fftwic[ cn_fftw_size_demod_cnt ];
fftw_complex *fftwoc[ cn_fftw_size_demod_cnt ];
fftw_plan fftwp_fwdc[ cn_fftw_size_demod_cnt ];
en_fftw_thread_id_tag ftw_plan_thrd_id_fwdc[ cn_fftw_size_demod_cnt ];	//see also: 'fftw_thrd_id_0'

fftw_plan fftwp_rvsc[ cn_fftw_size_demod_cnt ];
en_fftw_thread_id_tag ftw_plan_thrd_id_rvsc[ cn_fftw_size_demod_cnt ];


double *fftwir[ cn_fftw_size_demod_cnt ];
double *fftwor[ cn_fftw_size_demod_cnt ];

fftw_plan fftwp_fwdr[ cn_fftw_size_demod_cnt ];
en_fftw_thread_id_tag ftw_plan_thrd_id_fwdr[ cn_fftw_size_demod_cnt ];

fftw_plan fftwp_rvsr[ cn_fftw_size_demod_cnt ];
en_fftw_thread_id_tag ftw_plan_thrd_id_rvsr[ cn_fftw_size_demod_cnt ];

int i_fftw_trig_plan_create_state = 0;			//state 0: plans are pending creation, NOTE see 'b_need_dwn_srate_change' which triggers a plan adjustment
												//state 1: plans are created, NOTE see 'b_need_dwn_srate_change' which triggers a plan adjustment
												//state 2: plans are created and ready for use, NOTE see 'b_need_dwn_srate_change' which triggers a plan adjustment




//ping pong struct for thread safe passing of plot data, audio thread loads 'st_plot_atm[]', gui thread processes data and plots graph												
int plot_prep_idx = 0;													//0: will fill 'st_plot_atm[0]'   1: 'st_plot_atm[1]'  ping pong scheme for thread safe access
alignas(64) atomic<int> plot_prep_idx_atm{ -1 };						//-1: no 'st_plot_atm' is prepared, 0: 'st_plot_atm[0]' has data avail,  1: 'st_plot_atm[1]' has data avail
alignas(64) st_plot_probe_atm_tag st_plot_atm[2];						//dual struct ping pong scheme for thread safe access 



void plot_using_struct( st_plot_probe_atm_tag &st );
void plot_grph1_gui_thrd();



int aud_spect_idx = 0;													//0: will fill 'st_aud_spect_atm[0]'   1: 'st_aud_spect_atm[1]'  ping pong scheme for thread safe access
alignas(64) atomic<int> aud_spect_idx_atm{ -1 };						//-1: no 'st_aud_spect_atm' is prepared, 0: 'st_aud_spect_atm[0]' has data avail,  1: 'st_aud_spect_atm[1]' has data avail
st_aud_spect_atm_tag st_aud_spect_atm[2];



void btw_band_set_name( unsigned int idx, string &ss );
unsigned int dnwsrate_nearest_factor( unsigned int dwn_srate_in, bool b_set_dwnsrate, bool b_adj_gui_ctrl );








/*
std::atomic<fftw_state_tag> fftw_state_atm{ fftw_state_tag::ready };	//provides thread safe access to fftw plans/arrays and allows a 'fftw_adjust_plans()' mechanisim



//must only be called by gui (main()) thread, it gives exclusive access to fftw items
//'bneed_fftw_adj' ---> set this to trigger a 'fftw_adjust_plans()'
inline bool fftw_access_gui_thrd( bool bneed_fftw_adj )
{
    // --- inject rebuild request (from gui_thrd context) ---
    if( bneed_fftw_adj )
		{
        fftw_state_tag expected = fftw_state_tag::ready;

        fftw_state_atm.compare_exchange_strong(	expected, fftw_state_tag::rebuild_req, std::memory_order_acq_rel );	//if state is 'ready' go into 'rebuild_req'
		}

    // --- FSM handling ---
    fftw_state_tag s = fftw_state_atm.load( std::memory_order_acquire );

    if( s == fftw_state_tag::rebuild_req )								//has either gui or audio thrds flagged an 'fftw_adjust_plans()' is required 
		{
        fftw_state_atm.store( fftw_state_tag::draining, std::memory_order_release );	//flag the audio thrd that it needs to finish fftw acesses ('draining'), and also the audio thrd needs to set state 'rebuilding' when its cleared of fftw use
        return false;
		}

    if( s == fftw_state_tag::rebuilding )								//has audio thrd set state 'rebuilding' as it's clear of fftw access ?
		{
//      rebuild_fftw items;
		wnd_rtl_graph->fftw_adjust_plans();


        fftw_state_atm.store( fftw_state_tag::ready, std::memory_order_release );
        return true;   // now ready after rebuild
		}

    // ready or draining
    return ( s == fftw_state_tag::ready );
}








//must only be called by audio proc thread, it gives exclusive access to fftw items
//'bneed_fftw_adj' ---> set ths to trigger a 'fftw_adjust_plans()'
inline bool fftw_access_audio_thrd( bool bneed_fftw_adj )
{
    // request rebuild (rare)
    if( bneed_fftw_adj )
    {
        fftw_state_tag expected = fftw_state_tag::ready;				

        fftw_state_atm.compare_exchange_strong(
            expected,
            fftw_state_tag::rebuild_req,
            std::memory_order_acq_rel );
    }

    // check state
    fftw_state_tag s = fftw_state_atm.load( std::memory_order_acquire );

    if( s == fftw_state_tag::ready )
		{
        return true;
		}

    if( s == fftw_state_tag::draining )
		{
        // signal gui thread that we are clear
        fftw_state_atm.store( fftw_state_tag::rebuilding, std::memory_order_release );
		}

    return false;
}
*/












int64_t g_freq_tune = 92.9e6;
int64_t g_freq_tune_offset = 0;							//tuned freq including 'g_tuning_offset' and 'g_interfreq'
int g_freq_offsets_only = 0;							//just 'g_tuning_offset' and 'g_interfreq'
int g_tuning_offset = 0;								//user setable param adjust tuner freq and displayed spectrum
//int64_t g_freq_tune_cur = 118500000;
int64_t g_freq_sub_tune = 0;							//this is tuning within the supplied rtl bandwidth, it does not change rtl's onboard tuning

bool g_b_dwn_aa;										//first filter in sdr, just before downsampler, filters to half the downsampler o/p srate

int64_t g_bw_lower;
int64_t g_bw_upper;
bool g_b_if_freq = 0;									//set if to use intermediate freq scheme
int64_t g_interfreq = 10000;
double g_dev_bw = 2964000;
extern double g_dev_bw_last;
//bool g_b_rtl_srate_filter = 0;											//for debug only
//int g_rtl_srate_filter = 10000;

bool g_b_user_iir_lpf0 = 0;
int g_user_iir_lpf0 = 1000;

bool g_b_user_iir_lpf1 = 0;
int g_user_iir_lpf1 = 2000;

bool g_b_user_iir_lpf2 = 0;
int g_user_iir_lpf2 = 3000;


bool g_b_user_iir_hpf0 = 0;
int g_user_iir_hpf0 = 100;

//bool g_b_user_iir_notch0 = 0;											//refer: 'en_filter_idx_tag'
//int g_user_iir_notch_freq0 = 500;
//float g_user_iir_notch_Q_0 = 2.0f;





bool g_b_bw_bpass = 0;
float g_gain_aud = 1.0;
float aud_dim_time = 2.0;						//refer 'cn_aud_dim_time_default' time in secs for audio dim to complete, 0.0 means no audio dimming
int aud_gain_plus_minus_db = 0;											//0: none,  1: +xxdB gain,  2: -xxdB gain										


float g_gain_iq = 1;
float g_dev_gain = 30.0f;
//double g_squelch = 4.0;
float g_aud_lev_avg = 0.0f;

double g_squelch_gain_0 = 0.0;
double g_squelch_gain_5 = 0.05;
double g_squelch_gain_10 = 0.1;
double g_squelch_gain_20 = 0.2;
double g_squelch_gain_50 = 0.5;
double g_squelch_gain_75 = 0.75;
double g_squelch_gain_100 = 1.0;

double g_audio_out_gain = g_squelch_gain_100;
bool g_aud_mute = 0;
bool g_aud_mono = 0;

int g_dev_direct_sampling = 0;											//0: off, 1: I-branch,  2: Q-branch
bool g_dev_bias_t = 0;
bool g_dev_offset_tuning = 0;


float g_carrier_max_tune_threshold = 0.5f;

float g_freq_start;														//see 'do_scan()'
float g_freq_stop;
float g_freq_step;

bool g_fm_19K_pilot_tone = 0;											//fm stereo pilot tone

float carrier_max_lev;
int carrier_max_freq;

int i_carrier_max_freq_dead_band = cn_carrier_max_freq_dead_band;
bool b_carrier_max_tune = 0;
float carrier_max_tune_timer_delay = 2.0f;					//min period between max carrier tuning changes
float carrier_max_tune_timer = 0.0f;
int carrier_max_tune_freq_last;
vector<st_carrier_max_tag> vcarrier_max;
vector<st_carrier_max_tag> vcarrier_max_exclusion;			//items in here won't be considered targets
int carrier_max_led_flash_cnt = 0;							//counts down if a recent tune was done via carrier max 
int rtl_graph_tick_cnt = 0;



bool sql_delay_2 = 0;										//used for delaying squelch unmute by small amount to avoid pre tx noise being unmuted
bool sql_delay_3 = 0;										//high if unmute of audio req


double disp_spect_zoom_factor = 1.0;
double disp_spect_zoom_factor_last = 0;
bool b_need_h_zoom_recalc = 0;								//helps refresh stale hzoom 'idx_left'  'idx_right', e.g. when zero padding pref is toggled



int ineed_hzoom_and_center_to_cur_freq = 0;								//better to use, 'centre_graph( int delay_cnt, int hzoom )' 
																		//set this with a value above '1' to delay its action
double nhac_hzoom_factor = -1.0f;										//set this to -1 to not change hzoom factor





extern float gph_scaley;
extern int gph_loc_sel;
extern int gph_loc_sel_tmp;
extern int gph_samples_per_sec;
double gph_mouse_freq = 0;		//needs double precision as GHz freqs could be in use, it includes the rtl onboard tuner freq as the gph0 x-axis is in those dimensions
double gph_mouse_freq_in_bw = 0;										//this is the mouse freq within dev bandwidth 'g_dev_bw' (+/-g_dev_bw/2)
double gph_mouse_freq_ini = 0;	//this hold the last value 'gph_mouse_freq_in_bw' had when hzoom was changed, it's also saved and loaded from ini file, used to restore hzoom position on startup
int gph_hzoom_mousex;													//mouse x position at last hzoom change
int gph_hzoom_mousex_ini; //this hold the last value 'gph_hzoom_mousex' had when hzoom was changed, it's also saved and loaded from ini file, used to restore hzoom position on startup
//double gph_freq_left = 0;												//left most bin freq on 'gph0', freq is within 'g_dev_bw', see also 'idx_left'



vector<st_freq_pin>vpin;
int i_need_pin_offscreen_check = 0;			//used to delay a check if sel pin is offscreen and needs to be centred (determination reqs the graph to be replotted with sel pin's new 'freq_actual')
											//see 'update_gph0()' where it is processed

vector<st_gph_obj_tag> vgph_obj;
int gph0_obj_hover_idx = -1;
int gph0_obj_sel_idx = -1;

int bf_sz = 65536 * 16;

void low_pass_srconv_spect( vector <st_spect_tag> &vin,  vector <st_spect_tag> &vout, float downsample, float &filter_prev_index, float &now_ampl, float &now_freq );
void get_user_gui_control_params();



int sync_wr_rd_pointer = 0;												//0: normal running
																		//1: rtl async's callback ack by inc to state '2'
																		//2: audio proc callback ack by inc to state '3'
																		//3: actual sync of both wr/rd pointers and move back to state '0'
																		
float *rtl_bfI0 = 0;
float *rtl_bfQ0 = 0;

int rtl_bf0_wr = cn_rtl_buf_size - 1;					//this index stays within 'cn_rtl_buf_size-1' range
int rtl_bf0_rd = 0;										//this index stays within 'cn_rtl_buf_size-1' range

uint64_t idbg_wr = rtl_bf0_wr;							//this index is NOT cyclical wrt 'cn_rtl_buf_size', it is NOT kept within 'cn_rtl_buf_size-1' range, it's allowed to increase indefinitely and wrap only when it passes 'uint64_t' largest poss number
uint64_t idbg_rd = 0;									//this index is NOT cyclical wrt 'cn_rtl_buf_size', it is NOT kept within 'cn_rtl_buf_size-1' range, it's allowed to increase indefinitely and wrap only when it passes 'uint64_t' largest poss number

bool b_slew_adj_fine = 0;								//allow fine adj to the read rate of IQ data, to help keep within a data buf window


void buf_allocate();
void buf_free();
bool b_rtlsdr_callback_first = 0;

bool b_rdwr_rephase = 0;

fast_mgraph gph1;
vector<float> vgph1_x;
vector<float> vgph1_y0;
vector<float> vgph1_y1;
vector<st_mgraph_user_marker_tag> vgph1_vuser_marker;					//useful to give a plot point an index to further information, such as frequency (Hz) the point was generated with
vector<int>vgph1_user_marker_idx;										//index within 'vuser_marker[]'

//vector<float> vgph2_x;
//vector<float> vgph2_y0;
//vector<float> vgph2_y1;


//fast_mgraph gph5;
//vector<float> vgph50_x;
//vector<float> vgph50_y0;
//vector<float> vgph50_y1;


bool b_clip_enable = 1;
float clip_level_audio = 180e-3;										//refer 'cn_clip_audio_limit_low'
int clip_audio_cnt_max = 12;
int clip_audio_cnt = 0;													//provides the clipper led with a brightness level decay  to capture clipper is active


bool b_agc_dev_auto = 1;
float agc_dev_lev = 45;
float agc_dev_lev_limit_low = 4;
float agc_dev_lev_limit_high = 50;
float agc_dev_carrier_goal = 0.5;

float carrier_signal_level = 0.0f;				//received signal level

bool b_agc = 1;
float agc_audio_high_limit = 0.9f;
float agc_audio_low_limit = 0.1f;

//float agc_audio_goal = 0.75f;											//desired audio level to aim for
//float agc_audio_low_goal = 0.25f;										//
float agc_audio_attack_time = 0.2;										//in secs
float agc_audio_decay_time = 0.2;										//in secs
float agc_audio_peak = 0.0f;											//peak detector
float agc_gain_lvl = 1.0f;
float agc_audio_peak_charge_time = 1.0;									//attack towards current level for peak detector, in secs, 0.001 means it takes 1mS to reach a peak audio value of 1.0 from zero
float agc_audio_peak_discharge_time = 0.01;								//decay towards zero for peak detector, in secs, 0.01 means it takes 10mS to decay a peak audio value of 1.0 to zero

float agc_audio_control_val = 0.0;										//this is used to control audio gain out


float agc_audio_attack_factor;											// 'agc_audio_attack_time' / frametime;
float agc_audio_decay_factor;											// 'agc_audio_decay_time' / frametime;


int i_deemphasis = 1;													//0: off    1: 50uS     2: 75uS (USA)


filter_code::st_iir agc_iir0;											//peak detector lpf


//filter_code::st_iir usr_rtl_srate_lpf_iir_I0;							//debug user adj filter, is applied straight after rtl data is read, running at rtl samplerate, too taxing on cpu
//filter_code::st_iir usr_rtl_srate_lpf_iir_Q0;

filter_code::st_iir usr_hpf_iir0_I0;									//user adj filter
filter_code::st_iir usr_hpf_iir0_Q0;

filter_code::st_iir usr_lpf_iir0_I0;									//user adj filter
filter_code::st_iir usr_lpf_iir0_Q0;

filter_code::st_iir usr_lpf_iir1_I0;									//user adj filter
filter_code::st_iir usr_lpf_iir1_Q0;

filter_code::st_iir usr_lpf_iir2_I0;									//user adj filter
filter_code::st_iir usr_lpf_iir2_Q0;

//filter_code::st_iir usr_notch_iir0;										//user adj filter, refer 'g_user_iir_notch_freq0'

filter_code::st_iir usr_demod_hpf_iir_I0;								//not used at present						
filter_code::st_iir usr_demod_hpf_iir_Q0;									
filter_code::st_iir usr_demod_hpf_iir_I1;									
filter_code::st_iir usr_demod_hpf_iir_Q1;									



int g_bw_taps = 300;

filter_code::st_fir usr_demod_bpf_fir_I0;								//will be removed
filter_code::st_fir usr_demod_bpf_fir_Q0;

filter_code::st_fir ssb_fir_lsb_I0;
filter_code::st_fir ssb_fir_lsb_Q0;


filter_code::st_iir wfall_iir0;											//waterfall lpf


filter_code::st_fir fir_hilbert_45_plus;								//holds hilbert +45 deg filter
filter_code::st_fir fir_hilbert_45_minus;								//holds hilbert -45 deg filter


filter_code::st_fir fm_ster_fir_15k_lpf0;								//fm stereo L+R
filter_code::st_fir fm_ster_fir_23_53k_bpf;								//fm stereo L-R before 38KHz demux
filter_code::st_fir fm_ster_fir_15k_lpf1;								//fm stereo L-R (after bpf and 38KHz demux)

filter_code::st_iir fm_ster_iir_19k_notch;								//fm stereo pilot tone notch filter

bool b_need_filter_rebuild = 0;

iir_sos::st_iir_sos_tag fm_stereo_dwnsmpl_aa;
//cl_halfband_decimator fm_stereo_decimator;							//used for fm stereo demod
halfband_poly_optimised_float::st_hbpo_tag fm_stereo_decimator;


//#define cn_agc_avg_delay_max 8
//int agc_avg_delay_cnt = cn_agc_avg_delay_max;							//actual number of delayed sample to use for averaging

//float agc_avg[cn_agc_avg_delay_max];
//int agc_avg_wr = 0;													//write pointer index


bool bvoice_file = 1;
vector<float> vaudclip0;												//holds sample rate converted audio clip samples
vector<filter_code::st_cplex_tag>vc_audclip0;							//hilbert phase processed, used for uprSB, LwrSB

vector<float> vaudclip1;
vector<filter_code::st_cplex_tag>vc_audclip1;							//hilbert phase processed, used for uprSB, LwrSB

vector<float> vaudclip2;
vector<filter_code::st_cplex_tag>vc_audclip2;							//hilbert phase processed, used for uprSB, LwrSB

vector<float> vaudclip3;
vector<filter_code::st_cplex_tag>vc_audclip3;							//hilbert phase processed, used for uprSB, LwrSB

vector<float> vaudclip4;
vector<filter_code::st_cplex_tag>vc_audclip4;							//hilbert phase processed, used for uprSB, LwrSB

vector<float> vaudsynth0;												//tones stored in a vector
vector<filter_code::st_cplex_tag>vc_audsynth0;

vector<float> vaudsynth1;
vector<filter_code::st_cplex_tag>vc_audsynth1;


bool filter_sweep( unsigned int idx, unsigned int num_of_steps_to_probe );

bool filter_iir0_adjust( float freq_cutoff0, float freq_cutoff1 );
bool filter_iir1_adjust( float freq_cutoff );
bool filter_iir2_adjust( float freq_cutoff );
bool filter_iir_notch_adjust( en_filter_idx_tag filt_idx, float freq_cutoff,  float Q_in );
void filter_iir_rebuild();
void filter_fir_rebuild();

void zero_pad_fft();

bool fft_disp_state = 0;
vector <filter_code::st_cplex_tag> viq_local_fft_disp;


st_filter_sdr_tag st_filt[ cn_filters_max ];							//holds all filters
st_filt_bode_tag st_bode[ cn_filters_max ];






//bool b_taper_wnd_iq = 1;												//applies a window function on iq to reduction spectral leakage in fft ops
bool b_dc_block_iq = 1;													//will use a high pass filter on iq to hide dc spike

bool b_wfall_enable = 1;
float wfall_rate = 1.0f;												//1.0f is max rate
float wfall_rate_cnt = 0.0f;



bool b_build_taper_window0 = 1;
vector<float> vtaper_window0;											//used to reduce spectral leakage when performing an fft


int filt_prev_idx0 = 0;								//used for 'low_pass_srconv()' calls
float now_r0 = 0;
float now_j0 = 0;


//int filt_prev_idx1 = 0;							//used for 'low_pass_srconv()' calls
//float filt_prev_float_idx1 = 0;					//used for 'low_pass_srconv()' calls
//float now_r1 = 0;
//float now_j1 = 0;


int filt_prev_idx2 = 0;								//used for 'low_pass_srconv()' calls
//float filt_prev_float_idx2 = 0;					//used for 'low_pass_srconv()' calls
float now_r2 = 0;
float now_j2 = 0;


int filt_prev_idx3 = 0;								//used for 'low_pass_srconv()' calls
float now_r3 = 0;
float now_j3 = 0;



int filt_prev_idx10 = 0;							//used for 'zero_pad_fft()' calls
float now_r10 = 0;
float now_j10 = 0;


int filt_prev_idx11 = 0;							//used for 'thrd4_cb()' calls
float now_r11 = 0;
float now_j11 = 0;



int filt_prev_idx12 = 0;							//used for 'update_prep_gph0()' calls
float now_r12 = 0;
float now_j12 = 0;


int filt_prev_idx14 = 0;							//used for 'aud_spect_gui_thrd()' calls
float now_r14 = 0;
float now_j14 = 0;



int spect_dnwnsamp_prev_idx0 = 0;							//used for 'thrd4_cb()' calls
float now_ampl0 = 0;
float now_freq0 = 0;




//bool b_build_taper_window = 1;
//vector<float> vtaper_window;											//for debug, used to reduce spectral leakage when performing an fft

int pref_taper_windowing_for_fft = 1;									//applies a window function to I/Q to reduction spectral leakage in fft ops

int pref_zero_padding_for_fft = 0;										//refer 'pref_zero_padding_for_fft_cnt' 'cn_pref_zero_padding_for_fft_cnt_max'
int pref_zero_padding_for_fft_cnt = 2;									//e.g: if equal to 3, will add 3 additional blocks of zeros to sampled data before fft is performed, increase fft bin freq resolution, increases cpu usage 
																		//refer 'pref_zero_padding_for_fft_cnt' 'cn_pref_zero_padding_for_fft_cnt_max'
																		
																		
//int pref_zero_padding_for_fft_last = 1;								//used to detect padding pref has been changed, so a 'i_fftw_trig_plan_create_state' trigger will be req
int pref_wfall_non_linear_gain = 1;										//helps bring up detail in the noise by increasing gain of low level signals (for waterfall display)
int pref_freq_mousewheel_low_digit_zero_cnt = 1;						//e.g: '2' would zero last to digit, so mousewheel would adjust in 100Hz steps
int pref_freq_left_click_drag_low_digit_zero_cnt = 3;					//e.g: '2' would zero last to digit, so mousewheel would adjust in 100Hz steps
int pref_freq_right_click_drag_low_digit_zero_cnt = 3;					//e.g: '2' would zero last to digit, so mousewheel would adjust in 100Hz steps
int pref_show_dc_for_fft_graphs = 0;									//if 0 will zero DC bin's value

//int pref_show_graph_onboard_tuner_freq = 1;
int pref_show_graph_hz_units = 1;
int pref_show_graph_hz_eng = 1;
int pref_freq_mouse_hov = 1;

double pref_db_ref_A = 20.0;
double pref_db_ref_B = 1.0;
double pref_db_ref_C = 0.0;
string pref_db_ref_suffix = "dB";

bool pref_ask_preset_overwrite = 1;

string pref_scol_gph0_trace_col;
int col_gph0_trace_col_r = 224;
int col_gph0_trace_col_g = 255;
int col_gph0_trace_col_b = 212;

string pref_scol_xaxis_text_freq;
int col_xaxis_text_freq_r = 79;
int col_xaxis_text_freq_g = 170;
int col_xaxis_text_freq_b = 255;


string s_tops_thread_stats;												//refer 'tops_thread_stats()'
int tops_thread_stats_cnt = 0;
float tops_thread_stats_avg_sum = 0;									//sum's all valid stats for avg
float tops_thread_stats_avg = 0;										//current average:  'tops_thread_stats_avg_sum'/'tops_thread_stats_cnt'


bool b_dbg_skip_demod = 0;												//used for processor utilisation debugging
bool b_dbg_skip_update_prep_gph0 = 0;
bool b_dbg_skip_update_gph0 = 0;

extern bool b_dbg_no_graph_update;
extern bool b_fit_auto_plot;

extern void init_osc_integrators( bool calc_initial_freq, bool phase_zero_IQ );


extern float synth_carr_freq0;
extern int synth_carr_lsb_usb_fm_0;
My_Input_Wheel *miw_synth_dlg_carr_freq0;
extern float synth_tone_freq0;
My_Input_Wheel *miw_synth_dlg_tone_freq0;
extern int synth_morse_tone0;												//0: morse short, 1: morse long, 2: tone
My_Input_Wheel *miw_synth_dlg_pilot_freq0;
extern float synth_pilot_freq0;
My_Input_Wheel *miw_synth_dlg_pilot_gain0;
extern float synth_pilot_gain0;
My_Input_Wheel *miw_synth_dlg_dcoffs0;
extern float synth_tone_dcoffs0;

extern float synth_carr_freq1;
extern int synth_carr_lsb_usb_fm_1;
My_Input_Wheel *miw_synth_dlg_carr_freq1;
extern float synth_tone_freq1;
My_Input_Wheel *miw_synth_dlg_tone_freq1;
extern int synth_morse_tone1;												//0: morse short, 1: morse long, 2: tone

extern float synth_carr_freq2;
extern int synth_carr_lsb_usb_fm_2;
My_Input_Wheel *miw_synth_dlg_carr_freq2;
extern float synth_tone_freq2;
My_Input_Wheel *miw_synth_dlg_tone_freq2;
extern int synth_morse_tone2;												//0: morse short, 1: morse long, 2: tone

extern float synth_carr_freq3;
extern int synth_carr_lsb_usb_fm_3;
My_Input_Wheel *miw_synth_dlg_carr_freq3;
extern float synth_tone_freq3;
My_Input_Wheel *miw_synth_dlg_tone_freq3;
extern int synth_morse_tone3;												//0: morse short, 1: morse long, 2: tone

extern float synth_carr_freq4;
extern int synth_carr_lsb_usb_fm_4;
My_Input_Wheel *miw_synth_dlg_carr_freq4;
extern float synth_tone_freq4;
My_Input_Wheel *miw_synth_dlg_tone_freq4;
extern int synth_morse_tone4;												//0: morse short, 1: morse long, 2: tone


My_Input_Wheel *miw_synth_dlg_noise_gain;
extern float synth_noise_gain;

extern atomic<int> bf1_prep_idx_atm;
extern filter_code::st_cplex_tag bf1_iq[2][cn_bf1_iq_sz];
extern atomic<size_t> bf1_cnt_atm;

stereo_pll_state st_fm_decoder;

extern st_aud_notch_gui_ctrls_tag st_aud_notch_gui_ctrls[  cn_aud_spect_notch_max  ];


/*
float agc_audio_calc_test()
{
agc_audio_attack_factor = agc_audio_attack_time / frame_time;
agc_audio_decay_factor = agc_audio_attack_time / frame_time;

//agc_audio_average = 0;

float aud = 0.0;
//int ptr = agc_avg_wr;


float peak_discharg_factor = agc_audio_peak_discharge_time / frame_time;

if( fabs( aud ) > agc_audio_peak ) agc_audio_peak = fabs( aud );

agc_audio_peak -= peak_discharg_factor;
if( agc_audio_peak < 0.0f ) agc_audio_peak = 0.0f;





//for( int i = 0; i < agc_avg_delay_cnt; i++ )
//	{
	
//	agc_audio_average += fabsf( agc_avg[ptr] );							//rectify ac signal to obtain an envelope
	
//	ptr++;
//	if( ptr >= agc_avg_delay_cnt ) ptr -= agc_avg_delay_cnt;			//wrap
//	}

//agc_audio_average /= agc_avg_delay_cnt;

//agc_avg[ptr] = aud;														//save for next frame



if( agc_audio_attack_factor > 1.0f )
	{
	agc_audio_attack_factor = 1.0f;
	}
	
if( agc_audio_attack_factor < 0.1f )
	{
	agc_audio_attack_factor = 0.1f;
	}


if( agc_audio_decay_factor > 1.0f )
	{
	agc_audio_decay_factor = 1.0f;
	}

if( agc_audio_decay_factor < 1.0f )
	{
	agc_audio_decay_factor = 0.1f;
	}




if( agc_audio_peak > agc_audio_goal )
	{
	agc_audio_peak *= agc_audio_decay_time;
	}

if( agc_audio_peak < agc_audio_goal )
	{
	agc_audio_control_val *= agc_audio_attack_time;
	}
	
if( agc_audio_control_val > agc_audio_high_limit) 
	{
	agc_audio_control_val =  agc_audio_high_limit;
	}

if( agc_audio_control_val < agc_audio_low_limit) 
	{
	agc_audio_control_val =  agc_audio_low_limit;
	}
	
if( agc_audio_control_val > agc_audio_high_limit )
	{
	agc_audio_control_val = agc_audio_high_limit;	
	}

return agc_audio_control_val;
}
*/





int g_dbg0 = 0;
int g_dbg1 = 0;
int g_dbg2 = 6.0;
int g_dbg3 = 0;
int g_dbg4 = 0;
int g_dbg5 = 0;


//IF YOU ADD to this also modify 'en_demodulator_type_tag', ALSO MOD 'rtl_graph_wnd::demod_type_idx_from_str()' MOD ALSO 'pulldown_demod_mode'
char sz_demodulator_type[16][32];				//THIS IS LOADED is loaded in 'rtl_graph_wnd::rtl_graph_wnd()'
int demod_type_idx_from_str( string ss );

string slast_favourite_fname;

//char szdev_name[256];
//char szdev_manufact[256];
//char szdev_product[256];
//char szdev_serial[256];

string s_dev_name;
string s_dev_manufact;
string s_dev_product;
string s_dev_serial;

bool b_sanitise_favourites = 1;

bool b_plot_gph1 = 0;													//refer 'plot_gph01()'

//bool b_plot_gph2 = 0;
//int plot_gph_type = 0;												//0: spectral domain    1: time domain
int plot_gph_trace_cnt = 1;												//for time domain: num of traces to plot

int gph1_b_x_axis_values_derived = 0;									//set x axis numeric values to be used for axis labelling (if no 'x' vector is available)
float gph1_x_axis_values_derived_left_value = 0;
float gph1_x_axis_values_derived_inc_value = 0;


vector<st_morse_seq_tag> vmorse0;										//holds a morse code seq
vector<st_morse_seq_tag> vmorse1;										//holds a morse code seq
vector<st_morse_seq_tag> vmorse2;										//holds a morse code seq
vector<st_morse_seq_tag> vmorse10;										//holds a music tune

bool b_wnd_fav_is_open = 0;



bool b_rec_iq = 0;
bool b_play_iq = 0;
int rec_play_iq_state = 0;												//0: none (file closed),
																		//1: flag to start record seq, open file, alloc buf
																		//2: opened rec file, alloc'd buf
																		//3: recording into buf (audio proc) and writing iq file ('cb_timer1()' - file open) 
																		//4: flag to stop record seq
																		//5: not recording into buf, but still writing file ('cb_timer1()' - file open)
																		//6: file closed, freed buf --> goto state '0'
																	
																		//11: flag to start play seq, 
																		//12: opened play file, alloc'd buf
																		//13: playing iq samples from buf (audio proc), reading file into buf ('cb_timer1()' - file open)
																		//14: flag to stop play seq
																		//15: stopped playing iq samples from buf (audio proc), reading file into buf ('cb_timer1()' - file open)
																		//16: file closed, freed buf --> goto state '0'
																	
string rec_iq_fname = "zzrec00.iq";
string play_iq_fname = "zzrec00.iq";
FILE *fp_rec = 0;														//used in file ops for IQ rec and play
float *recply_bf;
unsigned long long int play_filesize;
int64_t rec_wr_cnt = 0;													//incs by 2, for an I/Q pair (I and Q are stored as floats), refer 'rec_play_iq_state_process()'
int64_t rec_rd_cnt = 0;													//incs by 2, for an I/Q pair (I and Q are stored as floats), refer 'rec_play_iq_state_process()'
//unsigned int rec_bf_sz = cn_rtl_sample_bandwith_max*2*sizeof(float)*5;	//buffers 5 seconds of I/Q pair values at highest rtl samplerate, while awaiting a file write to complete (I and Q are stored as floats)
unsigned int rec_bf_sz = 1920000*2*sizeof(float)*1;	//buffers 5 seconds of I/Q pair values at highest rtl samplerate, while awaiting a file write to complete (I and Q are stored as floats)
float rec_play_time = 0;												//cur rec or play time
float play_mode_end_secs = 0;
float rec_mode_end_secs = 240;											//time in secs to end recording state
int64_t rec_wr = 0;														//index while recording into 'recply_bf[]', also used as index to 'recply_bf[]' while doing an 'fread()' for play, refer 'rec_play_iq_state_process()'
int64_t rec_rd = 0;														//index while reading playing from 'recply_bf[]', refer 'rec_play_iq_state_process()'														
//bool b_rec_play_flush = 0;												//used when user changes play position

bool b_rec_play_start_pos_req = 0;		//used used to jump to a different play point in file, by stopping play, closing file, opening file and 'fseeko()' to play point, then playing file as in 'state_in' = 2, refer 'rec_play_iq_set_state()'
int64_t rec_play_start_pos = 0;//1920000*2*sizeof(float)*30;				//allow start play beyond zero position

//bool b_rec_change_play_pos = 0;
//int64_t rec_change_play_pos_to = 0;										//play pos to change to (i.e. change to 'rec_rd' index ), use 'b_rec_change_play_pos' to trigger change


vector<st_filter_sweep_tag> vfilt_sweep;								//filter bode calcs



bool b_keypad_clear = 1;												//provides auto clear if prev entry actually changed the freq, 'cb_bt_keypad()'


float dc_block0_dly0 = 0;												//used by vpcm[]
float dc_block0_dly1 = 0;

float dc_block1_dly0 = 0;												//used for IQ dc spike removal
float dc_block1_dly1 = 0;

float dc_block2_dly0 = 0;												//used for IQ dc spike removal
float dc_block2_dly1 = 0;


vector <filter_code::st_cplex_tag> vsynth_iq0;
//vector <filter_code::st_cplex_tag> vsynth_iq0;



st_ctrl_params_tag g_params;






//state_in = 0: start rec
//state_in = 1: stop rec
//state_in = 2: start play
//state_in = 3: stop play
//state_in = 4: stop play ---> start play		used to jump to a different play point in file via 'tick()', will stop play, close file, open file and 'fseeko()' to play point, then play file as in 'state_in' = 2
//returns 1 normally, returns 0 if user cancels a 'RecordOver' question
bool rec_play_iq_set_state( int state_in )
{
string s1;

if( state_in == 0 )
	{
	if( rec_play_iq_state == 0 )
		{
		mystr m1;
		
		unsigned long long int filesz;
		bool bwrite = 1;
		
		if ( m1.filesize( rec_iq_fname.c_str(), filesz ) )
			{
			strpf( s1, "File already exists, Record over it? : '%s'", rec_iq_fname.c_str() );
			int ret = fl_choice( s1.c_str(),"Cancel","RecordOver", 0 );
			if( ret == 0 )
				{
				bwrite = 0;
				return 0;
				}
			}

		if( bwrite )
			{
			rec_play_time = 0;
			
			rec_play_iq_state = 1;
			return 1;
			}
		}
	}
	

if( state_in == 1 )
	{
	if( rec_play_iq_state == 3 )
		{
		rec_play_iq_state = 4;
		b_rec_iq = 0;
		return 1;
		}
	}


if( state_in == 2 )
	{
	if( rec_play_iq_state == 0 )
		{
		rec_play_time = 0;
		
		rec_play_iq_state = 11;
		b_rec_play_start_pos_req = 0;
		return 1;
		}
	}


if( state_in == 3 )
	{
	if( rec_play_iq_state == 13 )
		{
		rec_play_iq_state = 14;
		b_play_iq = 0;
		return 1;
		}
	}


if( state_in == 4 )
	{
	if( rec_play_iq_state == 13 )
		{
		rec_play_iq_state = 14;
		b_rec_play_start_pos_req = 1;									//flag here, will be processed by 'tick()'
		return 1;
		}
	}

return 1;
}








void rec_play_iq_state_process()
{
int64_t dif = 0; 
mystr m1;
bool vb = 0;

if(vb)printf("rec_play_iq_state_process()0 -  rec_play_iq_state %d  time %.3f, wr %" PRIi64 " %" PRIi64 " %" PRIi64 " cnts %" PRIi64 " %" PRIi64 "\n", rec_play_iq_state, rec_play_time, rec_wr, rec_rd, dif, rec_wr_cnt, rec_rd_cnt );

if( rec_play_iq_state == 0 )
	{
	if( recply_bf != 0 )
		{
		delete[] recply_bf;
		recply_bf = 0;
		
		}
	goto done_it;
	}


//----
if( rec_play_iq_state == 1 )
	{
	
	if( recply_bf != 0 ) delete[] recply_bf;
	
	recply_bf = new float[rec_bf_sz];

	rec_wr = 0;
	rec_rd = 0;

	rec_wr_cnt = 0;
	rec_rd_cnt = 0;
	
	if( fp_rec != 0 ) fclose( fp_rec );
	
	fp_rec = fopen( rec_iq_fname.c_str(), "wb" );
	
	if( fp_rec == 0 )
		{
		printf( "rec_play_iq_state_process() - failed to create rec file: '%s'\n",  rec_iq_fname.c_str() );
		rec_play_iq_state = 0;
		return;
		}
	
	rec_play_iq_state = 2;
	goto done_it;
	}


//write to file buf
if( ( rec_play_iq_state == 3 ) || ( rec_play_iq_state == 4 ) || ( rec_play_iq_state == 5 ) )
	{

	dif = rec_wr - rec_rd;
	if( dif >= rec_bf_sz )
		{
		dif -= rec_bf_sz;
		}

	if( dif < 0 )
		{
		dif += rec_bf_sz;
		}


//printf( "dif: %d\n", (int)dif );

	
	float *bf = new float[ dif ];
	if( bf == 0 )
		{
		printf( "rec_play_iq_state_process() - failed to alloc fwrite buf 'bf' of size  %" PRIi64 "\n",  dif );
		}
	else{
	
		int64_t ui2 = rec_rd;
		for( int i = 0; i < dif; i++ )									//copy data to a new linear buf incase it wrapped around in 'rec_bf'
			{
			bf[i] = recply_bf[ ui2++ ];
			if( ui2 >= rec_bf_sz ) ui2 -= rec_bf_sz;
			}		
		int wrote = fwrite( bf, 1, dif*sizeof(float), fp_rec );

		delete[] bf;
		}

	rec_rd += dif;
	
	rec_rd_cnt += dif;
	
	if( rec_rd >= rec_bf_sz ) rec_rd -= rec_bf_sz;

//	goto done_it;
	//MUST be ALLOWED to reach 'if( rec_play_iq_state == 5 )' just below in this call, hence no 'goto' used here
	}


if( rec_play_iq_state == 5 )
	{
	if( recply_bf != 0 ) delete[] recply_bf;
	recply_bf = 0;
	
	if( fp_rec != 0 ) fclose( fp_rec );
	fp_rec = 0;
	
	rec_play_iq_state = 6;
	goto done_it;
	}


if( rec_play_iq_state == 6 )
	{
	rec_play_iq_state = 0;
	b_rec_iq = 0;
	goto done_it;
	}
//----


//---- read from file buf
if( rec_play_iq_state == 11 )
	{
	if( recply_bf != 0 ) delete[] recply_bf;
	
	recply_bf = new float[rec_bf_sz];

	rec_wr = 0;
	rec_rd = 0;

	rec_wr_cnt = 0;
	rec_rd_cnt = 0;
	
	if( fp_rec != 0 ) fclose( fp_rec );

	play_mode_end_secs = 0;

	if ( m1.filesize( play_iq_fname.c_str(), play_filesize ) == 0 )
		{
		rec_play_iq_state = 0;
		printf( "rec_play_iq_state_process() - play file does not exist: '%s'\n",  play_iq_fname.c_str() );
		return;	
		}


	if( play_filesize == 0 )
		{
		rec_play_iq_state = 0;
		printf( "rec_play_iq_state_process() - play file size is zero: '%s'\n",  play_iq_fname.c_str() );
		return;	
		}
		

	fp_rec = fopen( play_iq_fname.c_str(), "rb" );
	
	if( fp_rec == 0 )
		{
		printf( "rec_play_iq_state_process() - failed to open play file: '%s'\n",  play_iq_fname.c_str() );
		rec_play_iq_state = 0;
		return;
		}

	play_mode_end_secs = play_filesize / (g_dev_bw*sizeof(float)*2 );				//*2 as the data has I/Q pairs		

	//repeat fread calls till enough data in buf, even if there is need to rewind file position
	int need_cnt = rec_bf_sz;//play_filesize;
		
	int read_cnt = 0;
	float *bf = new float[ rec_bf_sz ];
	if( bf == 0 )
		{
		printf( "rec_play_iq_state_process() - failed to alloc fread buf 'bf' of size  %d\n",  rec_bf_sz );
		}
	else{
		while( 1 )
			{
			if( rec_play_start_pos != 0 )
				{
//			fpos_t pos;
//			fgetpos( fp_rec, &pos );

//			off_t position = ftello( fp_rec );
			
//printf( "cb_ld_rec_play_synth_event() -  fseek to %" PRIi64 " siz %" PRIu64 "\n", new_rd, (uint64_t)position );
				fseeko( fp_rec, rec_play_start_pos, SEEK_SET );
				}
			int cnt = need_cnt - read_cnt;
			int read = fread( bf, 1, cnt*sizeof(float), fp_rec );		//do an initial read

			read = read/sizeof(float);
			
//printf( "1st dif %d    cnt: %d  read %d\n", (int) dif, cnt, read );
			for( int i = 0; i < read; i++ )								//copy data to 'rec_bf'
				{
				recply_bf[ rec_wr++ ] = bf[i];
				if( rec_wr >= rec_bf_sz ) rec_wr -= rec_bf_sz;
	
				rec_wr_cnt++;
				}

			read_cnt += read;
			printf( "rec_play_iq_state_process() - initial read %d bytes, needed %d, file: '%s'\n",  read, need_cnt, play_iq_fname.c_str() );

						
//			if( read_cnt < need_cnt )
			if( read < cnt )
				{
				fseek( fp_rec, 0, SEEK_SET );								//rewind to beginning of file and continue 
				printf( "rec_play_iq_state_process()0 - rewind of file pos\n" );
				}
			else{
				break;
				}
			}
		}
	
	delete[] bf;
//printf( "read_cnt so far %d\n", (int) read_cnt );
	
	rec_play_iq_state = 12;
	goto done_it;
	}



if( ( rec_play_iq_state == 13 ) || ( rec_play_iq_state == 14 ) || ( rec_play_iq_state == 15 ) )
	{
	dif = rec_wr_cnt - rec_rd_cnt;	

//	dif = rec_wr - rec_rd;
//	if( dif >= rec_bf_sz )
//		{
//		dif -= rec_bf_sz;
//		}

//	if( dif < 0 )
//		{
//		dif += rec_bf_sz;
//		}


	off_t position = ftello( fp_rec );
	
	rec_play_time = position / (2*sizeof(float)*g_dev_bw);				//update play time for gui display


//	if( b_rec_play_flush ) 
//		{
//		dif = 0;														//this will trigger an fread() below
//		b_rec_play_flush = 0;
//		}


	if( dif < rec_bf_sz / 2 )											//less than half of buf data left ?
		{
//		dif += rec_bf_sz;

		int siz = rec_bf_sz / 2;
		
		float *bf = new float[ siz ];
		if( bf == 0 )
			{
			printf( "rec_play_iq_state_process() - failed to alloc fread buf 'bf' of size  %d\n",  siz );
			}
		else{
			int need_cnt = rec_bf_sz / 2 - dif;
			int read_cnt = 0;
			while( 1 )
				{
				int cnt = need_cnt - read_cnt;
				if( cnt <= 0 ) break;
				
				int read = fread( bf, 1, cnt*sizeof(float), fp_rec );
//				printf( "rec_play_iq_state_process() JJJJJJJJJJJJJJJJJJJJ - read %d bytes, needed %d, file: '%s'\n",  read, need_cnt, play_iq_fname.c_str() );

				read = read/sizeof(float);

				read_cnt += read;
				
//	printf( "dif %d    cnt: %d  read %d\n", (int) dif, cnt, read );
				for( int i = 0; i < read; i++ )								//copy data to 'rec_bf' with wrap around
					{
					recply_bf[ rec_wr++ ] = bf[i];
					if( rec_wr >= rec_bf_sz ) rec_wr -= rec_bf_sz;

					rec_wr_cnt++;
					}


	//			if( read_cnt < need_cnt )
				if( read < cnt )
					{
					fseek( fp_rec, 0, SEEK_SET );							//rewind to beginning of file and continue
					printf( "rec_play_iq_state_process()1 - rewind of file pos\n" );
					}
				else{
					break;
					}
				}	

			delete[] bf;
			}
		}
	}



	
//	goto done_it;
	//MUST be ALLOWED to reach 'if( rec_play_iq_state == 15 )' just below in this call, hence no 'goto' used here


if( rec_play_iq_state == 15 )
	{
	if( recply_bf != 0 ) delete[] recply_bf;
	recply_bf = 0;
	
	if( fp_rec != 0 ) fclose( fp_rec );
	fp_rec = 0;
	
	rec_play_iq_state = 16;
	goto done_it;
	}


if( rec_play_iq_state == 16 )
	{
	rec_play_iq_state = 0;
	goto done_it;
	}
//----

done_it:
if(vb)printf("rec_play_iq_state_process()1 -  rec_play_iq_state %d  time %.3f, wr %" PRIi64 " %" PRIi64 " %" PRIi64 " cnts %" PRIi64 " %" PRIi64 "\n", rec_play_iq_state, rec_play_time, rec_wr, rec_rd, dif, rec_wr_cnt, rec_rd_cnt );

}







enum en_menu_item_tag
{
en_meni_file_open,
en_meni_file_save,
en_meni_file_open_favoutrite,
en_meni_file_save_favoutrite,
en_meni_file_exit,

en_meni_edit_undo,
en_meni_edit_redo,
en_meni_edit_cut,
en_meni_edit_copy,
en_meni_edit_paste,

en_meni_edit_preferences,

en_meni_device_open,
en_meni_device_close,
en_meni_slew_adj_fine,
en_meni_synth_dialog,
en_meni_synth0,
en_meni_synth_am_mod,
en_meni_synth_am_ssb_lsb_mod,
en_meni_synth_am_ssb_usb_mod,
en_meni_synth_fm_mod,
en_meni_synth_1st_tone_ampl_freq,
en_meni_synth_2nd_tone_ampl_freq,
en_meni_synth_tone,
en_meni_synth_morse0,
en_meni_synth_music0,
en_meni_synth_voice0,
en_meni_synth_voice1,
en_meni_synth_voice2,
en_meni_synth_voice3,
en_meni_synth_single_sideband_usb,
en_meni_synth_noise0,
en_meni_rec_file_sel,
en_meni_rec_file_dur,
en_meni_rec_iq,
en_meni_play_file_sel,
en_meni_play_iq,

en_meni_windows_main,
en_meni_windows_favourites,
en_meni_windows_plot,
en_meni_windows_aud_spect,

en_meni_help_help,
en_meni_help_about,
en_meni_help_debug_adj_fftw_plan,
};








//--------------------- Main Menu --------------------------
Fl_Menu_Item menu_sdr_items[] =
{
	{ "&File",              0, 0, 0, FL_SUBMENU },
		{ "&Open Settings...", FL_CTRL + 'o'	, (Fl_Callback *)cb_menu_combo, (void*)en_meni_file_open },
		{ "&Save Settings...", FL_CTRL + 's'	, (Fl_Callback *)cb_menu_combo, (void*)en_meni_file_save, FL_MENU_DIVIDER },

		{ "&Open Favoutites...", FL_CTRL + 'o'	, (Fl_Callback *)cb_menu_combo, (void*)en_meni_file_open_favoutrite },
		{ "&Save Favoutites...", FL_CTRL + 's'	, (Fl_Callback *)cb_menu_combo, (void*)en_meni_file_save_favoutrite, FL_MENU_DIVIDER },
		{ "E&xit", FL_CTRL + 'q'	, (Fl_Callback *)cb_menu_combo, (void*)en_meni_file_exit },
		{ 0 },

	{ "&Edit", 0, 0, 0, FL_SUBMENU },
		{ "&Undo",  FL_CTRL + 'z', (Fl_Callback *)cb_menu_combo, (void*)en_meni_edit_undo },
		{ "&Redo",  FL_CTRL + 'y', (Fl_Callback *)cb_menu_combo, (void*)en_meni_edit_redo, FL_MENU_DIVIDER },
		{ "Cu&t",  FL_CTRL + 'x', (Fl_Callback *)cb_menu_combo, (void*)en_meni_edit_cut },
		{ "&Copy",  FL_CTRL + 'c', (Fl_Callback *)cb_menu_combo, (void*)en_meni_edit_copy },
		{ "Past&e",  FL_CTRL + 'v', (Fl_Callback *)cb_menu_combo, (void*)en_meni_edit_paste, FL_MENU_DIVIDER },
		{ "&Preferences...",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_edit_preferences },
		{ 0 },

	{ "&Device", 0, 0, 0, FL_SUBMENU },
		{ "&Open Dev...",  FL_CTRL + '1', (Fl_Callback *)cb_menu_combo, (void*)en_meni_device_open },
		{ "&Close Dev...",  FL_CTRL + '2', (Fl_Callback *)cb_menu_combo, (void*)en_meni_device_close, FL_MENU_DIVIDER },
//		{ "&IQ data rate slew (allow fine adj)",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_slew_adj_fine, FL_MENU_TOGGLE|FL_MENU_DIVIDER },
		{ "&Synth IQ samples (tune to 0Hz)",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_synth0, FL_MENU_TOGGLE },		////menu string MUST MATCH defined '&Device/&Synth IQ samples (tune to 0Hz)' menu definition, see 'update_controls()'
		{ "&Synth show test signal dialog...",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_synth_dialog, FL_MENU_DIVIDER },
//		{ "&Synth AM modulation",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_synth_am_mod, FL_MENU_RADIO|FL_MENU_VALUE },
//		{ "&Synth AM modulation ssb lsb",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_synth_am_ssb_lsb_mod, FL_MENU_RADIO },
//		{ "&Synth AM modulation ssb usb",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_synth_am_ssb_usb_mod, FL_MENU_RADIO },
//		{ "&Synth FM modulation",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_synth_fm_mod, FL_MENU_RADIO|FL_MENU_DIVIDER },
//		{ "&Synth 1st tone amplitude/freq...",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_synth_1st_tone_ampl_freq, 0 },
//		{ "&Synth 2nd tone amplitude/freq...",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_synth_2nd_tone_ampl_freq, 0 },
//		{ "&Synth tone modulation",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_synth_tone, FL_MENU_RADIO },
//		{ "&Synth morse modulation",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_synth_morse0, FL_MENU_RADIO|FL_MENU_VALUE },
//		{ "&Synth music tune modulation",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_synth_music0, FL_MENU_RADIO },
//		{ "&Synth voice audio file0 modul.",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_synth_voice0, FL_MENU_TOGGLE|FL_MENU_VALUE },
//		{ "&Synth voice audio file1 modul.",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_synth_voice1, FL_MENU_TOGGLE|FL_MENU_VALUE },

//		{ "&Synth sweep tone modul.",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_synth_voice2, FL_MENU_TOGGLE|FL_MENU_VALUE },
//		{ "&Synth 2 tone modul.",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_synth_voice3, FL_MENU_TOGGLE|FL_MENU_VALUE },
//		{ "&Synth single sideband (upper)",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_synth_single_sideband_usb, FL_MENU_TOGGLE|FL_MENU_VALUE },
		
//		{ "&Synth noise on IQ",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_synth_noise0, FL_MENU_TOGGLE|FL_MENU_DIVIDER|FL_MENU_VALUE },

		{ "&Record File Select...",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_rec_file_sel, 0 },
		{ "&Record File Duration...",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_rec_file_dur, 0 },

		//NOTE below menu string MUST match for:  'meMain->find_item("&Device/&Record IQ")'    	in 'update_controls()'
		{ "&Record IQ",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_rec_iq, FL_MENU_TOGGLE|FL_MENU_DIVIDER  },
		{ "&Play File Select...",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_play_file_sel, 0 },
		//NOTE below menu string MUST match for:  'meMain->find_item("&Device/&Play IQ")'    	in 'update_controls()'
		{ "&Play IQ",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_play_iq, FL_MENU_TOGGLE },
		{ 0 },


	{ "&Windows", 0, 0, 0, FL_SUBMENU },
		{ "&Main Wnd",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_windows_main },
		{ "&Favourites",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_windows_favourites },
		{ "&SDR Chain Plotter",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_windows_plot },
		{ "&Aud Spect Notcher",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_windows_aud_spect },
	
		{ 0 },

	{ "&Help", 0, 0, 0, FL_SUBMENU },
//		{ "Open 'help.txt'...",  0, (Fl_Callback *)cb_menu_combo, (void*)en_meni_help_help },	
		{ "&About...", 0				, (Fl_Callback *)cb_menu_combo, (void*)en_meni_help_about },
//		{ "&Debug - AdjustFFTWPlans", 0				, (Fl_Callback *)cb_menu_combo, (void*)en_meni_help_debug_adj_fftw_plan },

		{ 0 },


	{ 0 }
};
//----------------------------------------------------------








// compare character case-insensitive
struct icompare_char 
{
bool operator()( char c1, char c2 ) 
	{
	return std::toupper( c1 ) < std::toupper( c2 );
	}
};





// return true if s1 comes before s2
bool alpha_compare( std::string const& s1, std::string const& s2 ) 
{
if ( s1.length() < s2.length() ) return true;
if ( s1.length() > s2.length() ) return false;
return std::lexicographical_compare( s1.begin(), s1.end(), s2.begin(), s2.end(), icompare_char() );
}





void sort_vstring( vector<string> &vstr )
{
std::sort( vstr.begin(), vstr.end(), alpha_compare );
}







// return true if s1 comes before s2
bool alpha_compare_vsft( const st_scan_freq_tag &s1, const st_scan_freq_tag &s2 ) 
{
if ( s1.start.length() < s2.start.length() ) return true;
if ( s1.start.length() > s2.start.length() ) return false;
return std::lexicographical_compare( s1.start.begin(), s1.start.end(), s2.start.begin(), s2.start.end(), icompare_char() );
}



void sort_st_scan_freq_tag( vector<st_scan_freq_tag> &vsft )
{
std::sort( vsft.begin(), vsft.end(), alpha_compare_vsft );
}






//----------------------------------------------------------
My_Input_Choice::My_Input_Choice(int x,int y,int w, int h,const char *label) : Fl_Input_Choice(x,y,w,h,label)
{
b_allow_wheel_inc = 1;

}







int My_Input_Choice::handle(int e)
{
string s1;
int len;
char *szTmp;
bool need_redraw = 0;
bool dont_pass_on = 0;
int mousewheel;

if ( e == FL_KEYDOWN )						//key press?
	{
	if( Fl::event_key() == FL_Enter )		//is it CR ?
		{
//		cb_bt_freq_tune( 0, 0 );
		wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "My_Input_Choice::handle() 0" );
		dont_pass_on = 1;
		}
	}



if ( e == FL_MOUSEWHEEL )
	{
	mousewheel = Fl::event_dy();
	
	need_redraw = 1;
	dont_pass_on = 1;

	if( b_allow_wheel_inc )
		{
		s1 = value();
		double frq;
		sscanf( s1.c_str(), "%lf", &frq );

		double wheel_freq_step = 1000;
		if( mousewheel > 0 ) frq += wheel_freq_step;
		if( mousewheel < 0 ) frq -= wheel_freq_step;
		
		strpf( s1, "%d", (int)frq );
		value( s1.c_str() );
		wnd_rtl_graph->freq_listen( 0, 0, 0, 0, "My_Input_Choice::handle() 1" );
		}
	}

//if ( need_redraw ) redraw();

if( dont_pass_on ) return 1;

return Fl_Input_Choice::handle(e);
}




//----------------------------------------------------------















//----------------------------------------------------------


cl_waterfall::cl_waterfall( int xx, int yy, int wid, int hei, const char *label = 0 ) : Fl_Double_Window( xx, yy, wid, hei, label ) 
{
inside_control = 0;
left_button = 0;
ctrl_key = 0;
shift_key = 0;
left_button = 0;
right_button = 0;
middle_button = 0;


mousex = mousey = 0;
mousemove_p_cb = 0;
left_click_p_cb = 0;
mousewheel_p_cb = 0;


//pixbf0 = 0;
pixbf1 = 0;
sizex = 0;
sizey = 0;
//fbuf0 = 0;

lim_high = 1.0;
lim_low = 0.05;

gain = 5.0f;

wr = 0;
rd = 0;

make_colour_map();


freq_center = 0;
bwidth = 0;
iffreq = 0;


//win_wfall = new Fl_Double_Window( xx, yy, wid, hei, 0 );
//win_wfall->end();
}




cl_waterfall::~cl_waterfall()
{
//if( fbuf0 ) delete[] fbuf0;
//if( pixbf0 ) delete[] pixbf0;
if( pixbf1 ) delete[] pixbf1;
}






//no longer used
void cl_waterfall::set_freq_details( int freq_center_in, int bwidth_in, int iffreq_in, double disp_zoom_factor_in )
{
freq_center = freq_center_in;
bwidth = bwidth_in;
iffreq = iffreq_in;

disp_zoom_factor = disp_zoom_factor_in;
}





void cl_waterfall::set_mousemove_cb( void (*p_cb)( cl_waterfall*, void* ), void *args )
{
mousemove_p_cb = p_cb;
mousemove_cb_args = args;
}






void cl_waterfall::set_left_click_cb( void (*p_cb)( cl_waterfall*, void* ), void *args )
{
left_click_p_cb = p_cb;
left_click_cb_args = args;
}




void cl_waterfall::set_mousewheel_cb( void (*p_cb)( cl_waterfall*, void*, int ), void *args )
{
mousewheel_p_cb = p_cb;
mousewheel_cb_args = args;
}



void cl_waterfall::make_colour_map()
{

//black 0-->0.09
col_ramp[0].r = 0;
col_ramp[0].g = 0;
col_ramp[0].b = 0;

//blue 0.1-->0.19
col_ramp[1].r = 0;
col_ramp[1].g = 0;
col_ramp[1].b = 255;

//green 0.2-->0.29
col_ramp[2].r = 0;
col_ramp[2].g = 200;
col_ramp[2].b = 0;

//green2
col_ramp[3].r = 0;
col_ramp[3].g = 255;
col_ramp[3].b = 0;

//red
col_ramp[4].r = 200;
col_ramp[4].g = 0;
col_ramp[4].b = 0;

//red2
col_ramp[5].r = 255;
col_ramp[5].g = 0;
col_ramp[5].b = 0;

//orange
col_ramp[6].r = 255;
col_ramp[6].g = 145;
col_ramp[6].b = 0;

//orange2
col_ramp[7].r = 255;
col_ramp[7].g = 200;
col_ramp[7].b = 0;


//yellow
col_ramp[8].r = 255;
col_ramp[8].g = 255;
col_ramp[8].b = 0;

//yellow2
col_ramp[9].r = 250;
col_ramp[9].g = 243;
col_ramp[9].b = 137;


//white
col_ramp[10].r = 255;
col_ramp[10].g = 255;
col_ramp[10].b = 255;
}






//int cnt00 = 0;


//produce a colour proportional to predefined col table held in 'col_ramp[]'
//dissolve linearly between table colours
void cl_waterfall::dissolve_ramp( float level, st_wtrfall_col_tag &col )
{
int idx = 1, idx_low;


float band = 0;

								if( level > 1.0f ) level = 1.0f;
								if( level < 0.0f ) level = 0.0f;

if( level >= 0.9f )
	{
	band = 0.9f;
	idx = 10;
	goto fin;
	}

if( level >= 0.8f )
	{
	band = 0.8f;
	idx = 9;
	goto fin;
	}

if( level >= 0.7f )
	{
	band = 0.7f;
	idx = 8;
	goto fin;
	}

if( level >= 0.6f )
	{
	band = 0.6f;
	idx = 7;
	goto fin;
	}

if( level >= 0.5f )
	{
	band = 0.5f;
	idx = 6;
	goto fin;
	}

if( level >= 0.4f )
	{
	band = 0.4f;
	idx = 5;
	goto fin;
	}

if( level >= 0.3f )
	{
	band = 0.3f;
	idx = 4;
	goto fin;
	}

if( level >= 0.2f )
	{
	band = 0.2f;
	idx = 3;
	goto fin;
	}


if( level >= 0.1 )
	{
	band = 0.1f;
	idx = 2;
	}



fin:

//cnt00++;

float new_level = ( level - band ) * 10.0f;
//if(!(cnt00%50))printf("cl_waterfall::dissolve_ramp() - level %f  band %f  new_level %f  idx_low %d\n", level, band, new_level, idx_low );

idx_low = idx - 1;

if( idx_low < 0 ) idx_low = 0;

dissolve_col( new_level, col_ramp[ idx_low ], col_ramp[ idx ], col );
 
}




//dissolve linearly between 2 cols using fader value as control
//fader = 0, produces full col_a
//fader = 0.5 produces 50% mix between col_a and col_b
//fader = 0.99, produces full col_b
void cl_waterfall::dissolve_col( float fader, st_wtrfall_col_tag &col_a, st_wtrfall_col_tag &col_b, st_wtrfall_col_tag &col_fader )
{
col_fader.r = ( ( 1.0f - fader ) * col_a.r ) + fader * col_b.r;
col_fader.g = ( ( 1.0f - fader ) * col_a.g ) + fader * col_b.g;
col_fader.b = ( ( 1.0f - fader ) * col_a.b ) + fader * col_b.b;
}









void cl_waterfall::clear()
{
if( pixbf1 == 0 ) return;

memset( pixbf1, 0, sizey * sizex * cn_bytes_per_pixel );

/*
for( int iy = 0; iy < sizey; iy++ )
	{
	for( int ix = 0; ix < sizex; ix++ )
		{
		int ptr = iy * sizex * cn_bytes_per_pixel   +   ix * cn_bytes_per_pixel;

		fbuf0[ptr] = 0.0f;
		}
	}
*/
redraw();
}









float wfall_peak = 0.0;													//holds peak with slow decay
float wfall_lpf = 0.0f;



void cl_waterfall::add_line( vector<st_spect_tag> &vsp )
{

bool vb = 0;

//if( fbuf0 == 0 ) return;

vector<float> vf_line;

//vector<st_spect_tag> vsp2;

// ---- simple sample rate conversion ----

float ratio = ( (float) vsp.size() / sizex );		

//int downsample_factor = (float)vsp.size() / sizex;

//vsp2 = vsp;
if(vb)printf( "cl_waterfall::add_line() - vsp.size() %d sizex %d ratio %f\n",  vsp.size(), sizex, ratio );

float sp_ptr = 0;
float sum = 0;

int filter_prev_index = 0;

float max = 0;
float ff;
//float last_f0 = 0;

//int ptr_last = 0;


//for( int ix = 0; ix < vsp.size(); ix++ )
for( int ix = 0; ix < sizex; ix++ )
	{
//	if( ix >= sizex ) continue;
	
//	if(ix == 3000 ) vsp[ix].ampl = 0.1f;
//	float ff = vsp[ix].ampl*vsp[ix].ampl * 20.0f;//gain;


	float log_offset = 1.8f;
 
	float f0;
	float f1;
	if( ratio >= 15.0f )												//is downsampling skipping many samples (zoomed out) ?, use different plotting scheme
		{
		for( int sp_ptr2 = 0; sp_ptr2 < ratio; sp_ptr2++ )				//find max val in a section of samples to capture peak spectra detail
			{
			int pp = sp_ptr + sp_ptr2;
			if( pp < vsp.size() )
				{
				f0 = vsp[pp].ampl;
				}
			else{
				f0 = 0;
				}
			if( f0 > max ) max = f0;
			}

		ff = max;
//		ff = ( log_offset+log10f( max ) ) / 2.1f;
		max = 0;
		}
	else{
		//if here zoomed in, use linear interpolation
		if( sp_ptr >= 1 )
			{
//			f0 = ( log_offset+log10f( vsp[sp_ptr - 1].ampl ) ) / 2.1f;
			f0 = vsp[sp_ptr - 1].ampl;
			}
		else{
			f0 = 0;
			}

//		f1 = ( log_offset+log10f( vsp[sp_ptr].ampl ) ) / 2.1f;
		f1 = vsp[sp_ptr].ampl;

		double dint;
		float fract = modf(sp_ptr, &dint );

		ff = (f1 - f0) * fract + f0;									//linear interpolation

//		ff = ( log_offset+log10f( ff ) ) / 2.1f;
		}
	
/*
	if( ratio >= 15.0f )												//is downsampling skipping many samples?, use different plotting scheme
		{
		for( int sp_ptr2 = 0; sp_ptr2 < ratio; sp_ptr2++ )				//find max val in a section of samples to capture peak spectra detail
			{
			int pp = sp_ptr + sp_ptr2;
			if( pp < vsp.size() )
				{
				f0 = vsp[pp].ampl;
				}
			else{
				f0 = 0;
				}
			if( f0 > max ) max = f0;
			}
		
		ff = ( log_offset+log10f( max ) ) / 2.1f;
		max = 0;
		}
	else{

		
		if( sp_ptr >= 1 )
			{
			f0 = ( log_offset+log10f( vsp[sp_ptr - 1].ampl ) ) / 2.1f;
			}
		else{
			f0 = 0;
			}

		f1 = ( log_offset+log10f( vsp[sp_ptr].ampl ) ) / 2.1f;

		double dint;
		float fract = modf(sp_ptr, &dint );
		
		ff = (f1 - f0) * fract + f0;									//linear interpolation
		}
*/



	float f2 = iir_process( wfall_iir0, ff );
	wfall_lpf = fabsf(f2);
	
	if( ff > wfall_peak ) wfall_peak = ff;								//floating max val which is decayed further below

	if( wfall_peak > 0.05f ) wfall_peak = 0.05f;						//don't let avg get too high, otherwise low level carrier spectra will be assigned dim colours  (without this the prob happens when one carrier is far higher in level to other carriers)


//	if( wfall_peak < 0.0095f ) wfall_peak = 0.0095f;					//don't let avg get too low, otherwise noise will be assigned a bright colour
	if( wfall_peak < (wfall_lpf*50.0f) ) wfall_peak = wfall_lpf*50.0f;	//don't let 'wfall_peak' get too low, otherwise noise will be assigned a bright colour

	ff /= wfall_peak;													//scale according to 'wfall_peak'


	if( pref_wfall_non_linear_gain )	ff = ( 4.3f+logf( ff ) ) / 4.3f;										//non linear gain to bring up the low detail and noise so it's visible on wfall

//	if( ff > 1.0f ) ff = 1.0f;
//	if( ff < 0.0f ) ff = 0.0f;


if(vb)printf( "cl_waterfall::add_line() - wfall_peak %f  wfall_lpf %f\n",  wfall_peak, wfall_lpf );

	
//	if( ff > max ) max = ff;
	
//	sum += ff;
//	filter_prev_index += ratio;
//	filter_prev_index++;
//	if( filter_prev_index < downsample_factor ) continue;

//	if ( filter_prev_index < 1.0f ) continue;


//	int ptr = wr*sizex*cn_bytes_per_pixel + ix * cn_bytes_per_pixel;	
//	fbuf0[ptr] = ff;	
	
	vf_line.push_back( ff );
	
//	if( ptr > ptr_last )
		{
//		max = 0;	
		}
	
//	ptr_last = ptr;
	
//	fbuf0[ptr] = sum * ratio;											//complete average by effectively 'dividing' by num of samples added to 'su'm  (num of samples is 1/ratio)
	
//	sum = 0.0f;
//	max = 0;
	sp_ptr += ratio;
	
//	if( ratio_x >= sizex ) break;
//	filter_prev_index -= 1.0f;
	filter_prev_index = 0.0f;
	}


	wfall_peak = wfall_peak*0.94f;										//decay


if(vb)printf( "sizex %d sp_ptr %f\n", sizex, sp_ptr );

wr--;
if( wr < 0 ) wr = sizey - 1;

rd--;
if( rd < 0 ) rd = sizey - 1;


add_line_to_image( vf_line );											//v1.05



//----
tm tt;
string s1, s2, s3, s4, s5, s6, s7, stime;
mystr m1;


m1.get_time_now( tt );
m1.make_time_str( tt, s1, s2, s3, stime );

//void make_date_str( struct tm tn, string &dow, string &dom, string &mon_num, string &mon_name, string &year, string &year_short );

m1.make_date_str( tt, s1, s2, s3, s4, s5, s6 );
strpf( s7, "%s-%s-%s", s2.c_str(), s4.c_str(), s6.c_str() );
//printf( "   Date is: " );
//printf( "%s", s7.c_str() );

//strpf( s7, "   %s %s-%s-%s", s1.c_str(), s2.c_str(), s4.c_str(), s5.c_str() );
//printf( "%s", s7.c_str() );
strpf( sdate_stamp, "%s %s", stime.c_str(), s7.c_str() );
//----

redraw();
}


















void cl_waterfall::add_line_to_image( vector<float> &vf )				//v1.05
{
//printf( "cl_waterfall::add_line_to_image() - sizex %d,  vf.size() %d\n", sizex, vf.size() );



unsigned char *psrc = pixbf1;
unsigned char *pdest = pixbf1 + ( 1 * sizex * cn_bytes_per_pixel );

int cnt = (sizey - 1) * sizex * cn_bytes_per_pixel;

memmove( pdest, psrc, cnt );									//shift image down one line


/*
for( int iy = sizey - 2; iy >= 0 - 1; iy-- )
	{
	for( int ix = 0; ix < sizex; ix++ )
		{
		int p_rd = iy * sizex * cn_bytes_per_pixel   +   ix * cn_bytes_per_pixel;
		int p_wr = (iy+1) * sizex * cn_bytes_per_pixel   +   ix * cn_bytes_per_pixel;
		
		pixbf1[p_wr] = pixbf1[p_rd];
		pixbf1[p_wr + 1] = pixbf1[p_rd + 1];
		pixbf1[p_wr + 2] = pixbf1[p_rd + 1];
		
//		ff = 1.0 - (log( ff ) / 3.0);

//	float ff = rnd() * gain;
	
//	if( ff > 1.0f ) ff = 1.0f;
//	if( ff < 0.0f ) ff = 0.0f;


//		st_wtrfall_col_tag col;
//		dissolve_ramp( (float)ix/sizex, col );
//		dissolve_ramp( ff, col );


//		pixbf0[ p_image + 0 ] = col.r;
//		pixbf0[ p_image + 1 ] = col.g;
//		pixbf0[ p_image + 2 ] = col.b;
		}
//	ii++;
//	if( ii >= sizey ) ii = 0;
	}
*/



/*
	for( int ix = 0; ix < sizex; ix++ )
		{
		int iy = 0;
		int p_wr = iy * sizex * cn_bytes_per_pixel   +   ix * cn_bytes_per_pixel;

		float frnd = rnd();
		
		frnd += 1.0f;		
		frnd /= 2.0f;
		
		int ir = frnd * 255;

		pixbf1[p_wr] = ir;
		pixbf1[p_wr + 1] = 0x0;
		pixbf1[p_wr + 2] = 0x0;

		}
*/		


	//render a single line of pixels
	for( int ix = 0; ix < vf.size(); ix++ )
		{
		int iy = 0;
		int p_image = iy * sizex * cn_bytes_per_pixel   +   ix * cn_bytes_per_pixel;


		st_wtrfall_col_tag col;
//		dissolve_ramp( (float)ix/sizex, col );
		dissolve_ramp( vf[ix], col );


		pixbf1[ p_image ] = col.r;
		pixbf1[ p_image + 1 ] = col.g;
		pixbf1[ p_image + 2 ] = col.b;
		}

}



















void cl_waterfall::draw()
{
//printf( " cl_waterfall::draw()\n" );


string s1;
mystr m1;

int iF = fl_font();
int iS = fl_size();


int xx = x();
int yy = y();




bool alloc = 0;
if( sizex != w() ) alloc = 1;
if( sizey != h() ) alloc = 1;


sizex = w();
sizey = h();




fl_color( FL_BACKGROUND_COLOR );
//fl_rectf( xx , yy , w(), h() );
fl_rectf( 0 , 0 , w(), h() );






if( alloc )
	{
//	if( pixbf0 ) delete[] pixbf0;
	if( pixbf1 ) delete[] pixbf1;

//	pixbf0 = new unsigned char [ sizex * sizey * cn_bytes_per_pixel ];	//4x plotting buffer


	pixbf1 = new unsigned char [ sizex * sizey * cn_bytes_per_pixel ];

//	if( fbuf0 ) delete[] fbuf0;
//	fbuf0 = new float [ sizex * sizey * cn_bytes_per_pixel ];
	
	clear();
	
	wr = 0;
	rd = 0;
//	printf("size0x %d %d\n", sizex, sizey );
	}

/*

int ii = rd;


//	printf("size0x %d %d\n", sizex, sizey );
for( int iy = 0; iy < sizey; iy++ )
	{
	for( int ix = 0; ix < sizex; ix++ )
		{
		int p_rd = ii * sizex * cn_bytes_per_pixel   +   ix * cn_bytes_per_pixel;
		int p_image = iy * sizex * cn_bytes_per_pixel   +   ix * cn_bytes_per_pixel;

		float ff = fbuf0[p_rd];
		
//		ff = 1.0 - (log( ff ) / 3.0);

//	float ff = rnd() * gain;
	
//	if( ff > 1.0f ) ff = 1.0f;
//	if( ff < 0.0f ) ff = 0.0f;


		st_wtrfall_col_tag col;
//		dissolve_ramp( (float)ix/sizex, col );
		dissolve_ramp( ff, col );


		pixbf0[ p_image + 0 ] = col.r;
		pixbf0[ p_image + 1 ] = col.g;
		pixbf0[ p_image + 2 ] = col.b;
		}
	ii++;
	if( ii >= sizey ) ii = 0;
	}

*/

int menu_hei = 0;
//fl_draw_image( pixbf1, xx, yy, sizex, sizey, cn_bytes_per_pixel, sizex * cn_bytes_per_pixel );
fl_draw_image( pixbf1, 0, 0, sizex, sizey, cn_bytes_per_pixel, sizex * cn_bytes_per_pixel );



//text
fl_font( 4, 11 );
fl_color( 255, 255, 100 );


int fractional_digits = 3;

string snum, sunits, scombined;

m1.make_engineering_str( snum, sunits, scombined, fractional_digits, freq_disp0, " ", "Hz" );

//strpf( s1, "%.3f Hz %s", gph_mouse_freq, scombined.c_str() );
strpf( s1, "%d Hz %s", freq_disp0, scombined.c_str() );
//fl_draw( s1.c_str(), x() + 2, y() + 10 );


int txt_hei;
int txt_descnt;
int txt_wid_freq = wnd_rtl_graph->text_dim( s1, 4, 11, txt_hei, txt_descnt );
fl_color( 50, 50, 50 );

fl_rectf( 0, txt_hei+txt_descnt, txt_wid_freq+2, txt_hei+txt_descnt );	//freq bkg


int txt_wid_date = wnd_rtl_graph->text_dim( sdate_stamp, 4, 11, txt_hei, txt_descnt );

int txt_wid = txt_wid_freq;												//pick widest text dim
if( txt_wid < txt_wid_date ) txt_wid = txt_wid_date;

fl_rectf( 0, 0, txt_wid_date + 2, txt_hei + txt_descnt - 4 );			//time bkg


fl_color( 255, 255, 100 );

fl_draw( sdate_stamp.c_str(), 0 + 2, 0 + 9 );
fl_draw( s1.c_str(), 0 + 2, 0 + 30 );

//border
//fl_color( 180, 180, 180 );
//fl_line_style ( FL_SOLID, 0 );        	 					//for windows you must set line setting after the colour, see the manual
//if(1) fl_rect( xx , yy , w(), h() );
//if(1) fl_rect( 1 , 1 , w()-2, h()-2 );

fl_font( iF, iS );
}







int cl_waterfall::handle(int e)
{
string s1;
int len;
char *szTmp;
bool need_redraw = 0;
bool dont_pass_on = 0;
int mousewheel;



if( e == FL_ENTER )	
	{
	inside_control = 1;

	need_redraw = 1;
	dont_pass_on = 1;
	}


if( e == FL_LEAVE )	
	{
	inside_control = 0;

	need_redraw = 1;
	dont_pass_on = 1;
	}



if ( e & FL_MOVE )
	{
	mousex = Fl::event_x();
	mousey = Fl::event_y();

	if( mousemove_p_cb != 0 ) mousemove_p_cb( this, mousemove_cb_args );




/*
	double ratio = (mousex-x()) / (double)sizex;


	double bw_zoomed = bwidth;
	if( disp_zoom_factor != 0.0f )
		{
		bw_zoomed = (double)bwidth / (double)disp_zoom_factor;	
		}
		
	double freq_left = freq_center - bw_zoomed / 2.0 - iffreq;


//printf("cl_waterfall::handle() - FL_MOVE BBBBBBBBBBBBBBBBBBBBBBBBBBBB ratio %f   bw_zoomed %f  freq_left %f  freq_center %d\n", ratio, bw_zoomed, freq_left, (int)freq_center );




//	int freq_left = freq_center - bwidth/2 - iffreq;

	int freq = freq_left + bw_zoomed * ratio;
	freq_disp0 = freq;
*/

	need_redraw = 1;
    dont_pass_on = 1;
	}
	
	
	
if ( e == FL_PUSH )	
	{
	int ii = Fl::event_clicks();

	if( Fl::event_button() == 1 )											//left click
		{
		left_button = 1;
		if( ii == 2 )
			{
//			if( left_double_click_p_cb != 0 ) left_double_click_p_cb( this, left_double_click_cb_args );
			}
		else{
			if( left_click_p_cb != 0 ) left_click_p_cb( this, left_click_cb_args );
			}
		}
	need_redraw = 1;
    dont_pass_on = 1;
	}



if ( e == FL_RELEASE )	
	{
	int ii = Fl::event_clicks();

	if( Fl::event_button() == 1 )											//left click
		{
		left_button = 0;
		}
	need_redraw = 1;
    dont_pass_on = 1;
	}

if ( e == FL_MOUSEWHEEL )
	{
	if( !mousewheel_p_cb )
		{
//		do_callback();														//use fltk callback?
		}
	else{
		mousewheel_p_cb( this, mousewheel_cb_args, Fl::event_dy() );
		}
	need_redraw = 1;
    dont_pass_on = 1;
	}

if ( e == FL_KEYUP )  				                    //key release?
	{
	int key = Fl::event_key();
//	if( ( key == FL_Control_L ) || ( key == FL_Control_R ) ) ctrl_key = 0;
//`	if( ( key == FL_Shift_L ) || (  key == FL_Shift_R ) ) shift_key = 0;

	need_redraw = 1;
    dont_pass_on = 1;
	}


//if ( need_redraw ) redraw();

if( dont_pass_on ) return 1;

//return Fl_Widget::handle(e);
return Fl_Double_Window::handle(e);


}

//----------------------------------------------------------






/*
void cb_bt_freq_bandwidth( Fl_Widget *, void * )
{
string s1;

float ff;

s1 = wnd_rtl_graph->fi_freq_bandwidth->value();
sscanf( s1.c_str(), "%f", &ff );

rtl.set_srate( ff );
printf("!!!!!!!!!!!!!!!!!!!!!!!!cb_bt_freq_bandwidth() - %d\n", rtl.get_srate() );
}
*/





void cb_bt_freq_start_marker( Fl_Widget *, void * )
{
string s1;

s1 = wnd_rtl_graph->miwp_tune->miw->value();
wnd_rtl_graph->fi_freq_start->value( s1.c_str() );
}








void cb_bt_freq_stop_marker( Fl_Widget *, void * )
{
string s1;

s1 = wnd_rtl_graph->miwp_tune->miw->value();
wnd_rtl_graph->fi_freq_stop->value( s1.c_str() );
}








void do_scan()
{
string s1;
mystr m1;

if( !wnd_rtl_graph ) return;


if( !rtl.open_status() )
	{
	printf( "do_scan() - rtl dongle not open\n" );
	return;
	}

double freq_tune = 0; 
double freq_start = 410000;
double freq_stop = 490000;
double freq_step = 1000;
double freq_gain= 100;
double bandwidth = 2000000;
double freq_reso = 8192;
double threshold = 0;

double bw_lower;
double bw_upper;
double interfreq;
bool bandwidth_limit;

get_user_gui_control_params();


//get_user_params( freq_tune, freq_start, freq_stop, freq_step, freq_gain, bandwidth, freq_reso, threshold, bw_lower, bw_upper, interfreq, bandwidth_limit  );

//g_bw_lower = bw_lower;
//g_bw_upper = bw_upper;
//g_interfreq = interfreq;
//g_rtl_bw = bandwidth;
//g_bw_limit = bandwidth_limit;


//rtl.set_gain( freq_gain );

//rtl.set_srate( bandwidth );
//printf("set_srate() - %u\n", rtl.get_srate() );



//rtl_scan( freq_start, freq_stop, freq_step, freq_gain, bandwidth, freq_reso, threshold );
rtl_scan( g_freq_start, g_freq_stop, g_freq_step, g_dev_gain, g_dev_bw, freq_reso, threshold );

wnd_rtl_graph->update_gph0( vspect );

}







void get_user_params( st_ctrl_params_tag &prm )
{
string s1;

printf( "get_user_params()\n" );

if( !wnd_rtl_graph ) return;

prm.freq_tune = wnd_rtl_graph->miwp_tune->miw->get_value_as_double();

prm.freq_sub_tune = wnd_rtl_graph->miw_freq_sub_tune->get_value_as_double();

printf( "get_user_params() prm.freq_sub_tune %f\n", prm.freq_sub_tune );

s1 = wnd_rtl_graph->fi_freq_start->value();
sscanf( s1.c_str(), "%lf", &prm.freq_start );

s1 = wnd_rtl_graph->fi_freq_stop->value();
sscanf( s1.c_str(), "%lf", &prm.freq_stop );

s1 = wnd_rtl_graph->fi_freq_step->value();
sscanf( s1.c_str(), "%lf", &prm.freq_step );

//s1 = wnd_rtl_graph->fi_freq_gain->value();
//sscanf( s1.c_str(), "%lf", &gain );
prm.freq_gain = wnd_rtl_graph->miw_freq_gain->get_value_as_double();


s1 = wnd_rtl_graph->fi_freq_threshold->value();
sscanf( s1.c_str(), "%f", &prm.threshold );

s1 = wnd_rtl_graph->fi_freq_resolution->value();
sscanf( s1.c_str(), "%f", &prm.freq_reso );

prm.dev_bandwidth = wnd_rtl_graph->miw_dev_bwidth->get_value_as_double();

s1 = wnd_rtl_graph->fi_freq_threshold->value();
sscanf( s1.c_str(), "%f", &prm.threshold );
wnd_rtl_graph->threshold = prm.threshold;

//s1 = wnd_rtl_graph->fi_bw_lower->value();
//sscanf( s1.c_str(), "%lf", &bw_lower );


prm.b_dwn_aa =  wnd_rtl_graph->ck_user_dwn_aa->value();
prm.i_dwn_srate = wnd_rtl_graph->miw_dev_dwnconv_srate->get_value_as_double();

prm.b_bw_limit = wnd_rtl_graph->ck_bw_limit->value();


prm.bw_lower = wnd_rtl_graph->miw_filt_lwr->get_value_as_double();


prm.bw_upper = wnd_rtl_graph->miw_filt_upr->get_value_as_double();

prm.bw_taps = wnd_rtl_graph->miw_filt_taps->get_value_as_double();


prm.interfreq = wnd_rtl_graph->miw_if_freq->get_value_as_double();


if( prm.dev_bandwidth < cn_rtl_e4000_sample_bandwith_min )
	{
	prm.dev_bandwidth = cn_rtl_e4000_sample_bandwith_min;
//	strpf( s1, "%d", (int)bandwidth );
	
//	wnd_rtl_graph->fi_freq_bandwidth->value( s1.c_str() );
	}
	
if( prm.dev_bandwidth > cn_rtl_e4000_sample_bandwith_max )	
	{
	prm.dev_bandwidth = cn_rtl_e4000_sample_bandwith_max;
//	strpf( s1, "%d", (int)bandwidth );
	
//	wnd_rtl_graph->fi_freq_bandwidth->value( s1.c_str() );
	}

prm.b_if_freq = wnd_rtl_graph->ck_if_freq->value();


prm.tuning_offset = wnd_rtl_graph->miw_tuning_offset->get_value_as_double();


prm.gain_iq = wnd_rtl_graph->miw_gain_iq->get_value_as_double();	

prm.direct_sampling = wnd_rtl_graph->ld_direct_sampling->GetColIndex();

prm.bias_t = wnd_rtl_graph->ld_bias_t->GetColIndex();

prm.b_agc =  wnd_rtl_graph->ld_agc->GetColIndex();

prm.audgain = wnd_rtl_graph->fvs_gain->value();

prm.b_dcblk = wnd_rtl_graph->ck_user_dc_block_iq->value();

//prm.rtl_srate_filter = wnd_rtl_graph->miw_rtl_srate_filter->get_value_as_double();
//if( prm.rtl_srate_filter > cn_downsample_srate_max ) prm.rtl_srate_filter = cn_downsample_srate_max;
//if( prm.rtl_srate_filter < cn_downsample_srate_min ) prm.rtl_srate_filter = cn_downsample_srate_min;

prm.b_clip_enable = wnd_rtl_graph->ld_clip->GetColIndex();
}











void cb_bt_freq_stop( Fl_Widget *, void * )
{
printf( "cb_bt_freq_stop()\n" );

//if( rtl_fm_running ) call_rtl_fm_kill();

if( !wnd_rtl_graph ) return;

//rtl_listen_stop();

imode = en_mode_stop;

}












void cb_bt_freq_start( Fl_Widget *, void * )
{
string s1;
mystr m1;

if( !wnd_rtl_graph ) return;

cb_bt_freq_stop( 0, 0 );

imode = en_mode_scan_mode;

wnd_rtl_graph->add_to_scan_history();



stop_audio();
stop_threads();

//rtl.close();

m1.delay_ms( 80 );
//start_up_state = 0;


do_scan();


}








void cb_rotary_knob_mouse_button( void *w, void *v )
{
string s1;
int which = (intptr_t)v;

cl_rotary_knob *ow = (cl_rotary_knob *)w;

printf( "cb_rotary_knob_mouse_button() - which: %d, amount %f\n", which, ow->rotate_change_amount );

if( which == 0 )
	{
	if( (ow->left_button ) && !( ow->right_button ) )
		{
		if(ow->user_multiplier == 100000 ) ow->user_multiplier = 1000000;		//this will result in wrap back to 1, see below, therefore 1000000 is never reached
		if(ow->user_multiplier == 10000 ) ow->user_multiplier = 100000;
		if(ow->user_multiplier == 1000 ) ow->user_multiplier = 10000;
		if(ow->user_multiplier == 100 ) ow->user_multiplier = 1000;
		if(ow->user_multiplier == 10 ) ow->user_multiplier = 100;
		if(ow->user_multiplier == 1 ) ow->user_multiplier = 10;

		if(ow->user_multiplier == 1000000 ) ow->user_multiplier = 1;
		}
	}
}




void cb_rotary_knob_freq( Fl_Widget *w, void *v )
{
string s1;
int which = (intptr_t)v;

cl_rotary_knob *ow = (cl_rotary_knob *)w;

printf( "cb_rotary_knob_freq() - which: %d, amount %f\n", which, ow->rotate_change_amount );

if( which == 0 )
	{

	double dd = wnd_rtl_graph->miwp_tune->miw->get_value_as_double();

	dd += ow->rotate_change_amount;
	
	wnd_rtl_graph->miwp_tune->miw->set_value_from_double( dd );

	wnd_rtl_graph->freq_listen( 0, 0, 0, 0, "cb_rotary_knob_freq()" );
	}
}









void cb_bt_keypad( Fl_Widget *w, void *v )
{
string s1;
int which = (intptr_t)v;

rtl_graph_wnd *ow = wnd_rtl_graph;


//------- these MUST be before 'b_keypad_clear'	action further below

s1 = ow->miwp_tune->miw->value();

printf( "cb_bt_keypad()  '%s'\n", s1.c_str() );


if( which == 40 )														//zero SubFreq
	{
	g_freq_sub_tune = 0;
	wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( g_freq_sub_tune );
	return;
	}



if( which == 30 )														//'/2'
	{

	double d0;
	sscanf( s1.c_str(), "%lf", &d0 );

	d0 /= 2.0f;
	ow->miwp_tune->miw->set_value_from_double( d0 );

	wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_bt_keypad() 30" );

	b_keypad_clear = 0;													//flag a clear is req
	return;
	}


if( which == 31 )														//'x2'
	{
//	s1 = ow->miwp_tune->miw->value();
	

	double d0;
	sscanf( s1.c_str(), "%lf", &d0 );

	printf( "cb_bt_keypad() s1 '%s' %f\n", s1.c_str(), d0  );

	d0 *= 2.0f;
	ow->miwp_tune->miw->set_value_from_double( d0 );

	wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_bt_keypad() 31" );

	b_keypad_clear = 0;													//flag a clear is req
	return;
	}


if( which == 32 )														//'/3'
	{
//	s1 = ow->miwp_tune->miw->value();

	double d0;
	sscanf( s1.c_str(), "%lf", &d0 );

	d0 /= 3.0f;
	ow->miwp_tune->miw->set_value_from_double( d0 );

	wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_bt_keypad() 32" );

	b_keypad_clear = 0;													//flag a clear is req
	return;
	}


if( which == 33 )														//'x3'
	{
//	s1 = ow->miwp_tune->miw->value();

	double d0;
	sscanf( s1.c_str(), "%lf", &d0 );

	d0 *= 3.0f;
	ow->miwp_tune->miw->set_value_from_double( d0 );

	wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_bt_keypad() 33" );

	b_keypad_clear = 0;													//flag a clear is req
	return;
	}
//-------	
	




//------- these MUST be before 'b_keypad_clear'	action further below

if( which == 20 )														//'KHz'
	{
//	s1 = ow->miwp_tune->miw->value();
	double d0;
	sscanf( s1.c_str(), "%lf", &d0 );
	d0 *= 1e3;
	ow->miwp_tune->miw->set_value_from_double( d0 );

	g_freq_sub_tune = 0;
	wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( g_freq_sub_tune );

	wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_bt_keypad() 20" );
	b_keypad_clear = 1;													//flag a clear is req

//	strpf( s1, "%d", (int)gph_mouse_freq );
//	wnd_rtl_graph->idb_tune->add_at_head( s1.c_str() );
//	wnd_rtl_graph->idb_tune->trim_entry_count_to( cn_tune_history_dropdown_max, 0 );
	return;
	}


if( which == 21 )														//'MHz'
	{
//	s1 = ow->miwp_tune->miw->value();
	double d0;
	sscanf( s1.c_str(), "%lf", &d0 );
	d0 *= 1e6;
	ow->miwp_tune->miw->set_value_from_double( d0 );

	g_freq_sub_tune = 0;
	wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( g_freq_sub_tune );

	wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_bt_keypad() 21" );

	b_keypad_clear = 1;													//flag a clear is req
	return;
	}


if( which == 22 )														//'GHz'
	{
//	s1 = ow->miwp_tune->miw->value();
	double d0;
	sscanf( s1.c_str(), "%lf", &d0 );
	d0 *= 1e9;
	ow->miwp_tune->miw->set_value_from_double( d0 );

	g_freq_sub_tune = 0;
	wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( g_freq_sub_tune );

	wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_bt_keypad() 22" );

	b_keypad_clear = 1;													//flag a clear is req
	return;
	}
//-------	










if( b_keypad_clear )
	{
	ow->miwp_tune->miw->value_text_only( "0" );
	b_keypad_clear = 0;
	}


s1 = ow->miwp_tune->miw->value();


if( s1.compare("0") == 0 ) s1 = "";										//drop a solitary zero
if( which == 0 )
	{
	s1 += "0";
	ow->miwp_tune->miw->value_text_only( s1.c_str() );
	return;
	}

if( which == 1 )
	{
	s1 += "1";
	ow->miwp_tune->miw->value_text_only( s1.c_str() );
	return;
	}

if( which == 2 )
	{
	s1 += "2";
	ow->miwp_tune->miw->value_text_only( s1.c_str() );
	return;
	}

if( which == 3 )
	{
	s1 += "3";
	ow->miwp_tune->miw->value_text_only( s1.c_str() );
	return;
	}

if( which == 4 )
	{
	s1 += "4";
	ow->miwp_tune->miw->value_text_only( s1.c_str() );
	return;
	}

if( which == 5 )
	{
	s1 += "5";
	ow->miwp_tune->miw->value_text_only( s1.c_str() );
	return;
	}

if( which == 6 )
	{
	s1 += "6";
	ow->miwp_tune->miw->value_text_only( s1.c_str() );
	return;
	}

if( which == 7 )
	{
	s1 += "7";
	ow->miwp_tune->miw->value_text_only( s1.c_str() );
	return;
	}

if( which == 8 )
	{
	s1 += "8";
	ow->miwp_tune->miw->value_text_only( s1.c_str() );
	return;
	}

if( which == 9 )
	{
	s1 += "9";
	ow->miwp_tune->miw->value_text_only( s1.c_str() );
	return;
	}

if( which == 10 )														//'.'
	{
	s1 += ".";
	ow->miwp_tune->miw->value_text_only( s1.c_str() );
	return;
	}

if( which == 11 )														//'C'
	{
	ow->miwp_tune->miw->value_text_only( "0" );
	b_keypad_clear = 0;
	return;
	}



}








void cb_bt_carrier_max_tune_exclusion_combo( Fl_Widget *w, void *v )
{
string s1;
int which = (intptr_t)v;




if( which == 0 )		//add
	{
		
	wnd_rtl_graph->carrier_max_exclusion( 1, g_freq_tune );
	
	}


if( which == 1 )		//remove
	{
	wnd_rtl_graph->carrier_max_exclusion( 0, g_freq_tune );
	}


if( which == 2 )		//clear
	{
	vcarrier_max_exclusion.clear();
	}

}








//do a single scan between two freq
void cb_bt_freq_single( Fl_Widget *, void * )
{

if( !wnd_rtl_graph ) return;

cb_bt_freq_stop( 0, 0 );

imode = en_mode_scan_mode_single;



wnd_rtl_graph->add_to_scan_history();

//do_scan();

//cb_bt_freq_tune( 0, 0 );
wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_bt_freq_single()" );

return;
}










//read gui ctrl vals and set various global vars
void get_user_gui_control_params()
{
double freq_tune = 0;
double freq_start = 410000;
double freq_stop = 490000;
double freq_step = 1000;
double freq_gain= 100;
//double dev_bandwidth = 2000000;
double dev_bandwidth = 2496000;
double freq_reso = 8192;
double threshold = 0;

double bw_lower;
double bw_upper;
bool b_if_freq;
double interfreq;
bool b_bandwidth_limit;
int tuning_offset;
float gain_iq;
int direct_sampling;
bool bias_t;
float audgain;
int rtl_srate_filter;


get_user_params( g_params );


//downsample_srate_pending = g_params.i_dwn_srate;

if( downsample_srate_pending != g_params.i_dwn_srate)					//changed?
	{
	unsigned int dwn_srate_in = g_params.i_dwn_srate;
	bool b_set_dwnsrate = 1;
	bool b_adj_gui_ctrl = 1;
	dnwsrate_nearest_factor( dwn_srate_in, b_set_dwnsrate, b_adj_gui_ctrl );
	}


g_freq_start = g_params.freq_start;
g_freq_stop = g_params.freq_stop;
g_freq_step = g_params.freq_step;


g_freq_tune = g_params.freq_tune;

g_freq_sub_tune = g_params.freq_sub_tune;

g_b_dwn_aa = g_params.b_dwn_aa;



g_b_bw_bpass = g_params.b_bw_limit;
g_bw_lower = g_params.bw_lower;
g_bw_upper = g_params.bw_upper;
g_bw_taps = g_params.bw_taps;

g_b_if_freq = g_params.b_if_freq;
g_interfreq = g_params.interfreq;



g_dev_bw = g_params.dev_bandwidth;

//printf("get_user_gui_control_params() - PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP g_dev_bw %f\n", g_dev_bw );
//printf("get_user_gui_control_params() - PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP downsample_srate_pending %d\n", downsample_srate_pending );

g_dev_gain = g_params.freq_gain;
g_tuning_offset = g_params.tuning_offset;

g_gain_iq = g_params.gain_iq;
g_dev_direct_sampling = g_params.direct_sampling;
g_dev_bias_t = g_params.bias_t;

b_dc_block_iq = g_params.b_dcblk;


//g_rtl_srate_filter = g_params.rtl_srate_filter;

b_agc = g_params.b_agc;
g_gain_aud = g_params.audgain;

b_clip_enable = g_params.b_clip_enable;
}
















void listen_old_delete( double freq_in )
{
printf( "listen_old_delete()\n" );
string s1;

if( !wnd_rtl_graph ) return;

//wnd_rtl_graph->add_to_tune_history();

get_user_gui_control_params();
/*
double freq_tune = 0;
double freq_start = 410000;
double freq_stop = 490000;
double freq_step = 1000;
double freq_gain= 100;
double bandwidth = 2000000;
double freq_reso = 8192;
double threshold = 0;

double bw_lower;
double bw_upper;
double interfreq;
bool bandwidth_limit;

get_user_params( freq_tune, freq_start, freq_stop, freq_step, freq_gain, bandwidth, freq_reso, threshold, bw_lower, bw_upper, interfreq, bandwidth_limit  );

g_bw_lower = bw_lower;
g_bw_upper = bw_upper;
g_interfreq = interfreq;
g_rtl_bw = bandwidth;
g_bw_limit = bandwidth_limit;
g_tuner_gain = freq_gain;
*/

//rtl_listen_stop();

rtl.set_direct_sampling( g_dev_direct_sampling );

rtl.set_gain( g_dev_gain );

//rtl.set_srate( bandwidth );


imode = en_mode_listen;
rtl_listen( freq_in );
}





void listen_using_globals()
{

/*
g_freq_tune = freq_tune;
g_bw_lower = bw_lower;
g_bw_upper = bw_upper;
g_b_if_freq = b_if_freq;
g_interfreq = interfreq;
g_device_bw = dev_bandwidth;
g_b_bw_limit = b_bandwidth_limit;
g_dev_gain = freq_gain;
g_tuning_offset = tuning_offset;

g_gain_iq = gain_iq;
g_direct_sampling = direct_sampling;
g_bias_t = bias_t;
*/



imode = en_mode_listen;
rtl.set_center_freq( g_freq_tune );
rtl.set_gain( g_dev_gain );
rtl.set_srate( g_dev_bw );

rtl.set_direct_sampling( g_dev_direct_sampling );
rtl.set_offset_tuning( g_dev_offset_tuning );

}














void cb_fi_freq_gain( Fl_Widget *w, void *v )
{
get_user_gui_control_params();
printf( "cb_fi_freq_gain() - %f\n", g_dev_gain );

rtl.set_gain( g_dev_gain );

float fgain = rtl.get_gain();

printf( "cb_fi_freq_gain() - gain setting returned from rtl %f\n", fgain );

}





/*
void cb_bt_tune_history( Fl_Widget *, void * )
{
string s1;
double frq;


if( !wnd_rtl_graph ) return;

Fl_Menu_Button *m = wnd_rtl_graph->fi_tune->menubutton();

if( m->value() < 0 ) return;

//check if callback was due to a menu change, ie: ignore keyboard text change triggered callbacks
if( wnd_rtl_graph->last_tune_history_value != m->value() )		//has menu item been change?
	{
	
	s1 = wnd_rtl_graph->fi_tune->value();
	sscanf( s1.c_str(), "%lf", &frq );

	
	wnd_rtl_graph->freq_listen( 0, 0, 0, 0 );
	wnd_rtl_graph->last_tune_history_value = m->value();		//remember for next callabck

	wnd_rtl_graph->miwp_tune->miw->set_value_from_double( frq );
	}

}
*/





void cb_miw_if_freq( Fl_Widget *w, void *v )
{
string s1;

My_Input_Wheel *wdj = (My_Input_Wheel*)w;
rtl_graph_wnd *o = (rtl_graph_wnd*) v;

int ii = wdj->get_value_as_double();
printf( "cb_miw_if_freq() - %d\n", ii );

wnd_rtl_graph->freq_listen( 0, 0, 0, 0, "cb_miw_if_freq()" );

}
















void cb_bt_freq_tune_down_up( Fl_Widget *w, void *v )
{
string s1;
int frq;
int which = (intptr_t)v;

int step;

bool bset_from_num = 0;

frq = g_freq_tune;
if( which == 0 )
	{
	frq -= g_dev_bw/2.0f/disp_spect_zoom_factor;						//v1.04
	
	frq /= 1000;														//quantise
	frq *= 1000;
	bset_from_num = 1;
	}
	
if( which == 1 )
	{
	frq += g_dev_bw/2.0f/disp_spect_zoom_factor;						//v1.04

	frq /= 1000;
	frq *= 1000;
	bset_from_num = 1;
	}

if( which == 2 )
	{
	if( frq < 1e5 )
		{
		frq /= 1e4;
		frq *= 1e4;
		}
	else{
		frq /= 1e5;
		frq *= 1e5;
		}

	if( frq < 5e9 ) step = 100e6; 
	if( frq <= 100e6 ) step = 10e6; 
	if( frq <= 10e6 ) step = 1e6; 
	if( frq <= 1e6 ) step = 1e5; 
	if( frq <= 1e5 ) step = 1e4; 
	frq -= step;
	bset_from_num = 1;
	}


if( which == 3 )
	{
	if( frq < 1e5 )
		{
		frq /= 1e4;
		frq *= 1e4;
		}
	else{
		frq /= 1e5;
		frq *= 1e5;
		}
	if( frq < 5e9 ) step = 100e6; 
	if( frq < 100e6 ) step = 10e6; 
	if( frq < 10e6 ) step = 1e6; 
	if( frq < 1e6 ) step = 1e5; 
	if( frq < 1e5 ) step = 1e4; 
	frq += step;
	bset_from_num = 1;
	}

if( which == 10 )
	{
	s1 = wnd_rtl_graph->miwp_tune->miw->value();
	int len = s1.length();
	if( len > 1 )
		{
		bool done = 0;
printf( "cb_bt_freq_tune_down_up()000 - '%s' %d\n", s1.c_str(), len );
		if( s1[len - 1] != '0' ) { s1[len - 1] = '0'; done = 1; }
		if( !done) if( s1[len - 2] != '0' ) { s1[len - 2] = '0'; done = 1; }
		if( !done) if( s1[len - 3] != '0' ) { s1[len - 3] = '0'; done = 1; }
		if( !done) if( s1[len - 4] != '0' ) { s1[len - 4] = '0'; done = 1; }
		if( !done) if( s1[len - 5] != '0' ) { s1[len - 5] = '0'; done = 1; }
		if( !done) if( s1[len - 6] != '0' ) { s1[len - 6] = '0'; done = 1; }
		
printf( "cb_bt_freq_tune_down_up()111 - '%s' %d\n", s1.c_str(), len );
		if( done ) wnd_rtl_graph->miwp_tune->miw->value( s1 );
		}

	}



if( which == 20 )
	{
	frq -= 5000;
	bset_from_num = 1;
	}




if( which == 21 )
	{
	frq += 5000;
	bset_from_num = 1;
	}


if( which == 30 )														//round down
	{
	int i0 = frq / 1000;												//drop last 3 digits
	
	i0 -= 1;
	i0 *= 1000;
	
	frq = i0;
	bset_from_num = 1;
	}



if( which == 31 )														//round up
	{
	int i0 = frq / 1000;												//drop last 3 digits
	
	i0 += 1;
	i0 *= 1000;											
	
	frq = i0;
	bset_from_num = 1;
	}


if( bset_from_num) wnd_rtl_graph->miwp_tune->miw->set_value_from_double( frq );
//cb_bt_freq_tune( 0, 0 );
wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_bt_freq_tune_down_up()" );

//strpf( s1, "%d", frq );
//wnd_rtl_graph->idb_tune->add_at_head( s1.c_str() );
//wnd_rtl_graph->idb_tune->trim_entry_count_to( cn_tune_history_dropdown_max, 0 );

}







//clears multiplier char if any incase it's in the middle of a number,
//leaving the alpha char in the middle of the entered number may affect decimal point interpretation 
void sanitise_miw_number(  My_Input_Wheel *w )
{
string s1;
mystr m1;

m1 = w->value();

m1.FindReplace( s1, "k", "", 0 );
m1 = s1;
m1.FindReplace( s1, "m", "", 0 );
m1 = s1;
m1.FindReplace( s1, "g", "", 0 );
m1 = s1;

m1.FindReplace( s1, "K", "", 0 );
m1 = s1;
m1.FindReplace( s1, "M", "", 0 );
m1 = s1;
m1.FindReplace( s1, "G", "", 0 );

w->value_text_only( s1.c_str() );
//w->set_value_from_str( s1 );
}





void cb_miwp_tune_keydown( My_Input_Wheel *w, void *args )
{
string s1;
mystr m1;

int which = (intptr_t)args;
 
bool bdont_clear = 0;



if( w->keycode_last == FL_BackSpace ) bdont_clear = 1; 
if( w->keycode_last == FL_Enter ) bdont_clear = 1;
if( w->keycode_last == FL_Delete ) bdont_clear = 1;
if( w->keycode_last == FL_Left ) bdont_clear = 1;
if( w->keycode_last == FL_Right ) bdont_clear = 1;
if( w->keycode_last == FL_Home ) bdont_clear = 1;
if( w->keycode_last == FL_End ) bdont_clear = 1;

if( bdont_clear ) b_keypad_clear = 0;

printf( "cb_miwp_tune_keydown() - keycode_last %d\n", w->keycode_last );



if( ( b_keypad_clear == 1 ) )
	{
	w->value_text_only( "" );
	b_keypad_clear = 0;
	}

//	sanitise_miw_number( w );


}








void cb_miwp_tune_keyup( My_Input_Wheel *w, void *args )
{
string s1;
mystr m1;



//double dd = w->get_value_as_double();
//w->set_value_from_double( dd );





if( w->keycode_last == 'k' )
	{
	sanitise_miw_number( w );
	
//	printf("------------------------------pressed key %x  '%c'\n", w->keycode_last, w->keycode_last );
	double dd = w->get_value_as_double();
	
	dd *= 1e3;
	w->set_value_from_double( dd );

	g_freq_sub_tune = 0;
	wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( g_freq_sub_tune );

	wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_miwp_tune_keyup() k" );

	b_keypad_clear = 1;

//	strpf( s1, "%d", (int)dd );
//	wnd_rtl_graph->idb_tune->add_at_head( s1.c_str() );
//	wnd_rtl_graph->idb_tune->trim_entry_count_to( cn_tune_history_dropdown_max, 0 );
	}


if( w->keycode_last == 'm' )
	{
	sanitise_miw_number( w );

	double dd = w->get_value_as_double();
	
	dd *= 1e6;
	w->set_value_from_double( dd );

	g_freq_sub_tune = 0;
	wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( g_freq_sub_tune );

	wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_miwp_tune_keyup() m" );

	b_keypad_clear = 1;
	}


if( w->keycode_last == 'g' )
	{
	sanitise_miw_number( w );

	double dd = w->get_value_as_double();
	
	dd *= 1e9;
	w->set_value_from_double( dd );

	g_freq_sub_tune = 0;
	wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( g_freq_sub_tune );

	wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_miwp_tune_keyup() g" );

	b_keypad_clear = 1;
	
	}
	

if( w->keycode_last == FL_Enter )
	{
	sanitise_miw_number( w );

	strpf( s1, "%d", (int)g_freq_tune );
	wnd_rtl_graph->idb_tune->add_at_head( s1.c_str() );
	wnd_rtl_graph->idb_tune->trim_entry_count_to( cn_tune_history_dropdown_max, 0 );

	b_keypad_clear = 1;
	}




}








void cb_miwp_tune( Fl_Widget *w, void *v )
{
string s1;

My_Input_Wheel_Packable *wdj = (My_Input_Wheel_Packable*)w;
rtl_graph_wnd *o = (rtl_graph_wnd*) v;

int ii = wdj->miw->get_value_as_double();


				
				
printf( "cb_miwp_tune() - %d\n", ii );

//strpf( s1, "%d", ii );


//wnd_rtl_graph->fi_tune->value( s1.c_str() );
//listen( ii );

g_freq_sub_tune = 0;
wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( g_freq_sub_tune );


if( wdj->b_last_change_was_by_dragging )
	{
	wnd_rtl_graph->freq_listen( 0, 0, 0, 0, "cb_miwp_tune()" );			//if slider used don't add to tune history
//	printf( "cb_miwp_tune() - wdj->b_last_change_was_by_dragging %d\n", wdj->b_last_change_was_by_dragging );
	}
else{
	wnd_rtl_graph->freq_listen( 0, 0, 0, 0, "cb_miwp_tune()" );
	}

//wnd_rtl_graph->idb_tune->add_at_head( s1.c_str() );
//wnd_rtl_graph->idb_tune->trim_entry_count_to( cn_tune_history_dropdown_max, 0 );

//wnd_rtl_graph->last_tune_history_value = m->value();		//remember for next callabck
}




void cb_bt_freq_store( Fl_Widget *w, void *v )
{
string s1;

//s1 = wnd_rtl_graph->miwp_tune->miw->value();

//double dd;
//sscanf( s1.c_str(), "%lf", &dd );

int freq_tot = (int)g_freq_tune + (int)g_freq_sub_tune;

strpf( s1, "%d,%d,%d", freq_tot, (int)g_freq_tune, (int)g_freq_sub_tune );

wnd_rtl_graph->idb_tune->value( s1 );
wnd_rtl_graph->add_to_tune_history_skip_duplicates();

//wnd_rtl_graph->add_to_tune_history2();

//wnd_rtl_graph->idb_tune->vstr.push_back( "zzz");
}





















void cb_miw_freq_sub_tune( Fl_Widget *w, void *v )
{
string s1;

My_Input_Wheel *wdj = (My_Input_Wheel*)w;
rtl_graph_wnd *o = (rtl_graph_wnd*) v;

g_freq_sub_tune = wdj->get_value_as_double();
//g_freq_sub_tune = wdj->get_value_as_double();

printf( "cb_miw_freq_sub_tune() - g_freq_sub_tune %d\n", g_freq_sub_tune );
}








void cb_miw_dbg0( Fl_Widget *w, void *v )
{
string s1;

My_Input_Wheel *wdj = (My_Input_Wheel*)w;
rtl_graph_wnd *o = (rtl_graph_wnd*) v;

g_dbg0 = wdj->get_value_as_double();

aud_dim_time = g_dbg0;
//g_b_user_iir_notch0 = g_dbg0;

printf( "cb_iw_dbg0() - %d\n", g_dbg0 );
}






void cb_miw_dbg1( Fl_Widget *w, void *v )
{
string s1;

My_Input_Wheel *wdj = (My_Input_Wheel*)w;
rtl_graph_wnd *o = (rtl_graph_wnd*) v;

g_dbg1 = wdj->get_value_as_double();

//st_filt[ en_ftid_iir_notch0_aud ].fc0 = g_dbg1;
//filter_iir_notch_adjust( en_ftid_iir_notch0_aud, g_user_iir_notch_freq0, g_user_iir_notch_Q_0 );

printf( "cb_miw_dbg1() - %d\n", g_dbg1 );
}







void cb_miw_dbg2( Fl_Widget *w, void *v )
{
string s1;

My_Input_Wheel *wdj = (My_Input_Wheel*)w;
rtl_graph_wnd *o = (rtl_graph_wnd*) v;

g_dbg2 = wdj->get_value_as_double();

//g_user_iir_notch_Q_0 = g_dbg2;
//filter_iir_notch_adjust( en_ftid_iir_notch0_aud, g_user_iir_notch_freq0, g_user_iir_notch_Q_0 );

printf( "cb_miw_dbg2() - %d\n", g_dbg2 );
}









void cb_miw_dbg3( Fl_Widget *w, void *v )
{
string s1;

My_Input_Wheel *wdj = (My_Input_Wheel*)w;
rtl_graph_wnd *o = (rtl_graph_wnd*) v;

g_dbg3 = wdj->get_value_as_double();
printf( "cb_miw_dbg3() - %d\n", g_dbg3 );
}













void cb_miw_dbg4( Fl_Widget *w, void *v )
{
string s1;

My_Input_Wheel *wdj = (My_Input_Wheel*)w;
rtl_graph_wnd *o = (rtl_graph_wnd*) v;

g_dbg4 = wdj->get_value_as_double();
printf( "cb_miw_dbg4() - %d\n", g_dbg4 );
}







void cb_miw_dbg5( Fl_Widget *w, void *v )
{
string s1;

My_Input_Wheel *wdj = (My_Input_Wheel*)w;
rtl_graph_wnd *o = (rtl_graph_wnd*) v;

g_dbg5 = wdj->get_value_as_double();
printf( "cb_miw_dbg5() - %d\n", g_dbg5 );
}






void cb_miw_freq_gain( Fl_Widget *w, void *v )
{
string s1;

My_Input_Wheel *wdj = (My_Input_Wheel*)w;
rtl_graph_wnd *o = (rtl_graph_wnd*) v;

float ff = wdj->get_value_as_double();
printf( "cb_miw_freq_gain() - %f\n", ff );

get_user_gui_control_params();
printf( "cb_miw_freq_gain() - %f\n", g_dev_gain );

o->freq_listen( 1, 1, 1, 1, "cb_miw_freq_gain()" );

//rtl.set_gain( g_dev_gain );

//float fgain = rtl.get_gain();

//printf( "cb_miw_freq_gain() - gain setting returned from rtl %f\n", fgain );

}















void cb_ck_user_dc_block_iq( Fl_Widget *w, void *v )
{
Fl_Check_Button *ow = (Fl_Check_Button *)w;


b_dc_block_iq = ow->value();

//cb_bt_freq_tune( 0, 0 );
//wnd_rtl_graph->freq_listen( 1, 0, 0, 0 );
}






void cb_ck_user_dwn_aa( Fl_Widget *w, void *v )
{
Fl_Check_Button *ow = (Fl_Check_Button *)w;


g_b_dwn_aa = ow->value();

//cb_bt_freq_tune( 0, 0 );
//wnd_rtl_graph->freq_listen( 1, 0, 0, 0 );
}




void cb_ck_user_lpf0( Fl_Widget *w, void *v )
{
Fl_Check_Button *ow = (Fl_Check_Button *)w;


g_b_user_iir_lpf0 = ow->value();

//cb_bt_freq_tune( 0, 0 );
//wnd_rtl_graph->freq_listen( 1, 0, 0, 0 );
}






void cb_ck_user_hpf0( Fl_Widget *w, void *v )
{
Fl_Check_Button *ow = (Fl_Check_Button *)w;


g_b_user_iir_hpf0 = ow->value();

//cb_bt_freq_tune( 0, 0 );
//wnd_rtl_graph->freq_listen( 1, 0, 0, 0 );
}










void cb_miw_user_iir_hpf0( Fl_Widget *w, void *v )
{
string s1;

My_Input_Wheel *wdj = (My_Input_Wheel*)w;
rtl_graph_wnd *o = (rtl_graph_wnd*) v;

g_user_iir_hpf0 = wdj->get_value_as_double();
printf( "cb_miw_user_iir_hpf0() - %d\n", g_user_iir_hpf0 );

filter_iir0_adjust( g_user_iir_lpf0, g_user_iir_hpf0 );

//get_user_gui_control_params();

//o->freq_listen( 1, 1, 1, 1 );

//rtl.set_gain( g_dev_gain );

//float fgain = rtl.get_gain();

//printf( "cb_miw_freq_gain() - gain setting returned from rtl %f\n", fgain );

}







void cb_miw_user_iir_lpf0( Fl_Widget *w, void *v )
{
string s1;

My_Input_Wheel *wdj = (My_Input_Wheel*)w;
rtl_graph_wnd *o = (rtl_graph_wnd*) v;

g_user_iir_lpf0 = wdj->get_value_as_double();
printf( "cb_miw_user_iir_lpf0() - %d\n", g_user_iir_lpf0 );

filter_iir0_adjust( g_user_iir_lpf0, g_user_iir_hpf0 );

//get_user_gui_control_params();

//o->freq_listen( 1, 1, 1, 1 );

//rtl.set_gain( g_dev_gain );

//float fgain = rtl.get_gain();

//printf( "cb_miw_freq_gain() - gain setting returned from rtl %f\n", fgain );

}









void cb_ck_user_lpf1( Fl_Widget *w, void *v )
{
Fl_Check_Button *ow = (Fl_Check_Button *)w;


g_b_user_iir_lpf1 = ow->value();

//cb_bt_freq_tune( 0, 0 );
//wnd_rtl_graph->freq_listen( 1, 0, 0, 0 );
}






void cb_ck_user_lpf2( Fl_Widget *w, void *v )
{
Fl_Check_Button *ow = (Fl_Check_Button *)w;


g_b_user_iir_lpf2 = ow->value();

//cb_bt_freq_tune( 0, 0 );
//wnd_rtl_graph->freq_listen( 1, 0, 0, 0 );
}







void cb_miw_user_iir_lpf1( Fl_Widget *w, void *v )
{
string s1;

My_Input_Wheel *wdj = (My_Input_Wheel*)w;
rtl_graph_wnd *o = (rtl_graph_wnd*) v;

g_user_iir_lpf1 = wdj->get_value_as_double();
printf( "cb_miw_user_iir_lpf1() - %d\n", g_user_iir_lpf1 );

filter_iir1_adjust( g_user_iir_lpf1 );
}










void cb_miw_user_iir_lpf2( Fl_Widget *w, void *v )
{
string s1;

My_Input_Wheel *wdj = (My_Input_Wheel*)w;
rtl_graph_wnd *o = (rtl_graph_wnd*) v;

g_user_iir_lpf2 = wdj->get_value_as_double();
printf( "cb_miw_user_iir_lpf2() - %d\n", g_user_iir_lpf2 );

filter_iir2_adjust( g_user_iir_lpf2 );
}















void cb_miw_tuning_offset( Fl_Widget *w, void *v )
{
string s1;

My_Input_Wheel *wdj = (My_Input_Wheel*)w;
rtl_graph_wnd *o = (rtl_graph_wnd*) v;

int ff = wdj->get_value_as_double();
printf( "cb_miw_tuning_offset() - %d\n", ff );


//cb_bt_freq_tune( 0, 0 );
wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_miw_tuning_offset()" );
}







unsigned int dnwsrate_nearest_factor( unsigned int dwn_srate_in, bool b_set_dwnsrate, bool b_adj_gui_ctrl )
{
	
int dnw_factor = nearbyint( g_dev_bw / dwn_srate_in );												

int dwn_srate = g_dev_bw / dnw_factor;

printf( "dnwsrate_nearest_factor() - PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP\n" );

printf( "dnwsrate_nearest_factor() - g_dev_bw: %f  dnw_factor %d  dwn_srate_in %d  dwn_srate %d\n", g_dev_bw, dnw_factor, dwn_srate_in, dwn_srate );




if( b_adj_gui_ctrl ) wnd_rtl_graph->miw_dev_dwnconv_srate->set_value_from_double( dwn_srate );

//if( b_call_freq_listen ) wnd_rtl_graph->freq_listen( 0, 0, 0, 0, "dnwsrate_nearest_factor()" );

if( b_set_dwnsrate ) 
	{
	downsample_srate_pending = dwn_srate;
	b_need_dwn_srate_change = 1;
	}


return dwn_srate;
}






												
void cb_miw_dev_dwnconv_srate( Fl_Widget *w, void *v )
{
string s1;

My_Input_Wheel *wdj = (My_Input_Wheel*)w;
rtl_graph_wnd *o = (rtl_graph_wnd*) v;

int ii = wdj->get_value_as_double();
printf( "cb_miw_dev_dwnconv_srate() - %d\n", ii );

int dnw_factor = nearbyint( g_dev_bw / ii );												


int dwn_srate = g_dev_bw / dnw_factor;

printf( "cb_miw_dev_dwnconv_srate() - g_dev_bw: %f  dnw_factor %d  dwn_srate %d\n", g_dev_bw, dnw_factor, dwn_srate );


strpf( s1, "Will use nearest practical downsample srate (so it is an integer factor of device bandwidth setting DevBW)\nAbout to adjust filters and restart various threads, this will interrupt audio momentarily.\n\nContinue setting downsampler's o/p srate to: %d ?", dwn_srate );
int ret = fl_choice( s1.c_str(), "Cancel", "Set dwnconv srate", 0 );
if( ret == 1 )
	{
		
	
//	stop_audio();
//	stop_threads();

//	downsample_srate_pending = dwn_srate;
	
	wdj->set_value_from_double( dwn_srate );

//	b_need_dwn_srate_change = 1;										//set 'downsample_srate_pending' with new srate before setting this

//	i_fftw_trig_plan_create_state = 0;									//start the transition state going which will create require fftw plans
	
//	filter_adjust_fir_bw_lower_upper();

//	start_threads_rtl();
//	start_audio();

	wnd_rtl_graph->freq_listen( 0, 0, 0, 0, "cb_miw_dev_dwnconv_srate()" );
	}
else{
	wdj->set_value_from_double( downsample_srate );						//restore unchanged srate num
	}
}




				
				
				
				
					

void cb_miw_dev_bwidth( Fl_Widget *w, void *v )
{
string s1;

My_Input_Wheel *wdj = (My_Input_Wheel*)w;
rtl_graph_wnd *o = (rtl_graph_wnd*) v;

int ii = wdj->get_value_as_double();
printf( "\n\ncb_miw_dev_bwidth() - %d\n", ii );


aud_dimmer();

wnd_rtl_graph->set_dev_bwidth( ii );


unsigned int dwn_srate_in = downsample_srate;
bool b_set_dwnsrate = 1;
bool b_adj_gui_ctrl = 1;
dnwsrate_nearest_factor( dwn_srate_in, b_set_dwnsrate, b_adj_gui_ctrl );



//b_need_filter_rebuild = 1;
//bneed_hzoom_and_center_to_cur_freq = 5;
//nhac_hzoom_factor = 1.0f;
//b_need_h_zoom_recalc = 1;

wnd_rtl_graph->centre_graph( 5, 1 );

//rtl.set_bandwidth( ii );



//cb_bt_freq_tune( 0, 0 );

}









void cb_miw_ppm_offset_dev( Fl_Widget *w, void *v )
{
string s1;

My_Input_Wheel *wdj = (My_Input_Wheel*)w;
rtl_graph_wnd *o = (rtl_graph_wnd*) v;

int ii = wdj->get_value_as_double();
printf( "cb_miw_ppm_offset_dev() - %d\n", ii );

rtl.set_ppm( ii );
//cb_bt_freq_tune( 0, 0 );
}






void cb_miw_gph_scaley( Fl_Widget *w, void *v )
{
string s1;

My_Input_Wheel *wdj = (My_Input_Wheel*)w;
rtl_graph_wnd *o = (rtl_graph_wnd*) v;

float ff = wdj->get_value_as_double();
printf( "cb_miw_gph_scaley() - %f\n", ff );

gph_scaley = ff;
}









void cb_ck_graphs_enable( Fl_Widget *w, void *v )
{
Fl_Check_Button *ow = (Fl_Check_Button *)w;


b_dbg_no_graph_update = !ow->value();
}







void cb_ck_fit_auto_plot( Fl_Widget *w, void *v )
{
Fl_Check_Button *ow = (Fl_Check_Button *)w;


b_fit_auto_plot = ow->value();
}





void cb_miw_disp_spect_zoom_factor( Fl_Widget *w, void *v )
{
string s1;

My_Input_Wheel *wdj = (My_Input_Wheel*)w;
rtl_graph_wnd *o = (rtl_graph_wnd*) v;

float ff = wdj->get_value_as_double();
printf( "cb_miw_disp_spect_zoom_factor() - %f\n", ff );

disp_spect_zoom_factor = ff;
}











void spect_avg( unsigned int avg_wnd )
{
printf( "spect_avg() - avg_wnd %d\n", avg_wnd );


if( avg_wnd == 0 )
	{
	b_spect_average = 0;
	printf( "spect_avg() - avg turned offn" );
	return;
	}

if( avg_wnd >= cn_spec_avg_slots_max )
	{
	printf( "spect_avg() - avg was out of range\n" );
	return;
	}

ispec_avg_wnd = avg_wnd;
b_spect_average = 1;

printf( "spect_avg() - avg set to %d\n", avg_wnd );
}















void cb_miw_spect_avg( Fl_Widget *w, void *v )
{
string s1;

My_Input_Wheel *wdj = (My_Input_Wheel*)w;
rtl_graph_wnd *o = (rtl_graph_wnd*) v;


ispec_avg_wnd = wdj->get_value_as_double();

printf( "cb_miw_spect_avg() - ispec_avg_wnd %d\n", ispec_avg_wnd );

spect_avg( ispec_avg_wnd );

//if( ispec_avg_wnd == 0 ) b_spect_average = 0;
//else b_spect_average = 1;



}




void cb_miw_carrier_max_tune_threshold( Fl_Widget *w, void *v )
{
string s1;

My_Input_Wheel *wdj = (My_Input_Wheel*)w;
rtl_graph_wnd *o = (rtl_graph_wnd*) v;

g_carrier_max_tune_threshold = wdj->get_value_as_double();
printf( "cb_miw_carrier_max_tune_threshold() - %f\n", g_carrier_max_tune_threshold );

}




void cb_bt_favourite(Fl_Widget* w, void *v)
{
string s1;
int which = (intptr_t)v;

printf("cb_bt_favourite() - which %d\n", which );

if( which == 0 )
	{
	wnd_fav->hide();
	wnd_fav->fl_scrol->scroll_to( 0, 0 );
	wnd_fav->show();
	}

if( which == 1 )
	{
	wnd_rtl_graph->favourite_add( 1 );
	}
}





/*

void cb_bt_fav_store(Fl_Widget* w, void *v)
{
string s1;

printf("bt_fav_store()\n" );

wnd_fav->hide();
wnd_fav->fl_scrol->scroll_to( 0, 0 );
wnd_fav->show();

if( wnd_fav->row_sel >= 0 )
	{
	strpf( s1, "Store current tuning to the sel favourite [idx %d] ?", wnd_fav->row_sel );
	int ret = fl_choice( s1.c_str(), "Cancel", "Overwrite Fav", 0 );
	if( ret == 1 )
		{
		wnd_rtl_graph->favourite_add();
		}
	}
else{
	strpf( s1, "Select a location in favourite list first." );
	fl_alert( s1.c_str(), 0 );
	}
}
*/









//sets the buttons 'sname' var to 'ss' and extracts button's label by striping label before '::' then set button's label
//'ss' e.g: 40Mtr::7e6  will set label to 40Mtr, and ignore double colon and freq 
void btw_band_set_name( unsigned int idx, string &ss )
{
mystr m1;
string s1;

if( idx >= cn_btw_band_max ) return;

cl_button_wheel *ow = wnd_rtl_graph->btw_band[idx];


m1 = ss;
m1.cut_at_first_find( s1, "::", 0 );
ow->copy_label( s1.c_str() );
ow->sname = ss;
ow->value(0);	
}










void cb_btw_band(Fl_Widget* w, void *v)
{
mystr m1;
string s1, s2;

int which = (intptr_t) v;

if( w != 0 )
	{
	cl_button_wheel *ow = (cl_button_wheel*)w;

	printf("cb_btw_band() - which %d\n", which );

	if( ow->left_button )
		{
		printf( "cb_btw_band() - left_button\n" );
		m1 = ow->sname;
		m1.cut_at_end_of_first_find_and_keep_right( s1, "::", 0 );
		if( s1.length() > 0 )
			{
			double d0;
			
			sscanf( s1.c_str(), "%lf", &d0 );
			wnd_rtl_graph->miwp_tune->miw->set_value_from_double( d0 );
			wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_btw_band()" );
			}
		else{
			strpf( s1, "cb_btw_band() - which %d, 'sname' had no valid floating point number after double colon: '%s'\n", which, s1.c_str() );
			fl_alert( s1.c_str(), 0 );
			return;
			}
		}


	if( ow->right_button )
		{
		printf( "cb_btw_band() - right_button\n" );

		strpf( s1, "Enter label::freq, eg: 40Mtr::7e6 or 40M::7010000" );
		strpf( s2, "%s", ow->sname.c_str() );

		char *sz = fl_input( s1.c_str(), s2.c_str() );

		int	iv;

		if( sz != 0 )
			{
			s1 = sz;
			btw_band_set_name( which, s1 );
			
//			m1 = sz;
//			m1.cut_at_first_find( s1, "::", 0 );
//			ow->copy_label( s1.c_str() );
//			ow->sname = sz;
			ow->right_button = 0;										//clear this as button fails to do it, possibly because 'fl_input()' took focus
			ow->value(0);	
			}
		else{
			ow->right_button = 0;										//clear this as button fails to do it, possibly because 'fl_input()' took focus
			ow->value(0);	
			return;
			}
		}
	}
}






void cb_bt_graph_fit(Fl_Widget* w, void *v)
{

if( w != 0 )
	{
	cl_button_wheel *ow = (cl_button_wheel*)w;

	if( ow->b_mousewheel_changed )
		{
		printf( "cb_bt_graph_fit() - ow->b_mousewheel_changed %d", ow->b_mousewheel_changed );
		
		double extrme_y = gph1.fit_plot_get_extremey( 0 );
		if( extrme_y < 1e-99 ) extrme_y = 1.0f;
		
		if( ow->mousewheel < 0 ) extrme_y *= 1.25;
		else extrme_y /= 1.25;
		
		gph1.fit_plot_to_this_y_range( 0, -extrme_y, extrme_y );

//		gph1.hide();
//		gph1.show();
		return;
		}
	}


gph1.fit_plot( 0 );
gph1.hide();
gph1.show();

}









void cb_ch_graph_loc_sel(Fl_Widget* w, void *v)
{
//int which = (intptr_t)v;

Fl_Choice *o = (Fl_Choice*) w;

gph_loc_sel = o->value();

printf("ch_graph_loc_sel()0 - gph_loc_sel %d\n", gph_loc_sel );

gph_loc_sel_tmp = gph_loc_sel;
gph_loc_sel = 1;


printf("ch_graph_loc_sel()1 - gph_loc_sel %d   gph_loc_sel_tmp %d\n", gph_loc_sel, gph_loc_sel_tmp );


Fl_Menu_* mw = (Fl_Menu_*)w;
const Fl_Menu_Item* m = mw->mvalue();

if (!m) 
	{
	printf("cb_ch_graph_loc_sel() - NULL\n");
	}
else{
	
	vgph1_x.clear();
	vgph1_y0.clear();

	for( int i = 0; i < 50; i++ )
		{
		vgph1_x.push_back( i );
		if( i&1 ) vgph1_y0.push_back( 0.01 );							//plot a default triangle wfm incase no probe location signal is avail
		else vgph1_y0.push_back( -0.01 );
		}

	gph1.plot_vfloat( 0, vgph1_y0 );

//	if( b_fit_auto_plot ) gph1.fit_plot( 0 );

	if( m->shortcut() )
		{
		printf("cb_ch_graph_loc_sel() - %s - %s\n", m->label(), fl_shortcut_label( m->shortcut() ));
		}
	else
		{
		printf("cb_ch_graph_loc_sel() - %s\n", m->label());
		}
	}
	
gph1.hide();
gph1.show();

if( b_fit_auto_plot) 
	{
	wnd_rtl_graph->ineed_graph_fit = 7;
	wnd_rtl_graph->bneed_graph_fit_bring_for_front = 1;
	}
}






void cb_idb_tune( void* obj, void* args )
{
input_dropbox* o = (input_dropbox*) obj;

string s1 = "??";

if( o->hover_idx < o->vstr.size() )
	{
	s1 = o->vstr[o->hover_idx];
	printf("cb_idb_tune() - input_dropbox o->hover_idx %d  '%s'\n", o->hover_idx, s1.c_str() );

	int freq_tot, frq_tune, frq_sub_tune;
	sscanf( s1.c_str(), "%d,%d,%d", &freq_tot, &frq_tune, &frq_sub_tune );

//	wnd_rtl_graph->fi_tune->value( o->value() );
	wnd_rtl_graph->miwp_tune->miw->set_value_from_double( frq_tune );
	wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( frq_sub_tune );

//	cb_bt_freq_tune(0,0);
	
	wnd_rtl_graph->freq_listen( 0, 0, 0, 0, "cb_idb_tune()" );

//	listen( frq );
	}
}





void cb_idb_tune_fi_text( void* w, void* v )
{
Fl_Input* o = (Fl_Input*) w;

printf("cb_idb_tune_fi_text() - '%s'\n", o->value() );


double frq;
sscanf(  o->value(), "%lf", &frq );


int offset = 0;

wnd_rtl_graph->miwp_tune->miw->set_value_from_double( frq - offset );
wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( offset );


//wnd_rtl_graph->add_to_tune_history_skip_duplicates();
//listen( frq );

//cb_bt_freq_tune(0,0);
wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_idb_tune_fi_text()" );

}







void cb_bt_about(Fl_Widget *, void *)
{
string s1, st;

Fl_Window *wnd = new Fl_Window(wnd_rtl_graph->x()+20,wnd_rtl_graph->y()+20,500,200);
wnd->label("About");
Fl_Input *teText = new Fl_Input(10,10,wnd->w()-20,wnd->h()-20,"");
teText->type(FL_MULTILINE_OUTPUT);
teText->textsize(12);

strpf( s1, "%s,  %s,  Built: %s\n", cnsAppWndName, cns_version, cns_build_date );
st += s1;


strpf( s1, "\nSoftware defined radio app for Linux..." );
st += s1;

strpf( s1, "\n\nAudio SRate: %d, Frame Size: %d.", aud_op_srate, framecnt );
st += s1;

strpf( s1, "\nRTL DevName: '%s',   Manufacturer: '%s'", s_dev_name.c_str(), s_dev_manufact.c_str() );
st += s1;

teText->value(st.c_str());
wnd->end();

#ifndef compile_for_windows
wnd->set_modal();
#endif

wnd->show();

}











void cb_bt_synth_dlg_combo( Fl_Widget *w, void *v )
{
mystr m1;
string s1;
int which = (intptr_t)v;

//---------
if( which == 0 )
	{
	synth_dlg->hide();
	}


if( which == 1000 )
	{
	synth_carr_lsb_usb_fm_0 = 1;
	}

if( which == 1001 )
	{
	synth_carr_lsb_usb_fm_0 = 2;
	}
if( which == 1002 )
	{
	synth_carr_lsb_usb_fm_0 = 3;
	}

if( which == 1003 )
	{
	synth_carr_lsb_usb_fm_0 = 4;
	}
	
if( which == 1004 )
	{
	synth_carr_lsb_usb_fm_0 = 5;
	}

if( which == 1005 )
	{
	synth_carr_lsb_usb_fm_0 = 6;
	}

if( which == 1006 )
	{
	synth_carr_lsb_usb_fm_0 = 0;
	}



if( which == 1010 )
	{
	synth_morse_tone0 = 0;
	}

if( which == 1011 )
	{
	synth_morse_tone0 = 1;
	}

if( which == 1012 )
	{
	synth_morse_tone0 = 2;
	}

if( which == 1013 )
	{
	synth_morse_tone0 = 3;
	}

if( which == 1014 )
	{
	synth_morse_tone0 = 4;
	}

if( which == 1015 )
	{
	synth_morse_tone0 = 5;
	}

if( which == 1016 )
	{
	synth_morse_tone0 = 6;
	}

if( which == 1017 )
	{
	synth_morse_tone0 = 7;
	}

if( which == 1018 )
	{
	synth_morse_tone0 = 8;
	}

if( which == 1019 )
	{
	synth_morse_tone0 = 9;
	}
//---------



//---------
if( which == 10 )
	{
	synth_carr_lsb_usb_fm_1 = 1;
	}

if( which == 11 )
	{
	synth_carr_lsb_usb_fm_1 = 2;
	}
if( which == 12 )
	{
	synth_carr_lsb_usb_fm_1 = 3;
	}

if( which == 13 )
	{
	synth_carr_lsb_usb_fm_1 = 4;
	}
	
if( which == 14 )
	{
	synth_carr_lsb_usb_fm_1 = 0;
	}
	
	

if( which == 100 )
	{
	synth_morse_tone1 = 0;
	}

if( which == 101 )
	{
	synth_morse_tone1 = 1;
	}

if( which == 102 )
	{
	synth_morse_tone1 = 2;
	}

if( which == 103 )
	{
	synth_morse_tone1 = 3;
	}

if( which == 104 )
	{
	synth_morse_tone1 = 4;
	}

if( which == 105 )
	{
	synth_morse_tone1 = 5;
	}

if( which == 106 )
	{
	synth_morse_tone1 = 6;
	}

if( which == 107 )
	{
	synth_morse_tone1 = 7;
	}
//---------




//---------
if( which == 20 )
	{
	synth_carr_lsb_usb_fm_2 = 1;
	}

if( which == 21 )
	{
	synth_carr_lsb_usb_fm_2 = 2;
	}
if( which == 22 )
	{
	synth_carr_lsb_usb_fm_2 = 3;
	}

if( which == 23 )
	{
	synth_carr_lsb_usb_fm_2 = 4;
	}
	
if( which == 24 )
	{
	synth_carr_lsb_usb_fm_2 = 0;
	}




if( which == 200 )
	{
	synth_morse_tone2 = 0;
	}

if( which == 201 )
	{
	synth_morse_tone2 = 1;
	}

if( which == 202 )
	{
	synth_morse_tone2 = 2;
	}

if( which == 203 )
	{
	synth_morse_tone2 = 3;
	}

if( which == 204 )
	{
	synth_morse_tone2 = 4;
	}

if( which == 205 )
	{
	synth_morse_tone2 = 5;
	}

if( which == 206 )
	{
	synth_morse_tone2 = 6;
	}

if( which == 207 )
	{
	synth_morse_tone2 = 7;
	}
//---------



//---------
if( which == 30 )
	{
	synth_carr_lsb_usb_fm_3 = 1;
	}

if( which == 31 )
	{
	synth_carr_lsb_usb_fm_3 = 2;
	}
if( which == 32 )
	{
	synth_carr_lsb_usb_fm_3 = 3;
	}

if( which == 33 )
	{
	synth_carr_lsb_usb_fm_3 = 4;
	}
	
if( which == 34 )
	{
	synth_carr_lsb_usb_fm_3 = 0;
	}
	




if( which == 300 )
	{
	synth_morse_tone3 = 0;
	}

if( which == 301 )
	{
	synth_morse_tone3 = 1;
	}

if( which == 302 )
	{
	synth_morse_tone3 = 2;
	}

if( which == 303 )
	{
	synth_morse_tone3 = 3;
	}

if( which == 304 )
	{
	synth_morse_tone3 = 4;
	}

if( which == 305 )
	{
	synth_morse_tone3 = 5;
	}

if( which == 306 )
	{
	synth_morse_tone3 = 6;
	}

if( which == 307 )
	{
	synth_morse_tone3 = 7;
	}
//---------




if( which == 9990 )
	{
	wnd_rtl_graph->b_synth_noise_on_iq = !wnd_rtl_graph->b_synth_noise_on_iq;
	}


}









void cb_miw_synth_dlg( Fl_Widget *w, void *v )
{
int which = (intptr_t)v;

if( which == 1000 )
	{
	synth_carr_freq0 = miw_synth_dlg_carr_freq0->get_value_as_double();
	
	init_osc_integrators( 0, 1 );
	}


if( which == 1001 )
	{
	synth_pilot_freq0 = miw_synth_dlg_pilot_freq0->get_value_as_double();
	init_osc_integrators( 0, 1 );
	}

if( which == 1002 )
	{
	synth_pilot_gain0 = miw_synth_dlg_pilot_gain0->get_value_as_double();
	}

if( which == 1003 )
	{
	synth_tone_freq0 = miw_synth_dlg_tone_freq0->get_value_as_double();
	init_osc_integrators( 0, 1 );
	}

if( which == 1004 )
	{
	synth_tone_dcoffs0 = miw_synth_dlg_dcoffs0->get_value_as_double();
	}


if( which == 10 )
	{
	synth_carr_freq1 = miw_synth_dlg_carr_freq1->get_value_as_double();
	
	init_osc_integrators( 0, 1 );
	}
	

if( which == 11 )
	{
	synth_tone_freq1 = miw_synth_dlg_tone_freq1->get_value_as_double();
	init_osc_integrators( 0, 1 );
	}




if( which == 20 )
	{
	synth_carr_freq2 = miw_synth_dlg_carr_freq2->get_value_as_double();
	init_osc_integrators( 0, 1 );
	}


if( which == 21 )
	{
	synth_tone_freq2 = miw_synth_dlg_tone_freq2->get_value_as_double();
	init_osc_integrators( 0, 1 );
	}




if( which == 30 )
	{
	synth_carr_freq3 = miw_synth_dlg_carr_freq3->get_value_as_double();
	init_osc_integrators( 0, 1 );
	}


if( which == 31 )
	{
	synth_tone_freq3 = miw_synth_dlg_tone_freq3->get_value_as_double();
	init_osc_integrators( 0, 1 );
	}




if( which == 40 )
	{
	synth_carr_freq4 = miw_synth_dlg_carr_freq4->get_value_as_double();
	init_osc_integrators( 0, 1 );
	}


if( which == 41 )
	{
	synth_tone_freq4 = miw_synth_dlg_tone_freq4->get_value_as_double();
	init_osc_integrators( 0, 1 );
	}


if( which == 9990 )
	{
	synth_noise_gain = miw_synth_dlg_noise_gain->get_value_as_double();
	}

}











void make_synth_dlg_wnd()
{
if( synth_dlg != 0 )
	{
	synth_dlg->hide();	
	synth_dlg->show();
	return;	
	}

synth_dlg = new Fl_Window( 50, 50, 800, 500, "Synthesiser Modulated Carriers" );

Fl_Button *bt_synth_dlg_close = new Fl_Button( synth_dlg->w() - 65, synth_dlg->h() - 23, 50, 18, "Close" );
bt_synth_dlg_close->labelsize( 8 );
bt_synth_dlg_close->callback( cb_bt_synth_dlg_combo, 0 );

//----------------------------------------------------------------------
miw_synth_dlg_carr_freq0 = new My_Input_Wheel( 70, 20, 70, 15, "freq0" );
My_Input_Wheel *omiw = miw_synth_dlg_carr_freq0;
omiw->labelsize(10);
omiw->textsize(9);
omiw->align(FL_ALIGN_LEFT);
omiw->tooltip( "adjust carrier freq" );
omiw->b_show_modified = 1;
omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
omiw->col_update();
omiw->b_take_focus_on_inside = 0;
omiw->b_use_limit_min = 1;
omiw->limit_min =  0.001;
omiw->b_use_limit_max = 1;
omiw->limit_max = 100e3;

omiw->b_invert_wheel = 0;				
omiw->std_wheel_step = 100;
omiw->ctrl_wheel_step = 500;
omiw->shift_wheel_step = 1000;
omiw->ctrl_shift_wheel_step = 10;

omiw->s_printf_format = "%.3f";
omiw->force_integer = 0;
omiw->set_value_from_double( synth_carr_freq0 );
omiw->set_callback( (void*)cb_miw_synth_dlg, (void*)synth_dlg, (void*)1000 );
omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
omiw->id2= 0;
omiw->allow_right_but_drag = 1;
omiw->right_drag_x_val_change_factor = 0;
omiw->right_drag_y_val_change_factor = 10;

//------
Fl_Group *gp1 = new Fl_Group( 70, 40, 152, 132, "" );

Fl_Round_Button *bt = new Fl_Round_Button( gp1->x() + 2, gp1->y() + 2, 70, 17, "AM" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "AM" );
bt->callback( cb_bt_synth_dlg_combo, (void*)1000 );
bt->value(1);

bt = new Fl_Round_Button( gp1->x() + 2, gp1->y() + 22, 70, 17, "LwrSB" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "AM - lower sideband" );
bt->callback( cb_bt_synth_dlg_combo, (void*)1001 );
bt->type(FL_RADIO_BUTTON);

bt = new Fl_Round_Button( gp1->x() + 2, gp1->y() + 42, 70, 17, "UprSB" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "AM - upper sideband" );
bt->callback( cb_bt_synth_dlg_combo, (void*)1002 );

bt = new Fl_Round_Button( gp1->x() + 2, gp1->y() + 62, 70, 17, "FM" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "FM" );
bt->callback( cb_bt_synth_dlg_combo, (void*)1003 );


bt = new Fl_Round_Button( gp1->x() + 2, gp1->y() + 82, 70, 17, "FM Stereo" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "FM Stereo with two audio tones" );
bt->callback( cb_bt_synth_dlg_combo, (void*)1004 );


bt = new Fl_Round_Button( gp1->x() + 2, gp1->y() + 102, 70, 17, "FM Stereo(no Pilot)" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "FM Stereo with two audio tones (no pilot tone)" );
bt->callback( cb_bt_synth_dlg_combo, (void*)1005 );


bt = new Fl_Round_Button( gp1->x() + 2, gp1->y() + 122, 70, 17, "off" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "disable this modulated carrier" );
bt->callback( cb_bt_synth_dlg_combo, (void*)1006 );



miw_synth_dlg_pilot_freq0 = new My_Input_Wheel( gp1->x() + 79, gp1->y() + 48, 45, 15, "PilotFreq" );
omiw = miw_synth_dlg_pilot_freq0;
omiw->labelsize(9);
omiw->textsize(9);
omiw->align(FL_ALIGN_TOP);
omiw->tooltip( "adjust FM signal's stereo pilot freq, normally 19KHz" );
omiw->b_show_modified = 1;
omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
omiw->col_update();
omiw->b_take_focus_on_inside = 0;
omiw->b_use_limit_min = 1;
omiw->limit_min =  1.0f;
omiw->b_use_limit_max = 1;
omiw->limit_max = 480e3;

omiw->b_invert_wheel = 0;				
omiw->std_wheel_step = 10;
omiw->ctrl_wheel_step = 100;
omiw->shift_wheel_step = 0.1;
omiw->ctrl_shift_wheel_step = 1000;

omiw->s_printf_format = "%.2f";
omiw->force_integer = 0;
omiw->set_value_from_double( synth_pilot_freq0 );
omiw->set_callback( (void*)cb_miw_synth_dlg, (void*)synth_dlg, (void*)1001 );
omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
omiw->id2= 0;
omiw->allow_right_but_drag = 1;
omiw->right_drag_x_val_change_factor = 0;
omiw->right_drag_y_val_change_factor = 10;


miw_synth_dlg_pilot_gain0 = new My_Input_Wheel( gp1->x() + 79, gp1->y() + 83, 45, 15, "PilotLvl" );
omiw = miw_synth_dlg_pilot_gain0;
omiw->labelsize(9);
omiw->textsize(9);
omiw->align(FL_ALIGN_TOP);
omiw->tooltip( "adjust FM signal's stereo pilot level, 0.09 is 9% deviation" );
omiw->b_show_modified = 1;
omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
omiw->col_update();
omiw->b_take_focus_on_inside = 0;
omiw->b_use_limit_min = 1;
omiw->limit_min =  0.0f;
omiw->b_use_limit_max = 1;
omiw->limit_max = 10.0;

omiw->b_invert_wheel = 0;				
omiw->std_wheel_step = 0.001;
omiw->ctrl_wheel_step = 0.05;
omiw->shift_wheel_step = 0.1;
omiw->ctrl_shift_wheel_step = 0.5;

omiw->s_printf_format = "%.3f";
omiw->force_integer = 0;
omiw->set_value_from_double( synth_pilot_gain0 );
omiw->set_callback( (void*)cb_miw_synth_dlg, (void*)synth_dlg, (void*)1002 );
omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
omiw->id2= 0;
omiw->allow_right_but_drag = 1;
omiw->right_drag_x_val_change_factor = 0;
omiw->right_drag_y_val_change_factor = 10;

gp1->end();
//------




//------
Fl_Group *gp11 = new Fl_Group( 70, 202, 100, 282, "" );

bt = new Fl_Round_Button( gp11->x() + 2, gp11->y() + 2, 90, 17, "morse brief" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "short morse code with long silent break" );
bt->callback( cb_bt_synth_dlg_combo, (void*)1010 );

bt = new Fl_Round_Button( gp11->x() + 2, gp11->y() + 22, 90, 17, "morse long" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "morse code containing a long message" );
bt->callback( cb_bt_synth_dlg_combo, (void*)1011 );

bt = new Fl_Round_Button( gp11->x() + 2, gp11->y() + 42, 90, 17, "voice0" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "use voice as modulating signal" );
bt->callback( cb_bt_synth_dlg_combo, (void*)1012 );

bt = new Fl_Round_Button( gp11->x() + 2, gp11->y() + 62, 90, 17, "voice1" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "use voice as modulating signal" );
bt->callback( cb_bt_synth_dlg_combo, (void*)1013 );

bt = new Fl_Round_Button( gp11->x() + 2, gp11->y() + 82, 90, 17, "tone swept" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "use tone as modulating signal" );
bt->callback( cb_bt_synth_dlg_combo, (void*)1014 );

bt = new Fl_Round_Button( gp11->x() + 2, gp11->y() + 102, 90, 17, "tone warble" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "use tone as modulating signal" );
bt->callback( cb_bt_synth_dlg_combo, (void*)1015 );


bt = new Fl_Round_Button( gp11->x() + 2, gp11->y() + 122, 90, 17, "tone adj" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "use adjustable tone as modulating signal, change freq below" );
bt->callback( cb_bt_synth_dlg_combo, (void*)1016 );
bt->value(1);


bt = new Fl_Round_Button( gp11->x() + 2, gp11->y() + 142, 70, 17, "stereo tones" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "use two adj tones for FM Stereo testing" );
bt->callback( cb_bt_synth_dlg_combo, (void*)1017 );


bt = new Fl_Round_Button( gp11->x() + 2, gp11->y() + 162, 70, 17, "stereo voices" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "usetwo voices for FM Stereo testing" );
bt->callback( cb_bt_synth_dlg_combo, (void*)1018 );


bt = new Fl_Round_Button( gp11->x() + 2, gp11->y() + 182, 70, 17, "off" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "disable modulation of carrier" );
bt->callback( cb_bt_synth_dlg_combo, (void*)1019 );






miw_synth_dlg_tone_freq0 = new My_Input_Wheel( gp11->x() + 2, gp11->y() + 202, 70, 15, "tone frq" );
omiw = miw_synth_dlg_tone_freq0;
omiw->labelsize(10);
omiw->textsize(9);
omiw->align(FL_ALIGN_LEFT);
omiw->tooltip( "modulating tone's freq" );
omiw->b_show_modified = 1;
omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
omiw->col_update();
omiw->b_take_focus_on_inside = 0;
omiw->b_use_limit_min = 1;
omiw->limit_min = 1;
omiw->b_use_limit_max = 1;
omiw->limit_max = 20e3;

omiw->b_invert_wheel = 0;				
omiw->std_wheel_step = 10;
omiw->ctrl_wheel_step = 100;
omiw->shift_wheel_step = 500;
omiw->ctrl_shift_wheel_step = 10;

omiw->s_printf_format = "%.3f";
omiw->force_integer = 0;
omiw->set_value_from_double( synth_tone_freq0 );
omiw->set_callback( (void*)cb_miw_synth_dlg, (void*)synth_dlg, (void*)1003 );
omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
omiw->id2= 1;
omiw->allow_right_but_drag = 1;
omiw->right_drag_x_val_change_factor = 0;
omiw->right_drag_y_val_change_factor = 10;





miw_synth_dlg_dcoffs0 = new My_Input_Wheel( gp11->x() + 2, gp11->y() + 222, 70, 15, "dc" );
omiw = miw_synth_dlg_dcoffs0;
omiw->labelsize(10);
omiw->textsize(9);
omiw->align(FL_ALIGN_LEFT);
omiw->tooltip( "add a dc offset" );
omiw->b_show_modified = 1;
omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
omiw->col_update();
omiw->b_take_focus_on_inside = 0;
omiw->b_use_limit_min = 1;
omiw->limit_min = 0.0;
omiw->b_use_limit_max = 1;
omiw->limit_max = 20e3;

omiw->b_invert_wheel = 0;				
omiw->std_wheel_step = 0.01;
omiw->ctrl_wheel_step = 0.05;
omiw->shift_wheel_step = 0.1;
omiw->ctrl_shift_wheel_step = 0.25;

omiw->s_printf_format = "%.3f";
omiw->force_integer = 0;
omiw->set_value_from_double( synth_tone_dcoffs0 );
omiw->set_callback( (void*)cb_miw_synth_dlg, (void*)synth_dlg, (void*)1004 );
omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
omiw->id2= 1;
omiw->allow_right_but_drag = 1;
omiw->right_drag_x_val_change_factor = 0;
omiw->right_drag_y_val_change_factor = 10;




Fl_Check_Button *ck = new Fl_Check_Button( gp11->x() + 2, gp11->y() + 252, 70, 17, "Noise" );
ck->labelsize(12);
ck->tooltip( "add noise to synthesised signals" );
ck->callback( cb_bt_synth_dlg_combo, (void*)9990 );
ck->value( wnd_rtl_graph->b_synth_noise_on_iq );



miw_synth_dlg_noise_gain = new My_Input_Wheel( gp11->x() + 2, gp11->y() + 272, 70, 15, "noise gain" );
omiw = miw_synth_dlg_noise_gain;
omiw->labelsize(10);
omiw->textsize(9);
omiw->align(FL_ALIGN_LEFT);
omiw->tooltip( "adjusts noise level" );
omiw->b_show_modified = 1;
omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
omiw->col_update();
omiw->b_take_focus_on_inside = 0;
omiw->b_use_limit_min = 0;
omiw->limit_min = 0.0f;
omiw->b_use_limit_max = 0;
omiw->limit_max = 10.0f;

omiw->b_invert_wheel = 0;				
omiw->std_wheel_step = 0.05f;
omiw->ctrl_wheel_step = 0.1f;
omiw->shift_wheel_step = 0.25f;
omiw->ctrl_shift_wheel_step = 1.0f;

omiw->s_printf_format = "%.3f";
omiw->force_integer = 0;
omiw->set_value_from_double( synth_noise_gain );
omiw->set_callback( (void*)cb_miw_synth_dlg, (void*)synth_dlg, (void*)9990 );
omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
omiw->id2= 1;
omiw->allow_right_but_drag = 1;
omiw->right_drag_x_val_change_factor = 0;
omiw->right_drag_y_val_change_factor = 10;



gp11->end();
//------
//----------------------------------------------------------------------












//----------------------------------------------------------------------
miw_synth_dlg_carr_freq1 = new My_Input_Wheel( 200, 20, 70, 15, "freq1" );
omiw = miw_synth_dlg_carr_freq1;
omiw->labelsize(10);
omiw->textsize(9);
omiw->align(FL_ALIGN_LEFT);
omiw->tooltip( "adjust carrier freq" );
omiw->b_show_modified = 1;
omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
omiw->col_update();
omiw->b_take_focus_on_inside = 0;
omiw->b_use_limit_min = 1;
omiw->limit_min = 0.001;
omiw->b_use_limit_max = 1;
omiw->limit_max = 100e3;

omiw->b_invert_wheel = 0;				
omiw->std_wheel_step = 100;
omiw->ctrl_wheel_step = 500;
omiw->shift_wheel_step = 1000;
omiw->ctrl_shift_wheel_step = 10;

omiw->s_printf_format = "%.3f";
omiw->force_integer = 0;
omiw->set_value_from_double( synth_carr_freq1 );
omiw->set_callback( (void*)cb_miw_synth_dlg, (void*)synth_dlg, (void*)10 );
omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
omiw->id2= 1;
omiw->allow_right_but_drag = 1;
omiw->right_drag_x_val_change_factor = 0;
omiw->right_drag_y_val_change_factor = 10;



//------
Fl_Group *gp2 = new Fl_Group( 200, 40, 50, 120, "" );

bt = new Fl_Round_Button( gp2->x() + 2, gp2->y() + 2, 70, 17, "AM" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "AM" );
bt->callback( cb_bt_synth_dlg_combo, (void*)10 );
bt->value(1);

bt = new Fl_Round_Button( gp2->x() + 2, gp2->y() + 22, 70, 17, "LwrSB" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "AM - lower sideband" );
bt->callback( cb_bt_synth_dlg_combo, (void*)11 );
bt->type(FL_RADIO_BUTTON);

bt = new Fl_Round_Button( gp2->x() + 2, gp2->y() + 42, 70, 17, "UprSB" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "AM - upper sideband" );
bt->callback( cb_bt_synth_dlg_combo, (void*)12 );

bt = new Fl_Round_Button( gp2->x() + 2, gp2->y() + 62, 70, 17, "FM" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "FM" );
bt->callback( cb_bt_synth_dlg_combo, (void*)13 );

bt = new Fl_Round_Button( gp2->x() + 2, gp2->y() + 82, 70, 17, "off" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "disable this modulated carrier" );
bt->callback( cb_bt_synth_dlg_combo, (void*)14 );

gp2->end();
//------



//------
Fl_Group *gp21 = new Fl_Group( 200, 202, 100, 242, "" );

bt = new Fl_Round_Button( gp21->x() + 2, gp21->y() + 2, 90, 17, "morse brief" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "short morse code with long silent break" );
bt->callback( cb_bt_synth_dlg_combo, (void*)100 );

bt = new Fl_Round_Button( gp21->x() + 2, gp21->y() + 22, 90, 17, "morse long" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "morse code containing a long message" );
bt->callback( cb_bt_synth_dlg_combo, (void*)101 );

bt = new Fl_Round_Button( gp21->x() + 2, gp21->y() + 42, 90, 17, "voice0" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "use voice as modulating signal" );
bt->callback( cb_bt_synth_dlg_combo, (void*)102 );
bt->value(1);

bt = new Fl_Round_Button( gp21->x() + 2, gp21->y() + 62, 90, 17, "voice1" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "use voice as modulating signal" );
bt->callback( cb_bt_synth_dlg_combo, (void*)103 );

bt = new Fl_Round_Button( gp21->x() + 2, gp21->y() + 82, 90, 17, "tone swept" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "use tone as modulating signal" );
bt->callback( cb_bt_synth_dlg_combo, (void*)104 );

bt = new Fl_Round_Button( gp21->x() + 2, gp21->y() + 102, 90, 17, "tone warble" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "use tone as modulating signal" );
bt->callback( cb_bt_synth_dlg_combo, (void*)105 );


bt = new Fl_Round_Button( gp21->x() + 2, gp21->y() + 122, 90, 17, "tone" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "use adjustable tone as modulating signal, change freq below" );
bt->callback( cb_bt_synth_dlg_combo, (void*)106 );


bt = new Fl_Round_Button( gp21->x() + 2, gp21->y() + 142, 90, 17, "off" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "turn off modulation" );
bt->callback( cb_bt_synth_dlg_combo, (void*)107 );


miw_synth_dlg_tone_freq1 = new My_Input_Wheel( gp21->x() + 2, gp21->y() + 202, 70, 15, "tone frq" );
omiw = miw_synth_dlg_tone_freq1;
omiw->labelsize(10);
omiw->textsize(9);
omiw->align(FL_ALIGN_LEFT);
omiw->tooltip( "modulating tone's freq" );
omiw->b_show_modified = 1;
omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
omiw->col_update();
omiw->b_take_focus_on_inside = 0;
omiw->b_use_limit_min = 1;
omiw->limit_min = 1;
omiw->b_use_limit_max = 1;
omiw->limit_max = 20e3;

omiw->b_invert_wheel = 0;				
omiw->std_wheel_step = 10;
omiw->ctrl_wheel_step = 100;
omiw->shift_wheel_step = 500;
omiw->ctrl_shift_wheel_step = 10;

omiw->s_printf_format = "%.3f";
omiw->force_integer = 0;
omiw->set_value_from_double( synth_tone_freq1 );
omiw->set_callback( (void*)cb_miw_synth_dlg, (void*)synth_dlg, (void*)11 );
omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
omiw->id2= 1;
omiw->allow_right_but_drag = 1;
omiw->right_drag_x_val_change_factor = 0;
omiw->right_drag_y_val_change_factor = 10;

gp21->end();
//------
//----------------------------------------------------------------------





//----------------------------------------------------------------------
miw_synth_dlg_carr_freq2 = new My_Input_Wheel( 330, 20, 70, 15, "freq2" );
omiw = miw_synth_dlg_carr_freq2;
omiw->labelsize(10);
omiw->textsize(9);
omiw->align(FL_ALIGN_LEFT);
omiw->tooltip( "adjust carrier freq" );
omiw->b_show_modified = 1;
omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
omiw->col_update();
omiw->b_take_focus_on_inside = 0;
omiw->b_use_limit_min = 1;
omiw->limit_min = 0.001;
omiw->b_use_limit_max = 1;
omiw->limit_max = 100e3;

omiw->b_invert_wheel = 0;				
omiw->std_wheel_step = 100;
omiw->ctrl_wheel_step = 500;
omiw->shift_wheel_step = 1000;
omiw->ctrl_shift_wheel_step = 10;

omiw->s_printf_format = "%.3f";
omiw->force_integer = 0;
omiw->set_value_from_double( synth_carr_freq2 );
omiw->set_callback( (void*)cb_miw_synth_dlg, (void*)synth_dlg, (void*)30 );
omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
omiw->id2= 2;
omiw->allow_right_but_drag = 1;
omiw->right_drag_x_val_change_factor = 0;
omiw->right_drag_y_val_change_factor = 10;

//------
Fl_Group *gp3 = new Fl_Group( 330, 40, 50, 120, "" );

bt = new Fl_Round_Button( gp3->x() + 2, gp3->y() + 2, 70, 17, "AM" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "AM" );
bt->callback( cb_bt_synth_dlg_combo, (void*)20 );
bt->value(1);

bt = new Fl_Round_Button( gp3->x() + 2, gp3->y() + 22, 70, 17, "LwrSB" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "AM - lower sideband" );
bt->callback( cb_bt_synth_dlg_combo, (void*)21 );
bt->type(FL_RADIO_BUTTON);

bt = new Fl_Round_Button( gp3->x() + 2, gp3->y() + 42, 70, 17, "UprSB" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "AM - upper sideband" );
bt->callback( cb_bt_synth_dlg_combo, (void*)22 );

bt = new Fl_Round_Button( gp3->x() + 2, gp3->y() + 62, 70, 17, "FM" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "FM" );
bt->callback( cb_bt_synth_dlg_combo, (void*)23 );

bt = new Fl_Round_Button( gp3->x() + 2, gp3->y() + 82, 70, 17, "off" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "disable this modulated carrier" );
bt->callback( cb_bt_synth_dlg_combo, (void*)24 );

gp3->end();
//------



//------
Fl_Group *gp31 = new Fl_Group( 330, 202, 100, 242, "" );


bt = new Fl_Round_Button( gp31->x() + 2, gp31->y() + 2, 90, 17, "morse brief" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "short morse code with long silent break" );
bt->callback( cb_bt_synth_dlg_combo, (void*)200 );

bt = new Fl_Round_Button( gp31->x() + 2, gp31->y() + 22, 90, 17, "morse long" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "morse code containing a long message" );
bt->callback( cb_bt_synth_dlg_combo, (void*)201 );

bt = new Fl_Round_Button( gp31->x() + 2, gp31->y() + 42, 90, 17, "voice0" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "voice clip from file" );
bt->callback( cb_bt_synth_dlg_combo, (void*)202 );

bt = new Fl_Round_Button( gp31->x() + 2, gp31->y() + 62, 90, 17, "voice1" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "voice clip from file" );
bt->callback( cb_bt_synth_dlg_combo, (void*)203 );
bt->value(1);

bt = new Fl_Round_Button( gp31->x() + 2, gp31->y() + 82, 90, 17, "tone swept" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "use tone as modulating signal" );
bt->callback( cb_bt_synth_dlg_combo, (void*)204 );

bt = new Fl_Round_Button( gp31->x() + 2, gp31->y() + 102, 90, 17, "tone warble" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "use tone as modulating signal" );
bt->callback( cb_bt_synth_dlg_combo, (void*)205 );

bt = new Fl_Round_Button( gp31->x() + 2, gp31->y() + 122, 90, 17, "tone" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "use adjustable tone as modulating signal, change freq below" );
bt->callback( cb_bt_synth_dlg_combo, (void*)206 );

bt = new Fl_Round_Button( gp31->x() + 2, gp31->y() + 142, 90, 17, "off" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "turn off modulation" );
bt->callback( cb_bt_synth_dlg_combo, (void*)207 );


miw_synth_dlg_tone_freq2 = new My_Input_Wheel( gp31->x() + 2, gp31->y() + 202, 70, 15, "tone frq" );
omiw = miw_synth_dlg_tone_freq2;
omiw->labelsize(10);
omiw->textsize(9);
omiw->align(FL_ALIGN_LEFT);
omiw->tooltip( "modulating tone's freq" );
omiw->b_show_modified = 1;
omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
omiw->col_update();
omiw->b_take_focus_on_inside = 0;
omiw->b_use_limit_min = 1;
omiw->limit_min = 1;
omiw->b_use_limit_max = 1;
omiw->limit_max = 20e3;

omiw->b_invert_wheel = 0;				
omiw->std_wheel_step = 10;
omiw->ctrl_wheel_step = 100;
omiw->shift_wheel_step = 500;
omiw->ctrl_shift_wheel_step = 10;

omiw->s_printf_format = "%.3f";
omiw->force_integer = 0;
omiw->set_value_from_double( synth_tone_freq2 );
omiw->set_callback( (void*)cb_miw_synth_dlg, (void*)synth_dlg, (void*)31 );
omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
omiw->id2= 1;
omiw->allow_right_but_drag = 1;
omiw->right_drag_x_val_change_factor = 0;
omiw->right_drag_y_val_change_factor = 10;

gp31->end();
//------
//----------------------------------------------------------------------








//----------------------------------------------------------------------
miw_synth_dlg_carr_freq3 = new My_Input_Wheel( 460, 20, 70, 15, "freq3" );
omiw = miw_synth_dlg_carr_freq3;
omiw->labelsize(10);
omiw->textsize(9);
omiw->align(FL_ALIGN_LEFT);
omiw->tooltip( "adjust carrier freq" );
omiw->b_show_modified = 1;
omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
omiw->col_update();
omiw->b_take_focus_on_inside = 0;
omiw->b_use_limit_min = 1;
omiw->limit_min = 0.001;
omiw->b_use_limit_max = 1;
omiw->limit_max = 100e3;

omiw->b_invert_wheel = 0;				
omiw->std_wheel_step = 100;
omiw->ctrl_wheel_step = 500;
omiw->shift_wheel_step = 1000;
omiw->ctrl_shift_wheel_step = 10;

omiw->s_printf_format = "%.3f";
omiw->force_integer = 0;
omiw->set_value_from_double( synth_carr_freq3 );
omiw->set_callback( (void*)cb_miw_synth_dlg, (void*)synth_dlg, (void*)30 );
omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
omiw->id2= 2;
omiw->allow_right_but_drag = 1;
omiw->right_drag_x_val_change_factor = 0;
omiw->right_drag_y_val_change_factor = 10;

//------
Fl_Group *gp4 = new Fl_Group( 460, 40, 50, 120, "" );

bt = new Fl_Round_Button( gp4->x() + 2, gp4->y() + 2, 70, 17, "AM" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "AM" );
bt->callback( cb_bt_synth_dlg_combo, (void*)30 );
bt->value(1);

bt = new Fl_Round_Button( gp4->x() + 2, gp4->y() + 22, 70, 17, "LwrSB" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "AM - lower sideband" );
bt->callback( cb_bt_synth_dlg_combo, (void*)31 );
bt->type(FL_RADIO_BUTTON);

bt = new Fl_Round_Button( gp4->x() + 2, gp4->y() + 42, 70, 17, "UprSB" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "AM - upper sideband" );
bt->callback( cb_bt_synth_dlg_combo, (void*)32 );

bt = new Fl_Round_Button( gp4->x() + 2, gp4->y() + 62, 70, 17, "FM" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "FM" );
bt->callback( cb_bt_synth_dlg_combo, (void*)33 );

bt = new Fl_Round_Button( gp4->x() + 2, gp4->y() + 82, 70, 17, "off" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "disable this modulated carrier" );
bt->callback( cb_bt_synth_dlg_combo, (void*)34 );

gp4->end();
//------



//------
Fl_Group *gp41 = new Fl_Group( 460, 202, 100, 242, "" );


bt = new Fl_Round_Button( gp41->x() + 2, gp41->y() + 2, 90, 17, "morse brief" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "short morse code with long silent break" );
bt->callback( cb_bt_synth_dlg_combo, (void*)300 );

bt = new Fl_Round_Button( gp41->x() + 2, gp41->y() + 22, 90, 17, "morse long" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "morse code containing a long message" );
bt->callback( cb_bt_synth_dlg_combo, (void*)301 );

bt = new Fl_Round_Button( gp41->x() + 2, gp41->y() + 42, 90, 17, "voice0" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "voice clip from file" );
bt->callback( cb_bt_synth_dlg_combo, (void*)302 );

bt = new Fl_Round_Button( gp41->x() + 2, gp41->y() + 62, 90, 17, "voice1" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "voice clip from file" );
bt->callback( cb_bt_synth_dlg_combo, (void*)303 );

bt = new Fl_Round_Button( gp41->x() + 2, gp41->y() + 82, 90, 17, "tone swept" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "use tone as modulating signal" );
bt->callback( cb_bt_synth_dlg_combo, (void*)304 );

bt = new Fl_Round_Button( gp41->x() + 2, gp41->y() + 102, 90, 17, "tone warble" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "use tone as modulating signal" );
bt->callback( cb_bt_synth_dlg_combo, (void*)305 );
bt->value(1);

bt = new Fl_Round_Button( gp41->x() + 2, gp41->y() + 122, 90, 17, "tone" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "use adjustable tone as modulating signal, change freq below" );
bt->callback( cb_bt_synth_dlg_combo, (void*)306 );

bt = new Fl_Round_Button( gp41->x() + 2, gp41->y() + 142, 90, 17, "off" );
bt->type(FL_RADIO_BUTTON);
bt->labelsize(12);
bt->tooltip( "turn off modulation" );
bt->callback( cb_bt_synth_dlg_combo, (void*)307 );


miw_synth_dlg_tone_freq3 = new My_Input_Wheel( gp41->x() + 2, gp41->y() + 202, 70, 15, "tone frq" );
omiw = miw_synth_dlg_tone_freq3;
omiw->labelsize(10);
omiw->textsize(9);
omiw->align(FL_ALIGN_LEFT);
omiw->tooltip( "modulating tone's freq" );
omiw->b_show_modified = 1;
omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
omiw->col_update();
omiw->b_take_focus_on_inside = 0;
omiw->b_use_limit_min = 1;
omiw->limit_min = 1;
omiw->b_use_limit_max = 1;
omiw->limit_max = 20e3;

omiw->b_invert_wheel = 0;				
omiw->std_wheel_step = 10;
omiw->ctrl_wheel_step = 100;
omiw->shift_wheel_step = 500;
omiw->ctrl_shift_wheel_step = 10;

omiw->s_printf_format = "%.3f";
omiw->force_integer = 0;
omiw->set_value_from_double( synth_tone_freq3 );
omiw->set_callback( (void*)cb_miw_synth_dlg, (void*)synth_dlg, (void*)31 );
omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
omiw->id2= 1;
omiw->allow_right_but_drag = 1;
omiw->right_drag_x_val_change_factor = 0;
omiw->right_drag_y_val_change_factor = 10;

gp41->end();
//------
//----------------------------------------------------------------------





synth_dlg->end();
synth_dlg->show();
}














bool load_audio_file( string fname, audio_formats &af, st_audio_formats_tag &saf )
{
en_audio_formats fmt;

if( !af.find_file_format( "", fname, fmt ) )
	{
	printf("load_audio_file() - unable to determine audio file format: '%s'\n", fname.c_str() );

//	ck_use_audiofile->value( !bno_audio_file );
	return 0;
	}
else{
	printf("load_audio_file() - determined audio file format is: %d, '%s'\n", (int)fmt, fname.c_str() );
	}


saf.format = fmt;

if( !af.load_malloc( "", fname, 0, saf ) )
	{
	printf("load_audio_file() - unable to read audio file: '%s'\n", fname.c_str() );
	return 0;
	}
else{
	printf( "load_audio_file() - read audio file: '%s'\n", fname.c_str() );
	printf( "load_audio_file() - saf.srate: %d\n", saf.srate ); 
	printf( "load_audio_file() - saf.channels: %d\n", saf.channels ); 
	printf( "load_audio_file() - saf.format: %d\n", saf.format ); 
	printf( "load_audio_file() - saf.encoding: %d\n", saf.encoding ); 
	printf( "load_audio_file() - saf.is_big_endian: %d\n", saf.is_big_endian ); 
	}

//bno_audio_file = 0;

//ck_use_audiofile->value( !bno_audio_file );

//srate = saf.srate;



//aud_len = af.sizech0;
uint64_t fileaud_len = af.sizech0;
//if( af.sizech1 < af.sizech0 ) aud_len = af.sizech1;
int aud_channels = saf.channels;


printf("load_audio_file() - channels: %d, srate: %d\n", aud_channels,  aud_op_srate );
printf("load_audio_file() - sample count: %" PRIu64 ", length (secs): %f\n", fileaud_len, (float)fileaud_len / aud_op_srate );

return 1;
}









//refer also flag 'b_slow_load_read_voice_files'
void set_synth_mode_and_menu_state( bool bstate )
{
mystr m1;

//bvoice_file = 0;
wnd_rtl_graph->b_synth_iq = 0;


m1.delay_ms( 100 );														//wait for an audio proc to complete


vaudclip0.clear();
vc_audclip0.clear();


vaudclip1.clear();
vc_audclip1.clear();

vaudclip2.clear();
vc_audclip2.clear();

vaudclip3.clear();
vc_audclip3.clear();

vaudclip4.clear();
vc_audclip4.clear();

vaudsynth0.clear();
vc_audclip0.clear();

vaudsynth1.clear();
vc_audclip1.clear();


create_hilbert_fir_filters();											//used to gen I/Q versions of audio clips


if( b_fast_start_no_voice_files )
	{
	printf("set_synth_mode_and_menu_state(() - !!!! 'b_fast_start_no_voice_files' is set, voice files will be replaced by synth tones !!!\n" );
	}

if( !b_fast_start_no_voice_files )
	{
	//bvoice_file = 1;
	//------
	if( load_audio_file( cn_synth_voice_fname0, af0, saf0 ) )
		{
	//	af0.audio_formats::normalise_malloc( 2.0f );
		vector<float>vaud;
		
		int output_count = af0.sizech0 * ( (float)aud_op_srate / saf0.srate );
		float nyquist = 2000;//saf0.srate * 0.45;							//limit bwidth to 2KHz to avoid spilling into neighbouring signals


	//	printf("set_synth_mode_and_menu_state() - '%s', srate %d,  samples %d, about to upsample\n", cn_synth_voice_fname, saf0.srate, af0.sizech0 );
		
		for( int i = 0; i < af0.sizech0; i++ )								
			{
			vaud.push_back( af0.pch0[i] );
			
			
	//		if(!(i%100) ) printf("set_synth_mode_and_menu_state() - i %d\n", i );
			}

		gc_srateconv_code::qdss_resample_float_vector( output_count, saf0.srate, nyquist, 32, vaud, vaudclip0 );	//srate conv



		for( int i = 0; i < vaudclip0.size(); i++ )							//create I/Q audio versions							
			{
			filter_code::st_cplex_tag oc;
			
			float f0 = vaudclip0[i];
			
			filter_code::fir_in( fir_hilbert_45_minus, f0 );			//TO DO: use a 90 deg hilbert and a simple matching delay line to reduce calcs
			float f1 = filter_code::fir_out( fir_hilbert_45_minus );
			oc.real = f1;


			filter_code::fir_in( fir_hilbert_45_plus, f0 );
			f1 = filter_code::fir_out( fir_hilbert_45_plus );
			oc.imag = f1;
				
			vc_audclip0.push_back( oc );
			}

		printf("set_synth_mode_and_menu_state() - '%s', srate %d,  samples %d,    resampled:  srate %d,  samples %d\n", cn_synth_voice_fname0, saf0.srate, af0.sizech0, aud_op_srate, (int)vaudclip0.size() );

		}
	//------


	//------
	if( load_audio_file( cn_synth_voice_fname1, af1, saf1 ) )
		{
		vector<float>vaud;
		
		int output_count1 = af1.sizech0 * ( (float)aud_op_srate / saf1.srate );
		float nyquist = 2000;//saf1.srate * 0.45;							//limit bwidth to 2KHz to avoid spilling into neighbouring signals


	//	printf("set_synth_mode_and_menu_state() - '%s', srate %d,  samples %d, about to upsample\n", cn_synth_voice_fname, saf0.srate, af0.sizech0 );
		
		for( int i = 0; i < af1.sizech0; i++ )								
			{
			vaud.push_back(af1.pch0[i] );
			}

		gc_srateconv_code::qdss_resample_float_vector( output_count1, saf1.srate, nyquist, 32, vaud, vaudclip1 );


		for( int i = 0; i < vaudclip1.size(); i++ )							//create I/Q audio versions							
			{
			filter_code::st_cplex_tag oc;
			
			float f0 = vaudclip1[i];
			
			filter_code::fir_in( fir_hilbert_45_minus, f0 );			//TO DO: use a 90 deg hilbert and a simple matching delay line to reduce calcs
			float f1 = filter_code::fir_out( fir_hilbert_45_minus );
			oc.real = f1;


			filter_code::fir_in( fir_hilbert_45_plus, f0 );
			f1 = filter_code::fir_out( fir_hilbert_45_plus );
			oc.imag = f1;
				
			vc_audclip1.push_back( oc );
			}

		printf("set_synth_mode_and_menu_state() - '%s', srate %d,  samples %d,    resampled:  srate %d,  samples %d\n", cn_synth_voice_fname1, saf1.srate, af1.sizech0, aud_op_srate, (int)vaudclip1.size() );

	//	bvoice_file = 1;													//allow audio proc to access
		}
	//------




	//------
	if( load_audio_file( cn_synth_voice_fname2, af1, saf1 ) )
		{
		vector<float>vaud;
		
		int output_count1 = af1.sizech0 * ( (float)aud_op_srate / saf1.srate );
		float nyquist = 2000;//saf1.srate * 0.45;							//limit bwidth to 2KHz to avoid spilling into neighbouring signals


	//	printf("set_synth_mode_and_menu_state() - '%s', srate %d,  samples %d, about to upsample\n", cn_synth_voice_fname, saf0.srate, af0.sizech0 );
		
		for( int i = 0; i < af1.sizech0; i++ )								
			{
			vaud.push_back(af1.pch0[i] );
			}

		gc_srateconv_code::qdss_resample_float_vector( output_count1, saf1.srate, nyquist, 32, vaud, vaudclip2 );


		for( int i = 0; i < vaudclip2.size(); i++ )						//create I/Q audio versions							
			{
			filter_code::st_cplex_tag oc;
			
			float f0 = vaudclip2[i];
			
			filter_code::fir_in( fir_hilbert_45_minus, f0 );			//TO DO: use a 90 deg hilbert and a simple matching delay line to reduce calcs
			float f1 = filter_code::fir_out( fir_hilbert_45_minus );
			oc.real = f1;


			filter_code::fir_in( fir_hilbert_45_plus, f0 );
			f1 = filter_code::fir_out( fir_hilbert_45_plus );
			oc.imag = f1;

			vc_audclip2.push_back( oc );
			}

		printf("set_synth_mode_and_menu_state() - '%s', srate %d,  samples %d,    resampled:  srate %d,  samples %d\n", cn_synth_voice_fname2, saf1.srate, af1.sizech0, aud_op_srate, (int)vaudclip2.size() );

	//	bvoice_file = 1;													//allow audio proc to access
		}
	//------



	/*
	//------
	if( load_audio_file( cn_synth_voice_fname3, af1, saf1 ) )
		{
		vector<float>vaud;
		
		int output_count1 = af1.sizech0 * ( (float)aud_op_srate / saf1.srate );
		float nyquist = 3000;//saf1.srate * 0.45;							//limit bwidth to 3KHz to avoid spilling into neighbouring signals


	//	printf("set_synth_mode_and_menu_state() - '%s', srate %d,  samples %d, about to upsample\n", cn_synth_voice_fname, saf0.srate, af0.sizech0 );
		
		for( int i = 0; i < af1.sizech0; i++ )								
			{
			vaud.push_back(af1.pch0[i] );
			}

		gc_srateconv_code::qdss_resample_float_vector( output_count1, saf1.srate, nyquist, 32, vaud, vaudclip3 );

		printf("set_synth_mode_and_menu_state() - '%s', srate %d,  samples %d,    resampled:  srate %d,  samples %d\n", cn_synth_voice_fname3, saf1.srate, af1.sizech0, aud_op_srate, (int)vaudclip3.size() );

	//	bvoice_file = 1;													//allow audio proc to access
		}
	//------
	*/



	//float thetaz = 0.0f;
	//float theta_incz = twopi*400/24000.0f;



	//------
	if( load_audio_file( cn_synth_voice_fname3, af1, saf1 ) )
		{
		vector<float>vaud;
		
		int output_count1 = af1.sizech0 * ( (float)aud_op_srate / saf1.srate );
		float nyquist = 2000;//saf1.srate * 0.45;							//limit bwidth to 2KHz to avoid spilling into neighbouring signals

		fir_hilbert_45_plus.bypass = 0;

	//	printf("set_synth_mode_and_menu_state() - '%s', srate %d,  samples %d, about to upsample\n", cn_synth_voice_fname, saf0.srate, af0.sizech0 );
		
		for( int i = 0; i < af1.sizech0; i++ )								
			{
			vaud.push_back( af1.pch0[i] );
			}

		gc_srateconv_code::qdss_resample_float_vector( output_count1, saf1.srate, nyquist, 32, vaud, vaudclip3 );


		for( int i = 0; i < vaudclip3.size(); i++ )							//create I/Q audio versions							
			{
			filter_code::st_cplex_tag oc;
			
			float f0 = vaudclip3[i];
			
			filter_code::fir_in( fir_hilbert_45_minus, f0 );			//TO DO: use a 90 deg hilbert and a simple matching delay line to reduce calcs
			float f1 = filter_code::fir_out( fir_hilbert_45_minus );
			oc.real = f1;


			filter_code::fir_in( fir_hilbert_45_plus, f0 );
			f1 = filter_code::fir_out( fir_hilbert_45_plus );
			oc.imag = f1;
			
	//		oc.real = cosf( thetaz );
	//		oc.imag = sinf( thetaz );
			
	//		thetaz += theta_incz;
	//		if( thetaz > twopi ) thetaz -= twopi; 
			
			vc_audclip3.push_back( oc );
			}


		printf("set_synth_mode_and_menu_state() - '%s', srate %d,  samples %d,    resampled:  srate %d,  samples %d\n", cn_synth_voice_fname3, saf1.srate, af1.sizech0, aud_op_srate, (int)vaudclip3.size() );
	//	bvoice_file = 1;													//allow audio proc to access
		}
	//------




	/*

	//------
	if( load_audio_file( cn_synth_voice_fname4, af1, saf1 ) )
		{
		vector<float>vaud;
		
		int output_count1 = af1.sizech0 * ( (float)aud_op_srate / saf1.srate );
		float nyquist = 2000;//saf1.srate * 0.45;							//limit bwidth to 2KHz to avoid spilling into neighbouring signals


	//	printf("set_synth_mode_and_menu_state() - '%s', srate %d,  samples %d, about to upsample\n", cn_synth_voice_fname, saf0.srate, af0.sizech0 );
		
		for( int i = 0; i < af1.sizech0; i++ )								
			{
			vaud.push_back(af1.pch0[i] );
			}

		gc_srateconv_code::qdss_resample_float_vector( output_count1, saf1.srate, nyquist, 64, vaud, vaudclip4 );


		for( int i = 0; i < vaudclip4.size(); i++ )							//create I/Q audio versions							
			{
			filter_code::st_cplex_tag oc;
			
			float f0 = vaudclip4[i];
			
			filter_code::fir_in( fir_hilbert_45_plus, f0 );				//TO DO: use a 90 deg hilbert and a simple matching delay line to reduce calcs
			float f1 = filter_code::fir_out( fir_hilbert_45_plus );
			oc.real = f1;


			filter_code::fir_in( fir_hilbert_45_minus, f0 );
			f1 = filter_code::fir_out( fir_hilbert_45_minus );
			oc.imag = f1;
				
			vc_audclip4.push_back( oc );
			}


		printf("set_synth_mode_and_menu_state() - '%s', srate %d,  samples %d,    resampled:  srate %d,  samples %d\n", cn_synth_voice_fname4, saf1.srate, af1.sizech0, aud_op_srate, (int)vaudclip4.size() );

	//	bvoice_file = 1;													//allow audio proc to access
		}
	//------


	*/

	}




//vaudclip0.clear();
//vaudclip1.clear();


//------
if( 1 )																	//build a sweep tone
	{	
	float dur = 4.0f;
	int smpls = dur * aud_op_srate;
	
	int half = smpls/2;
	
	
	float theta0 = 0;

	float freq_freq_min = 100;
	float freq_freq_max = 3000;											//only sweep up to 3KHz to avoid spilling into neighbouring signals

	
	float freq0 = freq_freq_min;
	float freq_step = (freq_freq_max - freq_freq_min) / smpls;
	for( int i = 0; i < smpls; i++ )
		{
		float s, c;
		
		sincosf( theta0, &s, &c );

		vaudsynth0.push_back( 0.1f * c );

		filter_code::st_cplex_tag oc;
		
		oc.real = 0.1f * c;
		oc.imag = -0.1f * s;

		vc_audsynth0.push_back( oc );
		
		freq0 += freq_step;

		float theta_inc0 = freq0 / aud_op_srate * twopi;

		theta0 += theta_inc0;
		if( theta0 >= twopi ) theta0 -= twopi;
		}	
	}
//------


//------
if( 1 )																	//build interesting 2 tone warble signal
	{
	float dur = 3.0f;
	int smpls = dur * aud_op_srate;
	
	int half = smpls/2;
	
	float freq0 = 72;
	float freq1 = freq0;
	
	float theta0 = 0;
	float theta1 = 0;
	float theta_tone_inc0 = freq0 / aud_op_srate * twopi;
	float theta_tone_inc1 = freq0*2 / aud_op_srate * twopi;

	for( int i = 0; i < smpls; i++ )
		{
		
		float f0r = 0.1f*cosf( theta0 ) + 0.1f*cosf( 3.0f * theta0 ) + 0.1f*cosf( 5.0f * theta0 );
		float f1r = 0.1f*cosf( theta1 ) + 0.2f*cosf( 3.0f * theta1 ) + 0.3f*cosf( 5.0f * theta1 );

		float f0i = -( 0.1f*sinf( theta0 ) + 0.1f*sinf( 3.0f * theta0 ) + 0.1f*sinf( 5.0f * theta0 ) );
		float f1i = -( 0.1f*sinf( theta1 ) + 0.2f*sinf( 3.0f * theta1 ) + 0.3f*sinf( 5.0f * theta1 ) );

		if( i >= half )
			{
			theta_tone_inc0 = freq0*1.1 / aud_op_srate * twopi;				//audio tones
			theta_tone_inc1 = freq0*1.1*2 / aud_op_srate * twopi;
			} 
		else{
			f0r = 0.0f; 
			f0i = 0.0f; 
			}

		vaudsynth1.push_back( (0.3*f0r + f1r) );

		filter_code::st_cplex_tag oc;
		
		oc.real = 0.3*f0r + f1r;
		oc.imag = 0.3*f0i + f1i;

		vc_audsynth1.push_back( oc );
		
		theta0 += theta_tone_inc0;
		if( theta0 >= twopi ) theta0 -= twopi;

		theta1 += theta_tone_inc1;
		if( theta1 >= twopi ) theta1 -= twopi;
		}
	}
//------


//if( !slow_load_read_voice_files )
	{
	if( vaudclip0.size() == 0  ) vaudclip0 = vaudsynth0;				//use sin/cos synth signals instead of voice files,resampler and hilbert fir which are slower to generate
	if( vc_audclip0.size() == 0  ) vc_audclip0 = vc_audsynth0;	
	if( vaudclip1.size() == 0  ) 	vaudclip1 = vaudsynth1;
	if( vc_audclip1.size() == 0  ) 	vc_audclip1 = vc_audsynth1;	
	if( vaudclip2.size() == 0  ) 	vaudclip2 = vaudsynth1;
	if( vc_audclip2.size() == 0  ) 	vc_audclip2 = vc_audsynth1;	
	if( vaudclip3.size() == 0  ) 	vaudclip3 = vaudsynth1;
	if( vc_audclip3.size() == 0  ) 	vc_audclip3 = vc_audsynth1;	
	}



//if( vaudclip0.size() == 0 )												//no audio clip loaded?, use a generated one
//	{
//	vaudclip0 = vaudsynth0;
//	}


//if( vaudclip01.size() == 0 )												//no audio clip loaded?, use a generated one
//	{
//	vaudclip1 = vaudsynth1;
//	}



wnd_rtl_graph->b_synth_iq = bstate;

/*
Fl_Menu_Item *mi = wnd_rtl_graph->menu_sdr->find_item( "&Device/&Synth IQ samples (tune to 10.000MHz)" );		//menu string MUST MATCH defined 'Synth IQ samples (tune to 10.000MHz)' menu definition, see 'menu_capture'
if(mi)
	{
	if( wnd_rtl_graph->b_synth_iq ) mi->set();
	else mi->clear();
	}
*/
b_listen = 1;

}







void cb_menu_combo( Fl_Widget *w, void *v)
{
mystr m1;
string s1, s2;
int which = (intptr_t)v;

printf( "cb_menu_combo() - which %d\n", which );


//slast_filename_ini = csIniFilename;

if( which == en_meni_file_open )
	{
	mystr m1;
	string s1, s2;

	char *pPathName = fl_file_chooser( "Open Settings?", "*", (const char*)slast_filename_ini.c_str(), 0 );
	if (!pPathName) return;

	bool b_load_last_favourite_settings = 0;
	wnd_rtl_graph->load_settings( pPathName, b_load_last_favourite_settings );

	slast_filename_ini = pPathName;
	}


if( which == en_meni_file_save )
	{
	mystr m1;
	string s1, s2;

	char *pPathName = fl_file_chooser( "Save Settings?", "*", (const char*)slast_filename_ini.c_str(), 0 );
	if (!pPathName) return;

	wnd_rtl_graph->save_settings( pPathName );
	
	slast_filename_ini = pPathName;
	}




if( which == en_meni_rec_file_sel )
	{
	mystr m1;
	string s1, s2;

	char *pPathName = fl_file_chooser( "Record File?", "*", (const char*)rec_iq_fname.c_str(), 0 );
	if (!pPathName) return;
	
	rec_iq_fname = pPathName;
	}


if( which == en_meni_rec_file_dur )
	{
	string s1, s2;
	strpf( s1, "Enter duration of rec in secs (max is 3600, 60 Mins) :" );
	strpf( s2, "%d", (int)rec_mode_end_secs );

	char *sz = fl_input( s1.c_str(), s2.c_str() );

	int	iv;

	if( sz != 0 )
		{
		sscanf( sz, "%d", &iv );
		if( iv < 1 ) iv = 1;
		if( iv > 60*30 ) iv =  60*60;
		rec_mode_end_secs = iv;
		}
	else{
		return;
		}
	}


if( which == en_meni_rec_iq )
	{
	if( b_play_iq )
		{
		strpf( s1, "File play appears to be running, it needs to be stopped first." );
		fl_alert( s1.c_str(), 0 );
		return;
		}

	if( b_rec_iq )
		{
		strpf( s1, "File record appears to be running, it needs to be stopped first." );
		fl_alert( s1.c_str(), 0 );
		return;
		}

	b_rec_iq = !b_rec_iq;
	
	if( b_rec_iq ) 
		{
		if( !rec_play_iq_set_state( 0 ) )								//user cancel 'RecordOver' question ?
			{
			b_rec_iq = 0;	
			}
		else{
			play_iq_fname = rec_iq_fname;
			}
		}
	else{
		rec_play_iq_set_state( 1 );
		}
	}



if( which == en_meni_play_file_sel )
	{
	mystr m1;
	string s1;

	char *pPathName = fl_file_chooser( "Play File?", "*", (const char*)play_iq_fname.c_str(), 0 );
	if (!pPathName) return;
	
	play_iq_fname = pPathName;
	}


if( which == en_meni_play_iq )
	{
	if( b_rec_iq )
		{
		strpf( s1, "File record appears to be running, it needs to be stopped first." );
		fl_alert( s1.c_str(), 0 );
		return;
		}

	b_play_iq = !b_play_iq;
	
	if( b_play_iq ) rec_play_iq_set_state( 2 );
	else rec_play_iq_set_state( 3 );
	}



if( which == en_meni_file_open_favoutrite )
	{
	char *pPathName = fl_file_chooser( "Open Favourite List ?", "*", (const char*)slast_favourite_fname.c_str(), 0 );
	if (!pPathName) return;


		
	if( wnd_rtl_graph->load_favourite_list( pPathName ) )
		{
		slast_favourite_fname = pPathName;
		GCProfile p( csIniFilename );
		p.WritePrivateProfileStr( "Settings", "slast_favourite_fname", slast_favourite_fname.c_str() );
		}
	else{
		strpf( s1, "Failed to open file: '%s'", pPathName );
		fl_alert( s1.c_str(), 0 );
		}
	}


if( which == en_meni_file_save_favoutrite )
	{
	char *pPathName = fl_file_chooser( "Save Favourite List ?", "*", (const char*)slast_favourite_fname.c_str(), 0 );
	if (!pPathName) return;

	bool bwrite = 1;
	unsigned long long int filesz;
	if ( m1.filesize( pPathName, filesz ) )
		{
		strpf( s1, "File already exists, Overwrite it? : '%s'", pPathName );
		int ret = fl_choice( s1.c_str(),"Cancel","Overwrite", 0 );
		if( ret == 0 )
			{
			bwrite = 0;
			}
		}
	
	if( bwrite )
		{
		if( wnd_rtl_graph->save_favourite_list( pPathName ) )
			{
			slast_favourite_fname = pPathName;
			GCProfile p( csIniFilename );
			p.WritePrivateProfileStr( "Settings", "slast_favourite_fname", slast_favourite_fname.c_str() );
			}
		else{
			strpf( s1, "Failed to save file: '%s'", pPathName );
			fl_alert( s1.c_str(), 0 );
			}
		}
	}



if( which == en_meni_device_open )
	{
		
	wnd_rtl_graph->b_synth_iq = 0;
	stop_audio();
	stop_threads();

	rtl.close();

	m1.delay_ms( 80 );
	start_up_state = 0;
	}



if( which == en_meni_device_close )
	{
	wnd_rtl_graph->b_synth_iq = 0;
	
	stop_audio();
	stop_threads();

	rtl.close();

	m1.delay_ms( 80 );
	}
	
	

if( which == en_meni_slew_adj_fine )
	{
	b_slew_adj_fine = !b_slew_adj_fine;
	}




if( which == en_meni_synth_dialog )
	{
	make_synth_dlg_wnd();
	}




if( which == en_meni_synth0 )
	{
	stop_audio();
	stop_threads();
	
	wnd_rtl_graph->b_synth_iq = 1;

	set_synth_mode_and_menu_state( wnd_rtl_graph->b_synth_iq );

	sync_wr_rd_pointer = 1;

	start_threads_rtl();
	start_audio();
	filters_create();

//	set_synth_mode_and_menu_state( wnd_rtl_graph->b_synth_iq );

/*
	Fl_Menu_Item *mi = wnd_rtl_graph->menu_sdr->find_item( "Synth IQ samples" );		//menu string MUST MATCH defined 'Synth IQ...' menu definition, see 'menu_capture'

	if(mi)
		{
//		printf( "cb_trace() - found menu item: 'Jpg scaling when zooming (slows drawing)'\n" );
	//	getchar();
		if( wnd_rtl_graph->b_synth_iq ) mi->set();
		else mi->clear();
		}
	b_listen = 1;
*/
	}

if( which == en_meni_synth_1st_tone_ampl_freq )
	{
	strpf( s1, "Enter amplitude and freq of 1st modulation tone, noting high freqs may bleed into neigbouring carriers, e.g: (0.25 400) : " );
	strpf( s2, "%.2f %.2f", wnd_rtl_graph->synth_mod_1st_tone_ampl, wnd_rtl_graph->synth_mod_1st_tone_freq );
	char *sz = fl_input( s1.c_str(), s2.c_str() );
	float f0, f1;
	if( sz == 0 ) return;
	sscanf( sz, "%f %f", &f0, &f1 );
		
	wnd_rtl_graph->synth_mod_1st_tone_ampl = f0;
	wnd_rtl_graph->synth_mod_1st_tone_freq = f1;
	}



if( which == en_meni_synth_2nd_tone_ampl_freq )
	{
	strpf( s1, "Enter amplitude and freq of 2nd modulation tonee, noting high freqs may bleed into neigbouring carriers, e.g: (0.25 400) : " );
	strpf( s2, "%.2f %.2f", wnd_rtl_graph->synth_mod_2nd_tone_ampl, wnd_rtl_graph->synth_mod_2nd_tone_freq );
	char *sz = fl_input( s1.c_str(), s2.c_str() );
	float f0, f1;
	if( sz == 0 ) return;
	sscanf( sz, "%f %f", &f0, &f1 );
		
	wnd_rtl_graph->synth_mod_2nd_tone_ampl = f0;
	wnd_rtl_graph->synth_mod_2nd_tone_freq = f1;
	}



if( which == en_meni_synth_am_mod )
	{
	wnd_rtl_graph->synth_mod_type = en_mdt_am;
	}


if( which == en_meni_synth_am_ssb_lsb_mod )
	{
	wnd_rtl_graph->synth_mod_type = en_mdt_lsb;
	}



if( which == en_meni_synth_am_ssb_usb_mod )
	{
	wnd_rtl_graph->synth_mod_type = en_mdt_usb;
	}


if( which == en_meni_synth_fm_mod )
	{
	wnd_rtl_graph->synth_mod_type = en_mdt_fm;
	}


if( which == en_meni_synth_tone )
	{
	wnd_rtl_graph->morse_play_which = -1;
	}

if( which == en_meni_synth_morse0 )
	{
	if (wnd_rtl_graph->morse_play_which == 0 ) wnd_rtl_graph->morse_play_which = -1;
	else wnd_rtl_graph->morse_play_which = 0;

	wnd_rtl_graph->morse_reset = 1;													//flag for change in 'demod_iso()' thread
	}



if( which == en_meni_synth_music0 )
	{
	if (wnd_rtl_graph->morse_play_which == 1 ) wnd_rtl_graph->morse_play_which = -1;
	else wnd_rtl_graph->morse_play_which = 1;

	wnd_rtl_graph->morse_reset = 1;													//flag for change in 'demod_iso()' thread
	}



if( which == en_meni_synth_voice0 )
	{
	wnd_rtl_graph->b_synth_voice_on_iq0 = !wnd_rtl_graph->b_synth_voice_on_iq0;
	}


if( which == en_meni_synth_voice1 )
	{
	wnd_rtl_graph->b_synth_voice_on_iq1 = !wnd_rtl_graph->b_synth_voice_on_iq1;
	}


if( which == en_meni_synth_voice2 )
	{
	wnd_rtl_graph->b_synth_voice_on_iq2 = !wnd_rtl_graph->b_synth_voice_on_iq2;
	}


if( which == en_meni_synth_voice3 )
	{
	wnd_rtl_graph->b_synth_voice_on_iq3 = !wnd_rtl_graph->b_synth_voice_on_iq3;
	}


if( which == en_meni_synth_single_sideband_usb )
	{
	wnd_rtl_graph->b_synth_voice_on_iq4 = !wnd_rtl_graph->b_synth_voice_on_iq4;
	}


if( which == en_meni_synth_noise0 )
	{
	wnd_rtl_graph->b_synth_noise_on_iq = !wnd_rtl_graph->b_synth_noise_on_iq;
	}




if( which == en_meni_windows_main )
	{
	wnd_rtl_graph->hide();
	wnd_rtl_graph->show();
	}

	
if( which == en_meni_windows_favourites )
	{
	wnd_fav->hide();
	wnd_fav->show();
	b_wnd_fav_is_open = 1;
	}


if( which == en_meni_windows_plot )
	{
	gph1.hide();
	gph1.show();
	}
	
if( which == en_meni_windows_aud_spect )
	{
	wnd_aud_spect->hide();
	wnd_aud_spect->show();
	}
	
	
if( which == en_meni_help_about )
	{
	cb_bt_about( 0, 0 );
	}


if( which == en_meni_edit_preferences )
	{
	make_pref_wnd();
	}


if( which == en_meni_help_debug_adj_fftw_plan )
	{
//	i_fftw_trig_plan_create_state = 0; 									//start the transition state going which will create require fftw plans
	}


}




void cb_bt_tops_clear( Fl_Widget *w, void *v )
{
string s1;

//Fl_Button* ob = (Fl_Button*)w;

int which = (intptr_t)v;
printf( "cb_bt_tops_clear() - which %d\n", which );

if( which == 0 )
	{
	tops_thread_stats_cnt = 1;
	tops_thread_stats_avg_sum = 0;
	}

}













bool dbg_btn1 = 0;
bool dbg_btn2 = 0;
bool dbg_btn3 = 0;



void cb_bt_dbg_combo( Fl_Widget *w, void *v )
{
string s1;

Fl_Button* ob = (Fl_Button*)w;

int which = (intptr_t)v;
printf( "cb_bt_dbg_combo() - which %d\n", which );


if( which == 0 )
	{
//	b_rdwr_rephase = 1;

//	i_fftw_trig_plan_create_state = 0; 		//start the transition state going which will create require fftw plans

	dbg_btn1 = 0;
	}



if( which == 1 )
	{
	dbg_btn1 = 1;
	}


if( which == 2 )
	{
	dbg_btn2 = ob->value();
	}
	
if( which == 3 )
	{
	dbg_btn3 = ob->value();
	}

}










void cb_ld_fm_deemph( Fl_Widget *w, void *v )
{
int which = (intptr_t)v;
printf( "cb_ld_fm_deemph() - which %d\n", which );

GCLed *o;
o =(GCLed *)w;

int i = o->GetColIndex();

i++;
if( i > 2 ) i = 0;

i_deemphasis = i;
o->ChangeCol( i_deemphasis );
}






void cb_ld_agc( Fl_Widget *w, void *v )
{

///b_agc_enable = !b_agc_enable;

int which = (intptr_t)v;
printf( "cb_ld_agc() - which %d\n", which );

GCLed *o;
o =(GCLed *)w;

int i = o->GetColIndex();
if( i == 0 )
	{
	b_agc = 1;
	o->ChangeCol( b_agc );
	}
else{
	b_agc = 0;
	o->ChangeCol( b_agc );
	}

}












void cb_ld_19k( Fl_Widget *w, void *v )
{

int which = (intptr_t)v;
printf( "cb_ld_19k() - which %d\n", which );

GCLed *o;
o =(GCLed *)w;


demodul_mode_set( en_dmt_fm_stereo );

g_aud_mono = 0;
wnd_rtl_graph->ck_mono->value( g_aud_mono );



g_b_dwn_aa = 0;
g_b_user_iir_lpf0 = 0;
g_b_user_iir_lpf1 = 0;
g_b_user_iir_lpf2 = 0;
g_b_user_iir_hpf0 = 0;
g_b_bw_bpass = 0;
b_dc_block_iq = 0;
b_agc = 0;


wnd_rtl_graph->ck_user_dwn_aa->value( g_b_dwn_aa );
wnd_rtl_graph->ck_user_lpf0->value( g_b_user_iir_lpf0 );
wnd_rtl_graph->ck_user_lpf1->value( g_b_user_iir_lpf0 );
wnd_rtl_graph->ck_user_lpf2->value( g_b_user_iir_lpf1 );
wnd_rtl_graph->ck_user_lpf2->value( g_b_user_iir_lpf2 );
wnd_rtl_graph->ck_bw_limit->value( g_b_bw_bpass );
wnd_rtl_graph->ck_user_hpf0->value( g_b_user_iir_hpf0 );
wnd_rtl_graph->ck_user_dc_block_iq->value( b_dc_block_iq );
wnd_rtl_graph->ld_agc->ChangeCol( b_agc );




//---- turn off notch filters -----

int ii = 0;
st_aud_notch_gui_ctrls_tag ano = st_aud_notch_gui_ctrls[ii];
ano.active = 0;
ano.ck_en->value(0);

st_aud_notch_gui_ctrls[ii] = ano;




ii = 1;
ano = st_aud_notch_gui_ctrls[ii];
ano.active = 0;
ano.ck_en->value(0);

st_aud_notch_gui_ctrls[ii] = ano;



int filt_idx = en_ftid_iir_notch0_aud;
st_filt[filt_idx].flags &= ~en_fflg_on;
 
//filter_iir_notch_adjust( filt_idx,  st_filt[filt_idx].fc0,  st_filt[filt_idx].Q );	//this really only rebuilts bode plot as notch has been turned off above


filt_idx = en_ftid_iir_notch1_aud;
st_filt[filt_idx].flags &= ~en_fflg_on; 
//filter_iir_notch_adjust( filt_idx,  st_filt[filt_idx].fc0,  st_filt[filt_idx].Q );	//this really only rebuilts bode plot as notch has been turned off above
//printf( "EEEEEEEEEEEEEEEEEEEEEEEEEEEEE\n" );
//--------------------------------


downsample_srate_pending = 320000;
b_need_dwn_srate_change = 1;											//trigger dwn srate update




//int i = o->GetColIndex();
//if( i == 0 )
//	{
//	b_agc = 1;
//	o->ChangeCol( b_agc );
//	}
//else{
//	b_agc = 0;
//	o->ChangeCol( b_agc );
//	}

}




void cb_ld_clip( Fl_Widget *w, void *v )
{
GCLed *ow;
ow =(GCLed *)w;


if( ow->right_button )
	{
	string s1, s2;

	strpf( s1, "Enter audio clip level, e.g. 0.5: " );

	strpf( s2, "%.3f", clip_level_audio );

	char *sz = fl_input( s1.c_str(), s2.c_str() );

	if( sz != 0 )
		{
		float ff;
		sscanf( sz, "%f", &ff );
		
		ff = fabsf( ff );
		if( ff < cn_clip_audio_limit_low ) ff = cn_clip_audio_limit_low;
		if( ff > cn_clip_audio_limit_high  ) ff = cn_clip_audio_limit_high;
		
		clip_level_audio = ff;
		}
	}
else{
	int ii = ow->GetColIndex();
	
	ii++;
	if( ii >=2 ) ii = 0;
	ow->ChangeCol( ii );
	
	b_clip_enable = ii;
	
	}
}





//given a menu idx, get menu text, extract and show both start and stop freqs (removing comma delimit)
void show_scan_history_menu_sel( int menu_idx )
{
string s1;

if( !wnd_rtl_graph ) return;

if( menu_idx < 0 ) return;

Fl_Menu_Button *m = wnd_rtl_graph->fi_freq_start->menubutton();
//printf("cb_bt_scan_history_start1= %x\n", m );
//printf("cb_bt_scan_history_start2= %d\n", m->value() );

vector<string>vstr;
mystr m1;
m1 = m->text( menu_idx );
m1.LoadVectorStrings( vstr, ',' );


if( vstr.size() > 1 )
	{
	m1 = vstr[ 0 ].c_str();
	m1.FindReplace( s1, " ", "", 0 );
	wnd_rtl_graph->fi_freq_start->value( s1.c_str() );
	
	m1 = vstr[ 1 ].c_str();
	m1.FindReplace( s1, " ", "", 0 );
	wnd_rtl_graph->fi_freq_stop->value( s1.c_str() );
	}

}





//void cb_fi_ifreq( Fl_Widget *, void * )
//{
//string s1;

//get_user_gui_control_params();
//printf("cb_fi_ifreq= %f\n", g_interfreq );
//}






/*
void cb_fi_bw_lower_upper( Fl_Widget *w, void *v )
{
mystr m1;

int which = (intptr_t)v;
printf( "cb_fi_bw_lower_upper() - which %d\n", which );

//cb_bt_freq_tune( 0, 0 );
wnd_rtl_graph->freq_listen( 1, 0, 0, 0 );

}
*/















//see also  'filters_create()'  'filter_iir_rebuild()' 'filter_fir_rebuild()' 'filters_destroy()' 'filter_iir0_adjust()'   'filter_iir1_adjust()'  
void filter_iir_rebuild()
{
filter_iir0_adjust( g_user_iir_lpf0, g_user_iir_hpf0 );
filter_iir1_adjust( g_user_iir_lpf1 );
filter_iir2_adjust( g_user_iir_lpf2 );


int filt_idx = en_ftid_iir_notch0_aud;
filter_iir_notch_adjust( filt_idx,   st_filt[ filt_idx ].fc0,    st_filt[  filt_idx ].Q );


filt_idx = en_ftid_iir_notch1_aud;
filter_iir_notch_adjust( filt_idx,   st_filt[ filt_idx ].fc0,    st_filt[  filt_idx ].Q );


filt_idx = en_ftid_iir_fmst_19k_pilot_notch;
filter_iir_notch_adjust( filt_idx,   st_filt[ filt_idx ].fc0,    st_filt[  filt_idx ].Q );


}











//see also  'filters_create()'  'filter_iir_rebuild()' 'filter_fir_rebuild()' 'filters_destroy()' 'filter_iir0_adjust()'   'filter_iir1_adjust()'  
void filter_fir_rebuild()
{
bool vb = 1;
mystr m1;
float dt;

m1.time_start( m1.ns_tim_start );


//--------- adjust filter for new srate -------
int ii = en_ftid_fir_fmst_15k_lpf0;

//check audio thrd is ready for a filter switch
while( 1 )
	{
	if( st_filt[ii].pending_idx.load( std::memory_order_acquire ) == -1 ) break;
	
	dt = m1.time_passed( m1.ns_tim_start );
	if( dt > 0.1 )
		{
		if(vb)printf( "filter_fir_rebuild() - ZZZZZZZZZZZZZZZZZZZZZ timeout waiting for audio thrd to switch to 'st_filt[%d].pending_idx'\n", ii );
//		return;	
		}
	m1.delay_ms(1);														//don't hog processor
	}



int idx = st_filt[ii].inactive_idx;										//get idle filter's idx
st_filt[ii].pending_idx = -1;


//---------- alter st_filt[ii].fir[idx] -----------

//st_filt[ii].flags = (en_filter_flag_tag)en_fflg_zero | en_fflg_fir | en_fflg_alloc;	//set filter as alloc'd, set for fir

//st_filt[ii].sname = "en_ftid_fir_fmst_15k_lpf0";
//st_filt[ii].srate = fm_decimator_srate;									//use halfband downsampler srate
//st_filt[ii].db_gain = 0.0f;
//st_filt[ii].fc0 = 1800.0f;											//this set in: 'filters_create()'  'filter_iir_rebuild()'
//st_filt[ii].fc1 = 0.0f;
//st_filt[ii].Q = 7.0f;
//st_filt[ii].taps = 55;
//st_filt[ii].active_idx = 0;				//this is the index of which iir[] the audio thrd is using, audio thrd switches this when 'pending_idx' is not -1
//st_filt[ii].inactive_idx = 1;			//set this to 1 as 'st_filt[ii].iir[0]' will be first iir put into service in audio thrd	
//st_filt[ii].pending_idx = -1;			//this is an atomic obj
//st_filt[ii].fir[0].bypass = 0;


if(vb)printf( "filter_fir_rebuild() - srate %d, freq_cutoff %f  Q %f, '%s'\n", st_filt[ii].srate, st_filt[ii].fc0, st_filt[ii].Q, st_filt[ii].sname.c_str() );

if( ( st_filt[ii].fc0 < 0.0f ) || ( st_filt[ii].fc0 > st_filt[ii].srate / 2 )  )
	{
	printf( "filter_fir_rebuild() - freq_cutoff required is out of range: %f, (must be between 0-->%d), '%s'\n)", st_filt[ii].fc0, downsample_srate/2, st_filt[ii].sname.c_str() );
//	return 0;
	}



filter_code::en_filter_window_type_tag wnd_type  =  filter_code::fwt_kaiser;
filter_code::en_filter_pass_type_tag filt_type = filter_code::fpt_lowpass;
filter_code::kaiser_beta = 6;
//
//st_filt[ii].fir[0].verb = 1;
//st_filt[ii].fir[0].suser0 = "fm_ster_fir_15k_lpf0";
//st_filt[ii].fir[0].user_id0 = 0;
//st_filt[ii].fir[0].created = 0;
//st_filt[ii].fir[1].created = 0;											//also clear created flag for filter ping pong alternate
//
filter_code::filter_fir_windowed( wnd_type, filt_type, st_filt[ii].taps, st_filt[ii].fc0, st_filt[ii].fc1, fm_decimator_srate, st_filt[ii].vcef );
filter_code::create_filter_from_coeffs( st_filt[ii].fir[idx], st_filt[ii].vcef );
filter_code::fir_init( st_filt[ii].fir[idx] );
//st_filt[ii].fir[idx].bypass = 0;
//fm_ster_fir_15k_lpf0.created = 0;

st_filt[ii].pending_idx.store( st_filt[ii].inactive_idx, std::memory_order_release );	//let audio thrd know the idle filter is ready for use

st_filt[ii].inactive_idx = !st_filt[ii].inactive_idx;					//toggle to show other iir is now avail for gui to modify
//------------------------------






//--------- adjust filter for new srate -------
ii = en_ftid_fir_fmst_15k_lpf1;

//check audio thrd is ready for a filter switch
while( 1 )
	{
	if( st_filt[ii].pending_idx.load( std::memory_order_acquire ) == -1 ) break;
	
	dt = m1.time_passed( m1.ns_tim_start );
	if( dt > 0.1 )
		{
		if(vb)printf( "filter_fir_rebuild() - ZZZZZZZZZZZZZZZZZZZZZ timeout waiting for audio thrd to switch to 'st_filt[%d].pending_idx'\n", ii );
//		return 0;	
		}
	m1.delay_ms(1);														//don't hog processor
	}



idx = st_filt[ii].inactive_idx;										//get idle filter's idx
st_filt[ii].pending_idx = -1;


//---------- alter st_filt[ii].fir[idx] -----------

//st_filt[ii].flags = (en_filter_flag_tag)en_fflg_zero | en_fflg_fir | en_fflg_alloc;	//set filter as alloc'd, set for fir

//st_filt[ii].sname = "en_ftid_fir_fmst_15k_lpf0";
//st_filt[ii].srate = fm_decimator_srate;									//use halfband downsampler srate
//st_filt[ii].db_gain = 0.0f;
//st_filt[ii].fc0 = 1800.0f;											//this set in: 'filters_create()'  'filter_iir_rebuild()'
//st_filt[ii].fc1 = 0.0f;
//st_filt[ii].Q = 7.0f;
//st_filt[ii].taps = 55;
//st_filt[ii].active_idx = 0;				//this is the index of which iir[] the audio thrd is using, audio thrd switches this when 'pending_idx' is not -1
//st_filt[ii].inactive_idx = 1;			//set this to 1 as 'st_filt[ii].iir[0]' will be first iir put into service in audio thrd	
//st_filt[ii].pending_idx = -1;			//this is an atomic obj
//st_filt[ii].fir[0].bypass = 0;


if(vb)printf( "filter_fir_rebuild() - srate %d, freq_cutoff %f  Q %f, '%s'\n", st_filt[ii].srate, st_filt[ii].fc0, st_filt[ii].Q, st_filt[ii].sname.c_str() );

if( ( st_filt[ii].fc0 < 0.0f ) || ( st_filt[ii].fc0 > st_filt[ii].srate / 2 )  )
	{
	printf( "filter_fir_rebuild() - freq_cutoff required is out of range: %f, (must be between 0-->%d), '%s'\n)", st_filt[ii].fc0, downsample_srate/2, st_filt[ii].sname.c_str() );
//	return 0;
	}



wnd_type =  filter_code::fwt_kaiser;
filt_type = filter_code::fpt_lowpass;
filter_code::kaiser_beta = 6;
//
//st_filt[ii].fir[0].verb = 1;
//st_filt[ii].fir[0].suser0 = "fm_ster_fir_15k_lpf0";
//st_filt[ii].fir[0].user_id0 = 0;
//st_filt[ii].fir[0].created = 0;
//st_filt[ii].fir[1].created = 0;											//also clear created flag for filter ping pong alternate
//
filter_code::filter_fir_windowed( wnd_type, filt_type, st_filt[ii].taps, st_filt[ii].fc0, st_filt[ii].fc1, fm_decimator_srate, st_filt[ii].vcef );
filter_code::create_filter_from_coeffs( st_filt[ii].fir[idx], st_filt[ii].vcef );
filter_code::fir_init( st_filt[ii].fir[idx] );
//st_filt[ii].fir[idx].bypass = 0;
//fm_ster_fir_15k_lpf0.created = 0;

st_filt[ii].pending_idx.store( st_filt[ii].inactive_idx, std::memory_order_release );	//let audio thrd know the idle filter is ready for use

st_filt[ii].inactive_idx = !st_filt[ii].inactive_idx;					//toggle to show other iir is now avail for gui to modify
//------------------------------








//--------- adjust filter for new srate -------
ii = en_ftid_fir_fmst_23_53k_bpf;

//check audio thrd is ready for a filter switch
while( 1 )
	{
	if( st_filt[ii].pending_idx.load( std::memory_order_acquire ) == -1 ) break;
	
	dt = m1.time_passed( m1.ns_tim_start );
	if( dt > 0.1 )
		{
		if(vb)printf( "filter_fir_rebuild() - ZZZZZZZZZZZZZZZZZZZZZ timeout waiting for audio thrd to switch to 'st_filt[%d].pending_idx'\n", ii );
//		return 0;	
		}
	m1.delay_ms(1);														//don't hog processor
	}



idx = st_filt[ii].inactive_idx;										//get idle filter's idx
st_filt[ii].pending_idx = -1;


//---------- alter st_filt[ii].fir[idx] -----------

//st_filt[ii].flags = (en_filter_flag_tag)en_fflg_zero | en_fflg_fir | en_fflg_alloc;	//set filter as alloc'd, set for fir

//st_filt[ii].sname = "en_ftid_fir_fmst_15k_lpf0";
//st_filt[ii].srate = fm_decimator_srate;									//use halfband downsampler srate
//st_filt[ii].db_gain = 0.0f;
//st_filt[ii].fc0 = 1800.0f;											//this set in: 'filters_create()'  'filter_iir_rebuild()'
//st_filt[ii].fc1 = 0.0f;
//st_filt[ii].Q = 7.0f;
//st_filt[ii].taps = 55;
//st_filt[ii].active_idx = 0;				//this is the index of which iir[] the audio thrd is using, audio thrd switches this when 'pending_idx' is not -1
//st_filt[ii].inactive_idx = 1;			//set this to 1 as 'st_filt[ii].iir[0]' will be first iir put into service in audio thrd	
//st_filt[ii].pending_idx = -1;			//this is an atomic obj
//st_filt[ii].fir[0].bypass = 0;


if(vb)printf( "filter_fir_rebuild() - srate %d, freq_cutoffs %f %f  Q %f, '%s'\n", st_filt[ii].srate, st_filt[ii].fc0, st_filt[ii].fc1, st_filt[ii].Q, st_filt[ii].sname.c_str() );

if( ( st_filt[ii].fc0 < 0.0f ) || ( st_filt[ii].fc0 > st_filt[ii].srate / 2 )  )
	{
	printf( "filter_fir_rebuild() - freq_cutoff required is out of range: %f, (must be between 0-->%d), '%s'\n)", st_filt[ii].fc0, downsample_srate/2, st_filt[ii].sname.c_str() );
//	return 0;
	}



wnd_type =  filter_code::fwt_kaiser;
filt_type = filter_code::fpt_bandpass;
filter_code::kaiser_beta = 1;
//
//st_filt[ii].fir[0].verb = 1;
//st_filt[ii].fir[0].suser0 = "fm_ster_fir_15k_lpf0";
//st_filt[ii].fir[0].user_id0 = 0;
//st_filt[ii].fir[0].created = 0;
//st_filt[ii].fir[1].created = 0;											//also clear created flag for filter ping pong alternate
//
filter_code::filter_fir_windowed( wnd_type, filt_type, st_filt[ii].taps, st_filt[ii].fc0, st_filt[ii].fc1, fm_decimator_srate, st_filt[ii].vcef );
filter_code::create_filter_from_coeffs( st_filt[ii].fir[idx], st_filt[ii].vcef );
filter_code::fir_init( st_filt[ii].fir[idx] );
//st_filt[ii].fir[idx].bypass = 0;
//fm_ster_fir_15k_lpf0.created = 0;

st_filt[ii].pending_idx.store( st_filt[ii].inactive_idx, std::memory_order_release );	//let audio thrd know the idle filter is ready for use

st_filt[ii].inactive_idx = !st_filt[ii].inactive_idx;					//toggle to show other iir is now avail for gui to modify
//------------------------------











}













//see also 'filters_create()'
bool filter_iir0_adjust( float freq_cutoff0, float freq_cutoff1 )
{
bool vb = 0;
mystr m1;

if(vb)printf( "filter_iir0_adjust() - srate %f, freq_cutoff0 %f, freq_cutoff1 %f\n", downsample_srate, freq_cutoff0, freq_cutoff1 );

if( ( freq_cutoff0 < 10.0f ) || ( freq_cutoff0 > downsample_srate/2 )  )
	{
	printf( "filter_iir0_adjust() - freq_cutoff0 required is out of range: %f, (must be between 3000-->%d)\n)", freq_cutoff0, cn_rtl_sample_bandwith_max/2 );
	return 0;
	}



if( ( freq_cutoff1 < 10.0f ) || ( freq_cutoff1 > downsample_srate/2 )  )
	{
	printf( "filter_iir0_adjust() - freq_cutoff1 required is out of range: %f, (must be between 3000-->%d)\n)", freq_cutoff1, cn_rtl_sample_bandwith_max/2 );
	return 0;
	}


usr_hpf_iir0_I0.bypass = 1;
usr_hpf_iir0_Q0.bypass = 1;

usr_lpf_iir0_I0.bypass = 1;
usr_lpf_iir0_Q0.bypass = 1;

m1.delay_ms( 100 );														//as a precaution for atomicity, give the 'demod_iso()' thread time to see this 'bypass' flag change







//------------------
//----
iir_init( usr_hpf_iir0_I0 );

filter_code::en_filter_pass_type_tag filt_type = filter_code::fpt_highpass;
double fc = freq_cutoff1;
double Q = 1.0;
double db_gain = 0;
vector<double> vcoeff;

usr_hpf_iir0_I0.fc = freq_cutoff1;										//store just for ref not used in filter calcs
usr_hpf_iir0_I0.q = 1.0f;
usr_hpf_iir0_I0.db_gain = 0.0f;

calc_filter_iir_2nd_order( filt_type, fc, Q, db_gain, downsample_srate, vcoeff );
if( !create_iir_filter_from_coeffs( usr_hpf_iir0_I0, vcoeff ) )
	{
	printf( "filter_iir0_adjust() - failed at 'create_iir_filter_from_coeffs( usr_hpf_iir0_I0, vcoeff )'\n" );
	}
else{
	printf( "filter_iir0_adjust() - 'usr_hpf_iir0_I0'  srate %d  coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", (int)g_dev_bw, vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----



//----
iir_init( usr_hpf_iir0_Q0 );

//filt_type = filter_code::fpt_lowpass;
//fc1 = 10000;
//Q = 1.0;
//db_gain = 0;


//calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, g_dev_bw, vcoeff );
if( !create_iir_filter_from_coeffs( usr_hpf_iir0_Q0, vcoeff ) )
	{
	printf( "filter_iir0_adjust() - failed at 'create_iir_filter_from_coeffs( usr_hpf_iir0_Q0, vcoeff )'\n" );
	}
else{
	printf( "filter_iir0_adjust() - 'usr_hpf_iir0_Q0'  srate %d coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", (int)g_dev_bw, vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----
//------------------


//printf( "**********************filter_iir0_adjust() - getchar()\n");
 
//getchar();








//------------------
//----
iir_init( usr_lpf_iir0_I0 );

filt_type = filter_code::fpt_lowpass;
fc = freq_cutoff0;
Q = 1.0;
db_gain = 0;


usr_lpf_iir0_I0.fc = freq_cutoff0;										//store just for ref not used in filter calcs
usr_lpf_iir0_I0.q = 1.0f;
usr_lpf_iir0_I0.db_gain = 0.0f;

calc_filter_iir_2nd_order( filt_type, fc, Q, db_gain, downsample_srate, vcoeff );
if( !create_iir_filter_from_coeffs( usr_lpf_iir0_I0, vcoeff ) )
	{
	printf( "filter_iir0_adjust() - failed at 'create_iir_filter_from_coeffs( usr_lpf_iir0_I0, vcoeff )'\n" );
	}
else{
	printf( "filter_iir0_adjust() - 'usr_lpf_iir0_I0'  srate %d  coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", (int)g_dev_bw, vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----



//----
iir_init( usr_lpf_iir0_Q0 );

//filt_type = filter_code::fpt_lowpass;
//fc1 = 10000;
//Q = 1.0;
//db_gain = 0;


//calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, g_dev_bw, vcoeff );
if( !create_iir_filter_from_coeffs( usr_lpf_iir0_Q0, vcoeff ) )
	{
	printf( "filter_iir0_adjust() - failed at 'create_iir_filter_from_coeffs( usr_lpf_iir0_Q0, vcoeff )'\n" );
	}
else{
	printf( "filter_iir0_adjust() - 'usr_lpf_iir0_Q0'  srate %d coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", (int)g_dev_bw, vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----
//------------------


usr_hpf_iir0_I0.bypass = 0;
usr_hpf_iir0_Q0.bypass = 0;

usr_lpf_iir0_I0.bypass = 0;
usr_lpf_iir0_Q0.bypass = 0;


return 1;
}












//see also 'filters_create()'
bool filter_iir1_adjust( float freq_cutoff )
{
bool vb = 1;
mystr m1;

if(vb)printf( "filter_iir1_adjust() - srate %f, freq_cutoff %f\n", downsample_srate, freq_cutoff );

if( ( freq_cutoff < 0 ) || ( freq_cutoff > downsample_srate/2 )  )
	{
	printf( "filter_iir1_adjust() - freq_cutoff required is out of range: %f, (must be between 0-->%d)\n)", freq_cutoff, downsample_srate/2 );
	return 0;
	}



usr_lpf_iir1_I0.bypass = 1;
usr_lpf_iir1_Q0.bypass = 1;

m1.delay_ms( 100 );														//as a precaution for atomicity, give the 'demod_iso()' thread time to see this 'bypass' flag change

//------------------
//----
iir_init( usr_lpf_iir1_I0 );

filter_code::en_filter_pass_type_tag filt_type = filter_code::fpt_lowpass;
double fc = freq_cutoff;
double Q = 1.0;
double db_gain = 0;
vector<double> vcoeff;

calc_filter_iir_2nd_order( filt_type, fc, Q, db_gain, downsample_srate, vcoeff );
if( !create_iir_filter_from_coeffs( usr_lpf_iir1_I0, vcoeff ) )
	{
	printf( "filter_iir1_adjust() - failed at 'create_iir_filter_from_coeffs( usr_lpf_iir1_I0, vcoeff )'\n" );
	}
else{
	printf( "filter_iir1_adjust() - 'usr_lpf_iir1_I0'  srate %d  coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", (int)g_dev_bw, vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----



//----
iir_init( usr_lpf_iir1_Q0 );

//filt_type = filter_code::fpt_lowpass;
//fc1 = 10000;
//Q = 1.0;
//db_gain = 0;


//calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, g_dev_bw, vcoeff );
if( !create_iir_filter_from_coeffs( usr_lpf_iir1_Q0, vcoeff ) )
	{
	printf( "filter_iir1_adjust() - failed at 'create_iir_filter_from_coeffs( usr_lpf_iir1_Q0, vcoeff )'\n" );
	}
else{
	printf( "filter_iir1_adjust() - 'usr_lpf_iir1_Q0'  srate %d coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", (int)g_dev_bw, vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----
//------------------


usr_lpf_iir1_I0.bypass = 0;
usr_lpf_iir1_Q0.bypass = 0;


return 1;
}
















//see also 'filters_create()'
bool filter_iir2_adjust( float freq_cutoff )
{
bool vb = 1;
mystr m1;

if(vb)printf( "filter_iir2_adjust() - srate %d, freq_cutoff %f\n", downsample_srate, freq_cutoff );

if( ( freq_cutoff < 0.0f ) || ( freq_cutoff > downsample_srate/2 )  )
	{
	printf( "filter_iir2_adjust() - freq_cutoff required is out of range: %f, (must be between 0-->%d)\n)", freq_cutoff, downsample_srate/2 );
	return 0;
	}



usr_lpf_iir2_I0.bypass = 1;
usr_lpf_iir2_Q0.bypass = 1;

m1.delay_ms( 100 );														//as a precaution for atomicity, give the 'demod_iso()' thread time to see this 'bypass' flag change

//------------------
//----
iir_init( usr_lpf_iir2_I0 );

filter_code::en_filter_pass_type_tag filt_type = filter_code::fpt_lowpass;
double fc = freq_cutoff;
double Q = 1.0;
double db_gain = 0;
vector<double> vcoeff;

calc_filter_iir_2nd_order( filt_type, fc, Q, db_gain, downsample_srate, vcoeff );
if( !create_iir_filter_from_coeffs( usr_lpf_iir2_I0, vcoeff ) )
	{
	printf( "filter_iir2_adjust() - failed at 'create_iir_filter_from_coeffs( usr_lpf_iir2_I0, vcoeff )'\n" );
	}
else{
	printf( "filter_iir2_adjust() - 'usr_lpf_iir2_I0'  srate %d  coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", (int)g_dev_bw, vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----



//----
iir_init( usr_lpf_iir2_Q0 );

//filt_type = filter_code::fpt_lowpass;
//fc1 = 10000;
//Q = 1.0;
//db_gain = 0;


//calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, g_dev_bw, vcoeff );
if( !create_iir_filter_from_coeffs( usr_lpf_iir2_Q0, vcoeff ) )
	{
	printf( "filter_iir2_adjust() - failed at 'create_iir_filter_from_coeffs( usr_lpf_iir2_Q0, vcoeff )'\n" );
	}
else{
	printf( "filter_iir2_adjust() - 'usr_lpf_iir2_Q0'  srate %d coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", (int)g_dev_bw, vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----
//------------------


usr_lpf_iir2_I0.bypass = 0;
usr_lpf_iir2_Q0.bypass = 0;


return 1;
}









//build filter transfer function vectors for gui display
void filter_build_bode( unsigned int idx )
{

if( idx >= cn_filters_max ) return;

st_filter_sdr_tag *of = st_filt + idx;

if( ! (of->flags | en_fflg_alloc ) ) return;

vector<double> vnumer;
vector<double> vdenom;
vector<double> vfrq;
vector<double> vamp;



st_bode[idx].vx.clear();
st_bode[idx].vy.clear();
if(of->vcef.size() >= 5 )
	{
	vnumer.push_back( of->vcef[ 2] );
	vnumer.push_back( of->vcef[ 3 ] );
	vnumer.push_back( of->vcef[ 4 ] );

	vdenom.push_back( 1.0f );
	vdenom.push_back( of->vcef[ 0 ] );
	vdenom.push_back( of->vcef[ 1 ] );

/*
	int num_points = 300;
	if( downsample_srate >= 12000 ) num_points = 400;
	if( downsample_srate >= 24000 ) num_points = 600;
	if( downsample_srate >= 48000 ) num_points = 800;
	if( downsample_srate >= 96000 ) num_points = 1000;
	if( downsample_srate >= 192000 ) num_points = 1200;
	if( downsample_srate >= 240000 ) num_points = 1800;
*/	
//	dsp_utils_code::freqz_sos_inline( vnumer, vdenom, num_points, vomega, vamp );					//analyse iir response

	int num_points = 300;
	int delta = 500;
	float frq = of->fc0 - delta;										//start freq
	float step = (2.0*delta) / num_points;
	for( int i = 0; i < num_points; i++ )
		{
		float f0;
		
		frq += step;
		dsp_utils_code::freqz_sos_inline_single_freq( vnumer, vdenom, downsample_srate, frq, f0 );	//gather iir response
//		printf( "filter_build_bode() - %04d:  %f %f\n", i, frq, f0 );
		
		vfrq.push_back( frq );
		vamp.push_back( f0 );
		}

	st_bode[idx].vx = vfrq;
	st_bode[idx].vy = vamp;
	}

}













void filter_create_new()
{
printf("\n\n" );
printf( "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF\n" ); 
bool vb = 0;



//---------- prepare st_filt[ii].iir[0] -----------

int ii = en_ftid_iir_notch0_aud;

st_filt[ii].flags = (en_filter_flag_tag)en_fflg_zero | en_fflg_alloc;	//set filter as alloc'd, set for iir (not en_fflg_fir) 

int jj = 0;
st_aud_notch_gui_ctrls_tag ano = st_aud_notch_gui_ctrls[jj];
if( ano.active ) st_filt[ii].flags |= en_fflg_on;						//req to be on?

st_filt[ii].sname = "en_ftid_iir_notch0_aud";
st_filt[ii].srate = downsample_srate;
st_filt[ii].db_gain = 0.0f;
//st_filt[ii].fc0 = 400.0f;												//this set in: 'filters_create()'  'filter_iir_rebuild()'
st_filt[ii].fc1 = 0.0f;
//st_filt[ii].Q = 7.0f;
st_filt[ii].taps = 0;
st_filt[ii].active_idx = 0;				//this is the index of which iir[] the audio thrd is using, audio thrd switches this when 'pending_idx' is not -1
st_filt[ii].inactive_idx = 1;			//set this to 1 as 'st_filt[ii].iir[0]' will be first iir put into service in audio thrd	
st_filt[ii].pending_idx = -1;			//this is an atomic obj
st_filt[ii].iir[0].bypass = 0;


if(vb)printf( "filter_create_new() - srate %d, freq_cutoff %f  Q %f, '%s'\n", downsample_srate, st_filt[ii].fc0, st_filt[ii].Q, st_filt[ii].sname.c_str() );

if( ( st_filt[ii].fc0 < 0.0f ) || ( st_filt[ii].fc0 > downsample_srate/2 )  )
	{
	printf( "filter_create_new() - freq_cutoff required is out of range: %f, (must be between 0-->%d), '%s'\n)", st_filt[ii].fc0, downsample_srate/2, st_filt[ii].sname.c_str() );
//	return 0;
	}


filter_code::iir_init( st_filt[ii].iir[0] );
filter_code::en_filter_pass_type_tag filt_type = filter_code::fpt_notch;

calc_filter_iir_2nd_order( filt_type, st_filt[ii].fc0, st_filt[ii].Q, st_filt[ii].db_gain , downsample_srate, st_filt[ii].vcef );

if( !create_iir_filter_from_coeffs( st_filt[ii].iir[0], st_filt[ii].vcef ) )
	{
	printf( "filter_iir_notch_adjust() - failed at 'create_iir_filter_from_coeffs(), st_filt[%d][0], '%s'\n", ii, st_filt[ii].sname.c_str() );
	}
else{
	printf( "filter_iir_notch_adjust() - st_filt[%d][1]: '%s'  srate %d  coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", ii, st_filt[ii].sname.c_str(), st_filt[ii].srate, st_filt[ii].vcef[0], st_filt[ii].vcef[1], st_filt[ii].vcef[2], st_filt[ii].vcef[3], st_filt[ii].vcef[4] );
	}


if( !create_iir_filter_from_coeffs( st_filt[ii].iir[1], st_filt[ii].vcef ) )
	{
	printf( "filter_iir_notch_adjust() - failed at 'create_iir_filter_from_coeffs(), st_filt[%d][1], '%s'\n", ii, st_filt[ii].sname.c_str() );
	}
else{
	printf( "filter_iir_notch_adjust() - st_filt[%d][1]: '%s'  srate %d  coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", ii, st_filt[ii].sname.c_str(), st_filt[ii].srate, st_filt[ii].vcef[0], st_filt[ii].vcef[1], st_filt[ii].vcef[2], st_filt[ii].vcef[3], st_filt[ii].vcef[4] );
	}

//----------












//---------- prepare st_filt[ii].iir[0] -----------
ii = en_ftid_iir_notch1_aud;


st_filt[ii].flags = (en_filter_flag_tag)en_fflg_zero | en_fflg_alloc;	//set filter as alloc'd, set for iir (not en_fflg_fir) 

jj = 1;
ano = st_aud_notch_gui_ctrls[jj];		
if( ano.active ) st_filt[ii].flags |= en_fflg_on;						//req to be on?

st_filt[ii].sname = "en_ftid_iir_notch1_aud";
st_filt[ii].srate = downsample_srate;
st_filt[ii].db_gain = 0.0f;
//st_filt[ii].fc0 = 1800.0f;											//this set in: 'filters_create()'  'filter_iir_rebuild()'
st_filt[ii].fc1 = 0.0f;
//st_filt[ii].Q = 7.0f;
st_filt[ii].taps = 0;
st_filt[ii].active_idx = 0;				//this is the index of which iir[] the audio thrd is using, audio thrd switches this when 'pending_idx' is not -1
st_filt[ii].inactive_idx = 1;			//set this to 1 as 'st_filt[ii].iir[0]' will be first iir put into service in audio thrd	
st_filt[ii].pending_idx = -1;			//this is an atomic obj
st_filt[ii].iir[0].bypass = 0;


if(vb)printf( "filter_create_new() - srate %d, freq_cutoff %f  Q %f, '%s'\n", downsample_srate, st_filt[ii].fc0, st_filt[ii].Q, st_filt[ii].sname.c_str() );

if( ( st_filt[ii].fc0 < 0.0f ) || ( st_filt[ii].fc0 > downsample_srate/2 )  )
	{
	printf( "filter_create_new() - freq_cutoff required is out of range: %f, (must be between 0-->%d), '%s'\n)", st_filt[ii].fc0, downsample_srate/2, st_filt[ii].sname.c_str() );
//	return 0;
	}


filter_code::iir_init( st_filt[ii].iir[0] );
filt_type = filter_code::fpt_notch;

calc_filter_iir_2nd_order( filt_type, st_filt[ii].fc0, st_filt[ii].Q, st_filt[ii].db_gain , downsample_srate, st_filt[ii].vcef );

if( !create_iir_filter_from_coeffs( st_filt[ii].iir[0], st_filt[ii].vcef ) )
	{
	printf( "filter_iir_notch_adjust() - failed at 'create_iir_filter_from_coeffs(), st_filt[%d][0], '%s'\n", ii, st_filt[ii].sname.c_str() );
	}
else{
	printf( "filter_iir_notch_adjust() - st_filt[%d][1]: '%s'  srate %d  coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", ii, st_filt[ii].sname.c_str(), st_filt[ii].srate, st_filt[ii].vcef[0], st_filt[ii].vcef[1], st_filt[ii].vcef[2], st_filt[ii].vcef[3], st_filt[ii].vcef[4] );
	}


if( !create_iir_filter_from_coeffs( st_filt[ii].iir[1], st_filt[ii].vcef ) )
	{
	printf( "filter_iir_notch_adjust() - failed at 'create_iir_filter_from_coeffs(), st_filt[%d][1], '%s'\n", ii, st_filt[ii].sname.c_str() );
	}
else{
	printf( "filter_iir_notch_adjust() - st_filt[%d][1]: '%s'  srate %d  coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", ii, st_filt[ii].sname.c_str(), st_filt[ii].srate, st_filt[ii].vcef[0], st_filt[ii].vcef[1], st_filt[ii].vcef[2], st_filt[ii].vcef[3], st_filt[ii].vcef[4] );
	}

//----------




//---------- prepare st_filt[ii].iir[0] -----------
ii = en_ftid_iir_fmst_19k_pilot_notch;

st_filt[ii].flags = (en_filter_flag_tag)en_fflg_zero | en_fflg_alloc | en_fflg_on;	//set filter as alloc'd, set for iir (not en_fflg_fir) 

st_filt[ii].sname = "en_ftid_iir_notch_fm_19k_pilot";
st_filt[ii].srate = fm_decimator_srate;									//use halfband downsampler srate
st_filt[ii].db_gain = 0.0f;
//st_filt[ii].fc0 = 1800.0f;											//this set in: 'filters_create()'  'filter_iir_rebuild()'
st_filt[ii].fc1 = 0.0f;
//st_filt[ii].Q = 7.0f;
st_filt[ii].taps = 0;
st_filt[ii].active_idx = 0;				//this is the index of which iir[] the audio thrd is using, audio thrd switches this when 'pending_idx' is not -1
st_filt[ii].inactive_idx = 1;			//set this to 1 as 'st_filt[ii].iir[0]' will be first iir put into service in audio thrd	
st_filt[ii].pending_idx = -1;			//this is an atomic obj
st_filt[ii].iir[0].bypass = 0;
st_filt[ii].iir[1].bypass = 0;


if(vb)printf( "filter_create_new() - srate %d, freq_cutoff %f  Q %f, '%s'\n", st_filt[ii].srate, st_filt[ii].fc0, st_filt[ii].Q, st_filt[ii].sname.c_str() );

if( ( st_filt[ii].fc0 < 0.0f ) || ( st_filt[ii].fc0 > st_filt[ii].srate / 2 )  )
	{
	printf( "filter_create_new() - freq_cutoff required is out of range: %f, (must be between 0-->%d), '%s'\n)", st_filt[ii].fc0, downsample_srate/2, st_filt[ii].sname.c_str() );
//	return 0;
	}


filter_code::iir_init( st_filt[ii].iir[0] );
filt_type = filter_code::fpt_notch;

calc_filter_iir_2nd_order( filt_type, st_filt[ii].fc0, st_filt[ii].Q, st_filt[ii].db_gain , st_filt[ii].srate, st_filt[ii].vcef );

if( !create_iir_filter_from_coeffs( st_filt[ii].iir[0], st_filt[ii].vcef ) )
	{
	printf( "filter_iir_notch_adjust() - failed at 'create_iir_filter_from_coeffs(), st_filt[%d][0], '%s'\n", ii, st_filt[ii].sname.c_str() );
	}
else{
	printf( "filter_iir_notch_adjust() - st_filt[%d][1]: '%s'  srate %d  coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", ii, st_filt[ii].sname.c_str(), st_filt[ii].srate, st_filt[ii].vcef[0], st_filt[ii].vcef[1], st_filt[ii].vcef[2], st_filt[ii].vcef[3], st_filt[ii].vcef[4] );
	}


if( !create_iir_filter_from_coeffs( st_filt[ii].iir[1], st_filt[ii].vcef ) )
	{
	printf( "filter_iir_notch_adjust() - failed at 'create_iir_filter_from_coeffs(), st_filt[%d][1], '%s'\n", ii, st_filt[ii].sname.c_str() );
	}
else{
	printf( "filter_iir_notch_adjust() - st_filt[%d][1]: '%s'  srate %d  coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", ii, st_filt[ii].sname.c_str(), st_filt[ii].srate, st_filt[ii].vcef[0], st_filt[ii].vcef[1], st_filt[ii].vcef[2], st_filt[ii].vcef[3], st_filt[ii].vcef[4] );
	}

//----------







//---------- prepare st_filt[ii].fir[0] -----------
ii = en_ftid_fir_fmst_15k_lpf0;

st_filt[ii].flags = (en_filter_flag_tag)en_fflg_zero | en_fflg_fir | en_fflg_alloc | en_fflg_on;	//set filter as alloc'd, set for fir

st_filt[ii].sname = "en_ftid_fir_fmst_15k_lpf0";
st_filt[ii].srate = fm_decimator_srate;									//use halfband downsampler srate
st_filt[ii].db_gain = 0.0f;
//st_filt[ii].fc0 = 1800.0f;											//this set in: 'filters_create()'  'filter_iir_rebuild()'
st_filt[ii].fc1 = 0.0f;
//st_filt[ii].Q = 7.0f;
st_filt[ii].taps = 55;
st_filt[ii].active_idx = 0;				//this is the index of which iir[] the audio thrd is using, audio thrd switches this when 'pending_idx' is not -1
st_filt[ii].inactive_idx = 1;			//set this to 1 as 'st_filt[ii].iir[0]' will be first iir put into service in audio thrd	
st_filt[ii].pending_idx = -1;			//this is an atomic obj
//st_filt[ii].fir[0].bypass = 0;


if(vb)printf( "filter_create_new() - srate %d, freq_cutoff %f  Q %f, '%s'\n", st_filt[ii].srate, st_filt[ii].fc0, st_filt[ii].Q, st_filt[ii].sname.c_str() );

if( ( st_filt[ii].fc0 < 0.0f ) || ( st_filt[ii].fc0 > st_filt[ii].srate / 2 )  )
	{
	printf( "filter_create_new() - freq_cutoff required is out of range: %f, (must be between 0-->%d), '%s'\n)", st_filt[ii].fc0, downsample_srate/2, st_filt[ii].sname.c_str() );
//	return 0;
	}



filter_code::en_filter_window_type_tag wnd_type  =  filter_code::fwt_kaiser;
filt_type = filter_code::fpt_lowpass;
filter_code::kaiser_beta = 6;

st_filt[ii].fir[0].verb = 1;
st_filt[ii].fir[0].suser0 = "en_ftid_fir_fmst_15k_lpf0";
st_filt[ii].fir[0].user_id0 = 0;
st_filt[ii].fir[0].created = 0;
st_filt[ii].fir[1].created = 0;											//also clear created flag for filter ping pong alternate

filter_code::filter_fir_windowed( wnd_type, filt_type, st_filt[ii].taps, st_filt[ii].fc0, st_filt[ii].fc1, fm_decimator_srate, st_filt[ii].vcef );
filter_code::create_filter_from_coeffs( st_filt[ii].fir[0], st_filt[ii].vcef );
filter_code::fir_init( st_filt[ii].fir[0] );
st_filt[ii].fir[0].bypass = 0;
//----------




//---------- prepare st_filt[ii].fir[0] -----------
ii = en_ftid_fir_fmst_15k_lpf1;

st_filt[ii].flags = (en_filter_flag_tag)en_fflg_zero | en_fflg_fir | en_fflg_alloc | en_fflg_on;	//set filter as alloc'd, set for fir

st_filt[ii].sname = "en_ftid_fir_fmst_15k_lpf1";
st_filt[ii].srate = fm_decimator_srate;									//use halfband downsampler srate
st_filt[ii].db_gain = 0.0f;
//st_filt[ii].fc0 = 1800.0f;											//this set in: 'filters_create()'  'filter_iir_rebuild()'
st_filt[ii].fc1 = 0.0f;
//st_filt[ii].Q = 7.0f;
st_filt[ii].taps = 55;
st_filt[ii].active_idx = 0;				//this is the index of which iir[] the audio thrd is using, audio thrd switches this when 'pending_idx' is not -1
st_filt[ii].inactive_idx = 1;			//set this to 1 as 'st_filt[ii].iir[0]' will be first iir put into service in audio thrd	
st_filt[ii].pending_idx = -1;			//this is an atomic obj
//st_filt[ii].fir[0].bypass = 0;


if(vb)printf( "filter_create_new() - srate %d, freq_cutoff %f  Q %f, '%s'\n", st_filt[ii].srate, st_filt[ii].fc0, st_filt[ii].Q, st_filt[ii].sname.c_str() );

if( ( st_filt[ii].fc0 < 0.0f ) || ( st_filt[ii].fc0 > st_filt[ii].srate / 2 )  )
	{
	printf( "filter_create_new() - freq_cutoff required is out of range: %f, (must be between 0-->%d), '%s'\n)", st_filt[ii].fc0, downsample_srate/2, st_filt[ii].sname.c_str() );
//	return 0;
	}



wnd_type  = filter_code::fwt_kaiser;
filt_type = filter_code::fpt_lowpass;
filter_code::kaiser_beta = 6;

st_filt[ii].fir[0].verb = 1;
st_filt[ii].fir[0].suser0 = "en_ftid_fir_fmst_15k_lpf1";
st_filt[ii].fir[0].user_id0 = 0;
st_filt[ii].fir[0].created = 0;
st_filt[ii].fir[1].created = 0;											//also clear created flag for filter ping pong alternate

filter_code::filter_fir_windowed( wnd_type, filt_type, st_filt[ii].taps, st_filt[ii].fc0, st_filt[ii].fc1, fm_decimator_srate, st_filt[ii].vcef );
filter_code::create_filter_from_coeffs( st_filt[ii].fir[0], st_filt[ii].vcef );
filter_code::fir_init( st_filt[ii].fir[0] );
st_filt[ii].fir[0].bypass = 0;

//----------







//---------- prepare st_filt[ii].fir[0] -----------
ii = en_ftid_fir_fmst_23_53k_bpf;

st_filt[ii].flags = (en_filter_flag_tag)en_fflg_zero | en_fflg_fir | en_fflg_alloc | en_fflg_on;	//set filter as alloc'd, set for fir

st_filt[ii].sname = "en_ftid_fir_fmst_23_53k_bpf";
st_filt[ii].srate = fm_decimator_srate;									//use halfband downsampler srate
st_filt[ii].db_gain = 0.0f;
//st_filt[ii].fc0 = 1800.0f;											//this set in: 'filters_create()'  'filter_iir_rebuild()'
//st_filt[ii].fc1 = 0.0f;
//st_filt[ii].Q = 7.0f;
st_filt[ii].taps = cn_fm_stereo_bpf_taps;
st_filt[ii].active_idx = 0;				//this is the index of which iir[] the audio thrd is using, audio thrd switches this when 'pending_idx' is not -1
st_filt[ii].inactive_idx = 1;			//set this to 1 as 'st_filt[ii].iir[0]' will be first iir put into service in audio thrd	
st_filt[ii].pending_idx = -1;			//this is an atomic obj
//st_filt[ii].fir[0].bypass = 0;


if(vb)printf( "filter_create_new() - srate %d, freq_cutoffs %f %f  Q %f, '%s'\n", st_filt[ii].srate, st_filt[ii].fc0, st_filt[ii].fc1, st_filt[ii].Q, st_filt[ii].sname.c_str() );

if( ( st_filt[ii].fc0 < 0.0f ) || ( st_filt[ii].fc0 > st_filt[ii].srate / 2 )  )
	{
	printf( "filter_create_new() - freq_cutoff required is out of range: %f, (must be between 0-->%d), '%s'\n)", st_filt[ii].fc0, downsample_srate/2, st_filt[ii].sname.c_str() );
//	return 0;
	}



wnd_type  = filter_code::fwt_kaiser;
filt_type = filter_code::fpt_bandpass;
filter_code::kaiser_beta = 1;

st_filt[ii].fir[0].verb = 1;
st_filt[ii].fir[0].suser0 = "en_ftid_fir_fmst_23_53k_bpf";
st_filt[ii].fir[0].user_id0 = 0;
st_filt[ii].fir[0].created = 0;
st_filt[ii].fir[1].created = 0;											//also clear created flag for filter ping pong alternate

filter_code::filter_fir_windowed( wnd_type, filt_type, st_filt[ii].taps, st_filt[ii].fc0, st_filt[ii].fc1, fm_decimator_srate, st_filt[ii].vcef );
filter_code::create_filter_from_coeffs( st_filt[ii].fir[0], st_filt[ii].vcef );
filter_code::fir_init( st_filt[ii].fir[0] );
st_filt[ii].fir[0].bypass = 0;

//----------
}











//see also 'filters_create()'
bool filter_iir_notch_adjust( en_filter_idx_tag filt_idx, float freq_cutoff,  float Q_in )
{
bool vb = 1;
mystr m1;

m1.time_start( m1.ns_tim_start );


int ii = filt_idx;

float dt;

//check audio thrd is ready for a filter switch
while( 1 )
	{
	if( st_filt[ii].pending_idx.load( std::memory_order_acquire ) == -1 ) break;
	
	dt = m1.time_passed( m1.ns_tim_start );
	if( dt > 0.1 )
		{
		if(vb)printf( "filter_iir_notch_adjust() - ZZZZZZZZZZZZZZZZZZZZZ timeout waiting for audio thrd to switch to 'st_filt[].pending_idx'\n" );
		return 0;	
		}
	m1.delay_ms(1);														//don't hog processor
	}


if(vb)printf( "filter_iir_notch_adjust() - JJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJ filt_idx %d  srate %d   freq_cutoff %f   Q %f\n", filt_idx, downsample_srate, freq_cutoff, Q_in );

//st_filt[ii].flags = (en_filter_flag_tag)en_filtflg_zero | en_filtflg_in_use;
//st_filt[ii].sname = "notch_aud0";
//st_filt[ii].srate = downsample_srate;
//st_filt[ii].db_gain = 0.0f;

//if( enable ) st_filt[ii].flags |= en_fflg_on;
//else st_filt[filt_idx].flags &= ~en_fflg_on; 


st_filt[ii].fc0 = freq_cutoff;
//st_filt[ii].fc1 = 0.0f;
st_filt[ii].Q = Q_in;
//st_filt[ii].taps = 0;

//st_filt[ii].active_idx = 0;											//audio thrd toggles this

int idx = st_filt[ii].inactive_idx;										//get idle filter's idx
st_filt[ii].pending_idx = -1;

if(vb)printf( "filter_iir_notch_adjust() - filt_idx %d  srate %d, freq_cutoff %f  Q %f, st_filt[%d][%d], '%s'\n", ii, st_filt[ii].srate , st_filt[ii].fc0, st_filt[ii].Q, ii, idx, st_filt[ii].sname.c_str() );

if( ( st_filt[ii].fc0 < 0.0f ) || ( st_filt[ii].fc0 > st_filt[ii].srate / 2 )  )
	{
	printf( "filter_iir_notch_adjust() - freq_cutoff required is out of range: %f, (must be between 0-->%d), st_filt[%d][%d], '%s'\n)", st_filt[ii].fc0, downsample_srate/2, ii, idx, st_filt[ii].sname.c_str() );
//	return 0;
	}


//filter_code::iir_init( st_filt[ii].iir[idx] );
filter_code::en_filter_pass_type_tag filt_type = filter_code::fpt_notch;

calc_filter_iir_2nd_order( filt_type, st_filt[ii].fc0, st_filt[ii].Q, st_filt[ii].db_gain , st_filt[ii].srate , st_filt[ii].vcef );

if( !create_iir_filter_from_coeffs( st_filt[ii].iir[idx], st_filt[ii].vcef ) )
	{
	printf( "filter_iir_notch_adjust() - st_filt[%d][%d], failed at 'create_iir_filter_from_coeffs() '%s'\n", ii, idx, st_filt[ii].sname.c_str() );
	}
else{
	printf( "filter_iir_notch_adjust() - st_filt[%d][%d], '%s'  srate %d  coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", ii, idx, st_filt[ii].sname.c_str(), st_filt[ii].srate, st_filt[ii].vcef[0], st_filt[ii].vcef[1], st_filt[ii].vcef[2], st_filt[ii].vcef[3], st_filt[ii].vcef[4] );
	}

//pending_idx.store( inactive, std::memory_order_release );




st_filt[ii].pending_idx.store( st_filt[ii].inactive_idx, std::memory_order_release );	//let audio thrd know the idle filter is ready for use

st_filt[ii].inactive_idx = !st_filt[ii].inactive_idx;					//toggle to show other iir is now avail for gui to modify


filter_build_bode( ii );

return 1;
}



















//see also 'filters_create()'
bool filter_iir_notch_adjust_old( float freq_cutoff,  float Q_in )
{

//filter_iir_notch_adjust( freq_cutoff, Q_in );

//return 1;

/*
bool vb = 1;
mystr m1;

if(vb)printf( "filter_iir_notch_adjust() - srate %f, freq_cutoff %f  Q %f\n", downsample_srate, freq_cutoff, Q_in );

if( ( freq_cutoff < 0.0f ) || ( freq_cutoff > downsample_srate/2 )  )
	{
	printf( "filter_iir_notch_adjust() - freq_cutoff required is out of range: %f, (must be between 0-->%d)\n)", freq_cutoff, downsample_srate/2 );
	return 0;
	}



usr_notch_iir0.bypass = 1;

m1.delay_ms( 100 );														//as a precaution for atomicity, give the 'demod_iso()' thread time to see this 'bypass' flag change

//------------------
//----
iir_init( usr_notch_iir0 );

filter_code::en_filter_pass_type_tag filt_type = filter_code::fpt_notch;
double fc = freq_cutoff;
double Q = Q_in;
double db_gain = 0;
vector<double> vcoeff;

calc_filter_iir_2nd_order( filt_type, fc, Q, db_gain, downsample_srate, vcoeff );
if( !create_iir_filter_from_coeffs( usr_notch_iir0, vcoeff ) )
	{
	printf( "filter_iir_notch_adjust() - failed at 'create_iir_filter_from_coeffs( usr_notch_iir0, vcoeff )'\n" );
	}
else{
	printf( "filter_iir_notch_adjust() - 'usr_notch_iir0'  srate %d  coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", (int)g_dev_bw, vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----



//----
iir_init( usr_notch_iir0 );

//filt_type = filter_code::fpt_lowpass;
//fc1 = 10000;
//Q = 1.0;
//db_gain = 0;


//calc_filter_iir_2nd_order( filt_type, fc1, Q, db_gain, g_dev_bw, vcoeff );
if( !create_iir_filter_from_coeffs( usr_notch_iir0, vcoeff ) )
	{
	printf( "filter_iir2_adjust() - failed at 'create_iir_filter_from_coeffs( usr_notch_iir0, vcoeff )'\n" );
	}
else{
	printf( "filter_iir2_adjust() - 'usr_notch_iir0'  srate %d coeffs:  a1 %f  a2 %f  b0 %f  b1 %f  b2 %f\n", (int)g_dev_bw, vcoeff[0], vcoeff[1], vcoeff[2], vcoeff[3], vcoeff[4] );
	}
//----
//------------------


usr_notch_iir0.bypass = 0;



return 1;
*/
}


















bool filter_fir_demod_adjust( float freq_lwr, float freq_upr, unsigned int taps )
{
mystr m1;

printf( "filter_fir_demod_adjust() - freq_lwr %f   freq_lwr %f    taps %d\n", freq_lwr, freq_upr, taps );

if( taps < 2 )
	{
	printf( "filter_fir_demod_adjust() - taps required is out of range: %d, (must be between 2-->300)\n)", taps );
	return 0;
	}


if( ( freq_lwr < 0.0f ) ||  (freq_lwr > downsample_srate/2.0f) )
	{
	printf( "filter_fir_demod_adjust() - freq_lwr required is out of range: %f, (must be between 0.0-->8000.0)\n)", downsample_srate/2.0f );
	return 0;
	}


if( ( freq_upr < 100.0f ) ||  (freq_upr > downsample_srate/2.0f) )
	{
	printf( "filter_fir_demod_adjust() - freq_lwr required is out of range: %f, (must be between 0.0-->%f)\n)", freq_lwr, downsample_srate/2.0f );
	return 0;
	}


usr_demod_bpf_fir_I0.bypass = 1;
usr_demod_bpf_fir_Q0.bypass = 1;

m1.delay_ms( 100 );													//as a precaution for atomicity, give the 'demod_iso()' thread time to see this 'bypass' flag change


filter_code::en_filter_window_type_tag wnd_type = filter_code::fwt_blackman_harris;
filter_code::en_filter_pass_type_tag filt_type = filter_code::fpt_bandpass;

vector<double> vcoeff;
//taps = 300;
float fc01 = freq_lwr;//wnd_rtl_graph->miw_filt_lwr->get_value_as_double();
float fc02 = freq_upr;//wnd_rtl_graph->miw_filt_upr->get_value_as_double();

float sampl_rate = downsample_srate;


filter_code::filter_fir_windowed( wnd_type, filt_type, taps, fc01, fc02, sampl_rate, vcoeff );

filter_code::create_filter_from_coeffs( usr_demod_bpf_fir_I0, vcoeff );
filter_code::fir_init( usr_demod_bpf_fir_I0 );

filter_code::create_filter_from_coeffs( usr_demod_bpf_fir_Q0, vcoeff );
filter_code::fir_init( usr_demod_bpf_fir_Q0 );

usr_demod_bpf_fir_I0.bypass = 0;
usr_demod_bpf_fir_Q0.bypass = 0;


return 1;
}












void filter_adjust_fir_bw_lower_upper()
{

float fc01 = wnd_rtl_graph->miw_filt_lwr->get_value_as_double();
float fc02 = wnd_rtl_graph->miw_filt_upr->get_value_as_double();
g_bw_taps = wnd_rtl_graph->miw_filt_taps->get_value_as_double();


filter_fir_demod_adjust( fc01, fc02, g_bw_taps );

}









void cb_miw_bw_lower_upper( Fl_Widget *w, void *v )
{
mystr m1;

int which = (intptr_t)v;
printf( "cb_miw_bw_lower_upper() - which %d\n", which );




if( which == 0 )
	{
	filter_adjust_fir_bw_lower_upper();

//	float fc01 = wnd_rtl_graph->miw_filt_lwr->get_value_as_double();
//	float fc02 = wnd_rtl_graph->miw_filt_upr->get_value_as_double();

//	filter_fir_demod_adjust( fc01, fc02, fir_taps );
	}


if( which == 1 )
	{
	filter_adjust_fir_bw_lower_upper();

//	float fc01 = wnd_rtl_graph->miw_filt_lwr->get_value_as_double();
//	float fc02 = wnd_rtl_graph->miw_filt_upr->get_value_as_double();

//	filter_fir_demod_adjust( fc01, fc02, fir_taps );
	}


if( which == 2 )
	{
	filter_adjust_fir_bw_lower_upper();

//	float fc01 = wnd_rtl_graph->miw_filt_lwr->get_value_as_double();
//	float fc02 = wnd_rtl_graph->miw_filt_upr->get_value_as_double();

//	fir_taps = wnd_rtl_graph->miw_filt_taps->get_value_as_double();

//	filter_fir_demod_adjust( fc01, fc02, fir_taps );
	}

wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_miw_bw_lower_upper()" );

//cb_bt_freq_tune( 0, 0 );
}









void cb_miw_gain_iq( Fl_Widget *w, void *v )
{
int which = (intptr_t)v;
printf( "cb_miw_gain_iq() - which %d\n", which );


if( which == 0 )
	{
	g_gain_iq = wnd_rtl_graph->miw_gain_iq->get_value_as_double();	
	}
}









void cb_bt_scan_history_start( Fl_Widget *, void * )
{
string s1;
double frq;

//printf("cb_bt_scan_history_start\n" );

if( !wnd_rtl_graph ) return;

Fl_Menu_Button *m = wnd_rtl_graph->fi_freq_start->menubutton();
//printf("cb_bt_scan_history_start1= %x\n", m );
//printf("cb_bt_scan_history_start2= %d\n", m->value() );

if( m->value() < 0 ) return;

show_scan_history_menu_sel( m->value() );

//printf(" cb_bt_scan_history_start %d %d\n", wnd_rtl_graph->last_scan_history_start ,m->value() );


//check if callback was due to a menu change, ie: ignore keyboard text change triggered callbacks
if( wnd_rtl_graph->last_scan_history_start != m->value() )		//has menu item been change?
	{
	
//	wnd_rtl_graph->fi_freq_start->value();
//	wnd_rtl_graph->fi_freq_stop->value( m->value() );

	wnd_rtl_graph->last_scan_history_start = m->value();		//remember for next callabck
	wnd_rtl_graph->last_scan_history_stop = m->value();			//remember for next callabck

	cb_bt_freq_single( 0, 0 );
	}



}






void cb_bt_scan_history_stop( Fl_Widget *, void * )
{
string s1;
double frq;


if( !wnd_rtl_graph ) return;

Fl_Menu_Button *m = wnd_rtl_graph->fi_freq_stop->menubutton();

if( m->value() < 0 ) return;

show_scan_history_menu_sel( m->value() );


//printf(" cb_bt_scan_history_stop %d %d\n", wnd_rtl_graph->last_scan_history_stop ,m->value() );


//check if callback was due to a menu change, ie: ignore keyboard text change triggered callbacks
if( wnd_rtl_graph->last_scan_history_stop != m->value() )		//has menu item been change?
	{
	
//	wnd_rtl_graph->fi_freq_stop->value();
//	wnd_rtl_graph->fi_freq_start->value( m->value() );
	
//	listen( frq );

	wnd_rtl_graph->last_scan_history_start = m->value();		//remember for next callabck
	wnd_rtl_graph->last_scan_history_stop = m->value();		//remember for next callabck

	cb_bt_freq_single( 0, 0 );
	}

}









void cb_vs_gain( Fl_Widget *, void * )
{
string s1;

if( !wnd_rtl_graph ) return;

float val = wnd_rtl_graph->fvs_gain->value();


printf( "gain= %f\n", val );

g_gain_aud = val;
}






void cb_fvs_play_pos( Fl_Widget *w, void *v )
{

if( !wnd_rtl_graph ) return;

Fl_Slider* o = (Fl_Slider*) w;


int val = o->value();

bool b_absolute_time = 1;
rec_play_change_play_position( b_absolute_time, val );

printf( "cb_fvs_play_pos= %d\n", val );
}









void cb_ck_aud_mute_combo( Fl_Widget *w, void *v )
{
Fl_Check_Button *ow = (Fl_Check_Button *)w;

int which = (intptr_t)v;
printf( "cb_ck_aud_mute_combo() - which %d\n", which );

if( which == 0 )
	{
	g_aud_mute = ow->value();
	}

if( which == 1 )
	{
	g_aud_mono = ow->value();
	}



}





/*
void cb_vs_squelch( Fl_Widget *, void * )
{
string s1;

if( !wnd_rtl_graph ) return;

double val = wnd_rtl_graph->fvs_squelch->value();

printf( "squelch= %lf\n", val );

g_squelch = val;

}
*/











void cb_ck_if_freq( Fl_Widget *, void * )
{

//cb_bt_freq_tune( 0, 0 );
wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_ck_if_freq()" );

}




void cb_bw_limit( Fl_Widget *, void * )
{

//cb_bt_freq_tune( 0, 0 );
wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_bw_limit()" );

}







void rtl_graph_wnd::freq_listen_using_fav_idx( unsigned int idx )
{
string s1;

if( idx >= vfav.size() ) return;

aud_dimmer();

//return;

st_favourite_freq_tag o = vfav[idx];

wnd_rtl_graph->fi_name->value( o.sname.c_str() );
wnd_rtl_graph->fi_group_name->value( o.sgroup.c_str() );

wnd_rtl_graph->miwp_tune->miw->set_value_from_double( o.freq_center );
wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( o.freq - o.freq_center );


wnd_rtl_graph->fi_demodul_mode->value( o.demod_type );
//wnd_rtl_graph->demodul_mode = o.demod_type;
demodul_mode_set( o.demod_type );


miw_freq_gain->set_value_from_double( o.dev_gain );

wnd_rtl_graph->ld_agc->ChangeCol( o.b_agc );
wnd_rtl_graph->fvs_gain->value( o.audio_gain );
g_gain_aud = o.audio_gain;


wnd_rtl_graph->ck_user_dwn_aa->value( o.b_dwn_aa );

wnd_rtl_graph->miw_dev_dwnconv_srate->set_value_from_double( o.i_dwn_srate );

wnd_rtl_graph->ck_if_freq->value( o.b_use_iffreq );
g_b_if_freq = o.b_use_iffreq;

wnd_rtl_graph->ck_bw_limit->value( o.b_bw_bpass );
g_b_bw_bpass = o.b_bw_bpass;


wnd_rtl_graph->miw_filt_lwr->set_value_from_double( o.iffreq_bw_low );

wnd_rtl_graph->miw_filt_upr->set_value_from_double( o.iffreq_bw_high );

wnd_rtl_graph->miw_filt_taps->set_value_from_double( o.iffreq_bw_taps );

wnd_rtl_graph->ld_direct_sampling->ChangeCol( o.dev_direct_sampling );

wnd_rtl_graph->miw_gain_iq->set_value_from_double( o.iq_gain );


wnd_rtl_graph->ld_fm_deemph->ChangeCol( o.deemph );
i_deemphasis = o.deemph;

//strpf( s1, "%d", o.iffreq_bw_high );
//wnd_rtl_graph->miw_filt_upr->set_value_from_str( s1.c_str() );
//g_bw_upper = o.iffreq_bw_high;

//strpf( s1, "%d", o.iffreq_bw_low );
//wnd_rtl_graph->fi_bw_lower->value( s1.c_str() );
//g_bw_lower = o.iffreq_bw_low;


freq_listen( 0, 1, 1, 1, "freq_listen_using_fav_idx()" );

wnd_rtl_graph->led_dwn_srate_memory_turn_off();
wnd_rtl_graph->led_filter_memory_turn_off();
}











void rtl_graph_wnd::freq_listen( bool history_add, bool bset_dev_gain, bool bset_dev_srate, bool bset_direct_sampling, string scaller )
{
string s1;
mystr m1;

printf( "rtl_graph_wnd::freq_listen() - HHHHHHHHHHHHHHHHHHHHHHHHHHH caller '%s'\n", scaller.c_str() );


if( rtl.open_status() )								//rtl opened already?
	{
	if( !threads_rtl_started )						//no threads running?, this may happen after a 'do_scan()' which uses non async data gathering (bulk reads)
		{
		strpf( s1, "Starting threads" );
		fl_alert( s1.c_str(), 0 );

		vspect.clear();

		start_audio();
		start_threads_rtl();
		
		filters_create();
		i_fftw_trig_plan_create_state = 0;

		m1.delay_ms( 80 );	
		}
	}

//if( !wnd_rtl_graph ) return;

//if( history_add ) wnd_rtl_graph->add_to_tune_history();



get_user_gui_control_params();


if( history_add ) 
	{
	int freq_tot = (int)g_freq_tune + (int)g_freq_sub_tune;

	strpf( s1, "%d,%d,%d", freq_tot, (int)g_freq_tune, (int)g_freq_sub_tune );
	wnd_rtl_graph->idb_tune->add_at_head( s1.c_str() );
	wnd_rtl_graph->idb_tune->trim_entry_count_to( cn_tune_history_dropdown_max, 0 );
	wnd_rtl_graph->idb_tune->select_entry( 0 );
	wnd_rtl_graph->idb_tune->scroll_into_view( 0 );
//	wnd_rtl_graph->idb_tune->redraw();
	}

//rtl_listen_stop();



if(bset_dev_srate) 
	{
	rtl.set_srate( g_dev_bw );
	}


if(bset_direct_sampling) set_dev_direct_sampling( g_dev_direct_sampling );





//if( idbg_0 == 1 ) return;




//s1 = wnd_rtl_graph->fi_freq_mouse->value();
//double frq = wnd_rtl_graph->miwp_tune->miw->get_value_as_double();
//sscanf( s1.c_str(), "%lf", &frq );

g_freq_offsets_only = g_tuning_offset;

if( wnd_rtl_graph->demodul_mode != en_dmt_wfm ) 
	{
//	if( g_b_if_freq ) g_freq_offsets_only += g_interfreq;
	}


g_freq_tune_offset = g_freq_tune + g_freq_offsets_only;


imode = en_mode_listen;
rtl_listen( g_freq_tune_offset );


//have put this gain change after above 'rtl_listen()' call, found the gain does not change for some reason if its before - FIX THIS
if(bset_dev_gain)
	{
	printf( "freq_listen() - GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG setting dev gain to %.2f\n", g_dev_gain );	

	rtl.set_gain( g_dev_gain );							//the gain needs to be set after 'set_dev_direct_sampling()' call
	}


//listen( g_freq_tune_offset );
printf( "freq_listen() - %d Hz (offset: %d Hz)\n", (int)g_freq_tune, (int)g_freq_tune_offset );
}











/*
void cb_bt_freq_tune( Fl_Widget *, void * )
{
wnd_rtl_graph->freq_listen( 1, 0, 0, 0 );

carrier_max_tune_timer = 0.0f;

}
*/





/*
void cb_bt_freq_tune_off( Fl_Widget *, void * )
{
rtl_listen_stop();
imode = en_mode_silence;

//call_rtl_fm_kill();
}
*/





//IF YOU change this ALSO MOD 'en_demodulator_type_tag'
Fl_Menu_Item pulldown_demod_mode[] = {
  {"FM Stereo",	FL_ALT+'s'},
  {"WFM",	FL_ALT+'w'},
  {"FM",	FL_ALT+'f'},
  {"AM",	FL_ALT+'a'},
  {"SSB",	FL_ALT+'b'},
//  {"LSB",	FL_ALT+'l'},
//  {"USB",	FL_ALT+'u'},
   {0}
};





void call_rtl_fm_kill()
{
mystr m1;

rtl_fm_running = 0;

RunShell( "./tunekill.sh" );
m1.delay_ms( 300 );
}




/*
void call_rtl_fm( double freq, int demodul_mode, int iq_srate, int lpass_srate, double gain, double squelch, int acr_tang, bool deemph )
{
string s1, s2, s3, s4;


if( demodul_mode == en_dmt_wfm ) s2 = "wfm";
if( demodul_mode == en_dmt_fm ) s2 = "fm";
if( demodul_mode == en_dmt_am ) s2 = "am";
if( demodul_mode == en_dmt_ssb ) s2 = "ssb";
//if( demodul_mode == en_dmt_lsb ) s2 = "lsb";
//if( demodul_mode == en_dmt_usb ) s2 = "usb";
//if( demodul_mode == en_dmt_raw ) s2 = "raw";


if( acr_tang == en_at_std ) s3 = "std";
if( acr_tang == en_at_fast ) s3 = "fast";
if( acr_tang == en_at_lut ) s3 = "lut";

s4 = "none";
if( deemph ) s4 = "deemp";
strpf( s1, "./tune.sh %lf %s %d %d %lf %lf %s %s",freq, s2.c_str(), iq_srate, lpass_srate, gain, squelch, s3.c_str(), s4.c_str() );

call_rtl_fm_kill();

rtl_fm_running = 1;

RunShell( s1 );
}
*/





//does one scan between freqs spec, using spec params
bool rtl_scan( float freq_start, float freq_stop, float freq_step, float gain, float bandwidth, float freq_reso, float threshold )
{
//printf( "rtl_scan() - is DISABLED see 'return' in code\n" );	
//return 1;

vspect.clear();

rtl_scan_count = freq_stop - freq_start;

rtl_scan_count /= freq_step;			//work out how many a/d sample points pic will send
rtl_scan_count++;						//make sure stop freq is used as well

rtl_scan_cur_freq = freq_start;
rtl_scan_freq_step = freq_step;
rtl_scan_cur_count = 0;


rtl.set_srate( bandwidth );
//rtl.set_srate( 200000 );
rtl.set_buf_size( freq_reso );

int bwidth = rtl.get_srate();

//rtl.set_gain( gain );

int freq_span = freq_stop - freq_start;
if( freq_span < bandwidth ) freq_span = bandwidth;

double tunes_req = (double)freq_span / (double)bwidth;

int itunes_req = ceil( tunes_req );

//itunes_req = 1;

printf("rtl_scan() - itunes_req %d\n", itunes_req );

for( int i = 0; i < itunes_req; i++ )
	{
	rtl.set_center_freq( freq_start + bwidth / 2 + i * bwidth );
	printf("rtl_scan() - %d: rtl.tune()= %d\n", i, (int)freq_start + bwidth / 2 + i * bwidth );
	
	rtl.read_fft_graph( vspect, 1 );
	}


//printf( "rtl_scan() - vspect.size()=%d\n", vspect.size() );

return 1;
}








bool rtl_get_graph()
{
printf( "rtl_get_graph()\n" );
//rtl.tune( frq );

vspect.clear();
rtl.read_fft_graph( vspect, 1 );
return 1;
}







//tune to spec freq, start audio callbacks
void rtl_listen( double freq )
{
string s1;

printf("rtl_listen: %lf\n", freq );


rtl.set_center_freq( freq );

b_listen = 1;				 	//this causes another thread to call the blocking function rtl.read_async(..)
}






void rtl_listen_stop()
{
printf("rtl_listen_stop()\n" );

b_listen = 0;				//this stops another thread from calling the blocking function rtl.read_async(..)
rtl.cancel_async( );		//stop librtl async callbacks
}






/*

//tune to marker freq
void tune_rtl_old()
{
string s1;

if( !wnd_rtl_graph ) return;

s1 = wnd_rtl_graph->fi_freq_mouse->value();
double frq;
sscanf( s1.c_str(), "%lf", &frq );

int dmod = wnd_rtl_graph->demodul_mode;
int iq_srate = 200000;
int lpass_srate = 48000;

s1 = wnd_rtl_graph->fi_freq_gain->value();
double gain;
sscanf( s1.c_str(), "%lf", &gain );


double squelch = 0;

int acr_tang = en_at_std;
bool deemp = 1;

rtl.close();

call_rtl_fm( frq, dmod, iq_srate, lpass_srate, gain, squelch, acr_tang, deemp );
}

*/





double mgraph_freq_tune = 0;
double mgraph_freq_sub_tune = 0;

int mgraph_grabx = 0;
int mgraph_graby = 0;




//mystr mtim_graph_left_click;											//use to detect click and release without drag





void rtl_graph_wnd::gph0_make_sel_sample_text()
{
string s1;
mystr m1;

int sel_idx;
if( gph0->get_selected_idx( 0, sel_idx ) )
	{
	double valx, valy;
	gph0->get_sample_value( 0, sel_idx, valx, valy );
	
	int fractional_digits = 3;
	string snum, sunits, scombined;
	m1.make_engineering_str( snum, sunits, scombined, fractional_digits, valx, " ", "Hz" );

	double db = pref_db_ref_A * log10( valy / pref_db_ref_B ) + pref_db_ref_C;
	strpf( s1, "sel [ %d ]:\nx: %.2f  (%s)\ny: %f   lev: %.2f %s             ", sel_idx, valx, scombined.c_str(), valy, db, pref_db_ref_suffix.c_str() );

	s_gph0_sel_sample = s1;
	}
else{
	s_gph0_sel_sample = "sel [ ?? ]: ???";
	}
}






void cb_graph_left_click_anywhere_cb( void *o )
{
string s1;

if( !wnd_rtl_graph ) return;



gph0_obj_sel_idx = -1;

//mtim_graph_left_click.time_start( mtim_graph_left_click.ns_tim_start );	//start timer for release callback 'cb_graph_left_click_release_cb' to check


mgraph_grabx = wnd_rtl_graph->gph0->mousex;
mgraph_graby = wnd_rtl_graph->gph0->mousey;

mgraph_freq_tune = wnd_rtl_graph->miwp_tune->miw->get_value_as_double();
mgraph_freq_sub_tune = wnd_rtl_graph->miw_freq_sub_tune->get_value_as_double();


printf( "cb_graph_left_click_anywhere_cb() mgraph_grabx % d %d\n", mgraph_grabx, mgraph_graby );

//wnd_rtl_graph->miwp_tune->miw->set_value_from_double( gph_mouse_freq );

//imode = en_mode_stop;
//wnd_rtl_graph->freq_listen( 1, 0, 0, 0 );


}








void cb_graph_right_click_release_cb( void *o )
{
string s1;

if( !wnd_rtl_graph ) return;

//---
if( gph0_obj_hover_idx != -1 )											//inside a 'st_gph_obj_tag' obj
	{
	
	if( vgph_obj[ gph0_obj_hover_idx ].type == en_got_demod_btn )		//demod button?
		{
		gph0_obj_sel_idx = gph0_obj_hover_idx;
		
		int iv = vgph_obj[ gph0_obj_hover_idx ].int0;

		demodul_mode_set( iv );
		}

	if( vgph_obj[ gph0_obj_hover_idx ].type == en_got_freq_pin )		//pinned freq?		
		{
		gph0_obj_sel_idx = gph0_obj_hover_idx;
		int pin_idx = vgph_obj[ gph0_obj_hover_idx ].pin_idx;// gph0_obj_sel_idx = gph0_obj_hover_idx;
		
//		wnd_rtl_graph->pin_conform( g_freq_tune );						//helps avoid unecessary annoying shifting of pins on graph when cycling between pins
		wnd_rtl_graph->pin_select_by_idx( pin_idx, 1, 1 );				//adj pin to center its 'freq_tune' and zero 'freq_sub_tune'
		}
//---
	}

}









//mouse drag freq tune
void cb_graph_left_click_release_cb( void *o )
{
string s1;

if( !wnd_rtl_graph ) return;

if( gph0_obj_hover_idx == -1 )											//not inside a 'st_gph_obj_tag' obj
	{
//	float dt = mtim_graph_left_click.time_passed( mtim_graph_left_click.ns_tim_start );

//	printf( "++++++++++++++++++ cb_graph_left_click_release_cb()0 - dt %f\n", dt );

//---
	bool b_drag = 0;
	int mouse_dx = wnd_rtl_graph->gph0->mousex - mgraph_grabx;
	int mouse_dy = wnd_rtl_graph->gph0->mousey - mgraph_graby;


	if( fabsf( mouse_dx ) > 1 ) 
		{
		b_drag = 1;
		}
//---



		
//	if( dt < 0.2f )															//click/release without drag
	if( !b_drag )															//click/release without drag
		{
//		printf( "++++++++++++++++++ cb_graph_left_click_release_cb() - without dragging\n" );

		int px, py;


		wnd_rtl_graph->gph0->get_mouse_pixel_position_on_background( px, py );

		int frq = gph_mouse_freq_in_bw;
		
		if( py < 20 )
			{
			frq /= 10;				//quantize to 10Hz boundaries
			frq *= 10;
			}
		else{
			frq /= 1000;			//quantize to 1KHz boundaries
			frq *= 1000;
			}
		
		wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( frq );

		wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_graph_left_click_release_cb() 0" );
		}
	}
else{
	//handle graph (draw)obj clicks
	
	if( vgph_obj[ gph0_obj_hover_idx ].type == en_got_demod_btn )		//demod button?
		{
		gph0_obj_sel_idx = gph0_obj_hover_idx;
		
		int iv = vgph_obj[ gph0_obj_hover_idx ].int0;

		demodul_mode_set( iv );
		}

	if( vgph_obj[ gph0_obj_hover_idx ].type == en_got_freq_pin )		//pinned freq?		
		{
		gph0_obj_sel_idx = gph0_obj_hover_idx;
		int pin_idx = vgph_obj[ gph0_obj_hover_idx ].pin_idx;// gph0_obj_sel_idx = gph0_obj_hover_idx;

		wnd_rtl_graph->pin_conform( g_freq_tune );						//helps avoid unecessary annoying shifting of pins on graph when cycling between pins
		wnd_rtl_graph->pin_select_by_idx( pin_idx, 1, 0 );
		i_need_pin_offscreen_check = 1;
		}
	
	}

printf( "cb_graph_left_click_release_cb() - gph_mouse_freq %d\n", (int)gph_mouse_freq );

imode = en_mode_stop;
//wnd_rtl_graph->freq_listen( 1, 0, 0, 0 );


/*

int mouse_dx = wnd_rtl_graph->gph0->mousex - mgraph_grabx;
int mouse_dy = wnd_rtl_graph->gph0->mousey - mgraph_graby;


if( fabsf( mouse_dx ) > 5 )
	{
	//wnd_rtl_graph->miwp_tune->miw->set_value_from_double( gph_mouse_freq );

	//imode = en_mode_stop;

	float drag_ratio = ( (float)mouse_dx / wnd_rtl_graph->gph0->w() );

	float freq_delta = g_dev_bw * drag_ratio;

	printf( "cb_graph_left_click_release_cb() mouse_dx %d %d   drag_ratio %f  bw %f  freq_delta %f\n", mouse_dx, mouse_dy, drag_ratio, g_dev_bw, freq_delta );

	freq_delta /= disp_spect_zoom_factor;

	wnd_rtl_graph->miwp_tune->miw->set_value_from_double( g_freq_tune - freq_delta );
	wnd_rtl_graph->freq_listen( 1, 0, 0, 0 );
	}


*/
//mgraph_grabx = wnd_rtl_graph->gph0->mousex;								//update grab vals 
//mgraph_graby = wnd_rtl_graph->gph0->mousey;


}










void cb_graph_middle_click_anywhere_cb( void *o )
{
string s1;

if( !wnd_rtl_graph ) return;


printf( "cb_graph_middle_click_anywhere_cb()\n" );

wnd_rtl_graph->miwp_tune->miw->set_value_from_double( gph_mouse_freq - g_freq_tune );

imode = en_mode_stop;
wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_graph_middle_click_anywhere_cb()" );

}







void cb_graph_right_click_anywhere_cb( void *o )
{
string s1;


mgraph_grabx = wnd_rtl_graph->gph0->mousex;								//update grab vals 
mgraph_graby = wnd_rtl_graph->gph0->mousey;



mgraph_grabx = wnd_rtl_graph->gph0->mousex;
mgraph_graby = wnd_rtl_graph->gph0->mousey;

mgraph_freq_tune = wnd_rtl_graph->miwp_tune->miw->get_value_as_double();
mgraph_freq_sub_tune = wnd_rtl_graph->miw_freq_sub_tune->get_value_as_double();

}






//bring all pins into the same 'freq_cntr' range by adj their 'vpin[x].freq_tune'
void rtl_graph_wnd::pin_conform( int freq_cntr )
{
int freq_actual;
int freq_tune;
int freq_sub_tune;
	
for( int i = 0; i < vpin.size(); i++ )
	{
	vpin[i].freq_sub_tune = vpin[i].freq_actual - freq_cntr;
	vpin[i].freq_tune = freq_cntr;
	}

}







void cb_graph_mousemove( void *o, int int0, int int1, double dble0, double dble1 )
{
double mx, my;



//wnd_rtl_graph->gph0->get_mouse_position_relative_to_trc( 0, mx, my );

int px, py;


wnd_rtl_graph->gph0->get_mouse_pixel_position_on_background( px, py );

if( wnd_rtl_graph->gph0->get_pixel_position_as_trc_values( 0, px, py, mx, my, 1, 1 ) )
	{
	}


//printf( "cb_graph_mousemove() gph_mouse_freq %f  px %d %d  mx %f %f\n", gph_mouse_freq, px, py, mx, my );



//printf( "cb_graph_left_click_release_cb() >>>>>>>>>>>>>>>>> int0 %d %d   dble0 %f  %f  \n", int0, int1, dble0, dble1 );


//return;


gph_mouse_freq = mx ;													//this includes the rtl onboard tuner freq as the graph x-axis is in those dimensions
gph_mouse_freq_in_bw = mx - g_freq_tune;								//conv this to a freq with the dev bandwidth

/*
int wid, hei;
wnd_rtl_graph->gph0->get_background_dimensions( wid, hei );

double ratio = (double)px / wid - 0.5;
gph_mouse_freq_in_bw =  g_dev_bw * ratio;
*/



//printf( "cb_graph_left_click_release_cb() >>>>>>>>>>>>>>>>> ratio %f    gph_mouse_freq_in_bw %f\n", ratio, gph_mouse_freq_in_bw );

//		printf( "cb_graph_mousemove() gph_mouse_freq %f\n", gph_mouse_freq );

if( wnd_rtl_graph->gph0->right_button )		//dragging, change onboard tuner freq ?
	{
	int mouse_dx = wnd_rtl_graph->gph0->mousex - mgraph_grabx;
	int mouse_dy = wnd_rtl_graph->gph0->mousey - mgraph_graby;


	if( fabsf( mouse_dx ) > 1 )
		{
		//wnd_rtl_graph->miwp_tune->miw->set_value_from_double( gph_mouse_freq );

		//imode = en_mode_stop;

		double drag_ratio = ( (double)mouse_dx / wnd_rtl_graph->gph0->w() );

		double freq_delta = g_dev_bw * drag_ratio;

//		printf( "cb_graph_left_click_release_cb() mouse_dx %d %d   drag_ratio %f  bw %f  freq_delta %f\n", mouse_dx, mouse_dy, drag_ratio, g_dev_bw, freq_delta );

		freq_delta /= disp_spect_zoom_factor;

//		g_freq_sub_tune += -freq_delta;




	//zero lower freq digits as req
	int drop_factor = 1;												//'1' here means don't drop any digits
	
	double frq = mgraph_freq_tune - freq_delta;

	
	if( ( pref_freq_right_click_drag_low_digit_zero_cnt > 0 ) )
		{
		
		if( pref_freq_right_click_drag_low_digit_zero_cnt == 4 ) drop_factor = 10000;
		if( pref_freq_right_click_drag_low_digit_zero_cnt == 3 ) drop_factor = 1000;
		if( pref_freq_right_click_drag_low_digit_zero_cnt == 2 ) drop_factor = 100;
		if( pref_freq_right_click_drag_low_digit_zero_cnt == 1 ) drop_factor = 10;
		
		if( drop_factor > 1 )
			{
			int i0 = frq / drop_factor;									//drop digits
			
			i0 *= drop_factor;
			
			frq = i0;
			}
		}

	
		freq_delta = mgraph_freq_tune - frq;
	
		wnd_rtl_graph->miwp_tune->miw->set_value_from_double( frq );

		wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( mgraph_freq_sub_tune + freq_delta );

		wnd_rtl_graph->freq_listen( 0, 0, 0, 0, "cb_graph_mousemove() 0" );
		}


//	printf( "cb_graph_mousemove() - YYYYYYYYYYYyyyyyou are dragging\n" );
	}



if( wnd_rtl_graph->gph0->left_button )		//dragging, change sub tuner freq ?
	{
	int mouse_dx = wnd_rtl_graph->gph0->mousex - mgraph_grabx;
	int mouse_dy = wnd_rtl_graph->gph0->mousey - mgraph_graby;


	if( fabsf( mouse_dx ) > 1 )
		{
		//wnd_rtl_graph->miwp_tune->miw->set_value_from_double( gph_mouse_freq );

		//imode = en_mode_stop;

		float drag_ratio = ( (float)mouse_dx / wnd_rtl_graph->gph0->w() );

		float freq_delta = g_dev_bw * drag_ratio;

//		printf( "cb_graph_left_click_release_cb() mouse_dx %d %d   drag_ratio %f  bw %f  freq_delta %f\n", mouse_dx, mouse_dy, drag_ratio, g_dev_bw, freq_delta );

		freq_delta /= disp_spect_zoom_factor;

//		g_freq_sub_tune += -freq_delta;




		//zero lower freq digits as req
		int drop_factor = 1;												//'1' here means don't drop any digits
		
		double frq = mgraph_freq_sub_tune - freq_delta;

		
		if( ( pref_freq_left_click_drag_low_digit_zero_cnt > 0 ) )
			{
			
			if( pref_freq_left_click_drag_low_digit_zero_cnt == 4 ) drop_factor = 10000;
			if( pref_freq_left_click_drag_low_digit_zero_cnt == 3 ) drop_factor = 1000;
			if( pref_freq_left_click_drag_low_digit_zero_cnt == 2 ) drop_factor = 100;
			if( pref_freq_left_click_drag_low_digit_zero_cnt == 1 ) drop_factor = 10;
			
			if( drop_factor > 1 )
				{
				int i0 = frq / drop_factor;									//drop digits
				
				i0 *= drop_factor;
				
				frq = i0;
				}
			}

	
		freq_delta = mgraph_freq_sub_tune - frq;
	
		wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( mgraph_freq_sub_tune + freq_delta );

		wnd_rtl_graph->freq_listen( 0, 0, 0, 0, "cb_graph_mousemove() 1" );
		}


//	printf( "cb_graph_mousemove() - YYYYYYYYYYYyyyyyou are dragging\n" );
	}


if( ( !wnd_rtl_graph->gph0->left_button ) && ( !wnd_rtl_graph->gph0->right_button ) )
	{
	mgraph_grabx = wnd_rtl_graph->gph0->mousex;
	mgraph_graby = wnd_rtl_graph->gph0->mousey;
	}


//printf( "cb_mousemove_graph() - mx %f %f\n", mx, my );
}








bool gph0_ctrl_key = 0;

void cb_graph_keydown( void *w, int key )
{
string s1;
double mx, my;

mgraph* og = wnd_rtl_graph->gph0; 

if( ( key == FL_Control_L ) || ( key == FL_Control_R ) ) gph0_ctrl_key = 1;


if( key == ' ' )
	{
	wnd_rtl_graph->do_common_key_processing( og->control_key, og->shift_key, key, 0 );
	}


if( key == FL_KP + '-' )
	{
	wnd_rtl_graph->do_common_key_processing( og->control_key, og->shift_key, key, 0 );
	}


if( key == FL_KP + '+' )
	{
	wnd_rtl_graph->do_common_key_processing( og->control_key, og->shift_key, key, 0 );
	}
	

if( key == FL_KP + '/' )
	{
	wnd_rtl_graph->do_common_key_processing( og->control_key, og->shift_key, key, 0 );
	}


if( key == FL_KP + '*' )
	{
	wnd_rtl_graph->do_common_key_processing( og->control_key, og->shift_key, key, 0 );
	}


if( key == FL_Home )
	{
	wnd_rtl_graph->do_common_key_processing( og->control_key, og->shift_key, key, 0 );

	wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_graph_keydown() - home" );
	}


if( key == FL_Delete )
	{
	wnd_rtl_graph->do_common_key_processing( og->control_key, og->shift_key, key, 0 );
	}


if( key == FL_Left )
	{
	wnd_rtl_graph->pin_conform( g_freq_tune );						//helps avoid unecessary annoying shifting of pins on graph when cycling between pins
	
	int idx = wnd_rtl_graph->pin_select_inc_dec( -1, 1 );
	i_need_pin_offscreen_check = 1;

/*
	double freq_left, freq_right;
	wnd_rtl_graph->gph0_edge_freqs( 0, 0, freq_left, freq_right );

	
	int fr_a = vpin[idx].freq_actual;
	int fr_obt = vpin[idx].freq_tune;
	int fr_sub = vpin[idx].freq_sub_tune;

	int delta = freq_right - freq_left;
	
	delta *= 0.015f;													//come inwards from edges
	if( ( fr_a < ( freq_left + delta ) ) || ( fr_a > ( freq_right - delta ) ) )
		{
		wnd_rtl_graph->centre_graph( 1, -1 );
		}
*/
	}




if( key == FL_Right )
	{
	wnd_rtl_graph->pin_conform( g_freq_tune );						//helps avoid unecessary annoying shifting of pins on graph when cycling between pins

	int idx = wnd_rtl_graph->pin_select_inc_dec( 1, 1 );
	i_need_pin_offscreen_check = 1;

/*
	double freq_left, freq_right;
	wnd_rtl_graph->gph0_edge_freqs( 0, 0, freq_left, freq_right );


	int fr_a = vpin[idx].freq_actual;
	int fr_obt = vpin[idx].freq_tune;
	int fr_sub = vpin[idx].freq_sub_tune;

	
	int delta = freq_right - freq_left;

	
	delta *= 0.015f;													//come inwards from edges

printf( "cb_graph_keydown() - NNNNNNNNNNNNNNNNNN delta %d freq_left %f %f   freq_actual %d\n", delta, freq_left + delta, freq_right - delta, fr_a );
	if( ( fr_a < ( freq_left + delta ) ) || ( fr_a > ( freq_right - delta ) ) )
		{
printf( "cb_graph_keydown() - NNNNNNNNNNNNNNNNNN centered\n" );
		wnd_rtl_graph->centre_graph( 1, -1 );
		}
	else{
//		int frq_obt = wnd_rtl_graph->miwp_tune->miw->get_value_as_double();
//		int frq_sub = wnd_rtl_graph->miw_freq_sub_tune->get_value_as_double();
//		int frq_actual = frq_obt + frq_sub;
		
//		int fr_new_sub = frq_actual - fr_a;
		
//		wnd_rtl_graph->miwp_tune->miw->set_value_from_double( frq_obt );
//		wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( fr_new_sub );
		}
*/
	}





//printf( "cb_graph_keydown() - key %d %x\n", key, key );




//-------- freq keyin while mouse is in gph0 ---------
bool bchanged = 0;
bool benter = 0;
bool btune = 1;

s1 = wnd_rtl_graph->miwp_tune->miw->value();
//skeyin = wnd_rtl_graph->miwp_tune->miw->value();


if( ( key == FL_Enter ) || ( key == FL_KP_Enter ) )
	{
	float dt = m1_skeyin_freq_changed1.time_passed( m1_skeyin_freq_changed1.ns_tim_start );
	m1_skeyin_freq_changed1.time_start( m1_skeyin_freq_changed1.ns_tim_start );			//start timer to show freq text on graph

	if( dt < 1 )														//two 'enter' keys in succession ?
		{
		s1 = "";														//clear keyin
		wnd_rtl_graph->miwp_tune->miw->value_text_only( s1.c_str() );

		btune = 0;
		}

	benter = 1;
	bchanged = 1;
	}


if( ( key >= (FL_KP + '0') ) && ( key <= (FL_KP + '9')  ) || ( key == (FL_KP + '.') ) )
	{
	key -= FL_KP;
	}


if( ( key >= '0' ) && ( key <= '9' ) || ( key == '.' ) || ( key == 'E' ) || ( key == 'e' ) )
	{
	s1 += key;
	wnd_rtl_graph->miwp_tune->miw->value_text_only( s1.c_str() );
	bchanged = 1;
	}


if( key == FL_BackSpace )
	{
	int len = s1.length();

	if( len > 0 )
		{
		s1 = s1.substr( 0, len - 1 );
		wnd_rtl_graph->miwp_tune->miw->value_text_only( s1.c_str() );
		bchanged = 1;
		}
	}



if( ( key == 'k' ) || ( key == 'm' ) || ( key == 'g' ) )
	{
	double d0;
	
	sscanf( s1.c_str(), "%lf", &d0 );


	if( key == 'k' )
		{
		d0 *= 1e3;
		wnd_rtl_graph->miwp_tune->miw->set_value_from_double( d0 );

		bchanged = 1;
		benter = 1;
		}


	if( key == 'm' )
		{
		d0 *= 1e6;
		wnd_rtl_graph->miwp_tune->miw->set_value_from_double( d0 );
		
		bchanged = 1;
		benter = 1;
		}


	if( key == 'g' )
		{
		d0 *= 1e9;
		wnd_rtl_graph->miwp_tune->miw->set_value_from_double( d0 );

		bchanged = 1;
		benter = 1;
		}
	}

if( bchanged )
	{
	wnd_rtl_graph->skeyin_freq = s1;
	i_skeyin_restore_cnt = 50;		//start a one shot timer to allow restore of current on-board tuner freq (into gui ctrl) if no number is entered

	if(benter) 
		{
		if( btune ) 
			{
			wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( 0.0 );

			if(  wnd_rtl_graph->skeyin_freq.length() > 0  ) wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_graph_keydown() - skeyin_freq" );
			wnd_rtl_graph->centre_graph( 1, -1 );

			s1 = "";
			}
		
		}
	
	m1_skeyin_freq_changed.time_start( m1_skeyin_freq_changed.ns_tim_start );			//start timer to show freq text on graph
	}

//-------------------


//wnd_rtl_graph->miwp_tune->miw->keycode_last = key;
//cb_miwp_tune_keydown( wnd_rtl_graph->miwp_tune->miw, (void*)0 );

}







void cb_graph_keyup( void *o, int key )
{
string s1;
double mx, my;

if( ( key == FL_Control_L ) || ( key == FL_Control_R ) ) gph0_ctrl_key = 0;
}









void cb_graph_mousewheel( void *o, int dir )
{
double mx, my;



//int dir = wnd_rtl_graph->gph0->mouse_dir;

if( !wnd_rtl_graph->gph0->inside_control ) return;


bool bdone = 0;
if( wnd_rtl_graph->gph0->shift_key )									//change graph y scaling?
	{
	float multiplier = 1.0625;

	if( dir < 0 ) multiplier = 1.0f/multiplier;

	gph_scaley *= multiplier;

	wnd_rtl_graph->miw_gph_scaley->set_value_from_double( gph_scaley );


	bdone = 1;
	}


if( wnd_rtl_graph->gph0->control_key )									//change graph zoom/scaling?
	{
	float multiplier = 1.5;
//	if( gph0_ctrl_key ) multiplier = 1.03125;

	if( dir < 0 ) multiplier = 1.0f/multiplier;

	disp_spect_zoom_factor = disp_spect_zoom_factor * multiplier;

	if( disp_spect_zoom_factor < 1 ) disp_spect_zoom_factor = 1;

	//gph_mouse_freq = mx;

	wnd_rtl_graph->miw_gph_disp_spect_zoom_factor->set_value_from_double( disp_spect_zoom_factor );
//	printf( "cb_graph_mousewheel() - disp_spect_zoom_factor %f\n", disp_spect_zoom_factor );

	bdone = 1;
	}


if( ! bdone )
	{
//	printf( "cb_graph_mousewheel() - control_key %d\n", wnd_rtl_graph->gph0->control_key );
//	double frq = wnd_rtl_graph->miwp_tune->miw->get_value_as_double();
	double frq = wnd_rtl_graph->miw_freq_sub_tune->get_value_as_double();

	//zero lower freq digits as req
	int drop_factor = 1;												//'1' here means don't drop any digits


	int band = 3;														//assume near very bottom fraction of graph, mouse tweek hzoom ?


	if( ( wnd_rtl_graph->gph0->mousey < wnd_rtl_graph->gph0->h() * cn_graph_mousewheel_vert_region_verylow ) )	//near lower 3rd of graph, mouse tweek of sub freq at medium level ? ?
		{
		band = 2;
		}


	if( ( wnd_rtl_graph->gph0->mousey < wnd_rtl_graph->gph0->h() * cn_graph_mousewheel_vert_region_middle ) )	//near middle of graph, tweek freq at course level ?
		{
		band = 1;
		}

	
	
	if( wnd_rtl_graph->gph0->mousey < wnd_rtl_graph->gph0->h() * cn_graph_mousewheel_vert_region_high )	//near top, mouse tweek sub freq of at fine level ?
		{
		band = 0;
		}


printf( "cb_graph_mousewheel() - KKKKKKKKKKKKKKKK drop_factor %d   band %d\n", drop_factor, band );

	if( band != 3 )														//in a band that affects tuning of sub freq ?
		{
		if( ( pref_freq_mousewheel_low_digit_zero_cnt > 0 ) && ( band != 0 ) )
			{
			if( pref_freq_mousewheel_low_digit_zero_cnt == 4 ) drop_factor = 10000;
			if( pref_freq_mousewheel_low_digit_zero_cnt == 3 ) drop_factor = 1000;
			if( pref_freq_mousewheel_low_digit_zero_cnt == 2 ) drop_factor = 100;
			if( pref_freq_mousewheel_low_digit_zero_cnt == 1 ) drop_factor = 10;
			
			if( drop_factor > 1 )
				{
	//			int scaler = 1.0;
				
				if( band == 1 ) drop_factor *= 10;						//courser adj
				
				int i0 = frq / drop_factor;								//drop digits
				
				i0 *= drop_factor;
				
				frq = i0;
				}
			}


		int step = drop_factor;
//		bool b_changed = 0;

/*
		step = drop_factor;
			b_changed = 1;		

		if( band == 0 )														//near top of graph, mouse tweek freq at course level ?
			{
			step = drop_factor;
			b_changed = 1;		
			}

		if( ( band == 1 ) && ( !b_changed ) ) 								//near middle of graph, mouse tweek freq at medium level ?
			{
			step = drop_factor;
			b_changed = 1;
			}

		if( ( !b_changed ) )
			{
			step = drop_factor; 											//near bottom of graph, mouse tweek freq at fine level ?
			b_changed = 1;
			}
*/
//		printf("cb_graph_mousewheel() - frq %f  step %d\n", frq, step*dir ); 
		frq += step*dir;
		printf("cb_graph_mousewheel() - frq %f  step %d\n", frq, step*dir ); 

	//	wnd_rtl_graph->miwp_tune->miw->set_value_from_double(frq );
		wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( frq );
		wnd_rtl_graph->freq_listen( 0, 0, 0, 0, "cb_graph_mousewheel()" );
		}
	else{																//if here in a band that affects h zoom ?
		float multiplier = 1.125;
		if( gph0_ctrl_key ) multiplier = 1.03125;

		if( dir < 0 ) multiplier = 1.0f/multiplier;

		disp_spect_zoom_factor = disp_spect_zoom_factor * multiplier;

		gph_hzoom_mousex = wnd_rtl_graph->gph0->mousex;

		if( disp_spect_zoom_factor < 1 ) disp_spect_zoom_factor = 1;

		//gph_mouse_freq = mx;

		wnd_rtl_graph->miw_gph_disp_spect_zoom_factor->set_value_from_double( disp_spect_zoom_factor );
		}
	}

}





void aud_dimmer()
{
mystr m1;

bool bdelay = 0;
if( aud_dim_time == 0 ) bdelay = 1;

aud_dim_time = cn_aud_dim_time_default;									//dim audio momentarily

if( bdelay ) m1.delay_ms( 100 );										//allow time for audio thread to dim audio, if dim is not already underway	

}







void demodul_mode_set( unsigned int which )
{
aud_dimmer();															//dim audio momentarily


wnd_rtl_graph->demodul_mode = which;
wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "demodul_mode_set()" );

wnd_rtl_graph->fi_demodul_mode->value( which );

printf( "demodul_mode_set() -  %d\n", wnd_rtl_graph->demodul_mode );

}









void cb_demodul_mode( Fl_Widget *w, void * )
{
if( !wnd_rtl_graph ) return;



Fl_Choice *o = (Fl_Choice*) w;

demodul_mode_set( o->value() );

//wnd_rtl_graph->demodul_mode = o->value();
//wnd_rtl_graph->freq_listen( 1, 0, 0, 0 );

//cb_bt_freq_tune( 0, 0 );
//printf( "cb: %d\n", wnd_rtl_graph->demodul_mode );

//tune_rtl();
}











void cb_mb_dev_bwidth( Fl_Widget *w, void * )
{
if( !wnd_rtl_graph ) return;

Fl_Menu_Button *o = (Fl_Menu_Button*) w;

aud_dimmer();

int idx = o->value();

printf( "\n\ncb_mb_dev_bwidth() - idx %d,  %f Hz\n", idx, wnd_rtl_graph->st_dev_bwidth[idx].freq );
wnd_rtl_graph->set_dev_bwidth( wnd_rtl_graph->st_dev_bwidth[idx].freq );


unsigned int dwn_srate_in = downsample_srate;
bool b_set_dwnsrate = 1;
bool b_adj_gui_ctrl = 1;
dnwsrate_nearest_factor( dwn_srate_in, b_set_dwnsrate, b_adj_gui_ctrl );

//b_need_filter_rebuild = 1;
//bneed_hzoom_and_center_to_cur_freq = 5;
//nhac_hzoom_factor = 1.0f;
//b_need_h_zoom_recalc = 1;

wnd_rtl_graph->centre_graph( 5, 1 );

}










void multiply(double ar, double aj, double br, double bj, double *cr, double *cj )
{
	*cr = ar*br - aj*bj;
	*cj = aj*br + ar*bj;
}




double polar_discriminant( double ar, double aj, double br, double bj )
{
	double cr, cj;
	double angle;
	multiply( ar, aj, br, -bj, &cr, &cj);
	angle = atan2( cj, cr );
	return angle / 3.14159;
}




double last_real = 0;
double last_imag = 0;
double last_pcm = 0;
vector<double>vpcm;
vector<float> vpcm_ch0;
vector<float> vpcm_ch1;

vector <st_spect_tag> vsp;
vector <float> vaudio;
vector <float> vaudio_ch0;
vector <float> vaudio_ch1;












void demod_tone1( vector<filter_code::st_cplex_tag> vin, vector <double> &vpcm )
{
for( int i = 0; i < vin.size(); i++ ) 
	{
	vpcm.push_back( 0 );
	}

}








void demod_wfm( vector<filter_code::st_cplex_float_tag> vin, vector <float> &vpcm, float output_scale )
{
filter_code::st_cplex_float_tag oc;

vpcm.clear();

//last_real = 0;
//last_imag = 0;

for( int i = 0; i < vin.size(); i++ ) 
	{
	oc = vin[ i ];

	double pcm;

	pcm = polar_discriminant( oc.real, oc.imag, last_real, last_imag );

//	st_spect_tag o;
//	o.freq = i;
//	o.ampl = pcm;
//vsp.push_back( o );

	vpcm.push_back( pcm * output_scale );

	last_real = oc.real;
	last_imag = oc.imag;
	last_pcm = pcm;
//	printf("pcm= %lf\n", pcm );
	}

//printf("demod_fm() - vin.size()= %d\n", vin.size() );

}


















void demod_fm( vector<filter_code::st_cplex_float_tag> vin, vector <float> &vpcm, float output_scale )
{
filter_code::st_cplex_float_tag oc;

vpcm.clear();

//last_real = 0;
//last_imag = 0;

for( int i = 0; i < vin.size(); i++ ) 
	{
	oc = vin[ i ];

	double pcm;

	pcm = polar_discriminant( oc.real, oc.imag, last_real, last_imag );

//	st_spect_tag o;
//	o.freq = i;
//	o.ampl = pcm;
//vsp.push_back( o );

	vpcm.push_back( pcm * output_scale );

	last_real = oc.real;
	last_imag = oc.imag;
	last_pcm = pcm;
//	printf("pcm= %lf\n", pcm );
	}

//printf("demod_fm() - vin.size()= %d\n", vin.size() );

}










void demod_am( vector<filter_code::st_cplex_tag> vin, vector <double> &vpcm, float output_scale )
{
filter_code::st_cplex_tag oc;

vpcm.clear();

//last_real = 0;
//last_imag = 0;


//vspect.clear();

for( int i = 0; i < vin.size(); i++ ) 
	{
	oc = vin[ i ];

	double pcm;
	double scale = 1.0;
	pcm = ( oc.real / scale ) * ( oc.real / scale ) ;
	pcm += ( oc.imag / scale ) * ( oc.imag / scale );

	pcm = sqrt( pcm ) * output_scale;

//	if( std::isnan(pcm) )
//		{
//		pcm = 0.0f;
//		}


//	st_spect_tag o;
//	o.freq = i;
//	o.ampl = pcm;
//vspect.push_back( o );

	vpcm.push_back( pcm * output_scale );
	}

}








void demod_am_float( vector<filter_code::st_cplex_float_tag> vin, vector <float> &vpcm, float output_scale )
{
filter_code::st_cplex_float_tag oc;

vpcm.clear();


for( int i = 0; i < vin.size(); i++ ) 
	{
	oc = vin[ i ];

	float pcm;
	float scale = 1.0;
	pcm = ( oc.real ) * ( oc.real ) ;
	pcm += ( oc.imag ) * ( oc.imag );

	pcm = sqrtf( pcm ) * output_scale;

//	if( std::isnan(pcm) )
//		{
//		pcm = 0.0f;
//		}


//	st_spect_tag o;
//	o.freq = i;
//	o.ampl = pcm;
//vspect.push_back( o );

	vpcm.push_back( pcm * output_scale );
	}

}








void demod_lsb( vector<filter_code::st_cplex_float_tag> vin, vector <float> &vpcm, float output_scale )
{
filter_code::st_cplex_float_tag oc;

vpcm.clear();


//filter1( vin,  vfilt );

double min = 0;
double max = 0;


//vector <st_cplex_tag> vsp;
//complex_fwd_fft( vin, vsp );

int full = vin.size();
//int half = vin.size() / 2;


//for( int i = 0; i < vsp.size(); i++ )
//	{
//	if( ( i >= ( 0 )  ) && ( i < ( 1000 ) ) )		//positive freq
//		{
//		vsp[ i ].real = 0;
//		vsp[ i ].imag = 0;
//		}


//	if( ( i >= ( full - 1000 )  ) && ( i < ( full ) ) )		//neg freq
//		{
//		vsp[ i ].real = 0;
//		vsp[ i ].imag = 0;
//		}
//	}

//complex_fwd_displayable( vsp, vspect );

//vector<st_cplex_tag> vinxx;
//complex_rev_fft( vsp, vin );



for( int i = 0; i < vin.size(); i++ ) 
	{
	oc = vin[ i ];

	float pcm;

	float scale = 1.00f;						//needed this to make signal rise out of the noise
	
//	if( oc.real > max ) max = oc.real;
//	if( oc.real < min ) min = oc.real;
	
	
	pcm = oc.real - oc.imag;

	vpcm.push_back( pcm * output_scale );

//	st_spect_tag o;
//	o.freq = i;
//	o.ampl = pcm;

//	vpcm.push_back( pcm * 1.0 );
	}

//printf( "demod_lsb: min= %lf, max= %lf\n", min, max );

}















void demod_usb( vector<filter_code::st_cplex_float_tag> vin, vector <float> &vpcm, float output_scale )
{
filter_code::st_cplex_float_tag oc;

vpcm.clear();


//vector <st_cplex_tag> vsp;
//complex_fwd_fft( vin, vsp );
//complex_fwd_displayable( vsp, vspect );


for( int i = 0; i < vin.size(); i++ ) 
	{
	oc = vin[ i ];

	float pcm;

	float scale = 1.00f;					//needed this to make signal rise out of the noise
	
	pcm = oc.real / scale + oc.imag / scale;

	vpcm.push_back( pcm * output_scale );
	}

}











int prev_lpr_index = 0;
double now_lpr = 0;

//LOW quality audio downsampler, creates high freq noise due to sample skipping - see also 'low_pass_real_interpolator()'
//samplerate down converter with filtering
//does not clear 'vout'
void low_pass_real( vector <double> &vin,  vector <double> &vout, int src_rate, int dest_rate, float scale )
/* simple square window FIR */
// add support for upsampling?
{
	int i = 0;
	int fast = src_rate;
	int slow = dest_rate;
//see gc fprintf
//fprintf( stderr, "low_pass_real() - fast= %d, slow = %u\n", fast, slow );
//low_pass_real() - fast= 200000, slow = 48000, s->result_len= 21845



while( i < vin.size() ) 
	{
	now_lpr += vin[ i ];
	i++;
	prev_lpr_index += slow;

	if ( prev_lpr_index < fast) continue;

	double val = now_lpr / ( fast / slow );

	vout.push_back( val * scale );


//	st_spect_tag o;
//	o.freq = i;
//	o.ampl = val;
//vsp.push_back( o );

	prev_lpr_index -= fast;
	
	now_lpr = 0;
	}


//see gc fprintf
//fprintf( stderr, "low_pass_real() - s->result_len= %u\n", s->result_len );
//low_pass_real() - s->result_len= 5243
}















bool force_dbg0 = 0;

float lpf_x0 = 0;		//persistent across 'low_pass_real_interpolator()' calls

//takes vector of real values and downsamples to 'dest_rate', uses persistent fractional stepping value 'lpf_x'
//does not clear 'vout'
void low_pass_real_interpolator( vector <double> &vin,  vector <double> &vout, int src_rate, int dest_rate, int dest_cnt, float scale, string scaller )
{
bool vb0 = 0;

vout.clear();
//return;
//if( force_dbg0 == 1 ) return;


//dest_cnt = 2048;

//if( lpf_x0 >= dest_cnt ) lpf_x0 -= dest_cnt;
//if( lpf_x0 >= 4096 ) lpf_x0 = 0;

float iptr;
lpf_x0 = modff( lpf_x0, &iptr );


//	farrow_resample_double_vector( 2048, vin, vout );

//if( lpf_x0 != 0.0f )
//	{
//printf("low_pass_real_interpolator() - was not ZERO lpf_x0 %f\n", lpf_x0 );
//	getchar();
//	}
//float ratio = (float)src_rate/dest_cnt;

double *ptr = vin.data();
unsigned int bufsz = vin.size();

float step = (float)bufsz / dest_cnt;

if(vb0)printf("low_pass_real_interpolator() - start   src_rate %d  dest_rate %d  step %f  dest_cnt %d  lpf_x0 = %f\n", src_rate, dest_rate, dest_cnt, step, lpf_x0 );

int in_siz = vin.size();

if(vb0)printf("low_pass_real_interpolator() - bufsz %d\n", bufsz );

for( int i = 0; i < dest_cnt; i++ )
	{
//	if( i >= in_limit ) break;
	
	bool end_reached;
	double dd = gc_srateconv_code::farrow_resample_double( lpf_x0, ptr, bufsz, end_reached );

//if(i==0)printf("low_pass_real_interpolator() - end lpf_x0 = %f   vout[%d] %f\n", lpf_x0, i, dd );
//if(i==1)printf("low_pass_real_interpolator() - end lpf_x0 = %f   vout[%d] %f\n", lpf_x0, i, dd );



	vout.push_back( dd * scale );
//	if( dd != vin[ii] )
//	if( ( lpf_x0 >= 10.0f ) && (lpf_x0 <= 500.0f ) )
		{
//		force_dbg0 = 1;
//		return;
//		int ii = nearbyint(lpf_x0);
//if(vb0)printf("low_pass_real_interpolator() - lpf_x0 = %f    vin[%d] %f  vinterp %f end_reached %d\n", lpf_x0, ii, vin[ ii ], dd, end_reached );
//getchar();
		}
	

//	dd = vin[ii];
//	vout.push_back( dd * scale );

//	if( dd != vin[ii] )
//		{

//		}
	

	lpf_x0 += step;
	}		
//getchar();

if(vb0)printf("low_pass_real_interpolator() - end lpf_x0 = %f   vout[] %f %f\n", lpf_x0, vout[0], vout[1] );

}






/*

double grab0 = 0;
int dbg_cnt3 = 0;


namespace gc_srateconv_code
{


//see 'qdss_resample()' for a desciption of what is code does, BUT additionaly, 
//this version is 'frame buffer' friendly, allowing a stream of sample buffers to be supplied in succession and provides interpolation overlap across buffers,
//'x' is shifted backwards by 'wnwdth/2' so filter kernel convolution does not go past end of supplied 'buf',
//when 'x' is near the end of 'buf' it also copies some tailing samples to 'head_bf' for use when function is next called with 'x' near zero, this allows filter kernel convolution
//to access samples before 'buf[0]' (using the previous calls tailing samples),
//as mentioned this will happen when 'x' is near zero (ie. when the left side of the Hann windowed sinc impulse response sits over samples from previous call),
//also, to avoid forced zero samples don't allow 'x' to exceed 'bufsz', ie. don't allow filter kernel convolution to reach passed 'buf[bufsz-1]'
//'head_bf' must be big enough to hold 'wnwdth' samples
//NOTE the initial call of this function may use 'head_bf' so init its contents to zero, interpolation of the first few samples may not be accurate on this first call only
double qdss_resample_double_continuous( double xin, double *buf, unsigned int bufsz, double fmax, double fsrate, int wnwdth, double *head_bf, int &user_flag )
{
double r_g,r_w,r_a,r_snc,r_y;												//some local variables
int i, j;

if( wnwdth <= 0 ) return 0;
int half_wnd = wnwdth / 2;
if( bufsz < wnwdth ) return 0;

double x = xin - half_wnd;													//force a shift of 'x' position into so filter kernel convolution does not go past 'bufsz', this will cause use														
																			//of 'head_bf' samples (from prev call) when needing samples around the beginning of 'buf', i.e. when spec 'x' is at or near zero

r_g = 2.0 * fmax / fsrate;													//calc gain correction factor


r_y = 0;
for ( i = -( half_wnd ); i <= ( half_wnd - 1 ); i++ ) 						//for 1 window width
	{
    j = (int) x + i;          												//calc input sample index

    r_w = 0.5 - 0.5 * cos( twopi * ( 0.5 + ( j - x ) / wnwdth ) );			//make a Hann sample, will be used taper sinc's impulse response length

    r_a = twopi * ( j - x ) * fmax / fsrate;								//cur sinc location
    r_snc = 1;
	if ( r_a != 0 ) r_snc = sin( r_a ) / r_a;								//make a sinc (sin x/x) lpf sample

//printf("i: %d, r_w: %f, r_a: %f, r_snc: %f\n", i, r_w, r_a, r_snc );

	bool bneg_idx = 0;
	if( j < 0 ) bneg_idx = 1;												//need samples from previous call ?

	bool bforce_zero = 0;
	if( j >= ( ( (int)bufsz) ) ) bforce_zero = 1;	

//if( j<0 ) printf("qdss_resample_double_continuous() - x %f  i %d  j %d \n", x, i, j );

	int idx;

	if ( !bforce_zero )														//src sample avail?
		{
		if( bneg_idx )														//src sample pos in previous calls buf?
			{
			idx = wnwdth + j;											//j is negative here 
			r_y = r_y + r_g * r_w * r_snc * head_bf[ idx ];				//convolve/mac: first: apply hann tapering (window) to sinc impulse response, then both these to a src sample (adj gain as well)
//printf("qdss_resample_double_continuous() - xin %f  x %f   smpl is at %d,  getting smpl from head_bf[%d]\n", xin, x, j, idx );

//			if ( xin < 10 )   printf("qdss_resample_double_continuous()0 -  j %d  idx %d  xin %f  x %f r_y %f\n", j, idx, xin, x, r_y );
//			if ( xin > 4080 ) printf("qdss_resample_double_continuous()0 -  j %d  idx %d  xin %f  x %f r_y %f\n", j, idx, xin, x, r_y );
			}
		else{
			r_y = r_y + r_g * r_w * r_snc * buf[ j ];						//convolve
			}
		}
	else{
printf("qdss_resample_double_continuous() - forcing zero as  xin %f x %f bufsz %d  i %d j is %d\n", xin, x, bufsz, i, j );
		}	



	//if( ( dbg_cnt3 == 0 ) || ( dbg_cnt3 == 1 ) || ( dbg_cnt3 == 2 ) || ( dbg_cnt3 == 3 ) || ( dbg_cnt3 == 4 ) )
//	if( ( dbg_cnt3 <= 15 )  )
		{
	//	if( bneg_idx ) printf("qdss_resample_double_continuous()neg -  cnt %d  i %d  j %d  xin %f  x %f  idx %d  hbf[%f]  r_y %f\n", dbg_cnt3, i, j, xin, x, idx, head_bf[idx], r_y );
	//	else printf("qdss_resample_double_continuous()pos -  cnt %d  i %d  j %d  xin %f  x %f  idx %d  buf[%f]  r_y %f\n", dbg_cnt3, i, j, xin, x, idx, buf[idx], r_y );
	//	r_y = -1;
		}


 	}


//dbg_cnt3++;

//if ( xin < 10 )   printf("qdss_resample_double_continuous() -  j %d  idx %d  xin %f  x %f r_y %f\n", j, idx, xin, x, r_y );
//if ( xin > 4080 ) printf("qdss_resample_double_continuous() -  j %d  idx %d  xin %f  x %f r_y %f\n", j, idx, xin, x, r_y );

//for ( int i = 0; i < half_wnd; i++ )
//	{
//	head_bf[i] = buf[ (bufsz - 1) - ( half_wnd - i ) ];					//copy tailing samples to 'head_bf[]' for next call use if any
//	}



bool bcopy_buf = 0;

if( (floorf(x) >= ( bufsz - 16*wnwdth ) ) ) bcopy_buf = 1;				//approching samples near end of 'buf' ?, copy ending samples to 'head_bf'


if( bcopy_buf )
	{
	for ( int i = 0; i < wnwdth; i++ )
		{
		int idx = (bufsz) - ( wnwdth - i );
		head_bf[i] = buf[ idx ];					//copy tailing samples to 'head_bf[]' for next call use if any

	//if( user_flag == 1 ) printf("qdss_resample_double_continuous() - head_bf[%d] = buf[%d] val %f\n", i, idx, head_bf[i] );
		}
	}

if( user_flag == 1 ) user_flag = 2;

return r_y;                  											//new filtered intermediate sample

}


}	//namespace gc_srateconv_code::

*/





#define cn_wndwdth 12													//value of 8 here when upsampling 8K-->48K causes audible aliasing
double head_bf[cn_wndwdth*2];

float head_bf0[cn_wndwdth*2];											//used for resampler inter frame continuity
float head_bf1[cn_wndwdth*2];


//int dbg_cnt0 = 0;




//double ovrlap_bf[32768];
//int ovrlap_cnt = 4;




//takes vector of real values and downsamples to 'dest_rate', uses persistent fractional stepping value 'lpf_x'
//does not clear 'vout'
void low_pass_real_sinc_float( vector <float> &vin,  vector <float> &vout, int src_rate, int dest_rate, int dest_cnt, float scale, string scaller, float *headbf )
{
bool vb0 = 0;

vout.clear();


float iptr;
lpf_x0 = modff( lpf_x0, &iptr );

float *ptr = vin.data();
unsigned int bufsz = vin.size();

float step = (float)bufsz / dest_cnt;


if(vb0)printf("low_pass_real_sinc_float() - bufsz %d\n", bufsz );

for( int i = 0; i < dest_cnt; i++ )
	{
//	if( i >= in_limit ) break;
	
	float fmax = src_rate/2.5;
	float f0 = gc_srateconv_code::qdss_resample_float_continuous( lpf_x0, ptr, bufsz, fmax, src_rate, cn_wndwdth, headbf );	//uses samples kept from prev call for better interpolation calcs



	vout.push_back( f0 * scale );	
	

	lpf_x0 += step;
	}


if(vb0)printf("low_pass_real_sinc() - end lpf_x0 = %f   vout[] %f %f\n", lpf_x0, vout[0], vout[1] );

}








//takes vector of real values and downsamples to 'dest_rate', uses persistent fractional stepping value 'lpf_x'
//does not clear 'vout'
void low_pass_real_sinc( vector <double> &vin,  vector <double> &vout, int src_rate, int dest_rate, int dest_cnt, float scale, string scaller )
{
bool vb0 = 0;

vout.clear();
//return;
//if( force_dbg0 == 1 ) return;






//dest_cnt = 2048;

//if( lpf_x0 >= dest_cnt ) lpf_x0 -= dest_cnt;
//if( lpf_x0 >= 4096 ) lpf_x0 = 0;

float iptr;
lpf_x0 = modff( lpf_x0, &iptr );


//	farrow_resample_double_vector( 2048, vin, vout );

//if( lpf_x0 != 0.0f )
//	{
//printf("low_pass_real_sinc() - was not ZERO lpf_x0 %f\n", lpf_x0 );
//	getchar();
//	}
//float ratio = (float)src_rate/dest_cnt;

double *ptr = vin.data();
unsigned int bufsz = vin.size();

float step = (float)bufsz / dest_cnt;


//vin[0]=100.0f;


//form a combined src buffer with overlap at head using previous calls ending bytes
//memcpy( ovrlap_bf + ovrlap_cnt, ptr, bufsz*sizeof(double) );

//ovrlap_bf[ 1 ] = 500.0f;

//lpf_x0 = ovrlap_cnt;


//if(1)printf("low_pass_real_sinc() - start   vin sz %d, src_rate %d  dest_rate %d  step %f  dest_cnt %d  lpf_x0 = %f\n", vin.size(), src_rate, dest_rate, dest_cnt, step, lpf_x0 );



if(vb0)printf("low_pass_real_sinc() - bufsz %d\n", bufsz );

for( int i = 0; i < dest_cnt; i++ )
	{
//	if( i >= in_limit ) break;
	
	float fmax = 3000;
//	double dd = gc_srateconv_code::qdss_resample( lpf_x0, ptr, bufsz, fmax, src_rate, cn_wndwdth );
	double dd = gc_srateconv_code::qdss_resample_double_continuous( lpf_x0, ptr, bufsz, fmax, src_rate, cn_wndwdth, head_bf );	//uses samples kept from prev call for better interpolation calcs


//if(i==0)printf("low_pass_real_sinc() -  step %f lpf_x0 = %f   vout[%d] %f\n", step, lpf_x0, i, dd );
//if(i==1)printf("low_pass_real_sinc() -  step %f lpf_x0 = %f   vout[%d] %f\n", step, lpf_x0, i, dd );
//if(i==2)printf("low_pass_real_sinc() -  step %f lpf_x0 = %f   vout[%d] %f\n", step, lpf_x0, i, dd );
//if(i==3)printf("low_pass_real_sinc() -  step %f lpf_x0 = %f   vout[%d] %f\n", step, lpf_x0, i, dd );



	vout.push_back( dd * scale );
//	if( dd != vin[ii] )
//	if( ( lpf_x0 >= 10.0f ) && (lpf_x0 <= 500.0f ) )
		{
//		force_dbg0 = 1;
//		return;
//		int ii = nearbyint(lpf_x0);
//if(vb0)printf("low_pass_real_sinc() - lpf_x0 = %f    vin[%d] %f  vinterp %f end_reached %d\n", lpf_x0, ii, vin[ ii ], dd, end_reached );
//getchar();
		}
	

//	dd = vin[ii];
//	vout.push_back( dd * scale );

//	if( dd != vin[ii] )
//		{

//		}
	

	lpf_x0 += step;
	}


//dbg_cnt0++;
//printf("low_pass_real_sinc() - vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvdbg_cnt0 %d\n", dbg_cnt0 );

//----make overlap by moving ending bytes of this buf to its head for next call

//for( int i = 0; i < ovrlap_cnt; i++ )
//	{
//	int ii = bufsz - ovrlap_cnt;
	
//	ovrlap_bf[i] = 0;//100*rnd();
//	}

//getchar();

if(vb0)printf("low_pass_real_sinc() - end lpf_x0 = %f   vout[] %f %f\n", lpf_x0, vout[0], vout[1] );

	
}
















//given a complex fftw ordered spectrum in vin, convert spectrum to magn, does not sort freqs or remove DC term
//useful when working out how fftw provides its spectrum
//clears vsp

//with a complex sample size of 256, fftw ordering after a fwd complex fft is:

//mapping example below:
//     negative_most          first_neg         dc      first_positive             most_positive
//128,     129.......->..........255            0            1.........->............ 127,         128		(128 is nyquist)

//looking at it another way, positives and matching negative locations
//dc term:   0
//positives: 1,   2,   3,    4 ....->.....125, 126, 127,   (127 is highest positive freq, execpt of nyquist)
//negatives: 255, 254, 253,  252...->.....131, 130, 129,   (129 is highest negative freq, execpt of nyquist)
//nyquist:   128, --- you can't make out if this is a pos or neg freq, as either appear in this loc, see 'fftw_complex_test_work_out()'


void complex_fft_displayable_fftw_order( vector<filter_code::st_cplex_tag> vsp_in, vector<st_spect_tag> &vsp )
{
st_spect_tag o;
filter_code::st_cplex_tag oc;


int isize = vsp_in.size();

if( isize <= 1 ) return;

vsp.clear();


//scale and store fft result in fftw order
for( int i = 0; i < isize; i++ )
	{
	double d1;
	double d2;

	d1 = (double)vsp_in[ i ].real;
	d2 = (double)vsp_in[ i ].imag;

	o.freq = i;
	o.ampl = sqrt( d1 * d1 + d2 * d2  );

//if( i == 0 )  o.ampl = -5;

	vsp.push_back( o );
	}


}











//with a complex sample size of 256, fftw ordering after a fwd complex fft is:

//mapping example below:
//negative_most      first_neg         dc      first_positive             most_positive   nyq
//129.......->..........255            0            1.........->............ 127,         128		(128 is nyquist)

//looking at it another way, positives and matching negative locations
//dc term:   0
//positives: 1,   2,   3,    4 ....->.....125, 126, 127,   (127 is highest positive freq, execpt of nyquist)
//negatives: 255, 254, 253,  252...->.....131, 130, 129,   (129 is highest negative freq, execpt of nyquist)
//nyquist:   128, --- you can't make out if this is a pos or neg freq, as either appear in this term, see 'fftw_complex_test_work_out()'



//fftw ordering when N=8
// 0    1    2    3    4    5    6    7		<--- fftw array index when N=8
// |----|----|----|----|----|----|----|
// DC  +1   +2   +3   Nyq  -3   -2   -1		<--- index 4 is the nyquist  +1 +2 +3 are the positive freqs
 
 
//'vsp' output ordering
//0    1    2    3    4    5    6    7		<--- 'vsp' array index when N=8
//|----|----|----|----|----|----|----|
//-3   -2   -1   DC   +1   +2   +3	Nyq		<--- index 7 is the nyquist  +1 +2 +3 are the positive freqs





//given a complex fftw ordered spectrum in vsp_in, convert spectrum to magn, sort freqs, so 
//negs are first then followed by positive freqs, i.e: centre is lowest freqs
//useful when graphing is required,
//clears vsp

//with a complex sample size of 256, fftw ordering after a fwd ftt is:

//     negative most          first_neg         dc      first_positive             most positive
//128,     129.......->..........255            0            1.........->............ 127,         128		(128 is nyquist)


//but this code will reorder indices to this:

//negative most        first_neg     dc       first_positive             most positive
//129 ........->..........255        0           1.........->............ 127, 128			(bin 128 nyquist freq, is placed in highest positive bin, i.e: right most)


//'vsp' will have below ordering (127 negative bins, dc, 127 positive bins, and lastly nyquist:
//negative most        first_neg     dc       first_positive             most positive
// 0 .....................126        127        128.......................254, 255			//<----------'vsp' gets this ordering (255 is nyquist)

//will zero DC if 'show_dc' is set	(the zero is still placed in 'vsp')
//if 'show_nyquist' is cleared, nyquist is not placed in 'vsp', so 'vsp' is one sample shorter than 'vsp_in'
//'freq_offs' is used to fill 'st_spect_tag.freq_actual', and is sum of spectra's freq within srate and 'freq_offs'
void complex_fft_displayable( int srate_in, int freq_offs, vector<filter_code::st_cplex_tag> vsp_in, vector<st_spect_tag> &vsp, bool show_dc, bool show_nyquist )
{
st_spect_tag o;
filter_code::st_cplex_tag oc;


int isize = vsp_in.size();

if( isize <= 1 ) return;

vsp.clear();


int half = isize / 2;

double bw_bin = (double)srate_in/isize;

unsigned int cull_lowest = 0;	//for debug only, if this is zero: only the DC term is zeroed (normal operation), if this is '1' or above, the lower freq spectra are zeroed, i.e. '2' would zero bin '1' and bin '2' the 1st and 2nd bins above the DC bin 

//negative spectrum, highest neg to lowest neg freq, bin: 129->255

int j = -half + 1;

for( int i = ( half + 1 ); i < isize - cull_lowest; i++ ) //span 127 bins
	{
	double d1;
	double d2;
	
	d1 = (double)vsp_in[ i ].real;
	d2 = (double)vsp_in[ i ].imag;
//printf(" neg = %03d\n", i );

	o.freq = j * bw_bin;
	o.freq_actual = o.freq + freq_offs;
	o.ampl = sqrt( d1 * d1 + d2 * d2  );

	vsp.push_back( o );
	j++;
	}


	int highest = half;													//e.g. 256 samples, this is 128 (the nyquist)
	//if( show_nyquist ) highest++;				//nyquist to be shown?

//positive spectrum, lowest pos to highest pos freq, bin: 0->128 (n.b: bin 128 is nyquist freq)
for( int i = 0 + cull_lowest; i <= highest; i++ )  //span 129 bins hence 'less than equal' used here
	{
	double d1;
	double d2;
	
	if( ( i == highest ) && ( !show_nyquist ) ) continue;
	
	d1 = (double)vsp_in[ i ].real;
	d2 = (double)vsp_in[ i ].imag;

	if( ( i == 0 ) && ( !show_dc ) )									//zero dc
		{
		d1 = 0;	
		d2 = 0;	
		}

	o.freq = i * bw_bin;
	o.freq_actual = o.freq + freq_offs;
	o.ampl = sqrt( d1 * d1 + d2 * d2  );

//if( i == 0 ) o.ampl = 0.7;


	vsp.push_back( o );
//	j++;
//printf(" pos = %03d\n", i );
	}

//sleep( 60 );

return;
}






















//given a vector of complex time domain samples (reverse fft), load vgraph with either real or imag parts
//clears vgraph
void displayable_complex_time_domain( vector<filter_code::st_cplex_tag> vtdm, bool imag, vector<st_spect_tag> &vgraph )
{
st_spect_tag o;


int isize = vtdm.size();

if( isize == 0 ) return;

vgraph.clear();


//scale and store fft result in fftw order
for( int i = 0; i < isize; i++ )
	{
	double d1;

	if( !imag ) d1 = (double)vtdm[ i ].real;
	else d1 = (double)vtdm[ i ].imag;

	o.freq = i;
	o.ampl = d1;


	vgraph.push_back( o );
	}


}















//given a vector of real time domain samples (reverse fft), load vgraph
//clears vgraph
void displayable_real_time_domain( vector<double> vtdm, vector<st_spect_tag> &vgraph )
{
st_spect_tag o;


int isize = vtdm.size();

if( isize == 0 ) return;

vgraph.clear();


//scale and store fft result in fftw order
for( int i = 0; i < isize; i++ )
	{
	o.freq = i;
	o.ampl = vtdm[ i ];

	vgraph.push_back( o );
	}


}














//forward complex fft, clears vsp
//spectrum is supplied in fftw ordering
//scales result down to remove fftw 'gain'
void complex_fwd_fft( vector<filter_code::st_cplex_tag> vin, vector<filter_code::st_cplex_tag> &vsp, unsigned int fftwsize )
{
st_spect_tag o;
filter_code::st_cplex_tag oc;

vsp.clear();

if( vin.size() != fftwsize  )
	{
	printf("complex_fwd_fft() -  incorrect array size, need: %d, have: vin.size()= %d\n", fftwsize, vin.size() );
	return;
	}




//load complex array
for( int i = 0; i < fftwsize; i++ )
	{
	oc = vin[ i ];
	fft_in_c0[ i ][ 0 ] = oc.real;
	fft_in_c0[ i ][ 1 ] = oc.imag;
	}


fftw_execute( fft_p_fwd_c0 );						//calc complex to complex fwd fft


//store fft result
for( int i = 0; i < fftwsize; i++ )
	{
	oc.real = fft_out_c0[ i ][ 0 ] / fftwsize;
	oc.imag = fft_out_c0[ i ][ 1 ] / fftwsize;
	
	vsp.push_back( oc );
	}

}









//reverse complex fft
//vin spectum must be supplied in fftw ordering
void complex_rev_fft( vector<filter_code::st_cplex_tag> vsp_in, vector<filter_code::st_cplex_tag> &vsp_out, unsigned int fftwsize )
{
st_spect_tag o;
filter_code::st_cplex_tag oc;

vsp.clear();


if( vsp_in.size() != fftwsize )
	{
	printf("complex_rev_fft() -  incorrect array size, need: %d, have: vin.size()= %d\n", fftwsize, vsp_in.size() );
	return;
	}






//load complex array
for( int i = 0; i < fftwsize; i++ )
	{
	oc = vsp_in[ i ];
	fft_in_c0[ i ][ 0 ] = oc.real;
	fft_in_c0[ i ][ 1 ] = oc.imag;
	}
	

fftw_execute( fft_p_rvs_c0 );						//calc complex to complex rev fft


//store fft result
for( int i = 0; i < fftwsize; i++ )
	{
	oc.real = fft_out_c0[ i ][ 0 ];
	oc.imag = fft_out_c0[ i ][ 1 ];
	
	vsp_out.push_back( oc );
	}


}











//forward complex fft, clears vsp
//spectrum is supplied in fftw ordering,
//scales result down to remove fftw 'gain',
//'thrd_id' ensures different calling thread don't get allocated the same fttw plan,
//returns 1 if plan exists and fft was completed, else 0
bool complex_fwd_fft_multi( vector<filter_code::st_cplex_tag> vin, vector<filter_code::st_cplex_tag> &vsp, string user_str, en_fftw_thread_id_tag thrd_id )
{
bool vb = 1;
st_spect_tag o;
filter_code::st_cplex_tag oc;

vsp.clear();

int sz = vin.size();

//printf("complex_fwd_fft_multi() - size is vin.size(): %d  user str: '%s'\n", vin.size(), user_str.c_str() );

if( fftw_plan_countc == 0 )
	{
	printf("complex_fwd_fft_multi() - 'fftw_plan_countc' is zero, can't do an fft\n" );
	return 0;
	}


//find plan with matching size
int iplan;
int matched_size_idx = -1;

for ( iplan = 0; iplan < fftw_plan_countc; iplan++ )
	{
	if( sz == fftwsiz[ iplan ] )
		{
		matched_size_idx = iplan;
		
		if( ftw_plan_thrd_id_fwdc[iplan] == thrd_id )
			{

			goto found;
			}
		}
	}


//if here NO suitable plan found, failing either a size match or thread id match

if( matched_size_idx < 0 )
	{
	printf("complex_fwd_fft_multi() -  incorrect array size, mo matching plan found, size is vin.size(): %d, user str: '%s'\n", vin.size(), user_str.c_str() );
	return 0;
	}


//if here NO matching 'thrd_id'
printf("complex_fwd_fft_multi() -  found matching plan array size (idx %d), but no matching 'thrd_id', size is vin.size(): %d, user str: '%s', 'thrd_id' %d\n", matched_size_idx, vin.size(), user_str.c_str(), thrd_id );



for ( iplan = 0; iplan < fftw_plan_countc; iplan++ )
	{
	if(vb)printf("complex_fwd_fft_multi() - avail plan size in fftwsiz[%d] is: %d, with ftw_plan_thrd_id_fwdc[%d]: %d\n", iplan, fftwsiz[ iplan ], iplan, ftw_plan_thrd_id_fwdc[iplan] );
	}
return 0;



found:

fftw_complex *fftwi = fftwic[ iplan ];
fftw_complex *fftwo = fftwoc[ iplan ];
fftw_plan fftwp = fftwp_fwdc[ iplan ];


//load complex array
for( int i = 0; i < sz; i++ )
	{
	oc = vin[ i ];
	fftwi[ i ][ 0 ] = oc.real;
	fftwi[ i ][ 1 ] = oc.imag;
	}


fftw_execute( fftwp );						//calc complex to complex fwd fft


//store fft result
for( int i = 0; i < sz; i++ )
	{
	oc.real = fftwo[ i ][ 0 ] / sz;
	oc.imag = fftwo[ i ][ 1 ] / sz;
	
	vsp.push_back( oc );
	}
return 1;
}









//reverse complex fft
//vin spectrum must be supplied in fftw ordering
//vsp_out is cleared
//returns 1 if plan exists and fft was completed, else 0
bool complex_rev_fft_multi( vector<filter_code::st_cplex_tag> vsp_in, vector<filter_code::st_cplex_tag> &vsp_out )
{
bool vb = 1;
st_spect_tag o;
filter_code::st_cplex_tag oc;

vsp_out.clear();


int sz = vsp_in.size();

if( fftw_plan_countc == 0 )
	{
	printf("complex_rev_fft_multi() - 'fftw_plan_countc' is zero, can't do an fft\n" );
	return 0;
	}

//find plan with matching size
int iplan;
for ( iplan = 0; iplan < fftw_plan_countc; iplan++ )
	{
	if( sz == fftwsiz[ iplan ] ) goto found;
	}

printf("complex_rev_fft_multi() -  incorrect array size, mo matching plan found, size is vin.size(): %d\n", vsp_in.size() );
for ( iplan = 0; iplan < fftw_plan_countc; iplan++ )
	{
	if(vb)printf("complex_rev_fft_multi() - avail plan size in fftwsiz[%d] is: %d\n", iplan, fftwsiz[ iplan ] );
	}
return 0;


found:

fftw_complex *fftwi = fftwic[ iplan ];
fftw_complex *fftwo = fftwoc[ iplan ];
fftw_plan fftwp = fftwp_rvsc[ iplan ];






//load complex array
for( int i = 0; i < sz; i++ )
	{
	oc = vsp_in[ i ];
	fftwi[ i ][ 0 ] = oc.real;
	fftwi[ i ][ 1 ] = oc.imag;
	}
	

fftw_execute( fftwp );						//calc complex to complex rev fft


//store fft result
for( int i = 0; i < sz; i++ )
	{
	oc.real = fftwo[ i ][ 0 ];
	oc.imag = fftwo[ i ][ 1 ];
	
	vsp_out.push_back( oc );
	}

return 1;
}






















//forward real fft, as fftw only provides a complex spectrum of half the size of 'vtdm' due to other half being
//the complex conjugate and therefore not calculated, this code creates the other half, so vsp has same size as vtdm,
//clears vsp
//spectrum is supplied in fftw ordering
//scales result down to remove fftw 'gain'

//negative most        first_neg         first_positive             most positive
//129 ........->..........255                 1.........->............ 127, 128			(loc 128 nyquist freq, is place in highest positive loc, i.e: right most)

bool real_fwd_fft_multi_float( vector<float> vin, vector<filter_code::st_cplex_tag> &vsp, string user_str, en_fftw_thread_id_tag thrd_id )
{
bool vb = 0;

st_spect_tag o;
filter_code::st_cplex_tag oc;

vsp.clear();

int sz = vin.size();
int half = sz / 2;

if( fftw_plan_countr == 0 )
	{
	printf("real_fwd_fft_multi() - 'fftw_plan_countr' is zero, can't do an fft\n" );
	return 0;
	}


//find plan with matching size
int iplan;
int matched_size_idx = -1;

for ( iplan = 0; iplan < fftw_plan_countr; iplan++ )
	{
	if( sz == fftwsiz[ iplan ] )
		{
		matched_size_idx = iplan;
		
		if( ftw_plan_thrd_id_fwdr[iplan] == thrd_id )
			{

			goto found;
			}
		}
	}


//if here NO suitable plan found, failing either a size match or thread id match

if( matched_size_idx < 0 )
	{
	printf("fftw_plan_countr() -  incorrect array size, mo matching plan found, size is vin.size(): %d, user str: '%s'\n", vin.size(), user_str.c_str() );
	return 0;
	}


//if here NO matching 'thrd_id'
printf("fftw_plan_countr() -  found matching plan array size (idx %d), but no matching 'thrd_id', size is vin.size(): %d, user str: '%s', 'thrd_id' %d\n", matched_size_idx, vin.size(), user_str.c_str(), thrd_id );



for ( iplan = 0; iplan < fftw_plan_countr; iplan++ )
	{
	if(vb)printf("complex_fwd_fft_multi() - avail plan size in fftwsiz[%d] is: %d, with ftw_plan_thrd_id_fwdr[%d]: %d\n", iplan, fftwsiz[ iplan ], iplan, ftw_plan_thrd_id_fwdr[iplan] );
	}
return 0;



found:
if(vb) printf("real_fwd_fft_multi() -  using plan fftwir[ %d ]\n", iplan );
double *fftwi = fftwir[ iplan ];
fftw_complex *fftwo = fftwoc[ iplan ];
fftw_plan fftwp = fftwp_fwdr[ iplan ];





//load real array
for( int i = 0; i < sz; i++ )
	{
	fftwi[ i ] = vin[ i ];
	}


fftw_execute( fftwp );						//calc real to complex fwd fft





//store fft result
//for( int i = 0; i < sz/2; i++ )
	{
//	oc.real = fftwo[ i ][ 0 ];
//	oc.imag = fftwo[ i ][ 1 ];
	
//	vsp.push_back( oc );
	}




//scale fft result
for( int i = 0; i < half; i++ )
	{
	oc.real = fftwo[ i ][ 0 ] / sz;
	oc.imag = fftwo[ i ][ 1 ] / sz;
	
	vsp.push_back( oc );
	}

//store fft complex conjugate;
int j = half;
for( int i = half; i < sz; i++ )
	{
	oc.real = vsp[ j ].real;
	oc.imag = -vsp[ j ].imag;
//oc.real = 0;
//oc.imag = 0;
	vsp.push_back( oc );
	j--;
	}

return 1;
}













//forward real fft, as fftw only provides a complex spectrum of half the size of 'vtdm' due to other half being
//the complex conjugate and therefore not calculated, this code creates the other half, so vsp has same size as vtdm,
//clears vsp
//spectrum is supplied in fftw ordering
//scales result down to remove fftw 'gain'

//negative most        first_neg         first_positive             most positive
//129 ........->..........255                 1.........->............ 127, 128			(loc 128 nyquist freq, is place in highest positive loc, i.e: right most)

bool real_fwd_fft_multi( vector<double> vin, vector<filter_code::st_cplex_tag> &vsp, string user_str, en_fftw_thread_id_tag thrd_id )
{
bool vb = 0;

st_spect_tag o;
filter_code::st_cplex_tag oc;

vsp.clear();

int sz = vin.size();
int half = sz / 2;

if( fftw_plan_countr == 0 )
	{
	printf("real_fwd_fft_multi() - 'fftw_plan_countr' is zero, can't do an fft\n" );
	return 0;
	}


//find plan with matching size
int iplan;
int matched_size_idx = -1;

for ( iplan = 0; iplan < fftw_plan_countr; iplan++ )
	{
	if( sz == fftwsiz[ iplan ] )
		{
		matched_size_idx = iplan;
		
		if( ftw_plan_thrd_id_fwdr[iplan] == thrd_id )
			{

			goto found;
			}
		}
	}


//if here NO suitable plan found, failing either a size match or thread id match

if( matched_size_idx < 0 )
	{
	printf("fftw_plan_countr() -  incorrect array size, mo matching plan found, size is vin.size(): %d, user str: '%s'\n", vin.size(), user_str.c_str() );
	return 0;
	}


//if here NO matching 'thrd_id'
printf("fftw_plan_countr() -  found matching plan array size (idx %d), but no matching 'thrd_id', size is vin.size(): %d, user str: '%s', 'thrd_id' %d\n", matched_size_idx, vin.size(), user_str.c_str(), thrd_id );



for ( iplan = 0; iplan < fftw_plan_countr; iplan++ )
	{
	if(vb)printf("complex_fwd_fft_multi() - avail plan size in fftwsiz[%d] is: %d, with ftw_plan_thrd_id_fwdr[%d]: %d\n", iplan, fftwsiz[ iplan ], iplan, ftw_plan_thrd_id_fwdr[iplan] );
	}
return 0;



found:
//if(vb) printf("real_fwd_fft_multi() -  using plan fftwir[ %d ]\n", iplan );
double *fftwi = fftwir[ iplan ];
fftw_complex *fftwo = fftwoc[ iplan ];
fftw_plan fftwp = fftwp_fwdr[ iplan ];





//load real array
for( int i = 0; i < sz; i++ )
	{
	fftwi[ i ] = vin[ i ];
	}


fftw_execute( fftwp );						//calc real to complex fwd fft





//store fft result
//for( int i = 0; i < sz/2; i++ )
	{
//	oc.real = fftwo[ i ][ 0 ];
//	oc.imag = fftwo[ i ][ 1 ];
	
//	vsp.push_back( oc );
	}




//scale fft result
for( int i = 0; i < half; i++ )
	{
	oc.real = fftwo[ i ][ 0 ] / sz;
	oc.imag = fftwo[ i ][ 1 ] / sz;
	
	vsp.push_back( oc );
	}

//store fft complex conjugate;
int j = half;
for( int i = half; i < sz; i++ )
	{
	oc.real = vsp[ j ].real;
	oc.imag = -vsp[ j ].imag;
//oc.real = 0;
//oc.imag = 0;
	vsp.push_back( oc );
	j--;
	}

return 1;
}


















/*

//forward real fft, as fftw only provides a complex spectrum of half the size of 'vtdm' due to other half being
//the complex conjugate and therefore not calculated, this code creates the other half, so vsp has same size as vtdm,
//clears vsp
//spectrum is supplied in fftw ordering
//scales result down to remove fftw 'gain'

//negative most        first_neg         first_positive             most positive
//129 ........->..........255                 1.........->............ 127, 128			(loc 128 nyquist freq, is place in highest positive loc, i.e: right most)

void real_fwd_fft_multi_delete( vector<double> vtdm, vector<filter_code::st_cplex_tag> &vsp, string user_str, en_fftw_thread_id_tag thrd_id )
{
st_spect_tag o;
filter_code::st_cplex_tag oc;

vsp.clear();

int sz = vtdm.size();
int half = sz / 2;


if( fftw_plan_countc == 0 )
	{
	printf("real_fwd_fft_multi() - 'fftw_plan_countc' is zero, can't do an fft\n" );
	return;
	}


//find plan with matching size
int iplan;
for ( iplan = 0; iplan < fftw_plan_countr; iplan++ )					//FIX THIS - starts at idx '8' as 'fftw_adjust_plans()' adjs the first few plans and causes a crash at this function
	{
	if( sz == fftwsiz[ iplan ] ) goto found;
	}

printf("real_fwd_fft_multi() -  incorrect array size, mo matching plan found, size is vtdm.size(): %d\n", vtdm.size() );
return;



found:
printf("real_fwd_fft_multi() -  using plan fftwir[ %d ]\n", iplan );
double *fftwi = fftwir[ iplan ];
fftw_complex *fftwo = fftwoc[ iplan ];
fftw_plan fftwp = fftwp_fwdr[ iplan ];


//load real array
for( int i = 0; i < sz; i++ )
	{
	fftwi[ i ] = vtdm[ i ];
	}


fftw_execute( fftwp );						//calc real to complex fwd fft


//store fft result
for( int i = 0; i < half; i++ )
	{
	oc.real = fftwo[ i ][ 0 ] / sz;
	oc.imag = fftwo[ i ][ 1 ] / sz;
	
	vsp.push_back( oc );
	}

//store fft complex conjugate;
int j = half;
for( int i = half; i < sz; i++ )
	{
	oc.real = vsp[ j ].real;
	oc.imag = -vsp[ j ].imag;
//oc.real = 0;
//oc.imag = 0;
	vsp.push_back( oc );
	j--;
	}

}

*/















//reverse complex to real fft
//vin spectrum must be supplied in fftw ordering
//vtdm is cleared
void real_rev_fft_multi( vector<filter_code::st_cplex_tag> vsp_in, vector<double> &vtdm )
{
st_spect_tag o;
filter_code::st_cplex_tag oc;

vtdm.clear();


int sz = vsp_in.size();

if( fftw_plan_countc == 0 )
	{
	printf("real_rev_fft_multi() - 'fftw_plan_countc' is zero, can't do an fft\n" );
	return;
	}


//find plan with matching size
int iplan;
for ( iplan = 0; iplan < fftw_plan_countr; iplan++ )
	{
	if( sz == fftwsiz[ iplan ] ) goto found;
	}

printf("real_rev_fft_multi() -  incorrect array size, mo matching plan found, size is vin.size(): %d\n", vsp_in.size() );
return;


found:

fftw_complex *fftwi = fftwic[ iplan ];
double *fftwo = fftwor[ iplan ];
fftw_plan fftwp = fftwp_rvsr[ iplan ];






//load complex array
for( int i = 0; i < sz; i++ )
	{
	oc = vsp_in[ i ];
	fftwi[ i ][ 0 ] = oc.real;
	fftwi[ i ][ 1 ] = oc.imag;
	}
	

fftw_execute( fftwp );						//calc complex to complex rev fft


//store time domain result
for( int i = 0; i < sz; i++ )
	{	
	vtdm.push_back( fftwo[ i ] );
	}


}




















/*



int filter_prev_index = 0;
double f_now_r;
double f_now_j;



//low pass down conversion filter
void low_pass_srconv_hold( vector <st_cplex_tag> &vin,  vector <st_cplex_tag> &vout, int downsample  )
//simple square window FIR
{
//see gc fprintf
//fprintf( stderr, "low_pass(): len= %u, d->downsample= %d\n", d->lp_len, d->downsample );
//low_pass(): len= 262144, d->downsample= 6

st_cplex_tag o;

int i = 0;


vout.clear();

while( i < vin.size() ) 
	{
	now_r += vin[ i ].real;
	now_j += vin[ i ].imag;
	i++;

	filter_prev_index++;
	if ( filter_prev_index < downsample ) continue;

	o.real = now_r;
	o.imag = now_j;
	vout.push_back( o );

	filter_prev_index = 0;
	now_r = 0;
	now_j = 0;
	}

//see gc fprintf
//fprintf( stderr, "low_pass(): len= %u\n", d->lp_len );
//low_pass(): len= 43690

}

*/







//low pass down conversion filter, requires persistent vars to be supplied: 'filt_prev_idx', 'now_r', 'now_j'
//in effect a box filter
void low_pass_srconv( vector <filter_code::st_cplex_tag> &vin,  vector <filter_code::st_cplex_tag> &vout, int downsample, int &filt_prev_idx, float &now_r, float &now_j  )
{

filter_code::st_cplex_tag o;

int i = 0;

vout.clear();

const float inv_downsample = 1.0f / downsample;							//do a once off divide

while( i < vin.size() ) 
	{
	now_r += vin[ i ].real;
	now_j += vin[ i ].imag;
	i++;

	filt_prev_idx++;
	if ( filt_prev_idx < downsample ) continue;

	o.real = now_r * inv_downsample;									//emit one sample
	o.imag = now_j * inv_downsample;
	vout.push_back( o );

	filt_prev_idx = 0;
	now_r = 0;
	now_j = 0;
	}

//printf("low_pass_srconv() - inv_downsample %d, vin %d  vout %d  filt_prev_idx %d\n", downsample, vin.size(), vout.size(), filt_prev_idx );

}








//low pass down conversion filter, requires persistent vars to be supplied: 'filt_prev_idx', 'now_r'
//in effect a box filter
void low_pass_srconv_float( vector <float> &vin, vector <float> &vout, int downsample, int &filt_prev_idx, float &now_r  )
{
float f0;

int i = 0;

vout.clear();

const float inv_downsample = 1.0f / downsample;							//do a once off divide

while( i < vin.size() ) 
	{
	now_r += vin[ i ];
	i++;

	filt_prev_idx++;
	if ( filt_prev_idx < downsample ) continue;

	f0 = now_r * inv_downsample;										//emit one sample
	vout.push_back( f0 );

	filt_prev_idx = 0;
	now_r = 0;
	}

//printf("low_pass_srconv() - inv_downsample %d, vin %d  vout %d  filt_prev_idx %d\n", downsample, vin.size(), vout.size(), filt_prev_idx );

}








//low pass down conversion filter, requires persistent vars to be supplied: 'filt_prev_idx', 'now_r', 'now_j'
//box filter
void low_pass_srconv_delete( vector <filter_code::st_cplex_tag> &vin,  vector <filter_code::st_cplex_tag> &vout, int downsample, int &filt_prev_idx, float &now_r, float &now_j  )
{

filter_code::st_cplex_tag o;

vout.clear();


const float inv_downsample = 1.0f / downsample;

if(filt_prev_idx <= 0 ) filt_prev_idx = downsample;						//do a once off init if needed
 
int i = 0;
while( i < vin.size() ) 
	{
	now_r += vin[ i ].real;
	now_j += vin[ i ].imag;
	i++;

    if ( (--filt_prev_idx) != 0 ) continue;

	o.real = now_r * inv_downsample;									//emit one sample
	o.imag = now_j * inv_downsample;
	vout.push_back( o );

	filt_prev_idx = downsample;
	now_r = 0.0f;
	now_j = 0.0f;

	}

//printf("low_pass_srconv() - inv_downsample %d, vin %d  vout %d  filt_prev_idx %d\n", downsample, vin.size(), vout.size(), filt_prev_idx );

//see gc fprintf
//fprintf( stderr, "low_pass(): len= %u\n", d->lp_len );
//low_pass(): len= 43690

}










/*
float fidx = 0;
float fidx2 = 0;



//low pass down conversion filter, requires persistent vars to be supplied: 'filt_prev_idx', 'now_r', 'now_j'
void low_pass_srconv_real2( vector<double> &vin,  vector<double> &vout, float downsample, float &filt_prev_idx, float &now_r, float scale_in  )
// simple square window FIR
{
//see gc fprintf
//printf( "low_pass_srconv_real(): len= %u, d->downsample= %d\n", d->lp_len, d->downsample );
//low_pass(): len= 262144, d->downsample= 6

int i = 0;


int cnt = vin.size();

float step = cnt / downsample;

vout.clear();

fidx = 0;
fidx2 = 0;

//while( i < vin.size() ) 
while( fidx < vin.size() ) 
	{
	i = floorf(fidx);
	now_r += vin[ i ];
	
	fidx2 += 0.025f;
	fidx += 0.025f;
//printf("fidx %f %f\n", fidx, fidx2 );
	
//	filt_prev_idx++;
	if ( fidx2 < downsample ) continue;

	vout.push_back( now_r*(scale_in/100.0f) );
//printf("pushed %d\n", vout.size() );
	fidx2 = 0;
	now_r = 0;
	}

//see gc fprintf
//printf( "low_pass_srconv_real(): len= %u\n", d->lp_len );

}
*/












/*

//low pass down conversion filter, requires persistent vars to be supplied: 'filt_prev_idx', 'now_r', 'now_j'
void low_pass_srconv_real2_delete( vector<double> &vin,  vector<double> &vout, float downsample, float &filt_prev_idx, float &now_r, float scale_in  )
// simple square window FIR
{
//see gc fprintf
//printf( "low_pass_srconv_real(): len= %u, d->downsample= %d\n", d->lp_len, d->downsample );
//low_pass(): len= 262144, d->downsample= 6

int i = 0;


vout.clear();

while( i < vin.size() ) 
	{
	now_r += vin[ i ];
	i++;

	filt_prev_idx++;
	if ( filt_prev_idx < downsample ) continue;

	vout.push_back( now_r*scale_in );

	filt_prev_idx = 0;
	now_r = 0;
	}

//see gc fprintf
//printf( "low_pass_srconv_real(): len= %u\n", d->lp_len );

}

*/















int i_filter_prev_index = 0;
float f_now_r;
float f_now_j;

//low pass down conversion filter
void low_pass_srconv_hold2( vector <filter_code::st_cplex_tag> &vin,  vector <filter_code::st_cplex_tag> &vout, int downsample  )
/* simple square window FIR */
{
//see gc fprintf
//fprintf( stderr, "low_pass(): len= %u, d->downsample= %d\n", d->lp_len, d->downsample );
//low_pass(): len= 262144, d->downsample= 6

filter_code::st_cplex_tag o;

int i = 0;


vout.clear();

while( i < vin.size() ) 
	{
	f_now_r += vin[ i ].real;
	f_now_j += vin[ i ].imag;
	i++;

	i_filter_prev_index++;
	if ( i_filter_prev_index < downsample ) continue;

	o.real = f_now_r;
	o.imag = f_now_j;
	vout.push_back( o );

	i_filter_prev_index = 0;
	f_now_r = 0;
	f_now_j = 0;
	}

//see gc fprintf
//fprintf( stderr, "low_pass(): len= %u\n", d->lp_len );
//low_pass(): len= 43690

}












//float f_filter_prev_index = 0;

//float now_ampl;
//float now_freq;

//low pass down conversion filter
void low_pass_srconv_spect( vector <st_spect_tag> &vin,  vector <st_spect_tag> &vout, float downsample, int &filter_prev_index, float &now_ampl, float &now_freq )
/* simple square window FIR */
{
//see gc fprintf
//fprintf( stderr, "low_pass_srconv_spect(): len= %u, d->downsample= %d\n", d->lp_len, d->downsample );
//low_pass(): len= 262144, d->downsample= 6

st_spect_tag o;

int i = 0;


vout.clear();

while( i < vin.size() ) 
	{
	now_ampl += vin[ i ].ampl;
	now_freq += vin[ i ].freq;
	i++;

	filter_prev_index += 1.0f;
	if ( filter_prev_index < downsample ) continue;

	o.ampl = now_ampl;
	o.freq = now_freq;
	vout.push_back( o );

	filter_prev_index = 0;
	now_ampl = 0;
	now_freq = 0;
	}
}





/*


//float f_filter_prev_index = 0;

//float now_ampl;
//float now_freq;

//low pass down conversion filter
void low_pass_srconv_spect_hann( vector <st_spect_tag> &vin,  vector <st_spect_tag> &vout, float downsample, int &filter_prev_index, float &now_ampl, float &now_freq )
{
//see gc fprintf
//fprintf( stderr, "low_pass_srconv_spect(): len= %u, d->downsample= %d\n", d->lp_len, d->downsample );
//low_pass(): len= 262144, d->downsample= 6






st_spect_tag o;

int i = 0;


vout.clear();

while( i < vin.size() ) 
	{
	now_ampl += vin[ i ].ampl;
	now_freq += vin[ i ].freq;
	i++;

	filter_prev_index += 1.0f;
	if ( filter_prev_index < downsample ) continue;

	o.ampl = now_ampl;
	o.freq = now_freq;
	vout.push_back( o );

	filter_prev_index = 0;
	now_ampl = 0;
	now_freq = 0;
	}
}

*/





















vector <filter_code::st_cplex_tag> viq_loaded;


int iq_load_missed = 0;


int iq_loaded_state = 0;		//0: waiting for a load to occur in 'rtlsdr_callback()'
								//1:'rtlsdr_callback()' has loaded iq, waiting for 'thrd3_cb()' to process and set state 0 again 


void experiment0( int ptr_start, int cnt )
{

if( iq_loaded_state != 0 ) 
	{
	iq_load_missed++;

	return;
	}


int rd0 = ptr_start;


for( int i = 0; i < cnt; i++ )						//rtl adc srate 3200000
	{
	filter_code::st_cplex_tag o;

	o.real = rtl_bfI0[ rd0 ];
	o.imag = rtl_bfQ0[ rd0 ];
	viq_loaded.push_back( o );
	
	rd0++;

	if( rd0 >= cn_rtl_buf_size ) rd0 = 0;

//	dbg1-= 1;		//i+q count down
	}

iq_loaded_state = 1;


}










vector <filter_code::st_cplex_tag> viq;
//vector <filter_code::st_cplex_tag> viq_local_zero_pad;




//vector <st_cplex_tag> viq2;
//bool b_viq2_loaded;

bool rtl_loaded = 0;

bool vfm_loaded = 0;


double d_last = 0;

int shutdown_count = 0;




//int dbg_rtl0 = 0;

int rtlsdr_callback_cnt = 0;





//called periodically by rtl library, loads 'viq_loaded'
////note: 'vdownsamp' is loaded in 'thrd3_cb()'
static void cb_rtlsdr_async( unsigned char *buf, uint32_t len, void *arg )
{

double d = tim0.time_passed( tim0.ns_tim_start );
tim0.time_start( tim0.ns_tim_start );

if( !(rtlsdr_callback_cnt%50 ) ) printf( "rtlsdr_callback() - tim0.time_passed = %g, buf len is %d\n", d, len );
//printf( "rtlsdr_callback() - tim0.time_passed = %g, buf len is %d\n", d, len );
//printf( "rtlsdr_callback() - tim0.time_passed= %lf, delta: %lf,  len = %u\n", d, d - d_last, len );



if( !b_rtlsdr_callback_first )
	{
	printf( "rtlsdr_callback() - first call - rtl_bf0_wr %d rtl_bf0_rd %d\n", rtl_bf0_wr, rtl_bf0_rd );
	}

//if( rtl_loaded )
//	{
//	printf( "rtlsdr_callback() - unprocessed buffer, lost samples\n" );
//	return;
//	}


//pthread_mutex_lock( &mutex2 ); 					//mutex as viq is being modified


if( sync_wr_rd_pointer != 0 )
	{
	if( sync_wr_rd_pointer == 1 )
		{
		sync_wr_rd_pointer = 2;					//wait for audio proc ack
		}




	if( sync_wr_rd_pointer == 3 )
		{

		rtl_bf0_wr = cn_rtl_buf_size - 1;
		idbg_wr = rtl_bf0_wr;

		rtl_bf0_rd = 0;
		idbg_rd = 0;
		
		sync_wr_rd_pointer = 0;
		}
	}

//bool b_viq2_gather = 0;
//if (!b_viq2_loaded )
//	{
//	b_viq2_gather = 1;
//	viq2.clear();
//	}

/*
if( b_rdwr_rephase )
	{
	b_rdwr_rephase = 0;
	rtl_bf0_wr = cn_rtl_buf_size/2;
	rtl_bf0_rd = 0;
	
	idbg_wr = rtl_bf0_wr;
	idbg_rd = 0;
	dbg1 = 0;
	
	vgph1_x.clear();
	vgph1_y0.clear();
	vgph1_y1.clear();
	}
*/

int ptr_start = rtl_bf0_wr;
if( sync_wr_rd_pointer == 0 )
	{

	filter_code::st_cplex_tag o;
	for( int i = 0; i < len; i += 2 )
		{
		o.real = buf[ i ] - 127;
		o.imag = buf[ i + 1 ] - 127;
	//	viq.push_back( o );
		
	//	if( b_viq2_gather )
	//		{
	//		viq2.push_back( o );
	//		}
			
		rtl_bfI0[ rtl_bf0_wr ] = buf[ i ] - 127;
		rtl_bfQ0[ rtl_bf0_wr ] = buf[ i + 1 ] - 127;

		rtl_bf0_wr++;
		if( rtl_bf0_wr >= cn_rtl_buf_size ) rtl_bf0_wr = 0;
		
		idbg_wr++;
	//	dbg1 += 1 ; //i+q count up
		}
	}

//if( b_viq2_gather ) b_viq2_loaded = 1;

//bf_wr_float++;

//printf( "rtlsdr_callback() - rtl_bf0_wr %d, dbg1 %d\n", rtl_bf0_wr, dbg1 );

//printf( "rtlsdr_callback() - viq %u\n", viq.size() );



//pthread_mutex_unlock( &mutex2 ); 

//rtl.reset_buffer();

experiment0( ptr_start, len/2 );


//mystr mdly0;

//mdly0.delay_ms( 20 );



//rtl_loaded = 1;

shutdown_count++;

if( shutdown_count >= 5 )
	{
//	rtl.cancel_async( );
	}

b_rtlsdr_callback_first = 1;

d_last = d;
rtlsdr_callback_cnt++;
}










int filt_prev_idx20 = 0;					//used for 'low_pass_srconv()' calls
float now_r20 = 0;
float now_j20 = 0;

//thrd3_cb_cnt = 0;


/*

//thread converts 'viq_loaded' into 'vdownsamp'
//note:  'viq_loaded' is loaded in 'cb_rtlsdr_async()'
void thrd3_cb( void* args )
{
mystr m1;

string nm;
int count = 0;

//printf("callback1\n");

gcthrd *o = (gcthrd*) args;

strpf( nm, "%s -", o->obj_name.c_str() );

if( o->thrd_dbg ) printf( "%s thrd started\n", nm.c_str() );

if( !rtl.open_status() ) 
	{
	printf( "%s thrd: rtl not open\n", nm.c_str() );
	return;
	}


while( 1 )
	{
//	printf( "thrd3_cb()\n" );

	if( o->thrd.kill ) goto finthread;

	if( iq_loaded_state == 1 )
		{

		if( !(count%100) ) printf( "thrd3_cb() - processing, iq_load_missed %d\n", iq_load_missed );

		int downsample_factor = 40;

		vector <filter_code::st_cplex_tag> vdownsamp;
		vector <filter_code::st_cplex_tag> vcplex_sp0;
		//downsample_factor = 10;
		low_pass_srconv( viq_loaded,  vdownsamp, downsample_factor, filt_prev_idx20, now_r20, now_j20 );					//complex decimate

//vdownsamp = viq_loaded;

		if( i_fftw_trig_plan_create_state == 2 )						//plans created?
			{
			bool b_plan_exists = complex_fwd_fft_multi( vdownsamp, vcplex_sp0, "thrd3_cb() - for demod audio", en_ftid_gui );			//go to freq domain using an fft
			}

		vector<st_spect_tag> vspect_displayble;
		complex_fft_displayable( vcplex_sp0, vspect_displayble, 1, 1 );

//		if( ( b_plot_gph1 == 0 ) && ( count < 50 ) )
//		if( b_plot_gph2 == 0 )
		if( 0 )
			{
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

			for( int i = 0; i < vspect_displayble.size(); i++ )
				{
				st_spect_tag os;
				
				float freq_left = g_freq_tune - g_dev_bw/2;
				
				os.freq = freq_left + g_dev_bw * ( (float)i / vspect_displayble.size() );
				os.ampl = vspect_displayble[i].ampl;

				vgph1_x.push_back( os.freq );
				vgph1_y0.push_back( os.ampl );
				vgph1_y1.push_back( 0 );
				}
			if( !(count%2) ) b_plot_gph1 = 1;
//			b_plot_gph1 = 1;
//			if( !(count%10) ) if( wnd_rtl_graph ) wnd_rtl_graph->plot_gph01();
			}


//vgph1_x
//vgph1_y0

		viq_loaded.clear();		
		iq_loaded_state = 0;
		}

	m1.delay_ms( 1 );			//don't hog processor in this while()
	count++;
	}


finthread:
o->thrd.finished = 1;
o->thrd.kill = 0;

if( o->thrd_dbg ) printf( "%s thrd finished\n", nm.c_str() );
}
*/










//NOT USED ANYMORE
//processes iq data into a format suitable for disp on graticule graph
float theta04 = 0;
float theta05 = 0;

void thrd4_cb( void* args )
{
mystr m1;

string nm;
int count = 0;

//printf("callback1\n");

gcthrd *o = (gcthrd*) args;

strpf( nm, "%s -", o->obj_name.c_str() );

if( o->thrd_dbg ) printf( "%s thrd started\n", nm.c_str() );

//if( !rtl.open_status() ) 
//	{
//	printf( "%s thrd: rtl not open\n", nm.c_str() );
//	return;
//	}


while( 1 )
	{

	if( o->thrd.kill ) goto finthread;

//m1.delay_ms( 1 );			//don't hog processor in this while()
//continue;
	
	if( fft_disp_state == 1 )
		{
		if( !(count%200) ) printf("thrd4_cb() - viq_local_fft_disp.size() %d\n", viq_local_fft_disp.size() );

		count++;


		if( b_build_taper_window0 )
			{
			
			filter_code::window_function_float( filter_code::fwt_hann, viq_local_fft_disp.size(), vtaper_window0 );

//			int cnt = viq_local_zero_pad.size();
//			for( int i = 0; i < cnt; i++ )
//				{
//				float f0 = 0.5f - (0.5f * cosf( (twopi * i) / (cnt - 1) ) );		//hann window test
//				vtaper_window.push_back( f0 );
//				}
			b_build_taper_window0 = 0;
			}



int cnt = viq_local_fft_disp.size();

float theta_inc04 = 10000.0f * twopi / 100000;							//106496
float theta_inc05 = g_freq_tune * twopi / 100000;

		if( 1 )
			{
			//apply a window function to timedomain signal to reduce spectral leakage in fft conversion
			if( vtaper_window0.size() == viq_local_fft_disp.size() )
				{
				for( int i = 0; i < vtaper_window0.size(); i++ )
					{
					float f0 = vtaper_window0[i];
					
					if( 0 )
						{
						float fI0 = 0.5f + 0.5f*cosf( theta04 );			//debug sin wfm
						float fQ0 = 0.5f + 0.5f*sinf( theta04 );			//debug sin wfm

						float fI1 = 1.0f*cosf( theta05 );					//debug: tuner carrier to produce a zero I/F
						float fQ1 = 1.0f*sinf( theta05 );


						
						viq_local_fft_disp[i].real = fI0 * 1 * f0 ;
						viq_local_fft_disp[i].imag = fQ0 * 1 * f0;

						theta04 += theta_inc04;
						if( theta04 >= twopi ) theta04 -= twopi;

						theta05 += theta_inc05;
						if( theta05 >= twopi ) theta05 -= twopi;
						}
					else{

							//iq DC blocking
						if( b_dc_block_iq )
							{
							//DC blocking
							float alpha = 0.99985f;						//this val is only a guestimate
							
							float sum1 = viq_local_fft_disp[i].real - dc_block1_dly0;
							float sum2 = sum1 + alpha * dc_block1_dly1;

							dc_block1_dly0 = viq_local_fft_disp[i].real;
							dc_block1_dly1 = sum2;

							viq_local_fft_disp[i].real = sum2;



							//DC blocking
							sum1 = viq_local_fft_disp[i].imag - dc_block2_dly0;
							sum2 = sum1 + alpha * dc_block2_dly1;

							dc_block2_dly0 = viq_local_fft_disp[i].imag;
							dc_block2_dly1 = sum2;

							viq_local_fft_disp[i].imag = sum2;
							}


						//apply window taper
						if( pref_taper_windowing_for_fft )
							{
							viq_local_fft_disp[i].real *= f0;
							viq_local_fft_disp[i].imag *= f0;
							}
						}
					
					}
				}
			else{
				b_build_taper_window0 = 1;
				printf( "thrd4_cb() vtaper_window0.size() != vtaper_window0.size(): %d %d --- trigger a taper window rebuild\n", vtaper_window0.size(), viq_local_fft_disp.size() );
				}
			}



//int down_sample_factor_for_graph = vcplex_gph.size() / 4096;

//	//	down_sample_factor_for_graph = 30;

//vector <filter_code::st_cplex_tag> vcplex_sp_downsample;

//low_pass_srconv( viq_local_fft_disp,  vcplex_sp_downsample, down_sample_factor_for_graph, filt_prev_idx12, now_r12, now_j12 );			//complex decimate

//viq_local_fft_disp = vcplex_sp_downsample ;




//pref_zero_padding_for_fft = 1;

	//zero pad to increase fft bin reso
	int siz = viq_local_fft_disp.size();
	if( pref_zero_padding_for_fft )
		{
		filter_code::st_cplex_tag oc;
			
		oc.real = 0;
		oc.imag = 0;
		for( int i = 0; i < siz*(2-1); i++ )							//make SURE an fttw plan exists to hold the size of this signal, see 'fftw_adjust_plans()'
			{
			viq_local_fft_disp.push_back( oc );
			}
		}


	vector <filter_code::st_cplex_tag> vcplex_sp_downsample;

	//	down_sample_factor_for_graph = 30;
//		low_pass_srconv( viq_local_fft_disp,  vcplex_sp_downsample, down_sample_factor_for_graph, filt_prev_idx11, now_r11, now_j11 );			//complex decimate
	vcplex_sp_downsample = viq_local_fft_disp;




	vector <filter_code::st_cplex_tag> vcplex_sp;
	if( i_fftw_trig_plan_create_state == 2 )							//plans created?
		{
		bool b_plan_exists = complex_fwd_fft_multi( vcplex_sp_downsample, vcplex_sp, "thrd4_cb() for fft disp", en_ftid_gui ); //go to freq domain using an fft, for display on graticle graph
		}


	vector<st_spect_tag> vspect_displayble0;
	complex_fft_displayable( g_dev_bw, g_freq_tune, vcplex_sp, vspect_displayble0, 1, 1 );		//for display on graticle graph


//	int graph_points_required = vspect_displayble0.size();
	
	int down_sample_factor_for_graph = vspect_displayble0.size() /  (4096 * disp_spect_zoom_factor);	// use less downsampling if zoomed into graph

down_sample_factor_for_graph = 1;

	vector<st_spect_tag> vspect_displayble_dwnsamp;
//	low_pass_srconv_spect( vspect_displayble0,  vspect_displayble_dwnsamp, down_sample_factor_for_graph, spect_dnwnsamp_prev_idx0, now_ampl0, now_freq0 );





//hann
//0
//0.125
//0.25
//0.375
//0.5
//0.625
//0.75
//0.875


/*
float hann_sinc[8];
int wnwdth = 8;
float fsrate = 200000/50e-3;
float fmax = fsrate*0.25;

float r_g,r_w,r_a,r_snc,r_y;												//some local variables
int i, j;
float fractx = 0;

r_g = 2.0f * fmax / fsrate;													//calc gain correction factor

r_y = 0.0f;
for ( i = -( wnwdth / 2 ); i <= ( wnwdth / 2 - 1 ); i++ ) 					//for 1 window width
	{
    j = (int) fractx + i;          											//calc input sample index

    r_w = 0.5f - 0.5f * cosf( twopi * ( 0.5f + ( j - fractx ) / wnwdth ) );	//make a hann sample, will be used taper sinc's length

    r_a = twopi * ( j - fractx ) * fmax / fsrate;							//curr sinc location
    r_snc = 1.0f;
	if ( r_a != 0 ) r_snc = sinf( r_a ) / r_a;								//make a sinc (sin x/x) lpf sample

printf("thrd4_cb()  hann i: %d, r_w: %f, r_a: %f, r_snc: %f\n", i, r_w, r_a, r_snc );

//    if ( ( j >= 0 ) & ( j < bufsz ) )										//src sample avail?
//		{
//		r_y = r_y + r_g * r_w * r_snc * buf[ j ];							//convolve/mac: first: apply von hann tapering to sinc, then both these to a src sample (adj gain as well)
//		}
	}
*/








/*
		if( vspect_displayble0.size() > 8 )
			{
			for( int i = 0; i < vspect_displayble0.size()-8; i++ )
				{
				float sum = 0;

				for( int j = 0; j < 8; j++ )
					{				
					float f0 = vspect_displayble0[i + j].ampl;
					sum += f0 * hann_sinc[0];
					}
				
				sum /= 8.0f;
	//			vspect_displayble0[i].ampl = sum;
				}
			}
*/


/*
		vector<float> vampl;
		vector<float> vampl_dwnsmpl;
		
		for( int i = 0; i < vspect_displayble0.size(); i++ )
			{
			vampl.push_back( vspect_displayble0[i].ampl );
			}



		int srate_in = vspect_displayble0.size() / (50e-3);
		int output_count = 2048;//vspect_displayble0.size() / 4096;
		int wnwdth = 4;
		float fmax = srate_in * 0.75;
//		for( int i = 0; i < vspect_displayble0.size(); i++ )
			{
			gc_srateconv_code::qdss_resample_float_vector( output_count, srate_in, fmax, wnwdth, vampl, vampl_dwnsmpl );
			}


		
		for( int i = 0; i < vampl_dwnsmpl.size(); i++ )
			{
			st_spect_tag o;
			o.ampl = vampl_dwnsmpl[i];
			o.freq = 0;
			
			vspect_displayble_dwnsamp.push_back( o );
			}
*/

		//downsample while maintaining max spectral vals, downsampling reduces graph plot workload
		int cnt0 = vspect_displayble0.size();
		
		int dwnsmpl_ratio = ceilf( 32 / disp_spect_zoom_factor);		//reduce down sampling when zooming graph, graph plot workload reduces when zoomed in, this allows better zoomed in resolution
		if( dwnsmpl_ratio < 1 ) dwnsmpl_ratio = 1;
		
		for( int i = 0; i < cnt0; i += dwnsmpl_ratio )
			{
			if( i + dwnsmpl_ratio >= cnt0 ) break;
			float max = 0;
			st_spect_tag o;
			for( int j = 0; j < dwnsmpl_ratio; j++ )
				{
				o = vspect_displayble0[i+j];
				
				if( o.ampl > max ) max = o.ampl;

				}
			o.ampl = max;
//			o.freq = 0;
			
			vspect_displayble_dwnsamp.push_back( o );
			}




	if( !(count%100) ) printf("thrd4_cb() - disp_spect_zoom_factor %f   down_sample_factor_for_graph %d, vspect_displayble0.size() %d vspect_displayble_dwnsamp.size() %d\n", disp_spect_zoom_factor, down_sample_factor_for_graph, vspect_displayble0.size(), vspect_displayble_dwnsamp.size() );
//	if( !(count%200) ) printf("thrd4_cb() - down_sample_factor_for_graph %d, vspect_displayble0.size() %d vspect_displayble_dwnsamp.size() %d\n", vspect_displayble0.size(), vspect_displayble_dwnsamp.size() );

//vspect_displayble0 = vspect_displayble_dwnsamp;








//				float f0 = 0.5f - (0.5f * cosf( (twopi * i) / (cnt - 1) ) );		//hann window test


















//	vspect_displayble0 = vspect_displayble_dwnsamp;


	int half = vspect_displayble0.size() / 2;

	//vspect_displayble[ half + 100 ].ampl = 10.0f;

	//disp_spect_zoom_factor = 2.0f;

	if( disp_spect_zoom_factor != 0.0f )
		{
		int zoom_cnt = vspect_displayble0.size() / disp_spect_zoom_factor;

		int half2 = zoom_cnt / 2;
		
		int start = half - half2; 
		int end = half + half2; 
		
		vector<st_spect_tag> vspect_disp_actual;

		for( int i = start; i < end; i++ )
			{
			st_spect_tag o;
			
			o = vspect_displayble0[i];

//o.ampl *= 1.0f;
//o.freq = 0;
			vspect_disp_actual.push_back( o );
			}
			
		vspect_displayble = vspect_disp_actual;
		}


/*		
		if( 0 )
			{
			vgph50_y0.clear();
			for( int i = start; i < end; i++ )
				{
				st_spect_tag o;
				
				o = vspect_displayble0[i];

	//			vspect_disp_actual.push_back( o );
				
				vgph50_y0.push_back( o.ampl );
				}



			int gph_idx = 0;						//only one graph that has multiple traces
			gph5.position( 10, 100 );
			gph5.font_size( 9 );
			gph5.set_sig_dig( 2 );
			gph5.sample_rect_hints_distancex = 0;
			gph5.sample_rect_hints_distancey = 0;

			gph5.shift_y( gph_idx, 0, 0.0f, 0.0f, 0.0f);

			gph5.plot_vfloat( 0, vgph50_y0 );
			}
*/

			
//		vspect_displayble = vspect_disp_actual;
	//	complex_rev_fft_multi( vcplex_sp, vtdm );									//go to time domain using a rev fft








//----- mutex ----
		if( mutex3.try_lock() )
			{
			if( vcplex_gph.size() == 0 ) 
				{
				if( gph_loc_sel == 0 ) 				//
					{
	//				vcplex_gph = vcplex_iq_downsamp2;
					}

				if( gph_loc_sel == 1 ) 
					{
					}
				}


			if( vspect_gph.size() == 0 ) 
				{
				if( gph_loc_sel == 0 ) 				//
					{
					}

				if( gph_loc_sel == 1 ) 
					{
					vspect_gph = vspect_displayble;
					}
				}
			mutex3.unlock();
			}
//------------------

	fft_disp_state = 0;
	m1.delay_ms( 50 );			//don't hog processor in this while()
	}



	m1.delay_ms( 1 );			//don't hog processor in this while()
	}




finthread:
o->thrd.finished = 1;
o->thrd.kill = 0;

if( o->thrd_dbg ) printf( "%s thrd finished\n", nm.c_str() );
}













//thread calls 'rtl.read_async()' and blocks till 'rtl.cancel_async()' is called
void thrd_rtl_cb( void* args )
{
mystr m1;

string nm;
int count = 0;

//printf("callback1\n");

gcthrd *o = (gcthrd*) args;

strpf( nm, "%s -", o->obj_name.c_str() );

if( o->thrd_dbg ) printf( "%s thrd started\n", nm.c_str() );

if( !rtl.open_status() ) 
	{
	printf( "%s thrd: rtl not open\n", nm.c_str() );
	return;
	}



while( 1 )
	{
	printf( "thrd_rtl_cb()\n" );
	if( o->thrd.kill ) goto finthread;

	if( b_listen ) 
		{
		int bfcnt = 1;
		int bfsize = 4 * 65536 * 1;
		printf( "thrd_rtl_cb() - calling 'rtl.read_async()'\n" );


		rtl.read_async( cb_rtlsdr_async, bfcnt, bfsize );		//start repeated librtlsdr callbacks, this blocks till rtl.cancel_async() is called from another thread
		}
	
	double dt = m1.time_passed( m1.ns_tim_start );
	if( dt > 0.05 )
		{
		m1.time_start( m1.ns_tim_start );
//		if( o->thrd_dbg ) printf( "%s cb() -  hello %05d\n", nm.c_str(), count );

		}
	
	m1.delay_ms( 10 );			//don't hog processor in this while()
	count++;
	}


finthread:
o->thrd.finished = 1;
o->thrd.kill = 0;

if( o->thrd_dbg ) printf( "%s thrd finished\n", nm.c_str() );

}





void buf_free()
{
printf( "buf_free()\n");

if( rtl_bfI0 ) delete rtl_bfI0;
rtl_bfI0 = 0;

if( rtl_bfQ0 ) delete rtl_bfQ0;
rtl_bfQ0 = 0;
}





void buf_allocate()
{
buf_free();

printf( "buf_allocate() - buffers will be %d bytes in size\n", cn_rtl_buf_size );
rtl_bfI0 = new float[cn_rtl_buf_size];	
rtl_bfQ0 = new float[cn_rtl_buf_size];

if( rtl_bfI0 == 0 ) printf( "buf_allocate() - rtl_bfI0 not allocated\n" );
if( rtl_bfQ0 == 0 ) printf( "buf_allocate() - rtl_bfQ0 not allocated\n" );
}











void init_fftw_arrays()
{
for ( int i = 0; i < cn_fftw_size_demod_cnt; i++ )
	{

	fftwic[ i ] = 0;
	fftwoc[ i ] = 0;
	fftwp_fwdc[ i ] = 0;
	fftwp_rvsc[ i ] = 0;
	ftw_plan_thrd_id_fwdc[ i ] = en_ftid_gui;
	ftw_plan_thrd_id_rvsc[ i ] = en_ftid_gui;



	fftwir[ i ] = 0;
	fftwor[ i ] = 0;
	fftwp_fwdr[ i ] = 0;
	fftwp_rvsr[ i ] = 0;
	
	ftw_plan_thrd_id_fwdr[ i ] = en_ftid_gui;
	ftw_plan_thrd_id_rvsr[ i ] = en_ftid_gui;
	}
}



















void start_threads_rtl()
{
printf( "start_threads_rtl() ====================\n" );

pthread_mutex_init( &mutex2, NULL );



if( thrd_rtl == 0 ) 
	{
	thrd_rtl = new gcthrd();

	thrd_rtl->thrd_dbg = 1;
	thrd_rtl->set_name( "rtl_cb_thrd" );
	thrd_rtl->set_thrd_callback( thrd_rtl_cb, (void*)thrd_rtl );		//set thread callback
	}

thrd_rtl->create_thread( );



threads_rtl_started = 1;
}



















void start_threads_delete()
{
printf( "start_threads() ====================\n" );

//pthread_mutex_init( &mutex2, NULL );

/*
//alloc fftw pointers
fft_in_c0 = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftw_size_demod );
fft_out_c0 = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftw_size_demod );

//single dimension complex to complex fwd fft
fft_p_fwd_c0 = fftw_plan_dft_1d( fftw_size_demod, fft_in_c0, fft_out_c0, FFTW_FORWARD, FFTW_ESTIMATE );

//single dimension complex to complex rev fft
fft_p_rvs_c0 = fftw_plan_dft_1d( fftw_size_demod, fft_in_c0, fft_out_c0, FFTW_BACKWARD, FFTW_ESTIMATE );






fftwsiz[ 0 ] = fftw_size_demod_0;
fftwsiz[ 1 ] = fftw_size_demod_1;
fftwsiz[ 2 ] = fftw_size_demod_2;
fftwsiz[ 3 ] = fftw_size_demod_3;
fftwsiz[ 4 ] = fftw_size_demod_4;
fftwsiz[ 5 ] = fftw_size_demod_5;
fftwsiz[ 6 ] = fftw_size_demod_6;
fftwsiz[ 7 ] = fftw_size_demod_7;
fftwsiz[ 8 ] = fftw_size_demod_8;
fftwsiz[ 9 ] = fftw_size_demod_9;
fftwsiz[ 10 ] = fftw_size_demod_10;
fftwsiz[ 11 ] = fftw_size_demod_11;
fftwsiz[ 12 ] = fftw_size_demod_12;
fftwsiz[ 13 ] = fftw_size_demod_13;
fftwsiz[ 14 ] = fftw_size_demod_14;
fftwsiz[ 15 ] = fftw_size_demod_15;


fftw_plan_countc = sizeof( fftwsiz ) / sizeof( unsigned int );		//work out how many plans required


for ( int i = 0; i < fftw_plan_countc; i++ )
	{
	//alloc fftw pointers
	fftwic[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftwsiz[ i ] );
	fftwoc[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftwsiz[ i ] );

	//single dimension complex to complex fwd fft
	fftwp_fwdc[ i ] = fftw_plan_dft_1d( fftwsiz[ i ], fftwic[ i ], fftwoc[ i ], FFTW_FORWARD, FFTW_ESTIMATE );

	//single dimension complex to complex rev fft
	fftwp_rvsc[ i ] = fftw_plan_dft_1d( fftwsiz[ i ], fftwic[ i ], fftwoc[ i ], FFTW_BACKWARD, FFTW_ESTIMATE );
	}
	

//printf( "help1\n" );

fftw_plan_countr = fftw_plan_countc;								//work out how many plans required

for ( int i = 0; i < fftw_plan_countr; i++ )
	{
	//alloc fftw pointers
	fftwir[ i ] = ( double* ) fftw_malloc( sizeof( double ) * fftwsiz[ i ] );
	fftwor[ i ] = ( double* ) fftw_malloc( sizeof( double ) * fftwsiz[ i ] );

	//single dimension real to complex fwd fft
	fftwp_fwdr[ i ] = fftw_plan_dft_r2c_1d( fftwsiz[ i ], fftwir[ i ], fftwoc[ i ], FFTW_ESTIMATE );

	//single dimension complex to real rev fft
	fftwp_rvsr[ i ] = fftw_plan_dft_c2r_1d( fftwsiz[ i ], fftwic[ i ], fftwor[ i ], FFTW_ESTIMATE );
	}
*/


/*
if( b_use_rtl_dev )
	{
	if( thrd_rtl == 0 ) 
		{
		thrd_rtl = new gcthrd();

		thrd_rtl->thrd_dbg = 1;
		thrd_rtl->set_name( "rtl_cb_thrd" );
		thrd_rtl->set_thrd_callback( thrd_rtl_cb, (void*)thrd_rtl );		//set thread callback
		}

	thrd_rtl->create_thread( );
	}
*/

/*
if( thrd3 == 0 ) 
	{
	thrd3 = new gcthrd();

	thrd3->thrd_dbg = 1;
	thrd3->set_name( "cb_thrd3" );
	thrd3->set_thrd_callback( thrd3_cb, (void*)thrd3 );					//set thread callback
	}

thrd3->create_thread( );
*/



/*

if( thrd4 == 0 ) 
	{
	thrd4 = new gcthrd();

	thrd4->thrd_dbg = 1;
	thrd4->set_name( "cb_thrd4" );
	thrd4->set_thrd_callback( thrd4_cb, (void*)thrd4 );					//set thread callback
	}

thrd4->create_thread( );
*/

threads_started = 1;
}









void stop_threads()
{
printf( "stop_threads() ====================\n" );

pthread_mutex_unlock( &mutex2 );

pthread_mutex_destroy( &mutex2 );

rtl.cancel_async( );

if( thrd_rtl != 0 )
	{
	thrd_rtl->destroy_thread( );
	if( thrd_rtl->wait_till_thread_destroyed( 1000 ) )
		{
		delete thrd_rtl;
		thrd_rtl = 0;
		}
	}


/*
if( thrd3 != 0 )
	{
	thrd3->destroy_thread( );
	if( thrd3->wait_till_thread_destroyed( 1000 ) )
		{
		delete thrd3;
		thrd3 = 0;
		}
	}
*/


/*
if( thrd4 != 0 )
	{
	thrd4->destroy_thread( );
	if( thrd4->wait_till_thread_destroyed( 1000 ) )
		{
		delete thrd4;
		thrd4 = 0;
		}
	}
*/

/*
//free fftw items
if ( fft_p_fwd_c0 ) fftw_destroy_plan( fft_p_fwd_c0 );
fft_p_fwd_c0 = 0;

if ( fft_p_rvs_c0 ) fftw_destroy_plan( fft_p_rvs_c0 );
fft_p_rvs_c0 = 0;


if ( fft_in_c0 ) fftw_free( fft_in_c0 );
if ( fft_out_c0 ) fftw_free( fft_out_c0 );
fft_in_c0 = 0;
fft_out_c0 = 0;




for ( int i = 0; i < fftw_plan_countc; i++ )
	{

	if( fftwp_fwdc[ i ] ) fftw_destroy_plan( fftwp_fwdc[ i ] );
	fftwp_fwdc[ i ] = 0;

	if(  fftwp_rvsc[ i ] ) fftw_destroy_plan( fftwp_rvsc[ i ] );
	fftwp_rvsc[ i ] = 0;

	if( fftwic[ i ] ) fftw_free( fftwic[ i ] );
	fftwic[ i ] = 0;
	
	if( fftwoc[ i ] ) fftw_free( fftwoc[ i ] );
	fftwoc[ i ] = 0;
	}



for ( int i = 0; i < fftw_plan_countr; i++ )
	{

	if( fftwp_fwdr[ i ] ) fftw_destroy_plan( fftwp_fwdr[ i ] );
	fftwp_fwdr[ i ] = 0;
	
	if( fftwp_rvsr[ i ] ) fftw_destroy_plan( fftwp_rvsr[ i ] );
	fftwp_rvsr[ i ] = 0;

	if( fftwir[ i ] ) fftw_free( fftwir[ i ] );
	fftwir[ i ] = 0;
	
	if( fftwor[ i ] ) fftw_free( fftwor[ i ] );
	fftwor[ i ] = 0;
	}
*/

threads_rtl_started = 0;
threads_started = 0;
}







Fl_Menu_Item menu_graph_loc_sel[] = {		//ordering must MATCH 'en_graph_loc_sel_tag' enumerations, refer also 'gph_loc_sel' 'plot_using_vectors_eval()'
	{"IQ - timedomain",	0, cb_ch_graph_loc_sel, (void*)en_gls_IQ_timedomain},
	{"IQ - spect",	0, cb_ch_graph_loc_sel, (void*)en_gls_IQ_spect },
	{"sub tuner - spect",	0, cb_ch_graph_loc_sel, (void*)en_gls_sub_tuner_spect },
	{"dwncnv antialias iir - spect",	0, cb_ch_graph_loc_sel, (void*)en_gls_dwncnv_aa_spect },
	{"dwncnv - timedomain ",	0, cb_ch_graph_loc_sel, (void*)en_gls_downcnv_timedomain },
	{"dwncnv - spect",	0, cb_ch_graph_loc_sel, (void*)en_gls_downcnv_spect},
	{"bandpass fir - timedomain",	0, cb_ch_graph_loc_sel, (void*)en_gls_bandpass_fir_timedomain },
	{"bandpass fir - spect",	0, cb_ch_graph_loc_sel, (void*)en_gls_bandpass_fir_spect},
//	{"IQ - Spect Filter DwnCnv Shifted",	0, cb_ch_graph_loc_sel, (void*)en_gls_IQ_spect_downcnv_filter_iffreq_spect_shift},
//	{"IQ - Spect Noise Reduction",	0, cb_ch_graph_loc_sel, (void*)en_gls_IQ_spect_noise_reduction},
//	{"TimeDmain",	0, cb_ch_graph_loc_sel, (void*)en_gls_timedomain},
	{"demod aud - timedomain",	0, cb_ch_graph_loc_sel, (void*)en_gls_demod_aud_timedomain},
	{"demod aud dcblocked - timedomain",	0, cb_ch_graph_loc_sel, (void*)en_gls_demod_aud_dcblock_timedomain },
	
	{"demod aud - spect",	0, cb_ch_graph_loc_sel, (void*)en_gls_demod_aud_spect},
	{"fm demod mpx - spect",	0, cb_ch_graph_loc_sel, (void*)en_gls_fm_demod_spect_mpx},
	{"fm demod mpx halfband decimator - spect",	0, cb_ch_graph_loc_sel, (void*)en_gls_fm_demod_mpx_halfband_decimator_spect},

	{"fm demod pilot pll error - timedomain",	0, cb_ch_graph_loc_sel, (void*)en_gls_fm_demod_pilot_pll_err},
	{"fm demod pilot rectified peak - timedomain",	0, cb_ch_graph_loc_sel, (void*)en_gls_fm_demod_pilot_rectified_peak},
	{"fm demod pilot bounce - timedomain",	0, cb_ch_graph_loc_sel, (void*)en_gls_fm_demod_pilot_bounce},
	{"fm demod pll 38K osc - spect",	0, cb_ch_graph_loc_sel, (void*)en_gls_fm_demod_pll_38K_osc},

	{"fm demod pilot vs.local osc phase compare - timedomain",	0, cb_ch_graph_loc_sel, (void*)en_gls_fm_demod_pll_osc_compare},

	{"fm demod L-R BPF - spect",	0, cb_ch_graph_loc_sel, (void*)en_gls_fm_demod_left_minus_right_bpf_spect},
	{"fm demod L-R - timedomain",	0, cb_ch_graph_loc_sel, (void*)en_gls_fm_demod_left_minus_right_timedomain},
	{"fm demod L-R - spect",	0, cb_ch_graph_loc_sel, (void*)en_gls_fm_demod_left_minus_right_spect},

	{"fm demod demux aud ch0 - timedomain",	0, cb_ch_graph_loc_sel, (void*)en_gls_fm_ster_demux_aud_ch0_timedomain},
	{"fm demod demux aud ch1 - timedomain",	0, cb_ch_graph_loc_sel, (void*)en_gls_fm_ster_demux_aud_ch1_timedomain},
	{"fm demod demux aud ch0 - spect",	0, cb_ch_graph_loc_sel, (void*)en_gls_fm_ster_demux_aud_ch0_spect},
	{"fm demod demux aud ch1 - spect",	0, cb_ch_graph_loc_sel, (void*)en_gls_fm_ster_demux_aud_ch1_spect},
	
//	{"TimeDmain - demod DwnCnv",	0, cb_ch_graph_loc_sel, (void*)en_gls_timedomain_demod_downcnv},
//	{"Spect - demod DwnCnv",	0, cb_ch_graph_loc_sel, (void*)en_gls_spect_demod_downcnv},
	{"demod agc audio peak - timedomain",	0, cb_ch_graph_loc_sel, (void*)en_gls_demod_agc_audio_peak_timedomain },
//	{"TimeDmain - Demod - agc filter",	0, cb_ch_graph_loc_sel, (void*)en_gls_agc_filter },
	{"demod agc gain control lvl - timedomain",	0, cb_ch_graph_loc_sel, (void*)en_gls_demod_agc_gain_ctrl_timedomain },
	{"demod aud clipper/agc - timedomain",	0, cb_ch_graph_loc_sel, (void*)en_gls_demod_aud_clip_agc_timedomain },

	{"demod - aud resampler ch0 - timedomain",	0, cb_ch_graph_loc_sel, (void*)en_gls_demod_aud_resampler_ch0_timedomain },
	{"demod - aud resampler ch1 - timedomain",	0, cb_ch_graph_loc_sel, (void*)en_gls_demod_aud_resampler_ch1_timedomain },
	{"demod - aud resampler ch0 - spect",	0, cb_ch_graph_loc_sel, (void*)en_gls_demod_aud_resampler_ch0_spect },
	{"demod - aud resampler ch1 - spect",	0, cb_ch_graph_loc_sel, (void*)en_gls_demod_aud_resampler_ch1_spect },

	{"filtresponse - antialias iir",	0, cb_ch_graph_loc_sel, (void*)en_gls_filtresp_antialias_iir },
	{"filtresponse - demod bpf fir",	0, cb_ch_graph_loc_sel, (void*)en_gls_filtresp_demod_bpf_fir },
	{"filtresponse - demod lpf0 iir",	0, cb_ch_graph_loc_sel, (void*)en_gls_filtresp_demod_lpf0_iir },
	{"filtresponse - demod hpf iir",	0, cb_ch_graph_loc_sel, (void*)en_gls_filtresp_demod_hpf_iir },

	{"Probe0 - en_gls_tdm_probe_vflt0, set probe point where you like in code",	0, cb_ch_graph_loc_sel, (void*)en_gls_tdm_probe_vflt0 },
	{"Probe1 - en_gls_spect_probe_vflt0, set probe point where you like in code",	0, cb_ch_graph_loc_sel, (void*)en_gls_spect_probe_vflt0 },
	{"Probe2 - en_gls_spect_probe_vcpx0, set probe point where you like in code",	0, cb_ch_graph_loc_sel, (void*)en_gls_spect_probe_vcpx0 },
	{0}
};









void cb_wfall0_set_left_click_cb( Fl_Widget *w, void *v )
{

cl_waterfall* o = (cl_waterfall*) w;


int frq = wnd_rtl_graph->wfall0->freq_disp0 - g_freq_tune;

frq /= 1000;															//quantize to 1KHz boundaries
frq *= 1000;

wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( frq );

wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_wfall0_set_left_click_cb()" );




/*


int drop_factor = 1;
if( ( pref_freq_mousewheel_low_digit_zero_cnt > 0 ) )
	{
	if( pref_freq_mousewheel_low_digit_zero_cnt == 4 ) drop_factor = 10000;
	if( pref_freq_mousewheel_low_digit_zero_cnt == 3 ) drop_factor = 1000;
	if( pref_freq_mousewheel_low_digit_zero_cnt == 2 ) drop_factor = 100;
	if( pref_freq_mousewheel_low_digit_zero_cnt == 1 ) drop_factor = 10;
	
	if( drop_factor > 1 )
		{
//		if( band == 0 ) drop_factor *= 10;
		
		int i0 = frq / drop_factor;									//drop digits
		
		i0 *= drop_factor;
		
		frq = i0;
		}
	}



wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( frq - g_freq_tune );

//cb_bt_freq_tune( 0, 0 );
wnd_rtl_graph->freq_listen( 1, 0, 0, 0 );
*/

printf( "cb_wfall0_set_left_click_cb() - freq_disp0 %d\n", o->freq_disp0 );
}














void cb_wfall0_mousemove_cb( void *w, void *args, int dir )
{

cl_waterfall* o = wnd_rtl_graph->wfall0;

if( !o->inside_control ) return;


double mx, my;
int px, py;


px = wnd_rtl_graph->wfall0->mousex;
py = 0;

if( wnd_rtl_graph->gph0->get_pixel_position_as_trc_values( 0, px, py, mx, my, 1, 1 ) )		//use gph0 to get freq the mouse sits at
	{
	}

wnd_rtl_graph->wfall0->freq_disp0 = mx;
}











void cb_wfall0_mousewheel_cb( void *w, void *args, int dir )
{

cl_waterfall* o = wnd_rtl_graph->wfall0;

if( !o->inside_control ) return;

double frq = wnd_rtl_graph->miw_freq_sub_tune->get_value_as_double();


int drop_factor = 1;
if( ( pref_freq_mousewheel_low_digit_zero_cnt > 0 ) )
	{
	if( pref_freq_mousewheel_low_digit_zero_cnt == 4 ) drop_factor = 10000;
	if( pref_freq_mousewheel_low_digit_zero_cnt == 3 ) drop_factor = 1000;
	if( pref_freq_mousewheel_low_digit_zero_cnt == 2 ) drop_factor = 100;
	if( pref_freq_mousewheel_low_digit_zero_cnt == 1 ) drop_factor = 10;
	
	if( drop_factor > 1 )
		{		
//		if( band == 0 ) drop_factor *= 10;
		
		int i0 = frq / drop_factor;									//drop digits
		
		i0 *= drop_factor;
		
		frq = i0;
		}
	}


int step = drop_factor;

//printf("cb_wfall0_mousewheel_cb() - frq %f  step %d\n", frq, step*dir ); 
frq += step * dir;
//printf("cb_wfall0_mousewheel_cb() - frq %f  step %d\n", frq, step*dir ); 

wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( frq );
wnd_rtl_graph->freq_listen( 0, 0, 0, 0, "cb_wfall0_mousewheel_cb()" );

printf( "cb_wfall0_mousewheel_cb() - frq %d\n", frq );
}






bool preset_ask_overwrite()
{
string s1;

strpf( s1, "Overwrite Preset ?" );
int ret = fl_choice( s1.c_str(),"Cancel","Overwrite", 0 );
if( ret == 0 )
	{
	return 0;
	}
return 1;
}





void cb_led_freq_memory_combo( Fl_Widget *w, void* v )
{
int which = (intptr_t)v;

wnd_rtl_graph->led_freq_memory_turn_off();
wnd_rtl_graph->led_preset_memory_turn_off();
wnd_rtl_graph->led_preset_memory_turn_off2();

GCLed *o;
o =(GCLed *)w;

//printf( "cb_led_freq_memory_combo() - which %d, right_button %d\n", which, o->right_button );

//getchar();


if( o->right_button )
	{
	//store
	if( pref_ask_preset_overwrite ) 
		{
		if( !preset_ask_overwrite() ) return;
		}
	}


int i = o->GetColIndex();
if( i >= 0 )
	{
	i++;
	if( i > 1 ) i = 0;
	o->ChangeCol( i );
	}


if( o->right_button )
	{
	//store
		
	wnd_rtl_graph->freq_memory[which].freq_tune = wnd_rtl_graph->miwp_tune->miw->get_value_as_double();
	wnd_rtl_graph->freq_memory[which].freq_sub_tune = wnd_rtl_graph->miw_freq_sub_tune->get_value_as_double();
	}
else{
	int frq = wnd_rtl_graph->freq_memory[ which ].freq_tune;
	wnd_rtl_graph->miwp_tune->miw->set_value_from_double( frq );

	frq = wnd_rtl_graph->freq_memory[ which ].freq_sub_tune;
	wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( frq );

	wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_led_freq_memory_combo()" );
	}
}





void cb_led_freq_memory_mouse_event( void *obj_in, void *args_in )
{
GCLed *ol = (GCLed*)obj_in;

rtl_graph_wnd *ow = wnd_rtl_graph;

//printf( "cb_led_freq_memory_mouse_event() - args_in %d, id %d  last_event %d\n", (int)args_in, ol->id, ol->last_event );

ow->led_freq_mem_hov_idx = ol->id;
ow->led_preset_hov_idx = -1;
ow->led_preset_hov_idx2 = -1;
}








void cb_led_preset_memory_combo( Fl_Widget *w, void* v )
{
int which = (intptr_t)v;
string s1;
mystr m1;

wnd_rtl_graph->led_freq_memory_turn_off();
wnd_rtl_graph->led_preset_memory_turn_off();
wnd_rtl_graph->led_preset_memory_turn_off2();

GCLed *o;
o =(GCLed *)w;

rtl_graph_wnd *ow = wnd_rtl_graph;

printf( "cb_led_preset_memory_combo() - which %d, right_button %d\n", which, o->right_button );

//getchar();

if( o->right_button )
	{
	//store
	if( pref_ask_preset_overwrite ) 
		{
		if( !preset_ask_overwrite() ) return;
		}
	}


int i = o->GetColIndex();
if( i >= 0 )
	{
	i++;
	if( i > 1 ) i = 0;
	o->ChangeCol( i );
	}

if( o->right_button )
	{
	//store
	bool b_use_freq = 0;

	s1 = ow->fi_name->value();
	if( s1.length() == 0 ) b_use_freq = 1;
	if( s1.compare("???") == 0 ) b_use_freq = 1;

	if( b_use_freq )
		{
		string snum, sunits, scombined;
		int fractional_digits = 4;

		m1.make_engineering_str( snum, sunits, scombined, fractional_digits,  ow->miwp_tune->miw->get_value_as_double(), "", "Hz" );

		strpf( s1, "%s", scombined.c_str() );
		}


//g_user_iir_hpf0

	ow->preset_memory[which].sname = s1;
	ow->preset_memory[which].freq_tune = ow->miwp_tune->miw->get_value_as_double();
	ow->preset_memory[which].freq_sub_tune = ow->miw_freq_sub_tune->get_value_as_double();
	ow->preset_memory[which].demod_type = (en_demodulator_type_tag)ow->fi_demodul_mode->value();


	ow->preset_memory[which].b_dwn_aa = ow->ck_user_dwn_aa->value();
	ow->preset_memory[which].i_dwn_srate = ow->miw_dev_dwnconv_srate->get_value_as_double();

	ow->preset_memory[which].b_use_iffreq = ow->ck_if_freq->value();
	ow->preset_memory[which].if_freq = ow->miw_if_freq->get_value_as_double();

	ow->preset_memory[which].b_bw_limit = ow->ck_bw_limit->value();
	ow->preset_memory[which].iffreq_bw_low = ow->miw_filt_lwr->get_value_as_double();
	ow->preset_memory[which].iffreq_bw_high = ow->miw_filt_upr->get_value_as_double();
	ow->preset_memory[which].iffreq_bw_taps = ow->miw_filt_taps->get_value_as_double();

	ow->preset_memory[which].dev_gain = ow->miw_freq_gain->get_value_as_double();
	
	ow->preset_memory[which].aud_gain = ow->fvs_gain->value();

	ow->preset_memory[which].b_agc = ow->ld_agc->GetColIndex();
//	ow->preset_memory[which].b_agc = b_agc;

	
//ow->preset_memory[which].b_bias_t = 0;
	
	ow->preset_memory[which].direct_sampling = wnd_rtl_graph->ld_direct_sampling->GetColIndex();
	
	ow->preset_memory[which].iq_gain = wnd_rtl_graph->miw_gain_iq->get_value_as_double();
	
	
	ow->preset_memory[which].b_bias_t = wnd_rtl_graph->ld_bias_t->GetColIndex();

	ow->led_preset_memory_tooltip_update( ow->b_led_preset_memory_tooltip_show_tooltips );

	ow->preset_memory[which].i_deemph = wnd_rtl_graph->ld_fm_deemph->GetColIndex();

	ow->preset_memory[which].b_dcblk = ow->ck_user_dc_block_iq->value();
	}
else{
	//recall
	ow->fi_name->value( ow->preset_memory[which].sname.c_str() );
	
	int frq = ow->preset_memory[which].freq_tune;
	ow->miwp_tune->miw->set_value_from_double( frq );
	ow->miw_freq_sub_tune->set_value_from_double( ow->preset_memory[which].freq_sub_tune );

	ow->fi_demodul_mode->value( ow->preset_memory[which].demod_type );
	//ow->demodul_mode = ow->preset_memory[which].demod_type;
	demodul_mode_set( ow->preset_memory[which].demod_type );

	ow->ck_user_dwn_aa->value( ow->preset_memory[which].b_dwn_aa );
	ow->miw_dev_dwnconv_srate->set_value_from_double( ow->preset_memory[which].i_dwn_srate );
	
	ow->ck_if_freq->value( ow->preset_memory[which].b_use_iffreq );
	ow->miw_if_freq->set_value_from_double( ow->preset_memory[which].if_freq );

	ow->ck_bw_limit->value( ow->preset_memory[which].b_bw_limit );
	ow->miw_filt_lwr->set_value_from_double( ow->preset_memory[which].iffreq_bw_low );
	ow->miw_filt_upr->set_value_from_double( ow->preset_memory[which].iffreq_bw_high );
	ow->miw_filt_taps->set_value_from_double( ow->preset_memory[which].iffreq_bw_taps );

	ow->miw_freq_gain->set_value_from_double( ow->preset_memory[which].dev_gain );

	ow->ld_direct_sampling->ChangeCol( ow->preset_memory[which].direct_sampling );

	ow->miw_gain_iq->set_value_from_double( ow->preset_memory[which].iq_gain );

	ow->fvs_gain->value( ow->preset_memory[which].aud_gain );

	ow->ld_agc->ChangeCol( ow->preset_memory[which].b_agc );

	ow->ld_bias_t->ChangeCol( ow->preset_memory[which].b_bias_t );

	ow->ld_fm_deemph->ChangeCol( ow->preset_memory[which].i_deemph );

	ow->ck_user_dc_block_iq->value( ow->preset_memory[which].b_dcblk );	

	ow->freq_listen( 1, 1, 1, 1, "cb_led_preset_memory_combo()" );

	wnd_rtl_graph->led_dwn_srate_memory_turn_off();
	wnd_rtl_graph->led_filter_memory_turn_off();


	}
}









void cb_led_filter_mouse_event( void *obj_in, void *args_in )
{
GCLed *ol = (GCLed*)obj_in;

rtl_graph_wnd *ow = wnd_rtl_graph;

printf( "cb_led_filter_mouse_event() - args_in %d, id %d  last_event %d\n", (int)args_in, ol->id, ol->last_event );

//ow->led_preset_hov_idx = ol->id;
//ow->led_preset_hov_idx2 = -1;
}







void cb_led_filter_memory_combo( Fl_Widget *w, void* v )
{
int which = (intptr_t)v;
string s1;
mystr m1;

GCLed *o;
o =(GCLed *)w;

rtl_graph_wnd *ow = wnd_rtl_graph;

printf( "cb_led_filter_memory_combo() - which %d\n", which );

if( o->right_button )
	{
	//store
	if( pref_ask_preset_overwrite ) 
		{
		if( !preset_ask_overwrite() ) return;
		}
	}



ow->led_filter_memory_turn_off();

int i = o->GetColIndex();
if( i >= 0 )
	{
	i++;
	if( i > 1 ) i = 0;
	o->ChangeCol( i );
	}


if( o->right_button )
	{
	//store
	ow->filter_memory[which].b_bw_limit = ow->ck_bw_limit->value();
	ow->filter_memory[which].iffreq_bw_taps = ow->miw_filt_taps->get_value_as_double();
	ow->filter_memory[which].iffreq_bw_low = ow->miw_filt_lwr->get_value_as_double();
	ow->filter_memory[which].iffreq_bw_high = ow->miw_filt_upr->get_value_as_double();
	}
else{
	//recall
	ow->ck_bw_limit->value( ow->filter_memory[which].b_bw_limit );
	ow->miw_filt_lwr->set_value_from_double( ow->filter_memory[which].iffreq_bw_low );
	ow->miw_filt_upr->set_value_from_double( ow->filter_memory[which].iffreq_bw_high );
	ow->miw_filt_taps->set_value_from_double( ow->filter_memory[which].iffreq_bw_taps );

	wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_led_filter_memory_combo()" );
	
	filter_adjust_fir_bw_lower_upper();
	}

}





int aa_dwncnv_fc = 4000;
int aa_dwncnv_fc_user = 400000;




void cb_led_aa_dwncnv_fc( Fl_Widget *w, void* v )
{
int which = (intptr_t)v;
string s1, s2;
mystr m1;

GCLed *o;
o =(GCLed *)w;

rtl_graph_wnd *ow = wnd_rtl_graph;

printf( "cb_led_aa_dwncnv_fc() - which %d\n", which );


if( o->middle_button ) 
	{
	strpf( s2, "%d", aa_dwncnv_fc_user );
	char *sz = fl_input( "Enter a cutoff freq to use for antialias filter (between 2000-->500000): ", s2.c_str() );

	int	iv;

	if( sz != 0 )
		{
		sscanf( sz, "%d", &iv );
		if( iv < 2000 ) iv = 2000;
		if( iv > 500000 ) iv = 500000;
		
		aa_dwncnv_fc_user = iv;
		aa_dwncnv_fc = iv;

		strpf( s1, "%dK", aa_dwncnv_fc_user/1000  );
		
		o->copy_label( s1.c_str() );
		o->ChangeCol( 12 );

		g_dev_bw_last = 0;												//trigger a filter rebuild
		}
	}

int dir = 1;
if( o->right_button ) dir = -1;

if(  o->left_button ||  o->right_button )
	{
	int i = o->GetColIndex();
	if( i >= 0 )
		{
		i += dir;
		if( i < 0 ) i = 12;
		if( i > 12 ) i = 0;
		o->ChangeCol( i );
		
		if( i == 0 )
			{
			aa_dwncnv_fc = 3000;
			o->label("3K");
			}

		if( i == 1 )
			{
			aa_dwncnv_fc = 4000;
			o->label("4K");
			}

		if( i == 2 )
			{
			aa_dwncnv_fc = 5000;
			o->label("5K");
			}

		if( i == 3 )
			{
			aa_dwncnv_fc = 8000;
			o->label("8K");
			}

		if( i == 4 )
			{
			aa_dwncnv_fc = 10000;
			o->label("10K");
			}

		if( i == 5 )
			{
			aa_dwncnv_fc = 15000;
			o->label("15K");
			}
		
		if( i == 6 )
			{
			aa_dwncnv_fc = 20000;
			o->label("20K");
			}

		if( i == 7 )
			{
			aa_dwncnv_fc = 30000;
			o->label("30K");
			}

		if( i == 8 )
			{
			aa_dwncnv_fc = 50000;
			o->label("50K");
			}


		if( i == 9 )
			{
			aa_dwncnv_fc = 100000;
			o->label("100K");
			}

		if( i == 10 )
			{
			aa_dwncnv_fc = 200000;
			o->label("200K");
			}

		if( i == 11 )
			{
			aa_dwncnv_fc = 300000;
			o->label("300K");
			}

		if( i == 12 )
			{
			strpf( s1, "%dK", aa_dwncnv_fc_user/1000  );
			aa_dwncnv_fc = aa_dwncnv_fc_user;
			o->copy_label( s1.c_str() );
			}

		g_dev_bw_last = 0;												//trigger a filter rebuild
		}
	}

}




void cb_led_dwn_srate_memory_combo( Fl_Widget *w, void* v )
{
int which = (intptr_t)v;
string s1;
mystr m1;

GCLed *o;
o =(GCLed *)w;

rtl_graph_wnd *ow = wnd_rtl_graph;

printf( "cb_led_dwn_srate_memory_combo() - which %d\n", which );


if( o->right_button )
	{
	if( pref_ask_preset_overwrite ) 
		{
		if( !preset_ask_overwrite() ) return;
		}
	}

ow->led_dwn_srate_memory_turn_off();

int i = o->GetColIndex();
if( i >= 0 )
	{
	i++;
	if( i > 1 ) i = 0;
	o->ChangeCol( i );
	}


if( o->right_button )
	{
	//store
	ow->dwn_srate_memory[which].dwn_srate = ow->miw_dev_dwnconv_srate->get_value_as_double();
	}
else{
	//recall
	int  sr = ow->dwn_srate_memory[which].dwn_srate;

	if( sr < cn_downsample_srate_min ) sr = cn_downsample_srate_min;
	if( sr > cn_downsample_srate_max ) sr = cn_downsample_srate_max;

	ow->miw_dev_dwnconv_srate->set_value_from_double( sr );
	

	unsigned int dwn_srate_in = sr;
	bool b_set_dwnsrate = 1;
	bool b_adj_gui_ctrl = 1;
	dnwsrate_nearest_factor( dwn_srate_in, b_set_dwnsrate, b_adj_gui_ctrl );

//	downsample_srate_pending = sr;
//	b_need_dwn_srate_change = 1;										//trigger dwn srate update

	wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "cb_led_dwn_srate_memory_combo()" );
	}

}















void cb_led_preset_memory_combo2( Fl_Widget *w, void* v )
{
int which = (intptr_t)v;
string s1;
mystr m1;

wnd_rtl_graph->led_freq_memory_turn_off();
wnd_rtl_graph->led_preset_memory_turn_off();
wnd_rtl_graph->led_preset_memory_turn_off2();

GCLed *o;
o =(GCLed *)w;

rtl_graph_wnd *ow = wnd_rtl_graph;

printf( "cb_led_preset_memory_combo2() - which %d, right_button %d\n", which, o->right_button );

//getchar();

if( o->right_button )
	{
	//store
	if( pref_ask_preset_overwrite ) 
		{
		if( !preset_ask_overwrite() ) return;
		}
	}



int i = o->GetColIndex();
if( i >= 0 )
	{
	i++;
	if( i > 1 ) i = 0;
	o->ChangeCol( i );
	}

if( o->right_button )
	{
	//store
	bool b_use_freq = 0;
	
	s1 = ow->fi_name->value();
	if( s1.length() == 0 ) b_use_freq = 1;
	if( s1.compare("???") == 0 ) b_use_freq = 1;
	
	if( b_use_freq )
		{
		string snum, sunits, scombined;
		int fractional_digits = 4;

		m1.make_engineering_str( snum, sunits, scombined, fractional_digits,  ow->miwp_tune->miw->get_value_as_double(), "", "Hz" );

		strpf( s1, "%s", scombined.c_str() );
		}




	ow->preset_memory2[which].sname = s1;
	ow->preset_memory2[which].freq_tune = ow->miwp_tune->miw->get_value_as_double();
	ow->preset_memory2[which].freq_sub_tune = ow->miw_freq_sub_tune->get_value_as_double();
	ow->preset_memory2[which].demod_type = (en_demodulator_type_tag)ow->fi_demodul_mode->value();


	ow->preset_memory2[which].b_dwn_aa = ow->ck_user_dwn_aa->value();
	ow->preset_memory2[which].i_dwn_srate = ow->miw_dev_dwnconv_srate->get_value_as_double();
	
	ow->preset_memory2[which].b_use_iffreq = ow->ck_if_freq->value();
	ow->preset_memory2[which].if_freq = ow->miw_if_freq->get_value_as_double();

	ow->preset_memory2[which].b_bw_limit = ow->ck_bw_limit->value();
	ow->preset_memory2[which].iffreq_bw_low = ow->miw_filt_lwr->get_value_as_double();
	ow->preset_memory2[which].iffreq_bw_high = ow->miw_filt_upr->get_value_as_double();
	ow->preset_memory2[which].iffreq_bw_taps = ow->miw_filt_taps->get_value_as_double();

	ow->preset_memory2[which].dev_gain = ow->miw_freq_gain->get_value_as_double();
	
	ow->preset_memory2[which].aud_gain = ow->fvs_gain->value();
	ow->preset_memory2[which].b_agc = ow->ld_agc->GetColIndex();

	
//ow->preset_memory[which].b_bias_t = 0;
	
	ow->preset_memory2[which].direct_sampling = wnd_rtl_graph->ld_direct_sampling->GetColIndex();
	
	ow->preset_memory2[which].iq_gain = wnd_rtl_graph->miw_gain_iq->get_value_as_double();
	
	ow->preset_memory2[which].b_bias_t = wnd_rtl_graph->ld_bias_t->GetColIndex();

	ow->led_preset_memory_tooltip_update2( ow->b_led_preset_memory_tooltip_show_tooltips );
	ow->led_preset_memory_label_update2();

	ow->preset_memory2[which].i_deemph = wnd_rtl_graph->ld_fm_deemph->GetColIndex();
	
	ow->preset_memory2[which].b_dcblk = ow->ck_user_dc_block_iq->value();

	}
else{
	//recall
	ow->fi_name->value( ow->preset_memory2[which].sname.c_str() );
	
	int frq = ow->preset_memory2[which].freq_tune;
	ow->miwp_tune->miw->set_value_from_double( frq );

	ow->miwp_tune->miw->set_value_from_double( frq );

	ow->miw_freq_sub_tune->set_value_from_double( ow->preset_memory2[which].freq_sub_tune );

	ow->fi_demodul_mode->value( ow->preset_memory2[which].demod_type );

//	ow->demodul_mode = ow->preset_memory2[which].demod_type;	
	demodul_mode_set( ow->preset_memory2[which].demod_type );
	 
	ow->ck_user_dwn_aa->value( ow->preset_memory2[which].b_dwn_aa );
	ow->miw_dev_dwnconv_srate->set_value_from_double( ow->preset_memory2[which].i_dwn_srate );

	ow->ck_if_freq->value( ow->preset_memory2[which].b_use_iffreq );
	ow->miw_if_freq->set_value_from_double( ow->preset_memory2[which].if_freq );

	ow->ck_bw_limit->value( ow->preset_memory2[which].b_bw_limit );
	ow->miw_filt_lwr->set_value_from_double( ow->preset_memory2[which].iffreq_bw_low );
	ow->miw_filt_upr->set_value_from_double( ow->preset_memory2[which].iffreq_bw_high );
	ow->miw_filt_taps->set_value_from_double( ow->preset_memory2[which].iffreq_bw_taps );

	ow->miw_freq_gain->set_value_from_double( ow->preset_memory2[which].dev_gain );

	ow->ld_agc->ChangeCol( ow->preset_memory2[which].b_agc );

	ow->ld_direct_sampling->ChangeCol( ow->preset_memory2[which].direct_sampling );

	ow->miw_gain_iq->set_value_from_double( ow->preset_memory2[which].iq_gain );

	ow->fvs_gain->value( ow->preset_memory2[which].aud_gain );

	ow->ld_agc->ChangeCol( ow->preset_memory2[which].b_agc );

	ow->ld_bias_t->ChangeCol( ow->preset_memory2[which].b_bias_t );

	ow->ld_fm_deemph->ChangeCol( ow->preset_memory2[which].i_deemph );


	ow->ck_user_dc_block_iq->value( ow->preset_memory2[which].b_dcblk );	
		
	ow->freq_listen( 1, 1, 1, 1, "cb_led_preset_memory_combo2()" );

	wnd_rtl_graph->led_dwn_srate_memory_turn_off();
	wnd_rtl_graph->led_filter_memory_turn_off();
	}
}












void cb_led_preset_mouse_event2( void *obj_in, void *args_in )
{
GCLed *ol = (GCLed*)obj_in;

rtl_graph_wnd *ow = wnd_rtl_graph;

//printf( "cb_led_preset_mouse_event2() - args_in %d, id %d  last_event %d\n", (int)args_in, ol->id, ol->last_event );

ow->led_freq_mem_hov_idx = -1;
ow->led_preset_hov_idx = -1;
ow->led_preset_hov_idx2 = ol->id;
}




//b_absolute_time = 1,  changes play pos to 'time'
//b_absolute_time = 0,  changes play pos using 'time' as delta

bool rec_play_change_play_position( bool b_absolute_time, int itime )
{


if( rec_play_iq_state == 13 )
	{
	if( ( fp_rec != 0 ) && ( b_rec_play_start_pos_req == 0 ) )	//if 'b_rec_play_start_pos_req' is already set, skip further attemps below till it's processed by 'tick(()'
		{
		
		int dt = itime;		
		
		off_t position = ftello( fp_rec );

		if( b_absolute_time )
			{
			if( dt < 0 ) return 0;
			if( dt >= (play_mode_end_secs - 5 ) ) return 0;					//make sure at least a small amount of time is avail to play
			}
		
		//use delta play pos offset
		int64_t new_rd = (int64_t)position  +  dt * (g_dev_bw*sizeof(float)*2 );	//*2 as the data has I/Q pairs;

		if( b_absolute_time ) new_rd = dt * (g_dev_bw*sizeof(float)*2 );			//*2 as the data has I/Q pairs;

		if( new_rd >= play_filesize )
			{
			new_rd -= play_filesize;	
			}

		if( new_rd < 0 )
			{
			new_rd += play_filesize;	
			}

		rec_play_start_pos = new_rd;									//load new play point
		rec_play_iq_set_state( 4 );										//trigger new play from spec file point
		return 1;
		}
	}
return 0;
}









void cb_led_preset_mouse_event( void *obj_in, void *args_in )
{
GCLed *ol = (GCLed*)obj_in;

rtl_graph_wnd *ow = wnd_rtl_graph;

//printf( "cb_led_preset_mouse_event() - args_in %d, id %d  last_event %d\n", (int)args_in, ol->id, ol->last_event );

ow->led_freq_mem_hov_idx = -1;
ow->led_preset_hov_idx = ol->id;
ow->led_preset_hov_idx2 = -1;
}















void cb_ld_rec_play_synth_event( Fl_Widget *w, void* v )
{
int which = (intptr_t)v;

GCLed *o;
o =(GCLed *)w;

//printf( "cb_ld_rec_play_synth_event() -  o->last_event %d\n", o->last_event );

int dir = o->mousewheel - o->mousewheel_last;
printf( "cb_ld_rec_play_synth_event() -  o->mousewheel %d %d  dir %d\n", o->mousewheel, o->mousewheel_last, dir );

if ( o->last_event != FL_MOUSEWHEEL ) return;


if( rec_play_iq_state == 13 )
	{
	if( dir != 0 ) 															//mousewheel changed ?
		{
		if( ( fp_rec != 0 ) && ( b_rec_play_start_pos_req == 0 ) )	//if 'b_rec_play_start_pos_req' is already set, skip further attemps below till it's processed by 'tick(()'
			{

			int dt = 15*dir;														//5 sec play time change
			
			bool b_absolute_time = 0;
			rec_play_change_play_position( b_absolute_time, dt );
			
/*			
			off_t position = ftello( fp_rec );

			int64_t new_rd = (int64_t)position  +  dt * (g_dev_bw*sizeof(float)*2 );		//*2 as the data has I/Q pairs;


			if( new_rd >= play_filesize )
				{
				new_rd -= play_filesize;	
				}

			if( new_rd < 0 )
				{
				new_rd += play_filesize;	
				}

			rec_play_start_pos = new_rd;								//load new play point
			rec_play_iq_set_state( 4 );									//trigger new play from spec file point
*/


//			fpos_t pos;
//			fgetpos( fp_rec, &pos );

//			off_t position = ftello( fp_rec );
			
//printf( "cb_ld_rec_play_synth_event() -  fseek to %" PRIi64 " siz %" PRIu64 "\n", new_rd, (uint64_t)position );
//			fseeko( fp_rec, new_rd, SEEK_SET );

//			b_rec_play_flush = 1;										//trig an 'fread()' in 'rec_play_iq_state_process()'
			}
		}

	
//	rec_change_play_pos_to = new_rd;
	
//	b_rec_change_play_pos = 1;											//flag to change in 'demod_iso()'
	}
}







void cb_ld_aud_gain_plus_minus_db( Fl_Widget *w, void* v )
{
int which = (intptr_t)v;

GCLed *o;
o =(GCLed *)w;


int i = o->GetColIndex();
if( i >= 0 )
	{
	i++;
	if( i > 1 ) i = 0;
	}

wnd_rtl_graph->ld_aud_plus_db->ChangeCol( 0 );
wnd_rtl_graph->ld_aud_minus_db->ChangeCol( 0 );

o->ChangeCol( i );


int iv = wnd_rtl_graph->ld_aud_plus_db->GetColIndex();
if( iv == 1 ) 
	{
	aud_gain_plus_minus_db = 1;
	}
else{
	iv = wnd_rtl_graph->ld_aud_minus_db->GetColIndex();
	if( iv == 1 ) 
		{
		aud_gain_plus_minus_db = 2;
		}
	else{
		aud_gain_plus_minus_db = 0;
		}
	}


printf( "cb_ld_aud_gain_plus_minus_db() -  aud_gain_plus_minus_db %d\n", aud_gain_plus_minus_db );

}







void cb_ld_rec_play_synth( Fl_Widget *w, void* v )
{
int which = (intptr_t)v;
printf( "cb_ld_rec_play_synth() - which %d\n", which );

GCLed *o;
o =(GCLed *)w;
}





void cb_led_combo( Fl_Widget *w, void* v )
{
int which = (intptr_t)v;
printf( "cb_led_combo() - which %d\n", which );

GCLed *o;
o =(GCLed *)w;

int i = o->GetColIndex();
if( i >= 0 )
	{
	i++;
	if( i > 1 ) i = 0;
	o->ChangeCol( i );
	}

if( which == 0 )					//direct smplng
	{
	g_dev_direct_sampling++;
	if( g_dev_direct_sampling > 2 ) g_dev_direct_sampling = 0;
//	rtl.set_direct_sampling( g_dev_direct_sampling );
//	wnd_rtl_graph->set_dev_direct_sampling( g_dev_direct_sampling );
	
	o->ChangeCol( g_dev_direct_sampling );
	
	wnd_rtl_graph->freq_listen( 1, 0, 0, 1, "cb_led_combo()" );									//need this to set dev gain after a change to direct sampling state 

	}


if( which == 1 )					//offs tuning
	{
	g_dev_offset_tuning = i;
	rtl.set_offset_tuning( g_dev_offset_tuning );
	}


if( which == 2 )					//carrier max tune
	{
	b_carrier_max_tune = i;
	}

if( which == 3 )					//bias_t
	{
	g_dev_bias_t = !g_dev_bias_t;
	wnd_rtl_graph->set_bias_t( g_dev_bias_t );
//	rtl.set_bias_tee( g_dev_bias_t );
//	o->ChangeCol( g_dev_bias_t );
	}

}















void cb_wnd_fav(Fl_Widget*, void* v)
{
cl_fav_text_input* w = (cl_fav_text_input*)v;

b_wnd_fav_is_open = 0;
wnd_fav->hide();
}












//make device adc srates based on multiples of audio srate, helps synchronise buffer wr/rd access, reduces audio stutter
void rtl_graph_wnd::make_dev_bwidth_list()
{
string s1;



int max_factor = cn_rtl_e4000_sample_bandwith_max / aud_op_srate;

int max_freq = max_factor * aud_op_srate;

mb_dev_bwidth->clear();
for( int i = 0; i < cn_dev_bwidth_selections_max; i++ )
	{
	int freq = max_freq - (i*2)*aud_op_srate;
	if( freq < cn_rtl_e4000_sample_bandwith_min ) freq = cn_rtl_e4000_sample_bandwith_min;
	st_dev_bwidth[i].freq = freq;
	
	strpf( s1, "%d", st_dev_bwidth[i].freq );
	mb_dev_bwidth->add( s1.c_str(),  FL_CTRL + i+0x30, cb_mb_dev_bwidth );
	}
}










// return true if s1.freq_actual comes before s2.freq_actual
bool sort_st_freq_pin_compare( const st_freq_pin &s1, const st_freq_pin &s2 ) 
{
if( s1.freq_actual < s2.freq_actual ) return 1;			//return 1 if '<'		!! important to get this right for 'stable_sort()'

return 0;												//return 0 if '>='
}






//sort pins by freq
void sort_st_freq_pin( vector<st_freq_pin> &vp )
{
std::stable_sort( vp.begin(), vp.end(), sort_st_freq_pin_compare );
}






void rtl_graph_wnd::pin_deselect_all()
{
for( int i = 0; i < vpin.size(); i++ )
	{
	vpin[i].sel = 0;
	}
}







//add a pin to a freq does not use 'px', 'px' is determined by 'freq_actual'
bool rtl_graph_wnd::pin_add( int px, int py, int freq_center, int freq_sub )
{
bool vb = 0;
printf( "------------- rtl_graph_wnd::pin_add() -------------\n" );	

pin_deselect_all();

st_freq_pin op;

op.px = px;									//a pin to a freq does not use 'px', 'px' is determined by 'freq_actual'
op.py = py;
op.freq_actual = freq_center + freq_sub;
op.freq_tune = freq_center;
op.freq_sub_tune = freq_sub;
op.sel = 1;

vpin.push_back( op );

sort_st_freq_pin( vpin );

if(vb)
	{
	for( int i = 0; i < vpin.size(); i++ )
		{
		printf( "vpin[%02d].freq_actual: %d\n", i, (int)vpin[i].freq_actual );	
		}
	printf( "--------------------------------\n\n" );	
	}
	


return 1;
}











//del a pin
bool rtl_graph_wnd::pin_delete( unsigned int idx )
{
if( idx >= vpin.size() ) return 0;

vpin.erase( vpin.begin() + idx );
gph0_obj_hover_idx = -1;
gph0_obj_sel_idx = -1;

return 1;
}








int rtl_graph_wnd::pin_selected()
{
int idx = -1;

//if( vpin.size() == 0 ) return -1;


if( gph0_obj_sel_idx != -1 ) 
	{
	if( vgph_obj[ gph0_obj_sel_idx ].type == en_got_freq_pin )
		{
		idx = vgph_obj[ gph0_obj_sel_idx ].pin_idx;
		}

	}

return idx;
}



//select next or prev pin
int rtl_graph_wnd::pin_select_inc_dec( int dir, bool adj_freq )
{
bool vb = 1;

if(vb)printf( "rtl_graph_wnd::pin_select_inc_dec() - dir %d\n", dir );	

int idx = -1;

if( vpin.size() == 0 ) return -1;

if( vpin.size() == 1 ) 
	{
	idx = 0;
	}
else{
	if( gph0_obj_sel_idx == -1 ) 
		{
		idx = 0;
		}
	else{
		idx = vgph_obj[ gph0_obj_sel_idx ].pin_idx;

//	printf( "rtl_graph_wnd::pin_select()0     %d\n", idx );	

		idx += dir;
		
		if( idx >= vpin.size() ) idx -= vpin.size();
		if( idx < 0 ) idx = vpin.size() - 1;
		}
	}

pin_select_by_idx( idx, adj_freq, 0 );
//printf( "rtl_graph_wnd::pin_select()1     %d\n", idx );	

//gph0_obj_sel_idx = idx;
return idx;
}








bool rtl_graph_wnd::pin_select_by_idx( unsigned int idx, bool adj_freq, bool cntr_onbrd_zero_sub_tune )
{
bool vb = 1;

if(vb)printf( "rtl_graph_wnd::pin_select_by_idx() - idx %d\n", idx );	


if( idx >= vpin.size() ) return 0;

pin_deselect_all();
vpin[idx].sel = 1;

if( cntr_onbrd_zero_sub_tune )
	{
	vpin[idx].freq_tune = vpin[idx].freq_tune + vpin[idx].freq_sub_tune;
	vpin[idx].freq_sub_tune = 0;
	}

if( adj_freq )
	{
	wnd_rtl_graph->miwp_tune->miw->set_value_from_double( vpin[idx].freq_tune );
	wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( vpin[idx].freq_sub_tune );
	wnd_rtl_graph->freq_listen( 0, 0, 0, 0, "pin_select_by_idx()" );
	}
return 1;

/*
for( int i = 0; i < vpin.size(); i++ )
	{
	if( vgph_obj[i].pin_idx == idx )
		{

		gph0_obj_sel_idx = i;
		
		if( adj_freq )
			{
				
				//		wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( vgph_obj[ gph0_obj_hover_idx ].freq_actual - g_freq_tune );
//		wnd_rtl_graph->freq_listen( 0, 0, 0, 0, "cb_graph_left_click_release_cb() 1" );

//			wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( vgph_obj[ gph0_obj_sel_idx ].freq_actual - g_freq_tune );
			wnd_rtl_graph->miwp_tune->miw->set_value_from_double( vgph_obj[ gph0_obj_sel_idx ].freq_tune );
			wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( vgph_obj[ gph0_obj_sel_idx ].freq_sub_tune );
			wnd_rtl_graph->freq_listen( 0, 0, 0, 0, "pin_select_by_idx()" );
			}

		return 1;
		}
	}



for( int i = 0; i < vgph_obj.size(); i++ )
	{
	if( vgph_obj[i].pin_idx == idx )
		{
//		pin_deselect_all();
//		vpin[i].sel = 1;

		gph0_obj_sel_idx = i;
		
		if( adj_freq )
			{
				
				//		wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( vgph_obj[ gph0_obj_hover_idx ].freq_actual - g_freq_tune );
//		wnd_rtl_graph->freq_listen( 0, 0, 0, 0, "cb_graph_left_click_release_cb() 1" );

//			wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( vgph_obj[ gph0_obj_sel_idx ].freq_actual - g_freq_tune );
			wnd_rtl_graph->miwp_tune->miw->set_value_from_double( vgph_obj[ gph0_obj_sel_idx ].freq_tune );
			wnd_rtl_graph->miw_freq_sub_tune->set_value_from_double( vgph_obj[ gph0_obj_sel_idx ].freq_sub_tune );
			wnd_rtl_graph->freq_listen( 0, 0, 0, 0, "pin_select_by_idx()" );
			}

		return 1;
		}
	}
*/
}










//user interactive objs such as pins or rect buttons
void rtl_graph_wnd::gph0_obj_build()
{
vgph_obj.clear();

st_gph_obj_tag op;

op.pin_idx = -1;

//---																	//this is the hover obj rect, initially is not visible
op.flags = 0;
op.type = en_got_sel_rect;

op.stext = "hover";
op.shape = en_dobt_rect;
//	op.radius = 5;
op.freq_actual = 0;
op.font = 5;
op.font_size = 9;

op.line_thick = 1;
op.line_style = en_mls_dot;
op.justify = en_tj_none;

op.x = 50;
op.y = 0;

op.offsx = -2;
op.offsy = -2;

op.descent = 50;

op.wid = 7+2;
op.hei = 7+2;
op.rr = 220;
op.gg = 220;
op.bb = 220;


vgph_obj.push_back( op );
//---



int ibut_demod_x = wnd_rtl_graph->gph0->w() - 35;
int ibut_demod_y = 35;


//---
op.flags = en_gflg_vis | en_gflg_selectable;
op.type = en_got_demod_btn;
op.stext = "FMST";
op.shape = en_dobt_text;
op.int0 = en_dmt_fm_stereo;

op.freq_actual = 0;

op.line_thick = 1;
op.line_style = en_mls_solid;
op.justify = en_tj_vert_top;// | en_tj_horiz_center;// | en_tj_vert_center;

op.font = 4;
op.font_size = 10;

op.x = ibut_demod_x;
op.y = ibut_demod_y;

op.offsx = 0;
op.offsy = 0;

op.descent = 0;

op.wid = 25;
op.hei = 12;

op.rr = 180;
op.gg = 180;
op.bb = 255;

if( wnd_rtl_graph->demodul_mode == en_dmt_fm_stereo )
	{
	op.rr = 100;
	op.gg = 255;
	op.bb = 100;
	}


vgph_obj.push_back( op );
//---


//---
op.flags = en_gflg_vis | en_gflg_selectable;
op.type = en_got_demod_btn;
op.stext = "WFM";
op.shape = en_dobt_text;

op.int0 = en_dmt_wfm;

op.freq_actual = 0;

op.line_thick = 1;
op.line_style = en_mls_solid;
op.justify = en_tj_vert_top;// | en_tj_horiz_center;// | en_tj_vert_center;

op.font = 4;
op.font_size = 10;

op.x = ibut_demod_x;
op.y = ibut_demod_y + 12;

op.offsx = 0;
op.offsy = 0;

op.descent = 0;

op.wid = 25;
op.hei = 12;

op.rr = 180;
op.gg = 180;
op.bb = 255;

if( wnd_rtl_graph->demodul_mode == en_dmt_wfm )
	{
	op.rr = 100;
	op.gg = 255;
	op.bb = 100;
	}

vgph_obj.push_back( op );
//---



//---
op.flags = en_gflg_vis | en_gflg_selectable;
op.type = en_got_demod_btn;
op.stext = "FM";
op.shape = en_dobt_text;

op.int0 = en_dmt_fm;

op.freq_actual = 0;

op.line_thick = 1;
op.line_style = en_mls_solid;
op.justify = en_tj_vert_top;// | en_tj_horiz_center;// | en_tj_vert_center;

op.font = 4;
op.font_size = 10;

op.x = ibut_demod_x;
op.y = ibut_demod_y + 24;

op.offsx = 0;
op.offsy = 0;

op.descent = 0;

op.wid = 25;
op.hei = 12;

op.rr = 180;
op.gg = 180;
op.bb = 255;

if( wnd_rtl_graph->demodul_mode == en_dmt_fm )
	{
	op.rr = 100;
	op.gg = 255;
	op.bb = 100;
	}

vgph_obj.push_back( op );
//---




//---
op.flags = en_gflg_vis | en_gflg_selectable;
op.type = en_got_demod_btn;
op.stext = "AM";
op.shape = en_dobt_text;

op.int0 = en_dmt_am;


op.freq_actual = 0;

op.line_thick = 1;
op.line_style = en_mls_solid;
op.justify = en_tj_vert_top;// | en_tj_horiz_center;// | en_tj_vert_center;

op.font = 4;
op.font_size = 10;

op.x = ibut_demod_x;
op.y = ibut_demod_y + 36;

op.offsx = 0;
op.offsy = 0;

op.descent = 0;

op.wid = 15;
op.hei = 12;

op.rr = 180;
op.gg = 180;
op.bb = 255;

if( wnd_rtl_graph->demodul_mode == en_dmt_am )
	{
	op.rr = 100;
	op.gg = 255;
	op.bb = 100;
	}


vgph_obj.push_back( op );
//---




//---
op.flags = en_gflg_vis | en_gflg_selectable;
op.type = en_got_demod_btn;
op.stext = "SSB";
op.shape = en_dobt_text;

op.int0 = en_dmt_ssb;

op.freq_actual = 0;

op.line_thick = 1;
op.line_style = en_mls_solid;
op.justify = en_tj_vert_top;// | en_tj_horiz_center;// | en_tj_vert_center;

op.font = 4;
op.font_size = 10;

op.x = ibut_demod_x;
op.y = ibut_demod_y + 48;

op.offsx = 0;
op.offsy = 0;

op.descent = 0;

op.wid = 20;
op.hei = 12;

op.rr = 180;
op.gg = 180;
op.bb = 255;

if( wnd_rtl_graph->demodul_mode == en_dmt_ssb )
	{
	op.rr = 100;
	op.gg = 255;
	op.bb = 100;
	}


vgph_obj.push_back( op );
//---



gph0_obj_sel_idx = -1;

//add pinned freqs to obj list
for( int i = 0; i < vpin.size(); i++ )
	{
	//---
	op.flags = en_gflg_vis | en_gflg_selectable | en_gflg_follows_freq;
	op.type = en_got_freq_pin;
	op.stext = "pin";
	op.shape = en_dobt_pie;

	op.int0 = i;														//store index to allow pin to be deleted
	op.pin_idx = i;
	
	op.freq_actual = vpin[i].freq_actual;								//not used
	op.freq_tune = vpin[i].freq_tune;
	op.freq_sub_tune = vpin[i].freq_sub_tune;

	op.line_thick = 1;
	op.line_style = en_mls_solid;
	op.justify = en_tj_none;

	op.font = 5;
	op.font_size = 9;

	op.x = vpin[i].px;
	op.y = vpin[i].py;

	if( vpin[i].sel ) gph0_obj_sel_idx = vgph_obj.size();				//sel?
	
	
	if( op.flags | en_gflg_follows_freq )								//adj x pos
		{
		double x1, y1, x2, y2, x3, y3, x4, y4;
		
		x1 = op.freq_actual;
		
		wnd_rtl_graph->gph0->get_plot_values_zero_offsets( 0, x1, y1, x2, y2, x3, y3, x4, y4, 1, 1 );

		op.x = x1;
		
		if( op.x < 0 ) op.flags ^= en_gflg_vis;
		if( op.x > wnd_rtl_graph->gph0->w() ) op.flags ^= en_gflg_vis;
		}


	op.offsx = 0;
	op.offsy = 0;

	op.descent = 0;


	op.wid = 7;
	op.hei = 7;

	op.rr = 230;
	op.gg = 220;
	op.bb = 100;


	vgph_obj.push_back( op );
	//---
	}






//--- this MUST be sit at the LAST entry index as it uses an one of the earlier entries to allow this sel rect to encompass the earlier selected obj	------		
if( gph0_obj_sel_idx >= 0 )
	{
	st_gph_obj_tag og_sel = vgph_obj[ gph0_obj_sel_idx ];
	op.pin_idx = -1;

	op.flags = en_gflg_vis;
	op.type = en_got_sel_rect;

	op.stext = "sel";
	op.shape = en_dobt_rect;

	op.freq_actual = 0;
	op.font = 5;
	op.font_size = 9;

	op.line_thick = 1;
	op.line_style = en_mls_solid;
	op.justify = en_tj_none;

	op.x = og_sel.x;
	op.y = og_sel.y;

	op.offsx = og_sel.offsx-1;
	op.offsy = og_sel.offsy-1;

	op.descent = og_sel.descent;

	op.wid = og_sel.wid+1;
	op.hei = og_sel.hei+1;

	op.rr = 255;
	op.gg = 100;
	op.bb = 100;

	vgph_obj.push_back( op );
	}
//------------------------



}











//calc coords of each grp0 obj
void rtl_graph_wnd::gph0_obj_calc_coords()
{

for( int i = 0; i < vgph_obj.size(); i++ )
	{
	st_gph_obj_tag op = vgph_obj[i];

	int x1 = op.x + op.offsx;
	int y1 = op.y + op.offsy + op.descent;

	int x2 = x1 + op.wid;
	int y2 = y1 + op.hei;

	op.px1 = x1;
	op.py1 = y1;
	op.px2 = x2;
	op.py2 = y2;
	
	vgph_obj[i] = op;
	}
}







//find index of obj the mouse hovers over,
//need to call 'gph0_obj_build()'  and  'gph0_obj_calc_coords()' first,  see also 'update_gph0_user_obj()'  as it relies on corrd pixel values it produces,
//DOES NOT check index 0, this is the selection rectangle obj
int rtl_graph_wnd::gph0_obj_find_hover()
{
gph0_obj_hover_idx = -1;

int mx, my;
wnd_rtl_graph->gph0->get_mouse_pixel_position_on_background( mx, my );

for( int i = 0; i < vgph_obj.size(); i++ )
	{
	if( i == 0 ) continue;												//this is the selection rectangle obj, don't find its index

	st_gph_obj_tag og = vgph_obj[i];
	if( !(og.flags & en_gflg_vis) ) continue;	


//printf( "update_gph0_user_obj() - mx %d %d   op.px1 %d %d\n", mx, my, op.px1, op.px2 );
	if( ( mx >= og.px1 ) && ( mx <= og.px2) )							//mouse within obj ?
		{
		if( ( my >= og.py1 ) && ( my <= og.py2) )						//mouse within obj ?
			{
			gph0_obj_hover_idx = i;
//			printf("gph0_obj_find_hover() - gph0_obj_hover_idx %d\n", gph0_obj_hover_idx);
			break;
			}
		}
	}



//move/resize hover rect obj to encompass hover obj
if( gph0_obj_hover_idx >= 1 )											//this can't be index zero
	{
	st_gph_obj_tag og = vgph_obj[gph0_obj_hover_idx];
	if( og.flags & en_gflg_selectable )
		{
		st_gph_obj_tag og_sel = vgph_obj[0];

		og_sel.flags = og.flags |= en_gflg_vis;							//show the sel rect
	
		og_sel.x = og.x - 2;											//move/resize sel rect obj to encompass hover obj
		og_sel.y = og.y - 2;
		og_sel.wid = og.wid + 3;
		og_sel.hei = og.hei + 3;

		og_sel.offsx = og.offsx;
		og_sel.offsy = og.offsy;
		og_sel.descent = og.descent;

		int x1 = og_sel.x + og_sel.offsx;
		int y1 = og_sel.y + og_sel.offsy + og_sel.descent;

		int x2 = x1 + og_sel.wid;
		int y2 = y1 + og_sel.hei;

		og_sel.px1 = x1;
		og_sel.py1 = y1;
		og_sel.px2 = x2;
		og_sel.py2 = y2;

		vgph_obj[0] = og_sel;
		}
	}




return gph0_obj_hover_idx;
}











double d_tune_needle_x = 0;





void rtl_graph_wnd::update_gph0_user_obj()
{
string s1;
mystr m1;


wnd_rtl_graph->gph0_make_sel_sample_text();								//build selected sample details string



int offx, offy;
int wid, hei;
gph0->get_background_offsxy( offx, offy );
gph0->get_background_dimensions( wid, hei );

int gph0_sel_trc = 0;
bool gph0_selbox_idx1 = 0;
bool gph0_selbox_idx2 = 0;


gph0->vdrwobj.clear();

st_mgraph_draw_obj_tag mdo;


//--- show carriers detected
mdo.type = (en_mgraph_draw_obj_type)en_dobt_polygon;
mdo.visible = 1;
mdo.draw_ordering = 2;				//draw before graticle, 0 = before grid,  1 = after graticle but before traces,   2 = after traces

mdo.use_pos_x = 0;					//if not set to -1: then use this trace id to get a specific x position
mdo.use_scale_x = 0;				//if not set to -1: then use this trace id to get a specific x scale

mdo.use_pos_y = 0;                 //if not set to -1: then use this trace id to get a specific y position
mdo.use_scale_y = 0;               //if not set to -1: then use this trace id to get a specific y scale

mdo.clip_left = 0;					//same as background
mdo.clip_right = 0;
mdo.clip_top = 0;
mdo.clip_bottom = 0;

//strpf( s1, "press h, x:%f y:%.2f", 1.234, 5.678 );

mdo.arc1 = 0;
mdo.arc2 = 0;
mdo.stext = "";
mdo.r = 120;
mdo.g = 150;
mdo.b = 255;
mdo.justify = en_tj_none;
mdo.font = 4;
mdo.font_size = 11;
mdo.line_style = (en_mgraph_line_style)FL_SOLID;
mdo.line_thick = 1;


for( int i = 0; i < vcarrier_max.size(); i++ )
	{
	st_carrier_max_tag o = vcarrier_max[i];
	
//	float f0 = 0.01;//vcarrier_max[i].lev;
//	float f1 = 11.97e6;//vcarrier_max[i].freq;
	float f0 = vcarrier_max[i].lev;
	float f1 = vcarrier_max[i].freq;
//	int graph_x_idx = vcarrier_max[i].graph_x_idx;
	
	if( f0 > 0.0001 )
		{
		mdo.vpolyx.clear();
		mdo.vpolyy.clear();
		mdo.vpolyx.push_back( f1 - 3000/disp_spect_zoom_factor );		mdo.vpolyy.push_back( f0 - f0*0.015 );
		mdo.vpolyx.push_back( f1 );   									mdo.vpolyy.push_back( f0 + f0*0.03 );
		mdo.vpolyx.push_back( f1 + 3000/disp_spect_zoom_factor);		mdo.vpolyy.push_back( f0 - f0*0.015 );
	

//		gph0->vdrwobj.clear();
		gph0->vdrwobj.push_back( mdo );
		}
	}

//----





//------ hov freq detail ---------

if( pref_freq_mouse_hov )
	{
	mdo.type = (en_mgraph_draw_obj_type)en_dobt_text;

	mdo.visible = 1;
	mdo.draw_ordering = 2;				//draw before graticle, 0 = before grid,  1 = after graticle but before traces,   2 = after traces

	mdo.use_pos_x = -1;                 //if not set to -1: then use this trace id to get a specific x position
	mdo.use_scale_x = -1;               //if not set to -1: then use this trace id to get a specific x scale

	mdo.use_pos_y = -1;                 //if not set to -1: then use this trace id to get a specific y position
	mdo.use_scale_y = -1;               //if not set to -1: then use this trace id to get a specific y scale

	mdo.clip_left = 0;					//samke as background
	mdo.clip_right = 0;
	mdo.clip_top = 0;
	mdo.clip_bottom = 0;


	mdo.x1 = 4;
	mdo.y1 = 38;
	mdo.x2;
	mdo.y2;
	//mdo.wid = 10;
	//mdo.hei = 10;

	//mouse pos


	int fractional_digits = 4;
	if( g_freq_sub_tune + g_freq_tune >= 1e9 ) fractional_digits = 5;	//GHz?

	string snum, sunits, scombined;

	m1.make_engineering_str( snum, sunits, scombined, fractional_digits, gph_mouse_freq, " ", "Hz" );

	strpf( s1, "%d Hz %s", (int)gph_mouse_freq, scombined.c_str() );
	mdo.arc1 = 0;
	mdo.arc2 = 0;
	mdo.stext = s1;
	mdo.r = 159;
	mdo.g = 190;
	mdo.b = 201;
	mdo.justify = en_tj_none;
	mdo.font = 4;
	mdo.font_size = 10;
	mdo.line_style = (en_mgraph_line_style)FL_SOLID;
	mdo.line_thick = 1;

	gph0->vdrwobj.push_back( mdo );
	}

//--------------------------




//------ keyin freq popup ---------
if( 1 )
	{
	float dt = m1_skeyin_freq_changed.time_passed( m1_skeyin_freq_changed.ns_tim_start );
	
	if( dt < 2 )
		{
		
		//text backgrnd rect
		
		int px = 4;
		int py = hei/2 + 7;
		
		mdo.type = (en_mgraph_draw_obj_type)en_dobt_rectf;
	
		s1 = wnd_rtl_graph->miwp_tune->miw->value();
		strpf( s1, "Freq: %s ", s1.c_str() );

		int txt_hei;
		int txt_descnt;
		int txt_wid = text_dim( s1, 4, 18, txt_hei, txt_descnt );

		mdo.x1 = px - 2;
		mdo.y1 = py + txt_descnt + 2;
		mdo.x2 = txt_wid+4;
		mdo.y2 = py - (txt_hei);
		
		mdo.arc1 = 0;
		mdo.arc2 = 0;
		mdo.stext = s1;
		mdo.r = 60;
		mdo.g = 60;
		mdo.b = 60;
		mdo.justify = en_tj_none;
		mdo.font = 4;
		mdo.font_size = 18;
		mdo.line_style = (en_mgraph_line_style)FL_SOLID;
		mdo.line_thick = 1;

		gph0->vdrwobj.push_back( mdo );




		//text
		mdo.type = (en_mgraph_draw_obj_type)en_dobt_text;

		mdo.x1 = px;
		mdo.y1 = py;
		mdo.x2;
		mdo.y2;
		
		
		mdo.arc1 = 0;
		mdo.arc2 = 0;
		mdo.stext = s1;
		mdo.r = 159;
		mdo.g = 190;
		mdo.b = 201;
		mdo.justify = en_tj_none;
		mdo.font = 4;
		mdo.font_size = 18;
		mdo.line_style = (en_mgraph_line_style)FL_SOLID;
		mdo.line_thick = 1;

		gph0->vdrwobj.push_back( mdo );
		}
	}
//---------------------------------





//-----------------------------------
bool b_sub_tuner_needle = 1;

if( b_sub_tuner_needle )
	{
//--- sub tuner vertical needle  ----
	mdo.type = (en_mgraph_draw_obj_type)en_dobt_polyline;
	mdo.draw_ordering = 2;				//draw before graticle, 0 = before grid,  1 = after graticle but before traces,   2 = after traces
	mdo.use_pos_x = 0;                 //if not set to -1: then use this trace id to get a specific x position
	mdo.use_pos_y = 0;
	mdo.use_scale_x = 0;               //if not set to -1: then use this trace id to get a specific x scale
	mdo.use_scale_y = -1;              //if not set to -1: then use this trace id to get a specific x scale

	mdo.clip_left = 0;					//same as background
	mdo.clip_right = 0;
	mdo.clip_top = 5;
	mdo.clip_bottom = 9;


	mdo.r = 200;
	mdo.g = 170;
	mdo.b = 75;
	mdo.vpolyx.clear();
	mdo.vpolyy.clear();
	
	d_tune_needle_x = g_freq_sub_tune + g_freq_tune;
	
	mdo.vpolyx.push_back( d_tune_needle_x );			mdo.vpolyy.push_back( 0 );
	mdo.vpolyx.push_back( d_tune_needle_x );   			mdo.vpolyy.push_back( hei-33 );

	gph0->vdrwobj.push_back( mdo );
	
	
	mdo.draw_ordering = 1;				//draw before graticle, 0 = before grid,  1 = after graticle but before traces,   2 = after traces
	mdo.vpolyx.clear();
	mdo.vpolyy.clear();




	mdo.clip_left = 0;					//same as background
	mdo.clip_right = 0;
	mdo.clip_top = 0;
	mdo.clip_bottom = 0;
	mdo.r = 120;
	mdo.g = 120;
	mdo.b = 120;
	mdo.vpolyx.push_back( d_tune_needle_x );	mdo.vpolyy.push_back( hei );		//draw faint part of needle line in lying in x-axis freq scale (at top of graph)
	mdo.vpolyx.push_back( d_tune_needle_x );   	mdo.vpolyy.push_back( hei-33 );

	gph0->vdrwobj.push_back( mdo );

//------------------


//--- draw short horiz markers on needle to show mousewheel tweek regions ----
	mdo.draw_ordering = 2;				//draw before graticle, 0 = before grid,  1 = after graticle but before traces,   2 = after traces

	mdo.use_scale_x = -1;               //if not set to -1: then use this trace id to get a specific x scale
	mdo.use_scale_y = -1;               //if not set to -1: then use this trace id to get a specific x scale

	mdo.use_pos_x = -1;                 //if not set to -1: then use this trace id to get a specific x position
	mdo.use_pos_y = -1;

	mdo.clip_left = 0;					//same as background
	mdo.clip_right = 0;
	mdo.clip_top = 0;
	mdo.clip_bottom = 0;

	mdo.r = 200;
	mdo.g = 170;
	mdo.b = 75;
	mdo.vpolyx.clear();
	mdo.vpolyy.clear();

	double x1, y1, x2, y2, x3, y3, x4, y4;
	
	x1 = d_tune_needle_x;
	y1 = 0;
	
	gph0->get_plot_values_zero_offsets( 0, x1, y1, x2, y2, x3, y3, x4, y4, 1, 1 );	//convert trace vals to pixel positions

	int h0 = wnd_rtl_graph->gph0->h() * cn_graph_mousewheel_vert_region_middle;
	

	mdo.vpolyx.push_back( x1 - 2 );		mdo.vpolyy.push_back( h0 );
	mdo.vpolyx.push_back( x1 + 3 );   	mdo.vpolyy.push_back( h0);

	gph0->vdrwobj.push_back( mdo );
	


	mdo.vpolyx.clear();
	mdo.vpolyy.clear();
	h0 = wnd_rtl_graph->gph0->h() * cn_graph_mousewheel_vert_region_high;

	mdo.vpolyx.push_back( x1 - 2 );		mdo.vpolyy.push_back( h0 );
	mdo.vpolyx.push_back( x1 + 3 );   	mdo.vpolyy.push_back( h0);

	gph0->vdrwobj.push_back( mdo );




//-----------------------------------




	//------ sdr sub tuner freq text -------
	if( pref_show_graph_hz_units | pref_show_graph_hz_eng )
		{
		int fractional_digits = 4;
		if( g_freq_sub_tune + g_freq_tune >= 1e9 ) fractional_digits = 5;	//GHz?
		
		string snum, sunits, scombined;
		m1.make_engineering_str( snum, sunits, scombined, fractional_digits, (int)g_freq_sub_tune + g_freq_tune, " ", "Hz" );
		//if( pref_show_graph_hz_units ) strpf( s1, "%d Hz %s", (int)g_freq_sub_tune + g_freq_tune, scombined.c_str() );

		if( pref_show_graph_hz_units && pref_show_graph_hz_eng ) strpf( s1, "%d Hz %s", (int)g_freq_sub_tune + g_freq_tune, scombined.c_str() );
		if( pref_show_graph_hz_units && !pref_show_graph_hz_eng ) strpf( s1, "%d Hz", (int)g_freq_sub_tune + g_freq_tune );
		if( !pref_show_graph_hz_units && pref_show_graph_hz_eng ) strpf( s1, "%s", scombined.c_str() );

		mdo.type = (en_mgraph_draw_obj_type)en_dobt_text;

		mdo.use_pos_x = 0;                 //if not set to -1: then use this trace id to get a specific x position
		mdo.use_scale_x = 0;               //if not set to -1: then use this trace id to get a specific x scale

		mdo.use_pos_y = -1;                 //if not set to -1: then use this trace id to get a specific y position
		mdo.use_scale_y = -1;               //if not set to -1: then use this trace id to get a specific y scale

		mdo.x1 = d_tune_needle_x;//gph0->w()*6/8;
		mdo.y1 = 27;
		mdo.stext = s1;
		mdo.justify = en_tj_horiz_center;
		mdo.font_size = 14;
		mdo.r = 0;
		mdo.g = 200;
		mdo.b = 50;

		gph0->vdrwobj.push_back( mdo );
		}

	//---------------------------------------


	//--- draw horiz line bottom of graph to show in hzoom region ----
	if( wnd_rtl_graph->gph0->inside_control )
		{
		if( ( wnd_rtl_graph->gph0->mousey >= wnd_rtl_graph->gph0->h() * cn_graph_mousewheel_vert_region_verylow ) )
			{
			mdo.type = (en_mgraph_draw_obj_type)en_dobt_polyline;

			mdo.use_scale_x = -1;               //if not set to -1: then use this trace id to get a specific x scale
			mdo.use_scale_y = -1;               //if not set to -1: then use this trace id to get a specific x scale

			mdo.use_pos_x = -1;                 //if not set to -1: then use this trace id to get a specific x position
			mdo.use_pos_y = -1;

			mdo.clip_left = 0;					//same as background
			mdo.clip_right = 0;
			mdo.clip_top = 0;
			mdo.clip_bottom = 0;

			mdo.line_thick = 5;

			mdo.r = 86;
			mdo.g = 180;
			mdo.b = 235;

			mdo.vpolyx.clear();
			mdo.vpolyy.clear();

			int offx, offy;
			int wid, hei;
			gph0->get_background_offsxy( offx, offy );
			gph0->get_background_dimensions( wid, hei );

			
			h0 = wnd_rtl_graph->gph0->h() * cn_graph_mousewheel_vert_region_verylow + 5;

			int bar_wid = (wid) / disp_spect_zoom_factor;
			
			if( bar_wid < 10 ) bar_wid = 10;
			
			int midx = wid / 2;
			
			mdo.vpolyx.push_back( midx - bar_wid / 2 );					mdo.vpolyy.push_back( h0 );
			
			mdo.vpolyx.push_back( midx + bar_wid / 2 );   				mdo.vpolyy.push_back( h0 );

			gph0->vdrwobj.push_back( mdo );
			}
		}

	}
//------------------------------------------------------------------







//--- horiz freq ticks  ----
bool b_horiz_freq_ticks = 1;

//bool b_first_right_tick_done = 0;
//double first_right_tick_freq = 0.0;

if( b_horiz_freq_ticks )
	{
	float fcntr = g_freq_tune;
	
	fcntr /= 1000;
	fcntr =  ceilf( fcntr );
	fcntr *= 1000;

	float cnt = 100000;													//pick some large count of freq ticks, the 'for' loops will skip those 'off screen'

	float delta = 50000.0f / disp_spect_zoom_factor;
	delta /= 1000.0;
	delta = nearbyint( delta );
	delta *= 1000.0;
	float delta_prev = delta;
	float delta_next = delta;
	
	//try to keep tick values human friendly
	if( ( delta <= 50 ) ) { delta_prev = 25; delta = 50; delta_next = 100; }
	if( ( delta > 50 ) && ( delta <= 100 ) ) { delta_prev = 50; delta = 100; delta_next = 250; }
	if( ( delta > 100 ) && ( delta <= 250 ) ) { delta_prev = 100; delta = 250; delta_next = 500; }
	if( ( delta > 250 ) && ( delta <= 500 ) ) { delta_prev = 250; delta = 500; delta_next = 1000; }
	if( ( delta > 500 ) && ( delta <= 1000 ) ) { delta_prev = 500; delta = 1000; delta_next = 2500; }
	if( ( delta > 1000 ) && ( delta <= 2500 ) ) { delta_prev = 1000; delta = 2500; delta_next = 5000; }
	if( ( delta > 2500 ) && ( delta <= 5000 ) ) { delta_prev = 2500; delta = 5000; delta_next = 10000; }
	if( ( delta > 5000 ) && ( delta <= 10000 ) ) { delta_prev = 5000; delta = 10000; delta_next = 25000; }
//	if( ( delta > 10000 ) && ( delta <= 12500 ) ) { delta_prev = 10000; delta = 12500; delta_next = 25000; }
	if( ( delta > 10000 ) && ( delta <= 25000 ) ) { delta_prev = 10000; delta = 25000; delta_next = 50000; }
	if( delta > 25000 ) { delta_prev = 25000; delta = 50000; delta_next = 50000; }


	bool btrace_offs_zero = 1;
	bool bplot_offs_zero = 1;
	double x2, x3, y1;

	y1 = 0;
	double centr_x = fcntr;	
	gph0->get_plot_value_coord_zero_offsets( 0, centr_x, y1, btrace_offs_zero, bplot_offs_zero );	//find how far apart the freq ticks will be for given 'delta'

	y1 = 0;
	x2 = fcntr + delta;
	gph0->get_plot_value_coord_zero_offsets( 0, x2, y1, btrace_offs_zero, bplot_offs_zero );	//find how far apart the freq ticks will be for given 'delta'


	int delta_px = x2 - centr_x;										//pixel spaces between ticks
	

	if( delta_px < 60 ) delta = delta_next;								//ticks too close?     	use alternate 'delta'
	if( delta_px > 100 ) delta = delta_prev;							//ticks too far apart? 	use alternate 'delta'

	x3 = fcntr;
	gph0->get_plot_value_coord_zero_offsets( 0, x3, y1, btrace_offs_zero, bplot_offs_zero );	//find where rounded onboard tuner freq lies (in pixels)

//	float fcntr_horiz_ratio = fabsf( x1 / wid );
	
//	int tick_cnt = fabsf( 1.0f - fcntr_horiz_ratio ) * 18; 


	double freq_left, freq_right;
	gph0_edge_freqs( 0, -25, freq_left, freq_right );

	double fstart = fcntr + delta;
	double fstart2 = fstart / 100000;
	double fstart3 = 0.0;
	
	double dint, dfact;
	dfact = modf( fstart2, &dint );
//	if( dfact < 0.25 ) fstart2 = dint + 0.0;
//	if( ( dfact >= 0.25 ) && ( dfact < 0.75 ) ) fstart2 = dint + 0.5;
//	if( dfact >= 0.75 ) fstart2 = dint + 1.0;
	fstart2 = dint + 0.0;												//round down
	fstart3 = dint + 1.0;												//round up
		
//	fstart2 = nearbyint( fstart2 );
	fstart2 *= 100000;
	fstart3 *= 100000;
//printf(">>>>>>>>>>>>>>>>>>> delta_px %d,  x2 %d  tick_cnt %d  fstart %f %f  delta %f\n", (int)delta_px, (int)x3, tick_cnt, fstart, fstart2, delta);
//printf(">>> dint %f  dfact %f  fstart %f %f  delta %f\n", (int)delta_px, (int)x3, dint, dfact, fstart, fstart2, delta);

//	int tick_cnt = (freq_right - fcntr) / delta;
	
//printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>> x1 %d, tick_cnt %f	delta %f  freq_left %f %f\n", (int)x1, tick_cnt, delta, freq_left, freq_right );

//int last_x1 = centr_x;

float frq_prev = 0;

int tick_cnt = 0;														//not including fcntr
//bool b_tick_too_close = 1;

	//---- right side ticks
if( 1 )
	{
	//adj delta if ticks are too close
	for( int i = 0; i < 8; i++ )										//do an abitrary loop, will break when tick spacing measures ok 
		{
		double x1 = fcntr + delta;	
		gph0->get_plot_value_coord_zero_offsets( 0, x1, y1, btrace_offs_zero, bplot_offs_zero );	//find how far apart the freq ticks will be for given 'delta'

		if( ( (int)x1 - (int)centr_x ) < 55 ) 									//are ticks getting too close?
			{
	//		b_tick_too_close = 1;
			delta *= 2.0f;													//move ticks further apart


//	printf("tick_too_close i %d,   x1 %d   delta %f\n", i, (int)x1 - (int)centr_x, delta );


		//				last_x1 = x1;
			}
		else{
			break;
			}
		}


	//---- right side ticks
		for( int i = 0; i <= cnt; i++ )
			{
			double f0 = fcntr + delta * i;
			if( i == 0 ) f0 = fcntr;
			else f0 = fstart2 + delta * (i-1);

	//		printf("i %d  fcntr %f  f0 %f\n", i, fcntr, f0 );
			if( f0 < fcntr )											//ignore ticks before onboard tuner freq
					{
	//				printf("f0 < fcntr at i %d  fcntr %f  f0 %f\n", i, fcntr, f0 );
					continue;
					}
//if( i < 10 ) continue;


//	if( i != 0 ) continue;													//for dbg



			if( f0 < freq_left ) 										//past gph edge?
				{
	//			printf(" continue at i: %d\n", i );
				continue;
				}

			if( f0 >= freq_right ) 										//past gph edge?
				{
	//			printf(" break at i: %d\n", i );
				break;
				}


			if( i != 0 ) 						
				{
				double x1 = f0;	
				gph0->get_plot_value_coord_zero_offsets( 0, x1, y1, btrace_offs_zero, bplot_offs_zero );	//find how far apart the freq ticks will be for given 'delta'

	//			printf("delta x1 %d\n", (int)x1 - last_x1 );

/*				
				if( ( (int)x1 - last_x1 ) < 55 ) 							//are ticks getting too close?
					{
					b_tick_too_close = 1;
					
	//				last_x1 = x1;
					continue;
					}
*/

//				last_x1 = x1;

				if( x1 < ((int)centr_x + 90) ) 									//x pos too close to 'fcntr' x pos ?
					{
	//				printf("continue2 at i %d\n", i );
					continue;
					}

	//			if( !b_first_right_tick_done )
	//					{
	//					b_first_right_tick_done = 1;
	//					first_right_tick_freq = f0;
	//					printf("b_first_right_done at i: %d   f0: %f\n", i, first_right_tick_freq );
	//					}
				}

//	printf("i %d  fcntr %f   f0 %f    freq_left %f    freq_right %f\n", i, fcntr, f0, freq_left, freq_right );

	//--- tick mark
			mdo.draw_ordering = 1;				//draw before graticle, 0 = before grid,  1 = after graticle but before traces,   2 = after traces
			mdo.type = (en_mgraph_draw_obj_type)en_dobt_polyline;

			mdo.use_pos_x = 0;                 //if not set to -1: then use this trace id to get a specific x position
			mdo.use_pos_y = -1;
			mdo.use_scale_x = 0;               //if not set to -1: then use this trace id to get a specific x scale
			mdo.use_scale_y = -1;              //if not set to -1: then use this trace id to get a specific x scale

			mdo.clip_left = 0;					//same as background
			mdo.clip_right = 0;
			mdo.clip_top = 0;
			mdo.clip_bottom = 0;

			mdo.r = col_xaxis_text_freq_r;
			mdo.g = col_xaxis_text_freq_g;
			mdo.b = col_xaxis_text_freq_b;
			mdo.line_thick = 1;

			mdo.vpolyx.clear();
			mdo.vpolyy.clear();

			int py = 12;
			int dy = 4;
			
			if( i == 0 ) 
				{
				mdo.line_thick = 2;
				dy = 5;
				f0 = g_freq_tune;
				}


			mdo.vpolyx.push_back( f0 );				mdo.vpolyy.push_back( py );
			mdo.vpolyx.push_back( f0 );   			mdo.vpolyy.push_back( py + dy );

			gph0->vdrwobj.push_back( mdo );
	//---



	//---  in-between ticks without text
			double delta0 = f0 - frq_prev;
			double f1 = f0 - delta0 / 2.0f;

			mdo.vpolyx.clear();
			mdo.vpolyy.clear();

			mdo.vpolyx.push_back( f1 );									mdo.vpolyy.push_back( py );
			mdo.vpolyx.push_back( f1 );   								mdo.vpolyy.push_back( py + dy );

//			if( f1 > fcntr ) 
				{
				if( tick_cnt > 0 )
					{
//			printf("i %d  frq_prev + delta/2.0f %f\n", i, f1 );
					gph0->vdrwobj.push_back( mdo );
					}
				}

			frq_prev = f0;
	//---


	//---  do a single in-between tick between fcntr and first tick (with no text)
			if( (i != 0 ) && ( tick_cnt == 0 ) )
				{
				double f1 = f0 - delta/2.0;
				
				int py = 12;
				int dy = 4;

				mdo.vpolyx.clear();
				mdo.vpolyy.clear();

				mdo.vpolyx.push_back( f1 );									mdo.vpolyy.push_back( py );
				mdo.vpolyx.push_back( f1 );   								mdo.vpolyy.push_back( py + dy );

				gph0->vdrwobj.push_back( mdo );
				}
	//---


//	if( mdo.line_thick == 2 )
//		{
//		printf("i %d  fcntr %f   f0 %f    freq_left %f    freq_right %f\n", i, fcntr, f0, freq_left, freq_right );
//		}

//			frq_prev = f0;
	//---







	//--- tick text
			mdo.type = (en_mgraph_draw_obj_type)en_dobt_text;

			if( i == 0 )
				{
				mdo.r = 229;
				mdo.g = 142;
				mdo.b = 0;
				}

			mdo.justify = en_tj_horiz_center;
			mdo.font = 4;
			mdo.font_size = 10;
			mdo.line_style = (en_mgraph_line_style)FL_SOLID;

			if( i == 0 ) 													//onboard tuner freq req?
				{
				int fractional_digits = 4;
				if( g_freq_tune >= 1e9 ) fractional_digits = 5;			//GHz?
				string snum, sunits, scombined;
				
				m1.make_engineering_str( snum, sunits, scombined, fractional_digits, (int)g_freq_tune, " ", "Hz" );

				if( pref_show_graph_hz_units && pref_show_graph_hz_eng ) strpf( s1, "%d Hz %s", (int)g_freq_tune, scombined.c_str() );
				if( pref_show_graph_hz_units && !pref_show_graph_hz_eng ) strpf( s1, "%d Hz", (int)g_freq_tune );
				if( !pref_show_graph_hz_units && pref_show_graph_hz_eng ) strpf( s1, "%s", scombined.c_str() );
				}
			else{
				if( f0 < 1e9 ) strpf( s1, "%.3f", f0/1000000 );				//MHz
				else strpf( s1, "%.5fG", f0/1e9 );							//GHz

//				strpf( s1, "%d", tick_cnt );
//				mdo.stext = s1;
				}

			mdo.stext = s1;

			mdo.x1 = f0;
			mdo.y1 = 10;

			mdo.vpolyx.clear();
			mdo.vpolyy.clear();
			gph0->vdrwobj.push_back( mdo );
	//---
			if( i != 0 ) tick_cnt++;									//inc if not fcntr
			}
	}

//last_x1 = centr_x;

frq_prev = 0;



	//---- left side ticks
if( 1 )
	{
	double x1;
		for( int i = 0; i < cnt; i++ )
			{
			if( i == 0 ) continue;										//skip fcntr

			double f0 = fstart3 - delta * (i-1);
//			double f0 = first_right_tick_freq - delta * (i-1);

			if( f0 < freq_left ) 										//past gph edge?
				{
//				printf(" break at i: %d   f0 %f  < freq_left %f\n", i, f0, freq_left );
				break;
				}

			if( f0 > freq_right ) 										//past gph edge?
				{
//				printf("continue1 at i: %d   f0 %f  > freq_right %f\n", i, f0, freq_right );
				continue;
				}

			if( f0 >= fcntr ) 											//past beyond fcntr ?
				{
//				printf("continue2 at i %d   f0 %f >= fcntr %f\n", i, f0, fcntr );
				continue;
				}

			if( i != 0 ) 						
				{
				x1 = f0;	
				gph0->get_plot_value_coord_zero_offsets( 0, x1, y1, btrace_offs_zero, bplot_offs_zero );	//find how far apart the freq ticks will be for given 'delta'


//			if( i == 1159 ) printf("i %d f0 %f  last_x1 %d, x1 %d,   delta %d  \n",  i, f0, last_x1, (int)x1, last_x1 - (int)x1 );

				if( ( (int)centr_x - (int)x1 ) < 60 ) 						//are ticks getting too close?
					{
					continue;
					}

//				last_x1 = x1;


				if( x1 > ((int)centr_x - 90) ) 								//x pos too close to 'fcntr' x pos ?
					{
//					printf("continue3 at i %d\n", i );
					continue;
					}
				}

		
//		if( x1 < 0 ) bvis = 0;
//		if( x1 > wid ) bvis = 0;
		if( 1 )
			{
	//	printf("shown i %d  fstart3 %f   f0 %f\n", i, fstart3, f0 );

		//--- tick mark
				mdo.draw_ordering = 1;				//draw before graticle, 0 = before grid,  1 = after graticle but before traces,   2 = after traces
				mdo.type = (en_mgraph_draw_obj_type)en_dobt_polyline;

				mdo.use_pos_x = 0;                 //if not set to -1: then use this trace id to get a specific x position
				mdo.use_pos_y = -1;
				mdo.use_scale_x = 0;               //if not set to -1: then use this trace id to get a specific x scale
				mdo.use_scale_y = -1;              //if not set to -1: then use this trace id to get a specific x scale

				mdo.clip_left = 0;					//same as background
				mdo.clip_right = 0;
				mdo.clip_top = 0;
				mdo.clip_bottom = 0;

				mdo.r = col_xaxis_text_freq_r;
				mdo.g = col_xaxis_text_freq_g;
				mdo.b = col_xaxis_text_freq_b;
				mdo.line_thick = 1;

				mdo.vpolyx.clear();
				mdo.vpolyy.clear();

				int py = 12;
				mdo.vpolyx.push_back( f0 );				mdo.vpolyy.push_back( py );
				mdo.vpolyx.push_back( f0 );   			mdo.vpolyy.push_back( py+4 );

				gph0->vdrwobj.push_back( mdo );
		//---


		//---  in-between ticks without text
				double delta = frq_prev - f0;
				
				mdo.vpolyx.clear();
				mdo.vpolyy.clear();
				
				mdo.vpolyx.push_back( frq_prev + delta/2.0f );				mdo.vpolyy.push_back( py );
				mdo.vpolyx.push_back( frq_prev + delta/2.0f );   			mdo.vpolyy.push_back( py+4  );

				gph0->vdrwobj.push_back( mdo );

				frq_prev = f0;
		//---


		//--- tick text
				mdo.type = (en_mgraph_draw_obj_type)en_dobt_text;
			//	mdo.r = 229;
			//	mdo.g = 142;
			//	mdo.b = 0;
				mdo.justify = en_tj_horiz_center;
				mdo.font = 4;
				mdo.font_size = 10;
				mdo.line_style = (en_mgraph_line_style)FL_SOLID;

				if( f0 < 1e9 ) strpf( s1, "%.3f", f0/1000000 );				//MHz
				else strpf( s1, "%.5fG", f0/1e9 );							//GHz
				mdo.stext = s1;

				mdo.x1 = f0;
				mdo.y1 = 10;

				mdo.vpolyx.clear();
				mdo.vpolyy.clear();
				gph0->vdrwobj.push_back( mdo );
		//---
				}
			}
		}
	}
//------------------





//------- draw BPF rectangle -----
if( g_b_bw_bpass )
	{
	mdo.type = (en_mgraph_draw_obj_type)en_dobt_rectf;
	mdo.draw_ordering = 0;				//draw before graticle, 0 = before grid,  1 = after graticle but before traces,   2 = after traces

	mdo.use_pos_x = 0;					//if not set to -1: then use this trace id to get a specific x position
	mdo.use_pos_y = 0;

	mdo.use_scale_x = 0;				//if not set to -1: then use this trace id to get a specific x scale
	mdo.use_scale_y = -1;				//if not set to -1: then use this trace id to get a specific x scale

	mdo.clip_left = 0;					//same as background
	mdo.clip_right = 0;
	mdo.clip_top = 0;
	mdo.clip_bottom = 0;

	mdo.x1 = g_freq_sub_tune + g_freq_tune - (g_bw_upper - g_bw_lower)/2;
	mdo.y1 = 5;

	mdo.x2 = g_freq_sub_tune + g_freq_tune + (g_bw_upper - g_bw_lower)/2;
	mdo.y2 = wnd_rtl_graph->gph0->h() - 5;

	mdo.line_thick = 1;
	
	mdo.r = 100;
	mdo.g = 100;
	mdo.b = 100;
	
	gph0->vdrwobj.push_back( mdo );
	}
//----------------------------------




//int mx, my;
//gph0->get_mouse_pixel_position_on_background( mx, my );


//for( int i = 0; i < vpin.size(); i++ )
//	{
//	st_pin_tag op = vpin[i];
//	if( ( mx >= op.px1 ) && ( mx <= op.px2) )
//		{
//		vpin[i].flags = 0x0;
//		}
//	}


// --- draw demod text buttons/rectangles and pins ----
if( 1 )
	{
	gph0_obj_build();
	gph0_obj_calc_coords();
	gph0_obj_find_hover();			//need to call 'gph0_obj_build()' and  'gph0_obj_calc_coords()' first

	for( int i = 0; i < vgph_obj.size(); i++ )							//show user interactive objs
		{
		st_gph_obj_tag op = vgph_obj[i];

		if( !(op.flags & en_gflg_vis) )	continue;						//not visible ? 
		
		mdo.type = op.shape;
		mdo.draw_ordering = 1;				//draw before graticle, 0 = before grid,  1 = after graticle but before traces,   2 = after traces

		mdo.use_pos_x = -1;					//if not set to -1: then use this trace id to get a specific x position
		mdo.use_pos_y = -1;

		mdo.use_scale_x = -1;				//if not set to -1: then use this trace id to get a specific x scale
		mdo.use_scale_y = -1;				//if not set to -1: then use this trace id to get a specific x scale

		mdo.clip_left = 0;					//same as background
		mdo.clip_right = 0;
		mdo.clip_top = 0;
		mdo.clip_bottom = 0;


		mdo.stext = op.stext;

/*
		mdo.x1 = op.x + op.offsx;
		mdo.y1 = op.y + op.offsy + op.descent;

		mdo.x2 = mdo.x1 + op.wid;
		mdo.y2 = mdo.y1 + op.hei;

		op.px1 = mdo.x1;
		op.py1 = mdo.y1;
		op.px2 = mdo.x2;
		op.py2 = mdo.y2;
*/
		mdo.x1 = op.px1;												//calc'd in 'gph0_obj_calc_coords()'
		mdo.y1 = op.py1;
		mdo.x2 = op.px2;
		mdo.y2 = op.py2;
		
//		op.py1 = mdo.y1;
//		op.px2 = mdo.x2;
//		op.py2 = mdo.y2;

		mdo.font = op.font;
		mdo.font_size = op.font_size;
		mdo.justify = op.justify;
					
		mdo.line_thick = op.line_thick;
		mdo.line_style = op.line_style;
	
		mdo.arc1 = 0;
		mdo.arc2 = 360;
		
		mdo.r = op.rr;
		mdo.g = op.gg;
		mdo.b = op.bb;
		
		vgph_obj[i] = op;												//rem calc pixel positions for possible selection clicks
		
		gph0->vdrwobj.push_back( mdo );
		}

	}
//----------




}
























/*
void test_fftw_real()
{
int N=2048;

fftw_complex *out;
fftw_plan p;

out = (fftw_complex*) fftw_malloc( sizeof(fftw_complex) * (N / 2 + 1));

     fftw_real in[N], out[N], power_spectrum[N/2+1];

p = fftw_plan_dft_r2c_1d(N, in, out, FFTW_ESTIMATE);

     p = rfftw_create_plan(N, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);
}
*/





rtl_graph_wnd::rtl_graph_wnd( int xx, int yy, int wid, int hei, const char *label ) : Fl_Double_Window( xx, yy, wid, hei,label )
{
string s1, st;
My_Input_Wheel *omiw;


menu_hei = 25;
b_gph0_shrink = 0;

demodul_mode = en_dmt_wfm;
synth_mod_type = en_mdt_am;

iled_buf_rd_adj = 0;

b_synth_iq = 0;
b_synth_noise_on_iq = 0;
b_synth_voice_on_iq0 = 1;
b_synth_voice_on_iq1 = 1;
b_synth_voice_on_iq2 = 1;
b_synth_voice_on_iq3 = 1;
b_synth_voice_on_iq4 = 1;
b_synth_voice_on_iq5 = 1;
b_synth_voice_on_iq6 = 1;

synth_mod_1st_tone_ampl = 0.3;
synth_mod_1st_tone_freq = 200;
synth_mod_2nd_tone_ampl = 0.3;
synth_mod_2nd_tone_freq = 2000;

led_freq_mem_hov_idx = -1;
led_preset_hov_idx = -1;
led_preset_hov_idx2 = -1;


int jj = 0;
freq_memory[jj++].freq_tune = 50e6;
freq_memory[jj++].freq_sub_tune = 0.0f;

freq_memory[jj++].freq_tune = 100e6;
freq_memory[jj++].freq_sub_tune = 0.0f;

freq_memory[jj++].freq_tune = 150e6;
freq_memory[jj++].freq_sub_tune = 0.0f;

freq_memory[jj++].freq_tune = 200e6;
freq_memory[jj++].freq_sub_tune = 0.0f;

freq_memory[jj++].freq_tune = 250e6;
freq_memory[jj++].freq_sub_tune = 0.0f;

freq_memory[jj++].freq_tune = 300e6;
freq_memory[jj++].freq_sub_tune = 0.0f;

freq_memory[jj++].freq_tune = 400e6;
freq_memory[jj++].freq_sub_tune = 0.0f;

freq_memory[jj++].freq_tune = 500e6;
freq_memory[jj++].freq_sub_tune = 0.0f;

freq_memory[jj++].freq_tune = 50e6;
freq_memory[jj++].freq_sub_tune = 0.0f;

freq_memory[jj++].freq_tune = 100e6;
freq_memory[jj++].freq_sub_tune = 0.0f;

freq_memory[jj++].freq_tune = 150e6;
freq_memory[jj++].freq_sub_tune = 0.0f;

freq_memory[jj++].freq_tune = 200e6;
freq_memory[jj++].freq_sub_tune = 0.0f;

freq_memory[jj++].freq_tune = 250e6;
freq_memory[jj++].freq_sub_tune = 0.0f;

freq_memory[jj++].freq_tune = 300e6;
freq_memory[jj++].freq_sub_tune = 0.0f;

freq_memory[jj++].freq_tune = 400e6;
freq_memory[jj++].freq_sub_tune = 0.0f;

freq_memory[jj++].freq_tune = 500e6;
freq_memory[jj++].freq_sub_tune = 0.0f;




//IF YOU ADD to this also modify 'en_demodulator_type_tag'
//MOD ALSO 'sz_demodulator_type[]', ALSO MOD 'rtl_graph_wnd::demod_type_idx_from_str()'
//MOD ALSO 'pulldown_demod_mode'

strncpy( sz_demodulator_type[0], "en_dmt_fm_stereo", 32 );
strncpy( sz_demodulator_type[1], "en_dmt_wfm", 32 );
strncpy( sz_demodulator_type[2], "en_dmt_fm", 32 );
strncpy( sz_demodulator_type[3], "en_dmt_am", 32 );
strncpy( sz_demodulator_type[4], "en_dmt_ssb", 32 );
//strncpy( sz_demodulator_type[4], "en_dmt_lsb", 32 );
//strncpy( sz_demodulator_type[5], "en_dmt_usb", 32 );
//strncpy( sz_demodulator_type[6], "en_dmt_raw", 32 );

ineed_graph_fit = 20;
bneed_graph_fit_bring_for_front = 0;


//carrier_max_lev = 0.0f;
//carrier_max_freq = 0;

//b_carrier_max_tune = 0;
//carrier_max_tune_timer = 0.0f;
//carrier_max_tune_freq_last = 0;

//carrier_max_avg = 0.0f;
//carrier_max_avg_wr = 0;

//if( !rtl.open( 2048000, 1024 ) )

//if( !rtl.open( 600000, 512 ) )



//start_audio( this );
//start_audio();


//getchar();

/*
if( !rtl.open() )
	{
	printf( "rtl_graph_wnd::rtl_graph_wnd::() - failed to open rtl dongle\n" );
	}
else{
	rtl.set_srate( 2000000 );
	rtl.set_buf_size( 8 * 65536 );
	}
*/

//start_threads();

//tim.time_start( tim.ns_tim_start );


/*
while( 1 )
	{

	if( rtl_loaded )
		{
		printf("here1 %d\n", viq.size() );
		rtl_loaded = 0;
		
		}

	tim.delay_ms( 10 );
	}
*/



//menu bar
menu_sdr = new Fl_Menu_Bar(0, 0, this->w(), 25);
menu_sdr->textsize(12);
//menu_sdr->copy( menu_sdr_items, this );			//this causes RList selection to not correctly pass the 'v' index to callback: 'void cb_recentlist( Fl_Widget *, void *v )'
menu_sdr->menu( menu_sdr_items ) ;					//use this menu assignment for RList to  work
menu_sdr->hide();




int graticle_count_y = 8;					//this should be an even number
int graticle_count_x = 38;					//this should be an even number
int grat_pixels_x = 25;						//pxls per graticule
int grat_pixels_y = 25;

int border = 2;

int wfm_wid = graticle_count_x * grat_pixels_x; //calc size of wnd using graticule details
wfm_wid += 2 * border;

int wfm_hei = graticle_count_y * grat_pixels_y;
wfm_hei += 2 * border;


gph0_posy = 10;
gph0_hei = wfm_hei;//h() - 580;


gph0 = new mgraph( 10, gph0_posy, w() - 20, gph0_hei, "" );

gph0->background.r = 64;
gph0->background.g = 64;
gph0->background.b = 64;

gph0->bkgd_border_left = border;
gph0->bkgd_border_top = border;
gph0->bkgd_border_right = border;
gph0->bkgd_border_bottom = border;

gph0->graticule_border_left = border;
gph0->graticule_border_top = border;
gph0->graticule_border_right = border;
gph0->graticule_border_bottom = border;

gph0->graticle_count_x = graticle_count_x;
gph0->grat_pixels_x = grat_pixels_x;

gph0->graticle_count_y = graticle_count_y;
gph0->grat_pixels_y = grat_pixels_y;
gph0->cro_graticle = 1;
gph0->b_take_focus_on_enter = 1;	


//fi_ifreq = new Fl_Input( 520, h() - 100, 90, 20, "IFreq:" );
//fi_ifreq->when( FL_WHEN_ENTER_KEY );
//fi_ifreq->callback( cb_fi_ifreq, 0 );
//fi_ifreq->value( "5000" );




//fi_freq_bandwidth = new Fl_Input( 80, h() - 80, 70, 15, "BW:" );
//fi_freq_bandwidth->labelsize( 8 );
//fi_freq_bandwidth->textsize( 9 );
//fi_freq_bandwidth->tooltip( "rtl sampling bandwith" );
//fi_freq_bandwidth->value( "2400000" );
//fi_freq_bandwidth->callback( cb_bt_freq_bandwidth, 0 );
//fi_freq_bandwidth->when( FL_WHEN_ENTER_KEY );




//---
Fl_Group* gp_sweep = new Fl_Group( 300, h() - 55, 525, 53, "gp_sweep - UnderDevlpmt");
gp_sweep->labelsize(7);
gp_sweep->box( FL_BORDER_BOX );

bt_freq_start = new Fl_Button( gp_sweep->x() + 1, gp_sweep->y() + 1, 35, 15, "Start" );
bt_freq_start->labelsize( 9 );
bt_freq_start->tooltip( "freq of sweep start" );
bt_freq_start->callback( cb_bt_freq_start, 0 );

bt_freq_single = new Fl_Button( gp_sweep->x() + 1, gp_sweep->y() + 19, 35, 15, "Single" );
bt_freq_single->labelsize( 9 );
bt_freq_single->tooltip( "freq of sweep stop" );
bt_freq_single->callback( cb_bt_freq_single, 0 );

bt_freq_stop = new Fl_Button( gp_sweep->x() + 1, gp_sweep->y() + 37, 35, 15, "Stop" );
bt_freq_stop->labelsize( 9 );
bt_freq_single->tooltip( "stop sweep" );
bt_freq_stop->callback( cb_bt_freq_stop, 0 );



fi_freq_start = new My_Input_Choice( gp_sweep->x() + 82, gp_sweep->y() + 1, 120, 15, "StartFreq:" );
fi_freq_start->labelsize(9);
fi_freq_start->textsize(9);
fi_freq_start->callback( cb_bt_scan_history_start, 0 );
fi_freq_start->b_allow_wheel_inc = 0;
fi_freq_start->tooltip( "freq sweep start point" );

bt_freq_start_marker = new Fl_Button( gp_sweep->x() + 82 + 125, gp_sweep->y() + 1, 18, 15, "M" );
bt_freq_start_marker->labelsize(9);
bt_freq_start_marker->callback( cb_bt_freq_start_marker, 0 );


bx_freq_start = new Fl_Box( gp_sweep->x() + 82 + 125 + 20, gp_sweep->y() + 1, 100, 20, "118.5 MHz" );
bx_freq_start->labelsize( 8 );


fi_freq_stop = new My_Input_Choice( gp_sweep->x() + 82, gp_sweep->y() + 19, 120, 15, "StopFreq:" );
fi_freq_stop->labelsize(9);
fi_freq_stop->textsize(9);
fi_freq_stop->callback( cb_bt_scan_history_stop, 0 );
fi_freq_stop->b_allow_wheel_inc = 0;
fi_freq_stop->tooltip( "freq sweep stop point" );

fi_freq_resolution = new Fl_Input( gp_sweep->x() + 82, gp_sweep->y() + 37, 90, 15, "FReso:" );
fi_freq_resolution->labelsize(9);
fi_freq_resolution->textsize(9);
fi_freq_resolution->value( "512" );
fi_freq_resolution->tooltip( "freq sweep stepping rate" );




bx_freq_stop = new Fl_Box( gp_sweep->x() + 82 + 125 + 20, gp_sweep->y() + 21, 100, 20, "118.5 MHz" );
bx_freq_stop->labelsize( 8 );

bt_freq_stop_marker = new Fl_Button(  gp_sweep->x() + 82 + 125, gp_sweep->y() + 20, 18, 15, "M" );
bt_freq_stop_marker->labelsize(9);
bt_freq_stop_marker->callback( cb_bt_freq_stop_marker, 0 );


fi_freq_step = new Fl_Input( gp_sweep->x() + 82 + 150, gp_sweep->y() + 36, 40, 16, "StepFreq:" );
fi_freq_step->labelsize(9);
fi_freq_step->textsize(9);
fi_freq_step->value( "5000" );




fi_freq_threshold = new Fl_Input( gp_sweep->x() + 82 + 125 + 140, gp_sweep->y() + 1, 90, 15, "Thrshold:" );
fi_freq_threshold->labelsize( 9 );
fi_freq_threshold->textsize( 9 );
fi_freq_threshold->value( "-15" );

fi_freq_cur = new Fl_Input( gp_sweep->x() + 82 + 125 + 140, gp_sweep->y() + 21, 90, 15, "CurFreq:" );
fi_freq_cur->labelsize( 9 );
fi_freq_cur->textsize( 9 );
fi_freq_cur->value( "0" );

fi_ampl_mouse = new Fl_Input(  gp_sweep->x() + 82 + 125 + 140, gp_sweep->y() + 37, 160, 15, "MrkrAmpl:" );
fi_ampl_mouse->value( "0" );
fi_ampl_mouse->labelsize( 9 );
fi_ampl_mouse->textsize( 9 );

gp_sweep->end();
//------





//---
Fl_Group* gp_filter = new Fl_Group( 5, h() - 176, 285, 78, "gp_filter");
gp_filter->labelsize(7);
gp_filter->box( FL_BORDER_BOX );



ck_user_dwn_aa = new Fl_Check_Button( gp_filter->x() + 0, gp_filter->y() + 0, 50, 15, "DwnAA" );
ck_user_dwn_aa->labelsize( 10 );
ck_user_dwn_aa->callback( cb_ck_user_dwn_aa, 0 );
ck_user_dwn_aa->tooltip( "use an antialias filter before downsampler,\nthis helps reduce the chance of cross channel (image)\ninterference when trying to tune in a busy spectrum" );
ck_user_dwn_aa->value( g_b_dwn_aa );


jj = 0;
ld_aa_dwncnv_fc[jj] = new GCLed( gp_filter->x() + 75, gp_filter->y() + 2, 11, 11, "4K" );
ld_aa_dwncnv_fc[jj]->id = jj;
ld_aa_dwncnv_fc[jj]->labelsize( 8 );
ld_aa_dwncnv_fc[jj]->tooltip( "antialias iir filter cutfoff freq (pre downsampler)\nleft click to increment\nright click to decrement\n\nmiddle click to enter a specific freq\n\n* set this to about half of your DwnSrate setting *" );
ld_aa_dwncnv_fc[jj]->align( FL_ALIGN_LEFT );
ld_aa_dwncnv_fc[jj]->led_style = cn_gcled_style_square;
ld_aa_dwncnv_fc[jj]->SetColIndex(0, 80, 255, 80);						//lowest cutoff freq, brighter led
ld_aa_dwncnv_fc[jj]->SetColIndex(1, 80, 255, 80);
ld_aa_dwncnv_fc[jj]->SetColIndex(2, 80, 250, 80);
ld_aa_dwncnv_fc[jj]->SetColIndex(3, 80, 240, 80);
ld_aa_dwncnv_fc[jj]->SetColIndex(4, 80, 230, 80);
ld_aa_dwncnv_fc[jj]->SetColIndex(5, 80, 220, 80);
ld_aa_dwncnv_fc[jj]->SetColIndex(6, 80, 210, 80);
ld_aa_dwncnv_fc[jj]->SetColIndex(7, 80, 200, 80);
ld_aa_dwncnv_fc[jj]->SetColIndex(8, 80, 190, 80);
ld_aa_dwncnv_fc[jj]->SetColIndex(9, 80, 180, 80);
ld_aa_dwncnv_fc[jj]->SetColIndex(10, 80, 170, 80);
ld_aa_dwncnv_fc[jj]->SetColIndex(11, 80, 160, 80);
ld_aa_dwncnv_fc[jj]->SetColIndex(12, 80, 150, 80);
ld_aa_dwncnv_fc[jj]->ChangeCol( 1 );									//set to default of 4KHz  refer 'aa_dwncnv_fc'
ld_aa_dwncnv_fc[jj]->callback( cb_led_aa_dwncnv_fc, (void*)jj );
ld_aa_dwncnv_fc[jj]->deactivate();


ck_user_dc_block_iq = new Fl_Check_Button( gp_filter->x() + 0, gp_filter->y() + 20, 55, 15, "DcBlk" );
ck_user_dc_block_iq->labelsize( 10 );
ck_user_dc_block_iq->callback( cb_ck_user_dc_block_iq, 0 );
ck_user_dc_block_iq->tooltip( "use a dc blocking filter on audio samples" );
ck_user_dc_block_iq->value( b_dc_block_iq );


ck_user_hpf0 = new Fl_Check_Button( gp_filter->x() + 0, gp_filter->y() + 40, 47, 15, "hpf0" );
ck_user_hpf0->labelsize( 10 );
ck_user_hpf0->callback( cb_ck_user_hpf0, 0 );
ck_user_hpf0->tooltip( "highpass iir filter, try 100" );
ck_user_hpf0->value( g_b_user_iir_hpf0 );


miw_user_hpf0 = new My_Input_Wheel( gp_filter->x() + 40, gp_filter->y() + 40, 40, 15, "");
miw_user_hpf0->labelsize(10);
miw_user_hpf0->textsize(9);
miw_user_hpf0->align(FL_ALIGN_LEFT);
miw_user_hpf0->tooltip( "set the cutoff freq for iir filter" );
miw_user_hpf0->b_show_modified = 1;
miw_user_hpf0->col_bkg = fl_rgb_color( 220, 255, 220 );
miw_user_hpf0->color( miw_user_hpf0->col_bkg );
miw_user_hpf0->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
miw_user_hpf0->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
miw_user_hpf0->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
miw_user_hpf0->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
miw_user_hpf0->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
miw_user_hpf0->b_take_focus_on_inside = 0;
miw_user_hpf0->b_use_limit_min = 1;
miw_user_hpf0->limit_min = 0;
miw_user_hpf0->b_use_limit_max = 1;
miw_user_hpf0->limit_max = aud_op_srate/2;
miw_user_hpf0->b_invert_wheel = 0;
miw_user_hpf0->std_wheel_step = 1;
miw_user_hpf0->ctrl_wheel_step = 2;
miw_user_hpf0->shift_wheel_step = 5;
miw_user_hpf0->ctrl_shift_wheel_step = 10;

miw_user_hpf0->b_step_wheel_side_left = 1;								//these are overruled if ctrl/shift keys are down
miw_user_hpf0->b_step_wheel_side_left_center = 1;
miw_user_hpf0->b_step_wheel_side_right_center = 1;
miw_user_hpf0->b_step_wheel_side_right = 1;

miw_user_hpf0->step_wheel_side_right = 1;
miw_user_hpf0->step_wheel_side_right_center = 10;
miw_user_hpf0->step_wheel_side_left_center = 50;
miw_user_hpf0->step_wheel_side_left = 100;

miw_user_hpf0->s_printf_format = "%d";
miw_user_hpf0->force_integer = 1;
miw_user_hpf0->set_value_from_double( g_user_iir_hpf0 );
miw_user_hpf0->set_callback( (void*)cb_miw_user_iir_hpf0, miw_user_hpf0, (void*)0 );
miw_user_hpf0->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
miw_user_hpf0->id2 = 1;
miw_user_hpf0->allow_right_but_drag = 1;
miw_user_hpf0->right_drag_x_val_change_factor = 0;
miw_user_hpf0->right_drag_y_val_change_factor = miw_user_hpf0->limit_max / 100.0f;








//ck_user_lpf0 = new Fl_Check_Button( 10 + 120, h() - 250, 47, 15, "lpf0" );
ck_user_lpf0 = new Fl_Check_Button( gp_filter->x() + 100, gp_filter->y() + 0, 47, 15, "lpf0" );
ck_user_lpf0->labelsize( 10 );
ck_user_lpf0->callback( cb_ck_user_lpf0, 0 );
ck_user_lpf0->tooltip( "lowpass iir filter, try 1000" );
ck_user_lpf0->value( g_b_user_iir_lpf0 );


miw_user_lpf0 = new My_Input_Wheel( gp_filter->x() + 140, gp_filter->y() + 1, 40, 15, "");
miw_user_lpf0->labelsize(10);
miw_user_lpf0->textsize(9);
miw_user_lpf0->align(FL_ALIGN_LEFT);
miw_user_lpf0->tooltip( "set the cutoff freq for iir filter" );
miw_user_lpf0->b_show_modified = 1;
miw_user_lpf0->col_bkg = fl_rgb_color( 220, 255, 220 );
miw_user_lpf0->color( miw_user_lpf0->col_bkg );
miw_user_lpf0->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
miw_user_lpf0->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
miw_user_lpf0->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
miw_user_lpf0->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
miw_user_lpf0->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
miw_user_lpf0->b_take_focus_on_inside = 0;
miw_user_lpf0->b_use_limit_min = 1;
miw_user_lpf0->limit_min = 0;
miw_user_lpf0->b_use_limit_max = 1;
miw_user_lpf0->limit_max = aud_op_srate/2;
miw_user_lpf0->b_invert_wheel = 0;
miw_user_lpf0->std_wheel_step = 10;
miw_user_lpf0->ctrl_wheel_step = 25;
miw_user_lpf0->shift_wheel_step = 1;
miw_user_lpf0->ctrl_shift_wheel_step = 100;

miw_user_lpf0->b_step_wheel_side_left = 1;								//these are overruled if ctrl/shift keys are down
miw_user_lpf0->b_step_wheel_side_left_center = 1;
miw_user_lpf0->b_step_wheel_side_right_center = 1;
miw_user_lpf0->b_step_wheel_side_right = 1;

miw_user_lpf0->step_wheel_side_right = 1;
miw_user_lpf0->step_wheel_side_right_center = 10;
miw_user_lpf0->step_wheel_side_left_center = 50;
miw_user_lpf0->step_wheel_side_left = 100;

miw_user_lpf0->s_printf_format = "%d";
miw_user_lpf0->force_integer = 1;
miw_user_lpf0->set_value_from_double( g_user_iir_lpf0 );
miw_user_lpf0->set_callback( (void*)cb_miw_user_iir_lpf0, miw_user_lpf0, (void*)0 );
miw_user_lpf0->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
miw_user_lpf0->id2 = 1;
miw_user_lpf0->allow_right_but_drag = 1;
miw_user_lpf0->right_drag_x_val_change_factor = 0;
miw_user_lpf0->right_drag_y_val_change_factor = miw_user_lpf0->limit_max / 100.0f;







ck_user_lpf1 = new Fl_Check_Button( gp_filter->x() + 100, gp_filter->y() + 20, 47, 15, "lpf1" );
ck_user_lpf1->labelsize( 10 );
ck_user_lpf1->callback( cb_ck_user_lpf1, 0 );
ck_user_lpf1->tooltip( "lowpass iir filter, try 2000" );
ck_user_lpf1->value( g_b_user_iir_lpf1 );


miw_user_lpf1 = new My_Input_Wheel( gp_filter->x() + 140, gp_filter->y() + 21, 40, 15, "");
miw_user_lpf1->labelsize(10);
miw_user_lpf1->textsize(9);
miw_user_lpf1->align(FL_ALIGN_LEFT);
miw_user_lpf1->tooltip( "set the cutoff freq for iir filter" );
miw_user_lpf1->b_show_modified = 1;
miw_user_lpf1->col_bkg = fl_rgb_color( 220, 255, 220 );
miw_user_lpf1->color( miw_user_lpf1->col_bkg );
miw_user_lpf1->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
miw_user_lpf1->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
miw_user_lpf1->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
miw_user_lpf1->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
miw_user_lpf1->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
miw_user_lpf1->b_take_focus_on_inside = 0;
miw_user_lpf1->b_use_limit_min = 1;
miw_user_lpf1->limit_min = 0;
miw_user_lpf1->b_use_limit_max = 1;
miw_user_lpf1->limit_max = aud_op_srate/2;
miw_user_lpf1->b_invert_wheel = 0;
miw_user_lpf1->std_wheel_step = 10;
miw_user_lpf1->ctrl_wheel_step = 25;
miw_user_lpf1->shift_wheel_step = 1;
miw_user_lpf1->ctrl_shift_wheel_step = 100;

miw_user_lpf1->b_step_wheel_side_left = 1;								//these are overruled if ctrl/shift keys are down
miw_user_lpf1->b_step_wheel_side_left_center = 1;
miw_user_lpf1->b_step_wheel_side_right_center = 1;
miw_user_lpf1->b_step_wheel_side_right = 1;

miw_user_lpf1->step_wheel_side_right = 1;
miw_user_lpf1->step_wheel_side_right_center = 10;
miw_user_lpf1->step_wheel_side_left_center = 50;
miw_user_lpf1->step_wheel_side_left = 100;

miw_user_lpf1->s_printf_format = "%d";
miw_user_lpf1->force_integer = 1;
miw_user_lpf1->set_value_from_double( g_user_iir_lpf1 );
miw_user_lpf1->set_callback( (void*)cb_miw_user_iir_lpf1, miw_user_lpf1, (void*)0 );
miw_user_lpf1->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
miw_user_lpf1->id2 = 1;
miw_user_lpf1->allow_right_but_drag = 1;
miw_user_lpf1->right_drag_x_val_change_factor = 0;
miw_user_lpf1->right_drag_y_val_change_factor = miw_user_lpf1->limit_max / 100.0f;










ck_user_lpf2 = new Fl_Check_Button( gp_filter->x() + 100, gp_filter->y() + 40, 47, 15, "lpf2" );
ck_user_lpf2->labelsize( 10 );
ck_user_lpf2->callback( cb_ck_user_lpf2, 0 );
ck_user_lpf2->tooltip( "lowpass iir filter, try 3000" );
ck_user_lpf2->value( g_b_user_iir_lpf2 );


miw_user_lpf2 = new My_Input_Wheel( gp_filter->x() + 140, gp_filter->y() + 41, 40, 15, "");
miw_user_lpf2->labelsize(10);
miw_user_lpf2->textsize(9);
miw_user_lpf2->align(FL_ALIGN_LEFT);
miw_user_lpf2->tooltip( "set the cutoff freq for iir filter" );
miw_user_lpf2->b_show_modified = 1;
miw_user_lpf2->col_bkg = fl_rgb_color( 220, 255, 220 );
miw_user_lpf2->color( miw_user_lpf2->col_bkg );
miw_user_lpf2->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
miw_user_lpf2->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
miw_user_lpf2->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
miw_user_lpf2->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
miw_user_lpf2->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
miw_user_lpf2->b_take_focus_on_inside = 0;
miw_user_lpf2->b_use_limit_min = 1;
miw_user_lpf2->limit_min = 0;
miw_user_lpf2->b_use_limit_max = 1;
miw_user_lpf2->limit_max = aud_op_srate/2;
miw_user_lpf2->b_invert_wheel = 0;
miw_user_lpf2->std_wheel_step = 10;
miw_user_lpf2->ctrl_wheel_step = 25;
miw_user_lpf2->shift_wheel_step = 1;
miw_user_lpf2->ctrl_shift_wheel_step = 100;

miw_user_lpf2->b_step_wheel_side_left = 1;								//these are overruled if ctrl/shift keys are down
miw_user_lpf2->b_step_wheel_side_left_center = 1;
miw_user_lpf2->b_step_wheel_side_right_center = 1;
miw_user_lpf2->b_step_wheel_side_right = 1;

miw_user_lpf2->step_wheel_side_right = 1;
miw_user_lpf2->step_wheel_side_right_center = 10;
miw_user_lpf2->step_wheel_side_left_center = 50;
miw_user_lpf2->step_wheel_side_left = 100;

miw_user_lpf2->s_printf_format = "%d";
miw_user_lpf2->force_integer = 1;
miw_user_lpf2->set_value_from_double( g_user_iir_lpf2 );
miw_user_lpf2->set_callback( (void*)cb_miw_user_iir_lpf2, miw_user_lpf2, (void*)0 );
miw_user_lpf2->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
miw_user_lpf2->id2 = 1;
miw_user_lpf2->allow_right_but_drag = 1;
miw_user_lpf2->right_drag_x_val_change_factor = 0;
miw_user_lpf2->right_drag_y_val_change_factor = miw_user_lpf2->limit_max / 100.0f;








ck_bw_limit = new Fl_Check_Button( gp_filter->x() + 0, gp_filter->y() + 60, 60, 15, "BWFilt" );
ck_bw_limit->labelsize( 10 );
ck_bw_limit->callback( cb_bw_limit, 0 );
ck_bw_limit->tooltip( "Enable bandpass filter" );


//fi_bw_lower = new Fl_Input( 270, h() - 110, 60, 15, "BwLow:" );
//fi_bw_lower->labelsize(10);
//fi_bw_lower->textsize(9);
//fi_bw_lower->value( "1000" );
//fi_bw_lower->callback( cb_fi_bw_lower_upper, 0 );
//fi_bw_lower->when( FL_WHEN_ENTER_KEY );

//fi_bw_upper = new Fl_Input( 380, h() - 110, 60, 15, "BwUpr:" );
//fi_bw_upper->labelsize(10);
//fi_bw_upper->textsize(9);
//fi_bw_upper->value( "3000" );
//fi_bw_upper->callback( cb_fi_bw_lower_upper, (void*)1 );
//fi_bw_upper->when( FL_WHEN_ENTER_KEY );




miw_filt_lwr = new My_Input_Wheel( gp_filter->x() + 90, gp_filter->y() + 60, 40, 15, "BwLwr");
omiw = miw_filt_lwr;
omiw->labelsize(10);
omiw->textsize(9);
omiw->align(FL_ALIGN_LEFT);
omiw->tooltip( "filter lower cutoff freq" );
omiw->b_show_modified = 1;
omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
omiw->col_update();
omiw->b_take_focus_on_inside = 0;
omiw->b_use_limit_min = 1;
omiw->limit_min = 10;
omiw->b_use_limit_max = 1;
omiw->limit_max = 3.2e6/2;

omiw->b_invert_wheel = 0;				
omiw->std_wheel_step = 100;
omiw->ctrl_wheel_step = 500;
omiw->shift_wheel_step = 1000;
omiw->ctrl_shift_wheel_step = 10;

omiw->s_printf_format = "%d";
omiw->force_integer = 1;
omiw->set_value_from_double( 0 );
omiw->set_callback( (void*)cb_miw_bw_lower_upper, (void*)this, (void*)0 );
omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
omiw->id2= 1;
omiw->allow_right_but_drag = 1;
omiw->right_drag_x_val_change_factor = 0;
omiw->right_drag_y_val_change_factor = 10;






miw_filt_upr = new My_Input_Wheel( gp_filter->x() + 175, gp_filter->y() + 60, 40, 15, "BwUpr");
omiw = miw_filt_upr;
omiw->labelsize(10);
omiw->textsize(9);
omiw->align(FL_ALIGN_LEFT);
omiw->tooltip( "filter upper cutoff freq" );
omiw->b_show_modified = 1;
omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
omiw->col_update();
omiw->b_take_focus_on_inside = 0;
omiw->b_use_limit_min = 1;
omiw->limit_min = 10;
omiw->b_use_limit_max = 1;
omiw->limit_max = 3.2e6/2;

omiw->b_invert_wheel = 0;				
omiw->std_wheel_step = 100;
omiw->ctrl_wheel_step = 500;
omiw->shift_wheel_step = 1000;
omiw->ctrl_shift_wheel_step = 10;

omiw->s_printf_format = "%d";
omiw->force_integer = 1;
omiw->set_value_from_double( 0 );
omiw->set_callback( (void*)cb_miw_bw_lower_upper, (void*)this, (void*)1 );
omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
omiw->id2= 1;
omiw->allow_right_but_drag = 1;
omiw->right_drag_x_val_change_factor = 0;
omiw->right_drag_y_val_change_factor = 10;






miw_filt_taps = new My_Input_Wheel( gp_filter->x() + 250, gp_filter->y() + 60, 30, 15, "Taps");
omiw = miw_filt_taps;
omiw->labelsize(10);
omiw->textsize(9);
omiw->align(FL_ALIGN_LEFT);
omiw->tooltip( "filter length, number of coeffs (taps), more coeffs give a sharper transition region but increases computation, try 200" );
omiw->b_show_modified = 1;
omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
omiw->col_update();
omiw->b_take_focus_on_inside = 0;
omiw->b_use_limit_min = 5;
omiw->limit_min = 10;
omiw->b_use_limit_max = 1;
omiw->limit_max = 512;

omiw->b_invert_wheel = 0;				
omiw->std_wheel_step = 1;
omiw->ctrl_wheel_step = 5;
omiw->shift_wheel_step = 10;
omiw->ctrl_shift_wheel_step = 20;

omiw->s_printf_format = "%d";
omiw->force_integer = 1;
omiw->set_value_from_double( g_bw_taps );
omiw->set_callback( (void*)cb_miw_bw_lower_upper, (void*)this, (void*)2 );
omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
omiw->id2= 1;
omiw->allow_right_but_drag = 1;
omiw->right_drag_x_val_change_factor = 0;
omiw->right_drag_y_val_change_factor = 10;





//----
jj = 0;
ld_filter_memory[jj] = new GCLed( gp_filter->x() + 270, gp_filter->y() + 2, 11, 11, "" );
ld_filter_memory[jj]->id = jj;
ld_filter_memory[jj]->labelsize( 8 );
ld_filter_memory[jj]->tooltip( "recalls a filter preset, right click to store current filtering details into this preset" );
ld_filter_memory[jj]->align( FL_ALIGN_LEFT );
ld_filter_memory[jj]->led_style = cn_gcled_style_square;
ld_filter_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_filter_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_filter_memory[jj]->callback( cb_led_filter_memory_combo, (void*)jj );
ld_filter_memory[jj]->set_mouse_event_callback(cb_led_filter_mouse_event, (void*)ld_filter_memory[jj], (void*)jj );

jj++;
ld_filter_memory[jj] = new GCLed( gp_filter->x() + 270, gp_filter->y() + 2 + 10, 11, 11, "" );
ld_filter_memory[jj]->id = jj;
ld_filter_memory[jj]->labelsize( 8 );
ld_filter_memory[jj]->tooltip( "recalls a filter preset, right click to store current filtering details into this preset" );
ld_filter_memory[jj]->align( FL_ALIGN_LEFT );
ld_filter_memory[jj]->led_style = cn_gcled_style_square;
ld_filter_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_filter_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_filter_memory[jj]->callback( cb_led_filter_memory_combo, (void*)jj );
ld_filter_memory[jj]->set_mouse_event_callback(cb_led_filter_mouse_event, (void*)ld_filter_memory[jj], (void*)jj );

jj++;
ld_filter_memory[jj] = new GCLed( gp_filter->x() + 270, gp_filter->y() + 2 + 20, 11, 11, "" );
ld_filter_memory[jj]->id = jj;
ld_filter_memory[jj]->labelsize( 8 );
ld_filter_memory[jj]->tooltip( "recalls a filter preset, right click to store current filtering details into this preset" );
ld_filter_memory[jj]->align( FL_ALIGN_LEFT );
ld_filter_memory[jj]->led_style = cn_gcled_style_square;
ld_filter_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_filter_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_filter_memory[jj]->callback( cb_led_filter_memory_combo, (void*)jj );
ld_filter_memory[jj]->set_mouse_event_callback(cb_led_filter_mouse_event, (void*)ld_filter_memory[jj], (void*)jj );

jj++;
ld_filter_memory[jj] = new GCLed( gp_filter->x() + 270, gp_filter->y() + 2 + 30, 11, 11, "" );
ld_filter_memory[jj]->id = jj;
ld_filter_memory[jj]->labelsize( 8 );
ld_filter_memory[jj]->tooltip( "recalls a filter preset, right click to store current filtering details into this preset" );
ld_filter_memory[jj]->align( FL_ALIGN_LEFT );
ld_filter_memory[jj]->led_style = cn_gcled_style_square;
ld_filter_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_filter_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_filter_memory[jj]->callback( cb_led_filter_memory_combo, (void*)jj );
ld_filter_memory[jj]->set_mouse_event_callback(cb_led_filter_mouse_event, (void*)ld_filter_memory[jj], (void*)jj );

jj++;
ld_filter_memory[jj] = new GCLed( gp_filter->x() + 270, gp_filter->y() + 2 + 40, 11, 11, "" );
ld_filter_memory[jj]->id = jj;
ld_filter_memory[jj]->labelsize( 8 );
ld_filter_memory[jj]->tooltip( "recalls a filter preset, right click to store current filtering details into this preset" );
ld_filter_memory[jj]->align( FL_ALIGN_LEFT );
ld_filter_memory[jj]->led_style = cn_gcled_style_square;
ld_filter_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_filter_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_filter_memory[jj]->callback( cb_led_filter_memory_combo, (void*)jj );
ld_filter_memory[jj]->set_mouse_event_callback(cb_led_filter_mouse_event, (void*)ld_filter_memory[jj], (void*)jj );



gp_filter->end();
//---
























//bt_freq_tune = new Fl_Button( 350, h() - 30, 50, 20, "Tune" );
//bt_freq_tune->callback( cb_bt_freq_tune, 0 );


//bt_freq_tune_off = new Fl_Button( 290, h() - 30, 50, 20, "Off" );
//bt_freq_tune_off->callback( cb_bt_freq_tune_off, 0 );



//fi_tune = new My_Input_Choice( 405, h() - 30, 120, 20, "" );
//fi_tune->align( FL_ALIGN_TOP | FL_ALIGN_LEFT );
//fi_tune->tooltip("Select a freq , then hit tune");
//fi_tune->callback( cb_bt_tune_history, 0 );



//---
Fl_Group* gp_frq_mem = new Fl_Group( 690, h() - 100, 85, 25, "gp_frq_mem");
gp_frq_mem->labelsize(7);
gp_frq_mem->box( FL_BORDER_BOX );

//----
jj = 0;
ld_freq_memory[jj] = new GCLed( gp_frq_mem->x() + 2, gp_frq_mem->y() + 2, 11, 11, "" );
ld_freq_memory[jj]->id = jj;
ld_freq_memory[jj]->labelsize( 8 );
ld_freq_memory[jj]->tooltip( "recalls a stored freq (only), right click to store current tuned freq" );
ld_freq_memory[jj]->align( FL_ALIGN_LEFT );
ld_freq_memory[jj]->led_style = cn_gcled_style_square;
//ld_freq_memory[jj]->led_style = cn_gcled_style_round;
ld_freq_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_freq_memory[jj]->SetColIndex(1, 80, 255, 80);
//ld_freq_memory[jj]->set_col_from_str( "100 0 0, 255 0 0, 0 0 255", ',' );     //the thrid colour set not used in this app
ld_freq_memory[jj]->callback( cb_led_freq_memory_combo, (void*)jj );
ld_freq_memory[jj]->set_mouse_event_callback( cb_led_freq_memory_mouse_event, (void*)ld_freq_memory[jj], (void*)jj );

jj++;
ld_freq_memory[jj] = new GCLed( gp_frq_mem->x() + 12, gp_frq_mem->y() + 2, 11, 11, "" );
ld_freq_memory[jj]->id = jj;
ld_freq_memory[jj]->labelsize( 8 );
ld_freq_memory[jj]->tooltip( "recalls a stored (only), right click to store current tuned freq" );
ld_freq_memory[jj]->align( FL_ALIGN_LEFT );
ld_freq_memory[jj]->led_style = cn_gcled_style_square;
ld_freq_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_freq_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_freq_memory[jj]->callback( cb_led_freq_memory_combo, (void*)jj );
ld_freq_memory[jj]->set_mouse_event_callback( cb_led_freq_memory_mouse_event, (void*)ld_freq_memory[jj], (void*)jj );

jj++;
ld_freq_memory[jj] = new GCLed( gp_frq_mem->x() + 22, gp_frq_mem->y() + 2, 11, 11, "" );
ld_freq_memory[jj]->id = jj;
ld_freq_memory[jj]->labelsize( 8 );
ld_freq_memory[jj]->tooltip( "recalls a stored (only), right click to store current tuned freq" );
ld_freq_memory[jj]->align( FL_ALIGN_LEFT );
ld_freq_memory[jj]->led_style = cn_gcled_style_square;
ld_freq_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_freq_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_freq_memory[jj]->callback( cb_led_freq_memory_combo, (void*)jj );
ld_freq_memory[jj]->set_mouse_event_callback( cb_led_freq_memory_mouse_event, (void*)ld_freq_memory[jj], (void*)jj );

jj++;
ld_freq_memory[jj] = new GCLed( gp_frq_mem->x() + 32, gp_frq_mem->y() + 2, 11, 11, "" );
ld_freq_memory[jj]->id = jj;
ld_freq_memory[jj]->labelsize( 8 );
ld_freq_memory[jj]->tooltip( "recalls a stored (only), right click to store current tuned freq" );
ld_freq_memory[jj]->align( FL_ALIGN_LEFT );
ld_freq_memory[jj]->led_style = cn_gcled_style_square;
ld_freq_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_freq_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_freq_memory[jj]->callback( cb_led_freq_memory_combo, (void*)jj );
ld_freq_memory[jj]->set_mouse_event_callback( cb_led_freq_memory_mouse_event, (void*)ld_freq_memory[jj], (void*)jj );

jj++;
ld_freq_memory[jj] = new GCLed( gp_frq_mem->x() + 42, gp_frq_mem->y() + 2, 11, 11, "" );
ld_freq_memory[jj]->id = jj;
ld_freq_memory[jj]->labelsize( 8 );
ld_freq_memory[jj]->tooltip( "recalls a stored (only), right click to store current tuned freq" );
ld_freq_memory[jj]->align( FL_ALIGN_LEFT );
ld_freq_memory[jj]->led_style = cn_gcled_style_square;
ld_freq_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_freq_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_freq_memory[jj]->callback( cb_led_freq_memory_combo, (void*)jj );
ld_freq_memory[jj]->set_mouse_event_callback( cb_led_freq_memory_mouse_event, (void*)ld_freq_memory[jj], (void*)jj );

jj++;
ld_freq_memory[jj] = new GCLed( gp_frq_mem->x() + 52, gp_frq_mem->y() + 2, 11, 11, "" );
ld_freq_memory[jj]->id = jj;
ld_freq_memory[jj]->labelsize( 8 );
ld_freq_memory[jj]->tooltip( "recalls a stored freq (only), right click to store current tuned freq" );
ld_freq_memory[jj]->align( FL_ALIGN_LEFT );
ld_freq_memory[jj]->led_style = cn_gcled_style_square;
ld_freq_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_freq_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_freq_memory[jj]->callback( cb_led_freq_memory_combo, (void*)jj );
ld_freq_memory[jj]->set_mouse_event_callback( cb_led_freq_memory_mouse_event, (void*)ld_freq_memory[jj], (void*)jj );

jj++;
ld_freq_memory[jj] = new GCLed( gp_frq_mem->x() + 62, gp_frq_mem->y() + 2, 11, 11, "" );
ld_freq_memory[jj]->id = jj;
ld_freq_memory[jj]->labelsize( 8 );
ld_freq_memory[jj]->tooltip( "recalls a stored freq (only), right click to store current tuned freq" );
ld_freq_memory[jj]->align( FL_ALIGN_LEFT );
ld_freq_memory[jj]->led_style = cn_gcled_style_square;
ld_freq_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_freq_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_freq_memory[jj]->callback( cb_led_freq_memory_combo, (void*)jj );
ld_freq_memory[jj]->set_mouse_event_callback( cb_led_freq_memory_mouse_event, (void*)ld_freq_memory[jj], (void*)jj );

jj++;
ld_freq_memory[jj] = new GCLed( gp_frq_mem->x() + 72, gp_frq_mem->y() + 2, 11, 11, "" );
ld_freq_memory[jj]->id = jj;
ld_freq_memory[jj]->labelsize( 8 );
ld_freq_memory[jj]->tooltip( "recalls a stored freq (only), right click to store current tuned freq" );
ld_freq_memory[jj]->align( FL_ALIGN_LEFT );
ld_freq_memory[jj]->led_style = cn_gcled_style_square;
ld_freq_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_freq_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_freq_memory[jj]->callback( cb_led_freq_memory_combo, (void*)jj );
ld_freq_memory[jj]->set_mouse_event_callback( cb_led_freq_memory_mouse_event, (void*)ld_freq_memory[jj], (void*)jj );




jj++;
ld_freq_memory[jj] = new GCLed( gp_frq_mem->x() + 2, gp_frq_mem->y() + 12, 11, 11, "" );
ld_freq_memory[jj]->id = jj;
ld_freq_memory[jj]->labelsize( 8 );
ld_freq_memory[jj]->tooltip( "recalls a stored freq (only), right click to store current tuned freq" );
ld_freq_memory[jj]->align( FL_ALIGN_LEFT );
ld_freq_memory[jj]->led_style = cn_gcled_style_square;
//ld_freq_memory[jj]->led_style = cn_gcled_style_round;

ld_freq_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_freq_memory[jj]->SetColIndex(1, 80, 255, 80);
//ld_freq_memory[jj]->set_col_from_str( "100 0 0, 255 0 0, 0 0 255", ',' );     //the thrid colour set not used in this app
ld_freq_memory[jj]->callback( cb_led_freq_memory_combo, (void*)jj );
ld_freq_memory[jj]->set_mouse_event_callback( cb_led_freq_memory_mouse_event, (void*)ld_freq_memory[jj], (void*)jj );

jj++;
ld_freq_memory[jj] = new GCLed( gp_frq_mem->x() + 12, gp_frq_mem->y() + 12, 11, 11, "" );
ld_freq_memory[jj]->id = jj;
ld_freq_memory[jj]->labelsize( 8 );
ld_freq_memory[jj]->tooltip( "recalls a stored freq (only), right click to store current tuned freq" );
ld_freq_memory[jj]->align( FL_ALIGN_LEFT );
ld_freq_memory[jj]->led_style = cn_gcled_style_square;
ld_freq_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_freq_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_freq_memory[jj]->callback( cb_led_freq_memory_combo, (void*)jj );
ld_freq_memory[jj]->set_mouse_event_callback( cb_led_freq_memory_mouse_event, (void*)ld_freq_memory[jj], (void*)jj );

jj++;
ld_freq_memory[jj] = new GCLed( gp_frq_mem->x() + 22, gp_frq_mem->y() + 12, 11, 11, "" );
ld_freq_memory[jj]->id = jj;
ld_freq_memory[jj]->labelsize( 8 );
ld_freq_memory[jj]->tooltip( "recalls a stored freq (only), right click to store current tuned freq" );
ld_freq_memory[jj]->align( FL_ALIGN_LEFT );
ld_freq_memory[jj]->led_style = cn_gcled_style_square;
ld_freq_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_freq_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_freq_memory[jj]->callback( cb_led_freq_memory_combo, (void*)jj );
ld_freq_memory[jj]->set_mouse_event_callback( cb_led_freq_memory_mouse_event, (void*)ld_freq_memory[jj], (void*)jj );

jj++;
ld_freq_memory[jj] = new GCLed( gp_frq_mem->x() + 32, gp_frq_mem->y() + 12, 11, 11, "" );
ld_freq_memory[jj]->id = jj;
ld_freq_memory[jj]->labelsize( 8 );
ld_freq_memory[jj]->tooltip( "recalls a stored freq (only), right click to store current tuned freq" );
ld_freq_memory[jj]->align( FL_ALIGN_LEFT );
ld_freq_memory[jj]->led_style = cn_gcled_style_square;
ld_freq_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_freq_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_freq_memory[jj]->callback( cb_led_freq_memory_combo, (void*)jj );
ld_freq_memory[jj]->set_mouse_event_callback( cb_led_freq_memory_mouse_event, (void*)ld_freq_memory[jj], (void*)jj );

jj++;
ld_freq_memory[jj] = new GCLed( gp_frq_mem->x() + 42, gp_frq_mem->y() + 12, 11, 11, "" );
ld_freq_memory[jj]->id = jj;
ld_freq_memory[jj]->labelsize( 8 );
ld_freq_memory[jj]->tooltip( "recalls a stored freq (only), right click to store current tuned freq" );
ld_freq_memory[jj]->align( FL_ALIGN_LEFT );
ld_freq_memory[jj]->led_style = cn_gcled_style_square;
ld_freq_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_freq_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_freq_memory[jj]->callback( cb_led_freq_memory_combo, (void*)jj );
ld_freq_memory[jj]->set_mouse_event_callback( cb_led_freq_memory_mouse_event, (void*)ld_freq_memory[jj], (void*)jj );

jj++;
ld_freq_memory[jj] = new GCLed( gp_frq_mem->x() + 52, gp_frq_mem->y() + 12, 11, 11, "" );
ld_freq_memory[jj]->id = jj;
ld_freq_memory[jj]->labelsize( 8 );
ld_freq_memory[jj]->tooltip( "recalls a stored freq (only), right click to store current tuned freq" );
ld_freq_memory[jj]->align( FL_ALIGN_LEFT );
ld_freq_memory[jj]->led_style = cn_gcled_style_square;
ld_freq_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_freq_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_freq_memory[jj]->callback( cb_led_freq_memory_combo, (void*)jj );
ld_freq_memory[jj]->set_mouse_event_callback( cb_led_freq_memory_mouse_event, (void*)ld_freq_memory[jj], (void*)jj );

jj++;
ld_freq_memory[jj] = new GCLed( gp_frq_mem->x() + 62, gp_frq_mem->y() + 12, 11, 11, "" );
ld_freq_memory[jj]->id = jj;
ld_freq_memory[jj]->labelsize( 8 );
ld_freq_memory[jj]->tooltip( "recalls a stored freq (only), right click to store current tuned freq" );
ld_freq_memory[jj]->align( FL_ALIGN_LEFT );
ld_freq_memory[jj]->led_style = cn_gcled_style_square;
ld_freq_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_freq_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_freq_memory[jj]->callback( cb_led_freq_memory_combo, (void*)jj );
ld_freq_memory[jj]->set_mouse_event_callback( cb_led_freq_memory_mouse_event, (void*)ld_freq_memory[jj], (void*)jj );

jj++;
ld_freq_memory[jj] = new GCLed( gp_frq_mem->x() + 72, gp_frq_mem->y() + 12, 11, 11, "" );
ld_freq_memory[jj]->id = jj;
ld_freq_memory[jj]->labelsize( 8 );
ld_freq_memory[jj]->tooltip( "recalls a stored freq (only), right click to store current tuned freq" );
ld_freq_memory[jj]->align( FL_ALIGN_LEFT );
ld_freq_memory[jj]->led_style = cn_gcled_style_square;
ld_freq_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_freq_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_freq_memory[jj]->callback( cb_led_freq_memory_combo, (void*)jj );
ld_freq_memory[jj]->set_mouse_event_callback( cb_led_freq_memory_mouse_event, (void*)ld_freq_memory[jj], (void*)jj );

gp_frq_mem->end();
//---







//---
Fl_Group* gp_preset = new Fl_Group( 780, h() - 100, 85, 25, "gp_preset");
gp_preset->labelsize(7);
gp_preset->box( FL_BORDER_BOX );

//----
jj = 0;
ld_preset_memory[jj] = new GCLed( gp_preset->x() + 2, gp_preset->y() + 2, 11, 11, "" );
ld_preset_memory[jj]->id = jj;
ld_preset_memory[jj]->labelsize( 8 );
ld_preset_memory[jj]->tooltip( "recalls a stored preset which holds most params\nright click to store current tuned details into this preset" );
ld_preset_memory[jj]->align( FL_ALIGN_LEFT );
ld_preset_memory[jj]->led_style = cn_gcled_style_square;
ld_preset_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_preset_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_preset_memory[jj]->callback( cb_led_preset_memory_combo, (void*)jj );
ld_preset_memory[jj]->set_mouse_event_callback(cb_led_preset_mouse_event, (void*)ld_preset_memory[jj], (void*)jj );



jj++;
ld_preset_memory[jj] = new GCLed( gp_preset->x() + 12, gp_preset->y() + 2, 11, 11, "" );
ld_preset_memory[jj]->id = jj;
ld_preset_memory[jj]->labelsize( 8 );
ld_preset_memory[jj]->tooltip( "recalls a stored preset, right click to store current tuned preset" );
ld_preset_memory[jj]->align( FL_ALIGN_LEFT );
ld_preset_memory[jj]->led_style = cn_gcled_style_square;
ld_preset_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_preset_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_preset_memory[jj]->callback( cb_led_preset_memory_combo, (void*)jj );
ld_preset_memory[jj]->set_mouse_event_callback(cb_led_preset_mouse_event, (void*)ld_preset_memory[jj], (void*)jj );

jj++;
ld_preset_memory[jj] = new GCLed( gp_preset->x() + 22, gp_preset->y() + 2, 11, 11, "" );
ld_preset_memory[jj]->id = jj;
ld_preset_memory[jj]->labelsize( 8 );
ld_preset_memory[jj]->tooltip( "recalls a stored preset, right click to store current tuned preset" );
ld_preset_memory[jj]->align( FL_ALIGN_LEFT );
ld_preset_memory[jj]->led_style = cn_gcled_style_square;
ld_preset_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_preset_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_preset_memory[jj]->callback( cb_led_preset_memory_combo, (void*)jj );
ld_preset_memory[jj]->set_mouse_event_callback(cb_led_preset_mouse_event, (void*)ld_preset_memory[jj], (void*)jj );

jj++;
ld_preset_memory[jj] = new GCLed( gp_preset->x() + 32, gp_preset->y() + 2, 11, 11, "" );
ld_preset_memory[jj]->id = jj;
ld_preset_memory[jj]->labelsize( 8 );
ld_preset_memory[jj]->tooltip( "recalls a stored preset, right click to store current tuned preset" );
ld_preset_memory[jj]->align( FL_ALIGN_LEFT );
ld_preset_memory[jj]->led_style = cn_gcled_style_square;
ld_preset_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_preset_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_preset_memory[jj]->callback( cb_led_preset_memory_combo, (void*)jj );
ld_preset_memory[jj]->set_mouse_event_callback(cb_led_preset_mouse_event, (void*)ld_preset_memory[jj], (void*)jj );

jj++;
ld_preset_memory[jj] = new GCLed( gp_preset->x() + 42, gp_preset->y() + 2, 11, 11, "" );
ld_preset_memory[jj]->id = jj;
ld_preset_memory[jj]->labelsize( 8 );
ld_preset_memory[jj]->tooltip( "recalls a stored preset, right click to store current tuned preset" );
ld_preset_memory[jj]->align( FL_ALIGN_LEFT );
ld_preset_memory[jj]->led_style = cn_gcled_style_square;
ld_preset_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_preset_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_preset_memory[jj]->callback( cb_led_preset_memory_combo, (void*)jj );
ld_preset_memory[jj]->set_mouse_event_callback(cb_led_preset_mouse_event, (void*)ld_preset_memory[jj], (void*)jj );

jj++;
ld_preset_memory[jj] = new GCLed( gp_preset->x() + 52, gp_preset->y() + 2, 11, 11, "" );
ld_preset_memory[jj]->id = jj;
ld_preset_memory[jj]->labelsize( 8 );
ld_preset_memory[jj]->tooltip( "recalls a stored preset, right click to store current tuned preset" );
ld_preset_memory[jj]->align( FL_ALIGN_LEFT );
ld_preset_memory[jj]->led_style = cn_gcled_style_square;
ld_preset_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_preset_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_preset_memory[jj]->callback( cb_led_preset_memory_combo, (void*)jj );
ld_preset_memory[jj]->set_mouse_event_callback(cb_led_preset_mouse_event, (void*)ld_preset_memory[jj], (void*)jj );

jj++;
ld_preset_memory[jj] = new GCLed( gp_preset->x() + 62, gp_preset->y() + 2, 11, 11, "" );
ld_preset_memory[jj]->id = jj;
ld_preset_memory[jj]->labelsize( 8 );
ld_preset_memory[jj]->tooltip( "recalls a stored preset, right click to store current tuned preset" );
ld_preset_memory[jj]->align( FL_ALIGN_LEFT );
ld_preset_memory[jj]->led_style = cn_gcled_style_square;
ld_preset_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_preset_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_preset_memory[jj]->callback( cb_led_preset_memory_combo, (void*)jj );
ld_preset_memory[jj]->set_mouse_event_callback(cb_led_preset_mouse_event, (void*)ld_preset_memory[jj], (void*)jj );

jj++;
ld_preset_memory[jj] = new GCLed( gp_preset->x() + 72, gp_preset->y() + 2, 11, 11, "" );
ld_preset_memory[jj]->id = jj;
ld_preset_memory[jj]->labelsize( 8 );
ld_preset_memory[jj]->tooltip( "recalls a stored preset, right click to store current tuned preset" );
ld_preset_memory[jj]->align( FL_ALIGN_LEFT );
ld_preset_memory[jj]->led_style = cn_gcled_style_square;
ld_preset_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_preset_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_preset_memory[jj]->callback( cb_led_preset_memory_combo, (void*)jj );
ld_preset_memory[jj]->set_mouse_event_callback(cb_led_preset_mouse_event, (void*)ld_preset_memory[jj], (void*)jj );




jj++;
ld_preset_memory[jj] = new GCLed( gp_preset->x() + 2, gp_preset->y() + 12, 11, 11, "" );
ld_preset_memory[jj]->id = jj;
ld_preset_memory[jj]->labelsize( 8 );
ld_preset_memory[jj]->tooltip( "recalls a stored preset, right click to store current tuned preset" );
ld_preset_memory[jj]->align( FL_ALIGN_LEFT );
ld_preset_memory[jj]->led_style = cn_gcled_style_square;
ld_preset_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_preset_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_preset_memory[jj]->callback( cb_led_preset_memory_combo, (void*)jj );
ld_preset_memory[jj]->set_mouse_event_callback(cb_led_preset_mouse_event, (void*)ld_preset_memory[jj], (void*)jj );

jj++;
ld_preset_memory[jj] = new GCLed( gp_preset->x() + 12, gp_preset->y() + 12, 11, 11, "" );
ld_preset_memory[jj]->id = jj;
ld_preset_memory[jj]->labelsize( 8 );
ld_preset_memory[jj]->tooltip( "recalls a stored preset, right click to store current tuned preset" );
ld_preset_memory[jj]->align( FL_ALIGN_LEFT );
ld_preset_memory[jj]->led_style = cn_gcled_style_square;
ld_preset_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_preset_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_preset_memory[jj]->callback( cb_led_preset_memory_combo, (void*)jj );
ld_preset_memory[jj]->set_mouse_event_callback(cb_led_preset_mouse_event, (void*)ld_preset_memory[jj], (void*)jj );

jj++;
ld_preset_memory[jj] = new GCLed( gp_preset->x() + 22, gp_preset->y() + 12, 11, 11, "" );
ld_preset_memory[jj]->labelsize( 8 );
ld_preset_memory[jj]->tooltip( "recalls a stored preset, right click to store current tuned preset" );
ld_preset_memory[jj]->align( FL_ALIGN_LEFT );
ld_preset_memory[jj]->led_style = cn_gcled_style_square;
ld_preset_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_preset_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_preset_memory[jj]->callback( cb_led_preset_memory_combo, (void*)jj );
ld_preset_memory[jj]->set_mouse_event_callback(cb_led_preset_mouse_event, (void*)ld_preset_memory[jj], (void*)jj );

jj++;
ld_preset_memory[jj] = new GCLed( gp_preset->x() + 32, gp_preset->y() + 12, 11, 11, "" );
ld_preset_memory[jj]->id = jj;
ld_preset_memory[jj]->labelsize( 8 );
ld_preset_memory[jj]->tooltip( "recalls a stored preset, right click to store current tuned preset" );
ld_preset_memory[jj]->align( FL_ALIGN_LEFT );
ld_preset_memory[jj]->led_style = cn_gcled_style_square;
ld_preset_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_preset_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_preset_memory[jj]->callback( cb_led_preset_memory_combo, (void*)jj );
ld_preset_memory[jj]->set_mouse_event_callback(cb_led_preset_mouse_event, (void*)ld_preset_memory[jj], (void*)jj );

jj++;
ld_preset_memory[jj] = new GCLed( gp_preset->x() + 42, gp_preset->y() + 12, 11, 11, "" );
ld_preset_memory[jj]->id = jj;
ld_preset_memory[jj]->labelsize( 8 );
ld_preset_memory[jj]->tooltip( "recalls a stored preset, right click to store current tuned preset" );
ld_preset_memory[jj]->align( FL_ALIGN_LEFT );
ld_preset_memory[jj]->led_style = cn_gcled_style_square;
ld_preset_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_preset_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_preset_memory[jj]->callback( cb_led_preset_memory_combo, (void*)jj );
ld_preset_memory[jj]->set_mouse_event_callback(cb_led_preset_mouse_event, (void*)ld_preset_memory[jj], (void*)jj );

jj++;
ld_preset_memory[jj] = new GCLed( gp_preset->x() + 52, gp_preset->y() + 12, 11, 11, "" );
ld_preset_memory[jj]->id = jj;
ld_preset_memory[jj]->labelsize( 8 );
ld_preset_memory[jj]->tooltip( "recalls a stored preset, right click to store current tuned preset" );
ld_preset_memory[jj]->align( FL_ALIGN_LEFT );
ld_preset_memory[jj]->led_style = cn_gcled_style_square;
ld_preset_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_preset_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_preset_memory[jj]->callback( cb_led_preset_memory_combo, (void*)jj );
ld_preset_memory[jj]->set_mouse_event_callback(cb_led_preset_mouse_event, (void*)ld_preset_memory[jj], (void*)jj );

jj++;
ld_preset_memory[jj] = new GCLed( gp_preset->x() + 62, gp_preset->y() + 12, 11, 11, "" );
ld_preset_memory[jj]->id = jj;
ld_preset_memory[jj]->labelsize( 8 );
ld_preset_memory[jj]->tooltip( "recalls a stored preset, right click to store current tuned preset" );
ld_preset_memory[jj]->align( FL_ALIGN_LEFT );
ld_preset_memory[jj]->led_style = cn_gcled_style_square;
ld_preset_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_preset_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_preset_memory[jj]->callback( cb_led_preset_memory_combo, (void*)jj );
ld_preset_memory[jj]->set_mouse_event_callback(cb_led_preset_mouse_event, (void*)ld_preset_memory[jj], (void*)jj );

jj++;
ld_preset_memory[jj] = new GCLed( gp_preset->x() + 72, gp_preset->y() + 12, 11, 11, "" );
ld_preset_memory[jj]->id = jj;
ld_preset_memory[jj]->labelsize( 8 );
ld_preset_memory[jj]->tooltip( "recalls a stored preset, right click to store current tuned preset" );
ld_preset_memory[jj]->align( FL_ALIGN_LEFT );
ld_preset_memory[jj]->led_style = cn_gcled_style_square;
ld_preset_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_preset_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_preset_memory[jj]->callback( cb_led_preset_memory_combo, (void*)jj );
ld_preset_memory[jj]->set_mouse_event_callback(cb_led_preset_mouse_event, (void*)ld_preset_memory[jj], (void*)jj );


b_led_preset_memory_tooltip_show_tooltips = 1;

//---
gp_preset->end();

//---








//----

Fl_Group* gp_preset2 = new Fl_Group( 885, h() - 195, 150, 150, "gp_preset2");
gp_preset2->labelsize(7);
//gp_preset2->box( FL_BORDER_BOX );

Fl_Tabs *tb_preset2 = new Fl_Tabs( gp_preset2->x()+1, gp_preset2->y(), gp_preset2->w()-2, 150, "");
tb_preset2->labelsize( 9 );

//b_led_preset_memory_tooltip_show_tooltips = 1;

//-----

//---
	Fl_Group* gp_preset2_grp0 = new Fl_Group( tb_preset2->x() + 2, tb_preset2->y() + 15, tb_preset2->w()-4, 150, "0");
	gp_preset2_grp0->labelsize( 9 );

	Fl_Scroll* scl_preset2_scroll0;
	scl_preset2_scroll0 = new Fl_Scroll( gp_preset2_grp0->x() + 1, gp_preset2_grp0->y()+1, gp_preset2_grp0->w(), 150-17, "");
//	scl_preset2_scroll0->labelsize( 9 );

	int iled_offsx = 117;
	
	jj = 0;
	for( int i = 0; i < cn_preset_memory_max_clm; i++ )
		{
		ld_preset_memory2[jj+i] = new GCLed( scl_preset2_scroll0->x() + iled_offsx, scl_preset2_scroll0->y() + 15+2+i*13, 11, 11, "" );
		ld_preset_memory2[jj+i]->id = jj+i;
		ld_preset_memory2[jj+i]->labelsize( 8 );
		ld_preset_memory2[jj+i]->tooltip( "recalls a stored preset, right click to store current tuned details into this preset" );
		ld_preset_memory2[jj+i]->align( FL_ALIGN_LEFT );
		ld_preset_memory2[jj+i]->led_style = cn_gcled_style_square;
		ld_preset_memory2[jj+i]->SetColIndex(0, 120, 80, 80);
		ld_preset_memory2[jj+i]->SetColIndex(1, 80, 255, 80);
		ld_preset_memory2[jj+i]->callback( cb_led_preset_memory_combo2, (void*)jj+i );
		ld_preset_memory2[jj+i]->set_mouse_event_callback(cb_led_preset_mouse_event2, (void*)ld_preset_memory2[jj+i], (void*)jj+i );
		}
	scl_preset2_scroll0->end();
	gp_preset2_grp0->end();
//---



//---
	Fl_Group* gp_preset2_grp1 = new Fl_Group( tb_preset2->x() + 2, tb_preset2->y() + 15, tb_preset2->w()-4, 150, "1");
	gp_preset2_grp1->labelsize( 9 );
	
	Fl_Scroll* scl_preset2_scroll1;
	scl_preset2_scroll1 = new Fl_Scroll( gp_preset2_grp1->x() + 1, gp_preset2_grp1->y()+1, gp_preset2_grp1->w(), 150-17, "");

	jj += cn_preset_memory_max_clm;
	for( int i = 0; i < cn_preset_memory_max_clm; i++ )
		{
		ld_preset_memory2[jj+i] = new GCLed( scl_preset2_scroll1->x() + iled_offsx, scl_preset2_scroll1->y() + 15+2+i*13, 11, 11, "" );
		ld_preset_memory2[jj+i]->id = jj+i;
		ld_preset_memory2[jj+i]->labelsize( 8 );
		ld_preset_memory2[jj+i]->tooltip( "recalls a stored preset, right click to store current tuned details into this preset" );
		ld_preset_memory2[jj+i]->align( FL_ALIGN_LEFT );
		ld_preset_memory2[jj+i]->led_style = cn_gcled_style_square;
		ld_preset_memory2[jj+i]->SetColIndex(0, 120, 80, 80);
		ld_preset_memory2[jj+i]->SetColIndex(1, 80, 255, 80);
		ld_preset_memory2[jj+i]->callback( cb_led_preset_memory_combo2, (void*)jj+i );
		ld_preset_memory2[jj+i]->set_mouse_event_callback(cb_led_preset_mouse_event2, (void*)ld_preset_memory2[jj+i], (void*)jj+i );
		}
	scl_preset2_scroll1->end();
	gp_preset2_grp1->end();
//---




//---
	Fl_Group* gp_preset2_grp2 = new Fl_Group( tb_preset2->x() + 2, tb_preset2->y() + 15, tb_preset2->w()-4, 150, "2");
	gp_preset2_grp2->labelsize( 9 );
	
	Fl_Scroll* scl_preset2_scroll2;
	scl_preset2_scroll2 = new Fl_Scroll( gp_preset2_grp2->x() + 1, gp_preset2_grp2->y()+1, gp_preset2_grp2->w(), 150-17, "");

	jj += cn_preset_memory_max_clm;
	for( int i = 0; i < cn_preset_memory_max_clm; i++ )
		{
		ld_preset_memory2[jj+i] = new GCLed( scl_preset2_scroll2->x() + iled_offsx, scl_preset2_scroll2->y() + 15+2+i*13, 11, 11, "" );
		ld_preset_memory2[jj+i]->id = jj+i;
		ld_preset_memory2[jj+i]->labelsize( 8 );
		ld_preset_memory2[jj+i]->tooltip( "recalls a stored preset, right click to store current tuned details into this preset" );
		ld_preset_memory2[jj+i]->align( FL_ALIGN_LEFT );
		ld_preset_memory2[jj+i]->led_style = cn_gcled_style_square;
		ld_preset_memory2[jj+i]->SetColIndex(0, 120, 80, 80);
		ld_preset_memory2[jj+i]->SetColIndex(1, 80, 255, 80);
		ld_preset_memory2[jj+i]->callback( cb_led_preset_memory_combo2, (void*)jj+i );
		ld_preset_memory2[jj+i]->set_mouse_event_callback(cb_led_preset_mouse_event2, (void*)ld_preset_memory2[jj+i], (void*)jj+i );
		}
	scl_preset2_scroll2->end();
	gp_preset2_grp2->end();
//---





//---
	Fl_Group* gp_preset2_grp3 = new Fl_Group( tb_preset2->x() + 2, tb_preset2->y() + 15, tb_preset2->w()-4, 150, "3");
	gp_preset2_grp3->labelsize( 9 );
	
	Fl_Scroll* scl_preset2_scroll3;
	scl_preset2_scroll3 = new Fl_Scroll( gp_preset2_grp3->x() + 1, gp_preset2_grp3->y()+1, gp_preset2_grp3->w(), 150-17, "");

	jj += cn_preset_memory_max_clm;
	for( int i = 0; i < cn_preset_memory_max_clm; i++ )
		{
		ld_preset_memory2[jj+i] = new GCLed( scl_preset2_scroll3->x() + iled_offsx, scl_preset2_scroll3->y() + 15+2+i*13, 11, 11, "" );
		ld_preset_memory2[jj+i]->id = jj+i;
		ld_preset_memory2[jj+i]->labelsize( 8 );
		ld_preset_memory2[jj+i]->tooltip( "recalls a stored preset, right click to store current tuned details into this preset" );
		ld_preset_memory2[jj+i]->align( FL_ALIGN_LEFT );
		ld_preset_memory2[jj+i]->led_style = cn_gcled_style_square;
		ld_preset_memory2[jj+i]->SetColIndex(0, 120, 80, 80);
		ld_preset_memory2[jj+i]->SetColIndex(1, 80, 255, 80);
		ld_preset_memory2[jj+i]->callback( cb_led_preset_memory_combo2, (void*)jj+i );
		ld_preset_memory2[jj+i]->set_mouse_event_callback(cb_led_preset_mouse_event2, (void*)ld_preset_memory2[jj+i], (void*)jj+i );
		}
	scl_preset2_scroll3->end();
	gp_preset2_grp3->end();
//---






//---
	Fl_Group* gp_preset2_grp4 = new Fl_Group( tb_preset2->x() + 2, tb_preset2->y() + 15, tb_preset2->w()-4, 150, "4");
	gp_preset2_grp4->labelsize( 9 );
	
	Fl_Scroll* scl_preset2_scroll4;
	scl_preset2_scroll4 = new Fl_Scroll( gp_preset2_grp4->x() + 1, gp_preset2_grp4->y()+1, gp_preset2_grp4->w(), 150-17, "");

	jj += cn_preset_memory_max_clm;
	for( int i = 0; i < cn_preset_memory_max_clm; i++ )
		{
		ld_preset_memory2[jj+i] = new GCLed( scl_preset2_scroll4->x() + iled_offsx, scl_preset2_scroll4->y() + 15+2+i*13, 11, 11, "" );
		ld_preset_memory2[jj+i]->id = jj+i;
		ld_preset_memory2[jj+i]->labelsize( 8 );
		ld_preset_memory2[jj+i]->tooltip( "recalls a stored preset, right click to store current tuned details into this preset" );
		ld_preset_memory2[jj+i]->align( FL_ALIGN_LEFT );
		ld_preset_memory2[jj+i]->led_style = cn_gcled_style_square;
		ld_preset_memory2[jj+i]->SetColIndex(0, 120, 80, 80);
		ld_preset_memory2[jj+i]->SetColIndex(1, 80, 255, 80);
		ld_preset_memory2[jj+i]->callback( cb_led_preset_memory_combo2, (void*)jj+i );
		ld_preset_memory2[jj+i]->set_mouse_event_callback(cb_led_preset_mouse_event2, (void*)ld_preset_memory2[jj+i], (void*)jj+i );
		}
	scl_preset2_scroll4->end();
	gp_preset2_grp4->end();
//---





//---
	Fl_Group* gp_preset2_grp5 = new Fl_Group( tb_preset2->x() + 2, tb_preset2->y() + 15, tb_preset2->w()-4, 150, "5");
	gp_preset2_grp5->labelsize( 9 );
	
	Fl_Scroll* scl_preset2_scroll5;
	scl_preset2_scroll5 = new Fl_Scroll( gp_preset2_grp5->x() + 1, gp_preset2_grp5->y()+1, gp_preset2_grp5->w(), 150-17, "");

	jj += cn_preset_memory_max_clm;
	for( int i = 0; i < cn_preset_memory_max_clm; i++ )
		{
		ld_preset_memory2[jj+i] = new GCLed( scl_preset2_scroll5->x() + iled_offsx, scl_preset2_scroll5->y() + 15+2+i*13, 11, 11, "" );
		ld_preset_memory2[jj+i]->id = jj+i;
		ld_preset_memory2[jj+i]->labelsize( 8 );
		ld_preset_memory2[jj+i]->tooltip( "recalls a stored preset, right click to store current tuned details into this preset" );
		ld_preset_memory2[jj+i]->align( FL_ALIGN_LEFT );
		ld_preset_memory2[jj+i]->led_style = cn_gcled_style_square;
		ld_preset_memory2[jj+i]->SetColIndex(0, 120, 80, 80);
		ld_preset_memory2[jj+i]->SetColIndex(1, 80, 255, 80);
		ld_preset_memory2[jj+i]->callback( cb_led_preset_memory_combo2, (void*)jj+i );
		ld_preset_memory2[jj+i]->set_mouse_event_callback(cb_led_preset_mouse_event2, (void*)ld_preset_memory2[jj+i], (void*)jj+i );
		}
	scl_preset2_scroll5->end();
	gp_preset2_grp5->end();
//---




//---
	Fl_Group* gp_preset2_grp6 = new Fl_Group( tb_preset2->x() + 2, tb_preset2->y() + 15, tb_preset2->w()-4, 150, "6");
	gp_preset2_grp6->labelsize( 9 );
	
	Fl_Scroll* scl_preset2_scroll6;
	scl_preset2_scroll6 = new Fl_Scroll( gp_preset2_grp6->x() + 1, gp_preset2_grp6->y()+1, gp_preset2_grp6->w(), 150-17, "");

	jj += cn_preset_memory_max_clm;
	for( int i = 0; i < cn_preset_memory_max_clm; i++ )
		{
		ld_preset_memory2[jj+i] = new GCLed( scl_preset2_scroll6->x() + iled_offsx, scl_preset2_scroll6->y() + 15+2+i*13, 11, 11, "" );
		ld_preset_memory2[jj+i]->id = jj+i;
		ld_preset_memory2[jj+i]->labelsize( 8 );
		ld_preset_memory2[jj+i]->tooltip( "recalls a stored preset, right click to store current tuned details into this preset" );
		ld_preset_memory2[jj+i]->align( FL_ALIGN_LEFT );
		ld_preset_memory2[jj+i]->led_style = cn_gcled_style_square;
		ld_preset_memory2[jj+i]->SetColIndex(0, 120, 80, 80);
		ld_preset_memory2[jj+i]->SetColIndex(1, 80, 255, 80);
		ld_preset_memory2[jj+i]->callback( cb_led_preset_memory_combo2, (void*)jj+i );
		ld_preset_memory2[jj+i]->set_mouse_event_callback(cb_led_preset_mouse_event2, (void*)ld_preset_memory2[jj+i], (void*)jj+i );
		}
	scl_preset2_scroll6->end();
	gp_preset2_grp6->end();
//---



//---
	Fl_Group* gp_preset2_grp7 = new Fl_Group( tb_preset2->x() + 2, tb_preset2->y() + 15, tb_preset2->w()-4, 150, "7");
	gp_preset2_grp7->labelsize( 9 );
	
	Fl_Scroll* scl_preset2_scroll7;
	scl_preset2_scroll7 = new Fl_Scroll( gp_preset2_grp7->x() + 1, gp_preset2_grp7->y()+1, gp_preset2_grp7->w(), 150-17, "");

	jj += cn_preset_memory_max_clm;
	for( int i = 0; i < cn_preset_memory_max_clm; i++ )
		{
		ld_preset_memory2[jj+i] = new GCLed( scl_preset2_scroll7->x() + iled_offsx, scl_preset2_scroll7->y() + 15+2+i*13, 11, 11, "" );
		ld_preset_memory2[jj+i]->id = jj+i;
		ld_preset_memory2[jj+i]->labelsize( 8 );
		ld_preset_memory2[jj+i]->tooltip( "recalls a stored preset, right click to store current tuned details into this preset" );
		ld_preset_memory2[jj+i]->align( FL_ALIGN_LEFT );
		ld_preset_memory2[jj+i]->led_style = cn_gcled_style_square;
		ld_preset_memory2[jj+i]->SetColIndex(0, 120, 80, 80);
		ld_preset_memory2[jj+i]->SetColIndex(1, 80, 255, 80);
		ld_preset_memory2[jj+i]->callback( cb_led_preset_memory_combo2, (void*)jj+i );
		ld_preset_memory2[jj+i]->set_mouse_event_callback(cb_led_preset_mouse_event2, (void*)ld_preset_memory2[jj+i], (void*)jj+i );
		}
	scl_preset2_scroll7->end();
	gp_preset2_grp7->end();
//---


tb_preset2->end();

gp_preset2->end();
//------












//---
Fl_Group* gp_freq = new Fl_Group( 342, h() - 297, 230, 50, "gp_freq");
gp_freq->labelsize(7);
gp_freq->box(FL_BORDER_BOX);
gp_freq->tooltip("rtl device's onboard tuner's freq");

bx_tune = new Fl_Box(  gp_freq->x() + 22+45,  gp_freq->y() + 2, 80, 20, "118.5 MHz" );
bx_tune->labelsize( 8 );

bx_tune->tooltip("rtl device's onboard tuner's freq");



Fl_Button *bt_freq_tune_down_step = new Fl_Button( gp_freq->x() + 2+45, gp_freq->y() + 16, 18, 14, "|<" );
bt_freq_tune_down_step->labelsize(8);
bt_freq_tune_down_step->tooltip("round tuning downward");
bt_freq_tune_down_step->callback( cb_bt_freq_tune_down_up, (void*)2 );


Fl_Button *bt_freq_tune_down = new Fl_Button( gp_freq->x() + 2+45, gp_freq->y() + 33, 18, 14, "<<" );
bt_freq_tune_down->labelsize(8);
bt_freq_tune_down->tooltip("shift tuning down half a span");
bt_freq_tune_down->callback( cb_bt_freq_tune_down_up, (void*)0 );


Fl_Button *bt_freq_tune_up_step = new Fl_Button( gp_freq->x() + 104+45, gp_freq->y() + 16, 18, 14, ">|" );
bt_freq_tune_up_step->labelsize(8);
bt_freq_tune_up_step->tooltip("round tuning upward");
bt_freq_tune_up_step->callback( cb_bt_freq_tune_down_up, (void*)3 );


Fl_Button *bt_freq_tune_up = new Fl_Button( gp_freq->x() + 104+45, gp_freq->y() + 33, 18, 14, ">>" );
bt_freq_tune_up->labelsize(8);
bt_freq_tune_up->tooltip("shift tuning up half a span");
bt_freq_tune_up->callback( cb_bt_freq_tune_down_up, (void*)1 );



Fl_Button *bt_freq_lsd_zero = new Fl_Button( gp_freq->x() + 124+45, gp_freq->y() + 16, 14, 14, "." );
bt_freq_lsd_zero->labelsize(8);
bt_freq_lsd_zero->tooltip("progressively zero lower freq digits");
bt_freq_lsd_zero->callback( cb_bt_freq_tune_down_up, (void*)10 );



Fl_Button *bt_freq_round_up = new Fl_Button( gp_freq->x() + 140+45, gp_freq->y() + 16, 15, 14, "@8UpArrow" );	//this label is an up arrow
bt_freq_round_up->labelsize(-3);
bt_freq_round_up->tooltip("round cur freq upward");
bt_freq_round_up->callback( cb_bt_freq_tune_down_up, (void*)31 );



Fl_Button *bt_freq_round_dwn = new Fl_Button( gp_freq->x() - 15+45, gp_freq->y() + 16, 15, 14, "@-32UpArrow" );	//this label is a down arrow
bt_freq_round_dwn->labelsize(-3);
bt_freq_round_dwn->tooltip("round cur freq downward");
bt_freq_round_dwn->callback( cb_bt_freq_tune_down_up, (void*)30 );





Fl_Button *bt_freq_5k_minus = new Fl_Button( gp_freq->x() - 42+45, gp_freq->y() + 16, 25, 14, "<5K" );
bt_freq_5k_minus->labelsize(8);
bt_freq_5k_minus->tooltip("tune downward by 5KHz");
bt_freq_5k_minus->callback( cb_bt_freq_tune_down_up, (void*)20 );





Fl_Button *bt_freq_5k_plus = new Fl_Button( gp_freq->x() + 157+45, gp_freq->y() + 16, 25, 14, "5K>" );
bt_freq_5k_plus->labelsize(8);
bt_freq_5k_plus->tooltip("tune upward by 5KHz");
bt_freq_5k_plus->callback( cb_bt_freq_tune_down_up, (void*)21 );





miwp_tune = new My_Input_Wheel_Packable( gp_freq->x() + 22+45, gp_freq->y() + 16, 80, 28, "tune");
miwp_tune->tooltip("");
miwp_tune->labelsize(8);
miwp_tune->textsize(9);
miwp_tune->align(FL_ALIGN_BOTTOM);
//miwp_tune->tooltip( "Enter a freq, or scroll freq" );
miwp_tune->miw->b_show_modified = 1;
//miwp_tune->miw->color( fl_rgb_color( 220, 255, 220 ) );
miwp_tune->miw->col_bkg = fl_rgb_color( 220, 255, 220 );
miwp_tune->miw->color( miwp_tune->miw->col_bkg );
miwp_tune->miw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
miwp_tune->miw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
miwp_tune->miw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
miwp_tune->miw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
miwp_tune->miw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
//miwp_tune->miw->color( fl_rgb_color( 220, 255, 220 ) );
//miwp_tune->miw->col_bkg = miwp_tune->miw->color();
//miwp_tune->miw->col_bkg_hover = fl_rgb_color( 230, 255, 230 );
//miwp_tune->miw->col_bkg_focus = fl_rgb_color( 250, 220, 220 );
//miwp_tune->miw->col_bkg_hover_focus = fl_rgb_color( 250, 200, 200 );
miwp_tune->miw->b_take_focus_on_inside = 0;
miwp_tune->miw->b_use_limit_min = 1;
miwp_tune->limit_min( 0 );
miwp_tune->miw->b_use_limit_max = 1;
miwp_tune->limit_max( 2e9 );

miwp_tune->miw->b_invert_wheel = 0;				
miwp_tune->miw->std_wheel_step = 5e3;
miwp_tune->miw->ctrl_wheel_step = 10e3;
miwp_tune->miw->shift_wheel_step = 250e3;
miwp_tune->miw->ctrl_shift_wheel_step = 1e6;

miwp_tune->miw->b_step_wheel_side_left = 1;								//these are overruled if ctrl/shift keys are down
miwp_tune->miw->b_step_wheel_side_left_center = 1;
miwp_tune->miw->b_step_wheel_side_right_center = 1;
miwp_tune->miw->b_step_wheel_side_right = 1;

miwp_tune->miw->step_wheel_side_right = 100;
miwp_tune->miw->step_wheel_side_right_center = 250;
miwp_tune->miw->step_wheel_side_left_center = 5000;
miwp_tune->miw->step_wheel_side_left = 10000;


miwp_tune->miw->s_printf_format = "%d";
miwp_tune->miw->force_integer = 1;
miwp_tune->set_callback( (void*)cb_miwp_tune, miwp_tune, (void*)this );
miwp_tune->miw->set_value_from_double( 118.5e6f );
miwp_tune->set_id(0);														//additional value for callback, useful for matrix arrays of ctrls
miwp_tune->set_id2(1);
miwp_tune->miw->allow_right_but_drag = 1;
miwp_tune->miw->right_drag_x_val_change_factor = 0;
miwp_tune->miw->right_drag_y_val_change_factor = 50;
miwp_tune->bx_label->col_bkg = fl_rgb_color( 180, 180, 180 );
miwp_tune->bx_label->col_bkg_hover = fl_rgb_color( 230, 255, 230 );
miwp_tune->bx_label->col_border_hover = fl_rgb_color( 120, 120, 120 );


miwp_tune->miw->set_keydown_cb( (void*)cb_miwp_tune_keydown, (void*)0 );
miwp_tune->miw->set_keyup_cb( (void*)cb_miwp_tune_keyup, (void*)0 );


gp_freq->end();
//---








//---
Fl_Group* gp_dbg = new Fl_Group( 580, h() - 248, 555, 35, "gp_dbg - UnderDevlpmt");
gp_dbg->labelsize(7);
gp_dbg->box( FL_BORDER_BOX );



miw_dbg0 = new My_Input_Wheel( gp_dbg->x() + 102, gp_dbg->y()+1, 50, 15, "g_dbg0");
miw_dbg0->labelsize(10);
miw_dbg0->textsize(9);
miw_dbg0->align(FL_ALIGN_LEFT);
miw_dbg0->tooltip( "debug global integer" );
miw_dbg0->b_show_modified = 1;
miw_dbg0->col_bkg = fl_rgb_color( 220, 255, 220 );
miw_dbg0->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
miw_dbg0->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
miw_dbg0->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
miw_dbg0->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
miw_dbg0->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
miw_dbg0->col_update();
miw_dbg0->b_take_focus_on_inside = 0;
miw_dbg0->b_use_limit_min = 1;
miw_dbg0->limit_min = -1e9;
miw_dbg0->b_use_limit_max = 1;
miw_dbg0->limit_max = 1e9;
miw_dbg0->b_invert_wheel = 0;
miw_dbg0->std_wheel_step = 1;
miw_dbg0->ctrl_wheel_step = 2;
miw_dbg0->shift_wheel_step = 4;
miw_dbg0->ctrl_shift_wheel_step = 10;

miw_dbg0->s_printf_format = "%d";
miw_dbg0->force_integer = 1;
miw_dbg0->set_value_from_double( g_dbg0 );
miw_dbg0->set_callback( (void*)cb_miw_dbg0, miw_dbg0, (void*)this );
miw_dbg0->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
miw_dbg0->id2 = 1;
miw_dbg0->allow_right_but_drag = 1;
miw_dbg0->right_drag_x_val_change_factor = 0;
miw_dbg0->right_drag_y_val_change_factor = miw_dbg0->limit_max / 100.0f;











miw_dbg1 = new My_Input_Wheel( gp_dbg->x() + 202, gp_dbg->y()+1, 50, 15, "g_dbg1");
miw_dbg1->labelsize(10);
miw_dbg1->textsize(9);
miw_dbg1->align(FL_ALIGN_LEFT);
miw_dbg1->tooltip( "debug global integer" );
miw_dbg1->b_show_modified = 1;
miw_dbg1->col_bkg = fl_rgb_color( 220, 255, 220 );
miw_dbg1->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
miw_dbg1->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
miw_dbg1->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
miw_dbg1->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
miw_dbg1->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
miw_dbg1->col_update();
miw_dbg1->b_take_focus_on_inside = 0;
miw_dbg1->b_use_limit_min = 1;
miw_dbg1->limit_min = -1e9;
miw_dbg1->b_use_limit_max = 1;
miw_dbg1->limit_max = 1e9;
miw_dbg1->b_invert_wheel = 0;
miw_dbg1->std_wheel_step = 1;
miw_dbg1->ctrl_wheel_step = 5;
miw_dbg1->shift_wheel_step = 10;
miw_dbg1->ctrl_shift_wheel_step = 100;

miw_dbg1->s_printf_format = "%d";
miw_dbg1->force_integer = 1;
miw_dbg1->set_value_from_double( g_dbg1 );
miw_dbg1->set_callback( (void*)cb_miw_dbg1, miw_dbg1, (void*)this );
miw_dbg1->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
miw_dbg1->id2 = 1;
miw_dbg1->allow_right_but_drag = 1;
miw_dbg1->right_drag_x_val_change_factor = 0;
miw_dbg1->right_drag_y_val_change_factor = miw_dbg1->limit_max / 100.0f;






miw_dbg2 = new My_Input_Wheel( gp_dbg->x() + 302, gp_dbg->y()+1, 50, 15, "g_dbg2");
miw_dbg2->labelsize(10);
miw_dbg2->textsize(9);
miw_dbg2->align(FL_ALIGN_LEFT);
miw_dbg2->tooltip( "debug global integer" );
miw_dbg2->b_show_modified = 1;
miw_dbg2->col_bkg = fl_rgb_color( 220, 255, 220 );
miw_dbg2->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
miw_dbg2->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
miw_dbg2->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
miw_dbg2->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
miw_dbg2->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
miw_dbg2->col_update();
miw_dbg2->b_take_focus_on_inside = 0;
miw_dbg2->b_use_limit_min = 1;
miw_dbg2->limit_min = 1.01;
miw_dbg2->b_use_limit_max = 1;
miw_dbg2->limit_max = 1e9;
miw_dbg2->b_invert_wheel = 0;
miw_dbg2->std_wheel_step = 1;
miw_dbg2->ctrl_wheel_step = 0.05;
miw_dbg2->shift_wheel_step = 0.1;
miw_dbg2->ctrl_shift_wheel_step = 1;

miw_dbg2->s_printf_format = "%f";
miw_dbg2->force_integer = 0;
miw_dbg2->set_value_from_double( g_dbg2 );
miw_dbg2->set_callback( (void*)cb_miw_dbg2, miw_dbg2, (void*)this );
miw_dbg2->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
miw_dbg2->id2 = 1;
miw_dbg2->allow_right_but_drag = 1;
miw_dbg2->right_drag_x_val_change_factor = 0;
miw_dbg2->right_drag_y_val_change_factor = miw_dbg2->limit_max / 100.0f;










miw_dbg3 = new My_Input_Wheel( gp_dbg->x() + 402, gp_dbg->y()+1, 50, 15, "g_dbg3");
miw_dbg3->labelsize(10);
miw_dbg3->textsize(9);
miw_dbg3->align(FL_ALIGN_LEFT);
miw_dbg3->tooltip( "debug global integer" );
miw_dbg3->b_show_modified = 1;
miw_dbg3->col_bkg = fl_rgb_color( 220, 255, 220 );
miw_dbg3->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
miw_dbg3->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
miw_dbg3->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
miw_dbg3->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
miw_dbg3->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
miw_dbg3->col_update();
miw_dbg3->b_take_focus_on_inside = 0;
miw_dbg3->b_use_limit_min = 1;
miw_dbg3->limit_min = -1e9;
miw_dbg3->b_use_limit_max = 1;
miw_dbg3->limit_max = 1e9;
miw_dbg3->b_invert_wheel = 0;
miw_dbg3->std_wheel_step = 1;
miw_dbg3->ctrl_wheel_step = 2;
miw_dbg3->shift_wheel_step = 4;
miw_dbg3->ctrl_shift_wheel_step = 10;

miw_dbg3->s_printf_format = "%d";
miw_dbg3->force_integer = 1;
miw_dbg3->set_value_from_double( g_dbg3 );
miw_dbg3->set_callback( (void*)cb_miw_dbg3, miw_dbg3, (void*)this );
miw_dbg3->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
miw_dbg3->id2 = 1;
miw_dbg3->allow_right_but_drag = 1;
miw_dbg3->right_drag_x_val_change_factor = 0;
miw_dbg3->right_drag_y_val_change_factor = miw_dbg3->limit_max / 100.0f;










miw_dbg4 = new My_Input_Wheel( gp_dbg->x() + 502, gp_dbg->y()+1, 50, 15, "g_dbg4");
miw_dbg4->labelsize(10);
miw_dbg4->textsize(9);
miw_dbg4->align(FL_ALIGN_LEFT);
miw_dbg4->tooltip( "debug global integer" );
miw_dbg4->b_show_modified = 1;
miw_dbg4->col_bkg = fl_rgb_color( 220, 255, 220 );
miw_dbg4->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
miw_dbg4->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
miw_dbg4->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
miw_dbg4->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
miw_dbg4->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
miw_dbg4->col_update();
miw_dbg4->b_take_focus_on_inside = 0;
miw_dbg4->b_use_limit_min = 1;
miw_dbg4->limit_min = -1e9;
miw_dbg4->b_use_limit_max = 1;
miw_dbg4->limit_max = 1e9;
miw_dbg4->b_invert_wheel = 0;
miw_dbg4->std_wheel_step = 1;
miw_dbg4->ctrl_wheel_step = 2;
miw_dbg4->shift_wheel_step = 4;
miw_dbg4->ctrl_shift_wheel_step = 10;

miw_dbg4->s_printf_format = "%d";
miw_dbg4->force_integer = 1;
miw_dbg4->set_value_from_double( g_dbg4 );
miw_dbg4->set_callback( (void*)cb_miw_dbg4, miw_dbg4, (void*)this );
miw_dbg4->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
miw_dbg4->id2 = 1;
miw_dbg4->allow_right_but_drag = 1;
miw_dbg4->right_drag_x_val_change_factor = 0;
miw_dbg4->right_drag_y_val_change_factor = miw_dbg4->limit_max / 100.0f;



miw_dbg5 = new My_Input_Wheel( gp_dbg->x() + 45, gp_dbg->y() + 19, 50, 15, "g_dbg5");
miw_dbg5->labelsize(10);
miw_dbg5->textsize(9);
miw_dbg5->align(FL_ALIGN_LEFT);
miw_dbg5->tooltip( "debug global integer" );
miw_dbg5->b_show_modified = 1;
miw_dbg5->col_bkg = fl_rgb_color( 220, 255, 220 );
miw_dbg5->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
miw_dbg5->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
miw_dbg5->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
miw_dbg5->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
miw_dbg5->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
miw_dbg5->col_update();
miw_dbg5->b_take_focus_on_inside = 0;
miw_dbg5->b_use_limit_min = 1;
miw_dbg5->limit_min = -1e9;
miw_dbg5->b_use_limit_max = 1;
miw_dbg5->limit_max = 1e9;
miw_dbg5->b_invert_wheel = 0;
miw_dbg5->std_wheel_step = 1;
miw_dbg5->ctrl_wheel_step = 2;
miw_dbg5->shift_wheel_step = 4;
miw_dbg5->ctrl_shift_wheel_step = 10;

miw_dbg5->s_printf_format = "%d";
miw_dbg5->force_integer = 1;
miw_dbg5->set_value_from_double( g_dbg5 );
miw_dbg5->set_callback( (void*)cb_miw_dbg5, miw_dbg5, (void*)this );
miw_dbg5->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
miw_dbg5->id2 = 1;
miw_dbg5->allow_right_but_drag = 1;
miw_dbg5->right_drag_x_val_change_factor = 0;
miw_dbg5->right_drag_y_val_change_factor = miw_dbg5->limit_max / 100.0f;







Fl_Button *bt_dbg0 = new Fl_Button( gp_dbg->x() + 100, gp_dbg->y() + 20, 45, 15, "dbgbtn0" );
bt_dbg0->labelsize( 8 );
bt_dbg0->tooltip( "for debugging" );
bt_dbg0->callback( cb_bt_dbg_combo, (void*)0 );
bt_dbg0->when( FL_WHEN_CHANGED );

bt_dbg0 = new Fl_Button( gp_dbg->x() + 145, gp_dbg->y() + 20, 45, 15, "dbgbtn1" );
bt_dbg0->labelsize( 8 );
bt_dbg0->tooltip( "for debugging" );
bt_dbg0->callback( cb_bt_dbg_combo, (void*)1 );
bt_dbg0->when( FL_WHEN_CHANGED );

bt_dbg0 = new Fl_Button( gp_dbg->x() + 190, gp_dbg->y() + 20, 45, 15, "dbgbtn2" );
bt_dbg0->labelsize( 8 );
bt_dbg0->tooltip( "for debugging" );
bt_dbg0->callback( cb_bt_dbg_combo, (void*)2 );
bt_dbg0->when( FL_WHEN_CHANGED );

bt_dbg0 = new Fl_Button( gp_dbg->x() + 235, gp_dbg->y() + 20, 45, 15, "dbgbtn3" );
bt_dbg0->labelsize( 8 );
bt_dbg0->tooltip( "for debugging" );
bt_dbg0->callback( cb_bt_dbg_combo, (void*)3 );
bt_dbg0->when( FL_WHEN_CHANGED );








gp_dbg->end();
//---



//---
Fl_Group* gp_if_freq = new Fl_Group( 318, h() - 113, 94, 30, "gp_if_freq - UnderDevlpmt");
gp_if_freq->labelsize(7);
gp_if_freq->box( FL_BORDER_BOX );

ck_if_freq = new Fl_Check_Button( gp_if_freq->x()+1, gp_if_freq->y()+1, 47, 15, "iffreq" );
ck_if_freq->labelsize( 10 );
ck_if_freq->callback( cb_ck_if_freq, 0 );
ck_if_freq->tooltip( "Enable intermediate freq offset" );





miw_if_freq = new My_Input_Wheel( gp_if_freq->x()+45, gp_if_freq->y()+1, 47, 15, "iffreq");
omiw = miw_if_freq;

omiw->labelsize(8);
omiw->textsize(9);
omiw->align(FL_ALIGN_BOTTOM);
//omiw->tooltip( "Enter a freq, or scroll freq" );
omiw->b_show_modified = 1;
omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
omiw->col_update();
omiw->b_take_focus_on_inside = 0;
omiw->b_use_limit_min = 1;
omiw->limit_min = -2e9;
omiw->b_use_limit_max = 1;
omiw->limit_max = 2e9;

omiw->b_invert_wheel = 0;				
omiw->std_wheel_step = 1000;
omiw->ctrl_wheel_step = 100;
omiw->shift_wheel_step = 2000;
omiw->ctrl_shift_wheel_step = 10;

omiw->s_printf_format = "%d";
omiw->force_integer = 1;
omiw->set_callback( (void*)cb_miw_if_freq, omiw, (void*)this );
omiw->set_value_from_double( g_interfreq );
omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
omiw->id2 = 1;
omiw->allow_right_but_drag = 1;
omiw->right_drag_x_val_change_factor = 0;
omiw->right_drag_y_val_change_factor = 50;
//omiw->bx_label->col_bkg = fl_rgb_color( 180, 180, 180 );
//omiw->bx_label->col_border_hover = fl_rgb_color( 120, 120, 120 );

gp_if_freq->end();
//----






//----
Fl_Box *bx_label = new Fl_Box( 670, h() - 160, 200, 20, "Tune History:" );
bx_label->labelsize( 11 );

idb_tune = new input_dropbox( 680, h() - 140, 200, 20, "tune");
idb_tune->expand_max( idb_tune->w()+50, 300 );

//idb_tune->fi_text->textfont( 4 );
//idb_tune->fi_text->textsize( 12 );
//idb_tune->labelsize(12);

s1 = "Holds a history of tunings.\nEnter freq or select from dropdown.\nDropdown entries show:\n'effective tuned freq (sum of the other two)',\n'onboard tuner freq (rtl dongle's tuner freq)',\n'SubFreq(sub tuning within adc's bwidth or DevBW)'";
st += s1;

char *sztooltip = new char[ st.length()+1 ];							//v1.11,  make a static string

strncpy( sztooltip, st.c_str(), st.length() );
sztooltip[ st.length() ] = 0x0;

idb_tune->fi_text->tooltip( sztooltip );
idb_tune->bt_dropdown->tooltip( sztooltip );

idb_tune->set_callback( cb_idb_tune, (void *)idb_tune, (void *)0 );
idb_tune->fi_text->callback( (void*)cb_idb_tune_fi_text, (void*)0 );
idb_tune->fi_text->when( FL_WHEN_ENTER_KEY | FL_WHEN_NOT_CHANGED );

idb_tune->fi_text->textfont( 4 );
idb_tune->fi_text->textsize( 8 );

//idb_search->textsize(12);
idb_tune->box( FL_BORDER_BOX );
idb_tune->end();
//----










//---
//Fl_Group* gp_name = new Fl_Group( 430, h() - 175, 280, 60, "gp_name");
//gp_name->labelsize(7);
//gp_name->box( FL_BORDER_BOX );


bx_gph0_sel_sample_text = new Fl_Box(  628, h() - 213, 280, 40, "bx_gph0_sel_sample_text" );
bx_gph0_sel_sample_text->labelsize( 9 );
bx_gph0_sel_sample_text->labelcolor( FL_DARK_RED );
bx_gph0_sel_sample_text->tooltip( "selected sample details" );


bx_led_preset_hov_name = new Fl_Box(  625, h() - 174, 180, 16, "preset hov name" );
bx_led_preset_hov_name->labelsize( 8 );
bx_led_preset_hov_name->tooltip( "preset's name" );



Fl_Button *bt_freq_store = new Fl_Button( 650, h() - 140, 26, 16, "-->" );
bt_freq_store->labelsize( 8 );
bt_freq_store->tooltip( "copy current tuning to dropdown box" );
bt_freq_store->callback( cb_bt_freq_store, 0 );


fi_name = new Fl_Input( 295, h() - 140, 175, 16, "Name" );
fi_name->tooltip( "name for current tuned frequency, station name, change as req" );
fi_name->labelsize( 9 );
fi_name->textsize( 9 );
fi_name->value( "???" );
fi_name->align( FL_ALIGN_TOP );

fi_group_name = new Fl_Input( 475, h() - 140, 130, 16, "Group Name" );
fi_group_name->tooltip( "Favourites group name for current tuned frequency, change as req (not required for a preset store)" );
fi_group_name->labelsize( 9 );
fi_group_name->textsize( 9 );
fi_group_name->value( "???" );
fi_group_name->align( FL_ALIGN_TOP );


Fl_Button *bt_favourite = new Fl_Button( 610, h() - 160, 35, 16, "fav" );
bt_favourite->labelsize( 9 );
bt_favourite->tooltip("show favourite window" );
bt_favourite->callback( (void*)cb_bt_favourite, (void*)0 );



Fl_Button *bt_favourite_add = new Fl_Button( 610, h() - 140, 35, 16, "fav+" );
bt_favourite_add->labelsize( 9 );
bt_favourite_add->tooltip("add current tuning to empty favourite slot" );
bt_favourite_add->callback( (void*)cb_bt_favourite, (void*)1 );


//Fl_Button *bt_fav_store = new Fl_Button( 670, h() - 141, 50, 16, "store fav" );
//bt_fav_store->labelsize( 9 );
//bt_fav_store->tooltip("store current tuning to the sel favourite" );
//bt_fav_store->callback( (void*)cb_bt_fav_store, (void*)0 );

//gp_name->end();





//---
Fl_Group* gp_dev = new Fl_Group( 5, h() - 297, 330, 101, "gp_dev");
gp_dev->labelsize(7);
gp_dev->box( FL_BORDER_BOX );



bx_ppm_offset = new Fl_Box( gp_dev->x() + 45, gp_dev->y(), 80, 10, "PPM" );
bx_ppm_offset->labelsize( 9 );
bx_ppm_offset->tooltip( "device's reported ppm offset" );
bx_ppm_offset->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT);
//bx_ppm_offset->box( FL_BORDER_BOX );

miw_ppm_offset_dev = new My_Input_Wheel( gp_dev->x() + 45, gp_dev->y() + 10, 40, 15, "DevPPM");
miw_ppm_offset_dev->labelsize(9);
miw_ppm_offset_dev->textsize(9);
miw_ppm_offset_dev->align(FL_ALIGN_LEFT);
miw_ppm_offset_dev->tooltip( "device's ppm offset" );
miw_ppm_offset_dev->b_show_modified = 1;
miw_ppm_offset_dev->col_bkg = fl_rgb_color( 220, 255, 220 );
miw_ppm_offset_dev->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
miw_ppm_offset_dev->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
miw_ppm_offset_dev->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
miw_ppm_offset_dev->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
miw_ppm_offset_dev->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
miw_ppm_offset_dev->col_update();
miw_ppm_offset_dev->b_take_focus_on_inside = 0;
miw_ppm_offset_dev->b_use_limit_min = 1;
miw_ppm_offset_dev->limit_min = -100000;
miw_ppm_offset_dev->b_use_limit_max = 1;
miw_ppm_offset_dev->limit_max = 100000;

miw_ppm_offset_dev->b_invert_wheel = 0;				
miw_ppm_offset_dev->std_wheel_step = 2;
miw_ppm_offset_dev->ctrl_wheel_step = 50;
miw_ppm_offset_dev->shift_wheel_step = 100;
miw_ppm_offset_dev->ctrl_shift_wheel_step = 2000;

miw_ppm_offset_dev->s_printf_format = "%d";
miw_ppm_offset_dev->force_integer = 1;
miw_ppm_offset_dev->set_value_from_double( 0 );
miw_ppm_offset_dev->set_callback( (void*)cb_miw_ppm_offset_dev, miw_ppm_offset_dev, (void*)this );
miw_ppm_offset_dev->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
miw_ppm_offset_dev->id2= 1;
miw_ppm_offset_dev->allow_right_but_drag = 1;
miw_ppm_offset_dev->right_drag_x_val_change_factor = 0;
miw_ppm_offset_dev->right_drag_y_val_change_factor = 10;




bx_freq_gain = new Fl_Box( gp_dev->x() + 128, gp_dev->y(), 40, 10, "dB" );
bx_freq_gain->labelsize( 9 );
bx_freq_gain->tooltip( "device's reported gain" );

miw_freq_gain = new My_Input_Wheel( gp_dev->x() + 135, gp_dev->y() + 10, 25, 15, "DevGain");
miw_freq_gain->labelsize(9);
miw_freq_gain->textsize(9);
miw_freq_gain->align(FL_ALIGN_LEFT);
miw_freq_gain->tooltip( "device's gain" );
miw_freq_gain->b_show_modified = 1;
miw_freq_gain->col_bkg = fl_rgb_color( 220, 255, 220 );
miw_freq_gain->color( miw_freq_gain->col_bkg );
miw_freq_gain->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
miw_freq_gain->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
miw_freq_gain->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
miw_freq_gain->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
miw_freq_gain->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
miw_freq_gain->b_take_focus_on_inside = 0;
miw_freq_gain->b_use_limit_min = 1;
miw_freq_gain->limit_min = 0;
miw_freq_gain->b_use_limit_max = 1;
miw_freq_gain->limit_max = 200;
miw_freq_gain->b_invert_wheel = 0;
miw_freq_gain->std_wheel_step = 10;
miw_freq_gain->ctrl_wheel_step = 1;
miw_freq_gain->shift_wheel_step = 20;
miw_freq_gain->ctrl_shift_wheel_step = 30;

miw_freq_gain->s_printf_format = "%d";
miw_freq_gain->force_integer = 1;
miw_freq_gain->set_value_from_double( gph_scaley );
miw_freq_gain->set_callback( (void*)cb_miw_freq_gain, miw_freq_gain, (void*)this );
miw_freq_gain->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
miw_freq_gain->id2 = 1;
miw_freq_gain->allow_right_but_drag = 1;
miw_freq_gain->right_drag_x_val_change_factor = 0;
miw_freq_gain->right_drag_y_val_change_factor = miw_freq_gain->limit_max / 100.0f;





miw_gain_iq = new My_Input_Wheel( gp_dev->x() + 120, gp_dev->y() + 30, 40, 15, "IQGain");
omiw = miw_gain_iq;
omiw->labelsize(9);
omiw->textsize(9);
omiw->align(FL_ALIGN_LEFT);
omiw->tooltip( "adjust gain of IQ signal before it's fed to downsampler, filters and demodulator stages" );
omiw->b_show_modified = 1;
omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
omiw->col_update();
omiw->b_take_focus_on_inside = 0;
omiw->b_use_limit_min = 1;
omiw->limit_min = 0;
omiw->b_use_limit_max = 1;
omiw->limit_max = 1e3;

omiw->b_invert_wheel = 0;				
omiw->std_wheel_step = 0.025;
omiw->ctrl_wheel_step = 0.05;
omiw->shift_wheel_step = 0.0125;
omiw->ctrl_shift_wheel_step = 0.0001;

omiw->s_printf_format = "%.4f";
omiw->force_integer = 0;
omiw->set_value_from_double( g_gain_iq );
omiw->set_callback( (void*)cb_miw_gain_iq, (void*)this, (void*)0 );
omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
omiw->id2= 1;
omiw->allow_right_but_drag = 1;
omiw->right_drag_x_val_change_factor = 0;
omiw->right_drag_y_val_change_factor = 10;

omiw->b_step_wheel_side_left = 1;								//these are overruled if ctrl/shift keys are down
omiw->b_step_wheel_side_left_center = 1;
omiw->b_step_wheel_side_right_center = 1;
omiw->b_step_wheel_side_right = 1;

omiw->step_wheel_side_right = 0.0001;
omiw->step_wheel_side_right_center = 0.0015;
omiw->step_wheel_side_left_center = 0.025;
omiw->step_wheel_side_left = 0.05;



miw_tuning_offset = new My_Input_Wheel( gp_dev->x() + 120, gp_dev->y() + 55, 40, 15, "FrqOffs");
miw_tuning_offset->labelsize(9);
miw_tuning_offset->textsize(9);
miw_tuning_offset->align(FL_ALIGN_LEFT);
miw_tuning_offset->tooltip( "offset the tuned freq and conceal this offset on freq controls and spectrum display, can be used reduce noise/hum on decoded signal" );
miw_tuning_offset->b_show_modified = 1;
miw_tuning_offset->col_bkg = fl_rgb_color( 220, 255, 220 );
miw_tuning_offset->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
miw_tuning_offset->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
miw_tuning_offset->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
miw_tuning_offset->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
miw_tuning_offset->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
miw_tuning_offset->col_update();
miw_tuning_offset->b_take_focus_on_inside = 0;
miw_tuning_offset->b_use_limit_min = 1;
miw_tuning_offset->limit_min = -100000;
miw_tuning_offset->b_use_limit_max = 1;
miw_tuning_offset->limit_max = 100000;

miw_tuning_offset->b_invert_wheel = 0;				
miw_tuning_offset->std_wheel_step = 100;
miw_tuning_offset->ctrl_wheel_step = 10;
miw_tuning_offset->shift_wheel_step = 1;
miw_tuning_offset->ctrl_shift_wheel_step = 1000;

miw_tuning_offset->s_printf_format = "%d";
miw_tuning_offset->force_integer = 1;
miw_tuning_offset->set_value_from_double( g_tuning_offset );
miw_tuning_offset->set_callback( (void*)cb_miw_tuning_offset, miw_tuning_offset, (void*)this );
miw_tuning_offset->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
miw_tuning_offset->id2= 1;
miw_tuning_offset->allow_right_but_drag = 1;
miw_tuning_offset->right_drag_x_val_change_factor = 0;
miw_tuning_offset->right_drag_y_val_change_factor = 10;




bx_direct_sampling = new Fl_Box( gp_dev->x() + 35, gp_dev->y() + 30, 30, 10, "DS" );
bx_direct_sampling->labelsize( 9 );
bx_direct_sampling->tooltip( "device's reported direct sampling state" );


ld_direct_sampling = new GCLed( gp_dev->x() + 55, gp_dev->y() + 40, 11, 11, "DirectSmplg" );
ld_direct_sampling->labelsize( 8 );
ld_direct_sampling->tooltip( "direct sampling for HF rx if circuity is avail (bypasses RF tuner)\ndark: Off (uses on board tuner - VHF),\nyellow: I-branch(no tuner - HF if avail ),\ngreen: Q-branch(no tuner - HF if aval)\n\n** Don't use for Blog V4 dongles **" );
ld_direct_sampling->align( FL_ALIGN_LEFT );
//ld_direct_sampling->led_style = cn_gcled_style_square;
ld_direct_sampling->led_style = cn_gcled_style_round;

ld_direct_sampling->SetColIndex(0, 120, 80, 80);
ld_direct_sampling->SetColIndex(1, 255, 255, 0);
ld_direct_sampling->SetColIndex(2, 0, 255, 0);
//ld_direct_sampling->set_col_from_str( "100 0 0, 255 0 0, 0 0 255", ',' );     //the thrid colour set not used in this app
ld_direct_sampling->callback( cb_led_combo, (void*)0 );





bx_offset_tuning = new Fl_Box( gp_dev->x() + 35, gp_dev->y() + 55, 30, 10, "OT" );
bx_offset_tuning->labelsize( 9 );
bx_offset_tuning->tooltip( "device's reported offset tuning state" );

ld_offset_tuning = new GCLed( gp_dev->x() + 55, gp_dev->y() + 65, 11, 11, "OffsTuning" );
ld_offset_tuning->labelsize( 8 );
ld_offset_tuning->tooltip( "offset tuning (not supported on all dongle variants)\n\nNOTE: on Blog V4 dongles this actually turns on bias-tee (when using bias-tee: ensure no short circuit exists on arial feed)" );
ld_offset_tuning->align( FL_ALIGN_LEFT );
ld_offset_tuning->led_style = cn_gcled_style_round;

ld_offset_tuning->SetColIndex(0, 120, 80, 80);
ld_offset_tuning->SetColIndex(1, 255, 80, 80);
//ld_direct_sampling->set_col_from_str( "100 0 0, 255 0 0, 0 0 255", ',' );     //the thrid colour set not used in this app

ld_offset_tuning->callback( cb_led_combo, (void*)1 );





ld_bias_t = new GCLed( gp_dev->x() + 55, gp_dev->y() + 85, 11, 11, "BiasTee" );
ld_bias_t->labelsize( 8 );
ld_bias_t->tooltip( "turn on bias-tee voltage at rf connector\n(ensure no short circuit exists on arial feed,\na short causes excess bias-tee current\nin circuit components reducing lifespan)" );
ld_bias_t->align( FL_ALIGN_LEFT );
//ld_bias_t->led_style = cn_gcled_style_square;
ld_bias_t->led_style = cn_gcled_style_round;

ld_bias_t->SetColIndex(0, 120, 80, 80);
ld_bias_t->SetColIndex(1, 255, 0, 0);
//ld_direct_sampling->set_col_from_str( "100 0 0, 255 0 0, 0 0 255", ',' );     //the thrid colour set not used in this app
ld_bias_t->callback( cb_led_combo, (void*)3 );






bx_dev_bwidth = new Fl_Box( gp_dev->x() + 210, gp_dev->y() + 1, 100, 10, "MHz" );
bx_dev_bwidth->labelsize( 8 );
bx_dev_bwidth->tooltip( "device's reported bandwith" );


miw_dev_bwidth = new My_Input_Wheel( gp_dev->x() + 230, gp_dev->y() + 10, 50, 15, "DevBW");
miw_dev_bwidth->labelsize(9);
miw_dev_bwidth->textsize(9);
miw_dev_bwidth->align(FL_ALIGN_LEFT);
miw_dev_bwidth->tooltip( "device bandwith (this is the adc srate - the device may only set its adc srate to supported values)\n\nwill also adjust DnwSrate freq to nearest integer factor" );
miw_dev_bwidth->b_show_modified = 1;
miw_dev_bwidth->col_bkg = fl_rgb_color( 220, 255, 220 );
miw_dev_bwidth->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
miw_dev_bwidth->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
miw_dev_bwidth->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
miw_dev_bwidth->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
miw_dev_bwidth->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
miw_dev_bwidth->col_update();
miw_dev_bwidth->b_take_focus_on_inside = 0;
miw_dev_bwidth->b_use_limit_min = 1;
miw_dev_bwidth->limit_min = 0;
miw_dev_bwidth->b_use_limit_max = 1;
miw_dev_bwidth->limit_max = 4e6;

miw_dev_bwidth->b_invert_wheel = 0;				
miw_dev_bwidth->std_wheel_step = 5000;
miw_dev_bwidth->ctrl_wheel_step = 10000;
miw_dev_bwidth->shift_wheel_step = 50000;
miw_dev_bwidth->ctrl_shift_wheel_step = 100000;

miw_dev_bwidth->s_printf_format = "%d";
miw_dev_bwidth->force_integer = 1;
miw_dev_bwidth->set_value_from_double( 0 );
miw_dev_bwidth->set_callback( (void*)cb_miw_dev_bwidth, miw_dev_bwidth, (void*)this );
miw_dev_bwidth->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
miw_dev_bwidth->id2= 1;
miw_dev_bwidth->allow_right_but_drag = 1;
miw_dev_bwidth->right_drag_x_val_change_factor = 0;
miw_dev_bwidth->right_drag_y_val_change_factor = 10;






mb_dev_bwidth = new Fl_Menu_Button( gp_dev->x() + 230, gp_dev->y() + 25, 50, 16, "srate:" );
mb_dev_bwidth->labelsize(10);
mb_dev_bwidth->textsize(9);
//mb_dev_bwidth->menu( pulldown_demod_mode );
mb_dev_bwidth->callback( cb_mb_dev_bwidth, 0 );
//mb_dev_bwidth->add( "3200000",  FL_CTRL + '0', cb_mb_dev_bwidth );





ld_buf_rd_adj = new GCLed( gp_dev->x() + 312, gp_dev->y() + 12, 16, 16, "bfRd" );
ld_buf_rd_adj->labelsize( 8 );
ld_buf_rd_adj->tooltip( "flashes if a buffer read adjustment was required, this will cause an audio stutter" );
ld_buf_rd_adj->align( FL_ALIGN_LEFT );
//ld_buf_rd_adj->led_style = cn_gcled_style_square;
ld_buf_rd_adj->led_style = cn_gcled_style_round;

ld_buf_rd_adj->SetColIndex(0, 120, 80, 80);
ld_buf_rd_adj->SetColIndex(1, 255, 80, 80);
//ld_buf_rd_adj->set_col_from_str( "100 0 0, 255 0 0, 0 0 255", ',' );     //the thrid colour set not used in this app
//ld_buf_rd_adj->callback( cb_led_combo, (void*)0 );








bx_dwnconv_factor = new Fl_Box( gp_dev->x() + 215, gp_dev->y() + 54, 80, 15, "Dwn xx " );
bx_dwnconv_factor->labelsize( 8 );
bx_dwnconv_factor->tooltip( "shows downsampler's factor,\n2496000-->24000Hz (DevBW-->DwnSrate)\nwould be shown as: 104" );



miw_dev_dwnconv_srate = new My_Input_Wheel( gp_dev->x() + 230, gp_dev->y() + 65, 50, 15, "DwnSrate");
omiw = miw_dev_dwnconv_srate;
omiw->labelsize(10);
omiw->textsize(9);
omiw->align(FL_ALIGN_LEFT);
omiw->tooltip( "down sampler's o/p srate, this srate is where the selectivity filters operate, lower: reduces cpu usage, but lowers audio quality, e.g:\nAM: 12000\nFM Stereo: 320000,\n(actual downsampler srate is set to the nearest integer factor of DevBW)" );
omiw->b_show_modified = 1;
omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
omiw->col_update();
omiw->b_take_focus_on_inside = 0;
omiw->b_use_limit_min = 1;
omiw->limit_min = cn_downsample_srate_min;
omiw->b_use_limit_max = 1;
omiw->limit_max = cn_downsample_srate_max;


omiw->b_invert_wheel = 0;				
omiw->std_wheel_step = 1000;
omiw->ctrl_wheel_step = 100;
omiw->shift_wheel_step = 200;
omiw->ctrl_shift_wheel_step = 10;

omiw->s_printf_format = "%d";
omiw->force_integer = 1;
omiw->set_value_from_double( downsample_srate );
omiw->set_callback( (void*)cb_miw_dev_dwnconv_srate, miw_dev_dwnconv_srate, (void*)this );
omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
omiw->id2= 1;
omiw->allow_right_but_drag = 1;
omiw->right_drag_x_val_change_factor = 0;
omiw->right_drag_y_val_change_factor = 10;








jj = 0;
ld_dwn_srate_memory[jj] = new GCLed(gp_dev->x() + 315, gp_dev->y() + 50, 11, 11, "" );
ld_dwn_srate_memory[jj]->id = jj;
ld_dwn_srate_memory[jj]->labelsize( 8 );
ld_dwn_srate_memory[jj]->tooltip( "recalls a stored downsampler srate (DwnSrate), right click to store current DwnSrate,\n(actual downsampler srate is set to the nearest integer factor of DevBW)" );
ld_dwn_srate_memory[jj]->align( FL_ALIGN_LEFT );
ld_dwn_srate_memory[jj]->led_style = cn_gcled_style_square;
ld_dwn_srate_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_dwn_srate_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_dwn_srate_memory[jj]->callback( cb_led_dwn_srate_memory_combo, (void*)jj );
//ld_preset_memory[jj]->set_mouse_event_callback(cb_led_preset_mouse_event, (void*)ld_preset_memory[jj], (void*)jj );

jj++;
ld_dwn_srate_memory[jj] = new GCLed(gp_dev->x() + 315, gp_dev->y() + 50 + 10, 11, 11, "" );
ld_dwn_srate_memory[jj]->id = jj;
ld_dwn_srate_memory[jj]->labelsize( 8 );
ld_dwn_srate_memory[jj]->tooltip( "recalls a stored downsampler srate (DwnSrate), right click to store current DwnSrate,\n(actual downsampler srate is set to the nearest integer factor of DevBW)" );
ld_dwn_srate_memory[jj]->align( FL_ALIGN_LEFT );
ld_dwn_srate_memory[jj]->led_style = cn_gcled_style_square;
ld_dwn_srate_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_dwn_srate_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_dwn_srate_memory[jj]->callback( cb_led_dwn_srate_memory_combo, (void*)jj );
//ld_preset_memory[jj]->set_mouse_event_callback(cb_led_preset_mouse_event, (void*)ld_preset_memory[jj], (void*)jj );

jj++;
ld_dwn_srate_memory[jj] = new GCLed(gp_dev->x() + 315, gp_dev->y() + 50 + 20, 11, 11, "" );
ld_dwn_srate_memory[jj]->id = jj;
ld_dwn_srate_memory[jj]->labelsize( 8 );
ld_dwn_srate_memory[jj]->tooltip( "recalls a stored downsampler srate (DwnSrate), right click to store current DwnSrate,\n(actual downsampler srate is set to the nearest integer factor of DevBW)" );
ld_dwn_srate_memory[jj]->align( FL_ALIGN_LEFT );
ld_dwn_srate_memory[jj]->led_style = cn_gcled_style_square;
ld_dwn_srate_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_dwn_srate_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_dwn_srate_memory[jj]->callback( cb_led_dwn_srate_memory_combo, (void*)jj );
//ld_preset_memory[jj]->set_mouse_event_callback(cb_led_preset_mouse_event, (void*)ld_preset_memory[jj], (void*)jj );

jj++;
ld_dwn_srate_memory[jj] = new GCLed(gp_dev->x() + 315, gp_dev->y() + 50 + 30, 11, 11, "" );
ld_dwn_srate_memory[jj]->id = jj;
ld_dwn_srate_memory[jj]->labelsize( 8 );
ld_dwn_srate_memory[jj]->tooltip( "recalls a stored downsampler srate (DwnSrate), right click to store current DwnSrate,\n(actual downsampler srate is set to the nearest integer factor of DevBW)" );
ld_dwn_srate_memory[jj]->align( FL_ALIGN_LEFT );
ld_dwn_srate_memory[jj]->led_style = cn_gcled_style_square;
ld_dwn_srate_memory[jj]->SetColIndex(0, 120, 80, 80);
ld_dwn_srate_memory[jj]->SetColIndex(1, 80, 255, 80);
ld_dwn_srate_memory[jj]->callback( cb_led_dwn_srate_memory_combo, (void*)jj );
//ld_preset_memory[jj]->set_mouse_event_callback(cb_led_preset_mouse_event, (void*)ld_preset_memory[jj], (void*)jj );


gp_dev->end();
//---






//---
Fl_Group* gp_stats = new Fl_Group( 904, h() - 297, 291, 35, "gp_stats");
gp_stats->labelsize(7);
gp_stats->box( FL_BORDER_BOX );

bx_buf_rd_wr = new Fl_Box( gp_stats->x(), gp_stats->y(), 300, 20, "buf rw/wr" );
bx_buf_rd_wr->labelsize( 8 );
bx_buf_rd_wr->tooltip( "shows when a read or write to buffer will undeflow/overflow, which may cause an audio stutter due to IQ data starving or overflow" );


Fl_Button *bt_tops_clear = new Fl_Button( gp_stats->x()+2, gp_stats->y() + 17, 35, 16, "clear" );
bt_tops_clear->labelsize( 8 );
bt_tops_clear->tooltip( "clear 'tops' cpu usage cur average value" );
bt_tops_clear->callback( cb_bt_tops_clear, (void*)0 );

bx_tops_thread = new Fl_Box( gp_stats->x(), gp_stats->y() + 10, 300, 20, "top's cpu %" );
bx_tops_thread->labelsize( 8 );
bx_tops_thread->tooltip( "shows thread cpu utilisation, results come from a script running linux 'top' cmd, run 'tops_thread.sh' in a terminal to get these figures updated, the generated file 'tops_thread.txt' must be in same folder this app was run from" );

gp_stats->end();
//---











//---
Fl_Group* gp_disp = new Fl_Group( 5, h() - 77, 285, 75, "gp_disp");
gp_disp->labelsize(7);
gp_disp->box( FL_BORDER_BOX );




Fl_Check_Button* ck_k_graphs_enable = new Fl_Check_Button( gp_disp->x(), gp_disp->y(), 30, 15, "" );
ck_k_graphs_enable->labelsize( 10 );
ck_k_graphs_enable->callback( cb_ck_graphs_enable, 0 );
ck_k_graphs_enable->tooltip( "Enable graph refresh, turn this off to stop all graph redraws, helps when debugging cpu usage" );
ck_k_graphs_enable->value( !b_dbg_no_graph_update );


miw_gph_disp_spect_zoom_factor = new My_Input_Wheel( gp_disp->x() + 70, gp_disp->y(), 50, 15, "h-zoom");
miw_gph_disp_spect_zoom_factor->labelsize(10);
miw_gph_disp_spect_zoom_factor->textsize(9);
miw_gph_disp_spect_zoom_factor->align(FL_ALIGN_LEFT);
miw_gph_disp_spect_zoom_factor->tooltip( "inset graph h-zoom factor" );
miw_gph_disp_spect_zoom_factor->b_show_modified = 1;
miw_gph_disp_spect_zoom_factor->col_bkg = fl_rgb_color( 220, 255, 220 );
miw_gph_disp_spect_zoom_factor->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
miw_gph_disp_spect_zoom_factor->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
miw_gph_disp_spect_zoom_factor->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
miw_gph_disp_spect_zoom_factor->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
miw_gph_disp_spect_zoom_factor->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
miw_gph_disp_spect_zoom_factor->col_update();
miw_gph_disp_spect_zoom_factor->b_take_focus_on_inside = 0;
miw_gph_disp_spect_zoom_factor->b_use_limit_min = 1;
miw_gph_disp_spect_zoom_factor->limit_min = 1.0f;
miw_gph_disp_spect_zoom_factor->b_use_limit_max = 1;
miw_gph_disp_spect_zoom_factor->limit_max = 500000;

miw_gph_disp_spect_zoom_factor->b_invert_wheel = 0;				
miw_gph_disp_spect_zoom_factor->std_wheel_step = 2.5;
miw_gph_disp_spect_zoom_factor->ctrl_wheel_step = 1;
miw_gph_disp_spect_zoom_factor->shift_wheel_step = 5;
miw_gph_disp_spect_zoom_factor->ctrl_shift_wheel_step = 0.1;

miw_gph_disp_spect_zoom_factor->s_printf_format = "%.1f";
miw_gph_disp_spect_zoom_factor->force_integer = 0;
miw_gph_disp_spect_zoom_factor->set_value_from_double( disp_spect_zoom_factor );
miw_gph_disp_spect_zoom_factor->set_callback( (void*)cb_miw_disp_spect_zoom_factor, miw_gph_disp_spect_zoom_factor, (void*)this );
miw_gph_disp_spect_zoom_factor->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
miw_gph_disp_spect_zoom_factor->id2= 1;
miw_gph_disp_spect_zoom_factor->allow_right_but_drag = 1;
miw_gph_disp_spect_zoom_factor->right_drag_x_val_change_factor = 0;
miw_gph_disp_spect_zoom_factor->right_drag_y_val_change_factor = miw_gph_disp_spect_zoom_factor->limit_max / 1000000.0f;








miw_spect_avg = new My_Input_Wheel( gp_disp->x() + 150, gp_disp->y() + 0, 20, 15, "avg");
miw_spect_avg->labelsize(10);
miw_spect_avg->textsize(9);
miw_spect_avg->align(FL_ALIGN_LEFT);
miw_spect_avg->tooltip( "average spectrum using this window count, zero means no averaging will be performed" );
miw_spect_avg->b_show_modified = 1;
miw_spect_avg->col_bkg = fl_rgb_color( 220, 255, 220 );
miw_spect_avg->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
miw_spect_avg->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
miw_spect_avg->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
miw_spect_avg->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
miw_spect_avg->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
miw_spect_avg->col_update();
miw_spect_avg->b_take_focus_on_inside = 0;
miw_spect_avg->b_use_limit_min = 1;
miw_spect_avg->limit_min = 0;
miw_spect_avg->b_use_limit_max = 1;
miw_spect_avg->limit_max = cn_spec_avg_slots_max;

miw_spect_avg->b_invert_wheel = 0;				
miw_spect_avg->std_wheel_step = 1;
miw_spect_avg->ctrl_wheel_step = 1;
miw_spect_avg->shift_wheel_step = 1;
miw_spect_avg->ctrl_shift_wheel_step = 1;

miw_spect_avg->s_printf_format = "%d";
miw_spect_avg->force_integer = 1;
miw_spect_avg->set_value_from_double( ispec_avg_wnd );
miw_spect_avg->set_callback( (void*)cb_miw_spect_avg, miw_spect_avg, (void*)this );
miw_spect_avg->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
miw_spect_avg->id2= 1;
miw_spect_avg->allow_right_but_drag = 1;
miw_spect_avg->right_drag_x_val_change_factor = 0;
miw_spect_avg->right_drag_y_val_change_factor = miw_spect_avg->limit_max / 100.0f;





miw_gph_scaley = new My_Input_Wheel( gp_disp->x() + 70, gp_disp->y() + 20, 50, 15, "scleY");
miw_gph_scaley->labelsize(10);
miw_gph_scaley->textsize(9);
miw_gph_scaley->align(FL_ALIGN_LEFT);
miw_gph_scaley->tooltip( "inset graph scale y" );
miw_gph_scaley->b_show_modified = 1;
miw_gph_scaley->col_bkg = fl_rgb_color( 220, 255, 220 );
miw_gph_scaley->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
miw_gph_scaley->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
miw_gph_scaley->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
miw_gph_scaley->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
miw_gph_scaley->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
miw_gph_scaley->col_update();
miw_gph_scaley->b_take_focus_on_inside = 0;
miw_gph_scaley->b_use_limit_min = 1;
miw_gph_scaley->limit_min = 0.0f;
miw_gph_scaley->b_use_limit_max = 1;
miw_gph_scaley->limit_max = 500.0f;

miw_gph_scaley->b_invert_wheel = 0;				
miw_gph_scaley->std_wheel_step = 0.01;
miw_gph_scaley->ctrl_wheel_step = 0.005;
miw_gph_scaley->shift_wheel_step = 0.1;
miw_gph_scaley->ctrl_shift_wheel_step = 0.2;

miw_gph_scaley->s_printf_format = "%.4f";
miw_gph_scaley->force_integer = 0;
miw_gph_scaley->set_value_from_double( gph_scaley );
miw_gph_scaley->set_callback( (void*)cb_miw_gph_scaley, miw_gph_scaley, (void*)this );
miw_gph_scaley->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
miw_gph_scaley->id2= 1;
miw_gph_scaley->allow_right_but_drag = 1;
miw_gph_scaley->right_drag_x_val_change_factor = 0;
miw_gph_scaley->right_drag_y_val_change_factor = miw_gph_scaley->limit_max / 100.0f;




ch_graph_loc_sel = new Fl_Choice( gp_disp->x() + 50, gp_disp->y() + 40, 229, 18, "probe loc:");
ch_graph_loc_sel->labelsize(9); 
ch_graph_loc_sel->textsize(9); 
ch_graph_loc_sel->menu( menu_graph_loc_sel );
ch_graph_loc_sel->value( gph_loc_sel );

ch_graph_loc_sel->tooltip("sel what location in sdr chain to show on the Probe Graph,\ntriangle waveform or no trace may indicate that part of chain is not currently active");
ch_graph_loc_sel->callback( cb_ch_graph_loc_sel );



Fl_Check_Button* ck_fit_auto_plot = new Fl_Check_Button( gp_disp->x() + 70 + 105, gp_disp->y() + 12, 50, 15, "auto fit" );
ck_fit_auto_plot->labelsize( 10 );
ck_fit_auto_plot->callback( cb_ck_fit_auto_plot, 0 );
ck_fit_auto_plot->tooltip( "will auto scale y-axis to fit trace in plot wnd,\ndone after a new probe location is selected" );
ck_fit_auto_plot->value( b_fit_auto_plot );



cl_button_wheel *bt0 = new cl_button_wheel( gp_disp->x() + 50 + 184, gp_disp->y() + 6, 45, 30, "Fit" );
bt0->labelsize( 11 );
bt0->tooltip("fit trace on probe graph using y-axis scale/offset so it's visible,\nspin mousewheel within this button to further adj y-axis scale" );
bt0->callback( (void*)cb_bt_graph_fit, (void*)0 );



bx_samples_per_sec = new Fl_Box( gp_disp->x() + 0, gp_disp->y() + 60, gp_disp->w(), 15, "????" );
bx_samples_per_sec->labelsize( 9 );
bx_samples_per_sec->tooltip("probe graph data rate stats,\nhigh samples/sec will burden cpu");
bx_samples_per_sec->align( FL_ALIGN_CENTER );
gp_disp->end();
//---




//---
Fl_Group* gp_radioband = new Fl_Group( 580, h() - 297, 316, 20, "gp_radioband");
gp_radioband->labelsize(7);
gp_radioband->box( FL_BORDER_BOX );

int j = 0;
btw_band[j] = new cl_button_wheel( gp_radioband->x() + 3, gp_radioband->y() + 3, 50, 14, "160M" );
btw_band[j]->sname = "160M::1.8e3";
btw_band[j]->labelsize( 8 );
btw_band[j]->tooltip("select a predefined band, right click to modify, use a double colon :: to delineate name and a centre freq\ne.g for: 80Mtr at 3.8MHz\ndefine like so: 80Mtr::3.8e6 or 20Meter::14000000\nanything from the double colon onward will not appear in button label, it will be interpreted as a floating point freq,\nbuttons have a fixed size" );
btw_band[j]->callback( (void*)cb_btw_band, (void*)j );
btw_band[j]->when(FL_WHEN_CHANGED);

j++;
btw_band[j] = new cl_button_wheel( gp_radioband->x() + 3 + (2 + 50), gp_radioband->y() + 3, 50, 14, "80M" );
btw_band[j]->sname = "80M::3.8e6";
btw_band[j]->labelsize( 8 );
btw_band[j]->tooltip("select a predefined band, right click to modify, use a double colon :: to delineate name and a centre freq\ne.g for: 80Mtr at 3.8MHz\ndefine like so: 80Mtr::3.8e6 or 20Meter::14000000\nanything from the double colon onward will not appear in button label, it will be interpreted as a floating point freq,\nbuttons have a fixed size" );
btw_band[j]->callback( (void*)cb_btw_band, (void*)j );
btw_band[j]->when(FL_WHEN_CHANGED);


j++;
btw_band[j] = new cl_button_wheel( gp_radioband->x() + 3 + (2 + 50)*2, gp_radioband->y() + 3, 50, 14, "40M" );
btw_band[j]->sname = "40M::7e6";
btw_band[j]->labelsize( 8 );
btw_band[j]->tooltip("select a predefined band, right click to modify, use a double colon :: to delineate name and a centre freq\ne.g for: 80Mtr at 3.8MHz\ndefine like so: 80Mtr::3.8e6 or 20Meter::14000000\nanything from the double colon onward will not appear in button label, it will be interpreted as a floating point freq,\nbuttons have a fixed size" );
btw_band[j]->callback( (void*)cb_btw_band, (void*)j );
btw_band[j]->when(FL_WHEN_CHANGED);

j++;
btw_band[j] = new cl_button_wheel( gp_radioband->x() + 3 + (2 + 50)*3, gp_radioband->y() + 3, 50, 14, "20M" );
btw_band[j]->sname = "20M::14e6";
btw_band[j]->labelsize( 8 );
btw_band[j]->tooltip("select a predefined band, right click to modify, use a double colon :: to delineate name and a centre freq\ne.g for: 80Mtr at 3.8MHz\ndefine like so: 80Mtr::3.8e6 or 20Meter::14000000\nanything from the double colon onward will not appear in button label, it will be interpreted as a floating point freq,\nbuttons have a fixed size" );
btw_band[j]->callback( (void*)cb_btw_band, (void*)j );
btw_band[j]->when(FL_WHEN_CHANGED);

j++;
btw_band[j] = new cl_button_wheel( gp_radioband->x() + 3 + (2 + 50)*4, gp_radioband->y() + 3, 50, 14, "17M" );
btw_band[j]->sname = "17M::18e6";
btw_band[j]->labelsize( 8 );
btw_band[j]->tooltip("select a predefined band, right click to modify, use a double colon :: to delineate name and a centre freq\ne.g for: 80Mtr at 3.8MHz\ndefine like so: 80Mtr::3.8e6 or 20Meter::14000000\nanything from the double colon onward will not appear in button label, it will be interpreted as a floating point freq,\nbuttons have a fixed size" );
btw_band[j]->callback( (void*)cb_btw_band, (void*)j );
btw_band[j]->when(FL_WHEN_CHANGED);

j++;
btw_band[j] = new cl_button_wheel( gp_radioband->x() + 3 + (2 + 50)*5, gp_radioband->y() + 3, 50, 14, "15M" );
btw_band[j]->sname = "15M::21e6";
btw_band[j]->labelsize( 8 );
btw_band[j]->tooltip("select a predefined band, right click to modify, use a double colon :: to delineate name and a centre freq\ne.g for: 80Mtr at 3.8MHz\ndefine like so: 80Mtr::3.8e6 or 20Meter::14000000\nanything from the double colon onward will not appear in button label, it will be interpreted as a floating point freq,\nbuttons have a fixed size" );
btw_band[j]->callback( (void*)cb_btw_band, (void*)j );
btw_band[j]->when(FL_WHEN_CHANGED);

gp_radioband->end();
//---





//vtunehist2.push_back( "118500000" );
update_tune_history();







//Fl_Menu_Button *m = fi_tune->menubutton();
//last_tune_history_value = m->value();			//remember for next callabck

//fi_freq_mouse = new Fl_Input( 470, h() - 30, 90, 20, "MrkrFreq:" );
//fi_freq_mouse->value( "0" );

//---
Fl_Group* gp_demod = new Fl_Group( 887, h() - 28, 175, 27, "gp_demod");
gp_demod->labelsize(7);
gp_demod->box( FL_BORDER_BOX );


fi_demodul_mode = new Fl_Choice( gp_demod->x() + 41, gp_demod->y() + 1, 55, 16, "Demod:" );
fi_demodul_mode->labelsize( 9 );
fi_demodul_mode->textsize( 9 );
fi_demodul_mode->menu( pulldown_demod_mode );
fi_demodul_mode->callback( cb_demodul_mode, 0 );
fi_demodul_mode->tooltip("For FM Stereo, click 19K round led to set various controls to suitable values,\nthe 19k led should light for stereo decoding\n\nFor AM: set DwnSrate to 12KHz,\nturn on: DwnAA, BWFilt,\nadj filters for best sound" );


ld_fm_19k_pilot = new GCLed( gp_demod->x() + 105, gp_demod->y() + 1, 11, 11, "19K" );
ld_fm_19k_pilot->labelsize( 8 );
ld_fm_19k_pilot->tooltip( "shows if FM Stereo 19KHz pilot tone has been locked onto, click to change various controls to settings suitable for FM Stereo decoding.\n--Clicking will--\nset all filters to off,\nDwnSrate set high,\nagc set off,\nmono set off.\n(manually adj DevGain for lowest noise, try 5->10)" );
ld_fm_19k_pilot->align( FL_ALIGN_BOTTOM );
//ld_fm_19k_pilot->led_style = cn_gcled_style_square;
ld_fm_19k_pilot->led_style = cn_gcled_style_round;

ld_fm_19k_pilot->SetColIndex(0, 220, 0, 0);
ld_fm_19k_pilot->SetColIndex(1, 0, 255, 0);
ld_fm_19k_pilot->callback( (void*)cb_ld_19k, (void*)0 );


ld_fm_deemph = new GCLed( gp_demod->x() + 145, gp_demod->y() + 1, 11, 11, "DeEmph" );
ld_fm_deemph->labelsize( 8 );
ld_fm_deemph->tooltip( "Deemphasis for FM Stereo audio:\nred: off\norange: 50uS\ngreen: 75uS (USA)" );
ld_fm_deemph->align( FL_ALIGN_BOTTOM );
//ld_fm_deemph->led_style = cn_gcled_style_square;
ld_fm_deemph->led_style = cn_gcled_style_round;

ld_fm_deemph->SetColIndex(0, 220, 0, 0);
ld_fm_deemph->SetColIndex(1, 255, 200, 60);
ld_fm_deemph->SetColIndex(2, 0, 255, 0);
ld_fm_deemph->callback( (void*)cb_ld_fm_deemph, (void*)0 );
ld_fm_deemph->ChangeCol( i_deemphasis );

gp_demod->end();
//---







//---
Fl_Group* gp_aud_op = new Fl_Group( w() - 110,  h() - 160, 105, 160, "gp_aud_op");
gp_aud_op->labelsize(7);
gp_aud_op->box( FL_BORDER_BOX );


ld_agc = new GCLed( gp_aud_op->x() + 20, gp_aud_op->y()+1, 11, 11, "agc" );
ld_agc->labelsize( 8 );
ld_agc->tooltip( "shows agc gain correction level applied to audio\ngreen: gain applied,\norange: atten applied,\nclick to red to deactivate" );
ld_agc->align( FL_ALIGN_LEFT );
//ld_agc->led_style = cn_gcled_style_square;
ld_agc->led_style = cn_gcled_style_round;

ld_agc->SetColIndex(0, 220, 0, 0);
ld_agc->SetColIndex(1, 0, 0, 0);
ld_agc->ChangeCol( b_agc );
ld_agc->callback( (void*)cb_ld_agc, (void*)0 );


ld_clip = new GCLed( gp_aud_op->x() + 55, gp_aud_op->y()+1, 11, 11, "clip" );
ld_clip->labelsize( 8 );
ld_clip->callback( (void*)cb_ld_clip, (void*)0 );

ld_clip->tooltip( "shows if audio level clipper is active, white indicates audio was clipped and may sound distorted,\nleft click to toggle on/off,\nright click to change clip level, try 0.75.\n\nred: off\ngreen: on\nblack-->white: have clipped" );
ld_clip->align( FL_ALIGN_LEFT );
//ld_clip->led_style = cn_gcled_style_square;
ld_clip->led_style = cn_gcled_style_round;

ld_clip->SetColIndex(0, 220, 0, 0);
ld_clip->SetColIndex(1, 0, 255, 0);
ld_clip->ChangeCol( b_clip_enable );

//agc_vmeter = new vert_meter( gp_aud_op->x() + 3, gp_aud_op->y() + 15, 6, 100, "" );
//agc_vmeter->box( FL_DOWN_BOX );

//agc_vmeter->channels = 1;
//agc_vmeter->barwid = 2;

//agc_vmeter->set_levels( 0.85, 0 );


fvs_agc = new Fl_Slider( gp_aud_op->x() + 15, gp_aud_op->y() + 15, 10, 100, "" );
fvs_agc->labelsize( 8 );
fvs_agc->range( 1.0, -1.0f);
fvs_agc->tooltip( "shows applied agc level\ncentre: minimal change applied,\ntop half: gain applied,\nbottom half: attenuation applied" );


/*
fvs_squelch = new Fl_Slider( gp_aud_op->x() + 13, gp_aud_op->y() + 15, 20, 100, "Squel");
fvs_squelch->tooltip( "squelch" );
fvs_squelch->labelsize( 8 );
fvs_squelch->type( 0 );
fvs_squelch->range( 255.0, 0 );
fvs_squelch->value( g_squelch );
fvs_squelch->callback( cb_vs_squelch, 0 );



led_squelch = new GCLed(  gp_aud_op->x() + 6, gp_aud_op->y() + 126,  12, 12, "sqlch" );
led_squelch->labelsize(8);
led_squelch->tooltip( "squelch indication" );
led_squelch->SetColIndex( 0, 64, 0, 0 );
led_squelch->SetColIndex( 1, 255,0, 0 );
led_squelch->align( FL_ALIGN_BOTTOM );
*/





fvs_gain = new Fl_Slider( gp_aud_op->x() + 40, gp_aud_op->y() + 15, 20, 100, "Gain" );
fvs_gain->tooltip( "audio gain" );
fvs_gain->labelsize( 8 );
fvs_gain->type( 0 );
fvs_gain->range( 0.5, 0 );
fvs_gain->value( g_gain_aud );
fvs_gain->callback( cb_vs_gain, 0 );



aud_vmeter = new vert_meter( gp_aud_op->x() + 64, gp_aud_op->y() + 15, 6, 100, "" );
aud_vmeter->box( FL_DOWN_BOX );

aud_vmeter->channels = 1;
aud_vmeter->barwid = 2;

aud_vmeter->set_levels( 0.85, 0 );
aud_vmeter->tooltip( "cur audio level" );



ck_mono = new Fl_Check_Button(  gp_aud_op->x() + 53, gp_aud_op->y() + 125, 45, 17, "Mono" );
ck_mono->labelsize(10);
ck_mono->tooltip( "mono audio" );
ck_mono->callback( cb_ck_aud_mute_combo, (void*)1 );
ck_mono->value( g_aud_mono );


Fl_Check_Button *ck = new Fl_Check_Button(  gp_aud_op->x() + 53, gp_aud_op->y() + 140, 45, 17, "Mute" );
ck->labelsize(10);
ck->tooltip( "mute audio" );
ck->callback( cb_ck_aud_mute_combo, (void*)0 );
ck->value( g_aud_mute );






ld_aud_plus_db = new GCLed( gp_aud_op->x() + 80, gp_aud_op->y() + 60, 11, 11, "+10dB" );
ld_aud_plus_db->labelsize( 8 );
ld_aud_plus_db->tooltip( "increase audio gain by fixed amount" );
ld_aud_plus_db->align( FL_ALIGN_BOTTOM );
ld_aud_plus_db->led_style = cn_gcled_style_round;

ld_aud_plus_db->SetColIndex(0, 150, 80, 80);
ld_aud_plus_db->SetColIndex(1, 80, 255, 80);

ld_aud_plus_db->callback( cb_ld_aud_gain_plus_minus_db, (void*)0 );






ld_aud_minus_db = new GCLed( gp_aud_op->x() + 80, gp_aud_op->y() + 90, 11, 11, "-10dB" );
ld_aud_minus_db->labelsize( 8 );
ld_aud_minus_db->tooltip( "decrease audio gain by fixed amount" );
ld_aud_minus_db->align( FL_ALIGN_BOTTOM );
ld_aud_minus_db->led_style = cn_gcled_style_round;

ld_aud_minus_db->SetColIndex(0, 150, 80, 80);
ld_aud_minus_db->SetColIndex(1, 80, 255, 80);

ld_aud_minus_db->callback( cb_ld_aud_gain_plus_minus_db, (void*)1 );




gp_aud_op->end();
//---


//fvs_vert = new Fl_Slider( w() - 130, h() - 100, 20, 80, "VertGain");
//fvs_vert->type( 0 );
//fvs_horiz = new Fl_Slider( 800,  h() - 120, 130, 20, "HorizGain");
//fvs_horiz->type( 1 );




//ck_show_demod = new Fl_Check_Button( 10, h() - 110, 100, 20, "ShowDemod" );
//ck_show_demod->tooltip( "Show demodulated spectrum" );








//Fl_Check_Button *cb_ck_direct_sampling = new Fl_Check_Button( 600, h() - 150, 100, 16, "DirectSmplg" );
//cb_ck_direct_sampling->labelsize( 8 );
//cb_ck_direct_sampling->callback( cb_ck_combo, 0 );
//cb_ck_direct_sampling->tooltip( "direct sampling" );



fvs_play_pos = new Fl_Slider( w() - 165, h() - 211, 100, 15, "Play" );
fvs_play_pos->tooltip( "file play position, slide to change play point" );
fvs_play_pos->labelsize( 8 );
fvs_play_pos->type( 1 );
fvs_play_pos->range( 0, 240 );
fvs_play_pos->value( 0 );
fvs_play_pos->callback( cb_fvs_play_pos, 0 );
fvs_play_pos->align( FL_ALIGN_LEFT );



ld_rec_play_synth = new GCLed( w() - 65, h() - 213, 20, 20, "Synth" );
ld_rec_play_synth->labelsize( 8 );
ld_rec_play_synth->tooltip( "mode: Synthesis(lilac), Rec(red) or Play(green)\nscroll mousewheel to change play pos while playing" );
ld_rec_play_synth->align( FL_ALIGN_BOTTOM );
ld_rec_play_synth->led_style = cn_gcled_style_square;

ld_rec_play_synth->SetColIndex(0, 120, 80, 80);
ld_rec_play_synth->SetColIndex(1, 255, 80, 80);
ld_rec_play_synth->SetColIndex(2, 80, 255, 80);
ld_rec_play_synth->SetColIndex(3, 202, 163, 252 );
//ld_rec_play_synth->set_col_from_str( "100 0 0, 255 0 0, 0 0 255", ',' );     //the thrid colour set not used in this app

ld_rec_play_synth->callback( cb_ld_rec_play_synth, (void*)1 );
ld_rec_play_synth->set_mouse_event_callback( cb_ld_rec_play_synth_event, ld_rec_play_synth, 0 );





//bx_signal_level = new Fl_Box( 655, h() - 188, 50, 10, "SigLev 0" );
//bx_signal_level->labelsize( 8 );
//bx_signal_level->tooltip( "current tuned carrier's signal level" );





//---
Fl_Group* gp_carriers = new Fl_Group( 430, h() - 113, 240, 45, "gp_carriers - UnderDevlpmt");
gp_carriers->labelsize(7);
gp_carriers->box( FL_BORDER_BOX );


bx_carrier_max_tune_threshold_count = new Fl_Box( gp_carriers->x() + 125, gp_carriers->y(), 50, 10, "found 0" );
bx_carrier_max_tune_threshold_count->labelsize( 8 );
bx_carrier_max_tune_threshold_count->tooltip( "carriers found above the threshold (need to turn on spectrum averaging by setting 'avg' above zero)" );



ld_carrier_max_tune = new GCLed( gp_carriers->x() + 60, gp_carriers->y() + 12, 11, 11, "TuneHighCarr" );
ld_carrier_max_tune->labelsize( 8 );
ld_carrier_max_tune->tooltip( "tune to highest carrier above set threshold" );
ld_carrier_max_tune->align( FL_ALIGN_LEFT );
//ld_carrier_max_tune->led_style = cn_gcled_style_square;
ld_carrier_max_tune->led_style = cn_gcled_style_round;

ld_carrier_max_tune->SetColIndex(0, 80, 120, 80);
ld_carrier_max_tune->SetColIndex(1, 80, 255, 80);
//ld_direct_sampling->set_col_from_str( "100 0 0, 255 0 0, 0 0 255", ',' );     //the thrid colour set not used in this app
ld_carrier_max_tune->callback( cb_led_combo, (void*)2 );




bx_carrier_lev = new Fl_Box( gp_carriers->x() + 40, gp_carriers->y() + 27, 170, 10, "max carrier" );
bx_carrier_lev->labelsize( 9 );
bx_carrier_lev->tooltip( "max carrier level being received and its frequency" );




miw_carrier_max_tune_threshold = new My_Input_Wheel( gp_carriers->x() + 125, gp_carriers->y() + 10, 40, 15, "ThrshHld");
miw_carrier_max_tune_threshold->labelsize(10);
miw_carrier_max_tune_threshold->textsize(9);
miw_carrier_max_tune_threshold->align(FL_ALIGN_LEFT);
miw_carrier_max_tune_threshold->tooltip( "tune to highest carrier if above this threshold (need to turn on spectrum averaging by setting 'avg' above zero)" );
miw_carrier_max_tune_threshold->b_show_modified = 1;
miw_carrier_max_tune_threshold->col_bkg = fl_rgb_color( 220, 255, 220 );
miw_carrier_max_tune_threshold->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
miw_carrier_max_tune_threshold->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
miw_carrier_max_tune_threshold->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
miw_carrier_max_tune_threshold->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
miw_carrier_max_tune_threshold->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
miw_carrier_max_tune_threshold->col_update();
miw_carrier_max_tune_threshold->b_take_focus_on_inside = 0;
miw_carrier_max_tune_threshold->b_use_limit_min = 1;
miw_carrier_max_tune_threshold->limit_min = 0.0f;
miw_carrier_max_tune_threshold->b_use_limit_max = 1;
miw_carrier_max_tune_threshold->limit_max = 100.0f;

miw_carrier_max_tune_threshold->b_invert_wheel = 0;				
miw_carrier_max_tune_threshold->std_wheel_step = 0.05;
miw_carrier_max_tune_threshold->ctrl_wheel_step = 0.1;
miw_carrier_max_tune_threshold->shift_wheel_step = 0.25;
miw_carrier_max_tune_threshold->ctrl_shift_wheel_step = 0.001;

miw_carrier_max_tune_threshold->b_step_wheel_side_left = 1;								//these are overruled if ctrl/shift keys are down
miw_carrier_max_tune_threshold->b_step_wheel_side_left_center = 1;
miw_carrier_max_tune_threshold->b_step_wheel_side_right_center = 1;
miw_carrier_max_tune_threshold->b_step_wheel_side_right = 1;

miw_carrier_max_tune_threshold->step_wheel_side_right = 0.001;
miw_carrier_max_tune_threshold->step_wheel_side_right_center = 0.050;
miw_carrier_max_tune_threshold->step_wheel_side_left_center = 0.1;
miw_carrier_max_tune_threshold->step_wheel_side_left = 0.25;

miw_carrier_max_tune_threshold->s_printf_format = "%.3f";
miw_carrier_max_tune_threshold->force_integer = 0;
miw_carrier_max_tune_threshold->set_value_from_double( g_carrier_max_tune_threshold );
miw_carrier_max_tune_threshold->set_callback( (void*)cb_miw_carrier_max_tune_threshold, miw_carrier_max_tune_threshold, (void*)this );
miw_carrier_max_tune_threshold->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
miw_carrier_max_tune_threshold->id2= 1;
miw_carrier_max_tune_threshold->allow_right_but_drag = 1;
miw_carrier_max_tune_threshold->right_drag_x_val_change_factor = 0;
miw_carrier_max_tune_threshold->right_drag_y_val_change_factor = miw_carrier_max_tune_threshold->limit_max / 100.0f;


Fl_Button *bt_carrier_max_tune_exclusion_plus = new Fl_Button( gp_carriers->x() + 170, gp_carriers->y() + 10, 14, 14, "+" );
bt_carrier_max_tune_exclusion_plus->labelsize( 8 );
bt_carrier_max_tune_exclusion_plus->tooltip( "add current tuned freq to the max carrier exclusion list" );
bt_carrier_max_tune_exclusion_plus->callback( cb_bt_carrier_max_tune_exclusion_combo,  (void*)0 );


Fl_Button *bt_carrier_max_tune_exclusion_minus = new Fl_Button( gp_carriers->x() + 190, gp_carriers->y() + 10, 14, 14, "-" );
bt_carrier_max_tune_exclusion_minus->labelsize( 8 );
bt_carrier_max_tune_exclusion_minus->tooltip( "remove current tuned freq from the max carrier exclusion list" );
bt_carrier_max_tune_exclusion_minus->callback( cb_bt_carrier_max_tune_exclusion_combo, (void*)1 );


Fl_Button *bt_carrier_max_tune_exclusion_clear = new Fl_Button( gp_carriers->x() + 210, gp_carriers->y() + 10, 14, 14, "cl" );
bt_carrier_max_tune_exclusion_clear->labelsize( 8 );
bt_carrier_max_tune_exclusion_clear->tooltip( "clear exclusion list" );
bt_carrier_max_tune_exclusion_clear->callback( cb_bt_carrier_max_tune_exclusion_combo, (void*)2 );

gp_carriers->end();
//------





//---------------
int ikpwh = 17;			//keypad button wid/hei 
int ikp_xoffs = 3;
int ikp_yoffs = 53;
Fl_Group *gp_keypad = new Fl_Group( 342, h() - 248, 230, 72, "" );
gp_keypad->labelsize(7);
gp_keypad->box( FL_BORDER_BOX );

bx_freq_actual = new Fl_Box( gp_keypad->x() + 90, gp_keypad->y() + 27, 170, 40, "0.00Hz" );
bx_freq_actual->labelsize( 16 );
bx_freq_actual->tooltip( "actual tuned freq, onboard tuner freq + SubFreq" );



Fl_Button *btkp = new Fl_Button( gp_keypad->x() + ikp_xoffs, gp_keypad->y() + ikp_yoffs, ikpwh, ikpwh, "0" );
btkp->labelsize( 8 );
btkp->tooltip( "" );
btkp->callback( cb_bt_keypad, (void*)0 );


//---
btkp = new Fl_Button( gp_keypad->x() + ikp_xoffs + ikpwh, gp_keypad->y() + ikp_yoffs, ikpwh, ikpwh, "." );
btkp->labelsize( 8 );
btkp->tooltip( "" );
btkp->callback( cb_bt_keypad, (void*)10 );


btkp = new Fl_Button( gp_keypad->x() + ikp_xoffs + 2*ikpwh, gp_keypad->y() + ikp_yoffs, ikpwh, ikpwh, "C" );
btkp->labelsize( 8 );
btkp->tooltip( "" );
btkp->callback( cb_bt_keypad, (void*)11 );



bx_keypad_number = new Fl_Box( gp_keypad->x() + ikp_xoffs + 3*ikpwh + 2, gp_keypad->y() + ikp_yoffs, ikpwh+10, ikpwh, "1.2345" );
bx_keypad_number->labelsize( 7 );
bx_keypad_number->tooltip( "" );


btkp = new Fl_Button( gp_keypad->x() + ikp_xoffs + 4*ikpwh + 15, gp_keypad->y() + ikp_yoffs, ikpwh+0, ikpwh, "/3" );
btkp->labelsize( 8 );
btkp->tooltip( "" );
btkp->callback( cb_bt_keypad, (void*)32 );


btkp = new Fl_Button( gp_keypad->x() + ikp_xoffs + 5*ikpwh + 15, gp_keypad->y() + ikp_yoffs, ikpwh+0, ikpwh, "x3" );
btkp->labelsize( 8 );
btkp->tooltip( "" );
btkp->callback( cb_bt_keypad, (void*)33 );
//---


//---
btkp = new Fl_Button( gp_keypad->x() + ikp_xoffs, gp_keypad->y() + ikp_yoffs - ikpwh, ikpwh, ikpwh, "1" );
btkp->labelsize( 8 );
btkp->tooltip( "" );
btkp->callback( cb_bt_keypad, (void*)1 );


btkp = new Fl_Button( gp_keypad->x() + ikp_xoffs + ikpwh, gp_keypad->y() + ikp_yoffs - ikpwh, ikpwh, ikpwh, "2" );
btkp->labelsize( 8 );
btkp->tooltip( "" );
btkp->callback( cb_bt_keypad, (void*)2 );


btkp = new Fl_Button( gp_keypad->x() + ikp_xoffs + 2*ikpwh, gp_keypad->y() + ikp_yoffs - ikpwh, ikpwh, ikpwh, "3" );
btkp->labelsize( 8 );
btkp->tooltip( "" );
btkp->callback( cb_bt_keypad, (void*)3 );


btkp = new Fl_Button( gp_keypad->x() + ikp_xoffs + 3*ikpwh + 2, gp_keypad->y() + ikp_yoffs - ikpwh, ikpwh*2-6, ikpwh, "GHz" );
btkp->labelsize( 8 );
btkp->tooltip( "" );
btkp->callback( cb_bt_keypad, (void*)22 );



btkp = new Fl_Button( gp_keypad->x() + ikp_xoffs + 4*ikpwh + 15, gp_keypad->y() + ikp_yoffs - ikpwh, ikpwh+0, ikpwh, "/2" );
btkp->labelsize( 8 );
btkp->tooltip( "" );
btkp->callback( cb_bt_keypad, (void*)30 );


btkp = new Fl_Button( gp_keypad->x() + ikp_xoffs + 5*ikpwh + 15, gp_keypad->y() + ikp_yoffs - ikpwh, ikpwh+0, ikpwh, "x2" );
btkp->labelsize( 8 );
btkp->tooltip( "" );
btkp->callback( cb_bt_keypad, (void*)31 );
//---


//---
btkp = new Fl_Button( gp_keypad->x() + ikp_xoffs, gp_keypad->y() + ikp_yoffs - 2*ikpwh, ikpwh, ikpwh, "4" );
btkp->labelsize( 8 );
btkp->tooltip( "" );
btkp->callback( cb_bt_keypad, (void*)4 );

btkp = new Fl_Button( gp_keypad->x() + ikp_xoffs + ikpwh, gp_keypad->y() + ikp_yoffs - 2*ikpwh, ikpwh, ikpwh, "5" );
btkp->labelsize( 8 );
btkp->tooltip( "" );
btkp->callback( cb_bt_keypad, (void*)5 );


btkp = new Fl_Button( gp_keypad->x() + ikp_xoffs + 2*ikpwh, gp_keypad->y() + ikp_yoffs - 2*ikpwh, ikpwh, ikpwh, "6" );
btkp->labelsize( 8 );
btkp->tooltip( "" );
btkp->callback( cb_bt_keypad, (void*)6 );


btkp = new Fl_Button( gp_keypad->x() + ikp_xoffs + 3*ikpwh + 2, gp_keypad->y() + ikp_yoffs - 2*ikpwh, ikpwh*2-6, ikpwh, "MHz" );
btkp->labelsize( 8 );
btkp->tooltip( "" );
btkp->callback( cb_bt_keypad, (void*)21 );
//---


//---
btkp = new Fl_Button( gp_keypad->x() + ikp_xoffs, gp_keypad->y() + ikp_yoffs - 3*ikpwh, ikpwh, ikpwh, "7" );
btkp->labelsize( 8 );
btkp->tooltip( "" );
btkp->callback( cb_bt_keypad, (void*)7 );

btkp = new Fl_Button( gp_keypad->x() + ikp_xoffs + ikpwh, gp_keypad->y() + ikp_yoffs - 3*ikpwh, ikpwh, ikpwh, "8" );
btkp->labelsize( 8 );
btkp->tooltip( "" );
btkp->callback( cb_bt_keypad, (void*)8 );


btkp = new Fl_Button( gp_keypad->x() + ikp_xoffs + 2*ikpwh, gp_keypad->y() + ikp_yoffs - 3*ikpwh, ikpwh, ikpwh, "9" );
btkp->labelsize( 8 );
btkp->tooltip( "" );
btkp->callback( cb_bt_keypad, (void*)9 );



btkp = new Fl_Button( gp_keypad->x() + ikp_xoffs + 3*ikpwh + 2, gp_keypad->y() + ikp_yoffs - 3*ikpwh, ikpwh*2-6, ikpwh, "KHz" );
btkp->labelsize( 8 );
btkp->tooltip( "" );
btkp->callback( cb_bt_keypad, (void*)20 );




cl_rotary_knob *rk = new cl_rotary_knob( gp_keypad->x() + 85, gp_keypad->y(), 33, 33, "" );
rk->labelsize( 8 );
//rk->align( FL_ALIGN_LEFT );
rk->dimple_w = 4;
rk->dimple_h = 4;
rk->show_bkgd = 0;
rk->show_border = 0;
rk->show_user_multiplier_text = 1;
rk->user_multiplier_fontsize = 6;

rk->set_theta_degrees( 0 );
rk->callback( cb_rotary_knob_freq, (void*)0 );
rk->set_mouse_button_changed_callback( cb_rotary_knob_mouse_button,  (void*)rk, (void*)0 );
rk->user_multiplier = 1000;
//rk->tooltip( "adj tune freq" );

//---

//btkp->callback( cb_bt_carrier_max_tune_exclusion_combo, (void*)0 );



miw_freq_sub_tune = new My_Input_Wheel( gp_keypad->x() + 178, gp_keypad->y() + 2, 50, 15, "SubFreq");
omiw = miw_freq_sub_tune;
omiw->labelsize(10);
omiw->textsize(9);
omiw->align(FL_ALIGN_LEFT);
omiw->tooltip( "tunes within the rtl adc's supplied bandwidth, this is centred to the onboard tuner's freq (it does not change the onboard tuner freq)" );
omiw->b_show_modified = 1;
omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
omiw->color( omiw->col_bkg );
omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
omiw->b_take_focus_on_inside = 0;
omiw->b_use_limit_min = 1;
omiw->limit_min = -2e9;
omiw->b_use_limit_max = 1;
omiw->limit_max = 2e9;
omiw->b_invert_wheel = 0;
omiw->std_wheel_step = 100;
omiw->ctrl_wheel_step = 1000;
omiw->shift_wheel_step = 50000;
omiw->ctrl_shift_wheel_step = 10;

omiw->s_printf_format = "%d";
omiw->force_integer = 1;
omiw->set_value_from_double( g_freq_sub_tune );
omiw->set_callback( (void*)cb_miw_freq_sub_tune, miw_freq_sub_tune, (void*)this );
omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
omiw->id2 = 1;
omiw->allow_right_but_drag = 1;
omiw->right_drag_x_val_change_factor = 0;
omiw->right_drag_y_val_change_factor = omiw->limit_max / 100.0f;



Fl_Button* bt = new Fl_Button(gp_keypad->x() + 179, gp_keypad->y() + 18, 48, 9, "" );
bt->labelsize( 8 );
bt->tooltip( "zero SubFreq" );
bt->callback( cb_bt_keypad, (void*)40 );



gp_keypad->end();
//---------------



/*
Fl_JPEG_Image *jpg = new Fl_JPEG_Image( "earthmap.jpg" );      // load jpeg image into ram
printf("Image dimensions w=%d, h=%d, depth=%d\n", jpg->w(), jpg->h(), jpg->d() );

Fl_Box *bx_wtfall0 = new Fl_Box( 50, h() - 350, jpg->w(), jpg->h() );
bx_wtfall0->image( jpg );
*/


wfall0_posy = h() - 686;
wfall0_hei = 380;

wfall0 = new cl_waterfall( 10, wfall0_posy, w() - 20, wfall0_hei );
wfall0->set_left_click_cb( cb_wfall0_set_left_click_cb, this );
wfall0->set_mousemove_cb( cb_wfall0_mousemove_cb, this );
wfall0->set_mousewheel_cb( cb_wfall0_mousewheel_cb, this );

//allocate a fixed size
for( int i = 0; i < cn_spec_avg_slots_max; i++ )
	{
	vspect_avg[ i ].resize( cn_vspec_avg_size_max );
	}



gph1.copy_label( "gph1" );
//gph1.hide();



make_dev_bwidth_list();



this->remove(gph0);														//reorder widgets so graticle graph (gph0) will sit over wfall when 'gph0' widget is shifted down to reveal menu bar
this->add(gph0);


end();



//----
st_favourite_freq_tag o;
vfav.clear();

o.sgroup = "Noaa Weather Sats";
o.sname = "Noaa 15";
o.sname_long = "NOAA 15 [B]";
o.scomment0 = "Listen for sat's apt tick tock signal.\nDifferent images are transmitted depending on day/night time pass.";
o.sworld_coord = "40° 0' 0''  22° 0' 0''";
o.freq = 137620000;
o.freq_center = 137620000;
o.demod_type = en_dmt_am;
o.b_use_iffreq = 1;
o.if_freq = 10000;
o.iffreq_bw_low = 80;
o.iffreq_bw_high = 5000;
o.dev_srate = 2496000;
o.dev_gain = 50;
o.dev_ppm = 0;
o.audio_gain = 1.0;
o.tuned_count = 0;
o.sdev_name = "dev name in here";
o.sdev_manufacturer = "dev manufacturer in here";
o.sdev_prod = "dev prod in here";
o.sdev_serial = "dev serial in here";

vfav.push_back( o );


o.sgroup = "Noaa Weather Sats";
o.sname = "Noaa 18";
o.sname_long = "NOAA 18 [B]";
o.scomment0 = "Listen for sat's apt tick tock signal.\nDifferent images are transmitted depending on day/night time pass.";
o.sworld_coord = "40° 0' 0''  22° 0' 0''";
o.freq = 137912500;
o.freq_center = 137912500;
o.demod_type = en_dmt_am;
o.b_use_iffreq = 1;
o.if_freq = 10000;
o.iffreq_bw_low = 80;
o.iffreq_bw_high = 5000;
o.dev_srate = 2496000;
o.dev_gain = 50;
o.dev_ppm = 0;
o.audio_gain = 1.0;
o.tuned_count = 0;
o.sdev_name = "dev name in here";
o.sdev_manufacturer = "dev manufacturer in here";
o.sdev_prod = "dev prod in here";
o.sdev_serial = "dev serial in here";

vfav.push_back( o );



o.sgroup = "Noaa Weather Sats";
o.sname = "Noaa 19";
o.sname_long = "NOAA 19 [+]";
o.scomment0 = "Listen for sat's apt tick tock signal.\nDifferent images are transmitted depending on day/night time pass.";
o.sworld_coord = "40° 0' 0''  22° 0' 0''";
o.freq = 137100000;
o.freq_center = 137100000;
o.demod_type = en_dmt_am;
o.b_use_iffreq = 1;
o.if_freq = 10000;
o.iffreq_bw_low = 80;
o.iffreq_bw_high = 5000;
o.dev_srate = 2496000;
o.dev_gain = 50;
o.dev_ppm = 0;
o.audio_gain = 1.0;
o.tuned_count = 0;
o.sdev_name = "dev name in here";
o.sdev_manufacturer = "dev manufacturer in here";
o.sdev_prod = "dev prod in here";
o.sdev_serial = "dev serial in here";

vfav.push_back( o );

//wnd_fav->set_via_vector( vfav, sz_demodulator_type );

//----


wnd_fav = new cl_favourite_wnd( 100, 100, 1900, 500, "Frequency Favourites" );
wnd_fav->callback( (Fl_Callback *)cb_wnd_fav, wnd_fav );
wnd_fav->icon( img_icon_fav );
wnd_fav->resizable( wnd_fav->fl_scrol);


wnd_aa = new aa_wnd( 70, 200, 1400, 1000, "aa window" );
wnd_aa->end();
wnd_aa->show();


wnd_aud_spect = new cl_aud_spect_wnd( 30, 700, 400, 210, "Audio Spectrum Notcher" );
wnd_aud_spect->resizable( wnd_aud_spect );
wnd_aud_spect->end();
wnd_aud_spect->show();
wnd_aud_spect->icon( img_icon_ntc );





morse_build();



{
	
st_filter_sweep_tag os;
os.state = 1;

os.filter_type = 1;
os.oiir = &usr_lpf_iir0_I0;

os.srate = aud_op_srate;
os.freq_start = 5;
os.freq_stop = 8000;
os.freq_probe_idx = 0;

os.cycles_per_step = 10;
//os.freq_cur = os.freq_start;
os.vfreq.clear();
os.vampl.clear();
os.osc_amp = 1.0f;
//os.osc_theta0 = 0.0f;



float freq = os.freq_start;
float freq_step = 0.1;

if( freq < 10 ) freq_step = 0.1;										//variable freq step reso
if( freq < 100 )  freq_step = 1;
if( freq < 1000 )  freq_step = 10;
if( freq < 10000 )  freq_step = 100;
 
for( int i = 0; ; i++ )													//build freq list
	{
	os.vfreq_probe.push_back( freq );

	freq += freq_step;		
	if( freq >= os.freq_stop ) break;
	}
	
vfilt_sweep.push_back( os );

}


resizable( this );
}













void filter_test_sweep_plot()
{

string s1;
s1 = " plot_gph1()";
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

gph1.plot_vfloat_2( vgph1_y0, vgph1_y1 );


gph1.user_marker_add( 0, vgph1_vuser_marker );							//add freq details, call 'user_marker_idx_add()'  and lastly 'user_marker_show()' to trig a redraw
gph1.user_marker_idx_add( 0, vgph1_user_marker_idx );					//this must be as big as 'st_pnt', each plot point must have an index to 'vgph1_vuser_marker' for correct display, call 'user_marker_show()' lastly to trig a redraw
gph1.user_marker_show( 0, 1 );											//make visible, this finally calls 'plot_grph_internal()'


gph1.show();
}







/*
void filter_test_sweep()
{
printf( "filter_test_sweep()\n" );

filters_create();

st_filter_sweep_tag os;
os.state = 1;

os.filter_type = 1;
os.oiir = &usr_lpf_iir0_Q0;
os.ofir = &usr_demod_bpf_fir_I0;

os.srate = 48000;
os.freq_start = 10;
os.freq_stop = 15000;
os.freq_probe_idx = 0;

os.cycles_per_step = 100;
//os.freq_cur = os.freq_start;
os.vfreq.clear();
os.vampl.clear();
os.osc_amp = 1.0f;
//os.osc_theta0 = 0.0f;

float freq = os.freq_start;
float freq_step = 0.1;

if( freq < 10 ) freq_step = 0.1;										//variable freq step reso
if( freq < 100 )  freq_step = 1;
if( freq < 1000 )  freq_step = 10;
if( freq < 10000 )  freq_step = 100;
 
for( int i = 0; ; i++ )													//build freq list
	{
	os.vfreq_probe.push_back( freq );

	freq += freq_step;		
	if( freq >= os.freq_stop ) break;
	}

vfilt_sweep.push_back( os );

filter_sweep( 0, 200 );

filter_test_sweep_plot();


vgph1_x.clear();
vgph1_y0.clear();
vgph1_y1.clear();

vgph1_vuser_marker.clear();
vgph1_user_marker_idx.clear();



printf( "filter_sweep() - running: 'iAppRet=Fl::run()'\n" );
int iAppRet=Fl::run();

}
*/














//set 'num_of_steps_to_probe' to a large number to probe all 'vfreq_probe[]' entries in one call

bool filter_sweep( unsigned int idx, unsigned int num_of_steps_to_probe )
{
string s1;

bool vb = 1;

printf( "filter_sweep()\n" );

if( idx >= vfilt_sweep.size() ) return 0;

st_filter_sweep_tag os = vfilt_sweep[idx];

if( ( os.state == 0 ) || ( os.state == 3 ) ) return 1;

if( os.freq_probe_idx >= os.vfreq_probe.size() ) return 0;				//nothing left to do?



if( vb )
	{
	for( int i = 0; i <  os.vfreq_probe.size(); i++ )					//loop for x number of cycles' samples
		{
		printf( "filter_sweep() - [%d] freq %f\n", i, os.vfreq_probe[i] );
		}
	}



float dt = 1.0f / os.srate;

for( int step = 0; step < num_of_steps_to_probe; step++ )
	{

	float theta0 = 0.0f;
	float freq_cur = os.vfreq_probe[ os.freq_probe_idx ];

	float theta_inc = freq_cur * twopi * dt;							//angular 'velocity'


	float tim_per_cycle = 1.0f / freq_cur;								//time for one sinewave cycle

	float tim = tim_per_cycle * os.cycles_per_step;						//time require to produce x sinewave cycles

//	float sample = os.cycles_per_step * freq_cur;

	int cnt = tim / dt;													//number of srate samples required to probe x number of sinewave cycles:  'os.cycles_per_step'

	float sum = 0;

	st_mgraph_user_marker_tag ou;

	strpf( s1, "%.2f Hz", freq_cur );

	ou.s0 = s1;
	ou.val0 = freq_cur;
	ou.font0 = 4;
	ou.font_size0 = 10;
	ou.sel_offx0 = 5;
	ou.sel_offy0 = -5;
	
	ou.col0_r = 140;
	ou.col0_g = 104;
	ou.col0_b = 0;


	vgph1_vuser_marker.push_back( ou );									//added an entry 
//	gph1.gph[0]->user_marker_set_curr_idx( gph1.gph[0]->vuser_marker.size() - 1 );	//add what index to store in new 'st_pnt' entries


	float peak = 0;
	for( int i = 0; i < cnt; i++ )										//loop for x number of cycles' samples
		{
		float f1;
		float f0 = os.osc_amp * sinf( theta0 );
		if( os.filter_type == 0 )
			{
//if( i < 100 )	printf( "filter_sweep() - cnt %d [%d], freq_cur %f  dt %f  theta0 %f\n", cnt, i, freq_cur, dt, theta0 );

			f1 = filter_code::iir_process( *os.oiir, f0 );
//f1 = f0;	
			}


		if( os.filter_type == 1 )
			{
			filter_code::fir_in( *os.ofir, f0 );
			f1 = filter_code::fir_out( *os.ofir );
			}


		float f2 = fabsf( f1 );										//dc rectifify (diode)

		if( f2 > peak )	peak = f2;									//peak detector (diode/cap)

//			vgph1_y0.push_back( f2 );
//			vgph1_y1.push_back( peak );

		sum += peak;												//avg sum							

		peak -= 0.000001;											//decay factor (res)			


		theta0 += theta_inc;
		
		if( theta0 >= twopi  ) theta0 -= twopi;


		vgph1_x.push_back( i );
//		vgph1_y0.push_back( f1 );
		}

	
	sum /= cnt;															//average	

	vgph1_y0.push_back( sum );
	vgph1_y1.push_back( sum );
	vgph1_user_marker_idx.push_back( vgph1_vuser_marker.size() - 1 );

	os.vampl.push_back( sum );
	
	if( vgph1_y0.size() == 1 ) vgph1_y0[0] = 0.0f;						//ensure a zero sample for graph auto scaling to be resonable
	
	os.freq_probe_idx++;
	if( os.freq_probe_idx >= os.vfreq_probe.size() ) os.state = 3;

	if( ( os.state == 0 ) || ( os.state == 3 ) ) break;
	}


vfilt_sweep[idx] = os;


return 1;
}












bool dbg_do_once = 1;




//----------------------------------------------------------------------
aa_wnd::aa_wnd( int xx, int yy, int wid, int hei, const char *label ) : Fl_Double_Window( xx, yy, wid, hei,label )
{
left_button = middle_button = right_button = 0;
ctrl_key = shift_key = 0;
mousewheel = 0;

float aa_freq0 = 0.1f;
float aa_theta0 = 0.0f;
float aa_phse0 = 0;
float aa_freq1 = 0.1f;
float aa_theta1 = 0.0f;
float aa_phse1 = 0;



which_meter = 1;			//0: s-meter,   1: lillipu meter

//----
//s-meter
if( which_meter == 0 )
	{
	scale_factor = 0.25;
	//scale_factor = 0.125;
	//scale_factor = 0.0625;
	int kern_size = 15;

	bf0 = 0;
	bf1 = 0;
	bmp0 = 0;
	bmp1 = 0;

	bx_info = new Fl_Box( w() - 180,  50, 100, 20, "info shown here" );

	jpg = new Fl_JPEG_Image( "s-meter2.jpg" );      // load jpeg image into ram
	//jpg = new Fl_JPEG_Image( "zzznice_meter0.jpg" );      // load jpeg image into ram
	printf("aa_wnd::aa_wnd() - Image dimensions w %d  h %d  depth %d  ld %d\n", jpg->w(), jpg->h(), jpg->d(), jpg->ld() );


	int ww = jpg->w();
	int hh = jpg->h();
	int bytes_per_pixel = jpg->d();

	bf0 = new unsigned char[ ww * hh * bytes_per_pixel ];
	bf1 = new unsigned char[ ww * hh * bytes_per_pixel ];


	cnvs = new aa_canvas();

	int layer0 = 0;
	int layer1 = 1;
	int layer2 = 2;
	int layer3 = 3;

	int filter_kernel_type = en_afkt_blackman_harris;
	
	cnvs->create_layer( layer0, ww, hh, bytes_per_pixel, filter_kernel_type, kern_size );					//alloc mem for layer
	cnvs->create_layer( layer1, ww, hh, bytes_per_pixel, filter_kernel_type, kern_size );
	cnvs->create_layer( layer2, ww, hh, bytes_per_pixel, filter_kernel_type, kern_size );
	cnvs->create_layer( layer3, ww, hh, bytes_per_pixel, filter_kernel_type, kern_size );



	int srcx_in = 0;
	int srcy_in = 0;
	int linedx = ww;
	int destx = 0;
	int desty = 0;

	unsigned int rendered_wid;
	unsigned int rendered_hei;

	cnvs->block_set( layer0, (unsigned char*)*jpg->data(), 0, 0, ww, hh );	//full size to layer 0 (master copy)

	//cnvs->block_fill( layer0, 0, 0, ww, hh, 255, 100, 80 );


	cnvs->block_duplicate( layer0, layer1 );								//full size working copy to layer 1, gnomon needle will be drawn to this


	//cnvs->set_pixel25_layer( layer0, 170, 170, 255, 255, 255, 1 );

	cnvs->plot_pixel_multiple( layer0, 170, 170, 255, 255, 255, 7 );

	//cnvs->set_pixel25_layer( layer0, 165, 170, 255, 255, 255 );


	//cnvs->line_plot( layer0, ww/2-150, 10, ww/2, hh - 50, 237, 170, 33, linedx, 1 );

	//cnvs->line_plot( layer0, 20, 10, ww/2, hh - 50, 237, 170, 33, linedx, 1 );


	//cnvs->line_plot( layer1, ww/2 - 5, 2, ww/2, hh + 50, 237, 170, 33, linedx );

	//cnvs->line_plot( layer1, ww*(3.9f/4) - 5, 2, ww/2, hh + 50, 237, 170, 33, linedx );

	//cnvs->set_pixel_layer( layer0, 100, 100, 255, 255, 255 );

	float a1 = 10;

	float theta = 3/4.0f*pi;
	//int x0 = a0*cosf( theta );
	//int y0 = a0*sinf( theta );

	//theta = 3/4.0f*pi;
	//int x1 = a0*cosf( theta );
	//int y1 = a0*sinf( theta );

	for( int i = 0; i < 300; i++ )
		{
		theta = pi * 0.395 + i/300.0f * 0.85/4.0f*pi;
		
		int x0 = ww/2;
		int y0 = hh + 1220;

	//	theta = 0.1*pi;
		float a0 = 1700;
		int x1 = x0 + a0*cosf( theta );
		int y1 = y0 + -a0*sinf( theta );

	//	cnvs->set_pixel25_layer( layer0, x1, y1, 255, 255, 255 );



		theta = pi * 0.435 + i/300.0f * 0.5/4.0f*pi;
		
		x0 = ww/2;
		y0 = hh + 1660;

		a0 = 1700;
		x1 = x0 + a0*cosf( theta );
		y1 = y0 + -a0*sinf( theta );

	//	cnvs->set_pixel25_layer( layer0, x1, y1, 255, 255, 255 );
		}

	cnvs->block_get( layer0, bf0, 0, 0, ww, hh );

	float scle1 = scale_factor;

	//cnvs->filter_scale_block( layer0, bf0, srcx_in, srcy_in, ww, hh, linedx, bf1, destx, desty, linedx, 0.25, 15,  bytes_per_pixel, rendered_wid, rendered_hei );
	cnvs->block_filter_scale_layer( layer0, srcx_in, srcy_in, layer2, destx, desty, scle1,  bytes_per_pixel, rendered_wid, rendered_hei );	//downsample to layer2

	 
	cnvs->block_get( layer2, bf1, 0, 0, rendered_wid, rendered_hei );		//move layer2 pixels into bf1

	cnvs->block_duplicate( layer2, layer3 );								//downsampled working copy to layer3, downsampled gnomon needle pixel rects will be placed on over layer3

	//cnvs->set_layer_details( layer3, rendered_wid, rendered_hei, linedx, bytes_per_pixel );


	//Fl_Bitmap *bmp = new Fl_Bitmap( (const char*)jpg->data(), ww, hh );
	bmp0 = new Fl_RGB_Image( (unsigned char*)bf0, ww, hh, bytes_per_pixel );
	bmp1 = new Fl_RGB_Image( (unsigned char*)bf1, rendered_wid, rendered_hei, bytes_per_pixel );

	//printf("bmp %x\n", bmp );

	Fl_Scroll *scl_bmp0 = new Fl_Scroll( 1, 1, w() - 2, h() - 2, "" );
		bx_image0 = new Fl_Box( 2, 2, ww, hh );
		bx_image0->image( bmp0 );
	scl_bmp0->end();
		
	bx_image1 = new Fl_Box( 2, 2 + 2 + hei/2, wid, hei/2 );
	bx_image1->image( bmp1 );
	bx_image1->box( FL_BORDER_BOX );
	//delete[] bf0;
	//delete[] bf1;


	string sname = "gnomon_point.txt";
	poly0.posx = 150;
	poly0.posy = 150;

	float scle = 0.5f;
	int bkeep_coords_positive = 1;
	int binvert_x = 0;
	int binvert_y = 1;
	float rotate_theta = 0;//pi/4;
	int cntr_x = 0;
	int cntr_y = 0;

	if( !cnvs->polygon_file_load( sname, poly0, scle, rotate_theta, bkeep_coords_positive, binvert_x, binvert_y, 255, 255, 255, 0, 0 ) )
		{
		printf("failed to load polygon file '%s'\n", sname.c_str() );
		}
	else{
		cnvs->polygon_copy( poly0, poly1 );
		
		
		cnvs->polygon_rotate( poly1, pi/4, 1.0, cntr_x, cntr_y, kern_size/2, kern_size/2 );
		
	//	cnvs->plot_polygon_filled( layer0, poly1, poly1.posx, poly1.posy, 0.25f, 255, 255, 255 );
		}
	}
//----







//----
//lilliput-meter
if( which_meter == 1 )
	{
	scale_factor = 0.25;
	//scale_factor = 0.125;
	//scale_factor = 0.0625;

	int kern_size = 15;

	bf0 = 0;
	bf1 = 0;
	bmp0 = 0;
	bmp1 = 0;

	bx_info = new Fl_Box( w() - 180,  50, 100, 20, "info shown here" );




	cnvs = new aa_canvas();												//make antialias canvas obj

	unsigned int layer0 = 0;											//assign some unique layer ids, must be less than 'cn_aa_canvas_layer_max'
	unsigned int layer1 = 1;
	unsigned int layer2 = 2;
	unsigned int layer3 = 3;
	unsigned int ww;
	unsigned int hh;
	unsigned int bytes_per_pixel;
	unsigned int linedx;

	string fname = "zzznice_meter0.bmp";
	
	int filter_kernel_type = en_afkt_blackman_harris;

	cnvs->create_layer_from_bmp_file( fname, layer0, filter_kernel_type, kern_size );	//load an rgb bitmap file and create suitable layer to hold it


	cnvs->get_layer_details( layer0, ww, hh, linedx, bytes_per_pixel );
	
	printf("aa_wnd::aa_wnd() - Image dimensions w %d  h %d  depth %d  linedx %d\n", ww, hh, bytes_per_pixel, linedx );


//	jpg = new Fl_JPEG_Image( "s-meter2.jpg" );      // load jpeg image into ram
//	jpg = new Fl_JPEG_Image( "zzznice_meter0.jpg" );      // load jpeg image into ram
//	printf("aa_wnd::aa_wnd() - Image dimensions w %d  h %d  depth %d  ld %d\n", jpg->w(), jpg->h(), jpg->d(), jpg->ld() );

//	int ww = jpg->w();
//	int hh = jpg->h();
//	int bytes_per_pixel = jpg->d();

	bf0 = new unsigned char[ ww * hh * bytes_per_pixel ];
	bf1 = new unsigned char[ ww * hh * bytes_per_pixel ];






//	cnvs->create_layer( layer0, ww, hh, bytes_per_pixel, filter_kernel_type, kern_size );					//alloc mem for layer, some layers will have more memory alloc than is actually used, as they hold scaled down bitmaps
	cnvs->create_layer( layer1, ww, hh, bytes_per_pixel, filter_kernel_type, kern_size );					//alloc mem for layers, some layers will have more memory alloc than is actually used, as they hold scaled down bitmaps
	cnvs->create_layer( layer2, ww, hh, bytes_per_pixel, filter_kernel_type, kern_size );
	cnvs->create_layer( layer3, ww, hh, bytes_per_pixel, filter_kernel_type, kern_size );






	int srcx_in = 0;
	int srcy_in = 0;
//	int linedx = ww;
	int destx = 0;
	int desty = 0;

	unsigned int rendered_wid;
	unsigned int rendered_hei;

//	cnvs->block_set( layer0, (unsigned char*)*jpg->data(), 0, 0, ww, hh );	//full size to layer 0 (master copy)
//	cnvs->block_set( layer0, pp, 0, 0, ww, hh );	//full size to layer 0 (master copy)

//if( pp ) delete pp;

	//cnvs->block_fill( layer0, 0, 0, ww, hh, 255, 100, 80 );


	cnvs->block_duplicate( layer0, layer1 );								//full size working copy to layer 1, gnomon/needle will be drawn to this


	//cnvs->set_pixel25_layer( layer0, 170, 170, 255, 255, 255, 1 );

	cnvs->plot_pixel_multiple( layer0, 170, 170, 255, 255, 255, 7 );

	//cnvs->set_pixel25_layer( layer0, 165, 170, 255, 255, 255 );


	//cnvs->line_plot( layer0, ww/2-150, 10, ww/2, hh - 50, 237, 170, 33, linedx, 1 );

	//cnvs->line_plot( layer0, 20, 10, ww/2, hh - 50, 237, 170, 33, linedx, 1 );


	//cnvs->line_plot( layer1, ww/2 - 5, 2, ww/2, hh + 50, 237, 170, 33, linedx );

	//cnvs->line_plot( layer1, ww*(3.9f/4) - 5, 2, ww/2, hh + 50, 237, 170, 33, linedx );

	//cnvs->set_pixel_layer( layer0, 100, 100, 255, 255, 255 );

	float a1 = 10;

	float theta = 3/4.0f*pi;
	//int x0 = a0*cosf( theta );
	//int y0 = a0*sinf( theta );

	//theta = 3/4.0f*pi;
	//int x1 = a0*cosf( theta );
	//int y1 = a0*sinf( theta );

	for( int i = 0; i < 300; i++ )
		{
		theta = pi * 0.395 + i/300.0f * 0.85/4.0f*pi;
		
		int x0 = ww/2;
		int y0 = hh + 1220;

	//	theta = 0.1*pi;
		float a0 = 1700;
		int x1 = x0 + a0*cosf( theta );
		int y1 = y0 + -a0*sinf( theta );

	//	cnvs->set_pixel25_layer( layer0, x1, y1, 255, 255, 255 );



		theta = pi * 0.435 + i/300.0f * 0.5/4.0f*pi;
		
		x0 = ww/2;
		y0 = hh + 1660;

		a0 = 1700;
		x1 = x0 + a0*cosf( theta );
		y1 = y0 + -a0*sinf( theta );

	//	cnvs->set_pixel25_layer( layer0, x1, y1, 255, 255, 255 );
		}

	cnvs->block_get( layer0, bf0, 0, 0, ww, hh );						//move layer0 pixels into bf0 for display purposes				

	float scle1 = scale_factor;

	//cnvs->filter_scale_block( layer0, bf0, srcx_in, srcy_in, ww, hh, linedx, bf1, destx, desty, linedx, 0.25, 15,  bytes_per_pixel, rendered_wid, rendered_hei );
	cnvs->block_filter_scale_layer( layer0, srcx_in, srcy_in, layer2, destx, desty, scle1,  bytes_per_pixel, rendered_wid, rendered_hei );	//downsample to layer2

	 
	cnvs->block_get( layer2, bf1, 0, 0, rendered_wid, rendered_hei );		//move layer2 pixels into bf1 for display purposes

	cnvs->block_duplicate( layer2, layer3 );								//downsampled working copy to layer3, downsampled gnomon needle pixel rects will be placed on over layer3

	//cnvs->set_layer_details( layer3, rendered_wid, rendered_hei, linedx, bytes_per_pixel );


	//Fl_Bitmap *bmp = new Fl_Bitmap( (const char*)jpg->data(), ww, hh );
	bmp0 = new Fl_RGB_Image( (unsigned char*)bf0, ww, hh, bytes_per_pixel );
	bmp1 = new Fl_RGB_Image( (unsigned char*)bf1, rendered_wid, rendered_hei, bytes_per_pixel );

	//printf("bmp %x\n", bmp );


	Fl_Scroll *scl_bmp0 = new Fl_Scroll( 2, 2, w() - 4, h()/2 - 4, "" );
		bx_image0 = new Fl_Box( 2, 2, ww, hh );
//....	bx_image0->image( bmp0 );
	scl_bmp0->end();

	bx_image1 = new Fl_Box( 2, 2 + 2 + hei/2, wid, hei/2 );
	bx_image1->image( bmp1 );
	bx_image1->box( FL_BORDER_BOX );
	//delete[] bf0;
	//delete[] bf1;

//	string sname = "fancy_clock_hands_pnt00.txt";
	string sname = "gnomon3_point.txt";
	poly0.posx = 180;
	poly0.posy = 160;

	float scle = 0.5f;
	int bkeep_coords_positive = 1;
	int binvert_x = 0;
	int binvert_y = 1;
	float rotate_theta = 0;//pi/4;
	int cntr_x = 0;
	int cntr_y = 0;

	if( !cnvs->polygon_file_load( sname, poly0, scle, rotate_theta, bkeep_coords_positive, binvert_x, binvert_y, 255, 255, 255, 0, 0 ) )
		{
		printf("failed to load polygon file '%s'\n", sname.c_str() );
		}
	else{
		cnvs->polygon_copy( poly0, poly1 );
		
		
//		cnvs->polygon_rotate( poly1, pi/4, 1.0, cntr_x, cntr_y, kern_size/2, kern_size/2 );
		
	//	cnvs->plot_polygon_filled( layer0, poly1, poly1.posx, poly1.posy, 0.25f, 255, 255, 255 );
		}
	}
//----



dbg0 = 0;
dbg1 = 0;
dbg2 = 0;
dbg3 = 0;
dbg4 = 0;
dbg5 = 0;
}











float theta10 = 0;
float theta11 = 0;

int tcnt = 0;
int dbg_slow_inc = 0;


void aa_wnd::tick( float dt )
{
string s1;

//return;
strpf( s1, " dbg0:  %d  %d    %d  %d", dbg0, dbg1, dbg2, dbg3 );
bx_info->copy_label( s1.c_str() );


mtimer.time_start(mtimer.ns_tim_start);

tcnt++;

if( tcnt >= 0 ) 
	{
	tcnt = 0;
	dbg_slow_inc+=1;
	
	if( dbg_slow_inc > 299 ) dbg_slow_inc= 0;
	}






int layer0 = 0;
int layer1 = 1;
int layer2 = 2;
int layer3 = 3;


unsigned int ww;
unsigned int hh;
unsigned int linedx;
unsigned int bytes_per_pixel;
unsigned int dest_wid, dest_hei;



//----
//s-meter
if( which_meter == 0 )
	{
//	dbg_do_once = 0;
	cnvs->block_duplicate( layer0, layer1 );							//refresh full size working layer
	cnvs->block_duplicate( layer2, layer3 );							//refresh downsampled working layer

	bx_image0->image( 0 );
	delete bmp0;

	cnvs->vpixpnt.clear();
	//cnvs->set_pixel25_layer( layer1, 170, 170, 255, 255, 255 );

	cnvs->get_layer_details( layer1, ww, hh, linedx, bytes_per_pixel );


//	cnvs->plot_rect( layer1, 20, 20, 500, 300, 100, 20, 550, 300, 237, 170, 33 );


	float theta;

//dbg_slow_inc = 0;
	int ii = 299 - dbg_slow_inc;


	theta = pi * 0.390 + ii/300.0f * 0.87/4.0f*pi;




	float defl_full = 327;			//needs to be 1.0f
	float defl_min = -327;			//needs to be 0.0f

	float defl_span = ( defl_full - defl_min );

//		float volts = 0.0f + rnd()/100.0f;
	float volts = 0.0f + mousex/500.0f;

//printf("aa_wnd::tick() - volts %f\n", volts );

	if( volts < 0 ) volts = 0;

	if( volts > 1.1f ) volts = 1.1f;

	dbg0 = defl_min + ( defl_span ) * volts;

	theta = pi/2 - dbg0/1000.0f;

	float rn = dbg1/100.0f;//rnd();
//		theta += pi/40000 * rn;

	int x0 = ww/2;
	int y0 = hh + 1220;
	y0 = -1220;


//	theta = 0.1*pi;
	float a0 = 1700;		
	int needle_x1 = x0 + a0*cosf( theta );								//top of needle
	int needle_y1 = y0 + a0*sinf( theta );

//		cnvs->set_pixel25_layer( layer1, needle_x1, hh-needle_y1, 255, 255, 255, 1 );
	cnvs->plot_pixel_multiple( layer1, needle_x1, hh-needle_y1, 255, 255, 255, 7 );


//		theta = pi * 0.415 + ii/300.0f * 0.6/4.0f*pi;
	
	x0 = ww/2;
	y0 = hh + 1880;
	y0 = -680 + 80;
	a0 = 2000;
	a0 = 680;
	int needle_x0 = x0 + a0*cosf( theta );								//bottom of needle
	int needle_y0 = y0 + a0*sinf( theta );

	cnvs->plot_pixel_multiple( layer1, needle_x0, hh-needle_y0, 255, 255, 255, 7 );


	vector<st_aa_canvas_coord_tag> vpoly;

	st_aa_canvas_coord_tag op;

	op.x0 = 20;
	op.y0 = 20;
	vpoly.push_back( op );

	op.x0 = 20;
	op.y0 = 120;
	vpoly.push_back( op );

	op.x0 = 200;
	op.y0 = 200;
	vpoly.push_back( op );

	op.x0 = 300;
	op.y0 = 25;
	vpoly.push_back( op );

	//		cnvs->plot_polygon_filled( layer0, vpoly, 0.25f, 255, 255, 255 );


	//needle_x0 = ww/2;
	//needle_y0 = -1200;
	//cnvs->set_pixel25_layer( layer1, needle_x0, needle_y0, 255, 255, 255, 1 );


	//	cnvs->line_plot( layer1, ww*(3.9f/4) - 5, 10, ww/2, hh - 50, 237, 170, 33, linedx, 1 );

	//	cnvs->line_plot( layer1, needle_x0, needle_y0, needle_x1, needle_y1, 237, 170, 33, linedx, 1 );


	//cnvs->line_plot( layer1, ww/2, 0, ww/2, hh, 237, 170, 33, linedx, 1 );

		//printf("needle_x0 %d %d\n", needle_x0, needle_y0 );


	//cnvs->plot_line_thick( layer1, 50+dbg0, 50+dbg1, 500+dbg2, 300+dbg3, 237, 170, 33, 7 );


	vector<st_aa_canvas_bounding_rect_tag> vr;
	//cnvs->plot_line_thick_bound_rect( layer1, 50+dbg0, 50+dbg1, 500+dbg2, 300+dbg3, 237, 170, 33, 7, 28, vr );
	cnvs->plot_line_thick_bound_rect( layer1, needle_x0, hh-needle_y0, needle_x1, hh-needle_y1, 237, 170, 33, 7, 28, vr );


	//downsample/filter using bounding rects
	for( int i = 0; i < vr.size(); i++ )
		{
		st_aa_canvas_bounding_rect_tag o = vr[i];

		//printf( "rect111 x0 %d %d   ww %d %d     x %d %d  \n", o.x0,  (hh-o.y0), o.x1 - o.x0, o.y1 - o.y0,  (int)(o.x0*0.25f), (int)(hh*0.25f - o.y0*0.25f));
		cnvs->block_filter_partial_scale_layer( layer1, o.x0, o.y0, o.x1 - o.x0, o.y1 - o.y0, layer3, (o.x0+0)*scale_factor, (o.y0+0)*scale_factor, scale_factor, dest_wid, dest_hei );
		}


	//cnvs->block_filter_partial_scale_layer( layer1, 0, 0, ww, hh, layer3, 0*0.25, 0*0.25, 0.25f );


	//show bounding rects for debugging purposes
	for( int i = 0; i < vr.size(); i++ )
		{
		st_aa_canvas_bounding_rect_tag o = vr[i];
		cnvs->plot_rect_using_coords( layer1, o.x0, o.y0, o.x1 - o.x0, o.y1 - o.y0, 255, 255, 255 );
		}


	//cnvs->plot_pixel_multiple( layer1, 20+dbg0, 20+dbg1, 237, 170, 33, 9 );

	//cnvs->plot_region( layer1, 20+dbg1, 20+dbg3, 500+dbg0, 300+dbg2, 480, 20, 550, 310, 237, 170, 33 );



	//cnvs->plot_line_transparent_clamp( layer1, 20+dbg1, 20+dbg3, 600+dbg0, 400+dbg2, 255, 0, 255 );

	int kern_size = cnvs->st_aa[layer1].kern_size0;

	int rx = 20 + dbg0 - kern_size/4;
	int ry = 20 + dbg1 - kern_size/4;

	//cnvs->plot_rect_transparent_clamp( layer1, rx, ry, kern_size/2, kern_size/2, 255, 0, 255 );


	//cnvs->block_filter_partial_scale_layer( layer1, 0, 0, ww, hh, layer3, 0, 0, 0.25 );

	//cnvs->block_filter_partial_scale_layer( layer1, 100, 80, ww/2, hh/2, layer3, 100*0.25, 80*0.25, 0.25f );


	cnvs->polygon_copy( poly0, poly1 );
		
	float scle = 1.0;
	float rotate_theta = 0 + pi*dbg_slow_inc/50.0f;
	int cntr_x = 0;
	int cntr_y = 0;

	cnvs->polygon_rotate( poly1, rotate_theta, scle, poly1.cntr_x, poly1.cntr_y, kern_size/2, kern_size/2 );

	//	poly0.vcrd[5].x0 = 0;
	cnvs->plot_polygon_filled( layer1, poly1, 0, 0, 1.0f, 255, 255, 255 );



	int px = poly1.rectx + poly1.posx;
	int py = poly1.recty + poly1.posy;

	st_aa_canvas_rect_tag orct;

	orct.x0 = px;
	orct.y0 = py;
	orct.ww = poly1.ww;
	orct.hh = poly1.hh;

//	cnvs->rect_adj( orct, -kern_size/2, -kern_size/2, kern_size, kern_size );	//make rect bigger to cater for transversal of the kernel



	orct.x0 = orct.x0&0xfffffffc;					//round to nearest multiple of 4 (or zero), helps ensure filter kernel sits in over same pixels when compared to a full image downsample, stops partial filter block compositing shifts
	orct.y0 = orct.y0&0xfffffffc;

	//downsample/filter using bounding rect
	cnvs->block_filter_partial_scale_layer( layer1, orct.x0, orct.y0, orct.ww, orct.hh, layer3, nearbyint( (orct.x0)*scale_factor ), nearbyint( (orct.y0)*scale_factor ), scale_factor, dest_wid, dest_hei );

	cnvs->plot_rect_using_coords( layer1, orct.x0, orct.y0, orct.ww, orct.hh, 255, 255, 0 );

	cnvs->get_layer_details( layer1, ww, hh, linedx, bytes_per_pixel );
	cnvs->block_get( layer1, bf0, 0, 0, ww, hh );

	bmp0 = new Fl_RGB_Image( (unsigned char*)bf0, ww, hh, bytes_per_pixel, 0 );

	bx_image0->image( bmp0 );



	//cnvs->set_layer_details( layer3, ww, hh, linedx, bytes_per_pixel );

	//printf("ww %d %d  dx %d bpp %d\n", ww, hh, linedx, bytes_per_pixel );


	//cnvs->block_fill( layer0, 0, 0, ww, hh, 255, 100, 80 );




	//cnvs->set_pixel25_layer( layer1, 170, 170, 255, 255, 255 );

//	int idx = 0;

//	for( int i = 0; i < cnvs->vpixpnt.size(); i++ )
//		{
	//		printf("vpixpnt[%d].x0 %d %d\n", i, cnvs->vpixpnt[i].x0, cnvs->vpixpnt[i].y0);
	//		cnvs->pixel_filter( layer1, layer3, i, 0.25, 15 );
//		}
	//getchar();
	}
//----









//----
//lilliput-meter
if( which_meter == 1 )
	{
	bool bshow_bound_rect = 0;
	
	int kern_size = cnvs->st_aa[layer1].kern_size0;

//	dbg_do_once = 0;

	cnvs->block_duplicate( layer0, layer1 );							//refresh full size working layer
	cnvs->block_duplicate( layer2, layer3 );							//refresh downsampled working layer


	bx_image0->image( 0 );
	delete bmp0;

	cnvs->vpixpnt.clear();
	//cnvs->set_pixel25_layer( layer1, 170, 170, 255, 255, 255 );

	cnvs->get_layer_details( layer1, ww, hh, linedx, bytes_per_pixel );

//	cnvs->plot_rect( layer1, 20, 20, 500, 300, 100, 20, 550, 300, 237, 170, 33 );


//---- calc angle of needle shaft
	float theta;

//dbg_slow_inc = 0;
	int ii = 299 - dbg_slow_inc;


	theta = pi * 0.390 + ii/300.0f * 0.87/4.0f*pi;


	float defl_full = 227;			//needs to be 1.0f
	float defl_min = -227;			//needs to be 0.0f

	float defl_span = ( defl_full - defl_min );

//		float volts = 0.0f + rnd()/100.0f;
	float volts = 0.0f + mousex/400.0f;

//printf("aa_wnd::tick() - volts %f\n", volts );

	if( volts < 0 ) volts = 0;

	if( volts > 1.1f ) volts = 1.1f;

	int defl_0 = defl_min + ( defl_span ) * volts;

	theta = pi/2 - defl_0/1000.0f;

	float rn = dbg1/100.0f;//rnd();
//		theta += pi/40000 * rn;

	int x0 = ww/2;
	int y0 = hh + 1220;
	y0 = -1200;


int pivotx = 935;
int pivoty = 1292;
//---- 


//---- calc end points of needle shaft and position of gnomon circle
//	theta = 0.1*pi;
	float a0 = 1700;
	a0 = pivoty - 210+dbg5;												//gnomon's tip position, hugging its circle 		
	int needle_x1 = x0 + a0*cosf( theta );
	int needle_y1 = y0 + a0*sinf( theta );

float needle_theta = (3*pi/4)*(w() - (mousex-200)/1.0f) / (w()/4) / 3*pi/4;

needle_theta += -aa_sin0*pi/100;

if( needle_theta < pi/4 ) needle_theta = pi/4;
if( needle_theta > (3*pi/4) ) needle_theta = (3*pi/4);

needle_x1 = nearbyint( pivotx + a0*cosf( needle_theta ) );							//center of gnomon circle
needle_y1 = nearbyint( pivoty - a0*sinf( needle_theta ) );

a0 -= 19;
int needle_shaft_x1 = nearbyint( pivotx + a0*cosf( needle_theta ) );				//top of needle shaft, below circle
int needle_shaft_y1 = nearbyint( pivoty - a0*sinf( needle_theta ) );

//		cnvs->set_pixel25_layer( layer1, needle_x1, hh-needle_y1, 255, 255, 255, 1 );
//	cnvs->plot_pixel_multiple( layer1, needle_x1, needle_y1, 100, 100, 255, 7 );


//		theta = pi * 0.415 + ii/300.0f * 0.6/4.0f*pi;
	

	a0 = 280;
	int needle_x0 = nearbyint( pivotx + a0*cosf( needle_theta ) );						//bottom of needle shaft
	int needle_y0 = nearbyint( pivoty - a0*sinf( needle_theta ) );

//needle_x0 = 935;
//needle_y0 = 1291 - 30;

//	cnvs->plot_pixel_multiple( layer1, needle_x0, hh-needle_y0, 100, 100, 255, 7 );


//	vector<st_aa_canvas_coord_tag> vpoly;

//	st_aa_canvas_coord_tag op;

//	op.x0 = 20;
//	op.y0 = 20;
//	vpoly.push_back( op );

//	op.x0 = 20;
//	op.y0 = 120;
//	vpoly.push_back( op );

//	op.x0 = 200;
//	op.y0 = 200;
//	vpoly.push_back( op );

//	op.x0 = 300;
//	op.y0 = 25;
//	vpoly.push_back( op );

	//		cnvs->plot_polygon_filled( layer0, vpoly, 0.25f, 255, 255, 255 );


	//needle_x0 = ww/2;
	//needle_y0 = -1200;
	//cnvs->set_pixel25_layer( layer1, needle_x0, needle_y0, 255, 255, 255, 1 );


	//	cnvs->line_plot( layer1, ww*(3.9f/4) - 5, 10, ww/2, hh - 50, 237, 170, 33, linedx, 1 );

	//	cnvs->line_plot( layer1, needle_x0, needle_y0, needle_x1, needle_y1, 237, 170, 33, linedx, 1 );


	//cnvs->line_plot( layer1, ww/2, 0, ww/2, hh, 237, 170, 33, linedx, 1 );

		//printf("needle_x0 %d %d\n", needle_x0, needle_y0 );


//	cnvs->plot_line_thick( layer1, needle_x0, needle_y0, needle_shaft_x1, needle_shaft_y1, 0, 0, 0, 7 );	//needle shaft
//---- 



	vector<st_aa_canvas_bounding_rect_tag> vr;


	//cnvs->plot_pixel_multiple_with_rect( layer1, 200, 200, 7, 255, 0, 0, 7 + kern_size, vr );

	//{
	//st_aa_canvas_bounding_rect_tag o = vr[0];

	//cnvs->block_filter_partial_scale_layer( layer1, o.x0, o.y0, o.x1 - o.x0, o.y1 - o.y0, layer3, (o.x0)*scale_factor, (o.y0)*scale_factor, scale_factor );




//	if( bshow_bound_rect )
//		{
		//show bounding rects for debugging purposes
//		for( int i = 0; i < vr.size(); i++ )
//			{
//			st_aa_canvas_bounding_rect_tag o = vr[i];
//			cnvs->plot_rect( layer1, o.x0, o.y0, o.x1 - o.x0, o.y1 - o.y0, 100, 100, 255 );
//			}
//		}
//	}



//---- plot needle shaft
	//cnvs->plot_line_thick_bound_rect( layer1, 50+dbg0, 50+dbg1, 500+dbg2, 300+dbg3, 237, 170, 33, 7, 28, vr );
	cnvs->plot_line_thick_bound_rect( layer1, needle_x0, needle_y0, needle_shaft_x1, needle_shaft_y1, 0, 0, 0, 7, 31, vr );	//needle shaft


	//downsample/filter using bounding rects
	for( int i = 0; i < vr.size(); i++ )
		{
		st_aa_canvas_bounding_rect_tag o = vr[i];

		//printf( "rect111 x0 %d %d   ww %d %d     x %d %d  \n", o.x0,  (hh-o.y0), o.x1 - o.x0, o.y1 - o.y0,  (int)(o.x0*0.25f), (int)(hh*0.25f - o.y0*0.25f));
		cnvs->block_filter_partial_scale_layer( layer1, o.x0, o.y0, o.x1 - o.x0, o.y1 - o.y0, layer3, (o.x0+0)*scale_factor, (o.y0+0)*scale_factor, scale_factor, dest_wid, dest_hei );
		}


	//cnvs->block_filter_partial_scale_layer( layer1, 0, 0, ww, hh, layer3, 0*0.25, 0*0.25, 0.25f );


	if( bshow_bound_rect )
//	if( 1 )
		{
		//show bounding rects for debugging purposes
		for( int i = 0; i < vr.size(); i++ )
			{
			st_aa_canvas_bounding_rect_tag o = vr[i];
			cnvs->plot_rect_using_coords( layer1, o.x0, o.y0, o.x1 - o.x0, o.y1 - o.y0, 100, 100, 255 );
			}
		}
//---- 

	//cnvs->plot_pixel_multiple( layer1, 20+dbg0, 20+dbg1, 237, 170, 33, 9 );

	//cnvs->plot_region( layer1, 20+dbg1, 20+dbg3, 500+dbg0, 300+dbg2, 480, 20, 550, 310, 237, 170, 33 );



	//cnvs->plot_line_transparent_clamp( layer1, 20+dbg1, 20+dbg3, 600+dbg0, 400+dbg2, 255, 0, 255 );


	int rx = 20 + dbg0 - kern_size/4;
	int ry = 20 + dbg1 - kern_size/4;

	//cnvs->plot_rect_transparent_clamp( layer1, rx, ry, kern_size/2, kern_size/2, 255, 0, 255 );


//----- gnomon 'arrow' indicator at very top of needle
//	poly0.posx = 200;
//	poly0.posy = 300;
	
	poly0.posx = needle_x1;//200-13 + dbg0;
	poly0.posy = needle_y1-0;//200-84 + dbg1;


	cnvs->polygon_copy( poly0, poly1 );
		
	float scle = 1.0;
	float gnomon_theta = 0 + pi*dbg_slow_inc/50.0f;

gnomon_theta = needle_theta - pi/2;//0.1*twopi;
	cnvs->polygon_rotate( poly1, gnomon_theta, scle, poly1.cntr_x, poly1.cntr_y + poly1.hh/2 + 17, kern_size/2, kern_size/2  );



	int px = poly1.rectx + poly1.posx;
	int py = poly1.recty + poly1.posy;

	st_aa_canvas_rect_tag orct;

	orct.x0 = px;
	orct.y0 = py;
	orct.ww = poly1.ww;
	orct.hh = poly1.hh;

//	cnvs->rect_adj( orct, -7, -7, 7+7, 7+7 );
//	cnvs->rect_adj( orct, -kern_size/2, -kern_size/2, kern_size, kern_size );	//make rect bigger to cater for transversal of the kernel
//	cnvs->rect_adj( orct, 0, 0, kern_size, kern_size );	//make rect bigger to cater for transversal of the kernel


	float fx2 = orct.x0*scale_factor;				//scale down dest pos keeping fractional part as well
	float fy2 = orct.y0*scale_factor;



//double dint;

//modf( fry, &dint );





	//	poly0.vcrd[5].x0 = 0;
	cnvs->plot_polygon_filled( layer1, poly1, 0, 0, 1.0f, 0, 0, 0 );



//float fractx = dbg4/4.0f;
//float fracty = dbg5/4.0f;

//	printf(" aa_wnd::tick() - poly0.posx %d %d  %f %f  fractx %f %f\n", poly0.posx, poly0.posy, fx2, fy2, fractx, fracty );

	//downsample/filter using bounding rect
	cnvs->block_filter_partial_scale_layer( layer1, orct.x0, orct.y0, orct.ww, orct.hh, layer3, fx2, fy2, scale_factor, dest_wid, dest_hei );

	if( bshow_bound_rect )
		{
		cnvs->plot_rect_using_coords( layer1, poly1.posx + poly1.rectx, poly1.posy + poly1.recty, poly1.ww, poly1.hh, 255, 255, 0 );
		}
//----

//---- gnomon circle
	px = needle_x1;//200 + dbg0;
	py = needle_y1;//200 + dbg1;
	float radius = 18;
	float start_ang = 0;//pi/4;
	float stop_ang = twopi;//-twopi*(3.0f/4.0f);
	int segments = 30;
	int num_pixels = 5;
	int rect_size = kern_size/2 + num_pixels;
	bool one_rect_only = 1;

	cnvs->plot_arc_thick_rect( layer1, px, py, radius, start_ang, stop_ang, segments, num_pixels, rect_size, one_rect_only, 0, 0, 0, vr );

	{
	st_aa_canvas_bounding_rect_tag o = vr[0];
	cnvs->block_filter_partial_scale_layer( layer1, o.x0, o.y0, o.x1 - o.x0, o.y1 - o.y0, layer3, o.x0*scale_factor, o.y0*scale_factor, scale_factor, dest_wid, dest_hei );
	}

//	cnvs->plot_pixel_multiple( layer1, px, py, 237, 170, 33, 7 );


	if( bshow_bound_rect )
		{
		//show bounding rects for debugging purposes
		for( int i = 0; i < vr.size(); i++ )
			{
			st_aa_canvas_bounding_rect_tag o = vr[i];
			cnvs->plot_rect_using_coords( layer1, o.x0, o.y0, o.x1 - o.x0, o.y1 - o.y0, 100, 100, 255 );
			}
		}
//----





//---- squarewave oscillator

if( 1 )
	{
	//cnvs->ploygon_arc_build( 200,  200, 180, 0, twopi/2, 27, 7, 7, poly2 );

	poly2.posx = 50;
	poly2.posy = 1000;

	vector<st_aa_canvas_coord_tag> vpoly;

	st_aa_canvas_coord_tag op;
	st_aa_canvas_bounding_rect_tag orct0;

	aa_freq0 = 0.102f;
	float aa_theta_inc0 = aa_freq0 * twopi * dt;



	for( int i = 0; i < 100; i++ )
		{
		op.x0 = i*2;
		aa_sin0 = sinf( aa_theta0 + aa_phse0) + 1/3.0f*sinf( 3*aa_theta0 + aa_phse0) + 1/5.0f*sinf( 5*aa_theta0 + aa_phse0) + 1/7.0f*sinf( 7*aa_theta0 + aa_phse0);
		op.y0 = 40*-aa_sin0;
		vpoly.push_back( op );

		aa_theta0 += aa_theta_inc0;
		if( aa_theta0 >= twopi ) aa_theta0 -= twopi;
		}

	poly2.vcrd = vpoly;
	//aa_phse0 += twopi*0.01f;


	cnvs->polygon_update_rect( poly2, 0, 0 );

	int num_pixels = 3;
	cnvs->plot_polygon_thick_with_rect( layer1, poly2, 0, 0, 1.0f, num_pixels, 100, 100, 255, 25, 0, vr );

	cnvs->polygon_get_bounding_rect_absolute( poly2, kern_size/2 + num_pixels/2, kern_size/2 + num_pixels/2, orct0 );

	//cnvs->plot_polygon_filled( layer1, poly2, 0, 0, 1.0f, 0, 0, 0 );
	float fx2 = (poly2.posx + poly2.rectx)*scale_factor;				//scale down dest pos keeping fractional part as well
	float fy2 = (poly2.posy + poly2.recty)*scale_factor;



	//downsample/filter using bounding rects
//	for( int i = 0; i < vr.size(); i++ )
		{
//		st_aa_canvas_bounding_rect_tag o = vr[i];

		cnvs->block_filter_partial_scale_layer( layer1, orct0.x0, orct0.y0, orct0.x1 - orct0.x0, orct0.y1 - orct0.y0, layer3, (orct0.x0+0)*scale_factor, (orct0.y0+0)*scale_factor, scale_factor, dest_wid, dest_hei );
		}

	if( 0 )
//	if( bshow_bound_rect )
		{
		//show bounding rects for debugging purposes
		for( int i = 0; i < vr.size(); i++ )
			{
			st_aa_canvas_bounding_rect_tag o = vr[i];
			if(i&1) cnvs->plot_rect_using_coords( layer1, o.x0, o.y0, o.x1 - o.x0, o.y1 - o.y0, 100, 100, 255 );
			else cnvs->plot_rect_using_coords( layer1, o.x0, o.y0, o.x1 - o.x0, o.y1 - o.y0, 200, 80, 100 );
			}
		}
}
//----



	cnvs->get_layer_details( layer1, ww, hh, linedx, bytes_per_pixel );
	cnvs->block_get( layer1, bf0, 0, 0, ww, hh );

	bmp0 = new Fl_RGB_Image( (unsigned char*)bf0, ww, hh, bytes_per_pixel, 0 );

	bx_image0->image( bmp0 );



	//cnvs->set_layer_details( layer3, ww, hh, linedx, bytes_per_pixel );

	//printf("ww %d %d  dx %d bpp %d\n", ww, hh, linedx, bytes_per_pixel );


	//cnvs->block_fill( layer0, 0, 0, ww, hh, 255, 100, 80 );




	//cnvs->set_pixel25_layer( layer1, 170, 170, 255, 255, 255 );

//	int idx = 0;

//	for( int i = 0; i < cnvs->vpixpnt.size(); i++ )
//		{
	//		printf("vpixpnt[%d].x0 %d %d\n", i, cnvs->vpixpnt[i].x0, cnvs->vpixpnt[i].y0);
	//		cnvs->pixel_filter( layer1, layer3, i, 0.25, 15 );
//		}
	//getchar();
	}
//----












cnvs->get_layer_details( layer3, ww, hh, linedx, bytes_per_pixel );
//printf("layer3 ww hh %d %d\n", ww, hh );
cnvs->block_get( layer3, bf1, 0, 0, ww, hh );

//bx_image0->hide();

bx_image1->image( 0 );

delete bmp1;
bmp1 = new Fl_RGB_Image( (unsigned char*)bf1, ww, hh-2, bytes_per_pixel );		//crop bottom slightly to hide what filter kernel missed


bx_image1->image( bmp1 );




float theta10_inc;
float theta11_inc;

float freq0 = 1;
float freq1 = 2;

theta10_inc = freq0 * twopi / 10;
theta11_inc = freq1 * twopi / 10;


float famp = 1.51;
float f0 = famp * sinf( theta10 );
float f1 = 0.75 * sinf( theta11 );// - 2.25;

	
theta10 += theta10_inc;
if( theta10 >= twopi ) theta10 -= twopi;

theta11 += theta11_inc;
if( theta11 >= twopi ) theta11 -= twopi;

float dtime = mtimer.time_passed(mtimer.ns_tim_start);
printf("aa_wnd::tick() - time passed: %f\n" , dtime );

redraw();
}






//bx_image = new Fl_Box( 2, 2, ww, hh );
//bx_image->image( bmp );



void aa_wnd::draw( )
{
Fl_Double_Window::draw();

}







int aa_wnd::handle( int e )
{
bool need_redraw = 0;
bool dont_pass_on = 0;

if ( e & FL_MOVE )
	{
	mousex = Fl::event_x();
	mousey = Fl::event_y();


	if( left_button ) 
		{
		if( ctrl_key )
			{
			dbg2 = mousex;
			dbg3 = mousey;
			}
		else{
			dbg0 = mousex;
			dbg1 = mousey;
			}
		}
		
	need_redraw = 1;
    dont_pass_on = 0;
	}



if( e == FL_PUSH )
	{

	if(Fl::event_button() == 1 )
		{
		left_button = 1;
		}
	need_redraw = 1;
	dont_pass_on = 0;
	}


if( e == FL_RELEASE )
	{

	if(Fl::event_button() == 1 )
		{
		left_button = 0;
		}
	need_redraw = 1;
	dont_pass_on = 0;
	}
	


if ( e == FL_MOUSEWHEEL )
	{
	int mw = Fl::event_dy();
	mousewheel += mw;
	
	int wheel_step = 1*mw;
	
	if( !shift_key )
		{
		if( ctrl_key ) dbg1 += wheel_step;
		else  dbg0 += wheel_step;
		}
	else{
		if( ctrl_key ) dbg3 += wheel_step;
		else  dbg2 += wheel_step;
		}
	need_redraw = 1;
    dont_pass_on = 0;
	}


if ( e == FL_KEYDOWN )
	{
	int key = Fl::event_key();
	
	if( ( key == FL_Control_L ) || ( key == FL_Control_R ) ) ctrl_key = 1;
	if( ( key == FL_Shift_L ) || (  key == FL_Shift_R ) ) shift_key = 1;

	if( key == FL_Left ) dbg0 -= 1;
	if( key == FL_Right ) dbg0 += 1;
	
	if( key == FL_Up ) dbg1 -= 1;
	if( key == FL_Down ) dbg1 += 1;
	
	
	if( key == 'a' ) dbg4 -= 1;
	if( key == 's' ) dbg4 += 1;

	if( key == 'w' ) dbg5 -= 1;
	if( key == 'z' ) dbg5 += 1;
	
	need_redraw = 1;
    dont_pass_on = 0;
	}



if ( e == FL_KEYUP )  				                    //key release?
	{
	int key = Fl::event_key();
	
	if( ( key == FL_Control_L ) || ( key == FL_Control_R ) ) ctrl_key = 0;
	if( ( key == FL_Shift_L ) || (  key == FL_Shift_R ) ) shift_key = 0;

	need_redraw = 1;
    dont_pass_on = 0;
	}


//if ( need_redraw ) redraw();

if( dont_pass_on ) return 1;

return Fl_Double_Window::handle(e);
}
//----------------------------------------------------------------------



















rtl_graph_wnd::~rtl_graph_wnd()
{
//rtl.cancel_async();

DoQuit();

}




bool plot_first_time_flag = 1;			//initial plot call flag


void rtl_graph_wnd::plot_gph01()
{
//return;

if( !(gph1.shown() && gph1.visible()) ) return;							//wnd minimized?

plot_grph1_gui_thrd();



//		return;

//vgph1_x.clear();
//vgph1_y0.clear();
//vgph1_y1.clear();

//printf( "rtl_graph_wnd::plot_gph01() - b_plot_gph2 %d vgph1_x.size() %d\n", b_plot_gph2, vgph1_x.size() );

if( plot_first_time_flag ) 
	{
	string s1;
	s1 = " Probe Graph (gph1)";
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
	
//	gph1.set_col_trc1( 0, "drkgreen" );

/*
	gph1.set_col_bkgd( 0, "ofw" );
	
	gph1.set_col_axis1( gph_idx, "drkb" );
	
	gph1.set_trc_label1( 0, "trc0" );
	gph1.set_trc_label2( 0, "trc1" );
	gph1.set_trc_label3( 0, "trc2" );
	gph1.set_trc_label4( 0, "trc3" );

	gph1.set_col_trc1_rgb( 0, 200, 80, 80 );
	gph1.set_col_trc1_rgb( 0, 200, 80, 80 );

	gph1.set_col_trc2( 0, "orange" );

	gph1.set_col_trc3( 0, "drkg" );

	gph1.set_col_trc4_rgb( 0, 159, 15, 165 );

	gph1.set_col_axis1( 0, "drkb" );
	gph1.set_obj_col_text1( 0, "blk" );	
*/


	plot_first_time_flag = 0;
	}



//#define twopi 2*(float)M_PI
//int cnt = 1000;
//for( int i = 0; i < cnt; i++ )
//	{
//	float f0 = sinf( (float)i/cnt * twopi * 5 );
//	float f1 = 0.5*sinf( (float)i/cnt * twopi * 10 );
//	vgph1_x.push_back( i );
//	vgph1_y0.push_back( f0 );
//	vgph1_y1.push_back( f1 );
//	}
//fgph.plotxy( gph0_vx, gph0_vamp0, gph0_vamp1, gph0_vamp2, gph0_vamp4,"brwn", "drkg", "drkcy", "drkr", "ofw", "drkb", "drkgry", "trace 1", "trace 2", "trace 3", "trace 4"  );
//fgph.plotxy( gph0_vx, gph0_vamp0, gph0_vamp1 );
//fgph.plotxy( fgph_vx, fgph_vy0, fgph_vy1 );

//fgph0.plotxy( fgph0_vx, fgph0_vy0, "drky", "ofw", "drkb", "blk", "pc srate" );	//3 traces cols, bkgd col, axis col, text col, and trace colour coded labels, see defined colours: 'user_col'
//gph1.plotxy( vgph1_x, vgph1_y0, vgph1_y1, "drkr", "drkg", "ofw", "drkb", "blk", "y0", "y1" );	//3 traces cols, bkgd col, axis col, text col, and trace colour coded labels, see defined colours: 'user_col'


vector<float> vx;
vector<float> vy0;
vector<float> vy1;
vector<float> vy2;
vector<float> vy3;

bool bplot_it = 0;

if( b_plot_gph1 )
	{
//printf( "HHHHHHHHHHHHHHHHHHHHHHHHHEeeeeeeeeeeeeeeeeeeeeeeeeeee  %d\n", vgph1_x.size() );

//	for( int i = 0; i < vgph1_x.size(); i++ )
//		{
//		vx.push_back( vgph1_x[i] );
//		vx.push_back( i );
//		}

	for( int i = 0; i < vgph1_x.size(); i++ )
		{
		vx.push_back( vgph1_x[i] );
		}

	for( int i = 0; i < vgph1_y0.size(); i++ )
		{
		vy0.push_back( vgph1_y0[i] );
		}

	for( int i = 0; i < vgph1_y1.size(); i++ )
		{
		vy1.push_back( vgph1_y1[i] );
		}

	b_plot_gph1 = 0;
	bplot_it = 1;
	}


//---
bool sine_wave = 0;

if( sine_wave )
//if( gph_loc_sel_tmp == en_gls_filtresp_aa )
	{
	float theta00 = 0;
	float theta01 = 0;
	float theta02 = 0;
	float theta03 = 0;

	float theta00_inc;
	float theta01_inc;
	float theta02_inc;
	float theta03_inc;
	
	float freq0 = 1;
	float freq1 = 2;
	float freq2 = 3;
	float freq3 = 4;

	int cnt = 512;
	theta00_inc = freq0 * twopi / (cnt / 2);
	theta01_inc = freq1 * twopi / (cnt / 2 );
	theta02_inc = freq2 * twopi / (cnt / 2 );
	theta03_inc = freq3 * twopi / (cnt / 2 );

	vx.clear();
	vy0.clear();
	vy1.clear();
	vy2.clear();
	vy3.clear();
	
	for( int i = 0; i < cnt; i++ )
		{
		float famp = 1.51;
		float f0 = famp * sinf( theta00 );
		float f1 = 0.75 * sinf( theta01 );// - 2.25;
		float f2 = 0.810 * sinf( theta02 ) - 0.5;
		float f3 = famp * sinf( theta03 );

		vx.push_back( i );
		vy0.push_back( f0 );
		vy1.push_back( f1 );
		vy2.push_back( f2 );
		vy3.push_back( f3 );
//		vpcm_cache.push_back( f1 );
		
		theta00 += theta00_inc;
		if( theta00 >= twopi ) theta00 -= twopi;

		theta01 += theta01_inc;
		if( theta01 >= twopi ) theta01 -= twopi;
		
		theta02 += theta02_inc;
		if( theta02 >= twopi ) theta02 -= twopi;
		
		theta03 += theta03_inc;
		if( theta03 >= twopi ) theta03 -= twopi;
		}
	}
//---




/*
en_fast_mgraph_format_tag
{
en_fmgf_trace1 = 0x1;
en_fmgf_trace2 = 0x2;
en_fmgf_trace3 = 0x3;
en_fmgf_int_type = 0x4			//this must b
en_fmgf_float_type = 0x8
en_fmgf_double_type = 0x16

};

st_fast_mgraph_details_tag
{
vector<float> vx;
vector<float> vy0;
vector<float> vy1;
vector<float> vy3;

};
*/


gph1.b_x_axis_values_derived[0] = gph1_b_x_axis_values_derived;
gph1.x_axis_values_derived_left_value[0] = gph1_x_axis_values_derived_left_value;
gph1.x_axis_values_derived_inc_value[0] = gph1_x_axis_values_derived_inc_value;

if( bplot_it ) 
	{
	if( plot_gph_trace_cnt == 1 )
		{
		if( gph1_b_x_axis_values_derived ) gph1.plot_vfloat_1( vy0 );
		else gph1.plotxy_vfloat_1( vx, vy0 );
		}

	if( plot_gph_trace_cnt == 2 )										//2 traces
		{
		if( gph1_b_x_axis_values_derived ) gph1.plot_vfloat_2( vy0, vy1 );
		else gph1.plotxy_vfloat_2( vx, vy0, vy1 );
		}
	}

}






//set 'hzoom' to -1 to not alter current hzoom setting
void rtl_graph_wnd::centre_graph( int delay_cnt, int hzoom )
{

if( delay_cnt == 0 ) delay_cnt = 1;


miwp_tune->miw->set_value_from_double( g_freq_tune + g_freq_sub_tune );
miw_freq_sub_tune->set_value_from_double( 0 );

freq_listen( 1, 0, 0, 0, "centre_graph()" );

mgraph_freq_tune = g_freq_tune + g_freq_sub_tune;
mgraph_freq_sub_tune = 0;


ineed_hzoom_and_center_to_cur_freq = delay_cnt;
//	nhac_to_this_freq = g_freq_tune + g_freq_sub_tune;
//	need_hzoom_and_center_to_this_freq_in_bw = gph_mouse_freq_in_bw;
nhac_hzoom_factor = hzoom;											//don't change hzoom factor
}






//ASLO ADD TO 'cb_graph_keydown()' to intercept keydowns in graph

bool rtl_graph_wnd::do_common_key_processing( bool ctrl, bool shift, int key, int key2 )
{
printf( "rtl_graph_wnd::do_common_key_processing() - ctrl: %d, shift: %d, key: 0x%02x\n", ctrl, shift, key );


if( (key == '0') )
	{
	}



if( (key == ' ') && (!ctrl) && (!shift) )
	{
		
	int px, py;
	gph0->get_mouse_pixel_position_on_background( px, py );
	
	pin_add( px, py,  g_freq_tune, g_freq_sub_tune  );
	}


if( (key == FL_Delete) && (!ctrl) && (!shift) )
	{
	if( gph0_obj_sel_idx >= 0 )
		{
		
		st_gph_obj_tag og = vgph_obj[ gph0_obj_sel_idx ];

		if( og.type & en_got_freq_pin )
			{			
			int ii = og.pin_idx;
			if( pin_delete( ii ) )
				{
				gph0_obj_hover_idx = -1;
				gph0_obj_sel_idx = -1;
				}
			}
		}
	}



if( (key == FL_KP + '/') && (!ctrl) && (!shift) )
	{
	disp_spect_zoom_factor -= 20;
	if( disp_spect_zoom_factor < 1.0f ) disp_spect_zoom_factor = 1;
	miw_gph_disp_spect_zoom_factor->set_value_from_double( disp_spect_zoom_factor );
	return 1;
	}

if( (key == FL_KP + '*') && (!ctrl) && (!shift) )
	{
	disp_spect_zoom_factor += 20;
	if( disp_spect_zoom_factor > 2048.0f ) disp_spect_zoom_factor = 2048.0f;
	miw_gph_disp_spect_zoom_factor->set_value_from_double( disp_spect_zoom_factor );
	return 1;
	}

if( (key == FL_KP + '-') && (!ctrl) && (!shift) )
	{
	disp_spect_zoom_factor -= 5;
	if( disp_spect_zoom_factor < 1.0f ) disp_spect_zoom_factor = 1;
	miw_gph_disp_spect_zoom_factor->set_value_from_double( disp_spect_zoom_factor );
	return 1;
	}

if( (key == FL_KP + '+') && (!ctrl) && (!shift) )
	{
	disp_spect_zoom_factor += 5;
	if( disp_spect_zoom_factor > 2048.0f ) disp_spect_zoom_factor = 2048.0f;
	miw_gph_disp_spect_zoom_factor->set_value_from_double( disp_spect_zoom_factor );
	return 1;
	}


if( (key == FL_Home ) && (!ctrl) && (!shift) )
	{
	centre_graph( 1, -1 );
/*	
	miwp_tune->miw->set_value_from_double( g_freq_tune + g_freq_sub_tune );
	miw_freq_sub_tune->set_value_from_double( 0 );

	freq_listen( 1, 0, 0, 0, "do_common_key_processing()" );

	mgraph_freq_tune = g_freq_tune + g_freq_sub_tune;
	mgraph_freq_sub_tune = 0;
	
	
	bneed_hzoom_and_center_to_cur_freq = 1;
//	nhac_to_this_freq = g_freq_tune + g_freq_sub_tune;
//	need_hzoom_and_center_to_this_freq_in_bw = gph_mouse_freq_in_bw;
	nhac_hzoom_factor = -1.0f;											//don't change hzoom factor
*/

	return 1;
	}


if( (key == '1') )
	{
	return 1;
	}

//'b' and left cursor?
if( (!ctrl) && (!shift) && (key == FL_Left) )
	{
	return 1;
	}


if( (!ctrl) && (!shift) && (key == 's') )								//solo toggle
	{
	return 1;
	}


return 0;
}



















int rtl_graph_wnd::handle( int e )
{
string s1;
bool need_redraw = 0;
bool dont_pass_on = 0;


if ( e == FL_ENTER )
	{

	need_redraw = 1;
	dont_pass_on = 0;
	}

if ( e == FL_LEAVE )
	{
//	take_focus();
//if( pref_main_wnd_auto_take_focus ) Fl::focus( this );
//	Fl::focus( this->parent() );
	need_redraw = 1;
	dont_pass_on = 0;
	}



if ( (e == FL_MOVE) || (e == FL_DRAG) )
	{
	mousex = Fl::event_x();
	mousey = Fl::event_y();

//	printf( "rtl_graph_wnd::handle() - FL_MOVE %d %d\n", mousex, mousey );


	if( mousey <= (menu_hei-15) ) 
		{
		if( mousex <= 250 ) 
			{
			if( b_gph0_shrink == 0 )
				{
				b_gph0_shrink = 1;
				
	//			gph0->size( gph0->w(), gph0_hei - menu_hei );
				gph0->position( gph0->x(), gph0_posy + menu_hei );			//repos gph0

				wfall0->position( wfall0->x(), wfall0_posy + menu_hei );	//alter wfall0 dim

				menu_sdr->show();
				}
			}
		}

	if( mousey > (menu_hei) )
		{
		if( b_gph0_shrink == 1 )
			{
			b_gph0_shrink = 0;
			menu_sdr->hide();
			
//			gph0->size( gph0->w(), gph0_hei );							//restore gph0 size
			gph0->position( gph0->x(), gph0_posy );						//restore gph0 dim

			wfall0->position( wfall0->x(), wfall0_posy );
			}
		}

	need_redraw = 1;
    dont_pass_on = 0;
	}


if ( ( e == FL_PUSH ) )
	{
//	printf( "rtl_graph_wnd::handle() - FL_PUSH\n" );

	need_redraw = 1;
    dont_pass_on = 0;
	}





if ( e == FL_KEYDOWN )						//key press?
	{
	int key = Fl::event_key();

	printf( "rtl_graph_wnd::handle() - FL_KEYDOWN 0x%02x\n", key );

	int key2 = 0;

	int ret = do_common_key_processing( ctrl_key, shift_key, key, key2 );
	
//	if( key == FL_Enter )		//is it CR ?
//		{
//		}
	need_redraw = 1;
    dont_pass_on = 1;
    if( ret == 1 ) dont_pass_on = 0;
	}


if ( e == FL_KEYUP )												//key release?
	{
	int key = Fl::event_key();
	
	if( ( key == FL_Control_L ) || ( key == FL_Control_R ) ) ctrl_key = 0;
	if( ( key == FL_Shift_L ) || (  key == FL_Shift_R ) ) shift_key = 0;

	need_redraw = 1;
    dont_pass_on = 0;
	}


if ( e == FL_MOUSEWHEEL )
	{
	mousewheel = Fl::event_dy();
//	printf( "dble_wnd::handle() - mousewheel: %d\n", mousewheel );

	need_redraw = 1;
    dont_pass_on = 0;
	}


if ( need_redraw ) redraw();

if( dont_pass_on ) return 1;

return Fl_Double_Window::handle(e);

}





void rtl_graph_wnd::led_freq_memory_turn_off()
{
for( int i = 0; i < cn_freq_memory_max; i++ )
	{
	ld_freq_memory[i]->ChangeCol( 0 );
	}	

}






void rtl_graph_wnd::led_filter_memory_turn_off()
{
for( int i = 0; i < cn_filter_memory_max; i++ )
	{
	ld_filter_memory[i]->ChangeCol( 0 );
	}	
}







void rtl_graph_wnd::led_preset_memory_turn_off()
{
for( int i = 0; i < cn_preset_memory_max; i++ )
	{
	ld_preset_memory[i]->ChangeCol( 0 );
	}	
}





void rtl_graph_wnd::led_dwn_srate_memory_turn_off()
{
for( int i = 0; i < cn_dwn_srate_memory_max; i++ )
	{
	ld_dwn_srate_memory[i]->ChangeCol( 0 );
	}	
}









void rtl_graph_wnd::led_preset_memory_turn_off2()
{
for( int i = 0; i < cn_preset_memory_max2; i++ )
	{
	ld_preset_memory2[i]->ChangeCol( 0 );
	}	
}













void rtl_graph_wnd::led_preset_memory_tooltip_update( bool bshow_tooltips )
{
string s1;
mystr m1;

for( int i = 0; i < cn_preset_memory_max; i++ )
	{
	string snum, sunits, scombined;
	int fractional_digits = 3;

	m1.make_engineering_str( snum, sunits, scombined, fractional_digits,  preset_memory[i].freq_tune, " ", "Hz" );

	strpf( s1, " '%s' (%s) - %s", preset_memory[i].sname.c_str(), scombined.c_str(), cns_led_preset_tooltip );

	preset_memory[i].tooltip = "";

	if( bshow_tooltips ) 
		{
		preset_memory[i].tooltip = s1;
		}

	ld_preset_memory[i]->tooltip( preset_memory[i].tooltip.c_str() );
	}	
}










void rtl_graph_wnd::led_preset_memory_label_update2()
{
string s1, s2;
mystr m1;

for( int i = 0; i < cn_preset_memory_max2; i++ )
	{
//	string snum, sunits, scombined;
//	int fractional_digits = 3;
	
//	m1.make_engineering_str( snum, sunits, scombined, fractional_digits,  preset_memory2[i].freq_tune, " ", "Hz" );

//	strpf( s1, " '%s' (%s) - %s", preset_memory2[i].sname.c_str(), scombined.c_str(), cns_led_preset_tooltip );
	
//	preset_memory[i].tooltip = "";
	
//	if( bshow_tooltips ) 
//		{
//		preset_memory2[i].tooltip = s1;
//		}

//	ld_preset_memory2[i]->tooltip( preset_memory[i].tooltip.c_str() );

	strpf( s1, "%s", preset_memory2[i].sname.c_str() );
	if( s1.length() > 16 )									//use 'WWWWWWWWWWWWWWWWWWWWWW' a wide font char str to determine where to clip label
		{
		s2 = s1.substr( 0, 16 );
		s2 += " ..";
		}
	else{
		s2 = s1;
		}
	ld_preset_memory2[i]->copy_label( s2.c_str() );
	}
}












void rtl_graph_wnd::led_preset_memory_tooltip_update2( bool bshow_tooltips )
{
string s1;
mystr m1;

for( int i = 0; i < cn_preset_memory_max2; i++ )
	{
	string snum, sunits, scombined;
	int fractional_digits = 3;
	
	m1.make_engineering_str( snum, sunits, scombined, fractional_digits,  preset_memory2[i].freq_tune, " ", "Hz" );

	strpf( s1, " '%s' (%s) - %s", preset_memory2[i].sname.c_str(), scombined.c_str(), cns_led_preset_tooltip );
	
	preset_memory2[i].tooltip = "";
	
	if( bshow_tooltips ) 
		{
		preset_memory2[i].tooltip = s1;
		}

	ld_preset_memory2[i]->tooltip( preset_memory2[i].tooltip.c_str() );
	}	
}










void rtl_graph_wnd::set_bias_t( bool bset )
{

g_dev_bias_t = bset;
	
rtl.set_bias_tee( g_dev_bias_t );
wnd_rtl_graph->ld_bias_t->ChangeCol( g_dev_bias_t );
}










void rtl_graph_wnd::toggle_bias_t()
{

g_dev_bias_t = !g_dev_bias_t;

set_bias_t( g_dev_bias_t );

//rtl.set_bias_tee( g_dev_bias_t );
//wnd_rtl_graph->ld_bias_t->ChangeCol( g_dev_bias_t );
}







void ld_filter_memory_label_update()
{
string s1;
mystr m1;

for( int i = 0; i < cn_filter_memory_max; i++ )
	{
	strpf( s1, "%d->%d", wnd_rtl_graph->filter_memory[ i ].iffreq_bw_low, wnd_rtl_graph->filter_memory[ i ].iffreq_bw_high );  

	wnd_rtl_graph->ld_filter_memory[i]->copy_label( s1.c_str() );
	}
}








void ld_dwn_srate_memory_label_update()
{
string s1;
mystr m1;

for( int i = 0; i < cn_dwn_srate_memory_max; i++ )
	{
	int fractional_digits = 0;												//KHz		

	string snum, sunits, scombined;

	int iv = wnd_rtl_graph->dwn_srate_memory[i].dwn_srate;
	
	m1.make_engineering_str( snum, sunits, scombined, fractional_digits, iv, "", "" );
	wnd_rtl_graph->ld_dwn_srate_memory[i]->copy_label( scombined.c_str() );
	}
}






//int bias_t_toggle_cnt = 0;




void rtl_graph_wnd::tick( float dt )
{
string s1;
mystr m1;

carrier_max_tune_timer += dt;


rtl_graph_tick_cnt++;

//bias_t_toggle_cnt++;
//if( bias_t_toggle_cnt > 10 )
//	{
//	bias_t_toggle_cnt = 0;
//	toggle_bias_t();
//	}

if( carrier_max_led_flash_cnt > 0 )
	{
	printf("rtl_graph_wnd::tick() - carrier_max_led_flash_cnt %d\n", carrier_max_led_flash_cnt );
	
	if( carrier_max_led_flash_cnt & 2 )
		{
		ld_carrier_max_tune->adj_brightness_offset( 255, -255, -255 );
		}
	else{
		ld_carrier_max_tune->adj_brightness_offset( 0, 0, 0 );
		}

	carrier_max_led_flash_cnt--;
	}
else{
	ld_carrier_max_tune->adj_brightness_offset( 0, 0, 0 );
	}

wnd_fav->tick( dt );


if( wnd_fav->flag_to_tune_using_fav_idx >= 0 )							//was a fav selected ?
	{
	freq_listen_using_fav_idx( wnd_fav->flag_to_tune_using_fav_idx );
	
	wnd_fav->flag_to_tune_using_fav_idx = -1;
	}


if( rtl_graph_tick_cnt == 20 ) 
	{
	printf("rtl_graph_wnd::tick() - hiding 'wnd_aa'\n" );
	wnd_aa->hide();
	}
	

rec_play_iq_state_process();


if( start_up_state < 3 )												//at startup ? 
	{
	miw_gph_disp_spect_zoom_factor->set_value_from_double( disp_spect_zoom_factor );
	}





extern void aud_spect_gui_thrd();

if( !(rtl_graph_tick_cnt % 2) ) aud_spect_gui_thrd();					//reduce plot rate


if( !(rtl_graph_tick_cnt % 2) ) 
	{
	ld_dwn_srate_memory_label_update();
	ld_filter_memory_label_update();
	}





//------ audio clipper led ------
clip_audio_cnt--;
if( clip_audio_cnt < 0 ) clip_audio_cnt = 0;

int rr = 255.0f * ( (float)clip_audio_cnt / clip_audio_cnt_max );

//if( ( !b_clip_enable ) || ( rr == 0 ) ) ld_clip->override_col( 0, 0, 0 );

if( ( clip_audio_cnt == 0 ) || ( rr == 0 ) ) ld_clip->override_col( 0, 0, 0 );
else ld_clip->override_col( rr, rr , rr );


//printf("rtl_graph_wnd::tick() - clip_audio_cnt %d\n", clip_audio_cnt );

//------------------------



//------ vert meter ------
float f0;


bool b_yellow = 0;
if( agc_gain_lvl < 1.0f ) b_yellow = 1;

if( agc_gain_lvl > 2 ) f0 = 0.6;

if( agc_gain_lvl > 40 ) f0 = 0.75;

if( agc_gain_lvl > 60 ) f0 = 1.0;


if( agc_gain_lvl < 0.7 ) f0 = 0.6;

if( agc_gain_lvl < 0.6 ) f0 = 0.8;

if( agc_gain_lvl < 0.4 ) f0 = 1.0;

rr = 0;
int gg = 0;
int bb = 0;

if( b_yellow ) { rr = 255*f0; gg = 207*f0; bb = 86; }
else { rr = 0; gg = 255*f0; bb = 0; }

if( bb > 255 ) bb = 255;
if( gg > 255 ) gg = 255;

if( b_agc ) 
	{
	ld_agc->SetColIndex( 1, rr, gg, bb );

	//------ agc indicator as a slider ------
//	agc_vmeter->set_levels( f0, 0 );

	if( agc_gain_lvl >= 1.0f ) 
		{
//		fvs_agc->range( 1, -1 );
		fvs_agc->value( agc_gain_lvl/100.0f );
		}
	else{
//		fvs_agc->range( 1, -1 );
		fvs_agc->value( agc_gain_lvl-1.0f );
		}
		
	strpf( s1, "%.2f", agc_gain_lvl );
	fvs_agc->copy_label( s1.c_str() );
	//------------------------
	}
else{
	fvs_agc->value( 0.0f );
	
	fvs_agc->copy_label( "1.0" );
	}
ld_agc->redraw();


//------ audio vert meter ------
aud_vmeter->set_levels( g_aud_lev_avg*2.0f, 0 );
//------------------------



bx_gph0_sel_sample_text->copy_label( s_gph0_sel_sample.c_str() );



//printf("rtl_graph_wnd::tick() - agc_audio_peak: %f  %d\n", agc_audio_peak, gg );

//ld_agc->adj_brightness_of_col_index( 0, 0, gg, bb );


//------------------------

m1 = wnd_rtl_graph->bx_tune->label();

m1.FindReplace( s1, " ", "\n", 0 );										//wrap 'units' text, e.g. MHz

wnd_rtl_graph->bx_keypad_number->copy_label(  m1.szptr()  );


int frq_actual = g_freq_tune + g_freq_sub_tune;

//strpf( s1, "%d Hz", frq_actual );
//wnd_rtl_graph->bx_freq_actual->copy_label( s1.c_str() );

int fractional_digits = 3;												//KHz		
if( frq_actual >= 1e6 ) fractional_digits = 6;
if( frq_actual >= 10e6 ) fractional_digits = 6;
if( frq_actual >= 100e6 ) fractional_digits = 6;
if( frq_actual >= 1000e6 ) fractional_digits = 9;

string snum, sunits, scombined;

m1.make_engineering_str( snum, sunits, scombined, fractional_digits, g_freq_tune + g_freq_sub_tune, " ", "Hz" );

strpf( s1, "%s\n%sHz", snum.c_str(), sunits.c_str() );
wnd_rtl_graph->bx_freq_actual->copy_label( s1.c_str() );





//------ restore gui ctrl value if user did not keyin a freq after hitting 'enter' key ------
if( i_skeyin_restore_cnt > 0 ) 											//one shot timer
	{
	i_skeyin_restore_cnt--;
	if( i_skeyin_restore_cnt == 0 )
		{
		if( wnd_rtl_graph->skeyin_freq.length() == 0 ) wnd_rtl_graph->miwp_tune->miw->set_value_from_double( g_freq_tune );	
		}
	}
//------------------------



//------
if( ck_user_dwn_aa->value() == 1 ) ld_aa_dwncnv_fc[0]->activate();
else ld_aa_dwncnv_fc[0]->deactivate();
//------


if( wnd_rtl_graph->ineed_graph_fit > 0) 
	{
	ineed_graph_fit--;
	if( ineed_graph_fit == 0 ) 
		{
		if( wnd_rtl_graph->bneed_graph_fit_bring_for_front ) cb_bt_graph_fit( 0, 0 );
		else gph1.fit_plot( 0 );
		}
	
//printf("rtl_graph_wnd::rtl_graph_wnd() - ineed_graph_fit %d\n", ineed_graph_fit );
	}


if( b_need_filter_rebuild )
	{
	filter_iir_rebuild();
	
	b_need_filter_rebuild = 0;
	}



if( b_need_dwn_srate_change )											//set 'downsample_srate_pending' with new srate before setting this
	{
	aud_dimmer();
	rec_play_iq_set_state( 3 );
	rec_play_iq_set_state( 1 );
	
	if( downsample_srate_pending < cn_downsample_srate_min ) downsample_srate_pending = cn_downsample_srate_min;
	if( downsample_srate_pending > cn_downsample_srate_max ) downsample_srate_pending = cn_downsample_srate_max;

	downsample_srate = downsample_srate_pending;
	fm_decimator_srate = downsample_srate / 2;

	stop_audio();
	stop_threads();

	i_fftw_trig_plan_create_state = 0;									//show fttw plans are to be created

	filter_adjust_fir_bw_lower_upper();

	start_threads_rtl();
	start_audio();

	filters_create();
	filter_iir_rebuild();
	filter_fir_rebuild();
	
	b_need_dwn_srate_change = 0;
	
	wnd_rtl_graph->miw_dev_dwnconv_srate->set_value_from_double( downsample_srate );
	}
							



//------------------------
if( b_rec_play_start_pos_req )											//need to start playing from a different file pos?
	{
	if( rec_play_iq_state == 0 )										//stopped?
		{
		rec_play_iq_set_state(2);										//start play
printf(" rtl_graph_wnd::tick() - FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF b_rec_play_start_pos_req\n" );
		b_rec_play_start_pos_req = 0;
		}
	}
//------------------------





//wnd_aa->tick( dt );
}











void rtl_graph_wnd::add_to_tune_history_skip_duplicates()
{
string s1, s2;


s1 = idb_tune->value();

//mystr m1;

//again_repl:
//m1 = s;
//if( m1.FindReplace( s, "//","/", 0 ) ) goto again_repl;		//looped removal of duplicate "//"

int count = vtunehist2.size();

for( int i = 0; i < count; i++ )								//check if already in history
	{
	if( s1.compare( vtunehist2[ i ] ) == 0 )
		{
		s2 = vtunehist2[ 0 ];
		vtunehist2[ 0 ] = vtunehist2[ i ];
		vtunehist2[ i ] = s2;
		goto skip_adding;										//if so skip adding it to history
		}
	}

vtunehist2.insert( vtunehist2.begin(), s1 );						//add to top if not already in history

skip_adding:

//fi_tune->clear();
for(int i = 0; i < count; i++ )
	{
	idb_tune->add( vtunehist2[i].c_str() );
	}

update_tune_history();
}







/*
void rtl_graph_wnd::add_to_tune_history()
{
string s1, s2;


s1 = fi_tune->value();



//mystr m1;

//again_repl:
//m1 = s;
//if( m1.FindReplace( s, "//","/", 0 ) ) goto again_repl;		//looped removal of duplicate "//"


int count = vtunehist.size();

for( int i = 0; i < count; i++ )								//check if already in history
	{
	if( s1.compare( vtunehist[ i ] ) == 0 )
		{
		s2 = vtunehist[ 0 ];
		vtunehist[ 0 ] = vtunehist[ i ];
		vtunehist[ i ] = s2;
		goto skip_adding;										//if so skip adding it to history
		}
	}

vtunehist.insert( vtunehist.begin(), s1 );						//add to top if not already in history

skip_adding:

fi_tune->clear();
for(int i = 0; i < count; i++ )
	{
	fi_tune->add( vtunehist[i].c_str() );
	}


}
*/












//reload Fl_Input_Choice entires 
void rtl_graph_wnd::reload_scanhist()
{
string s1;

fi_freq_start->clear();
fi_freq_stop->clear();

for( int i = 0; i < vscanhist.size(); i++ )
	{
	strpf( s1, "%s, %s", vscanhist[ i ].start.c_str(), vscanhist[ i ].stop.c_str() );

//	s2 = s1;
//	s2 += vscanhist[ i ].start.c_str();
	fi_freq_start->add( s1.c_str() );

//	s2 = s1;
//	s2 += vscanhist[ i ].stop.c_str();
	fi_freq_stop->add( s1.c_str() );
	}
}







void rtl_graph_wnd::morse_code_add_word( vector<st_morse_seq_tag> &vm, string s_word, bool b_add_word_silence )
{

int len = s_word.length();
for( int i = 0; i < len; i++ )
	{
	
	if( i < (len - 1) )
		{
		morse_code_add_symbol( vm, s_word[i], 1, 0 );
		}
	else{
		morse_code_add_symbol( vm, s_word[i], 0, b_add_word_silence );	//last char if here
		}
	}
}



void rtl_graph_wnd::morse_code_add_text( vector<st_morse_seq_tag> &vm, string s_text, bool b_add_word_silence )
{
string s1;
mystr m1;
m1 = s_text;

vector<string> vstr;
m1.LoadVectorStrings( vstr, ' '  );
for( int i = 0; i < vstr.size(); i++ )
	{
		
	printf("rtl_graph_wnd::morse_code_add_text() - %d: word found: '%s'\n", i, vstr[i].c_str() );
	morse_code_add_word( vm,  vstr[i].c_str(), 1 );
	}
	
}




void rtl_graph_wnd::morse_code_add_symbol( vector<st_morse_seq_tag> &vm, char ch, bool b_add_char_silence, bool b_add_word_silence )
{
//st_morse_code_tag om;

st_morse_seq_tag om;



string s1;


s1 = vmorse_alpha[ch].scode;


for( int i = 0; i < s1.length(); i++ )
	{
	if( s1[i] == '0' )
		{
		om.freq = morse_tone_freq;										//fade up
		om.dur = morse_fade_time;
		om.amp0 = 0.0f;
		om.amp1 = 1.0f;
		vm.push_back( om );


		om.freq = morse_tone_freq;
		om.dur = morse_dit_time;
		om.amp0 = 1.0f;
		om.amp1 = 1.0f;
		vm.push_back( om );


		om.freq = morse_tone_freq;										//fade down
		om.dur = morse_fade_time;
		om.amp0 = 1.0f;
		om.amp1 = 0.0f;
		vm.push_back( om );
		}

	if( s1[i] == '1' )
		{
		om.freq = morse_tone_freq;										//fade up
		om.dur = morse_fade_time;
		om.amp0 = 0.0f;
		om.amp1 = 1.0f;
		vm.push_back( om );


		om.freq = morse_tone_freq;
		om.dur = morse_dah_time;
		om.amp0 = 1.0f;
		om.amp1 = 1.0f;
		vm.push_back( om );


		om.freq = morse_tone_freq;										//fade down
		om.dur = morse_fade_time;
		om.amp0 = 1.0f;
		om.amp1 = 0.0f;
		vm.push_back( om );
		}

	if( s1[i] == ',' )
		{
		om.freq = 0.0f;													//silence
		om.dur = morse_dit_time;
		om.amp0 = 0.0f;
		om.amp1 = 0.0f;
		vm.push_back( om );
		}
	}
	

if( b_add_word_silence )
	{
	om.freq = 0.0f;														//silence
	om.dur = morse_word_gap_time;
	om.amp0 = 0.0f;
	om.amp1 = 0.0f;
	vm.push_back( om );
	}
else{
	if( b_add_char_silence )
		{
		om.freq = 0.0f;													//silence
		om.dur = morse_letter_gap_time;
		om.amp0 = 0.0f;
		om.amp1 = 0.0f;
		vm.push_back( om );
		}
	
	}	

}





bool rtl_graph_wnd::morse_render_to_audio_file( vector<st_morse_seq_tag> &vm, string fname, int srate_in )
{
audio_formats af0;
st_audio_formats_tag saf0;

saf0.format = en_af_wav_pcm;
saf0.srate = srate_in;
saf0.offset = 28;
saf0.encoding = 3;
saf0.channels = 1;
saf0.is_big_endian = 1;
saf0.bits_per_sample = 16;



float freq0 = 400;
float tim_per_smpl = 1.0f/srate_in;


float theta0 = 0;
float theta_inc0 = freq0 * twopi * tim_per_smpl;

bool bdone = 0;
for( int i = 0; i < 120*srate_in; i++ )
	{
	float famp0 = 0.25;


//----- render morse sinewave tones
	if( 1 )
		{
		freq0 = vm[ morse_idx ].freq;

		float amp0 = vm[ morse_idx ].amp0;
		float amp1 = vm[ morse_idx ].amp1;
		
		float ctrl_mix = morse_time_note / vm[ morse_idx ].dur;		//1.0 at start of dur, 0.0 at end of dur
	//			if( wnd_rtl_graph->morse_idx == 0 ) printf("ctrl_mix %f    %f/%f\n", ctrl_mix, wnd_rtl_graph->morse_time_note , vmorse0[ wnd_rtl_graph->morse_idx ].dur );
		float ampl = amp0*ctrl_mix + (amp1 * (1.0f-ctrl_mix));
		
	//			if( wnd_rtl_graph->morse_idx == 0 ) printf("ampl1 %f\n", ampl1 );

		morse_time_note -= tim_per_smpl;			//play a musical note seq
		morse_time_tot += tim_per_smpl;
		if( morse_time_note <= 0.0f ) 
			{
			morse_idx++;
	//				printf( ".........playing music %d   %f\n", wnd_rtl_graph->music_idx, wnd_rtl_graph->music_time_note ); 
			if( morse_idx >= vm.size() )
				{
				morse_idx = 0;
				morse_time_note = vm[ morse_idx ].dur;
				bdone = 1;
				}
			else{
				morse_time_note = vm[ morse_idx ].dur;	
				}
			}

		float f0 = ampl * sinf( theta0 );

		af0.push_ch0( f0 );
		
		if( bdone ) break;
		}
//--------

	
	theta0 += theta_inc0;
	if( theta0 >= twopi ) theta0 -= twopi;
	}



if( !af0.save_malloc( "", fname, 32767, saf0 ) )
	{
	printf("rtl_graph_wnd::morse_render_to_audio_file() - failed to save audio file: '%s'\n", fname.c_str() );
	return 0;
	}

return 1;
}










void rtl_graph_wnd::morse_code_table_build()
{
st_morse_code_tag om;

morse_tone_freq = cn_morse_tone_freq;
morse_dit_time = cn_morse_dit_time;
morse_dah_time = cn_morse_dah_time;
morse_word_gap_time = cn_morse_word_gap_time;


om.scode = "";

for( int i = 0; i < 128; i++ )
	{
	vmorse_alpha.push_back( om );
	}


int idx;

idx = '0';
vmorse_alpha[idx].scode = "1,1,1,1,1";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = '1';
vmorse_alpha[idx].scode = "0,1,1,1,1";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = '2';
vmorse_alpha[idx].scode = "0,0,1,1,1";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = '3';
vmorse_alpha[idx].scode = "0,0,0,1,1";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = '4';
vmorse_alpha[idx].scode = "0,0,0,0,1";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = '5';
vmorse_alpha[idx].scode = "0,0,0,0,0";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = '6';
vmorse_alpha[idx].scode = "1,0,0,0,0";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = '7';
vmorse_alpha[idx].scode = "1,1,0,0,0";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = '8';
vmorse_alpha[idx].scode = "1,1,1,0,0";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = '9';
vmorse_alpha[idx].scode = "1,1,1,1,0";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;


idx = 'A';
vmorse_alpha[idx].scode = "0,1";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'B';
vmorse_alpha[idx].scode = "1,0,0,0";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'C';
vmorse_alpha[idx].scode = "1,0,1,0";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'D';
vmorse_alpha[idx].scode = "1,0,0";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'E';
vmorse_alpha[idx].scode = "0";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'F';
vmorse_alpha[idx].scode = "0,0,1,0";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'G';
vmorse_alpha[idx].scode = "1,1,0";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'H';
vmorse_alpha[idx].scode = "0,0,0,0";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'I';
vmorse_alpha[idx].scode = "0,0";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'J';
vmorse_alpha[idx].scode = "0,1,1,1";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'K';
vmorse_alpha[idx].scode = "1,0,1";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'L';
vmorse_alpha[idx].scode = "0,1,0,0";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'M';
vmorse_alpha[idx].scode = "1,1";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'N';
vmorse_alpha[idx].scode = "1,0";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'O';
vmorse_alpha[idx].scode = "1,1,1";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'P';
vmorse_alpha[idx].scode = "0,1,1,0";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'Q';
vmorse_alpha[idx].scode = "1,1,0,1";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'R';
vmorse_alpha[idx].scode = "0,1,0";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'S';
vmorse_alpha[idx].scode = "0,0,0";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'T';
vmorse_alpha[idx].scode = "1";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'U';
vmorse_alpha[idx].scode = "0,0,1";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'V';
vmorse_alpha[idx].scode = "0,0,0,1";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'W';
vmorse_alpha[idx].scode = "0,1,1";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'X';
vmorse_alpha[idx].scode = "1,0,0,1";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'Y';
vmorse_alpha[idx].scode = "1,0,1,1";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

idx = 'Z';
vmorse_alpha[idx].scode = "1,1,0,0";
vmorse_alpha[idx+0x20].scode = vmorse_alpha[idx].scode;

}









void rtl_graph_wnd::morse_build()
{
morse_code_table_build();


st_morse_seq_tag om;



morse_fade_time = cn_morse_fade_time;
morse_letter_gap_time = cn_morse_letter_gap_time;
morse_word_gap_time = cn_morse_word_gap_time;

float morse_music_time = 0.5f;

//------ music phrase ------
//---
om.freq = 1046.5f;
om.dur = morse_fade_time;
om.amp0 = 0.0f;
om.amp1 = 1.0f;
vmorse10.push_back( om );


om.freq = 1046.5f;
om.dur = morse_music_time;
om.amp0 = 1.0f;
om.amp1 = 1.0f;
vmorse10.push_back( om );


om.freq = 1046.5f;
om.dur = morse_fade_time;
om.amp0 = 1.0f;
om.amp1 = 0.0f;
vmorse10.push_back( om );
//---



//---
om.freq = 0.0f;										//silence
om.dur = morse_word_gap_time;
om.amp0 = 0.0f;
om.amp1 = 0.0f;
vmorse10.push_back( om );
//---



//---
om.freq = 1174.66f;
om.dur = morse_fade_time;
om.amp0 = 0.0f;
om.amp1 = 1.0f;
vmorse10.push_back( om );

om.freq = 1174.66f;
om.dur = morse_music_time;
om.amp0 = 1.0f;
om.amp1 = 1.0f;
vmorse10.push_back( om );

om.freq = 1174.66f;
om.dur = morse_fade_time;
om.amp0 = 1.0f;
om.amp1 = 0.0f;
vmorse10.push_back( om );

//---


//---
om.freq = 0.0f;										//silence
om.dur = morse_word_gap_time;
om.amp0 = 0.0f;
om.amp1 = 0.0f;
vmorse10.push_back( om );
//---


//---
om.freq = 932.33f;
om.dur = morse_fade_time;
om.amp0 = 0.0f;
om.amp1 = 1.0f;
vmorse10.push_back( om );

om.freq = 932.33f;
om.dur = morse_music_time;
om.amp0 = 1.0f;
om.amp1 = 1.0f;
vmorse10.push_back( om );

om.freq = 932.33f;
om.dur = morse_fade_time;
om.amp0 = 1.0f;
om.amp1 = 0.0f;
vmorse10.push_back( om );

//---


//---
om.freq = 0.0f;										//silence
om.dur = morse_word_gap_time;
om.amp0 = 0.0f;
om.amp1 = 0.0f;
vmorse10.push_back( om );
//---





//---
om.freq = 587.33f;
om.dur = morse_fade_time;
om.amp0 = 0.0;
om.amp1 = 2.5f;
vmorse10.push_back( om );

om.freq = 587.33f;
om.dur = morse_music_time*2;
om.amp0 = 2.5f;
om.amp1 = 2.5f;
vmorse10.push_back( om );

om.freq = 587.33f;
om.dur = morse_fade_time;
om.amp0 = 2.5f;
om.amp1 = 0.0f;
vmorse10.push_back( om );
//---


//---
om.freq = 0.0f;										//silence
om.dur = morse_word_gap_time;
om.amp0 = 0.0f;
om.amp1 = 0.0f;
vmorse10.push_back( om );
//---



//---
om.freq = 932.33f;
om.dur = morse_fade_time;
om.amp0 = 0.0f;
om.amp1 = 1.0f;
vmorse10.push_back( om );

om.freq = 932.33f;
om.dur = morse_music_time*3;
om.amp0 = 1.0f;
om.amp1 = 1.0f;
vmorse10.push_back( om );

om.freq = 932.33f;
om.dur = morse_fade_time;
om.amp0 = 1.0f;
om.amp1 = 0.0f;
vmorse10.push_back( om );
//---


//---
om.freq = 0.0f;										//silence
om.dur = morse_word_gap_time*3;
om.amp0 = 0.0f;
om.amp1 = 0.0f;
vmorse10.push_back( om );
//---
//---------------------------





vmorse1.clear();

//morse_code_add_word( vmorse1, "hello", 1 );
//morse_code_add_word( vmorse1, "world", 1 );
morse_code_add_text( vmorse0, ",,,,,,,,, sos ,,,,,,,, brown", 1 );
morse_code_add_text( vmorse1, "quick brown fox jumped over the lazy dog 0123456789", 1 );

//morse_code_add_symbol( vmorse0, 'S', 0, 1 );
//morse_code_add_symbol( vmorse0, 'S', 0, 1 );






//vmorse0 = vmorse1;														//set which seq to play



morse_play_which = 0;
morse_idx = 0;
morse_time_note = vmorse0[0].dur;
morse_time_tot = 0.0f;
morse_reset = 0;


if( 1 )
	{
	morse_render_to_audio_file( vmorse0, cn_synth_voice_fname0, 24000 );

	morse_play_which = -1;												//restore any changed vals
	morse_idx = 0;
	morse_time_note = vmorse0[0].dur;
	morse_time_tot = 0.0f;
	morse_reset = 0;
	}


if( 1 )
	{
	morse_render_to_audio_file( vmorse1, cn_synth_voice_fname1, 24000 );

	morse_play_which = -1;												//restore any changed vals
	morse_idx = 0;
	morse_time_note = vmorse0[0].dur;
	morse_time_tot = 0.0f;
	morse_reset = 0;
	}


}











void rtl_graph_wnd::add_to_scan_history()
{
string s1, s2;

st_scan_freq_tag sf1, sf2;

sf1.start = fi_freq_start->value();
sf1.stop = fi_freq_stop->value();

//s1 = fi_freq_start->value();



//mystr m1;

//again_repl:
//m1 = s;
//if( m1.FindReplace( s, "//","/", 0 ) ) goto again_repl;		//looped removal of duplicate "//"


int count = vscanhist.size();

for( int i = 0; i < count; i++ )		//check if already in history
	{
	if( ( sf1.start.compare( vscanhist[ i ].start ) == 0 ) && ( sf1.stop.compare( vscanhist[ i ].stop ) == 0 ) )
		{
		sf2 = vscanhist[ 0 ];
		vscanhist[ 0 ] = vscanhist[ i ];
		vscanhist[ i ] = sf2;
		last_scan_history_start = 0;				//rem were are set first menu choice entry
		last_scan_history_stop = 0;					//rem were are set first menu choice entry
		goto skip_adding;							//if so skip adding it to history
		}
	}

vscanhist.insert( vscanhist.begin(), sf1 );			//add to top if not already in history

skip_adding:


reload_scanhist();

/*
fi_freq_start->clear();
fi_freq_stop->clear();
for( int i = 0; i < count; i++ )
	{
	strpf( s1, "%02d..", i );
	
	s2 = s1;
	s2 += vscanhist[ i ].start.c_str();
	fi_freq_start->add( s2.c_str() );

	s2 = s1;
	s2 += vscanhist[ i ].stop.c_str();
	fi_freq_stop->add( s2.c_str() );

//	fi_freq_start->add( vscanhist[ i ].start.c_str() );
//	fi_freq_stop->add( vscanhist[ i ].stop.c_str() );
	}

*/
}











//with a complex sample size of 256, fftw ordering after a fwd complex fft is:

//fftw mapping example below:
//     negative most          first_neg         dc      first_positive             most positive
//128,     129.......->..........255            0            1.........->............ 127,         128		(128 is nyquist)

//zero between min and max, spectrum must be in fftw ordering
//min and max range should be in human values not fftw loc values
//DC at loc 0 will also be cleared if it lies in range
void zero_spectrum( int min, int max, vector <filter_code::st_cplex_tag> &vsp )
{


int sz = vsp.size();
if( sz <= 1 ) return;

//if( min > max )
//	{
//	int tmp = min;
//	min = max;
//	max = tmp;
//	}


int half = sz / 2;

int pos1;
int pos2;
int neg1;
int neg2;
bool zero_pos = 0;
bool zero_neg = 0;

if( min >= sz ) return;

if( max < ( -half + 1 ) ) max = -half + 1;

if( max > half ) max = half;					//limit to most pos

if( min < ( -half + 1 ) ) min= -half + 1;		//limit to most neg


if( ( min >= 0 ) && ( max > min ) )
	{
	pos1 = min;
	pos2 = max;
	zero_pos = 1;		
	}


if( ( min <= 0 ) && ( max > 0 ) )
	{
	pos1 = 0;
	pos2 = max;

	neg1 = sz + min;		
	neg2 = sz;
	zero_pos = 1;
	zero_neg = 1;		
	}


if( ( min < 0 ) && ( max == 0 ) )
	{
	neg2 = sz + max;		
	neg1 = sz + min;
	zero_neg = 1;		
	}


if( ( min <= 0 ) && ( max < min ) )
	{
	neg1 = sz + max;		
	neg2 = sz + min;
	zero_neg = 1;		
//printf( "matching zero_pos= %d, zero_neg= %d, neg1= %d, neg2= %d\n", zero_pos, zero_neg, neg1, neg2 );	
	}


if( min == max )
	{
	neg1 = min;		
	neg2 = min;

	if( min >= 0 )
		{
		pos1 = min;
		pos2 = min;
		zero_pos = 1;
		}
	else{
		zero_neg = 1;
		neg1 = sz + min;		
		neg2 = sz + min;
		}

	}



//pos freqs
if( zero_pos )
	{
	for( int i = 0; i < half; i++ )					//start after dc
		{
		if( ( i >= pos1 ) && ( i <= pos2 ) )
			{
			vsp[ i ].real = 0;
			vsp[ i ].imag = 0;
			}
		}
	}


//neg freqs
if( zero_neg )
	{
	for( int i = half + 1; i < sz; i++ )
		{
		if( ( i >= neg1 ) && ( i <= neg2 ) )
			{
			vsp[ i ].real = 0;
			vsp[ i ].imag = 0;
			}
		}

	}

}










//with a complex sample size of 256, fftw ordering after a fwd complex fft is:

//fftw mapping example below:
//     negative most          first_neg         dc      first_positive             most positive
//128,     129.......->..........255            0            1.........->............ 127,         128		(128 is nyquist)

//pass spectrum between lwr and upr, spectrum must be in fftw ordering
//lwr and upr range should be in human values not fftw loc values
//DC at loc 0 will also be cleared if it lies out of range
void pass_spectrum_hard( int lwr, int upr, vector <filter_code::st_cplex_tag> &vsp )
{


int sz = vsp.size();
if( sz <= 1 ) return;


if( lwr > upr )
	{
	int tmp = lwr;
	lwr = upr;
	upr = tmp;
	}

filter_code::st_cplex_tag ozero;
int half = sz / 2;

ozero.real = 0;
ozero.imag = 0;

for( int i = -half + 1; i < half; i++ )
	{
	
	if ( ( i >= lwr ) && ( i <= upr ) )
		{

		}
	else{
		int ptr = i;
		if( i < 0 ) ptr = sz + i;
		
		vsp[ ptr ] = ozero;
		}

	}

}











//with a complex sample size of 256, fftw ordering after a fwd complex fft is:

//fftw mapping example below:
//     negative most          first_neg         dc      first_positive             most positive
//128,     129.......->..........255            0            1.........->............ 127,         128		(128 is nyquist)

//shift spectrum by spec offest, see also 'spectrum_shift()'
//positive offset moves spectrum to high positive freqs,
//will skip moving DC at loc 0,

void spectrum_shift_complex( int offset, vector <filter_code::st_cplex_tag> &vsp )
{
int sz = vsp.size();
if( sz <= 1 ) return;

if( offset == 0 ) return;

int half = sz / 2;

if( offset >= half ) return;

vector <filter_code::st_cplex_tag> vtmp;
filter_code::st_cplex_tag o;

vtmp = vsp;

//vtmp.reserve( sz );							//make dest vector same size as src

vtmp[ 0 ] = vsp[ 0 ];						//copy DC at loc 0


int psrc;
int pdest;

if( offset > 0 )								//moving right, to more positive freqs?
	{
	int idxsrc = -half + 1 - offset;
	int idxdest = -half + 1;


	for( int i = 0; i < ( sz - 1 ); i++ )
		{
		psrc = idxsrc + 1;
		if( idxsrc < 0 ) psrc = sz + idxsrc;			//adj for neg fftw loc

		pdest = idxdest + 1;
		if( idxdest < 0 ) pdest = sz + idxdest;			//adj for neg fftw loc

		if( idxsrc < ( -half + 1 ) )					//src index past end?
			{
			vtmp[ pdest ].real = 0;
			vtmp[ pdest ].imag = 0;
//printf(" idxsrc= %03d, idxdest= %03d, psrc= %03d, pdest= %03d, storing zero\n", idxsrc, idxdest, psrc, pdest );
			}
		else{
			vtmp[ pdest ] = vsp[ psrc ];
//printf(" idxsrc= %03d, idxdest= %03d, psrc= %03d, pdest= %03d, vsp[ psrc ]= %lf\n", idxsrc, idxdest, psrc, pdest, vsp[ psrc ].real );
			}
		idxsrc++;
		idxdest++;

		}
	}
else{
	//moving left, to more negative freqs?
	int idxsrc =  half  - 1 - offset;
	int idxdest = half - 1;


	for( int i = 0; i < ( sz - 1 ); i++ )		 		//move positives, low freqs first
		{
		psrc = idxsrc + 1;
		if( idxsrc < 0 ) psrc = sz + idxsrc;			//adj for neg fftw loc

		pdest = idxdest + 1;
		if( idxdest < 0 ) pdest = sz + idxdest;			//adj for neg fftw loc

		if( idxsrc >= half )							//src index past end?
			{
			vtmp[ pdest ].real = 0;
			vtmp[ pdest ].imag = 0;
//printf(" idxsrc= %03d, idxdest= %03d, psrc= %03d, pdest= %03d, storing zero\n", idxsrc, idxdest, psrc, pdest );
			}
		else{
			vtmp[ pdest ] = vsp[ psrc ];
//printf(" idxsrc= %03d, idxdest= %03d, psrc= %03d, pdest= %03d\n", idxsrc, idxdest, psrc, pdest );
			}
		idxsrc--;
		idxdest--;

		}
	}


vsp = vtmp;

//printf(" vsp.size()= %d\n", vsp.size() );
}






//positive 'offset' moves spectrum upwards freq wise, see also 'spectrum_shift_complex()'
//adds null spectra where required, i.e: both '.ampl' and '.freq' are zero,
//setting 'keep_same_bin_count' will result in some supplied spectra being lost due to the add of null spectra
void spectrum_shift( int offset, vector <st_spect_tag> &vsp, bool keep_same_bin_count )
{
int sz = vsp.size();
if( sz <= 1 ) return;

if( offset == 0 ) return;

int half = sz / 2;

vector <st_spect_tag> vv;


int cnt = vsp.size();
if( offset > 0 )
	{
	//add null spectra first
	for( int i = 0; i < offset; i++ )
		{
		st_spect_tag o;
		o.freq = 0;
		o.ampl = 0;
		vv.push_back( o );
		}

	if( keep_same_bin_count )
		{
		cnt -= offset;		//reduce supplied spectrum count
		if( cnt < 0 ) cnt = 0;
		}

	//add supplied spectra
	for( int i = 0; i < cnt; i++ )
		{
		st_spect_tag o;
		
		o.freq = vsp[i].freq;
		o.ampl = vsp[i].ampl;
		
		vv.push_back( o );
		}
	}
else{
	offset = -offset;			//make offset positve
	int start_idx = 0;
	if( keep_same_bin_count )
		{
		start_idx = offset;		//reduce supplied spectrum count
		if( start_idx >= cnt ) cnt = 0;
		}

	//add supplied spectra
	for( int i = offset; i < cnt; i++ )
		{
		st_spect_tag o;
		
		o.freq = vsp[i].freq;
		o.ampl = vsp[i].ampl;
		
		vv.push_back( o );
		}

	//add null spectra last
	for( int i = 0; i < offset; i++ )
		{
		st_spect_tag o;
		o.freq = 0;
		o.ampl = 0;
		vv.push_back( o );
		}
	
	}
vsp = vv;
}







//algebraic add random noise to 'vtime_domain', allows dc components to be included
//checks if vtime_domain is emtpy and will push_back to create new samples if so, otherwise algebraic addition to existing samples will occur
//useful for diagnosing fftw problems, see 'fftw_complex_test_work_out()'
void add_complex_noise_signal( double srate, int size, double dc_offset_real, double dc_offset_imag, double ampl, vector <filter_code::st_cplex_tag> &vtime_domain )
{
int sz = vtime_domain.size();

if( sz != 0 )
	{
	if ( size != sz )
		{
		printf( "add_complex_noise_signal() - 'vtime_domain.size()'[%d], does not match 'size'[%d]\n", vtime_domain.size(), size );
		return;
		}
	}


bool is_first = 0;
if( sz == 0 ) is_first = 1;

filter_code::st_cplex_tag o;

for ( int i = 0; i < size; i++ )
	{
	double val_r = dc_offset_real + ampl * ( (double) rand() / (double)( RAND_MAX / 2 ) - 1.0 );	//RAND_MAX is 2147483647 0x7fffffff
	double val_i = dc_offset_imag + ampl * ( (double) rand() / (double)( RAND_MAX / 2 ) - 1.0 );	//RAND_MAX is 2147483647 0x7fffffff

	if( is_first )
		{
		o.real = val_r;
		o.imag = val_i;	
		vtime_domain.push_back( o );
		}
	else{
		vtime_domain[ i ].real += val_r;
		vtime_domain[ i ].imag += val_i;
		}

	}
//printf("noise %lf\n", o.real );
}







//algebraic add a complex tone to 'vtime_domain', allows dc components to be included, allows phase offets between real and imaginary,
//checks if vtime_domain is emtpy and will push_back to create new samples if so, otherwise algebraic addition to existing samples will occur
//useful for diagnosing fftw problems, see 'fftw_complex_test_work_out()'
void add_complex_test_tone( double srate, int size, double dc_offset_real, double dc_offset_imag, double freq, double ampl, double phase_real, double phase_imag, vector <filter_code::st_cplex_tag> &vtime_domain )
{

int sz = vtime_domain.size();

if( sz != 0 )
	{
	if ( size != sz )
		{
		printf( "add_complex_test_tone() - 'vtime_domain.size()'[%d], does not match 'size'[%d]\n", vtime_domain.size(), size );
		return;
		}
	}


bool is_first = 0;
if( sz == 0 ) is_first = 1;

double time_per_sample = 1.0 / srate;

double theta_inc = freq * twopi * time_per_sample;
double theta = 0;

filter_code::st_cplex_tag o;

for ( int i = 0; i < size; i++ )
	{
	double val_r = dc_offset_real + ampl * cos( theta + phase_real );
	double val_i = dc_offset_imag + ampl * sin( theta + phase_imag );

	if( is_first )
		{
		o.real = val_r;
		o.imag = val_i;	
		vtime_domain.push_back( o );
		}
	else{
		vtime_domain[ i ].real += val_r;
		vtime_domain[ i ].imag += val_i;
		}

	theta += theta_inc;
	if( theta >= twopi ) theta -= twopi;
//printf( "val_r= %lf\n", val_r );

	}

//printf( "add_complex_test_signal() - 'vtime_domain.size()= %d\n", vtime_domain.size() );
}












//algebraic add a real tone to 'vtime_domain', allows dc component to be included, allows phase offet,
//checks if vtime_domain is emtpy and will push_back to create new sampmes if so, otherwise algebraic addition to existing samples will occur
//useful for diagnosing fftw problems, see 'fftw_real_test_work_out()'
void add_real_test_tone( double srate, int size, double dc_offset_real, double freq, double ampl, double phase_real, vector <double> &vtime_domain )
{

int sz = vtime_domain.size();

if( sz != 0 )
	{
	if ( size != sz )
		{
		printf( "add_real_test_tone() - 'vtime_domain.size()'[%d], does not match 'size'[%d]\n", vtime_domain.size(), size );
		return;
		}
	}


bool is_first = 0;
if( sz == 0 ) is_first = 1;

double time_per_sample = 1.0 / srate;

double theta_inc = freq * twopi * time_per_sample;
double theta = 0;


for ( int i = 0; i < size; i++ )
	{
	double val_r = dc_offset_real + ampl * cos( theta + phase_real );

	if( is_first )
		{
		vtime_domain.push_back( val_r );
		}
	else{
		vtime_domain[ i ] += val_r;
		}

	theta += theta_inc;
	if( theta >= twopi ) theta -= twopi;
//printf( "val_r= %lf\n", val_r );

	}

//printf( "add_real_test_tone() - 'vtime_domain.size()= %d\n", vtime_domain.size() );
}























//use this to understand complex fftw formatting, it allows some tests to be run
void fftw_complex_test_work_out()
{
vector <filter_code::st_cplex_tag> vtdm;			//time domain complex array, load by calling add_complex_test_tone(..)
vector <filter_code::st_cplex_tag> vtdm2;		//time domain complex array, load by calling add_complex_test_tone(..)
vector <filter_code::st_cplex_tag> vfft;			//freq domain complex array after fftw has completed
vector <filter_code::st_cplex_tag> vc;			//freq domain complex array after fftw has completed

	//below is code to test fftw complex fwd transforms via test tones, it was set to srate of 22050, and fftw size was 256, 
	//the array locations of spectra shown are in fftw order, spectra amplitudes did match sinewave amplitudes exactly,
	//loc 0 is DC term, loc 1->127 are positive increasing spectra(127 locs), loc 255->129 are negative increasing spectra (127 locs),
	//loc 128 represents either a positive or negative nyquist freq, can't differentiate between these two freq (11025.0Hz)
	
	//mapping example below:
	//     negative_most          first_neg         dc      first_positive             most_positive
	//128,     129.......->..........255            0            1.........->............ 127,         128		(128 is nyquist)

	//looking at it another way, positives and matching negative locations
	//dc term:   0
	//positives: 1,   2,   3,    4 ....->.....125, 126, 127,   (127 is highest positive freq, execpt of nyquist)
	//negatives: 255, 254, 253,  252...->.....131, 130, 129,   (129 is highest negative freq, execpt of nyquist)
	//nyquist:   128, --- you can't make out if this is a pos or neg freq, as either appear in this loc, see 'fftw_complex_test_work_out()'

//	add_complex_noise_signal( srate, 256, 0, 0, 1.0, vtdm );						//add noise


//	add_complex_test_tone( srate, 256, 1.0, 0.0, 500, 0.0, 0, 0,  vtdm  );			//DC only, at loc 0, value of 1
	add_complex_test_tone( aud_op_srate, 256, 0.5, 0.0, 86.133, 1.0, 0, 0, vtdm  );		//positive, at loc 1, 1st postive, (one whole cos cycle in 256 array) 
//	add_complex_test_tone( srate, 256, 0.5, 0.0, 86.133, 1.0, 0, M_PI, vtdm  );		//negative, at loc 255, 1st negative,(one whole cos cycle in 256 array) 
//	add_complex_test_tone( srate, 256, 0.0, 0.0, 172.266, 1.0, 0, 0, vtdm  );		//positive, at loc 2
//	add_complex_test_tone( srate, 256, 0.0, 0.0, 172.266, 1.0, 0, M_PI, vtdm  );	//negative, at loc 254
//	add_complex_test_tone( srate, 256, 0.0, 0.0, 1808.79, 1.0, 0, 0, vtdm  );		//positive, at loc 21
//	add_complex_test_tone( srate, 256, 0.0, 0.0, 1808.79, 1.0, 0, M_PI, vtdm  );	//negative, at loc 235
//	add_complex_test_tone( srate, 256, 0.0, 0.0, 10852.735, 1.0, 0, 0, vtdm  );		//positive, at loc 126
//	add_complex_test_tone( srate, 256, 0.0, 0.0, 10852.735, 1.0, 0, M_PI, vtdm  );	//negative, at loc 130
//	add_complex_test_tone( srate, 256, 0.0, 0.0, 10938.867, 1.0, 0, 0, vtdm  );		//positive, at loc 127 (max pos freq, not counting nyquist)
//	add_complex_test_tone( srate, 256, 0.0, 0.0, 10938.867, 1.0, 0, M_PI, vtdm  );	//negative, at loc 129 (max neg freq, not counting nyquist) 
//	add_complex_test_tone( srate, 256, 0.0, 0.0, 11025.0, 1.0, 0, 0, vtdm  );		//positive, at loc 128, at nyquist, same as negtive at 11025.0 
//	add_complex_test_tone( srate, 256, 0.0, 0.0, 11025.0, 1.0, 0, M_PI, vtdm  );	//negative, at loc 128, at nyquist, same as positive at 11025.0 
//	add_complex_test_tone( srate, 256, 0.0, 0.0, 11111.133, 1.0, 0, 0, vtdm  );		//positive, at loc 129, beyond nyquist, folding back into max neg
//	add_complex_test_signal( srate, 256, 0.0, 0.0, 11111.133, 1.0, 0, M_PI, vtdm  );	//negative, at loc 127, beyond nyquist, folding back into max pos

	complex_fwd_fft_multi( vtdm, vfft, "called by fftw_complex_test_work_out()", en_ftid_gui );							//use this to do a fwd fft, need to have a plan that's complex to complex and 256 samples in size
//	complex_fft_displayable_fftw_order( vfft, vspect_demod );		//use this to display fftw's spectra ordering (where neg freq are backward, i.e: loc 129 holds largest neg spectrum)

//	zero_spectrum( -126, 126, vfft );

	spectrum_shift_complex( -127, vfft );
	spectrum_shift_complex( 1, vfft );

//	complex_rev_fft_multi( vfft, vtdm2 );

//	displayable_complex_time_domain( vtdm2, 0, vspect_demod );

	complex_fft_displayable( aud_op_srate, 0, vfft, vspect_displayble, 1, 1 );					//use this to conv and display fftw's spectra in human order(negs first followed by positives)

//graph 'vspect_demod' to see visually using: complex_fft_displayable_fftw_order()
}
















//use this to understand real fftw formatting, it allows some tests to be run
void fftw_real_test_work_out()
{
vector <double> vtdm;				//time domain real array, load by calling add_real_test_tone(..)
vector <filter_code::st_cplex_tag> vtdm2;		//time domain complex array, load by calling add_complex_test_signal(..)
vector <filter_code::st_cplex_tag> vfft;			//freq domain complex array after fftw has completed
vector <filter_code::st_cplex_tag> vc;			//freq domain complex array after fftw has completed


//add_real_test_tone( srate, 256, 0, 86.133, 1.0, 0, vtdm );		//positive, at loc 1, 1st postive, (one whole cos cycle in 256 array)
add_real_test_tone( aud_op_srate, 256, 0, 86.133, 1.0, M_PI, vtdm );		//negative, at loc 1, 1st postive, (one whole sin cycle in 256 array)
//add_real_test_tone( srate, 256, 0, 172.266, 1.0, 0, vtdm );		//positive, at loc 2
//add_real_test_tone( srate, 256, 0, 172.266, 1.0, M_PI, vtdm );	//negative, at loc 2
//add_real_test_tone( srate, 256, 0, 11025.0, 1.0, 0, vtdm );
//add_real_test_tone( srate, 256, 0, 11025.0, 1.0, M_PI, vtdm );
//add_real_test_tone( srate, 256, 0, 10938.867, 1.0, 0, vtdm );		//positive, at loc 127 (max pos freq, not counting nyquist)
//add_real_test_tone( srate, 256, 0, 10938.867, 1.0, M_PI, vtdm );	//negative, at loc ??? (max pos freq, not counting nyquist)
//add_real_test_tone( srate, 256, 0, 10852.735, 1.0, 0, vtdm );		//positive, at loc 126
//add_real_test_tone( srate, 256, 0, 10852.735, 1.0, M_PI, vtdm );	//negative, at loc ???


real_fwd_fft_multi( vtdm, vfft, "gggg0", en_ftid_gui );							//use this to do a fwd fft, need to have a plan that's real to complex and 256 samples in size
//spectrum_shift_complex( -3, vfft );

//complex_fft_displayable_fftw_order( vfft, vspect_demod );		//use this to conv and display spectra in fftw ordering
//real_rev_fft_multi( vfft, vtdm );

//complex_fft_displayable( vfft, vspect_demod, 0 );				//use this to conv and display fftw's spectra in human order(negs first followed by positives)
displayable_real_time_domain( vtdm, vspect_displayble );				//use this to show real time domain data

}








void complex_amplifier( vector <filter_code::st_cplex_tag>&vin, double gain_real, double gain_imag )
{
int sz = vin.size();

if( sz == 0 ) return;

for ( int i = 0; i < sz; i++ )
	{
	vin[ i ].real *= gain_real;
	vin[ i ].imag *= gain_imag;
	}


}













#define cn_filter_real_real_avg_hist 100

double filter_real_real_avg_prev_sampl[ cn_filter_real_real_avg_hist ];


void filter_real_real_avg( vector <double> &vin, vector <double> &vout, unsigned int hist_count )
{
vout.clear();

int sz = vin.size();

if( sz == 0 ) return;

if( hist_count == 0 ) return;


for( int i = 0; i < sz; i++ )
	{
	double val = 0;

	int one_less = hist_count - 1;
	
	for( int j = 0; j < hist_count; j++ )
		{
		double prev = filter_real_real_avg_prev_sampl[ j ];				//ripple history down one loc
		if( j < one_less ) filter_real_real_avg_prev_sampl[ j ] = filter_real_real_avg_prev_sampl[ j + 1 ];

		val += prev;									//accum
		}

	val /= (double)hist_count;							//avg

	vout.push_back( val );

	filter_real_real_avg_prev_sampl[ hist_count - 1 ] = vin[ i ];	//load next sample to filter into history buf's end
	
	}

}





#define cn_filter_complex_real_avg_hist 100

double filter_complex_real_avg_prev_sampl[ cn_filter_complex_real_avg_hist ];






void filter_complex_real_avg( vector <filter_code::st_cplex_tag> &vcplx, vector <double> &vreal, bool imag, unsigned int hist_count )
{
vreal.clear();

int sz = vcplx.size();

if( sz == 0 ) return;

if( hist_count == 0 ) return;


if( hist_count >= cn_filter_complex_real_avg_hist ) hist_count = cn_filter_complex_real_avg_hist - 1;


for( int i = 0; i < sz; i++ )
	{
	double val = 0;

	int one_less = hist_count - 1;
	
	for( int j = 0; j < hist_count; j++ )
		{
		double prev = filter_complex_real_avg_prev_sampl[ j ];				//ripple history down one loc
		if( j < one_less ) filter_complex_real_avg_prev_sampl[ j ] = filter_complex_real_avg_prev_sampl[ j + 1 ];

		val += prev;									//accum
		}

	val /= (double)hist_count;							//avg

	vreal.push_back( val );

	filter_code::st_cplex_tag o = vcplx[ i ];
	if ( imag ) filter_complex_real_avg_prev_sampl[ hist_count - 1 ] = o.imag;	//load next sample to filter into history buf's end
	else filter_complex_real_avg_prev_sampl[ hist_count - 1 ] = o.real;
	
	
	}
	
}











vector <filter_code::st_cplex_tag> vsquelch_avg;
double squelch_avg = 0;
int squelch_timout = 0;
int downsample_factor;
int down_sample_factor_for_graph = 40;

int demod_call_cnt = 0;
int demod_call_cnt_modulo0 = 50;





/*


rtlsdr_callback()					                                    AudioProc
~~~~~~~~~~~~~~~                                                         ~~~~~~~~~
rtl adc rate 2496000 Hz (78x 32000Hz audio)								32000 Hz by framecnt 2048 = 64.000mS


2496000 i/q pairs per second

4992000 Bytes/s (write/rate for [i+q])									Need to read 2496000 i/q pairs per second, so 64mS*2496000 = 159744 i/q pairs per audio callback (4992000 Bytes/s)

if rtl async buf is set for 262144 byte length then
rtlsdr_callback() period is  262144/4992000  52.513 mS




rtlsdr_callback()					                                    AudioProc
~~~~~~~~~~~~~~~                                                         ~~~~~~~~~
rtl adc rate 3200000 Hz (100x 32000Hz audio)							32000 Hz by 2048 framecnt = 64.000mS


3200000 i/q pairs per second

6400000 Bytes/s (write/rate for [i+q])									Need to read 3200000 i/q pairs per second, so 64mS*3200000 = 204800 i/q pairs per audio callback (6400000 Bytes/s)

if rtl async buf is set for 262144 byte length then
rtlsdr_callback() period is  262144/6400000  40.960 mS

*/







//calc 'iq_count' and 'iq_count_downsample' used by 'demod_iso()' and for fftw plan creation, see 'i_fftw_trig_plan_create_state'
int rtl_graph_wnd::iq_count_calc( int &iq_count_downsample, int &iq_count_downsample_widefm )
{
int iq_count = nearbyint( g_dev_bw * ( (1.0f/aud_op_srate) * framecnt) ); 					//rtl adc srate, e.g: 2496000 Byte/s x audio_proc_period (64mS[32KHz]) = 159744

//printf("iq_count_calc() - downsample_srate %d  downsample_factor %d\n", downsample_srate, downsample_factor );

downsample_factor = g_dev_bw / downsample_srate;

if( downsample_factor < 1 ) downsample_factor = 1;						//avoid divide by zero below

iq_count_downsample = iq_count / downsample_factor;


iq_count_downsample_widefm = iq_count / cn_down_sample_factor_small;

return iq_count;
}



















//see also 'fftw_adjust_plans()'
void rtl_graph_wnd::fftw_create_plans()
{
printf( "rtl_graph_wnd::fftw_create_plans()\n" );


//alloc fftw pointers
fft_in_c0 = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftw_size_demod );
fft_out_c0 = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftw_size_demod );

//single dimension complex to complex fwd fft
fft_p_fwd_c0 = fftw_plan_dft_1d( fftw_size_demod, fft_in_c0, fft_out_c0, FFTW_FORWARD, FFTW_ESTIMATE );

//single dimension complex to complex rev fft
fft_p_rvs_c0 = fftw_plan_dft_1d( fftw_size_demod, fft_in_c0, fft_out_c0, FFTW_BACKWARD, FFTW_ESTIMATE );






fftwsiz[ 0 ] = fftw_size_demod_0;
fftwsiz[ 1 ] = fftw_size_demod_1;
fftwsiz[ 2 ] = fftw_size_demod_2;
fftwsiz[ 3 ] = fftw_size_demod_3;
fftwsiz[ 4 ] = fftw_size_demod_4;
fftwsiz[ 5 ] = fftw_size_demod_5;
fftwsiz[ 6 ] = fftw_size_demod_6;

fftwsiz[ 7 ] = fftw_size_demod_7;
fftwsiz[ 8 ] = fftw_size_demod_8;
fftwsiz[ 9 ] = fftw_size_demod_9;
fftwsiz[ 10 ] = fftw_size_demod_10;
fftwsiz[ 11 ] = fftw_size_demod_11;
fftwsiz[ 12 ] = fftw_size_demod_12;
fftwsiz[ 13 ] = fftw_size_demod_13;
fftwsiz[ 14 ] = fftw_size_demod_14;
fftwsiz[ 15 ] = fftw_size_demod_15;
fftwsiz[ 16 ] = fftw_size_demod_16;
fftwsiz[ 17 ] = fftw_size_demod_17;
fftwsiz[ 18 ] = fftw_size_demod_18;


fftw_plan_countc = sizeof( fftwsiz ) / sizeof( unsigned int );		//work out how many plans required


for ( int i = 0; i < fftw_plan_countc; i++ )
	{
	//alloc fftw pointers
	fftwic[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftwsiz[ i ] );
	fftwoc[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftwsiz[ i ] );

	//single dimension complex to complex fwd fft
	fftwp_fwdc[ i ] = fftw_plan_dft_1d( fftwsiz[ i ], fftwic[ i ], fftwoc[ i ], FFTW_FORWARD, FFTW_ESTIMATE );
//	ftw_plan_thrd_fwdc[ i ] = en_thrid_audio_proc;

	if( i == 0 ) ftw_plan_thrd_id_fwdc[ i ] = fftw_thrd_id_0;			//set which thread can use this plan
	if( i == 1  ) ftw_plan_thrd_id_fwdc[ i ] = fftw_thrd_id_1;
	if( i == 2  ) ftw_plan_thrd_id_fwdc[ i ] = fftw_thrd_id_2;
	if( i == 3  ) ftw_plan_thrd_id_fwdc[ i ] = fftw_thrd_id_3;
	if( i == 4  ) ftw_plan_thrd_id_fwdc[ i ] = fftw_thrd_id_4;
	if( i == 5 ) ftw_plan_thrd_id_fwdc[ i ] = fftw_thrd_id_5;
	if( i == 6 ) ftw_plan_thrd_id_fwdc[ i ] = fftw_thrd_id_6;
	if( i == 7 ) ftw_plan_thrd_id_fwdc[ i ] = fftw_thrd_id_7;
	if( i == 8 ) ftw_plan_thrd_id_fwdc[ i ] = fftw_thrd_id_8;
	if( i == 9 ) ftw_plan_thrd_id_fwdc[ i ] = fftw_thrd_id_9;
	if( i == 10 ) ftw_plan_thrd_id_fwdc[ i ] = fftw_thrd_id_10;
	if( i == 11 ) ftw_plan_thrd_id_fwdc[ i ] = fftw_thrd_id_11;
	if( i == 12 ) ftw_plan_thrd_id_fwdc[ i ] = fftw_thrd_id_12;
	if( i == 13 ) ftw_plan_thrd_id_fwdc[ i ] = fftw_thrd_id_13;
	if( i == 14 ) ftw_plan_thrd_id_fwdc[ i ] = fftw_thrd_id_14;
	if( i == 15 ) ftw_plan_thrd_id_fwdc[ i ] = fftw_thrd_id_15;
	if( i == 16 ) ftw_plan_thrd_id_fwdc[ i ] = fftw_thrd_id_16;
	if( i == 17 ) ftw_plan_thrd_id_fwdc[ i ] = fftw_thrd_id_17;
	if( i == 18 ) ftw_plan_thrd_id_fwdc[ i ] = fftw_thrd_id_18;


	//single dimension complex to complex rev fft
	fftwp_rvsc[ i ] = fftw_plan_dft_1d( fftwsiz[ i ], fftwic[ i ], fftwoc[ i ], FFTW_BACKWARD, FFTW_ESTIMATE );


	if( i == 0 ) ftw_plan_thrd_id_rvsc[ i ] = fftw_thrd_id_0;			//set which thread can use this plan
	if( i == 1  ) ftw_plan_thrd_id_rvsc[ i ] = fftw_thrd_id_1;
	if( i == 2  ) ftw_plan_thrd_id_rvsc[ i ] = fftw_thrd_id_2;
	if( i == 3  ) ftw_plan_thrd_id_rvsc[ i ] = fftw_thrd_id_3;
	if( i == 4  ) ftw_plan_thrd_id_rvsc[ i ] = fftw_thrd_id_4;
	if( i == 5 ) ftw_plan_thrd_id_rvsc[ i ] = fftw_thrd_id_5;
	if( i == 6 ) ftw_plan_thrd_id_rvsc[ i ] = fftw_thrd_id_6;
	if( i == 7 ) ftw_plan_thrd_id_rvsc[ i ] = fftw_thrd_id_7;
	if( i == 8 ) ftw_plan_thrd_id_rvsc[ i ] = fftw_thrd_id_8;
	if( i == 9 ) ftw_plan_thrd_id_rvsc[ i ] = fftw_thrd_id_9;
	if( i == 10 ) ftw_plan_thrd_id_rvsc[ i ] = fftw_thrd_id_10;
	if( i == 11 ) ftw_plan_thrd_id_rvsc[ i ] = fftw_thrd_id_11;
	if( i == 12 ) ftw_plan_thrd_id_rvsc[ i ] = fftw_thrd_id_12;
	if( i == 13 ) ftw_plan_thrd_id_rvsc[ i ] = fftw_thrd_id_13;
	if( i == 14 ) ftw_plan_thrd_id_rvsc[ i ] = fftw_thrd_id_14;
	if( i == 15 ) ftw_plan_thrd_id_rvsc[ i ] = fftw_thrd_id_15;
	if( i == 16 ) ftw_plan_thrd_id_rvsc[ i ] = fftw_thrd_id_16;
	if( i == 17 ) ftw_plan_thrd_id_rvsc[ i ] = fftw_thrd_id_17;
	if( i == 18 ) ftw_plan_thrd_id_rvsc[ i ] = fftw_thrd_id_18;

	}


//printf( "help1\n" );

fftw_plan_countr = fftw_plan_countc;									//work out how many plans required

for ( int i = 0; i < fftw_plan_countr; i++ )
	{
	//alloc fftw pointers
	fftwir[ i ] = ( double* ) fftw_malloc( sizeof( double ) * fftwsiz[ i ] );
	fftwor[ i ] = ( double* ) fftw_malloc( sizeof( double ) * fftwsiz[ i ] );

	//single dimension real to complex fwd fft
	fftwp_fwdr[ i ] = fftw_plan_dft_r2c_1d( fftwsiz[ i ], fftwir[ i ], fftwoc[ i ], FFTW_ESTIMATE );

	if( i == 0 ) ftw_plan_thrd_id_fwdr[ i ] = fftw_thrd_id_0;			//set which thread can use this plan
	if( i == 1  ) ftw_plan_thrd_id_fwdr[ i ] = fftw_thrd_id_1;
	if( i == 2  ) ftw_plan_thrd_id_fwdr[ i ] = fftw_thrd_id_2;
	if( i == 3  ) ftw_plan_thrd_id_fwdr[ i ] = fftw_thrd_id_3;
	if( i == 4  ) ftw_plan_thrd_id_fwdr[ i ] = fftw_thrd_id_4;
	if( i == 5 ) ftw_plan_thrd_id_fwdr[ i ] = fftw_thrd_id_5;
	if( i == 6 ) ftw_plan_thrd_id_fwdr[ i ] = fftw_thrd_id_6;
	if( i == 7 ) ftw_plan_thrd_id_fwdr[ i ] = fftw_thrd_id_7;
	if( i == 8 ) ftw_plan_thrd_id_fwdr[ i ] = fftw_thrd_id_8;
	if( i == 9 ) ftw_plan_thrd_id_fwdr[ i ] = fftw_thrd_id_9;
	if( i == 10 ) ftw_plan_thrd_id_fwdr[ i ] = fftw_thrd_id_10;
	if( i == 11 ) ftw_plan_thrd_id_fwdr[ i ] = fftw_thrd_id_11;
	if( i == 12 ) ftw_plan_thrd_id_fwdr[ i ] = fftw_thrd_id_12;
	if( i == 13 ) ftw_plan_thrd_id_fwdr[ i ] = fftw_thrd_id_13;
	if( i == 14 ) ftw_plan_thrd_id_fwdr[ i ] = fftw_thrd_id_14;
	if( i == 15 ) ftw_plan_thrd_id_fwdr[ i ] = fftw_thrd_id_15;
	if( i == 16 ) ftw_plan_thrd_id_fwdr[ i ] = fftw_thrd_id_16;
	if( i == 17 ) ftw_plan_thrd_id_fwdr[ i ] = fftw_thrd_id_17;
	if( i == 18 ) ftw_plan_thrd_id_fwdr[ i ] = fftw_thrd_id_18;


	//single dimension complex to real rev fft
	fftwp_rvsr[ i ] = fftw_plan_dft_c2r_1d( fftwsiz[ i ], fftwic[ i ], fftwor[ i ], FFTW_ESTIMATE );

	if( i == 0 ) ftw_plan_thrd_id_rvsr[ i ] = fftw_thrd_id_0;			//set which thread can use this plan
	if( i == 1  ) ftw_plan_thrd_id_rvsr[ i ] = fftw_thrd_id_1;
	if( i == 2  ) ftw_plan_thrd_id_rvsr[ i ] = fftw_thrd_id_2;
	if( i == 3  ) ftw_plan_thrd_id_rvsr[ i ] = fftw_thrd_id_3;
	if( i == 4  ) ftw_plan_thrd_id_rvsr[ i ] = fftw_thrd_id_4;
	if( i == 5 ) ftw_plan_thrd_id_rvsr[ i ] = fftw_thrd_id_5;
	if( i == 6 ) ftw_plan_thrd_id_rvsr[ i ] = fftw_thrd_id_6;
	if( i == 7 ) ftw_plan_thrd_id_rvsr[ i ] = fftw_thrd_id_7;
	if( i == 8 ) ftw_plan_thrd_id_rvsr[ i ] = fftw_thrd_id_8;
	if( i == 9 ) ftw_plan_thrd_id_rvsr[ i ] = fftw_thrd_id_9;
	if( i == 10 ) ftw_plan_thrd_id_rvsr[ i ] = fftw_thrd_id_10;
	if( i == 11 ) ftw_plan_thrd_id_rvsr[ i ] = fftw_thrd_id_11;
	if( i == 12 ) ftw_plan_thrd_id_rvsr[ i ] = fftw_thrd_id_12;
	if( i == 13 ) ftw_plan_thrd_id_rvsr[ i ] = fftw_thrd_id_13;
	if( i == 14 ) ftw_plan_thrd_id_rvsr[ i ] = fftw_thrd_id_14;
	if( i == 15 ) ftw_plan_thrd_id_rvsr[ i ] = fftw_thrd_id_15;
	if( i == 16 ) ftw_plan_thrd_id_rvsr[ i ] = fftw_thrd_id_16;
	if( i == 17 ) ftw_plan_thrd_id_rvsr[ i ] = fftw_thrd_id_17;
	if( i == 18 ) ftw_plan_thrd_id_rvsr[ i ] = fftw_thrd_id_18;
	}

i_fftw_trig_plan_create_state = 1;
}









void rtl_graph_wnd::fftw_destroy_plans()
{
printf( "rtl_graph_wnd::fftw_destroy_plans()\n" );

//free fftw items
if ( fft_p_fwd_c0 ) fftw_destroy_plan( fft_p_fwd_c0 );
fft_p_fwd_c0 = 0;

if ( fft_p_rvs_c0 ) fftw_destroy_plan( fft_p_rvs_c0 );
fft_p_rvs_c0 = 0;


if ( fft_in_c0 ) fftw_free( fft_in_c0 );
if ( fft_out_c0 ) fftw_free( fft_out_c0 );
fft_in_c0 = 0;
fft_out_c0 = 0;




for ( int i = 0; i < fftw_plan_countc; i++ )
	{

	if( fftwp_fwdc[ i ] ) fftw_destroy_plan( fftwp_fwdc[ i ] );
	fftwp_fwdc[ i ] = 0;

	if(  fftwp_rvsc[ i ] ) fftw_destroy_plan( fftwp_rvsc[ i ] );
	fftwp_rvsc[ i ] = 0;

	if( fftwic[ i ] ) fftw_free( fftwic[ i ] );
	fftwic[ i ] = 0;
	
	if( fftwoc[ i ] ) fftw_free( fftwoc[ i ] );
	fftwoc[ i ] = 0;
	}



for ( int i = 0; i < fftw_plan_countr; i++ )
	{

	if( fftwp_fwdr[ i ] ) fftw_destroy_plan( fftwp_fwdr[ i ] );
	fftwp_fwdr[ i ] = 0;
	
	if( fftwp_rvsr[ i ] ) fftw_destroy_plan( fftwp_rvsr[ i ] );
	fftwp_rvsr[ i ] = 0;

	if( fftwir[ i ] ) fftw_free( fftwir[ i ] );
	fftwir[ i ] = 0;
	
	if( fftwor[ i ] ) fftw_free( fftwor[ i ] );
	fftwor[ i ] = 0;
	}

}







st_fftw_plans_tag st_fftw[ cn_fftw_plan_siz ];


//----------------------------------------------------------------------

//clear fftw structs, see also: 'fftw_build_if_req()' 'fftw_destroy_all_structs()'
void fftw_clear_all_structs()
{
bool vb = vb0;

if(vb)printf("fftw_clear_all_structs() - clearing %d fftw structs\n", cn_fftw_plan_siz );

for( int i = 0; i < cn_fftw_plan_siz; i++ )
	{
	st_fftw_plans_tag *of = st_fftw + i;
	
	of->built = 0;
	of->type = en_fpt_illegal_type;
	of->size = 0;
	of->sname = "";

	of->fftwp = 0;

	of->fftwir = 0;
	of->fftwor = 0;
	of->fftwic = 0;
	of->fftwoc = 0;
	}
}





//rebuild a plan if type and size are different
//refer also 'fftw_clear_struct()'  'fftw_destroy_all_structs()'
//'b_destroy_only' frees all allocation if any
bool fftw_build_if_req(en_fftw_index_allocations_tag idx, bool b_destroy_only, const char* szname_in, en_fftw_plans_type_tag typ, unsigned int size )
{
bool vb = vb0;

if( size < 32 )
	{
	printf( "fftw_build_if_req() - fftw size is too small: %d for st_fftw[%d], min is %d -- '%s'\n", idx, size, 32, szname_in ); 
	return 0;
	}


if( idx >= cn_fftw_plan_siz )
	{
	printf( "fftw_build_if_req() - idx %d is out of range, max is %d -- '%s'\n", idx, cn_fftw_plan_siz - 1, szname_in  ); 
	return 0;
	}


if( typ >= en_fpt_illegal_type ) 
	{
	printf( "fftw_build_if_req() - typ %d is out of range, max is %d -- '%s'\n", typ, en_fpt_illegal_type - 1, szname_in  ); 
	return 0;
	}



st_fftw_plans_tag *of = st_fftw + idx;


int b_build = 0;

if( typ != of->type ) b_build = 1;
if( size != of->size ) b_build = 1;

if( !of->built ) b_build = 1;

if( !b_build )
	{
//	if(1)printf( "fftw_build_if_req() - already built and correct type and size, st_fftw[%d] -- '%s'\n", idx, szname_in ); 
	return 1;				//already built to the correct type an size ?
	}



if( of->built )
	{
	if(vb)printf( "fftw_build_if_req() - freeing fftw obj in st_fftw[%d] -- '%s'\n", idx, of->sname.c_str() ); 
	
	fftw_destroy_plan( of->fftwp );
	
	fftw_free( of->fftwir );
	fftw_free( of->fftwor );
	fftw_free( of->fftwic );
	fftw_free( of->fftwoc );
	}


if( b_destroy_only ) 
	{
	if(vb)printf( "fftw_build_if_req() - not rebuilding st_fftw[%d] -- '%s'\n", idx, of->sname.c_str() ); 

	of->built = 0;
	
	of->type = en_fpt_illegal_type;
	of->size = 0;
	of->sname = "";
	
	of->fftwp = 0;

	of->fftwir = 0;
	of->fftwor = 0;
	of->fftwic = 0;
	of->fftwoc = 0;
	return 0;
	}



of->built = 1;
of->type = typ;
of->size = size;
of->sname = szname_in; 													//"0: fftw fwd cplx to cplx: for graticle graph"

if(vb)printf( "fftw_build_if_req() - rebuilding st_fftw[%d], type %d, size %d -- '%s'\n", idx, of->type, of->size, szname_in ); 


of->fftwir = ( double* ) fftw_malloc( sizeof( double ) * of->size );
of->fftwor = ( double* ) fftw_malloc( sizeof( double ) * of->size );

of->fftwic = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * of->size );
of->fftwoc = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * of->size );

if( typ == en_fpt_fwd_real_to_cplx ) of->fftwp = fftw_plan_dft_r2c_1d( of->size, of->fftwir, of->fftwoc, FFTW_ESTIMATE );
if( typ == en_fpt_fwd_cplx_to_cplx ) of->fftwp = fftw_plan_dft_1d( of->size, of->fftwic, of->fftwoc, FFTW_FORWARD, FFTW_ESTIMATE );

if( typ == en_fpt_rvs_cplx_to_real ) of->fftwp = fftw_plan_dft_c2r_1d( of->size, of->fftwic, of->fftwor, FFTW_ESTIMATE );
if( typ == en_fpt_rvs_cplx_to_cplx ) of->fftwp = fftw_plan_dft_1d( of->size, of->fftwic, of->fftwoc, FFTW_BACKWARD, FFTW_ESTIMATE );


return 1;
}






//refer 'fftw_clear_struct()'  'fftw_build_if_req()'
void fftw_destroy_all_structs()
{
bool vb = vb0;

if(vb)printf("fftw_destroy_all_structs() - freeing %d fftw structs\n", cn_fftw_plan_siz );

for( int i = 0; i < cn_fftw_plan_siz; i++ )
	{
	st_fftw_plans_tag *of = st_fftw + i;
	
	bool b_destroy_only = 1;
	
	fftw_build_if_req( i, b_destroy_only, "", en_fpt_fwd_real_to_cplx, 32 );	//pass some dummy param as only destroying
	}
}
//----------------------------------------------------------------------







//adjusts some fftw plans based on device adc samplerate (bandwidth) and pc audio samplerate
//see also 'fftw_create_plans()'
//DON'T call this directly if threads are running, use 'i_fftw_trig_plan_create_state' state to initiate a call to this function
void rtl_graph_wnd::fftw_adjust_plans()
{
bool vb = 1;


int iq_count_downsample, iq_count_downsample_widefm;
int iq_count = iq_count_calc( iq_count_downsample, iq_count_downsample_widefm );

printf( "rtl_graph_wnd::fftw_adjust_plans() - >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n" );
printf( "rtl_graph_wnd::fftw_adjust_plans() - iq_count %d  iq_count_downsample %d  iq_count_downsample_widefm %d (g_dev_bw %f)\n", iq_count, iq_count_downsample, iq_count_downsample_widefm, g_dev_bw );
printf( "rtl_graph_wnd::fftw_adjust_plans() - pref_zero_padding_for_fft %d\n", pref_zero_padding_for_fft );

//---
int i = 0;

if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 0\n" );

fftw_destroy_plan( fftwp_fwdc[ i ] );
if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 1\n" );

fftw_destroy_plan( fftwp_rvsc[ i ] );
if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 2\n" );

fftw_free( fftwic[ i ] );
fftw_free( fftwoc[ i ] );
if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 3\n" );


int siz = iq_count;

printf( "rtl_graph_wnd::fftw_adjust_plans() - fftwsiz[%d] %d\n", i, siz );

//alloc fftw pointers
fftwic[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * siz );
fftwoc[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * siz );

//single dimension complex to complex fwd fft
fftwp_fwdc[ i ] = fftw_plan_dft_1d( siz, fftwic[ i ], fftwoc[ i ], FFTW_FORWARD, FFTW_ESTIMATE );

//single dimension complex to complex rev fft
fftwp_rvsc[ i ] = fftw_plan_dft_1d( siz, fftwic[ i ], fftwoc[ i ], FFTW_BACKWARD, FFTW_ESTIMATE );


//single dimension real to complex fwd fft
fftwp_fwdr[ i ] = fftw_plan_dft_r2c_1d( siz, fftwir[ i ], fftwoc[ i ], FFTW_ESTIMATE );


//single dimension complex to real fwd fft
fftwp_rvsr[ i ] = fftw_plan_dft_c2r_1d( siz, fftwic[ i ], fftwor[ i ], FFTW_ESTIMATE );

fftwsiz[ i ] = siz;
//---


//return;


//---
i = 1;
if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 4\n" );

fftw_destroy_plan( fftwp_fwdc[ i ] );
fftw_destroy_plan( fftwp_rvsc[ i ] );

fftw_free( fftwic[ i ] );
fftw_free( fftwoc[ i ] );

if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 5\n" );

siz = iq_count_downsample;

printf( "rtl_graph_wnd::fftw_adjust_plans() - fftwsiz[%d] %d\n", i, siz );

fftwic[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * siz );
fftwoc[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * siz );


fftwp_fwdc[ i ] = fftw_plan_dft_1d( siz, fftwic[ i ], fftwoc[ i ], FFTW_FORWARD, FFTW_ESTIMATE );

fftwp_rvsc[ i ] = fftw_plan_dft_1d( siz, fftwic[ i ], fftwoc[ i ], FFTW_BACKWARD, FFTW_ESTIMATE );


//single dimension real to complex fwd fft
fftwp_fwdr[ i ] = fftw_plan_dft_r2c_1d( siz, fftwir[ i ], fftwoc[ i ], FFTW_ESTIMATE );


//single dimension complex to real fwd fft
fftwp_rvsr[ i ] = fftw_plan_dft_c2r_1d( siz, fftwic[ i ], fftwor[ i ], FFTW_ESTIMATE );
fftwsiz[ i ] = siz;
//---



//return;



//---
i = 2;
if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 6\n" );
fftw_destroy_plan( fftwp_fwdc[ i ] );
fftw_destroy_plan( fftwp_rvsc[ i ] );

fftw_free( fftwic[ i ] );
fftw_free( fftwoc[ i ] );
if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 7\n" );

fftwsiz[ i ] = iq_count_downsample + 1;			//do one slightly bigger incase downsampler 'low_pass_srconv()' is slighty over, see 'demod_iso()'

printf( "rtl_graph_wnd::fftw_adjust_plans() - fftwsiz[%d] %d\n", i, fftwsiz[ i ] );

fftwic[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftwsiz[ i ] );
fftwoc[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftwsiz[ i ] );


fftwp_fwdc[ i ] = fftw_plan_dft_1d( fftwsiz[ i ], fftwic[ i ], fftwoc[ i ], FFTW_FORWARD, FFTW_ESTIMATE );

fftwp_rvsc[ i ] = fftw_plan_dft_1d( fftwsiz[ i ], fftwic[ i ], fftwoc[ i ], FFTW_BACKWARD, FFTW_ESTIMATE );


//single dimension real to complex fwd fft
fftwp_fwdr[ i ] = fftw_plan_dft_r2c_1d( fftwsiz[ i ], fftwir[ i ], fftwoc[ i ], FFTW_ESTIMATE );


//single dimension complex to real fwd fft
fftwp_rvsr[ i ] = fftw_plan_dft_c2r_1d( fftwsiz[ i ], fftwic[ i ], fftwor[ i ], FFTW_ESTIMATE );
//---



//--- widefm
i = 3;
if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 8\n" );
fftw_destroy_plan( fftwp_fwdc[ i ] );
fftw_destroy_plan( fftwp_rvsc[ i ] );

fftw_free( fftwic[ i ] );
fftw_free( fftwoc[ i ] );
if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 9\n" );

fftwsiz[ i ] = iq_count_downsample_widefm;

printf( "rtl_graph_wnd::fftw_adjust_plans() - fftwsiz[%d] %d\n", i, fftwsiz[ i ] );

fftwic[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftwsiz[ i ] );
fftwoc[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftwsiz[ i ] );


fftwp_fwdc[ i ] = fftw_plan_dft_1d( fftwsiz[ i ], fftwic[ i ], fftwoc[ i ], FFTW_FORWARD, FFTW_ESTIMATE );

fftwp_rvsc[ i ] = fftw_plan_dft_1d( fftwsiz[ i ], fftwic[ i ], fftwoc[ i ], FFTW_BACKWARD, FFTW_ESTIMATE );


//single dimension real to complex fwd fft
fftwp_fwdr[ i ] = fftw_plan_dft_r2c_1d( fftwsiz[ i ], fftwir[ i ], fftwoc[ i ], FFTW_ESTIMATE );


//single dimension complex to real fwd fft
fftwp_rvsr[ i ] = fftw_plan_dft_c2r_1d( fftwsiz[ i ], fftwic[ i ], fftwor[ i ], FFTW_ESTIMATE );
//---


//---
i = 4;
if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 10\n" );
fftw_destroy_plan( fftwp_fwdc[ i ] );
fftw_destroy_plan( fftwp_rvsc[ i ] );

fftw_free( fftwic[ i ] );
fftw_free( fftwoc[ i ] );
if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 11\n" );

fftwsiz[ i ] = iq_count_downsample_widefm + 1;			//do one slightly bigger incase downsampler 'low_pass_srconv()' is slighty over, see 'demod_iso()'

printf( "rtl_graph_wnd::fftw_adjust_plans() - fftwsiz[%d] %d\n", i, fftwsiz[ i ] );

fftwic[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftwsiz[ i ] );
fftwoc[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftwsiz[ i ] );


fftwp_fwdc[ i ] = fftw_plan_dft_1d( fftwsiz[ i ], fftwic[ i ], fftwoc[ i ], FFTW_FORWARD, FFTW_ESTIMATE );

fftwp_rvsc[ i ] = fftw_plan_dft_1d( fftwsiz[ i ], fftwic[ i ], fftwoc[ i ], FFTW_BACKWARD, FFTW_ESTIMATE );


//single dimension real to complex fwd fft
fftwp_fwdr[ i ] = fftw_plan_dft_r2c_1d( fftwsiz[ i ], fftwir[ i ], fftwoc[ i ], FFTW_ESTIMATE );


//single dimension complex to real fwd fft
fftwp_rvsr[ i ] = fftw_plan_dft_c2r_1d( fftwsiz[ i ], fftwic[ i ], fftwor[ i ], FFTW_ESTIMATE );
//---




//---
i = 5;
if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 12\n" );
fftw_destroy_plan( fftwp_fwdc[ i ] );
fftw_destroy_plan( fftwp_rvsc[ i ] );

fftw_free( fftwic[ i ] );
fftw_free( fftwoc[ i ] );
if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 13\n" );

fftwsiz[ i ] = iq_count;

printf( "rtl_graph_wnd::fftw_adjust_plans() - fftwsiz[%d] %d\n", i, fftwsiz[ i ] );

fftwic[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftwsiz[ i ] );
fftwoc[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftwsiz[ i ] );


fftwp_fwdc[ i ] = fftw_plan_dft_1d( fftwsiz[ i ], fftwic[ i ], fftwoc[ i ], FFTW_FORWARD, FFTW_ESTIMATE );

fftwp_rvsc[ i ] = fftw_plan_dft_1d( fftwsiz[ i ], fftwic[ i ], fftwoc[ i ], FFTW_BACKWARD, FFTW_ESTIMATE );


//single dimension real to complex fwd fft
fftwp_fwdr[ i ] = fftw_plan_dft_r2c_1d( fftwsiz[ i ], fftwir[ i ], fftwoc[ i ], FFTW_ESTIMATE );


//single dimension complex to real fwd fft
fftwp_rvsr[ i ] = fftw_plan_dft_c2r_1d( fftwsiz[ i ], fftwic[ i ], fftwor[ i ], FFTW_ESTIMATE );
//---






//---
i = 6;
if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 14\n" );
fftw_destroy_plan( fftwp_fwdc[ i ] );
fftw_destroy_plan( fftwp_rvsc[ i ] );

fftw_free( fftwic[ i ] );
fftw_free( fftwoc[ i ] );
if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 15\n" );

fftwsiz[ i ] = iq_count*4;

printf( "rtl_graph_wnd::fftw_adjust_plans() - fftwsiz[%d] %d\n", i, fftwsiz[ i ] );

fftwic[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftwsiz[ i ] );
fftwoc[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftwsiz[ i ] );


fftwp_fwdc[ i ] = fftw_plan_dft_1d( fftwsiz[ i ], fftwic[ i ], fftwoc[ i ], FFTW_FORWARD, FFTW_ESTIMATE );

fftwp_rvsc[ i ] = fftw_plan_dft_1d( fftwsiz[ i ], fftwic[ i ], fftwoc[ i ], FFTW_BACKWARD, FFTW_ESTIMATE );


//single dimension real to complex fwd fft
fftwp_fwdr[ i ] = fftw_plan_dft_r2c_1d( fftwsiz[ i ], fftwir[ i ], fftwoc[ i ], FFTW_ESTIMATE );


//single dimension complex to real fwd fft
fftwp_rvsr[ i ] = fftw_plan_dft_c2r_1d( fftwsiz[ i ], fftwic[ i ], fftwor[ i ], FFTW_ESTIMATE );
//---







i = 14;

if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 16\n" );
fftw_destroy_plan( fftwp_fwdc[ i ] );
fftw_destroy_plan( fftwp_rvsc[ i ] );

fftw_free( fftwic[ i ] );
fftw_free( fftwoc[ i ] );
if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 17\n" );


fftwsiz[ i ] = fftwsiz[ i ];

printf( "rtl_graph_wnd::fftw_adjust_plans() - idx 14, fftwsiz[%d] %d\n", i, fftwsiz[ i ] );

//alloc fftw pointers
fftwic[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftwsiz[ i ] );
fftwoc[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftwsiz[ i ] );

//single dimension complex to complex fwd fft
fftwp_fwdc[ i ] = fftw_plan_dft_1d( fftwsiz[ i ], fftwic[ i ], fftwoc[ i ], FFTW_FORWARD, FFTW_ESTIMATE );

//single dimension complex to complex rev fft
fftwp_rvsc[ i ] = fftw_plan_dft_1d( fftwsiz[ i ], fftwic[ i ], fftwoc[ i ], FFTW_BACKWARD, FFTW_ESTIMATE );


//single dimension real to complex fwd fft
fftwp_fwdr[ i ] = fftw_plan_dft_r2c_1d( fftwsiz[ i ], fftwir[ i ], fftwoc[ i ], FFTW_ESTIMATE );


//single dimension complex to real fwd fft
fftwp_rvsr[ i ] = fftw_plan_dft_c2r_1d( fftwsiz[ i ], fftwic[ i ], fftwor[ i ], FFTW_ESTIMATE );
//---







//---
i = 15;

if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 18\n" );
fftw_destroy_plan( fftwp_fwdc[ i ] );
fftw_destroy_plan( fftwp_rvsc[ i ] );

fftw_free( fftwic[ i ] );
fftw_free( fftwoc[ i ] );
if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 19\n" );


fftwsiz[ i ] = fftwsiz[ i ];

printf( "rtl_graph_wnd::fftw_adjust_plans() - idx 15, fftwsiz[%d] %d\n", i, fftwsiz[ i ] );

//alloc fftw pointers
fftwic[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftwsiz[ i ] );
fftwoc[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftwsiz[ i ] );

//single dimension complex to complex fwd fft
fftwp_fwdc[ i ] = fftw_plan_dft_1d( fftwsiz[ i ], fftwic[ i ], fftwoc[ i ], FFTW_FORWARD, FFTW_ESTIMATE );

//single dimension complex to complex rev fft
fftwp_rvsc[ i ] = fftw_plan_dft_1d( fftwsiz[ i ], fftwic[ i ], fftwoc[ i ], FFTW_BACKWARD, FFTW_ESTIMATE );


//single dimension real to complex fwd fft
fftwp_fwdr[ i ] = fftw_plan_dft_r2c_1d( fftwsiz[ i ], fftwir[ i ], fftwoc[ i ], FFTW_ESTIMATE );


//single dimension complex to real fwd fft
fftwp_rvsr[ i ] = fftw_plan_dft_c2r_1d( fftwsiz[ i ], fftwic[ i ], fftwor[ i ], FFTW_ESTIMATE );
//---







i = 16;

if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 20\n" );
fftw_destroy_plan( fftwp_fwdc[ i ] );
fftw_destroy_plan( fftwp_rvsc[ i ] );

fftw_free( fftwic[ i ] );
fftw_free( fftwoc[ i ] );
if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 21\n" );


fftwsiz[ i ] = iq_count * ( 2 - (!pref_zero_padding_for_fft) );			//also take into account is padding is enabled

printf( "rtl_graph_wnd::fftw_adjust_plans() - idx 16, fftwsiz[%d] %d\n", i, fftwsiz[ i ] );

//alloc fftw pointers
fftwic[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftwsiz[ i ] );
fftwoc[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftwsiz[ i ] );

//single dimension complex to complex fwd fft
fftwp_fwdc[ i ] = fftw_plan_dft_1d( fftwsiz[ i ], fftwic[ i ], fftwoc[ i ], FFTW_FORWARD, FFTW_ESTIMATE );

//single dimension complex to complex rev fft
fftwp_rvsc[ i ] = fftw_plan_dft_1d( fftwsiz[ i ], fftwic[ i ], fftwoc[ i ], FFTW_BACKWARD, FFTW_ESTIMATE );


//single dimension real to complex fwd fft
fftwp_fwdr[ i ] = fftw_plan_dft_r2c_1d( fftwsiz[ i ], fftwir[ i ], fftwoc[ i ], FFTW_ESTIMATE );


//single dimension complex to real fwd fft
fftwp_rvsr[ i ] = fftw_plan_dft_c2r_1d( fftwsiz[ i ], fftwic[ i ], fftwor[ i ], FFTW_ESTIMATE );
//---



i = 17;

if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 22\n" );
fftw_destroy_plan( fftwp_fwdc[ i ] );
fftw_destroy_plan( fftwp_rvsc[ i ] );

fftw_free( fftwic[ i ] );
fftw_free( fftwoc[ i ] );
if(vb)printf( "rtl_graph_wnd::fftw_adjust_plans() - step 23\n" );

fftwsiz[ i ] = iq_count;

printf( "rtl_graph_wnd::fftw_adjust_plans() - idx 17, fftwsiz[%d] %d\n", i, fftwsiz[ i ] );

//alloc fftw pointers
fftwic[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftwsiz[ i ] );
fftwoc[ i ] = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftwsiz[ i ] );

//single dimension complex to complex fwd fft
fftwp_fwdc[ i ] = fftw_plan_dft_1d( fftwsiz[ i ], fftwic[ i ], fftwoc[ i ], FFTW_FORWARD, FFTW_ESTIMATE );

//single dimension complex to complex rev fft
fftwp_rvsc[ i ] = fftw_plan_dft_1d( fftwsiz[ i ], fftwic[ i ], fftwoc[ i ], FFTW_BACKWARD, FFTW_ESTIMATE );


//single dimension real to complex fwd fft
fftwp_fwdr[ i ] = fftw_plan_dft_r2c_1d( fftwsiz[ i ], fftwir[ i ], fftwoc[ i ], FFTW_ESTIMATE );


//single dimension complex to real fwd fft
fftwp_rvsr[ i ] = fftw_plan_dft_c2r_1d( fftwsiz[ i ], fftwic[ i ], fftwor[ i ], FFTW_ESTIMATE );
//---


i_fftw_trig_plan_create_state = 1;

}








static float freq0 = 200;
static float freq1 = 400;
static float theta00 = 0;
static float theta01 = 0;
static float theta02 = 0;





int dbg0 = 0;


mystr tim2;

int demod_cnt = 0;
int demod_cnt_modulo0 = 50;






float theta_mux = 0;													//for offset tuning
static float theta_mux_inc;

static float theta_tune_play_file = 0;									//for file play tuning
static float theta_tune_play_file_inc;


vector <filter_code::st_cplex_tag> vcplex_sp0;
vector <filter_code::st_cplex_tag> vcplex_sp1;
vector <filter_code::st_cplex_tag> vcplex_sp2;
vector <filter_code::st_cplex_tag> vcplex_sp3;
vector <filter_code::st_cplex_tag> vcplex_sp4;
vector <filter_code::st_cplex_tag> vcplex_sp5;
vector <filter_code::st_cplex_tag> vcplex_sp6;
vector <filter_code::st_cplex_tag> vcplex_sp7;



float synth_theta00 = 0;
float synth_tone_theta1 = 0;
float synth_tone_theta2 = 0;
float synth_tone_theta3 = 0;
float synth_tone_theta4 = 0;
float synth_tone_theta5 = 0;
float synth_tone_theta6 = 0;

float synth_theta10 = 0;												//used for synth IQ voice carrier
float synth_theta20 = 0;												//used for synth IQ voice carrier
float synth_theta30 = 0;												//used for synth IQ voice carrier
float synth_theta40 = 0;												//used for synth IQ voice carrier
float synth_theta50 = 0;												//used for single sideband (Weaver's bandband (audio) quadrature modulator)
float synth_theta60 = 0;												//used for single sideband

double fract_ptr00 = 0;													//synth voice clip fractional ptr
double fract_ptr000 = 0;												//synth voice clip fractional ptr
double fract_ptr10 = 0;
double fract_ptr20 = 0;
double fract_ptr30 = 0;
double fract_ptr40 = 0;
double fract_ptr50 = 0;


int synth_iq_ptr0 = 0;



float min00 = 1e9;
float max00 = -1e9;


//int dbg_cnt1 = 0;
//int dbg_cnt2 = 0;
//int dbg_cnt4 = 0;


int64_t delta_wr_rd;
uint64_t uiwnd_low;
uint64_t uiwnd_high;
uint64_t uiwnd_low2;
uint64_t uiwnd_high2;

float inc_rate = 1.0f;
float inc_rate2 = 0.0f;











int zero_pad_fft_cnt = 0;


int zero_pad_state = 0;




/*


//at present this is for a debug 'fast_mgraph' graph
void zero_pad_fft()
{
bool vb = 0;

//return; //DO NOTHING FOR NOW


zero_pad_fft_cnt++;

if( !( zero_pad_fft_cnt%1 ) )
	{
	}
else{
	return;
	}



if( zero_pad_state == 1 )
	{
//	vector <filter_code::st_cplex_tag> viq_local_zero_pad_downsample;
	vector <filter_code::st_cplex_tag> vcplex0;
	vector <filter_code::st_cplex_tag> vcplex_downsample0;
	vector<st_spect_tag> vspect_displayble0;

	int siz = viq_local_zero_pad.size();
if(vb)printf( "zero_pad_fft() - viq_local_zero_pad.size()  %d\n", siz );



//int down_sample_factor_for_grph = viq_local_zero_pad.size() / 4096;

//low_pass_srconv( viq_local_zero_pad,  viq_local_zero_pad_downsample, down_sample_factor, filt_prev_idx10, now_r10, now_j10 );			//complex decimate (WFM uses a lower 'down_sample_factor')





	if( 1 )
		{
//		vector<float> vsmpl_r;
//		vector<float> vsmpl_i;
		 
//		for( int i = 0; i < viq_local_zero_pad.size(); i++ )
			{
//			vsmpl_r.push_back( viq_local_zero_pad[i].real );
//			vsmpl_i.push_back( viq_local_zero_pad[i].imag );
			}



//		filter_code::window_calc_and_apply_float( filter_code::fwt_kaiser, vsmpl_r, vwnd );
//		filter_code::window_calc_and_apply_float( filter_code::fwt_kaiser, vsmpl_r, vwnd );


		if(b_build_taper_window )
			{
filter_code::window_function_float( filter_code::fwt_hann, viq_local_zero_pad.size(), vtaper_window );

//			int cnt = viq_local_zero_pad.size();
//			for( int i = 0; i < cnt; i++ )
//				{
//				float f0 = 0.5f - (0.5f * cosf( (twopi * i) / (cnt - 1) ) );		//hann window test
//				vtaper_window.push_back( f0 );
//				}
			b_build_taper_window = 0;
			}

//pref_taper_windowing_for_fft = 0;

		if( pref_taper_windowing_for_fft )
			{
			//apply a window function to timedomain signal to reduce spectral leakage in fft conversion
			if( vtaper_window.size() == viq_local_zero_pad.size() )
				{
				for( int i = 0; i < vtaper_window.size(); i++ )
					{
					float f0 = vtaper_window[i];

					viq_local_zero_pad[i].real *= f0;
					viq_local_zero_pad[i].imag *= f0;
					}
				}
			else{
				b_build_taper_window = 1;
				printf( "zero_pad_fft() vtaper_window.size() != viq_local_zero_pad.size(): %d %d --- trigger a taper window rebuild\n", vtaper_window.size(), viq_local_zero_pad.size() );
				}
			}



//	printf( "zero_pad_fft() ====================== vtaper_window %d\n", vtaper_window.size() );

//		vgph50_y0.clear();

//		for( int i = 0; i < vwnd.size(); i++ )
//			{
//			vgph50_y0.push_back( vwnd[i] );
//			}

		}





	
//pref_zero_padding_for_fft = 1;

	if( pref_zero_padding_for_fft )
		{
		for( int i = 0; i < siz*(6-1); i++ )							//make SURE an fttw plan exists to hold the size of the zero padded signal
			{
			filter_code::st_cplex_tag oc;
			
			oc.real = 0;
			oc.imag = 0;
			viq_local_zero_pad.push_back( oc );
			}
		}

//printf( "zero_pad_fft() - padded size now: viq_local_zero_pad_downsample.size()  %d\n", (int)viq_local_zero_pad_downsample.size() );
if(vb)printf( "zero_pad_fft() - padded size now: viq_local_zero_pad_downsample.size()  %d\n", (int)viq_local_zero_pad.size() );

	if( i_fftw_trig_plan_create_state == 2 )							//plans created?
		{
		bool b_plan_exists = complex_fwd_fft_multi( viq_local_zero_pad, vcplex0, "for display with zero padding", en_ftid_gui );			//go to freq domain using an fft
		}
	else{
		return;
		}

if(vb)printf( "zero_pad_fft() vcplex0.size() %d\n", (int)vcplex0.size() );

int down_sample_factor_for_grph = vcplex0.size() / 4096;

//	filt_prev_idx3 = 0;
//	now_r3 = 0;
//	now_j3 = 0;

//	low_pass_srconv( vcplex0,  vcplex_downsample0, down_sample_factor_for_grph, filt_prev_idx3, now_r3, now_j3 );			//complex decimate


//	complex_fft_displayable( vcplex_downsample0, vspect_displayble0, 1 );
	complex_fft_displayable( vcplex0, vspect_displayble0, 1 );


	if(vb)printf( "zero_pad_fft() vspect_displayble0 %d\n", vspect_displayble0.size() );

//	int half = vcplex_downsample0.size() / 2;
	int half = vspect_displayble0.size() / 2;

	//vspect_displayble[ half + 100 ].ampl = 10.0f;

	float disp_spect_zoom_factor0 = 1.0f;

	if( disp_spect_zoom_factor != 0.0f )
		{
//		int zoom_cnt = vcplex_downsample0.size() / disp_spect_zoom_factor0;
		int zoom_cnt = vspect_displayble0.size() / disp_spect_zoom_factor0;

		int half2 = zoom_cnt / 2;
		
		int start = half - half2; 
		int end = half + half2; 
		
	//	vector<st_spect_tag> vspect_disp_actual;


		vgph50_y0.clear();


		for( int i = start; i < end; i++ )
			{
			st_spect_tag o;
			
			o = vspect_displayble0[i];

//			vspect_disp_actual.push_back( o );
			
			vgph50_y0.push_back( o.ampl );
			}



//		for( int i = 0; i < viq_local_zero_pad.size(); i++ )
//			{
//			filter_code::st_cplex_tag o;
			
//			o = viq_local_zero_pad[i];

	//		vspect_disp_actual.push_back( o );
			
//			vgph50_y0.push_back( sqrtf( o.real*o.real + o.imag*o.imag ) );
//			vgph50_y0.push_back( o.real );
			}


	//	vspect_displayble0 = vspect_disp_actual;
		}






	int gph_idx = 0;						//only one graph that has multiple traces
	gph5.position( 10, 100 );
	gph5.font_size( 9 );
	gph5.set_sig_dig( 2 );
	gph5.sample_rect_hints_distancex = 0;
	gph5.sample_rect_hints_distancey = 0;

	gph5.shift_y( gph_idx, 0, 0.0f, 0.0f, 0.0f);

	gph5.plot_vfloat( 0, vgph50_y0 );
//	gph5.fit_plot( 0 );

	//gph5.user_marker_add( 0, vgph1_vuser_marker );							//add freq details, call 'user_marker_idx_add()'  and lastly 'user_marker_show()' to trig a redraw
	//gph5.user_marker_idx_add( 0, vgph1_user_marker_idx );					//this must be as big as 'st_pnt', each plot point must have an index to 'vgph1_vuser_marker' for correct display, call 'user_marker_show()' lastly to trig a redraw
	//gph5.user_marker_show( 0, 1 );											//make visible, this finally calls 'plot_grph_internal()'


	zero_pad_state = 0;
	}
else{
//		bool b_plan_exists = complex_fwd_fft_multi( viq_local, vcplex_sp, "for display" );			//go to freq domain using an fft
	}
	
//	b_viq2_loaded = 0;									//flg that rtl callback can reload

	
//down_sample_factor_for_graph = vcplex_sp.size() / 4096;

//	low_pass_srconv( vcplex_sp,  vcplex_sp_downsample, down_sample_factor_for_graph, filt_prev_idx2, now_r2, now_j2 );			//complex decimate

}

*/













/*
//used a different method

void create_lsb_usb_iq()
{
printf("create_lsb_usb_iq() - srate %d  g_dev_bw %d\n", int (srate), int (g_dev_bw));

vsynth_iq0.clear();

float time_per_sample = 1.0f/srate;

float freq_carrier0 = 10.06e6;
float theta_carrier0 = 0;
float theta_carrier0_inc = (freq_carrier0) * twopi * (1.0f/g_dev_bw);


float freq_tone0 = 300;
float theta_tone0 = 0;
float theta_tone0_inc = (freq_tone0) * twopi * time_per_sample;


float freq_tone1 = 3500;
float theta_tone1 = 0;
float theta_tone1_inc = (freq_tone1) * twopi * time_per_sample;




for( int i = 0; i < 5*g_dev_bw; i++ )
	{
	float f1 = 0.75;//0.2f*cosf( freq_tone0 );
	float f2 = 0;//0.1f*cosf( freq_tone1 );

	float I0 = (f1 + f2 ) * cosf( theta_carrier0 );
	float Q0 = (f1 + f2 ) * sinf( theta_carrier0 );
	
	theta_carrier0 += theta_carrier0_inc;
	if( theta_carrier0 >= twopi ) theta_carrier0 -= twopi;
	if( theta_carrier0 <= -twopi ) theta_carrier0 += twopi;

	if( theta_carrier0 >= 2*twopi )  theta_carrier0 = 0;				//handle a large theta, possibly due to high freq usage
	if( theta_carrier0 <= -2*twopi )  theta_carrier0 = 0;


	theta_tone0 += theta_tone0_inc;
	if( theta_tone0 >= twopi ) theta_tone0 -= twopi;
	if( theta_tone0 <= -twopi ) theta_tone0 += twopi;

	theta_tone1 += theta_tone1_inc;
	if( theta_tone1 >= twopi ) theta_tone1 -= twopi;
	if( theta_tone1 <= -twopi ) theta_tone1 += twopi;

//	float f10 = f0;
	
	filter_code::st_cplex_tag oc;
	
	oc.real = I0;
	oc.real = Q0;
	
	vsynth_iq0.push_back( oc );
	}
//getchar();

return 1;
}








*/









//assuming file coeffs are for srate of 24KHz
bool create_hilbert_fir_filters()
{
string s1;

s1 = cns_hilbert_plus45_127taps_24KHz;
//s1 = cns_hilbert_plus45_32taps_24KHz;

fir_hilbert_45_plus.created = 0;
if( !filter_code::create_filter_from_file( fir_hilbert_45_plus, s1 ) )
	{
	printf("create_hilbert_fir_filters() - failed to load fir from file '%s'\n", s1.c_str() );
	return 0;
	}
printf("create_hilbert_fir_filters() - %d fir coeffs loaded from file '%s'\n", fir_hilbert_45_plus.coeff_cnt, s1.c_str() );



s1 = cns_hilbert_minus45_127taps_24KHz;
//s1 = cns_hilbert_minus45_32taps_24KHz;

fir_hilbert_45_minus.created = 0;
if( !filter_code::create_filter_from_file( fir_hilbert_45_minus, s1 ) )
	{
	printf("create_hilbert_fir_filters() - failed to load fir from file '%s'\n", s1.c_str() );
	return 0;
	}
printf("create_hilbert_fir_filters() - %d fir coeffs loaded from file '%s'\n", fir_hilbert_45_minus.coeff_cnt, s1.c_str() );

//getchar();

return 1;
}























































//IF YOU ADD to this also modify 'en_demodulator_type_tag'  ALSO MOD 'sz_demodulator_type' MOD ALSO 'pulldown_demod_mode'
int demod_type_idx_from_str( string ss )
{

if( ss.compare( sz_demodulator_type[0] ) == 0 ) return 0;
if( ss.compare( sz_demodulator_type[1] ) == 0 ) return 1;
if( ss.compare( sz_demodulator_type[2] ) == 0 ) return 2;
if( ss.compare( sz_demodulator_type[3] ) == 0 ) return 3;
if( ss.compare( sz_demodulator_type[4] ) == 0 ) return 4;
//if( ss.compare( sz_demodulator_type[5] ) == 0 ) return 5;
//if( ss.compare( sz_demodulator_type[6] ) == 0 ) return 6;

return -1;
}




void rtl_graph_wnd::favourite_add( bool bstore_in_first_empty_slot )
{
string s1;
st_favourite_freq_tag o;


o.sname = fi_name->value();

if( o.sname.length() == 0 )
	{
	strpf( s1, "No Name is entered, require favourite to be named." );
	fl_alert( s1.c_str(), 0 );
	return;
	}

o.sgroup = fi_group_name->value();
o.scomment0 ="comment";
o.sworld_coord ="40° 0' 0''  22° 0' 0''";

o.sdev_name = s_dev_name;
o.sdev_manufacturer = s_dev_manufact;
o.sdev_prod = s_dev_product;
o.sdev_serial = s_dev_serial;

o.demod_type = wnd_rtl_graph->demodul_mode;

o.freq = g_freq_tune + g_freq_sub_tune;
o.freq_center = g_freq_tune;
o.b_use_iffreq = g_b_if_freq;

o.b_dwn_aa = g_b_dwn_aa;
o.i_dwn_srate = downsample_srate;

o.b_bw_bpass = g_b_bw_bpass;

o.if_freq = g_interfreq;

o.b_use_iffreq = g_b_if_freq;
o.iffreq_bw_low = g_bw_lower;
o.iffreq_bw_high = g_bw_upper;
o.iffreq_bw_taps = g_bw_taps;

o.b_agc = b_agc;
o.audio_gain = g_gain_aud;

o.dev_gain = g_dev_gain;
o.dev_direct_sampling = g_dev_direct_sampling;
o.dev_bias_t = g_dev_bias_t;
o.iq_gain = g_gain_iq;

o.deemph = i_deemphasis;

tm tt;
string s2, s3, s4, s5, s6, s7;
mystr m1;


m1.get_time_now( tt );
m1.make_time_str( tt, s1, s2, s3, s4 );

o.stime = s4;

//void make_date_str( struct tm tn, string &dow, string &dom, string &mon_num, string &mon_name, string &year, string &year_short );

m1.make_date_str( tt, s1, s2, s3, s4, s5, s6 );
strpf( s7, "%s-%s-%s", s5.c_str(), s3.c_str(), s2.c_str() );

o.sdate = s7;




if( bstore_in_first_empty_slot == 0 ) 
	{
	wnd_fav->store_to_sel_fav( o );
	}
else{
	int idx = wnd_fav->store_to_free_slot( o );
	
	if( idx != -1 )
		{
		strpf( s1, "Saved favourite into slot: %d", idx );
		fl_alert( s1.c_str(), 0 );
		}
	else{
		strpf( s1, "No avail slot to save into. To remove unused favs, clear their Name" );
		fl_alert( s1.c_str(), 0 );
		}
	}
}









bool rtl_graph_wnd::load_favourite_list( string fname )
{
bool vb = 0;

string s1, s2, st;
mystr m1, m2;

printf( "rtl_graph_wnd::load_favourite_list() '%s'\n", fname.c_str() );
 
GCProfile p( fname );

string ssect, skey, sparam, sequ;

ssect = "List";

vector<st_favourite_freq_tag> vv;

bool bgot_one = 0;

for( int i = 0; i < cn_favourite_row_max; i++ )
	{
	st_favourite_freq_tag o;

	strpf( skey, "fav%03d", i );

	p.GetPrivateProfileStr( ssect, skey, "", &sparam );


//	if(vb)printf( "rtl_graph_wnd::load_favourite_list() - skey '%s'\n", skey.c_str() );

	if( sparam.length() == 0 ) continue;

	bgot_one = 1;

	if(vb)printf( "rtl_graph_wnd::load_favourite_list() - sparam '%s'\n", sparam.c_str() );
	
	m1 = sparam;
	float fv;
	int iv;
	unsigned int ui;

	if( m1.ExtractParamVal_with_delimit( "freq=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );
		o.freq = iv;
		if(vb)printf( "o.freq '%s'  %d\n", sequ.c_str(), iv );
		}


	if( m1.ExtractParamVal_with_delimit( "freq_center=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );
		o.freq_center = iv;
		if(vb)printf( "o.freq_center '%s'  %d\n", sequ.c_str(), iv );
		}

	if( m1.ExtractParamVal_with_delimit( "demod_type=", ",", sequ ) )
		{
		m2 = sequ;
		m2.EscToStr();
		int idx = demod_type_idx_from_str( m2.szptr() );
		if( idx < 0 )
			{
			printf( "Can't find matching demo_type '%s'\n", m2.szptr() );	
			}
		else{
			o.demod_type = idx;
			if(vb)printf( "o.demod_type '%s' idx %d\n", m2.szptr(), o.demod_type );
			}
		}

	if( m1.ExtractParamVal_with_delimit( "sname=", ",", sequ ) )
		{
		m2 = sequ;
		m2.EscToStr();
		o.sname = m2.szptr();
		if(vb)printf( "o.sname '%s'\n", o.sname.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "sgroup=", ",", sequ ) )
		{
		m2 = sequ;
		m2.EscToStr();
		o.sgroup = m2.szptr();
		if(vb)printf( "o.sgroup '%s'\n", o.sgroup.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "sname_long=", ",", sequ ) )
		{
		m2 = sequ;
		m2.EscToStr();
		o.sname_long = m2.szptr();
		if(vb)printf( "o.sname_long '%s'\n", o.sname_long.c_str() );
		}

/*
	if( m1.ExtractParamVal_with_delimit( "scomment=", ",", sequ ) )
		{
		m2 = sequ;
		m2.EscToStr();
		o.scomment = m2.szptr();
		if(vb)printf( "o.scomment '%s'\n", o.scomment.c_str() );
		}
*/

	if( m1.ExtractParamVal_with_delimit( "sworld_coord=", ",", sequ ) )
		{
		m2 = sequ;
		m2.EscToStr();
		o.sworld_coord = m2.szptr();
		if(vb)printf( "o.sworld_coord '%s'\n", o.sworld_coord.c_str() );
		}



	if( m1.ExtractParamVal_with_delimit( "dev_name=", ",", sequ ) )
		{
		m2 = sequ;
		m2.EscToStr();
		o.sdev_name = m2.szptr();
		if(vb)printf( "o.dev_name '%s'\n", o.sdev_name.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "dev_manufacturer=", ",", sequ ) )
		{
		m2 = sequ;
		m2.EscToStr();
		o.sdev_manufacturer = m2.szptr();
		if(vb)printf( "o.dev_manufacturer '%s'\n", o.sdev_manufacturer.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "scomment0=", ",", sequ ) )
		{
		m2 = sequ;
		m2.EscToStr();
		o.scomment0 = m2.szptr();
		if(vb)printf( "o.scomment0 '%s'\n", o.scomment0.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "dev_prod=", ",", sequ ) )
		{
		m2 = sequ;
		m2.EscToStr();
		o.sdev_prod = m2.szptr();
		if(vb)printf( "o.dev_prod '%s'\n", o.sdev_prod.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "dev_serial=", ",", sequ ) )
		{
		m2 = sequ;
		m2.EscToStr();
		o.sdev_serial = m2.szptr();
		if(vb)printf( "o.dev_serial '%s'\n", o.sdev_serial.c_str() );
		}
	




	if( m1.ExtractParamVal_with_delimit( "b_use_iffreq=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );
		o.b_use_iffreq = iv;
		if(vb)printf( "o.b_use_iffreq '%s'  %d\n", sequ.c_str(), iv );
		}


	if( m1.ExtractParamVal_with_delimit( "if_freq=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );
		o.if_freq = iv;
		if(vb)printf( "o.if_freq '%s'  %d\n", sequ.c_str(), iv );
		}
		
	if( m1.ExtractParamVal_with_delimit( "b_bw_bpass=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );
		o.b_bw_bpass = iv;
		if(vb)printf( "o.b_bw_bpass '%s'  %d\n", sequ.c_str(), iv );
		}

	if( m1.ExtractParamVal_with_delimit( "iffreq_bw_low=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );
		o.iffreq_bw_low = iv;
		if(vb)printf( "o.iffreq_bw_low '%s'  %d\n", sequ.c_str(), iv );
		}

	if( m1.ExtractParamVal_with_delimit( "iffreq_bw_high=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );
		o.iffreq_bw_high = iv;
		if(vb)printf( "o.iffreq_bw_high '%s'  %d\n", sequ.c_str(), iv );
		}

	if( m1.ExtractParamVal_with_delimit( "dev_srate=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );
		o.dev_srate = iv;
		if(vb)printf( "o.dev_srate '%s'  %d\n", sequ.c_str(), iv );
		}


	if( m1.ExtractParamVal_with_delimit( "dev_gain=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%f" , &fv );
		o.dev_gain = fv;
		if(vb)printf( "o.dev_gain '%s'  %f\n", sequ.c_str(), fv );
		}


	if( m1.ExtractParamVal_with_delimit( "dev_ppm=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );
		o.dev_ppm = iv;
		if(vb)printf( "o.dev_ppm '%s'  %d\n", sequ.c_str(), iv );
		}


	if( m1.ExtractParamVal_with_delimit( "audio_gain=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%f" , &fv );
		o.audio_gain = fv;
		if(vb)printf( "o.audio_gain '%s'  %f\n", sequ.c_str(), fv );
		}



	if( m1.ExtractParamVal_with_delimit( "sdate=", ",", sequ ) )
		{
		m2 = sequ;
		m2.EscToStr();
		o.sdate = m2.szptr();
		if(vb)printf( "o.sdate '%s'\n", o.sdate.c_str() );
		}


	if( m1.ExtractParamVal_with_delimit( "stime=", ",", sequ ) )
		{
		m2 = sequ;
		m2.EscToStr();
		o.stime = m2.szptr();
		if(vb)printf( "o.stime '%s'\n", o.stime.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "dev_bias_t=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );
		o.dev_bias_t = iv;
		if(vb)printf( "o.dev_bias_t '%s'  %d\n", sequ.c_str(), iv );
		}

	if( m1.ExtractParamVal_with_delimit( "dev_direct_sampling=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );
		o.dev_direct_sampling = iv;
		if(vb)printf( "o.dev_direct_sampling '%s'  %d\n", sequ.c_str(), iv );
		}


	if( m1.ExtractParamVal_with_delimit( "iq_gain=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%f" , &fv );
		o.iq_gain = fv;
		if(vb)printf( "o.dev_gain '%s'  %f\n", sequ.c_str(), fv );
		}


	if( m1.ExtractParamVal_with_delimit( "b_dwn_aa=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );
		o.b_dwn_aa = iv;
		if(vb)printf( "o.b_dwn_aa '%s'  %d\n", sequ.c_str(), iv );
		}


	if( m1.ExtractParamVal_with_delimit( "dwn_srate=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );
		o.i_dwn_srate = iv;
		if(vb)printf( "o.i_dwn_srate '%s'  %d\n", sequ.c_str(), iv );
		}


	if( m1.ExtractParamVal_with_delimit( "iffreq_bw_taps=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );
		o.iffreq_bw_taps = iv;
		if(vb)printf( "o.iffreq_bw_taps '%s'  %d\n", sequ.c_str(), iv );
		}


	if( m1.ExtractParamVal_with_delimit( "b_agc=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );
		o.b_agc = iv;
		if(vb)printf( "o.b_agc '%s'  %d\n", sequ.c_str(), iv );
		}


	if( m1.ExtractParamVal_with_delimit( "deemph=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );
		o.deemph = iv;
		if(vb)printf( "o.deemph '%s'  %d\n", sequ.c_str(), iv );
		}

	vv.push_back( o );
	}


vfav = vv;
if( bgot_one) wnd_fav->set_via_vector( vv, sz_demodulator_type, b_sanitise_favourites);

return 1;
}








bool rtl_graph_wnd::save_favourite_list( string fname )
{
string s1, s2, st;
mystr m0, m1, m2, m3, m4, m5, m6;

//GCProfile p( fname );

st = "[List]\n";

for( int i = 0; i < vfav.size(); i++ )
	{
	st_favourite_freq_tag o = vfav[i];

	m1 = o.sgroup;
	m1.StrToEscMostCommon1();

	m2 = o.sname;
	m2.StrToEscMostCommon1();
	
	m3 = o.sname_long;
	m3.StrToEscMostCommon1();

//	m4 = o.scomment;
//	m4.StrToEscMostCommon1();

	m4 = o.scomment0;
	m4.StrToEscMostCommon1();

	m5 = o.sworld_coord;
	m5.StrToEscMostCommon1();

	strpf( s1, "fav%03d=freq=%d,freq_center=%d,demod_type=%s,sgroup=%s,sname=%s,sname_long=%s,scomment0=%s,sworld_coord=%s",\
	i, o.freq, o.freq_center, sz_demodulator_type[o.demod_type], m1.szptr(), m2.szptr(), m3.szptr(), m4.szptr(), m5.szptr() );
	st += s1;



	m1 = o.sdev_name;
	m1.StrToEscMostCommon1();

	m2 = o.sdev_manufacturer;
	m2.StrToEscMostCommon1();
	
	m3 = o.sdev_prod;
	m3.StrToEscMostCommon1();

	m4 = o.sdev_serial;
	m4.StrToEscMostCommon1();

	strpf( s1, ",dev_name=%s,dev_manufacturer=%s,dev_prod=%s,dev_serial=%s",\
	m1.szptr(), m2.szptr(), m3.szptr(), m4.szptr() );
	st += s1;



	strpf( s1, ",b_use_iffreq=%d,if_freq=%d,b_bw_bpass=%d,iffreq_bw_low=%d,iffreq_bw_high=%d,dev_srate=%d,dev_gain=%f,dev_ppm=%d,audio_gain=%f",\
	o.b_use_iffreq, o.if_freq, o.b_bw_bpass, o.iffreq_bw_low, o.iffreq_bw_high, o.dev_srate, o.dev_gain, o.dev_ppm, o.audio_gain );
	st += s1;

	m1 = o.sdate;
	m1.StrToEscMostCommon1();

	m2 = o.stime;
	m2.StrToEscMostCommon1();

	strpf( s1, ",tuned_count=%d,sdate=%s,stime=%s,dev_bias_t=%d,dev_direct_sampling=%d,iq_gain=%f",\
	o.tuned_count, m1.szptr(), m2.szptr(), o.dev_bias_t, o.dev_direct_sampling, o.iq_gain );
	st += s1;

	strpf( s1, ",b_dwn_aa=%d,i_dwn_srate=%d,iffreq_bw_taps=%d,b_agc=%d,deemph=%d",\
	o.b_dwn_aa, o.i_dwn_srate, o.iffreq_bw_taps, o.b_agc, o.deemph );
	st += s1;


	st += '\n';
	}


m1 = st;
if( !m1.writefile( fname ) ) return 0;


return 1;
}







void rtl_graph_wnd::load_settings( string ini_fname, bool b_load_last_favourite_settings )
{
bool vb = 0;
string s1, s2;

GCProfile p( ini_fname );

int x = p.GetPrivateProfileLONG( "Settings", "GraphWinX", 100 );
int y = p.GetPrivateProfileLONG( "Settings", "GraphWinY", 100 );
int cx = p.GetPrivateProfileLONG( "Settings", "GraphWinCX", 750 );
int cy = p.GetPrivateProfileLONG( "Settings", "GraphWinCY", 550 );

position( x , y );	
//size( cx , cy );	


x = p.GetPrivateProfileLONG( "Settings", "wnd_fav_x", 100 );
y = p.GetPrivateProfileLONG( "Settings", "wnd_fav_y", 100 );
cx = p.GetPrivateProfileLONG( "Settings", "wnd_fav_cx", 1000 );
cy = p.GetPrivateProfileLONG( "Settings", "wnd_fav_cy", 500 );

wnd_fav->resize( x, y, cx, cy );



p.GetPrivateProfileStr( "Settings", "slast_filename_ini", "rtl_lnx.ini", &slast_filename_ini );


p.GetPrivateProfileStr( "Settings", "NetAnalyserFreqStep", "5000", &s1 );
fi_freq_step->value( s1.c_str() );

p.GetPrivateProfileStr( "Settings", "dev_gain", "30", &s1 );
miw_freq_gain->set_value_from_str( s1 );


p.GetPrivateProfileStr( "Settings", "gain_iq", "1", &s1 );
miw_gain_iq->set_value_from_str( s1 );

p.GetPrivateProfileStr( "Settings", "NetAnalyserThreshold", "-15", &s1 );
fi_freq_threshold->value( s1.c_str() );

//p.GetPrivateProfileStr( "Settings", "FreqBandwidth", "2400000", &s1 );
//fi_freq_bandwidth->value( s1.c_str() );

p.GetPrivateProfileStr( "Settings", "miw_dev_bw", "2400000", &s1 );
miw_dev_bwidth->set_value_from_str( s1 );

p.GetPrivateProfileStr( "Settings", "FreqReso", "512", &s1 );
fi_freq_resolution->value( s1.c_str() );

//p.GetPrivateProfileStr( "Settings", "BwLower", "512", &s1 );
//fi_bw_lower->value( s1.c_str() );

p.GetPrivateProfileStr( "Settings", "BwLower", "80", &s1 );
miw_filt_lwr->set_value_from_str( s1.c_str() );

p.GetPrivateProfileStr( "Settings", "BwUpper", "2500", &s1 );
miw_filt_upr->set_value_from_str( s1.c_str() );

p.GetPrivateProfileStr( "Settings", "miw_filt_taps", "100", &s1 );
miw_filt_taps->set_value_from_str( s1.c_str() );

//int ii = p.GetPrivateProfileLONG( "Settings", "InterFreq", 0 );
//miw_if_freq->set_value_from_double( ii );

int ii = p.GetPrivateProfileLONG( "Settings", "ck_if_freq", 1 );
ck_if_freq->value( ii );

ii = p.GetPrivateProfileLONG( "Settings", "iffreq", 0 );
miw_if_freq->set_value_from_double( ii );


int state = p.GetPrivateProfileLONG( "Settings", "BWLimit", 0 );
ck_bw_limit->value( state );


//p.GetPrivateProfileStr( "Settings", "FreqMouse", "512", &s1 );
//fi_freq_mouse->value( s1.c_str() );

int d;
d = p.GetPrivateProfileLONG( "Settings", "FreqDemodMode", 0 );
fi_demodul_mode->value( d );
demodul_mode = d;


double dd = p.GetPrivateProfileDOUBLE( "Settings", "g_gain_aud", 2.0 );
fvs_gain->value( dd );
g_gain_aud = dd;



//dd = p.GetPrivateProfileDOUBLE( "Settings", "Squelch", 0.0 );
//fvs_squelch->value( dd );
//g_squelch = dd;




for( int i = 0; i < cn_max_tune_history; i++ )
	{
	strpf( s1 ,"TuneHist_%02d", i );
	p.GetPrivateProfileStr( "Settings", s1.c_str(), "" , &s2 );
	vtunehist.push_back( s2 );
	}

//sort_vstring( vtunehist );

//for( int i = 0; i < vtunehist.size(); i++ )
//	{
//	fi_tune->add( vtunehist[ i ].c_str() );
//	}


p.GetPrivateProfileStr( "Settings", "LastTuneFreq", "929000000", &s1 );
//fi_tune->value( s1.c_str() );
miwp_tune->miw->set_value_from_str( s1 );
idb_tune->value( s1 );





mystr m1, m2;

for( int i = 0; i < cn_max_scan_history; i++ )
	{
	strpf( s1 ,"ScanHist_%02d", i );
	p.GetPrivateProfileStr( "Settings", s1.c_str(), "88000000,108000000" , &s2 );
	
//	s2 = "8800000,10800000";
	
	vector<string>vstr;
	m1 = s2;
	m1.LoadVectorStrings( vstr, ',' );


	if( vstr.size() > 1 )
		{
		st_scan_freq_tag sf1;
		
		
		sf1.start = vstr[ 0 ];


		sf1.stop = vstr[ 1 ];

		vscanhist.push_back( sf1 );
		}
	
	}

sort_st_scan_freq_tag( vscanhist );



for( int i = 0; i < cn_max_tune_history; i++ )
	{
	strpf( s1,"tunehist_%02d", i);
	p.GetPrivateProfileStr( "Settings", s1.c_str(), "", &s2 );

	vtunehist2.push_back( s2 );
	}
update_tune_history();


last_tune_history_value = 0;
last_scan_history_start = 0;
last_scan_history_stop = 0;

reload_scanhist();

fi_freq_start->value( last_scan_history_start );		//set these to something, so a keyin does not trigger a callback
fi_freq_stop->value( last_scan_history_stop );


p.GetPrivateProfileStr( "Settings", "NetAnalyserFreqStart", "88000000", &s1 );
fi_freq_start->value( s1.c_str() );

p.GetPrivateProfileStr( "Settings", "NetAnalyserFreqStop", "108000000", &s1 );
fi_freq_stop->value( s1.c_str() );


gph_loc_sel = p.GetPrivateProfileLONG( "Settings", "gph_loc_sel", "0" );
ch_graph_loc_sel->value( gph_loc_sel );

		gph_loc_sel_tmp = gph_loc_sel;
		gph_loc_sel = 1;


g_tuning_offset = p.GetPrivateProfileLONG( "Settings", "g_tuning_offset", 0 );
miw_tuning_offset->set_value_from_double( g_tuning_offset );


g_freq_sub_tune = p.GetPrivateProfileLONG( "Settings", "g_freq_sub_tune", 0 );
miw_freq_sub_tune->set_value_from_double( g_freq_sub_tune );



g_carrier_max_tune_threshold = p.GetPrivateProfileDOUBLE( "Settings", "g_carrier_max_tune_threshold", 0.5 );
miw_carrier_max_tune_threshold->set_value_from_double( g_carrier_max_tune_threshold );




p.GetPrivateProfileStr( "Settings", "slast_favourite_fname", "", &slast_favourite_fname );

if( b_load_last_favourite_settings) load_favourite_list( slast_favourite_fname );



g_dev_direct_sampling = p.GetPrivateProfileLONG( "Settings", "g_dev_direct_sampling", 0 );
ld_direct_sampling->ChangeCol( g_dev_direct_sampling );


g_dev_bias_t = p.GetPrivateProfileLONG( "Settings", "g_dev_bias_t", 0 );
set_bias_t( g_dev_bias_t );
//ld_bias_t->ChangeCol( g_dev_bias_t );


for( int i = 0; i < cn_freq_memory_max; i++ )
	{
	strpf( s1, "freq_memory%02d", i );
	p.GetPrivateProfileStr( "Settings", s1.c_str(), "100e6,0", &s1 );

	int i0, i1;
	sscanf( s1.c_str(), "%d,%d\n", &i0, &i1 );
	freq_memory[i].freq_tune = i0;
	freq_memory[i].freq_sub_tune = i1;
	}



//-------
for( int i = 0; i < cn_preset_memory_max; i++ )
	{
	strpf( s1, "preset_memory%02d", i );
	p.GetPrivateProfileStr( "Settings", s1.c_str(), "", &s2 );

	if(vb)printf( "load_settings() - %d: %s\n", i, s2.c_str() );

	m1 = s2;
	string sequ;
	int iv;
	float fv;
	if( m1.ExtractParamVal_with_delimit( "sname=", ",", sequ ) )
		{
//		sscanf( sequ.c_str(), "%d" , &iv );
//		og.enabled = iv;

		m2 = sequ;
		m2.EscToStr();
		preset_memory[i].sname  = m2.szptr();
		if(vb)printf( "got '%s'\n", m2.szptr() );
		}

	if( m1.ExtractParamVal_with_delimit( "freq_tune=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory[i].freq_tune = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "freq_sub_tune=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory[i].freq_sub_tune = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "demod_type=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory[i].demod_type = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}


	if( m1.ExtractParamVal_with_delimit( "b_dwn_aa=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory[i].b_dwn_aa = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}


	if( m1.ExtractParamVal_with_delimit( "i_dwn_srate=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory[i].i_dwn_srate = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "b_use_iffreq=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory[i].b_use_iffreq = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "if_freq=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory[i].if_freq = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "b_bw_limit=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory[i].b_bw_limit = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "iffreq_bw_low=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory[i].iffreq_bw_low = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "iffreq_bw_high=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory[i].iffreq_bw_high = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "iffreq_bw_taps=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory[i].iffreq_bw_taps = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "dev_gain=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%f" , &fv );

		preset_memory[i].dev_gain = fv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "direct_sampling=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory[i].direct_sampling = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "iq_gain=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%f" , &fv );

		preset_memory[i].iq_gain = fv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "aud_gain=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%f" , &fv );

		preset_memory[i].aud_gain = fv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "b_agc=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory[i].b_agc = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}


	if( m1.ExtractParamVal_with_delimit( "b_bias_t=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory[i].b_bias_t = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}


	if( m1.ExtractParamVal_with_delimit( "i_deemph=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory[i].i_deemph = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}


	if( m1.ExtractParamVal_with_delimit( "b_dcblk=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory[i].b_dcblk = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	}
//-------



//-------

for( int i = 0; i < cn_preset_memory_max2; i++ )
	{
	strpf( s1, "preset_memory2_%02d", i );
	p.GetPrivateProfileStr( "Settings", s1.c_str(), "", &s2 );

	if(vb)printf( "load_settings() - %d: %s\n", i, s2.c_str() );

	m1 = s2;
	string sequ;
	int iv;
	float fv;
	if( m1.ExtractParamVal_with_delimit( "sname=", ",", sequ ) )
		{
//		sscanf( sequ.c_str(), "%d" , &iv );
//		og.enabled = iv;

		m2 = sequ;
		m2.EscToStr();
		preset_memory2[i].sname  = m2.szptr();
		if(vb)printf( "got '%s'\n", m2.szptr() );
		}

	if( m1.ExtractParamVal_with_delimit( "freq_tune=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory2[i].freq_tune = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "freq_sub_tune=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory2[i].freq_sub_tune = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "demod_type=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory2[i].demod_type = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "b_dwn_aa=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory2[i].b_dwn_aa = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "i_dwn_srate=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory2[i].i_dwn_srate = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "b_use_iffreq=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory2[i].b_use_iffreq = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "if_freq=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory2[i].if_freq = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "b_bw_limit=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory2[i].b_bw_limit = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "iffreq_bw_low=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory2[i].iffreq_bw_low = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "iffreq_bw_high=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory2[i].iffreq_bw_high = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "iffreq_bw_taps=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory2[i].iffreq_bw_taps = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "dev_gain=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%f" , &fv );

		preset_memory2[i].dev_gain = fv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "direct_sampling=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory2[i].direct_sampling = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "iq_gain=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%f" , &fv );

		preset_memory2[i].iq_gain = fv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "aud_gain=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%f" , &fv );

		preset_memory2[i].aud_gain = fv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}

	if( m1.ExtractParamVal_with_delimit( "b_agc=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory2[i].b_agc = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}


	if( m1.ExtractParamVal_with_delimit( "b_bias_t=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory2[i].b_bias_t = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}


	if( m1.ExtractParamVal_with_delimit( "i_deemph=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory2[i].i_deemph = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}


	if( m1.ExtractParamVal_with_delimit( "b_dcblk=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		preset_memory2[i].b_dcblk = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}
	}
//-------



led_preset_memory_tooltip_update( b_led_preset_memory_tooltip_show_tooltips );
led_preset_memory_label_update2();




//-------
for( int i = 0; i < cn_filter_memory_max; i++ )
	{
	strpf( s1, "filter_memory_%02d", i );
	p.GetPrivateProfileStr( "Settings", s1.c_str(), "", &s2 );

	if(vb)printf( "load_settings() - %d: %s\n", i, s2.c_str() );

	m1 = s2;
	string sequ;
	int iv;
	float fv;

	if( m1.ExtractParamVal_with_delimit( "sname=", ",", sequ ) )
		{
		m2 = sequ;
		m2.EscToStr();
		filter_memory[i].sname  = m2.szptr();
		if(vb)printf( "got '%s'\n", m2.szptr() );
		}


	if( m1.ExtractParamVal_with_delimit( "b_bw_limit=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		filter_memory[i].b_bw_limit = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}


	if( m1.ExtractParamVal_with_delimit( "iffreq_bw_low=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		filter_memory[i].iffreq_bw_low = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}


	if( m1.ExtractParamVal_with_delimit( "iffreq_bw_high=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		filter_memory[i].iffreq_bw_high = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}


	if( m1.ExtractParamVal_with_delimit( "iffreq_bw_taps=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		filter_memory[i].iffreq_bw_taps = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}
	}
//-------


//-------
for( int i = 0; i < cn_dwn_srate_memory_max; i++ )
	{
	strpf( s1, "dwn_srate_memory%02d", i );
	p.GetPrivateProfileStr( "Settings", s1.c_str(), "", &s2 );

	if(vb)printf( "load_settings() - %d: %s\n", i, s2.c_str() );

	m1 = s2;
	string sequ;
	int iv;
	float fv;

	if( m1.ExtractParamVal_with_delimit( "sname=", ",", sequ ) )
		{
		m2 = sequ;
		m2.EscToStr();
		dwn_srate_memory[i].sname  = m2.szptr();
		if(vb)printf( "got '%s'\n", m2.szptr() );
		}


	if( m1.ExtractParamVal_with_delimit( "dwn_srate=", ",", sequ ) )
		{
		sscanf( sequ.c_str(), "%d" , &iv );

		dwn_srate_memory[i].dwn_srate = iv;
		if(vb)printf( "got '%s'\n", sequ.c_str() );
		}
	}

//-------











b_wnd_fav_is_open = p.GetPrivateProfileLONG( "Settings", "b_wnd_fav_is_open", 1 );
if( wnd_fav ) if( b_wnd_fav_is_open ) wnd_fav->show();



b_clip_enable = p.GetPrivateProfileLONG( "Settings", "b_clip_enable", 1 );
ld_clip->ChangeCol( b_clip_enable );

float ff = p.GetPrivateProfileDOUBLE( "Settings", "clip_level_audio", 0.180f );
ff = fabsf( ff );
if( ff < cn_clip_audio_limit_low ) ff = cn_clip_audio_limit_low;
if( ff > cn_clip_audio_limit_high  ) ff = cn_clip_audio_limit_high;
clip_level_audio = ff;

get_user_gui_control_params();




ispec_avg_wnd = p.GetPrivateProfileLONG( "Settings", "ispec_avg_wnd", 0 );

miw_spect_avg->set_value_from_double( ispec_avg_wnd );
spect_avg( ispec_avg_wnd );

disp_spect_zoom_factor = p.GetPrivateProfileDOUBLE( "Settings", "disp_spect_zoom_factor", 1.0 );
if( disp_spect_zoom_factor < 1.0 ) disp_spect_zoom_factor = 1;
if( disp_spect_zoom_factor > 5000 ) disp_spect_zoom_factor = 5000;



gph_mouse_freq_ini = p.GetPrivateProfileDOUBLE( "Settings", "gph_mouse_freq_ini", 0.0 );
gph_hzoom_mousex_ini = p.GetPrivateProfileLONG( "Settings", "gph_hzoom_mousex_ini", 100 );



g_user_iir_hpf0 = p.GetPrivateProfileLONG( "Settings", "g_user_iir_hpf0", 100 );
g_user_iir_lpf0 = p.GetPrivateProfileLONG( "Settings", "g_user_iir_lpf0", 1000 );
g_user_iir_lpf1 = p.GetPrivateProfileLONG( "Settings", "g_user_iir_lpf1", 2000 );
g_user_iir_lpf2 = p.GetPrivateProfileLONG( "Settings", "g_user_iir_lpf2", 3000 );

miw_user_hpf0->set_value_from_double( g_user_iir_hpf0 );
miw_user_lpf0->set_value_from_double( g_user_iir_lpf0 );
miw_user_lpf1->set_value_from_double( g_user_iir_lpf1 );
miw_user_lpf2->set_value_from_double( g_user_iir_lpf2 );


p.GetPrivateProfileStr( "Settings", "rec_iq_fname", "zzrec00.iq", &rec_iq_fname );
p.GetPrivateProfileStr( "Settings", "play_iq_fname", "zzrec00.iq", &play_iq_fname );

downsample_srate = p.GetPrivateProfileLONG( "Settings", "downsample_srate", 12000 );

if( downsample_srate < cn_downsample_srate_min ) downsample_srate = cn_downsample_srate_min;
if( downsample_srate > cn_downsample_srate_max ) downsample_srate = cn_downsample_srate_max;

miw_dev_dwnconv_srate->set_value_from_double( downsample_srate );


g_b_dwn_aa = p.GetPrivateProfileLONG( "Settings", "g_b_dwn_aa", 1 );
ck_user_dwn_aa->value( g_b_dwn_aa );


b_dc_block_iq = p.GetPrivateProfileLONG( "Settings", "b_dc_block_iq", 1 );
ck_user_dc_block_iq->value( b_dc_block_iq );


b_agc = p.GetPrivateProfileLONG( "Settings", "b_agc", 1 );
ld_agc->ChangeCol( b_agc );


i_deemphasis = p.GetPrivateProfileLONG( "Settings", "i_deemphasis", 0 );
ld_fm_deemph->ChangeCol( i_deemphasis );


for( int i = 0; i < cn_btw_band_max; i++ )
	{
//	cl_button_wheel *bt = btw_band[i];
	
	strpf( s1, "btw_band%02d", i );

	p.GetPrivateProfileStr( "Settings", s1.c_str(), "40M::7e6", &s2 );
	
	mystr m1;

	m1 = s2;
	m1.EscToStr();
	
	s1 = m1.szptr();
	
	btw_band_set_name( i, s1 );
	}


}









void rtl_graph_wnd::save_settings( string ini_fname )
{

string s1, s2, s3, s4;

GCProfile p( ini_fname );

p.WritePrivateProfileLONG( "Settings", "GraphWinX", this->x() - iBorderWidth );
p.WritePrivateProfileLONG( "Settings", "GraphWinY", this->y() - iBorderHeight );
p.WritePrivateProfileLONG( "Settings", "GraphWinCX", this->w() );
p.WritePrivateProfileLONG( "Settings", "GraphWinCY", this->h() );

p.WritePrivateProfileLONG( "Settings", "wnd_fav_x", wnd_fav->x() );
p.WritePrivateProfileLONG( "Settings", "wnd_fav_y", wnd_fav->y() );
p.WritePrivateProfileLONG( "Settings", "wnd_fav_cx", wnd_fav->w() );
p.WritePrivateProfileLONG( "Settings", "wnd_fav_cy", wnd_fav->h() );



p.WritePrivateProfileStr( "Settings", "slast_filename_ini", slast_filename_ini );

s1 = fi_freq_start->value();
p.WritePrivateProfileStr( "Settings", "NetAnalyserFreqStart", s1 );

s1 = fi_freq_stop->value();
p.WritePrivateProfileStr( "Settings", "NetAnalyserFreqStop", s1 );

s1 = fi_freq_step->value();
p.WritePrivateProfileStr( "Settings", "NetAnalyserFreqStep", s1 );

s1 = miw_freq_gain->value();
p.WritePrivateProfileStr( "Settings", "dev_gain", s1 );

s1 = miw_gain_iq->value();
p.WritePrivateProfileStr( "Settings", "gain_iq", s1 );


s1 = fi_freq_threshold->value();
p.WritePrivateProfileStr( "Settings", "NetAnalyserThreshold", s1 );

//s1 = fi_freq_bandwidth->value();
//p.WritePrivateProfileStr( "Settings", "FreqBandwidth", s1 );

s1 = miw_dev_bwidth->value();
p.WritePrivateProfileStr( "Settings", "miw_dev_bw", s1 );


s1 = fi_freq_resolution->value();
p.WritePrivateProfileStr( "Settings", "FreqReso", s1 );

//s1 = fi_bw_lower->value();
//p.WritePrivateProfileStr( "Settings", "BwLower", s1 );

s1 = miw_filt_lwr->value();
p.WritePrivateProfileStr( "Settings", "BwUpper", s1 );

s1 = miw_filt_upr->value();
p.WritePrivateProfileStr( "Settings", "BwUpper", s1 );

s1 = miw_filt_taps->value();
p.WritePrivateProfileStr( "Settings", "miw_filt_taps", s1 );

//int ii = miw_if_freq->value();
//p.WritePrivateProfileLONG( "Settings", "InterFreq", ii );


int ii = ck_if_freq->value();
p.WritePrivateProfileLONG( "Settings", "ck_if_freq", ii );

ii = miw_if_freq->get_value_as_double();
p.WritePrivateProfileLONG( "Settings", "iffreq", ii );


p.WritePrivateProfileLONG( "Settings", "BWLimit", ck_bw_limit->value() );


//s1 = fi_freq_mouse->value();
//p.WritePrivateProfileStr( "Settings", "FreqMouse", s1 );

//s1 = fi_tune->value();
//p.WritePrivateProfileStr( "Settings", "LastTuneFreq", s1 );

ii = miwp_tune->miw->get_value_as_double();
p.WritePrivateProfileLONG( "Settings", "LastTuneFreq", ii );


int d = fi_demodul_mode->value();
p.WritePrivateProfileLONG( "Settings", "FreqDemodMode", d );



double dd = fvs_gain->value();
p.WritePrivateProfileDOUBLE( "Settings", "g_gain_aud", dd );


//dd = fvs_squelch->value();
//p.WritePrivateProfileDOUBLE( "Settings", "Squelch", dd );

int count = vtunehist.size();

for( int i = 0; i < count; i++ )
	{
	strpf( s1 ,"TuneHist_%02d", i );
	p.WritePrivateProfileStr( "Settings", s1.c_str(), vtunehist[ i ] );
	}



mystr m1;

count = vscanhist.size();

for( int i = 0; i < count; i++ )
	{
	strpf( s1 ,"ScanHist_%02d", i );
	m1 = vscanhist[ i ].start;
	m1.FindReplace( s2, ",","", 0 );

	m1 = vscanhist[ i ].stop;
	m1.FindReplace( s3, ",","", 0 );

	strpf( s4 ,"%s,%s", s2.c_str(), s3.c_str() );
	
	p.WritePrivateProfileStr( "Settings", s1.c_str(), s4.c_str() );
	}



vtunehist2 = idb_tune->vstr;
for( int i = 0; i < cn_max_tune_history; i++)
	{
	if( i >= cn_max_tune_history ) break;
	
	strpf(s1,"tunehist_%02d",i);
	if( i < idb_tune->vstr.size() )
		{
		p.WritePrivateProfileStr("Settings", s1.c_str(), idb_tune->vstr[i] );
		}
	else{
		p.WritePrivateProfileStr("Settings", s1.c_str(), "" );			//set as empty
		}
	}


p.WritePrivateProfileLONG( "Settings", "gph_loc_sel", ch_graph_loc_sel->value() );


p.WritePrivateProfileLONG( "Settings", "g_tuning_offset", miw_tuning_offset->get_value_as_double() );

p.WritePrivateProfileLONG( "Settings", "g_freq_sub_tune", g_freq_sub_tune );



p.WritePrivateProfileDOUBLE( "Settings", "g_carrier_max_tune_threshold", miw_carrier_max_tune_threshold->get_value_as_double() );


p.WritePrivateProfileStr( "Settings", "slast_favourite_fname", slast_favourite_fname.c_str() );


p.WritePrivateProfileLONG( "Settings", "g_dev_direct_sampling", ld_direct_sampling->GetColIndex() );

p.WritePrivateProfileLONG( "Settings", "g_dev_bias_t", ld_bias_t->GetColIndex() );




for( int i = 0; i < cn_freq_memory_max; i++ )
	{
	strpf( s1, "freq_memory%02d", i );
	strpf( s2, "%d,%d", freq_memory[i].freq_tune, freq_memory[i].freq_sub_tune );
	
	p.WritePrivateProfileStr( "Settings", s1.c_str(), s2.c_str() );
	}




for( int i = 0; i < cn_preset_memory_max; i++ )
	{
	strpf( s1, "preset_memory%02d", i );
	
	mystr m1 = preset_memory[i].sname;
	m1.StrToEscMostCommon3();
	strpf( s2, "sname=%s,freq_tune=%d,freq_sub_tune=%d,demod_type=%d,b_use_iffreq=%d,if_freq=%d,b_bw_limit=%d,iffreq_bw_low=%d,iffreq_bw_high=%d,iffreq_bw_taps=%d,dev_gain=%f,direct_sampling=%d,iq_gain=%f,aud_gain=%f,b_agc=%d,b_bias_t=%d,i_deemph=%d,b_dwn_aa=%d,i_dwn_srate=%d,b_dcblk=%d", m1.szptr(), preset_memory[i].freq_tune, preset_memory[i].freq_sub_tune, preset_memory[i].demod_type, preset_memory[i].b_use_iffreq, preset_memory[i].if_freq, preset_memory[i].b_bw_limit, preset_memory[i].iffreq_bw_low, preset_memory[i].iffreq_bw_high, preset_memory[i].iffreq_bw_taps, preset_memory[i].dev_gain, preset_memory[i].direct_sampling, preset_memory[i].iq_gain, preset_memory[i].aud_gain, preset_memory[i].b_agc, preset_memory[i].b_bias_t, preset_memory[i].i_deemph, preset_memory[i].b_dwn_aa, preset_memory[i].i_dwn_srate, preset_memory[i].b_dcblk );

	p.WritePrivateProfileStr( "Settings", s1.c_str(), s2.c_str() );
	}


for( int i = 0; i < cn_preset_memory_max2; i++ )
	{
	strpf( s1, "preset_memory2_%02d", i );
	
	mystr m1 = preset_memory2[i].sname;
	m1.StrToEscMostCommon3();
	strpf( s2, "sname=%s,freq_tune=%d,freq_sub_tune=%d,demod_type=%d,b_use_iffreq=%d,if_freq=%d,b_bw_limit=%d,iffreq_bw_low=%d,iffreq_bw_high=%d,iffreq_bw_taps=%d,dev_gain=%f,direct_sampling=%d,iq_gain=%f,aud_gain=%f,b_agc=%d,b_bias_t=%d,i_deemph=%d,b_dwn_aa=%d,i_dwn_srate=%d,b_dcblk=%d", m1.szptr(), preset_memory2[i].freq_tune, preset_memory2[i].freq_sub_tune, preset_memory2[i].demod_type, preset_memory2[i].b_use_iffreq, preset_memory2[i].if_freq, preset_memory2[i].b_bw_limit, preset_memory2[i].iffreq_bw_low, preset_memory2[i].iffreq_bw_high, preset_memory2[i].iffreq_bw_taps, preset_memory2[i].dev_gain, preset_memory2[i].direct_sampling, preset_memory2[i].iq_gain, preset_memory2[i].aud_gain, preset_memory2[i].b_agc, preset_memory2[i].b_bias_t, preset_memory2[i].i_deemph, preset_memory2[i].b_dwn_aa, preset_memory2[i].i_dwn_srate, preset_memory2[i].b_dcblk );

	p.WritePrivateProfileStr( "Settings", s1.c_str(), s2.c_str() );
	}


s1 = "zzz_fav_save_on_exit.txt";

save_favourite_list( s1 );



for( int i = 0; i < cn_filter_memory_max; i++ )
	{
	st_filter_memory_tag *st = filter_memory + i;
	
	strpf( s1, "filter_memory_%02d", i );
	
	mystr m1 = st->sname;
	m1.StrToEscMostCommon3();
	strpf( s2, "sname=%s,b_bw_limit=%d,iffreq_bw_low=%d,iffreq_bw_high=%d,iffreq_bw_taps=%d", m1.szptr(), st->b_bw_limit, st->iffreq_bw_low, st->iffreq_bw_high, st->iffreq_bw_taps );

	p.WritePrivateProfileStr( "Settings", s1.c_str(), s2.c_str() );
	}





for( int i = 0; i < cn_dwn_srate_memory_max; i++ )
	{
	st_dwn_srate_memory_tag *st = dwn_srate_memory + i;
	
	strpf( s1, "dwn_srate_memory%02d", i );
	
	mystr m1 = st->sname;
	m1.StrToEscMostCommon3();
	strpf( s2, "sname=%s,dwn_srate=%d", m1.szptr(), st->dwn_srate );

	p.WritePrivateProfileStr( "Settings", s1.c_str(), s2.c_str() );
	}




p.WritePrivateProfileLONG( "Settings", "b_wnd_fav_is_open", b_wnd_fav_is_open );

p.WritePrivateProfileLONG( "Settings", "b_clip_enable", b_clip_enable );

p.WritePrivateProfileDOUBLE( "Settings", "clip_level_audio", clip_level_audio );

p.WritePrivateProfileLONG( "Settings", "ispec_avg_wnd", ispec_avg_wnd );

p.WritePrivateProfileDOUBLE( "Settings", "disp_spect_zoom_factor", disp_spect_zoom_factor );
p.WritePrivateProfileDOUBLE( "Settings", "gph_mouse_freq_ini", gph_mouse_freq_ini );
p.WritePrivateProfileLONG( "Settings", "gph_hzoom_mousex_ini", gph_hzoom_mousex_ini );




p.WritePrivateProfileLONG( "Settings", "g_user_iir_hpf0", g_user_iir_hpf0 );
p.WritePrivateProfileLONG( "Settings", "g_user_iir_lpf0", g_user_iir_lpf0 );
p.WritePrivateProfileLONG( "Settings", "g_user_iir_lpf1", g_user_iir_lpf1 );
p.WritePrivateProfileLONG( "Settings", "g_user_iir_lpf2", g_user_iir_lpf2 );



p.WritePrivateProfileStr( "Settings", "rec_iq_fname", rec_iq_fname.c_str() );
p.WritePrivateProfileStr( "Settings", "play_iq_fname", play_iq_fname.c_str() );

p.WritePrivateProfileLONG( "Settings", "downsample_srate", downsample_srate );


p.WritePrivateProfileLONG( "Settings", "g_b_dwn_aa", g_b_dwn_aa );

p.WritePrivateProfileLONG( "Settings", "b_agc", b_agc );

p.WritePrivateProfileLONG( "Settings", "b_dc_block_iq", b_dc_block_iq );

p.WritePrivateProfileLONG( "Settings", "i_deemphasis", i_deemphasis );


for( int i = 0; i < cn_btw_band_max; i++ )
	{
	cl_button_wheel *bt = btw_band[i];
	
	strpf( s1, "btw_band%02d", i );
	
	mystr m1;
	m1 = bt->sname;
	m1.StrToEscMostCommon3();
	
	strpf( s2, "%s", m1.szptr() );

	p.WritePrivateProfileStr( "Settings", s1.c_str(), s2.c_str() );
	}


}










void rtl_graph_wnd::get_trace_min_max( double &xmin, double &xmax, double &ymin, double &ymax )
{
gph0->get_trace_min_max( 0, xmin, xmax, ymin, ymax );
}




void rtl_graph_wnd::get_trace_maxy( int &idx, double &x, double &y )
{

gph0->get_trace_maxy( 0, idx, y );

//grph->get_trace_min_max( 0, sample_idx, xmin, xmax, ymin, ymax );
}







void rtl_graph_wnd::set_selected_sample( int trc, int sel_sample_idx, bool issue_which_callback )
{
gph0->set_selected_sample( trc, sel_sample_idx, issue_which_callback );
}






void rtl_graph_wnd::set_dev_bwidth( int freq )
{
string s1;


if( freq < cn_rtl_e4000_sample_bandwith_min ) freq = cn_rtl_e4000_sample_bandwith_min;
if( freq > cn_rtl_e4000_sample_bandwith_max ) freq = cn_rtl_e4000_sample_bandwith_max;

strpf(s1, "%d", freq );

miw_dev_bwidth->value( s1.c_str() );
//cb_bt_freq_tune( 0, 0 );
wnd_rtl_graph->freq_listen( 1, 1, 1, 1, "set_dev_bwidth()" );

//i_fftw_trig_plan_create_state = 0; 		//start the transition state going which will create require fftw plans
sync_wr_rd_pointer = 1;					//trigger a wr/rd pointer reset
}










//NOTE the gain NEEDS to be set after 'set_dev_direct_sampling()' call
bool rtl_graph_wnd::set_dev_direct_sampling( unsigned int state )
{
if( state > 2 ) return 0;

g_dev_direct_sampling = state;

rtl.set_direct_sampling( g_dev_direct_sampling );

return 1;
}










//update sel graph sample's details
void rtl_graph_wnd::update_sel( int trc, vector<st_spect_tag> &vspct )
{
string s1;
int sel_idx;

if( gph0->get_selected_idx( trc, sel_idx ) )
	{
	strpf( s1, "%d", (int) vspct[ sel_idx ].freq );
//	fi_tune->value( s1.c_str() );
	miwp_tune->miw->set_value_from_double( vspct[ sel_idx ].freq );
	idb_tune->value( s1 );

	strpf( s1, "%.2g dB", vspct[ sel_idx ].ampl );
	fi_ampl_mouse->value( s1.c_str() );

	}
else{
	s1 = "????";
//	fi_tune->value( s1.c_str() );
	fi_ampl_mouse->value( s1.c_str() );
	}

}








/*
void rtl_graph_wnd::update()
{

if( ck_show_demod->value() )
	{
	update_graph( vspect_demod );
	}
else{
	update_graph( vspect );
	}


if( g_audio_out_gain >= g_squelch_gain_100 )
	{
	led_squelch->ChangeCol( 1 );
	}
else{
	led_squelch->ChangeCol( 0 );
	}
}
*/












//'idx' is to 'st_fftw[]'
//forward complex fft, clears vsp
//spectrum is supplied in fftw ordering,
//scales result down to remove fft 'gain',
//returns 1 if 'idx' exists and fft was completed, else 0

bool fftw_fwd_clpx_cplx( en_fftw_index_allocations_tag idx, vector<filter_code::st_cplex_tag> vin, vector<filter_code::st_cplex_tag> &vsp, float gain_in )
{
bool vb = 0;
st_spect_tag o;
filter_code::st_cplex_tag oc;

vsp.clear();


st_fftw_plans_tag *of = st_fftw + idx;



int siz = vin.size();


if( !of->built )
	{
	printf("fftw_fwd_clpx_cplx() -  idx%d: plan not built, '%s'\n", idx, of->sname.c_str() );
	return 0;
	}

if( siz != of->size )
	{
	printf("fftw_fwd_clpx_cplx() - failed as incorrect size, size is vin.size(): %d  of->size %d, sname: '%s'\n", vin.size(), of->size, of->sname.c_str() );
	return 0;
	}


if(vb)printf("fftw_fwd_clpx_cplx() - size is vin.size(): %d  of->size %d, sname: '%s'\n", vin.size(), of->size, of->sname.c_str() );


fftw_complex *fftwi = of->fftwic;
fftw_complex *fftwo = of->fftwoc;
fftw_plan fftwp = of->fftwp;


//load complex array
for( int i = 0; i < siz; i++ )
	{
	oc = vin[ i ];
	fftwi[ i ][ 0 ] = oc.real;
	fftwi[ i ][ 1 ] = oc.imag;
	}


fftw_execute( fftwp );						//calc complex to complex fwd fft


//store fft result
for( int i = 0; i < siz; i++ )
	{
	oc.real = fftwo[ i ][ 0 ] / siz * gain_in;
	oc.imag = fftwo[ i ][ 1 ] / siz * gain_in;
	
	vsp.push_back( oc );
	}

//vsp[0].real = 0.3;
//vsp[0].imag = 0;

return 1;
}












//'idx' is to 'st_fftw[]'
//forward complex fft, clears vsp
//spectrum is supplied in fftw ordering,
//scales result down to remove fft 'gain',
//as this is a real to complex fwd fft, the neg spectrum in not provided by fftw, so this code duplicates the positive freqs as conjugates to make the negative spectrum
//returns 1 if 'idx' exists and fft was completed, else 0

//fftw ordering when N=8
// 0    1    2    3    4  (no neg freq are provided by fftw as this is a real to complex fwd fft)
// |----|----|----|----|
// DC  +1   +2   +3   Nyq  -3   -2   -1		<--- index 4 is the nyquist  +1 +2 +3 are the positive freqs (negative freqs -1 -2 -3 will be inserted by this code)


//vsp[] ordering with mimiced neg freq using complex conjugate  (no neg freq are provided by fftw as this is a real to complex fwd fft)

// 0    1    2    3    4    5    6    7		<--- vsp[] indexes in the final arangement (mimics fftw ordering)
// 0    1    2    3    4    3    2    1		<--- vsp[] indexes used to create the neg freqs from positive freqs when N=8
// |----|----|----|----|----|----|----|
// DC  +1   +2   +3   Nyq  -3   -2   -1		<--- index 4 is the nyquist  +1 +2 +3 are the positive freqs (as this is a real to complex fwd fft, the neg
//													spectrum in not provided by fftw, so this code duplicates the positive freqs as conjugates to make the negative spectrum)


//fftw ordering when N=512
// 0    1    2       255  256  257      510   511		<--- vsp[] indexes in the final arangement (mimics fftw ordering)
// 0    1    2       255  256  255       2     1		<--- vsp[] indexes used to create the neg freqs from positive freqs when N=512
// |----|----|--....--|----|----|--....--|-----|
// DC  +1   +2      +254  Nyq  -254     -2    -1		<--- index 256 is the nyquist  +1 +2 are the positive freqs (as this is a real to complex fwd fft, the neg
//																spectrum in not provided by fftw, so this code duplicates the positive freqs as conjugates to make the negative spectrum)




bool fftw_fwd_real_cplx( en_fftw_index_allocations_tag idx, vector<float> &vin, vector<filter_code::st_cplex_tag> &vsp, float gain_in )
{
bool vb = 0;
st_spect_tag o;
filter_code::st_cplex_tag oc;

vsp.clear();


st_fftw_plans_tag *of = st_fftw + idx;



int siz = vin.size();


if( !of->built )
	{
	printf("fftw_fwd_clpx_cplx() -  idx%d: plan not built, '%s'\n", idx, of->sname.c_str() );
	return 0;
	}

if(siz != of->size )
	{
	printf("fftw_fwd_clpx_cplx() - failed as incorrect size, size is vin.size(): %d  of->size %d, sname: '%s'\n", vin.size(), of->size, of->sname.c_str() );
	return 0;
	}


int half = siz / 2;


if(vb)printf("fftw_fwd_clpx_cplx() - size is vin.size(): %d  of->size %d, sname: '%s'\n", vin.size(), of->size, of->sname.c_str() );


double *fftwi = of->fftwir;
fftw_complex *fftwo = of->fftwoc;
fftw_plan fftwp = of->fftwp;


//load real array
for( int i = 0; i < siz; i++ )
	{
	fftwi[ i ] = vin[ i ];
	}


fftw_execute( fftwp );						//calc complex to complex fwd fft

//fftwo[ 0 ][ 0 ] = 1024.0;
//fftwo[ 0 ][ 1 ] = 0.0;
//fftwo[ 256 ][ 0 ] = 512.0;				//nyquist test
//fftwo[ 256 ][ 1 ] = 0.0;


float scl = 1.0f / siz * gain_in;

//collect and scale positive freqs
for( int i = 0; i < half+1; i++ )				//nyquist is last freq bin fftw fills (only positive freqs are provided)
	{
	oc.real = fftwo[ i ][ 0 ] * scl;
	oc.imag = fftwo[ i ][ 1 ] * scl;
	
	vsp.push_back( oc );
	}

//vsp.clear();
//vsp.resize( 5 );

//half = 8 / 2;

//store fft complex conjugate
int j = half-1;
for( int i = 0; i < (half - 1); i++ )				//e.g. access vsp[] positive freqs in this order to make neg freqs, vsp[3], vsp[2], vsp[1]
	{
	oc.real = vsp[ j ].real;
	oc.imag = -vsp[ j ].imag;
	vsp.push_back( oc );
//	printf("fftw_fwd_clpx_cplx() - vsp[%03d] j %d  \n", vsp.size() - 1, j );
	j--;
//	if( j == 0 )
//		{
//		printf("fftw_fwd_clpx_cplx() -  idx%d: 'j' became neg when duplicating complex conjugate for neg freqs, '%s'\n", idx, of->sname.c_str() );
//		}
	}



//printf("fftw_fwd_clpx_cplx() - vsp %03d  \n", vsp.size() );

//getchar();


/*
//store fft result
for( int i = 0; i < siz; i++ )
	{
	oc.real = fftwo[ i ][ 0 ] / siz * gain_in;
	oc.imag = fftwo[ i ][ 1 ] / siz * gain_in;
	
	vsp.push_back( oc );
	}

//vsp[0].real = 0.3;
//vsp[0].imag = 0;
*/

return 1;
}



void hzoom_index_calc( float zoom_h, unsigned int wid, unsigned int idx_cnt, unsigned int idx_zoom, unsigned int idx_zoom_pixel_pos, unsigned int &idx_left,unsigned int &idx_right )
{
    if( zoom_h < 1.0f ) zoom_h = 1.0f;

    // visible span in bins
    float visible_bins = (float)idx_cnt / zoom_h;

    // bins per pixel
    float bins_per_pixel = visible_bins / (float)wid;

    // compute left edge (float)
    float idx_left_f = (float)idx_zoom - ((float)idx_zoom_pixel_pos * bins_per_pixel);

    // clamp left
    if( idx_left_f < 0.0f )
		{
        idx_left_f = 0.0f;
		}

    // compute right edge (float)
    float idx_right_f = idx_left_f + visible_bins;

    // clamp right
    if( idx_right_f >= (float)idx_cnt )
		{
        idx_right_f = (float)idx_cnt - 1;
        idx_left_f  = idx_right_f - visible_bins;

        if( idx_left_f < 0.0f )
			{
            idx_left_f = 0.0f;
			}
		}

    // convert to integer indices
    idx_left  = (unsigned int)(idx_left_f + 0.5f);
    idx_right = (unsigned int)(idx_right_f + 0.5f);

    // final safety clamp
    if( idx_right >= idx_cnt ) idx_right = idx_cnt - 1;
    if( idx_left >= idx_right ) idx_left = (idx_right > 0) ? idx_right - 1 : 0;
}








bool find_indexes_at_freq( vector<st_spect_tag> &vsp, double freq, int &idx_before, int &idx_beyond, bool b_use_freq_actual )
{
bool vb = 1;

if( vsp.size() < 2 ) return 0;


idx_before = -1;
idx_beyond = -1;

int cnt = vsp.size();

//if(vb)printf("vsp[0].freq_actual %f  vsp[%d].freq_actual %f\n", vsp[0].freq_actual, cnt-1, vsp[cnt-1].freq_actual );

for( int i = 0; i < vsp.size(); i++ )
	{
		
	if( b_use_freq_actual )
		{
		if( vsp[i].freq_actual > freq )
			{
if(vb)printf("hzoom_index_calc() - vsp[0].freq_actual %f  vsp[%d].freq_actual %f\n", vsp[0].freq_actual, cnt-1, vsp[cnt-1].freq_actual );
			idx_beyond = i;
			break;
			}
		}
	else{
		if( vsp[i].freq > freq )
			{
			idx_beyond = i;
			break;
			}
		}
	}

if(vb)printf("VVVVVVVVVVVhzoom_index_calc() - idx_before %d %d\n", idx_before, idx_beyond );

if( idx_beyond == -1 ) return 0;
idx_before = idx_beyond - 1;

if( idx_before < 0 ) idx_before = 0;
if(vb)printf("hzoom_index_calc() - idx_before %d %d\n", idx_before, idx_beyond );


return 1;
}















unsigned int idx_left = 100;											//left most bin index in '' for 'gph0', see also 'freq_left'
unsigned int idx_right = 101;



int taper_windowing_size_last0 = 0;			//these are used to detect if a newly calc'd taper window is required (due to a signal sample block size change)
//vector<float> vtaper_window0;
float taper_window_gain_corr0;





int gph0_cnt = 0;
int update_gph0_cnt = 0;	//used for delaying the startup settings for gph0 such as hzoom and 'idx_left' and 'idx_right' restoration, refer 'gph_mouse_freq_ini'
							//also used for other things












void rtl_graph_wnd::update_prep_gph0_downsample()
{
vector <filter_code::st_cplex_tag> viq;


int idx = bf1_prep_idx_atm.load(std::memory_order_acquire);				//this ensures thread safe access

if( idx != -1 )
	{
	int cnt = bf1_cnt_atm.load( std::memory_order_relaxed );	

//	printf("update_prep_gph0_downsample() - HHHHHHHHHHHHHHHHHHHHHH idx %d cnt %d  iq %f \n", idx, cnt, bf1_iq[idx][0].real );
//	printf("update_prep_gph0_downsample() - HHHHHHHHHHHHHHHHHHHHHH idx %d iq %f \n", cnt);

	for ( int i = 0; i < cnt; i++ )
		{
		viq.push_back( bf1_iq[idx][i] );
		}
		
	bf1_prep_idx_atm.store(-1, std::memory_order_release);				//flag buf processed
	}
else{
	return;
	}






if( viq.size() < 2 )
	{
	printf("QQQQQQQQQQQQQQQQQQQQQQQ update_prep_gph0_downsample() - 'viq' is too small: %d, expect no gph0 display\n\n", viq.size() );
	return;
	}


//vspect_gph.clear();
//return;


int down_sample_factor_for_graph2 = 1;//viq.size() / 2048;

vector <filter_code::st_cplex_tag> vcpx0;

low_pass_srconv( viq,  vcpx0, down_sample_factor_for_graph2, filt_prev_idx12, now_r12, now_j12 );			//complex decimate







viq = vcpx0;

int zeropad_factor = 0;

int zero_pad_cnt = viq.size() * (zeropad_factor - 1);//pref_zero_padding_for_fft;

for( int i = 0; i < zero_pad_cnt; i++ )
	{
	filter_code::st_cplex_tag oc = {0,0};
	
	viq.push_back( oc );
	}



if( (!b_dbg_no_graph_update ) )
//	if( (!b_dbg_no_graph_update ) && ( !(demod_cnt%3 ) ) )
	{
	vector <filter_code::st_cplex_tag> vcplex_sp;
	vector <filter_code::st_cplex_tag> vcplex_sp_downsample;


//--------------- taper windowing ------------------
	if( pref_taper_windowing_for_fft )
		{
		if( taper_windowing_size_last0 != viq.size() )				//reduce excessive calcs
			{
		
			filter_code::window_function_float( filter_code::fwt_hann, viq.size(), vtaper_window0 );
			
			taper_window_gain_corr0 = filter_code::window_calc_normalisation_factor_float( vtaper_window0 );
			
			taper_windowing_size_last0 = viq.size();
			printf( "update_prep_gph0_downsample() - TTTTTTTTTTTTTTTTTT built taper window 'vtaper_window0', size is %d\n", vtaper_window0.size() );
			}
						
		for( int j = 0; j < viq.size(); j++ )
			{
			viq[j].real *= vtaper_window0[j] * taper_window_gain_corr0;
			viq[j].imag *= vtaper_window0[j] * taper_window_gain_corr0;
			}

	//			int cnt = viq_local_zero_pad.size();
	//			for( int i = 0; i < cnt; i++ )
	//				{
	//				float f0 = 0.5f - (0.5f * cosf( (twopi * i) / (cnt - 1) ) );		//hann window test
	//				vtaper_window.push_back( f0 );
	//				}
	//				b_build_taper_window0 = 0;
		}
//------------------------------------------------





//--------------- zero padding ------------------
	bool bzeropad = pref_zero_padding_for_fft;
	int zeropad_factor = pref_zero_padding_for_fft_cnt;
	if( bzeropad )
		{
		int cnt = viq.size();
//				printf( "plot_using_vectors_eval() - zeropad_factor %d,  vcpx00.size() %d\n", zeropad_factor, (int)vcpx00.size() );
		filter_code::st_cplex_tag oc;
		oc.real = 0;
		oc.imag = 0;
		
//				bw_bin /= (float)zeropad_factor;						//adj bwidth as padding
		for( int j = 0; j < cnt * (zeropad_factor); j++ )
			{
			viq.push_back( oc );
			}
		}
//------------------------------------------------




	int b_destroy_only = 0;
	fftw_build_if_req( en_fia_gph0, b_destroy_only, "0: fftw fwd cplx to cplx: for graticle graph", en_fpt_fwd_cplx_to_cplx, viq.size() );	//check if fftw needs to be built/resized


	float fft_gain = 1;
	if( bzeropad ) fft_gain *= zeropad_factor+1;


	fftw_fwd_clpx_cplx( 0, viq, vcplex_sp, fft_gain );						//go to freq domain using an fft





//	if( i_fftw_trig_plan_create_state == 2 )						//plans created?
//		{
//		else{
//			bool b_plan_exists = complex_fwd_fft_multi( viq, vcplex_sp, "0: for display", en_ftid_gui );			//go to freq domain using an fft
//			}
//		}










//printf("update_prep_gph0_downsample() - i_fftw_trig_plan_create_state %d  viq %d  vcplex_sp %d\n", i_fftw_trig_plan_create_state, viq.size(), vcplex_sp.size() );



//		down_sample_factor_for_graph = 30;

//	down_sample_factor_for_graph = 30;


//		low_pass_srconv( vcplex_sp,  vcplex_sp_downsample, down_sample_factor_for_graph, filt_prev_idx2, now_r2, now_j2 );			//complex decimate

	vcplex_sp_downsample = vcplex_sp;


//printf("update_prep_gph0_downsample() - vcplex_sp_downsample %d\n", vcplex_sp_downsample.size() );

//		printf("update_prep_gph0_downsample() - down_sample_factor_for_graph %d, vcplex_sp.size() %d vcplex_sp_downsample.size() %d\n", down_sample_factor_for_graph, vcplex_sp.size(), vcplex_sp_downsample.size() );


	complex_fft_displayable( g_dev_bw, g_freq_tune, vcplex_sp_downsample, vspect_displayble, pref_show_dc_for_fft_graphs, 1 );

//printf("update_prep_gph0_downsample() - vspect_displayble[0].freq %f %f\n", vspect_displayble[0].freq, vspect_displayble[vspect_displayble.size()-1].freq );






//printf("update_prep_gph0_downsample() -vcplex_sp_downsample %d vspect_displayble %d\n", vcplex_sp_downsample.size(), vspect_displayble.size() );

/*
	int half = vspect_displayble.size() / 2;

	//vspect_displayble[ half + 100 ].ampl = 10.0f;

	//disp_spect_zoom_factor = 2.0f;

	if( disp_spect_zoom_factor != 0.0f )
		{
		int zoom_cnt = vspect_displayble.size() / disp_spect_zoom_factor;	//reduce size of spectral span to fit widget width

//printf("update_prep_gph0_downsample() - disp_spect_zoom_factor %f\n", disp_spect_zoom_factor );

		int half2 = zoom_cnt / 2;
		
		int start = half - half2; 
		int end = half + half2; 
		
		vector<st_spect_tag> vspect_disp_actual;

		for( int i = start; i < end; i++ )
			{
			st_spect_tag o;
			
			o = vspect_displayble[i];

			vspect_disp_actual.push_back( o );
			}

//			vspect_displayble = vspect_disp_actual;
//			vspect_gph = vspect_displayble;

//		vspect_gph = vspect_disp_actual;
		}
	//	complex_rev_fft_multi( vcplex_sp, vtdm );									//go to time domain using a rev fft

*/

	//---- adjust display for hzoom and h position (pan) -----




		double mouse_freq = gph_mouse_freq_in_bw;
		gph_hzoom_mousex = wnd_rtl_graph->gph0->mousex;

		#define cn_hzoom_restore_from_ini_delay 10
		
		if( update_gph0_cnt == cn_hzoom_restore_from_ini_delay )		//do only once on start up 
			{
			mouse_freq = gph_mouse_freq_ini;							
			gph_hzoom_mousex = gph_hzoom_mousex_ini;
//			disp_spect_zoom_factor = 69.4;
			disp_spect_zoom_factor_last = 0;
printf("update_prep_gph0_downsample() - ZZZZZZZZZZZZZZZZZZZZZZZZZZZ restoring hzoom position:  mouse_freq %f, gph_hzoom_mousex %d   disp_spect_zoom_factor %f\n", mouse_freq, gph_hzoom_mousex, disp_spect_zoom_factor  );
			}



	int idx_before = 0, idx_beyond;

	if( b_need_h_zoom_recalc )											//helps refresh stale hzoom 'idx_left'  'idx_right', e.g. when zero padding pref is toggled
		{
		b_need_h_zoom_recalc = 0;
		disp_spect_zoom_factor_last = 1.234e9;							//any large number to trigger below
		}

	if( disp_spect_zoom_factor != disp_spect_zoom_factor_last )			//zoom change?
//	if( 1 )
		{
		disp_spect_zoom_factor_last = disp_spect_zoom_factor;


		if( update_gph0_cnt > cn_hzoom_restore_from_ini_delay )			//always do once started up
			{ 
			gph_mouse_freq_ini = mouse_freq;							//keep a copy for saving to ini
			gph_hzoom_mousex_ini = gph_hzoom_mousex;
			}


		disp_spect_zoom_factor_last = disp_spect_zoom_factor;
		if( disp_spect_zoom_factor != 0.0f )
			{

			if( find_indexes_at_freq( vspect_displayble, mouse_freq, idx_before, idx_beyond, 0 ) )
				{
//				printf("update_prep_gph0_downsample() - KKKKKKKKK mouse_freq %f, idx_before %d %d  vspect_displayble %d   freq %f, freq_actual %f\n", mouse_freq, idx_before, idx_beyond, vspect_displayble[idx_before].freq, vspect_displayble[idx_before].freq_actual  );
//				printf("update_prep_gph0_downsample() - KKKKKKKKK  gph_hzoom_mousex %d\n", gph_hzoom_mousex );
	//			printf("update_prep_gph0_downsample() - idx_before %d %d  vspect_displayble %d   freq %f, mx %d\n", idx_before, idx_beyond, vspect_displayble.size(), vspect_displayble[0].freq,wnd_rtl_graph->gph0->mousex );
				}

printf("update_prep_gph0_downsample() - KKKKKKKKK mouse_freq %f, gph_hzoom_mousex %d\n", mouse_freq, gph_hzoom_mousex  );


	//		hzoom_index_calc( 1.25, 1200, 512, 128, 300, idx_left, idx_right );
	//void hzoom_index_calc( float zoom_h, unsigned int wid, unsigned int idx_cnt, unsigned int idx_zoom, unsigned int idx_zoom_pixel_pos, unsigned int &idx_left,unsigned int &idx_right )

			hzoom_index_calc( disp_spect_zoom_factor, gph0->w(), vspect_displayble.size(), idx_before, gph_hzoom_mousex, idx_left, idx_right );	//calc which indexes are left most and right most

printf("update_prep_gph0_downsample() - KKKKKKKKK idx_left %d %d \n", idx_left, idx_right );
			}
		}



if( ineed_hzoom_and_center_to_cur_freq > 0 )
	{
	ineed_hzoom_and_center_to_cur_freq--;

	if( ineed_hzoom_and_center_to_cur_freq == 0 )
		{
		ineed_hzoom_and_center_to_cur_freq = 0;
	//			need_hzoom_and_center_to_this_freq;
		
		if( nhac_hzoom_factor > 0.0f ) 
			{
			disp_spect_zoom_factor = nhac_hzoom_factor;
			miw_gph_disp_spect_zoom_factor->set_value_from_double( disp_spect_zoom_factor );
			}


		gph_mouse_freq_ini;							
		gph_hzoom_mousex_ini;


	//	need_hzoom_and_center_to_this_freq

		if( find_indexes_at_freq( vspect_displayble, 0, idx_before, idx_beyond, 0 ) )
			{
	//				printf("update_prep_gph0_downsample() - KKKKKKKKK mouse_freq %f, idx_before %d %d  vspect_displayble %d   freq %f, freq_actual %f\n", mouse_freq, idx_before, idx_beyond, vspect_displayble[idx_before].freq, vspect_displayble[idx_before].freq_actual  );
	//				printf("update_prep_gph0_downsample() - KKKKKKKKK  gph_hzoom_mousex %d\n", gph_hzoom_mousex );
	//			printf("update_prep_gph0_downsample() - idx_before %d %d  vspect_displayble %d   freq %f, mx %d\n", idx_before, idx_beyond, vspect_displayble.size(), vspect_displayble[0].freq,wnd_rtl_graph->gph0->mousex );
			}
		
		int pixel_x = gph0->w()/2;
		hzoom_index_calc( disp_spect_zoom_factor, gph0->w(), vspect_displayble.size(), idx_before, pixel_x, idx_left, idx_right );	//calc which indexes are left most and right most
		}
//printf("update_prep_gph0_downsample() - VVVVVVVVVVVVV nhac_to_this_freq %f, idx_before %d %d   idx_left %d %d\n", nhac_to_this_freq, idx_before, idx_beyond, idx_left, idx_right );
	}



	if( idx_left < 0 ) 
		{
		idx_left = 0;													//limit to valid indexes
		idx_right = idx_left + 1;
		}

	if( idx_left >= vspect_displayble.size() )
		{
		idx_left = vspect_displayble.size()/2;
		idx_right = idx_left + 1;
		}

//	if( idx_right >= vspect_displayble.size() )
//		{
//		idx_right = idx_left;
//		}

	if( idx_right >= vspect_displayble.size() - 1 )
		{
		if( idx_right < 0 ) idx_right = 0;
		idx_left = 0;
//		idx_right = idx_left;
		}

//			if( find_indexes_at_freq( vspect_displayble, mouse_freq, idx_before, idx_beyond, 0 ) )
//				{
//				}

//hzoom_index_calc( disp_spect_zoom_factor, gph0->w(), vspect_displayble.size(), 22000, gph_hzoom_mousex, idx_left, idx_right );	//calc which indexes are left most and right most

//printf("update_prep_gph0_downsample() - KKKKKKKKKK idx_before %d %d  idx_left %d %d    vspect_displayble.size() %d\n", idx_before, idx_beyond, idx_left, idx_right, vspect_displayble.size() );
//printf("update_prep_gph0_downsample() - idx_left %d %d\n", idx_left, idx_right );

if(vb0)printf("update_prep_gph0_downsample() - idx_left %d %d\n", idx_left, idx_right );





	vector<st_spect_tag> vspect_disp_actual;
//	vspect_disp_actual.clear();

//	gph_freq_left = vspect_displayble[0].freq;


	for( int i = idx_left; i < idx_right; i++ )							//only keep spectra in visible region
		{
		st_spect_tag o;

		o = vspect_displayble[i];

		vspect_disp_actual.push_back( o );
		}

	vspect_gph = vspect_disp_actual;

//printf("update_prep_gph0() - vspect_gph %d\n", vspect_gph.size() );
//printf("update_prep_gph0_downsample() - KKKKKKKKKK gph_hzoom_mousex %d, idx_left %d %d  idx_before %d\n", gph_hzoom_mousex, idx_left, idx_right, idx_before );
	}
	//---------------------------------------------------------




//	printf("update_prep_gph0_downsample() - HHHHHHHHHHHHHHHHHHHHHH vspect_gph %d\n", vspect_gph.size());



//		gc_srateconv_code::qdss_resample_float_vector( output_count1, saf1.srate, nyquist, 32, vaud, vaudclip1 );


}









/*

//have very low reso, but shows entire bw
void temp_code0_small_fft()
{
vector <filter_code::st_cplex_tag> viq;


int idx = bf1_prep_idx_atm.load(std::memory_order_acquire);				//this ensures thread safe access

if( idx != -1 )
	{
	int cnt = bf1_cnt_atm.load( std::memory_order_relaxed );	

//	printf("temp_code0() - HHHHHHHHHHHHHHHHHHHHHH idx %d cnt %d  iq %f \n", idx, cnt, bf1_iq[idx][0].real );

	if( cnt >= 2048 )
		{
		for ( int i = 0; i < 2048; i++ )
			{
			viq.push_back( bf1_iq[idx][i] );
			}
		}
		
	bf1_prep_idx_atm.store(-1, std::memory_order_release);				//flag buf processed
	}
else{
	return;
	}






//	int down_sample_factor_for_graph2 = viq.size() / 2048;

//vector <filter_code::st_cplex_tag> vcpx0;

//low_pass_srconv( viq,  vcpx0, down_sample_factor_for_graph2, filt_prev_idx12, now_r12, now_j12 );			//complex decimate

//viq = vcpx0;


	if( (!b_dbg_no_graph_update ) )
//	if( (!b_dbg_no_graph_update ) && ( !(demod_cnt%3 ) ) )
//	if( 1 )
		{
		vector <filter_code::st_cplex_tag> vcplex_sp;
		vector <filter_code::st_cplex_tag> vcplex_sp_downsample;

		if( i_fftw_trig_plan_create_state == 2 )						//plans created?
			{
	//		else{
				bool b_plan_exists = complex_fwd_fft_multi( viq, vcplex_sp, "0: for display", en_ftid_audio_proc );			//go to freq domain using an fft
	//			}
			}





//		down_sample_factor_for_graph = 1;

	//	down_sample_factor_for_graph = 30;


//		low_pass_srconv( vcplex_sp,  vcplex_sp_downsample, down_sample_factor_for_graph, filt_prev_idx2, now_r2, now_j2 );			//complex decimate


vcplex_sp_downsample = vcplex_sp;


//		printf("temp_code0() - down_sample_factor_for_graph %d, vcplex_sp.size() %d vcplex_sp_downsample.size() %d\n", down_sample_factor_for_graph, vcplex_sp.size(), vcplex_sp_downsample.size() );


		complex_fft_displayable( vcplex_sp_downsample, vspect_displayble, 1 );


		int half = vspect_displayble.size() / 2;

		//vspect_displayble[ half + 100 ].ampl = 10.0f;

		//disp_spect_zoom_factor = 2.0f;

		if( disp_spect_zoom_factor != 0.0f )
			{
			int zoom_cnt = vspect_displayble.size() / disp_spect_zoom_factor;

			int half2 = zoom_cnt / 2;
			
			int start = half - half2; 
			int end = half + half2; 
			
			vector<st_spect_tag> vspect_disp_actual;

			for( int i = start; i < end; i++ )
				{
				st_spect_tag o;
				
				o = vspect_displayble[i];

				vspect_disp_actual.push_back( o );
				}
				
//			vspect_displayble = vspect_disp_actual;
//			vspect_gph = vspect_displayble;
			
			vspect_gph = vspect_disp_actual;
			}
		//	complex_rev_fft_multi( vcplex_sp, vtdm );									//go to time domain using a rev fft

		}



}
*/










//called by timer callback, preps data to update graticule graph and waterfall via 'update_gph0()'
int update_prep_gph0_cnt = 0;







void rtl_graph_wnd::update_prep_gph0()
{
//return;
bool vb = 0;


update_prep_gph0_cnt++;



if( pref_zero_padding_for_fft_cnt > cn_pref_zero_padding_for_fft_cnt_max )
	{
	printf("update_prep_gph0() - 'pref_zero_padding_for_fft_cnt' is %d and out of range, setting max of: %d\n", pref_zero_padding_for_fft_cnt, cn_pref_zero_padding_for_fft_cnt_max );
	pref_zero_padding_for_fft_cnt = cn_pref_zero_padding_for_fft_cnt_max;
	}

if( pref_zero_padding_for_fft_cnt < cn_pref_zero_padding_for_fft_cnt_min )
	{
	printf("update_prep_gph0() - 'pref_zero_padding_for_fft_cnt' is %d and out of range, setting min of: %d\n", pref_zero_padding_for_fft_cnt, cn_pref_zero_padding_for_fft_cnt_min );
	pref_zero_padding_for_fft_cnt = cn_pref_zero_padding_for_fft_cnt_min;
	}




if(b_dbg_skip_update_prep_gph0) 												//SKIP this function's code
	{
	if( !(update_prep_gph0_cnt % 10) ) vb = 1;
	if(vb) printf("update_prep_gph0() - 'b_dbg_skip_update_prep_gph0' is set SKIPPING this function ******************** \n" );
	update_prep_gph0_cnt++;
	return;
	}


//zero_pad_fft();

bool vb0 = 0;
if( !(update_prep_gph0_cnt % 10 ) ) vb0 = 1;


if( b_dbg_no_graph_update ) return;


//temp_code0_small_fft();

//if( !(update_prep_gph0_cnt%2) )
	{

	}
//else{
//	return;
//	}

update_prep_gph0_downsample();

//get_user_gui_control_params();

carrier_max_lev = 0.0f;

//----- mutex ----
//mutex3.lock();															//see 'demod_iso()' for other lock


//printf("update_prep_gph0() - bw_downsample %f vspect_gph.size() %d\n", vspect_gph.size() );





vector<st_spect_tag> vs;
//if( imode == en_mode_listen )
	{
	if( gph_loc_sel == 0 )
		{
if(vb) printf("update_prep_gph0() vcplex_gph %d\n", vcplex_gph.size() );
		for( int i = 0; i < vcplex_gph.size(); i++ )
			{
			st_spect_tag os;
			double freq_left = g_freq_tune - g_dev_bw/2.0;			//THESE MUST be double types, when tuned freq approaches 1GHz the spectra appear quantized on x-axis

			os.freq = freq_left + g_dev_bw * ( (double)i / vcplex_gph.size() );
			os.ampl = sqrt( ( vcplex_gph[i].real * vcplex_gph[i].real ) +  ( vcplex_gph[i].imag * vcplex_gph[i].imag ) );	 //sinf( pi2 * ((float)i/vcplex_gph.size()) );

			vs.push_back( os );
			}
		}




	//load fft spect for plotting on graticle graph, update waterfall
	if( gph_loc_sel == 1 )
		{

		if( (vspect_gph.size()%2) == 0 )								//keep odd so center of graph has a sample point.
			{

//			st_spect_tag osp = {0,0};
//			vspect_gph.push_back( osp );
			}



if(vb) printf("update_prep_gph0() vspect_gph %d\n", vspect_gph.size() );



		double bw_downsample = g_dev_bw / 1;//(float)down_sample_factor_for_graph;

		double bw_zoomed = g_dev_bw / disp_spect_zoom_factor;

		double freq_left = g_freq_tune - bw_zoomed / 2.0;		//THESE MUST be double types, else when tuned freq approaches 1GHz the spectra appear quantized on x-axis


		//shift the spectrum for display by an amount to conceal any tuning freq offsets (such as 'g_tuning_offset')
		float bin_freq = bw_zoomed / (float)vspect_gph.size();
		float freq_offs = g_freq_offsets_only;
		int shift = freq_offs / bin_freq;
//		spectrum_shift( shift, vspect_gph, 1 );

//printf("update_prep_gph0() - g_dev_bw %f  bw_downsample %f  down_sample_factor_for_graph %d freq_left %f\n", g_dev_bw, bw_downsample, down_sample_factor_for_graph, freq_left );

//printf("update_prep_gph0() - bw_downsample %f vspect_gph.size() %d freq_offs %f bin_freq %f, shift %d\n", bw_downsample, vspect_gph.size(), freq_offs, bin_freq, shift );

//spectrum_shift_complex( shift, vcplex_sp );									//move spectrum back to zero to counteract intermediate freq offset tuning


		for( int i = 0; i < vspect_gph.size(); i++ )	//'vspect_gph' only holds visible spectra on gph0 if 'disp_spect_zoom_factor' is > 1.0, some spectra were skipped in 'update_prep_gph0_downsample()' while building 'vspect_gph'
			{
			st_spect_tag os;

			os.freq = vspect_gph[i].freq;
			os.freq_actual = vspect_gph[i].freq_actual;
			os.ampl = vspect_gph[i].ampl;	 //sinf( pi2 * ((float)i/vcplex_gph.size()) );

//			if( i == 0 ) os.ampl = 0.1;

//			if( i == 65 ) os.ampl = 0.1;
//else  os.ampl = 0.05;
//if( i == 0 ) printf("update_prep_gph0() - os.freq %f\n", os.freq );
//			if( os.ampl > carrier_max_lev ) carrier_max_lev = os.ampl;
			
			vs.push_back( os );
			}

//		wfall0->set_freq_details( g_freq_tune, g_dev_bw, g_interfreq, disp_spect_zoom_factor );

//	printf("update_prep_gph0() -  graph read end\n" );
		}
	}




//printf("update_prep_gph0() - bw_downsample %f vspect_gph.size() %d  vs.size() %d\n", vspect_gph.size(), vs.size() );



//if( vs.size() == 8191 ) update_graph( vs );
if( vs.size() != 0 ) 
	{
//	vs[ vs.size()/2 ].ampl = 0.5;

	update_gph0( vs );


	if( b_wfall_enable )
		{
		wfall_rate_cnt += wfall_rate;

		bool b_wfall_add = 0;
		if( wfall_rate_cnt >= 1.0 )
			{
			wfall_rate_cnt = 0;
			b_wfall_add = 1;
			}
 
		if( b_wfall_add ) wfall0->add_line( vs );
		}
	}
 
vcplex_gph.clear();
vspect_gph.clear();
//	printf("update_prep_gph0() -  graph clear1.1\n" );

//mutex3.unlock();
//-----------------

//	if( imode == en_mode_listen ) rtl_get_graph();
//	wnd_rtl_graph->update();

}













/*
//called by timer callback, preps data to update graticule graph and waterfall via 'update_gph0()'
int update_prep_gph0_cnt = 0;

void rtl_graph_wnd::update_prep_gph0()
{
//return;
bool vb = 0;





int idx = bf1_prep_idx_atm.load(std::memory_order_acquire);				//this ensures thread safe access

if( idx != -1 )
	{
	int cnt = bf1_cnt_atm.load( std::memory_order_relaxed );	

	printf("update_prep_gph0() - HHHHHHHHHHHHHHHHHHHHHH idx %d cnt %d  iq %f \n", idx, cnt, bf1_iq[idx][0].real() );

	for ( int i = 0; i < cnt; i++ )
		{
		bf1_iq[idx][i];
		}
		
	bf1_prep_idx_atm.store(-1, std::memory_order_release);				//flag buf processed
	}






if(b_dbg_skip_update_prep_gph0) 												//SKIP this function's code
	{
	if( !(update_prep_gph0_cnt % 10) ) vb = 1;
	if(vb) printf("update_prep_gph0() - 'b_dbg_skip_update_prep_gph0' is set SKIPPING this function ******************** \n" );
	update_prep_gph0_cnt++;
	return;
	}


//zero_pad_fft();

bool vb0 = 0;
if( !(update_prep_gph0_cnt % 10 ) ) vb0 = 1;

//get_user_gui_control_params();

carrier_max_lev = 0.0f;

//----- mutex ----
mutex3.lock();															//see 'demod_iso()' for other lock


//printf("update_prep_gph0() - bw_downsample %f vspect_gph.size() %d\n", vspect_gph.size() );


//printf("update_prep_gph0() vcplex_gph %d\n", vcplex_gph.size() );


vector<st_spect_tag> vs;
//if( imode == en_mode_listen )
	{
	if( gph_loc_sel == 0 )
		{
		for( int i = 0; i < vcplex_gph.size(); i++ )
			{
			st_spect_tag os;
			
			double freq_left = g_freq_tune - g_dev_bw/2;			//THESE MUST be double types as when tuned freq approaches 1GHz the spectra appear quantized on x-axis
			
			os.freq = freq_left + g_dev_bw * ( (double)i / vcplex_gph.size() );
			os.ampl = sqrt( ( vcplex_gph[i].real * vcplex_gph[i].real ) +  ( vcplex_gph[i].imag * vcplex_gph[i].imag ) );	 //sinf( pi2 * ((float)i/vcplex_gph.size()) );

			vs.push_back( os );
			}
		}


	if( gph_loc_sel == 1 )
		{
		float bw_downsample = g_dev_bw / (float)down_sample_factor_for_graph;
		
		int bw_zoomed = g_dev_bw / disp_spect_zoom_factor;
		
		double freq_left = g_freq_tune - bw_zoomed / 2;				//THESE MUST be double types as when tuned freq approaches 1GHz the spectra appear quantized on x-axis


		//shift the spectrum for display by an amount to conceal any tuning freq offsets (such as 'g_tuning_offset')
		float bin_freq = bw_zoomed / (float)vspect_gph.size();
		float freq_offs = g_freq_offsets_only;
		int shift = freq_offs / bin_freq;
		spectrum_shift( shift, vspect_gph, 1 );

//	printf("update_prep_gph0() - bw_downsample %f vspect_gph.size() %d freq_offs %f bin_freq %f, shift %d\n", bw_downsample, vspect_gph.size(), freq_offs, bin_freq, shift );
	
//spectrum_shift_complex( shift, vcplex_sp );									//move spectrum back to zero to counteract intermediate freq offset tuning
		 
		for( int i = 0; i < vspect_gph.size(); i++ )
			{
			st_spect_tag os;


			
			
			os.freq = freq_left + bw_zoomed * ( (double)i / vspect_gph.size() );			//THESE MUST be double types as when tuned freq approaches 1GHz the spectra appear quantized on x-axis
			os.ampl = vspect_gph[i].ampl;	 //sinf( pi2 * ((float)i/vcplex_gph.size()) );

//			if( os.ampl > carrier_max_lev ) carrier_max_lev = os.ampl;
			
			vs.push_back( os );
			}

		wfall0->set_freq_details( g_freq_tune_offset, g_dev_bw, g_interfreq, disp_spect_zoom_factor );
		
//	printf("update_prep_gph0() -  graph read end\n" );
		}
	}




//printf("update_prep_gph0() - bw_downsample %f vspect_gph.size() %d  vs.size() %d\n", vspect_gph.size(), vs.size() );



//if( vs.size() == 8191 ) update_graph( vs );
if( vs.size() != 0 ) 
	{
	update_gph0( vs );


	if( b_wfall_enable )
		{
		wfall_rate_cnt += wfall_rate;

		bool b_wfall_add = 0;
		if( wfall_rate_cnt >= 1.0 )
			{
			wfall_rate_cnt = 0;
			b_wfall_add = 1;
			}
 
		if( b_wfall_add ) wfall0->add_line( vs );
		}
	}
 
vcplex_gph.clear();
vspect_gph.clear();
//	printf("update_prep_gph0() -  graph clear1.1\n" );

mutex3.unlock();
//-----------------

//	if( imode == en_mode_listen ) rtl_get_graph();
//	wnd_rtl_graph->update();

update_prep_gph0_cnt++;
}

*/
















void rtl_graph_wnd::graph_adjust_graticule()
{


//int graticle_count_y = 22;					//this should be an even number
//int graticle_count_x = 38;					//this should be an even number
int grat_pixels_x = 25;							//pxls per graticule
int grat_pixels_y = 25;

int ii = gph0->w();
int graticle_count_x = ii / grat_pixels_x;

ii = gph0->h();
int graticle_count_y = ii / grat_pixels_y;

int border = 2;

int wfm_wid = graticle_count_x * grat_pixels_x; //calc size of wnd using graticule details
wfm_wid += 2 * border;

int wfm_hei = graticle_count_y * grat_pixels_y;
wfm_hei += 2 * border;

gph0->bkgd_border_left = border;
gph0->bkgd_border_top = border;
gph0->bkgd_border_right = border;
gph0->bkgd_border_bottom = border;

gph0->graticule_border_left = border;
gph0->graticule_border_top = border;
gph0->graticule_border_right = border;
gph0->graticule_border_bottom = border;

gph0->graticle_count_x = graticle_count_x;
gph0->grat_pixels_x = grat_pixels_x;

gph0->graticle_count_y = graticle_count_y;
gph0->grat_pixels_y = grat_pixels_y;
gph0->cro_graticle = 1;
}








//do some culling of multiple carriers at similar freqs
void rtl_graph_wnd::carrier_max_sanitize( vector<st_carrier_max_tag> &vv )
{

vector<st_carrier_max_tag> vs;

if( vv.size() == 0 ) return;

int last_freq = 0;
for( int i = 0; i < vv.size(); i++ )
	{
	st_carrier_max_tag o;
	
	o = vv[i];

	int freq_low = last_freq - i_carrier_max_freq_dead_band/2;
	int freq_high = last_freq + i_carrier_max_freq_dead_band/2;
	
	if( ( o.freq < freq_low ) || ( o.freq > freq_high ) )
		{
		vs.push_back( o );
		last_freq = o.freq;
		}
	}


printf( "rtl_graph_wnd::carrier_max_sanitize() - before %d after %d\n", vv.size(), vs.size() );

vv = vs;
}







//'shift_left_px'  or 'shift_right_px' allows moving the of horiz edges (in pixels) and consequently a move in calc'd  'freq_left'  or 'freq_right'
void rtl_graph_wnd::gph0_edge_freqs( int shift_left_px, int shift_right_px, double &freq_left, double &freq_right )
{
bool vb = 0;


int ww, hh;
wnd_rtl_graph->gph0->get_background_dimensions( ww, hh );


//wnd_rtl_graph->gph0->get_mouse_pixel_position_on_background( px, py );

int py = 0;

double my;
if( wnd_rtl_graph->gph0->get_pixel_position_as_trc_values( 0, 0 + shift_left_px, py, freq_left, my, 1, 1 ) )
	{
	}


if( wnd_rtl_graph->gph0->get_pixel_position_as_trc_values( 0, ww + shift_right_px, py, freq_right, my, 1, 1 ) )
	{
	}

if(vb)printf( "rtl_graph_wnd::gph0_edge_freq() - freq_left %f, freq_left %f\n", freq_left, freq_left  );
}









//calc string pixel dimensions
//returns width
int rtl_graph_wnd::text_dim( string ss, int fonttype, int fontsize, int &height, int &descent )
{
int iF = fl_font();
int iS = fl_size();
 
fl_font( fonttype, fontsize );
descent = fl_descent();

int width = 0;
fl_measure( ss.c_str(), width, height );

fl_font( iF, iS );

return width;
}








int gph0_last_sel_idx = 0;



//this is the graph with graticule (not a 'fast_mgraph' obj), it's called from 'update_prep_gph0()'
void rtl_graph_wnd::update_gph0( vector<st_spect_tag> &vspct )
{
bool vb = 0;



if(b_dbg_skip_update_gph0) 												//SKIP this function's code
	{
	if( !(update_prep_gph0_cnt % 10) ) vb = 1;
	if(vb) printf("update_gph0() - 'b_dbg_skip_update_gph0' is set SKIPPING this function ******************** \n" );
	update_gph0_cnt++;
	return;
	}




//printf( "rtl_graph_wnd::update_gph0()\n" );

graph_adjust_graticule();

mg_col_tag col;
trace_tag tr1;



col.r = col_gph0_trace_col_r;
col.g = col_gph0_trace_col_g;
col.b = col_gph0_trace_col_b;



tr1.id = 0;                      //identify which trace this is, helps when traces are push_back'd in unknown order
tr1.vis = 1;
tr1.col = col;
tr1.line_thick = 1;
tr1.lineplot = 1;
tr1.line_style = (en_mgraph_line_style) en_mls_solid;
tr1.border_left = 0;
tr1.border_right = 0;
tr1.border_top = 0;
tr1.border_bottom = 0;

tr1.show_as_spectrum = 0;                           //not a spectra plot
tr1.spectrum_baseline_y = 0;

tr1.b_limit_auto_scale_min_for_y = 0;
tr1.b_limit_auto_scale_max_for_y = 0;

int graticle_count_y = gph0->graticle_count_y;					//this should be an even number
int graticle_count_x = gph0->graticle_count_x;					//this should be an even number
int grat_pixels_x = gph0->grat_pixels_x;						//pxls per graticule
int grat_pixels_y = gph0->grat_pixels_y;

tr1.xunits_perpxl = -1;//1.0/grat_pixels_x;
tr1.yunits_perpxl = 0.01;//0.1 / grat_pixels_y;


tr1.posx = 0;
tr1.posy = -100+5;

tr1.plot_offsx = 0; 								//not affected by override: 'use_pos_y', still allows independent trace offsets at pixel level
tr1.plot_offsy = 0;

tr1.use_pos_y = -1;				//if not -1, use this trace's id as a reference for this val
tr1.use_scale_y = -1;			//if not -1, use this trace's id as a reference for this val


tr1.scalex = 1;
tr1.scaley = gph_scaley;

tr1.single_sel_col.r = 255;
tr1.single_sel_col.g = 0;
tr1.single_sel_col.b = 255;

tr1.group_sel_trace_col.r = 230;
tr1.group_sel_trace_col.g = 230;
tr1.group_sel_trace_col.b = 230;

tr1.group_sel_rect_col.r = 255;
tr1.group_sel_rect_col.g = 153;
tr1.group_sel_rect_col.b = 0;

tr1.sample_rect_hints_double_click = 1;

tr1.sample_rect_hints = 1;
tr1.sample_rect_hints_col.r = 255;
tr1.sample_rect_hints_col.g = 120;
tr1.sample_rect_hints_col.b = 150;
tr1.sample_rect_hints_distancex = 12;						//helps stop over hinting on x-axis
tr1.sample_rect_hints_distancey = 0;						//disable over hinting test/clearing for y-axis 

tr1.clip_left = tr1.border_left;
tr1.clip_right = tr1.border_right;
tr1.clip_top = tr1.border_top;
tr1.clip_bottom = tr1.border_bottom;


tr1.sample_rect_flicker = 0;

//tr1.minx = 0;

pnt_tag pnt1;
//int count = 700;



vcarrier_max.clear();

int wr = ispec_avg_idx_wr;
int rd = ispec_avg_idx_rd;


int spect_center_low = vspct.size()/2-5; 	// -5, a fudge needed to get tuned freq's level
int spect_center_high = vspct.size()/2+5;

carrier_signal_level = -1.0f;


//printf("update_gph0() vspct %d\n", vspct.size() );


for( int i = 0; i <	vspct.size(); i++ )
	{
	if( i >= cn_vspec_avg_size_max ) break;

//printf("here\n" );	
		
//	b_spect_average = 0;
	if( b_spect_average )
		{
		pnt1.x = vspct[ i ].freq_actual;
		pnt1.y = 0;
		pnt1.sel = 0;
//----
		int k = rd;
		for( int j = 0; j <	ispec_avg_wnd; j++ )
			{
			pnt1.y += vspect_avg[k][i].ampl;							//sum for avg calc

			k--;
			if( k < 0 ) k = cn_spec_avg_slots_max - 1;
			}
			
		pnt1.y /= ispec_avg_wnd;										//complete avg calc
//----

		//use a band of spectra around center of spectral plot to derive tuned carrier's level
		if( ( i >= spect_center_low ) && ( i <= spect_center_high ) )
			{
			if( pnt1.y > carrier_signal_level ) carrier_signal_level = pnt1.y;		//keep the highest carrier found
			
//			printf("spect_center %f\n", pnt1.x );
			}

		if( pnt1.y > carrier_max_lev ) 
			{
			carrier_max_lev = pnt1.y;
			carrier_max_freq = pnt1.x;
			
			if( carrier_max_lev >= g_carrier_max_tune_threshold )
				{
				st_carrier_max_tag o;
				o.lev = carrier_max_lev;
				o.freq = carrier_max_freq;
				o.graph_x_idx = i;
				vcarrier_max.push_back( o );
				}
			
//			carrier_max_tune_timer = 0;
			}
		}
	else{
		pnt1.x = vspct[ i ].freq_actual;
		pnt1.y = vspct[ i ].ampl;
		pnt1.sel = 0;
		}

	vspect_avg[wr][i] = vspct[ i ];										//store for avg purposes
	
//printf("plotting %d\n", i );
	tr1.pnt.push_back( pnt1 );											//load graph points
	}


carrier_max_tune();




ispec_avg_idx_wr++;
if( ispec_avg_idx_wr >= cn_spec_avg_slots_max ) ispec_avg_idx_wr = 0;

ispec_avg_idx_rd++;
if( ispec_avg_idx_rd >= cn_spec_avg_slots_max ) ispec_avg_idx_rd = 0;



//printf("update_graph() - vspct.size() %d\n", vspct.size() );

/*
int count = 700;
for( int i = 0; i <	count; i++ )
	{
	pnt1.x = i;

	double theta = 2.0 * M_PI * (double)i / (double)count;

	pnt1.y = 250 * sin( theta );

	tr1.pnt.push_back( pnt1 );						//load graph points
	}
*/



gph0->get_selected_idx( 0, gph0_last_sel_idx );

gph0->clear_traces();
gph0->add_trace( tr1 );
gph0->set_left_click_anywhere_cb( cb_graph_left_click_anywhere_cb, (void*)this );
gph0->set_middle_click_anywhere_cb( cb_graph_middle_click_anywhere_cb, (void*)this );
gph0->set_right_click_anywhere_cb( cb_graph_right_click_anywhere_cb, (void*)this );

gph0->set_left_click_release_cb( cb_graph_left_click_release_cb, (void*)this );
gph0->set_right_click_release_cb( cb_graph_right_click_release_cb, (void*)this );

gph0->set_mousemove_cb( 0, cb_graph_mousemove, this );
gph0->set_mousewheel_cb( 0, cb_graph_mousewheel, this );
gph0->set_keydown_cb( 0, cb_graph_keydown, this );
gph0->set_keyup_cb( 0, cb_graph_keyup, this );

col.r = 64;
col.g = 64;
col.b = 64;
gph0->background = col;


gph0->set_selected_sample( 0, gph0_last_sel_idx, 0 );

update_gph0_user_obj();

//gph0->sample_rect_showing[0] = 1;

gph0->render( -1 );

if( start_up_state == 4 ) update_gph0_cnt++;

double x0,y0,x1,y1;
//gph0->get_trace_min_max( 0, x0,x1,y0,y1);

//printf( "update_gph0() - x0, x1 %f %f   y0 y1 %f %f\n",  x0,x1,y0,y1 );






//------------------------
if( i_need_pin_offscreen_check > 0 )
	{
	i_need_pin_offscreen_check--;

	if( i_need_pin_offscreen_check == 0 )
		{
		int idx = pin_selected();
		if( idx != -1 )
			{
			double freq_left, freq_right;
			wnd_rtl_graph->gph0_edge_freqs( 0, 0, freq_left, freq_right );


			int fr_a = vpin[idx].freq_actual;
			int fr_obt = vpin[idx].freq_tune;
			int fr_sub = vpin[idx].freq_sub_tune;

			
			int delta = freq_right - freq_left;

			
			delta *= 0.015f;													//come inwards from edges

		printf( "rtl_graph_wnd::tick() - NNNNNNNNNNNNNNNNNN idx %d  delta %d freq_left %f %f   freq_actual %d\n", idx, delta, freq_left + delta, freq_right - delta, fr_a );
			if( ( fr_a < ( freq_left + delta ) ) || ( fr_a > ( freq_right - delta ) ) )
				{
		printf( "rtl_graph_wnd::tick() - NNNNNNNNNNNNNNNNNN centered\n" );
				wnd_rtl_graph->centre_graph( 1, -1 );
				}
			}
		}
	}
//------------------------
}















void rtl_graph_wnd::carrier_max_tune()
{
if( !b_carrier_max_tune ) return;
	

if( vcarrier_max.size() == 0 ) return;


if( carrier_max_tune_timer < carrier_max_tune_timer_delay ) return;
	
			
carrier_max_sanitize( vcarrier_max );
printf("rtl_graph_wnd::carrier_max_tune() - vcarrier_max.size() %d\n", vcarrier_max.size() );


//find if current tuned freq is in carrier list
for( int i = 0; i < vcarrier_max.size(); i++ )
	{
	st_carrier_max_tag o;
	
	o = vcarrier_max[i];
	
	int dead_band = 50e3;
	int freq_low = g_freq_tune - dead_band/2;
	int freq_high = g_freq_tune + dead_band/2;
	
	if( ( o.freq >= freq_low ) && ( o.freq <= freq_high ) )
		{
		printf( "rtl_graph_wnd::carrier_max_tune() - already tuned to a freq in max carrier list, checking if its in exclusion list\n" );
		if( carrier_max_exclusion_find_idx(  o.freq ) >= 0 )
			{
			goto in_exclusion;	
			}
		return;
		}
	}

in_exclusion:

//find max carrier in list
int max_carrier_idx = -1;
int max_carrier_lev = 0;
for( int i = 0; i < vcarrier_max.size(); i++ )
	{
	st_carrier_max_tag o;
	
	o = vcarrier_max[i];
	
	if( o.lev > max_carrier_lev )
		{
		//not in exclusion list ?
		if( carrier_max_exclusion_find_idx(  vcarrier_max[i].freq ) < 0 )
			{
			max_carrier_idx = i;
			max_carrier_lev = o.lev;
			}
		}
	}

if( max_carrier_idx >= 0 )
	{
	printf( "rtl_graph_wnd::carrier_max_tune() - tuning using max carrier list idx %d, lev %f, freq %d\n", max_carrier_idx, vcarrier_max[max_carrier_idx].lev, vcarrier_max[max_carrier_idx].freq );

	
	carrier_max_led_flash_cnt = 30;
	
	//tune freq
	wnd_rtl_graph->miwp_tune->miw->set_value_from_double( vcarrier_max[max_carrier_idx].freq );
//	cb_bt_freq_tune( 0, 0 );
	wnd_rtl_graph->freq_listen( 1, 0, 0, 0, "carrier_max_tune()" );
	}
}










//returns idx if in list, else -1
int rtl_graph_wnd::carrier_max_exclusion_find_idx( int freq )
{
printf( "rtl_graph_wnd::carrier_max_exclusion_find() - freq %d\n", freq );

//find if 'freq' in 'vcarrier_max_exclusion'
st_carrier_max_tag o;
for( int i = 0; i < vcarrier_max_exclusion.size(); i++ )
	{
	
	o = vcarrier_max_exclusion[i];
	
	int dead_band = i_carrier_max_freq_dead_band;

	int freq_low = o.freq - dead_band/2;
	int freq_high = o.freq + dead_band/2;
	
	if( ( freq >= freq_low ) && ( freq <= freq_high ) )
		{
		printf( "rtl_graph_wnd::carrier_max_exclusion_find() - freq %d found list, idx %d\n", freq, i );

		return i;
		}
	}
return -1;
}









void rtl_graph_wnd::carrier_max_exclusion( bool b_add, int freq )
{
printf( "rtl_graph_wnd::vcarrier_max_exclusion() - freq %d\n", freq );



//find if 'freq' already in 'vcarrier_max_exclusion'
st_carrier_max_tag o;
for( int i = 0; i < vcarrier_max_exclusion.size(); i++ )
	{
	
	o = vcarrier_max_exclusion[i];
	
	int dead_band = i_carrier_max_freq_dead_band;

	int freq_low = o.freq - dead_band/2;
	int freq_high = o.freq + dead_band/2;
	
	if( ( freq >= freq_low ) && ( freq <= freq_high ) )
		{
		if( b_add )
			{
			printf( "rtl_graph_wnd::vcarrier_max_exclusion() - freq %d: already in list, won't add\n", freq );
			return;
			}
		else{
			printf( "rtl_graph_wnd::vcarrier_max_exclusion() - freq %d: found in list, removing\n", freq );
			vcarrier_max_exclusion.erase(  vcarrier_max_exclusion.begin() + i );
			return;
			}
		}
	}

if( b_add )
	{
	printf( "rtl_graph_wnd::vcarrier_max_exclusion() - freq %d: adding to list\n", freq );
	o.freq = freq;
	o.lev = 0.0f;
	vcarrier_max_exclusion.push_back( o );
	return;
	}
}










void rtl_graph_wnd::update_tune_history()
{
//printf("rtl_graph_wnd::update_tune_history() - !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1\n" );
	
idb_tune->clear();
for( int i = 0; i < vtunehist2.size(); i++ )
	{
	idb_tune->add( vtunehist2[i]);
	}
}









int flash0_loop = 8;
int flash0_cnt = 0;



void rtl_graph_wnd::update_controls()
{
string s1, s2;
double frq;
mystr m1;


bool flash_flag = 0;

flash0_cnt++;
if( flash0_cnt >= flash0_loop ) flash0_cnt = 0;
if( flash0_cnt > 4 ) flash_flag = 1;


int fractional_digits = 3;

string snum, sunits, scombined;


//---
s1 = wnd_rtl_graph->miwp_tune->miw->value();
if(s1.length() > 0 )
	{
	sscanf( s1.c_str(), "%lf", &frq );
	m1.make_engineering_str( snum, sunits, scombined, fractional_digits, frq, " ", "Hz" );

	strpf( s1, "%s", scombined.c_str() );
	}
else{
	s1 = "???";										//show unknown, refer 'i_skeyin_restore_cnt', where a user may have cleared keyin but not entered a new freq
	}


bx_tune->copy_label( s1.c_str() );
//---



int freq = rtl.get_center_freq();

strpf( s1, "dev: %d", freq );
miwp_tune->copy_label( s1.c_str() );




s1 = wnd_rtl_graph->fi_freq_start->value();
sscanf( s1.c_str(), "%lf", &frq );

m1.make_engineering_str( snum, sunits, scombined, fractional_digits, frq, " ", "Hz" );

strpf( s1, "%s", scombined.c_str() );
bx_freq_start->copy_label( s1.c_str() );





s1 = wnd_rtl_graph->fi_freq_stop->value();
sscanf( s1.c_str(), "%lf", &frq );

m1.make_engineering_str( snum, sunits, scombined, fractional_digits, frq, " ", "Hz" );

strpf( s1, "%s", scombined.c_str() );
bx_freq_stop->copy_label( s1.c_str() );



float fgain = rtl.get_gain();
strpf( s1, "%.2f dB", fgain );
bx_freq_gain->copy_label( s1.c_str() );




float frame_time = (1.0f/aud_op_srate)*framecnt;
 
strpf( s1, "         %d smpls/sec (%d smpls/frame)  framecnt: %d          ", gph_samples_per_sec, (int)(gph_samples_per_sec*frame_time), framecnt );
bx_samples_per_sec->copy_label( s1.c_str() );



//char name[256];
//char manufact[256];
//char product[256];
//char serial[256];
string ss_name;
string ss_manufact;
string ss_product;
string ss_serial;



//strncpy( name, rtl.get_device_name( ilast_device_index ), 256 );

//rtl.get_get_device_usb_strings( ilast_device_index, manufact, product, serial );

if( rtl.open_status() )
	{
//	strncpy( name, rtl.get_device_name( ilast_device_index ), 256 );

	ss_name = rtl.get_device_name();
	rtl.get_get_device_usb_strings( ss_manufact, ss_product, ss_serial );

	strpf( s1, "%s:  device --> '%s', Mfr: %s, Prod: %s, Serial: %s", cnsAppName, ss_name.c_str(), ss_manufact.c_str(), ss_product.c_str(), ss_serial.c_str() );
	copy_label( s1.c_str() );
	}
else{
	strpf( s1, "%s: No rtl device is open", cnsAppName );
	copy_label( s1.c_str() );
	}

int sr = rtl.get_srate();


m1.make_engineering_str( snum, sunits, scombined, fractional_digits, sr, " ", "Hz" );

strpf( s1, "%d Hz (%s)", sr, scombined.c_str() );
bx_dev_bwidth->copy_label( s1.c_str() );



//------ show a crude text graph of where buf rd/wr pointers are at -------


float fwnd = (uiwnd_high - uiwnd_low);
float ff0 = (idbg_rd - uiwnd_low) / fwnd;								//make a ratio 0.0--->1.0 of where 'idbg_rd' is relative to buf window, 0.5 would be in the middle of buf window

s2 = "..........";														//build a text graph
int idx = nearbyint( fabsf(ff0)*10.0 );	
if( idx < 0 ) idx = 0;
if( idx > 9 ) idx = 9;
s2[idx] = '|';															//show cur position in buf window


float buf_wnd = cn_rtl_buf_size/2;
float ff1 = 1.0f - (delta_wr_rd - buf_wnd/2 )/buf_wnd;							//make a ratio 0-->1.0 of where 'delta_wr_rd' is relative to buf window size, 0.5 would be in the middle of buf window




strpf( s1, "%s %06d[%.1f%%]  inc %.4f %.4f   %"PRIu64"  %"PRIu64"      ", s2.c_str(), (int)delta_wr_rd, ff1*100.0f, inc_rate, inc_rate2, idbg_rd, idbg_wr );
bx_buf_rd_wr->copy_label( s1.c_str() );

strpf( s1, "tops cpu %% : %s", s_tops_thread_stats.c_str() );
bx_tops_thread->copy_label( s1.c_str() );


//----------------------------------------------------------------------

fractional_digits = 0;
m1.make_engineering_str( snum, sunits, scombined, fractional_digits, downsample_srate, " ", "" );


strpf( s1, "Dwn: %d  (%s%sHz)", downsample_factor, snum.c_str(), sunits.c_str() );
bx_dwnconv_factor->copy_label( s1.c_str() );

int ii = rtl.get_direct_sampling();
strpf( s1, "DS: %d", ii );
bx_direct_sampling->copy_label( s1.c_str() );


ii = rtl.get_offset_tuning();
strpf( s1, "OT: %d", ii );
bx_offset_tuning->copy_label( s1.c_str() );



ii = rtl.get_ppm();
strpf( s1, "PPM: %d", ii );
bx_ppm_offset->copy_label( s1.c_str() );



//led brightness adj
int rr = 120 + (iled_buf_rd_adj / 1000.0f )*(255 - 120);
int gg = 80;

if( flash_flag ) if( iled_buf_rd_adj > 800 ) 							//flash led if a high value stays in 'iled_buf_rd_adj'
	{
	rr = 255;
	gg = 255;
	}

ld_buf_rd_adj->set_index_col_rgb( 0, rr, gg, 80 );
iled_buf_rd_adj -= 100;
if( iled_buf_rd_adj < 0 ) iled_buf_rd_adj = 0;

//if( ( iled_buf_rd_adj > 0 ) && ( !( iled_buf_rd_adj%3 ) ) )
//	{
//	ld_buf_rd_adj->resize( ld_buf_rd_adj->x(), ld_buf_rd_adj->y(), 15, 15 );	
//	}
//else{
//	ld_buf_rd_adj->resize( ld_buf_rd_adj->x(), ld_buf_rd_adj->y(), 13, 13 );	
//	}

fractional_digits = 3;
m1.make_engineering_str( snum, sunits, scombined, fractional_digits, carrier_max_freq, " ", "Hz" );

strpf( s1, "carrsig: %.2f  frq: %d (%s)", carrier_max_lev, carrier_max_freq, scombined.c_str() );
bx_carrier_lev->copy_label( s1.c_str() );



strpf( s1, "Fnd: %d", vcarrier_max.size() );
bx_carrier_max_tune_threshold_count->copy_label( s1.c_str() );


//strpf( s1, "SigLev: %.2f", carrier_signal_level );
//bx_signal_level->copy_label( s1.c_str() );




if( led_freq_mem_hov_idx >= 0 )
	{
	fractional_digits = 3;
	m1.make_engineering_str( snum, sunits, scombined, fractional_digits, freq_memory[  led_freq_mem_hov_idx  ].freq_tune + freq_memory[  led_freq_mem_hov_idx  ].freq_sub_tune, " ", "Hz" );
	
	strpf( s1, "%s", scombined.c_str() );
	bx_led_preset_hov_name->copy_label( s1.c_str() );	
	}


if( led_preset_hov_idx >= 0 )
	{
	fractional_digits = 3;
	m1.make_engineering_str( snum, sunits, scombined, fractional_digits, preset_memory[  led_preset_hov_idx  ].freq_tune, " ", "Hz" );
	
	strpf( s1, "%s  %s", scombined.c_str(), preset_memory[  led_preset_hov_idx  ].sname.c_str() );
	bx_led_preset_hov_name->copy_label( s1.c_str() );	
	}



if( led_preset_hov_idx2 >= 0 )
	{
	fractional_digits = 3;
	m1.make_engineering_str( snum, sunits, scombined, fractional_digits, preset_memory2[  led_preset_hov_idx2  ].freq_tune, " ", "Hz" );
	
	strpf( s1, "%s  %s", scombined.c_str(), preset_memory2[  led_preset_hov_idx2  ].sname.c_str() );
	bx_led_preset_hov_name->copy_label( s1.c_str() );	
	}


if( g_fm_19K_pilot_tone ) ld_fm_19k_pilot->ChangeCol( 1 );
else ld_fm_19k_pilot->ChangeCol( 0 );


ld_rec_play_synth->ChangeCol( 0 );
//ld_rec_play_synth->label( "     Rtl     " );
ld_rec_play_synth->label( "Rtl" );

if( wnd_rtl_graph->b_synth_iq )
	{
	if( flash_flag ) ld_rec_play_synth->ChangeCol( 3 );
	
//	ld_rec_play_synth->label( "         Synth   " );
	ld_rec_play_synth->label( "Synth" );
	}

if( ( rec_play_iq_state >= 3 ) && ( rec_play_iq_state <= 5 ) ) 
	{
	if( flash_flag ) ld_rec_play_synth->ChangeCol( 1 );

//	strpf( s1, "      Rec %d/%d    ", (int)rec_play_time, (int)rec_mode_end_secs );
	strpf( s1, "Rec %d/%d", (int)rec_play_time, (int)rec_mode_end_secs );
	ld_rec_play_synth->copy_label( s1.c_str() );

	Fl_Menu_Item *mn = menu_sdr->find_item( "&Device/&Record IQ" );		//menu string MUST MATCH defined '&Device/&Record IQ' menu definition, see 'menuitems'

	if( mn != 0 )
		{
		mn->set();
		}
	}
else{
	Fl_Menu_Item *mn = menu_sdr->find_item( "&Device/&Record IQ" );		//menu string MUST MATCH defined '&Device/&Record IQ' menu definition, see 'menuitems'

	if( mn != 0 )
		{
		mn->clear();
		}
	}



if( ( rec_play_iq_state >= 13 ) && ( rec_play_iq_state <= 15 ) )
	{
	if( flash_flag ) ld_rec_play_synth->ChangeCol( 2 );

//	strpf( s1, "     Play %d/%d    ", (int)rec_play_time, (int)play_mode_end_secs );
	strpf( s1, "Play %d/%d", (int)rec_play_time, (int)play_mode_end_secs );

	ld_rec_play_synth->copy_label( s1.c_str() );
	fvs_play_pos->value( rec_play_time );

	Fl_Menu_Item *mn = menu_sdr->find_item( "&Device/&Play IQ" );		//menu string MUST MATCH defined '&Device/&Play IQ' menu definition, see 'menuitems'

	if( mn != 0 )
		{
		mn->set();
		}
	}
else{
	Fl_Menu_Item *mn = menu_sdr->find_item( "&Device/&Play IQ" );		//menu string MUST MATCH defined '&Device/&Play IQ' menu definition, see 'menuitems'

	if( mn != 0 )
		{
		mn->clear();
		}
	}



Fl_Menu_Item *mi = wnd_rtl_graph->menu_sdr->find_item( "&Device/&Synth IQ samples (tune to 0Hz)" );		//menu string MUST MATCH defined '&Device/&Synth IQ samples (tune to 0Hz)' menu definition, see 'update_controls()'
if(mi)
	{
	if( wnd_rtl_graph->b_synth_iq ) mi->set();
	else mi->clear();
	}




if( wnd_rtl_graph->b_synth_iq )
	{
	}
else{
//	mn->clear();
	}



}




/*
//----------------------------------------------------------


mgraph::mgraph( int x, int y, int w, int h, const char *label ) : Fl_Box( x, y, w, h, label )
{

rect_size = 10 / 2;

woffx = x;
woffy = y;
double_click_left = 0;

bkgd_border_left = 0;
bkgd_border_top = 0;
bkgd_border_right = 0;
bkgd_border_bottom = 0;


graticule_border_right = 0;
graticule_border_top = 0;
graticule_border_right = 0;
graticule_border_bottom = 0;


cro_graticle = 0;

idx_maxy = -1;
idx_maxx = -1;

//printf(" mgraph w= %d, h= %d", w, h );
//exit(0);

//val_y_min = 0;
//val_y_max = 0;

//val_x_min = 0;
//val_x_max = 0;

//trce.clear();


}








void mgraph::clear_traces()
{
trce.clear();
//redraw();
}








void mgraph::clear_trace( int idx )
{
if( idx < 0 ) return;
if( idx >= trce.size() ) return;

trce.erase( trce.begin() + idx  );

//redraw();
}






//double trace_max = 0;

void mgraph::add_trace( trace_tag &tr )
{

if( tr.pnt.size() == 0 ) return;

tr.minx = tr.pnt[ 0 ].x;
tr.maxx = tr.pnt[ 0 ].x;

tr.miny = tr.pnt[ 0 ].y;
tr.maxy = tr.pnt[ 0 ].y;

idx_maxx = -1;
idx_maxy = -1;

for( int j = 0; j < tr.pnt.size(); j++ )		//work out mins,maxs
	{
	if( tr.pnt[ j ].x <  tr.minx )  tr.minx = tr.pnt[ j ].x;
	if( tr.pnt[ j ].x > tr.maxx )  { tr.maxx = tr.pnt[ j ].x; idx_maxx = j; }

	if( tr.pnt[ j ].y <  tr.miny ) tr.miny = tr.pnt[ j ].y;
	if( tr.pnt[ j ].y > tr.maxy ) { tr.maxy = tr.pnt[ j ].y; idx_maxy = j; }
	}

tr.left_click_cb_p_callback = 0;
tr.right_click_cb_p_callback = 0;
tr.selected_idx = -1;


trce.push_back( tr );
}









void mgraph::set_trace_visibility( int idx, bool visible )
{
if( idx >= trce.size() ) return;

trce[ idx ].vis = visible;

}








void mgraph::get_trace_maxx( int trc, int &idx, double &x )
{
idx = -1;

if( trc >= trce.size() ) return;

idx = idx_maxx;

double xmin, xmax, ymin, ymax;

get_trace_min_max( trc, xmin, xmax, ymin, ymax );

x = xmax;
}











void mgraph::get_trace_maxy( int trc, int &idx, double &y )
{
idx = -1;

if( trc >= trce.size() ) return;

idx = idx_maxy;

double xmin, xmax, ymin, ymax;

get_trace_min_max( trc, xmin, xmax, ymin, ymax );

y = ymax;
}











void mgraph::get_trace_min_max( int idx, double &xmin, double &xmax, double &ymin, double &ymax )
{
xmin = 0;
xmax = 0;
ymin = 0;
ymax = 0;

if( idx >= trce.size() ) return;

xmin = trce[ idx ].minx;
xmax = trce[ idx ].maxx;
ymin = trce[ idx ].miny;
ymax = trce[ idx ].maxy;

//cslpf( 5, "trace freq min: %lf, max: %lf\n", xmin, xmax );
}





void mgraph::get_selected_value( int trc, double &xx, double &yy )
{
xx = yy = 0;

if ( trce.size() == 0 ) return;


if ( trc < 0 ) return;
if( trc >= trce.size() ) return;

int idx = trce[ trc ].selected_idx;

if ( idx == -1 ) return;

xx = trce[ trc ].pnt[ idx ].x;
yy = trce[ trc ].pnt[ idx ].y;
}









bool mgraph::get_selected_idx( int trc, int &sel_idx )
{
if ( trce.size() == 0 ) return 0;

if ( trc < 0 ) return 0;
if( trc >= trce.size() ) return 0;

sel_idx = trce[ trc ].selected_idx;
if( sel_idx == -1 ) return 0;

return 1;
}










//will call a callback if one is set, which callback depends on value of issue_which_callback,
//issue_which_callback == 1, will call: left_click_cb_p_callback(..)
//issue_which_callback == 2, will call: right_click_cb_p_callback(..)
void mgraph::set_selected_sample( int trc, int sel_sample_idx, int issue_which_callback )
{
if ( trce.size() == 0 ) return;


if ( trc < 0 ) return;
if( trc >= trce.size() ) return;

if( sel_sample_idx < 0 ) return;
if( sel_sample_idx >= trce[ trc ].pnt.size() ) return;


int i = trc;

int wid = w();
int hei = h();


int bkgd_reductionx = bkgd_border_right + bkgd_border_left;
int bkgd_reductiony = bkgd_border_bottom + bkgd_border_top;

int bkgd_offx = woffx + bkgd_border_left;
int bkgd_offy = woffy + bkgd_border_top;

int bkgd_wid = wid - bkgd_reductionx;
int bkgd_hei = hei - bkgd_reductiony;

int bkgd_midx = bkgd_wid / 2;
int bkgd_midy = bkgd_hei / 2;


double x, y;



//calc trace sample point locations
int trace_offx;
int trace_offy;
int trace_wid;
int trace_hei;


int trace_reductionx = trce[ i ].border_right + trce[ i ].border_left;
int trace_reductiony = trce[ i ].border_bottom + trce[ i ].border_top;
trace_offx = woffx + trce[ i ].border_left;
trace_offy = woffy + trce[ i ].border_top;

trace_wid = wid - trace_reductionx;
trace_hei = hei - trace_reductiony;


double trace_midx = trace_wid / 2;
double trace_midy = trace_hei / 2;

int cutoffx = trace_wid;					//stop drawing if x gets to this


double deltax, deltay;
double scalex, scaley;
double d_midx, d_midy;


double positionx = trce[ i ].posx;
double positiony = -trce[ i ].posy;


deltax = trce[ i ].maxx - trce[ i ].minx;
deltay = trce[ i ].maxy - trce[ i ].miny;


if( trce[ i ].xunits_perpxl == -1 )			//use auto scaling, to span x raster
	{
	if( deltax != 0 )						//avoid poss div by zero
		{
		scalex = (double)trace_wid / deltax;
		d_midx = deltax / 2.0 + trce[ i ].minx;
		}
	else{									//no deltax
		scalex = grat_pixels_x;
		d_midx = 0;
		}
	}


if( trce[ i ].xunits_perpxl == -2 )		//use graticle unit/division, so 1.0 amounts to 1 div
	{
	if( deltax != 0 )						//avoid poss div by zero
		{
		scalex = grat_pixels_x;
		d_midx = 0;//deltax / 2.0 + trce[ i ].minx;
		}
	else{									//no deltax
		scalex = grat_pixels_x;
		d_midx = 0;
		}
	}


if( trce[ i ].xunits_perpxl > -1 )					//use user spec scale
	{
	scalex = 1.0 / trce[ i ].xunits_perpxl;			//scale as directed
	d_midx = (double)trace_midx / scalex;
	}





if( trce[ i ].yunits_perpxl == -1 )					//use auto scaling
	{
	if( deltay != 0 )							//avoid poss div by zero
		{
		scaley = (double)trace_hei / deltay;
		d_midy = deltay / 2.0 + trce[ i ].miny;
		}
	else{										//no deltay
		scaley = grat_pixels_y;
		d_midy = 0;
		}
	}


if( trce[ i ].yunits_perpxl == -2 )		//use graticle unit/division, so 1.0 amounts to 1 div
	{
	if( deltay != 0 )							//avoid poss div by zero
		{
		scaley = grat_pixels_y;
		d_midy = 0;//deltay / 2.0 + trce[ i ].miny;
		}
	else{										//no deltay
		scaley = grat_pixels_y;
		d_midy = 0;
		}
	}

if( trce[ i ].yunits_perpxl > -1 )					//use user spec scale
	{
	scaley = 1.0 / trce[ i ].yunits_perpxl;			//scale as directed
	d_midy = 0;
	}

if( i == 0 )										// 1st trace?
	{
	ref_scalex = scalex;							//remember x scale
	ref_d_midx = d_midx;
	}


if( i != 0 )										// not 1st trace?
	{
	scalex = ref_scalex;							//use 1st trace's x scale
	d_midx = ref_d_midx;
	}


int j = sel_sample_idx;

double x1 = trce[ i ].pnt[ j ].x;
double y1 = trce[ i ].pnt[ j ].y;

x = x1 * scalex - d_midx * scalex;
y = y1 * scaley - d_midy * scaley;


int xx = nearbyint( trace_midx + x + trace_offx + positionx );
int yy = nearbyint( trace_midy - y + trace_offy + positiony );

rect_size = 10 / 2;

int left =  xx - rect_size / 2;
int top =  yy - rect_size / 2;


int right =  left + rect_size;
int bot = top + rect_size;

trce[ i ].selected_idx = j;
trce[ i ].selected_pixel_rect_left = left;
trce[ i ].selected_pixel_rect_right = rect_size;
trce[ i ].selected_pixel_rect_top = top;
trce[ i ].selected_pixel_rect_bot = rect_size;

if( issue_which_callback != 0 )
	{
	if( issue_which_callback == 1 )
		{
		if( trce[ i ].left_click_cb_p_callback )  trce[ i ].left_click_cb_p_callback( trce[ i ].left_click_cb_args );
		}

	if( issue_which_callback == 2 )
		{
		if( trce[ i ].right_click_cb_p_callback )  trce[ i ].right_click_cb_p_callback( trce[ i ].right_click_cb_args );
		}
	}

redraw();
}




















void mgraph::render()
{
redraw();
}










void mgraph::draw()
{
Fl_Box::draw();


int wid = w();
int hei = h();




int bkgd_reductionx = bkgd_border_right + bkgd_border_left;
int bkgd_reductiony = bkgd_border_bottom + bkgd_border_top;

int bkgd_offx = woffx + bkgd_border_left;
int bkgd_offy = woffy + bkgd_border_top;

int bkgd_wid = wid - bkgd_reductionx;
int bkgd_hei = hei - bkgd_reductiony;

int bkgd_midx = bkgd_wid / 2;
int bkgd_midy = bkgd_hei / 2;


fl_color( 0, 0, 0 );
fl_rect( woffx, woffy, w(), h() );


//fl_push_clip( bkgd_offx, bkgd_offy, bkgd_wid, bkgd_hei );		//clip

fl_color( background.r, background.g, background.b );

fl_rectf( bkgd_offx, bkgd_offy, bkgd_wid, bkgd_hei );







//return;

//fl_rect( woffx, woffy, w() - woffx , h() - woffy + 3 );



double x, y;

//	fl_line( axisx + t * factorx , midy - y, axisx + oldx, midy - oldy );




//draw graticles
if( cro_graticle )
	{
	int graticule_reductionx = graticule_border_right + graticule_border_left;
	int graticule_reductiony = graticule_border_bottom + graticule_border_top;

	int grat_offx = woffx + graticule_border_left;
	int grat_offy = woffy + graticule_border_top;

	int grat_wid = wid - graticule_reductionx;
	int grat_hei = hei - graticule_reductiony;

	int grat_midx = grat_wid / 2;
	int grat_midy = grat_hei / 2;

	//draw 2 centre graticules
	int x1 = 0;
	int x2 = grat_wid;

	int y1 = grat_hei / 2;
	int y2 = y1;

	fl_color( 150, 150, 150 );
	fl_line( grat_offx + x1, grat_offy + y1, grat_offx + x2 , grat_offy + y2 );

	x1 = grat_wid / 2;
	x2 = x1;

	y1 = 0;
	y2 = grat_hei;
	fl_line( grat_offx + x1, grat_offy + y1, grat_offx + x2 , grat_offy + y2 );



//printf( "woffx= %d, raster_offx= %d, border_reductionx= %d\n", woffx, raster_offx, border_reductionx );
//printf( "w()= %d, h()= %d\n", w(), h() );
//printf( "wid= %d, hei= %d\n", wid, hei );


//printf( "trce[ 0 ].border_left= %d, trce[ 0 ].border_right= %d\n", trce[ 0 ].border_left, trce[ 0 ].border_right );



	int cnt = graticle_count_y / 2;

	fl_color( 90, 90, 90 );
	for( int i = 0; i < cnt - 1; i++ )						//horiz grats
		{
		
		x1 = 0;
		x2 = grat_wid;

		y1 = grat_pixels_y * ( i + 1 );
		y2 = y1;
		fl_line( grat_offx + x1, grat_offy + grat_midy - y1, grat_offx + x2 , grat_offy + grat_midy - y2 );

		fl_line( grat_offx + x1, grat_offy + grat_midy + y1, grat_offx + x2 , grat_offy + grat_midy + y2 );
		}

	cnt = graticle_count_x / 2;
	for( int i = 0; i < cnt - 1; i++ )						//vert grats
		{

		x1 = grat_pixels_x * ( i + 1 );
		x2 = x1;

		y1 = 0;
		y2 = grat_hei;
		fl_line( grat_offx + grat_midx - x1, grat_offy + y1, grat_offx + grat_midx - x2 , grat_offy + y2 );

		fl_line( grat_offx + grat_midx + x1, grat_offy + y1, grat_offx + grat_midx + x2 , grat_offy + y2 );
		}

	}





vector<int>vpx;								//for sample bounding rectangles
vector<int>vpy;


//draw traces
for( int i = 0; i < trce.size(); i++ )						//cycle each trace
	{
	bool lineplot = trce[ i ].lineplot; 

	int trace_offx;
	int trace_offy;
	int trace_wid;
	int trace_hei;


	int trace_reductionx = trce[ i ].border_right + trce[ i ].border_left;
	int trace_reductiony = trce[ i ].border_bottom + trce[ i ].border_top;
	trace_offx = woffx + trce[ i ].border_left;
	trace_offy = woffy + trce[ i ].border_top;

	trace_wid = wid - trace_reductionx;
	trace_hei = hei - trace_reductiony;

//	sizex = trace_wid;
//	sizey = trace_hei;

	double trace_midx = trace_wid / 2;
	double trace_midy = trace_hei / 2;

	int cutoffx = trace_wid;// - trce[ i ].border_right;		//stop drawing if x gets to this


	int style = FL_SOLID;

	if( trce[ i ].line_type == tlt_dash ) style = FL_DASH;
	if( trce[ i ].line_type == tlt_dot ) style = FL_DOT;
	if( trce[ i ].line_type == tlt_dashdot ) style = FL_DASHDOT;



	fl_line_style ( style, trce[ i ].line_thick );
//	fl_line_style ( style | FL_CAP_ROUND, 5 );
	fl_color( trce[ i ].col.r, trce[ i ].col.g, trce[ i ].col.b );

//	int border_reductionx = trce[ i ].border_right + trce[ i ].border_left;
//	int border_reductiony = trce[ i ].border_bottom + trce[ i ].border_top;


//	sizex = w() - woffx - border_reductionx;
//	sizey = h() - woffy - border_reductiony;



//	int midx = sizex / 2;
//	int midy = sizey / 2;

	double deltax, deltay;
	double scalex, scaley;
//	double offx, offy;
//	double axisx, axisy;
	double d_midx, d_midy;

//	int raster_offx = woffx + border_reductionx / 2 + ( trce[ i ].border_left - trce[ i ].border_right ) / 2;
//	int raster_offy = woffy + border_reductiony / 2 + ( trce[ i ].border_top - trce[ i ].border_bottom ) / 2;

//printf("traceminx= %lf, traceminy=%lf\n", trce[ i ].minx, trce[ i ].miny );
//printf("tracemaxx= %lf, tracemaxy=%lf\n", trce[ i ].maxx, trce[ i ].maxy );

//tr.border_left = 10;
//tr.border_right = 10;
//tr.border_top = 10;
//tr.border_bottom = 10;


//	offx = trce[ i ].minx;
//	offy = trce[ i ].miny;
	
//	int ref = i;
//	bool use_trace1 = 0;
//	if( use_trace1 ) ref = 0;

//		deltax = trce[ i ].maxx - trce[ i ].minx;
//		deltay = trce[ i ].y_perpxl;

	double positionx = trce[ i ].posx;
	double positiony = -trce[ i ].posy;


	deltax = trce[ i ].maxx - trce[ i ].minx;
	deltay = trce[ i ].maxy - trce[ i ].miny;


	if( trce[ i ].xunits_perpxl == -1 )			//use auto scaling, to span x raster
		{
		if( deltax != 0 )						//avoid poss div by zero
			{
			scalex = (double)trace_wid / deltax;
			d_midx = deltax / 2.0 + trce[ i ].minx;
			}
		else{									//no deltax
			scalex = grat_pixels_x;
			d_midx = 0;
			}
		}


	if( trce[ i ].xunits_perpxl == -2 )		//use graticle unit/division, so 1.0 amounts to 1 div
		{
		if( deltax != 0 )						//avoid poss div by zero
			{
			scalex = grat_pixels_x;
			d_midx = 0;//deltax / 2.0 + trce[ i ].minx;
			}
		else{									//no deltax
			scalex = grat_pixels_x;
			d_midx = 0;
			}
		}


	if( trce[ i ].xunits_perpxl > -1 )					//use user spec scale
		{
		scalex = 1.0 / trce[ i ].xunits_perpxl;			//scale as directed
		d_midx = (double)trace_midx / scalex;
		}





	if( trce[ i ].yunits_perpxl == -1 )					//use auto scaling
		{
		if( deltay != 0 )							//avoid poss div by zero
			{
			scaley = (double)trace_hei / deltay;
			d_midy = deltay / 2.0 + trce[ i ].miny;
			}
		else{										//no deltay
			scaley = grat_pixels_y;
			d_midy = 0;
			}
		}


	if( trce[ i ].yunits_perpxl == -2 )		//use graticle unit/division, so 1.0 amounts to 1 div
		{
		if( deltay != 0 )							//avoid poss div by zero
			{
			scaley = grat_pixels_y;
			d_midy = 0;//deltay / 2.0 + trce[ i ].miny;
			}
		else{										//no deltay
			scaley = grat_pixels_y;
			d_midy = 0;
			}
		}

	if( trce[ i ].yunits_perpxl > -1 )					//use user spec scale
		{
		scaley = 1.0 / trce[ i ].yunits_perpxl;			//scale as directed
		d_midy = 0;
		}

	if( i == 0 )										// 1st trace?
		{
		ref_scalex = scalex;							//remember x scale
		ref_d_midx = d_midx;
		}

//printf("deltay= %lf, scaley= %lf, d_midy= %lf\n", deltay, scaley, d_midy );
	
	double oldx = 0;
	double oldy = 0;


	if( i != 0 )										// not 1st trace?
		{
		scalex = ref_scalex;							//use 1st trace's x scale
		d_midx = ref_d_midx;
		}


	
	if( trce[ i ].vis )
		{
		for( int j = 0; j < trce[ i ].pnt.size(); j++ )			//cycle each coord point
			{
			double x1 = trce[ i ].pnt[ j ].x;
			double y1 = trce[ i ].pnt[ j ].y;

			x = x1 * scalex - d_midx * scalex;
			y = y1 * scaley - d_midy * scaley;

//printf("y1= %f, scaley= %f, d_midy= %f\n", y1, scaley, d_midy );

//y = y1 * unit_per_div_y - trace_midy * scaley;

			if( j == 0 )
				{
				oldx = x;
				oldy = y;
				continue;
				}
	//		fl_point( axisx + t * factorx , midy - y );


	//double chk1x = ( trce[ i ].minx ) * scalex - trace_midx * scalex;
	//double chk1y = ( trce[ i ].miny ) * scaley;
	//double chk2x = ( trce[ i ].maxx ) * scalex;
	//double chk2y = ( trce[ i ].maxy ) * scaley;

			int xx = nearbyint( trace_midx + x + trace_offx + positionx );
			int yy = nearbyint( trace_midy - y + trace_offy + positiony );

			int xxx = nearbyint( trace_midx + oldx + trace_offx + positionx );
			int yyy = nearbyint( trace_midy - oldy  + trace_offy + positiony );


//printf("trace_midy= %lf, trace_offy= %lf, positiony= %lf\n", trace_midy, trace_offy, positiony );

//			if( xxx > cutoffx ) break;					//don't go past right border

			if ( lineplot ) fl_line( xx, yy, xxx, yyy );
			else fl_point( xx, yy );
			oldx = x;
			oldy = y;


			if( double_click_left )						//show all sample point bounding rects
				{
				vpx.push_back( xx );					//store sample point pixel coord for later drawing of bounding rects
				vpy.push_back( yy );
				}

			}
		}
	}

fl_line_style( 0 );


if( double_click_left )						//show all sample point bounding rects
	{

	fl_color( 255, 0, 255 );

	for( int i = 0; i < vpx.size(); i++ )
		{
		int left = vpx[ i ] - rect_size / 2;
		int top = vpy[ i ] - rect_size / 2;

		int right =  left + rect_size;
		int bot = top + rect_size;

		fl_rect(  left , top, rect_size, rect_size );

		}
	}


for( int i = 0; i < trce.size(); i++ )						//cycle each trace
	{

	//show selected sample
	if( trce[ i ].selected_idx != -1 )
		{
		fl_color( 255, 0, 255 );
		fl_rect(  trce[ i ].selected_pixel_rect_left , trce[ i ].selected_pixel_rect_top, trce[ i ].selected_pixel_rect_right, trce[ i ].selected_pixel_rect_bot );
		}
	}

//fl_pop_clip();


//double xx, yy;
//get_mouse_vals( mousex, mousey, xx, yy, 1 );

//exit(0);

}










//callback to call when left mouse button is clicked
bool mgraph::set_left_click_cb( int trc, void (*p_cb)( void* ), void *args_in )
{
if ( trce.size() == 0 ) return 0;

if ( trc < 0 ) return 0;
if( trc >= trce.size() ) return 0;

trce[ trc ].left_click_cb_p_callback = p_cb;
trce[ trc ].left_click_cb_args = args_in;

return 1;
}











bool mgraph::get_mouse_vals( int trc, int mx, int my, double &xx, double &yy )
{
if ( trce.size() == 0 ) return 0;

if ( trc < 0 ) return 0;
if( trc >= trce.size() ) return 0;


int i = trc;

trce[ i ].selected_idx = -1;



int wid = w();
int hei = h();




int bkgd_reductionx = bkgd_border_right + bkgd_border_left;
int bkgd_reductiony = bkgd_border_bottom + bkgd_border_top;

int bkgd_offx = woffx + bkgd_border_left;
int bkgd_offy = woffy + bkgd_border_top;

int bkgd_wid = wid - bkgd_reductionx;
int bkgd_hei = hei - bkgd_reductiony;

int bkgd_midx = bkgd_wid / 2;
int bkgd_midy = bkgd_hei / 2;


double x, y;






//calc trace sample point locations

	bool lineplot = trce[ i ].lineplot; 

	int trace_offx;
	int trace_offy;
	int trace_wid;
	int trace_hei;


	int trace_reductionx = trce[ i ].border_right + trce[ i ].border_left;
	int trace_reductiony = trce[ i ].border_bottom + trce[ i ].border_top;
	trace_offx = woffx + trce[ i ].border_left;
	trace_offy = woffy + trce[ i ].border_top;

	trace_wid = wid - trace_reductionx;
	trace_hei = hei - trace_reductiony;


	double trace_midx = trace_wid / 2;
	double trace_midy = trace_hei / 2;

	int cutoffx = trace_wid;					//stop drawing if x gets to this


	double deltax, deltay;
	double scalex, scaley;
	double d_midx, d_midy;


	double positionx = trce[ i ].posx;
	double positiony = -trce[ i ].posy;


	deltax = trce[ i ].maxx - trce[ i ].minx;
	deltay = trce[ i ].maxy - trce[ i ].miny;


	if( trce[ i ].xunits_perpxl == -1 )			//use auto scaling, to span x raster
		{
		if( deltax != 0 )						//avoid poss div by zero
			{
			scalex = (double)trace_wid / deltax;
			d_midx = deltax / 2.0 + trce[ i ].minx;
			}
		else{									//no deltax
			scalex = grat_pixels_x;
			d_midx = 0;
			}
		}


	if( trce[ i ].xunits_perpxl == -2 )		//use graticle unit/division, so 1.0 amounts to 1 div
		{
		if( deltax != 0 )						//avoid poss div by zero
			{
			scalex = grat_pixels_x;
			d_midx = 0;//deltax / 2.0 + trce[ i ].minx;
			}
		else{									//no deltax
			scalex = grat_pixels_x;
			d_midx = 0;
			}
		}


	if( trce[ i ].xunits_perpxl > -1 )					//use user spec scale
		{
		scalex = 1.0 / trce[ i ].xunits_perpxl;			//scale as directed
		d_midx = (double)trace_midx / scalex;
		}





	if( trce[ i ].yunits_perpxl == -1 )					//use auto scaling
		{
		if( deltay != 0 )							//avoid poss div by zero
			{
			scaley = (double)trace_hei / deltay;
			d_midy = deltay / 2.0 + trce[ i ].miny;
			}
		else{										//no deltay
			scaley = grat_pixels_y;
			d_midy = 0;
			}
		}


	if( trce[ i ].yunits_perpxl == -2 )		//use graticle unit/division, so 1.0 amounts to 1 div
		{
		if( deltay != 0 )							//avoid poss div by zero
			{
			scaley = grat_pixels_y;
			d_midy = 0;//deltay / 2.0 + trce[ i ].miny;
			}
		else{										//no deltay
			scaley = grat_pixels_y;
			d_midy = 0;
			}
		}

	if( trce[ i ].yunits_perpxl > -1 )					//use user spec scale
		{
		scaley = 1.0 / trce[ i ].yunits_perpxl;			//scale as directed
		d_midy = 0;
		}

	if( i == 0 )										// 1st trace?
		{
		ref_scalex = scalex;							//remember x scale
		ref_d_midx = d_midx;
		}

	
	double oldx = 0;
	double oldy = 0;


	if( i != 0 )										// not 1st trace?
		{
		scalex = ref_scalex;							//use 1st trace's x scale
		d_midx = ref_d_midx;
		}


	
	if( trce[ i ].vis )
		{
		for( int j = 0; j < trce[ i ].pnt.size(); j++ )			//cycle each coord point
			{
			double x1 = trce[ i ].pnt[ j ].x;
			double y1 = trce[ i ].pnt[ j ].y;

			x = x1 * scalex - d_midx * scalex;
			y = y1 * scaley - d_midy * scaley;


			if( j == 0 )
				{
				oldx = x;
				oldy = y;
				continue;
				}
	//		fl_point( axisx + t * factorx , midy - y );


			int xx = nearbyint( trace_midx + x + trace_offx + positionx );
			int yy = nearbyint( trace_midy - y + trace_offy + positiony );

			int xxx = nearbyint( trace_midx + oldx + trace_offx + positionx );
			int yyy = nearbyint( trace_midy - oldy  + trace_offy + positiony );


//			if( xxx > cutoffx ) break;					//don't go past right border

			int left =  xx - rect_size / 2;
			int top =  yy - rect_size / 2;


			int right =  left + rect_size;
			int bot = top + rect_size;

//			if( double_click_left )
//				{
//				fl_color( 255, 0, 255 );
//				fl_rect(  left , top, rect_size, rect_size );
//				}

			//check if mouse with a sample point
			if( ( ( mx + woffx ) >= left ) && (( mx + woffx ) <=  right ) )
				{
				if( ( ( my + woffy )  >= top ) && ( ( my + woffy ) <=  bot ) )
					{
					
					trce[ i ].selected_idx = j;
//					fl_rect( left , top, rect_size, rect_size );

					trce[ i ].selected_pixel_rect_left = left;
					trce[ i ].selected_pixel_rect_right = rect_size;
					trce[ i ].selected_pixel_rect_top = top;
					trce[ i ].selected_pixel_rect_bot = rect_size;

					break;
					}
				}

			oldx = x;
			oldy = y;
			}
		}


int idx = trce[ i ].selected_idx;

if( trce[ i ].selected_idx != -1 )
	{
	xx = trce[ i ].pnt[ idx ].x;
	yy = trce[ i ].pnt[ idx ].y;

	printf( "pressed sample selected_idx= %d\n", trce[ i ].selected_idx );
	}
	

return 1;
}


















int mgraph::handle( int e )
{
bool need_redraw = 0;
bool dont_pass_on = 0;

double xx, yy;

if ( e == FL_PUSH )	
	{
	mousex = Fl::event_x() - woffx;
	mousey = Fl::event_y() - woffy;
	double_click_left = Fl::event_clicks();


	for( int i = 0; i < trce.size(); i++ )
		{
		if( get_mouse_vals( i, mousex, mousey, xx, yy ) )
			{
			if( trce[ i ].left_click_cb_p_callback ) trce[ i ].left_click_cb_p_callback( trce[ i ].left_click_cb_args );
			}
		}

	need_redraw = 1;
//	dont_pass_on = 0;		/allow this through to parent
	}

if ( need_redraw ) redraw();

if( dont_pass_on ) return 1;

return Fl_Widget::handle( e );
}


//----------------------------------------------------------
*/














/*
//audio card will call this when it has audio data to offer - audio data from mic/line i/p, from A/D
int cb_audio_process_in( unsigned int bufid, unsigned int frames, void *arg )
{
//printf("bufid=%d\n", (int)bufid );
inbuf1 = audio_card_get_inbuf( 0 );
inbuf2 = audio_card_get_inbuf( 1 );

if( inbuf1 ) audio_card_prepare_in( 0 );
if( inbuf2 ) audio_card_prepare_in( 1 );

}
*/













void filter1( vector <filter_code::st_cplex_tag> &vbuf,  vector <filter_code::st_cplex_tag> &vfilt )
{

filter_code::st_cplex_tag store[ 10 ];


store[ 0 ].real = 0;
store[ 0 ].imag = 0;

store[ 1 ].real = 0;
store[ 1 ].imag = 0;

store[ 2 ].real = 0;
store[ 2 ].imag = 0;

store[ 3 ].real = 0;
store[ 3 ].imag = 0;

store[ 4 ].real = 0;
store[ 4 ].imag = 0;

store[ 5 ].real = 0;
store[ 5 ].imag = 0;

filter_code::st_cplex_tag o;
for( int i = 0; i < vbuf.size(); i++ )
	{
	
	
	o.real = ( store[ 0 ].real + store[ 1 ].real + store[ 2 ].real + store[ 3 ].real + store[ 4 ].real + store[ 5 ].real ) / 5.0;
	o.imag = ( store[ 0 ].imag + store[ 1 ].imag + store[ 2 ].imag + store[ 3 ].imag + store[ 4 ].imag + store[ 5 ].imag ) / 5.0;
	
	vfilt.push_back( o );

	
	store[ 0 ] = store[ 1 ];
	store[ 1 ] = store[ 2 ];
	store[ 2 ] = store[ 3 ];
	store[ 3 ] = store[ 4 ];
	store[ 4 ] = store[ 5 ];
	
	store[ 5 ] = vbuf[ i ];
	}

}




//fftw_complex *fftw_in_c0;						//complex pointers
//fftw_complex *fftw_out_c0;

//fftw_plan fftw_p_c0 = 0;









//called by realtime audio proc
//update audio samples, must be completed within: 1.0 / srate * framecnt
//with framecnt = 4096 and srate = 48000, must execute within 85.333mS, 0.085333 Secs
//with framecnt = 4096 and srate = 44100, must execute within 92.880mS, 0.092880 Secs
void update_audio( float *bf0, float *bf1, int frmcnt )
{
bool vb = 0;

if( b_listen )
	{
	demod_iso();

	bool done_once = 0;


	//printf(" update_audio()\n" );

//	double scale = 0.001 * g_audio_out_gain;



	if( vaudio_ch0.size() >= framecnt )
		{
		for( int i = 0; i < framecnt; i++ )
			{
	//		printf("ok\n" );
			
			if( g_aud_mute )
				{
				bf0[ i ] = 0.0f;
				bf1[ i ] = 0.0f;
				}
			else{
				bf0[ i ] = vaudio_ch0[ i ];
				bf1[ i ] = vaudio_ch1[ i ];
				}
			}

//	if(vb)printf("update_audio() - vaudio_ch0 0000.size() %d\n", vaudio_ch0.size() );

	//printf("vfm   : %d\n", vfm.size() );
		vaudio_ch0.erase( vaudio_ch0.begin(), vaudio_ch0.begin() + framecnt );
		vaudio_ch1.erase( vaudio_ch1.begin(), vaudio_ch1.begin() + framecnt );
		
		if( vaudio_ch0.size() > framecnt )
			{
	//		vfm.erase( vfm.begin(), vfm.begin() + 3 );
	//		printf("vfm_erased: %d\n", vfm.size() );
			}

		
	if(vb)printf("update_audio() - vaudio_ch0.size() %d\n", vaudio_ch0.size() );
		
		vfm_loaded = 0;
		}
	else{
		if(vb)printf( "update_audio() - missed samples\n" );
		}

	}


bool mute = 0;

if( !b_listen ) mute = 1;

//printf("here3: imode= %u, mute= %u\n", imode, mute );

//mute = 1;
if ( mute )
	{
	for ( int i = 0; i < framecnt; i++ )	//mute audio
		{
//		double whnz = (double) rand() / (double)( RAND_MAX / 2 ) - 1.0;	//RAND_MAX is 2147483647 0x7fffffff

		bf0[ i ] = 0.0f;
		bf1[ i ] = 0.0f;
		}

	}
}












/*

//update audio samples, must be completed within: 1.0 / srate * framecnt
//with framecnt = 4096 and srate = 48000, must execute within 85.333mS, 0.085333 Secs
//with framecnt = 4096 and srate = 44100, must execute within 92.880mS, 0.092880 Secs
void update_audio_pre_v1_02()
{

if( outbuf1 == 0 )
	{
	if ( my_verbose ) printf( "update_audio() - outbuf1 == 0\n" );
	return;
	}


if( outbuf2 == 0 )
	{
	if ( my_verbose ) printf( "update_audio() - outbuf2 == 0\n" );
	return;
	}


if( inbuf1 == 0 )
	{
//	if ( my_verbose ) printf( "update_audio() - inbuf1 == 0\n" );
//	return;
	}


if( inbuf2 == 0 )
	{
//	if ( my_verbose ) printf( "update_audio() - inbuf2 == 0\n" );
//	return;
	}




if( b_listen )
	{
	do_demod();



	bool done_once = 0;


	//printf(" update_audio()\n" );

	double scale = 0.001 * g_audio_out_gain;



	if( vaudio.size() >= framecnt )
		{
		for( int i = 0; i < framecnt; i++ )
			{
	//		printf("ok\n" );
			
			outbuf1[ i ] = vaudio[ i ] * scale;
			outbuf2[ i ] = vaudio[ i ] * scale;
			}

	//printf("vfm   : %d\n", vfm.size() );
		vaudio.erase( vaudio.begin(), vaudio.begin() + framecnt );
		
		if( vaudio.size() > framecnt )
			{
	//		vfm.erase( vfm.begin(), vfm.begin() + 3 );
	//		printf("vfm_erased: %d\n", vfm.size() );
			}

		
	printf("vfm_erased: %d\n", vaudio.size() );
		
		vfm_loaded = 0;
		}
	else{
		printf( "update_audio() - missed samples\n" );
		}

	}


bool mute = 0;

if( !b_listen ) mute = 1;

//printf("here3: imode= %u, mute= %u\n", imode, mute );

//mute = 1;
if ( mute )
	{
	for ( int i = 0; i < framecnt; i++ )	//mute audio
		{
//		double whnz = (double) rand() / (double)( RAND_MAX / 2 ) - 1.0;	//RAND_MAX is 2147483647 0x7fffffff

		outbuf1[ i ] = 0.0;
		outbuf2[ i ] = 0.0;
		}

	}

audio_card_prepare_out( 0 );			//load audio
audio_card_prepare_out( 1 );


return;

}

*/








/*


bool done_once = 0;


//printf(" update_audio()\n" );

double scale = 1.00;

if( vfm_loaded )
	{
	for( int i = 0; i < framecnt; i++ )
		{
//		printf("ok\n" );
		
		outbuf1[ i ] = vfm[ i ] / scale;
		outbuf2[ i ] = vfm[ i ] / scale;
		}

	printf("vfm   : %d\n", vfm.size() );
	vfm.erase( vfm.begin(), vfm.begin() + framecnt );
	printf("vfm_erased: %d\n", vfm.size() );
	
	vfm_loaded = 0;
	}
else{
	printf("update_audio() - missed samples\n" );
	}
	
bool mute = 0;

if ( mute )
	{
	for ( int i = 0; i < framecnt; i++ )	//mute audio
		{
		double whnz = (double) rand() / (double)( RAND_MAX / 2 ) - 1.0;	//RAND_MAX is 2147483647 0x7fffffff

		outbuf1[ i ] = 0.0;
		outbuf2[ i ] = 0.0;
		}

	return;
	}

audio_card_prepare_out( 0 );			//load audio
audio_card_prepare_out( 1 );


return;






vector<st_cplex_tag> vfft;
vector<st_cplex_tag> vbuf;
vector <st_cplex_tag> vfilt;



rtl.read( vbuf );


if( fftw_size != vbuf.size() )
	{
	printf("!!!size difference, skipping: fftw_size= %d, vbuf.size()= %d\n", fftw_size, vbuf.size());
	return;
	}


filter1( vbuf,  vfilt );

//vfilt = vbuf;

int half = fftw_size / 2;

for( int i = 0; i < fftw_size; i++ )			//box filter the fft
	{
	if( i > fftw_size - 2048  )
		{
//		vbuf[ i ].real = 0;
//		vbuf[ i ].imag = 0;
		}


//vfft[ i ].real = 0;
//vfft[ i ].imag = 0;
	}

//printf("!!!size vfft.size()= %d\n", vfft.size());

	{
//	vfft[ 0 ].real = 0;					//skip dc
//	vfft[ 0 ].imag = 0;
	}

	{
//	vfft[ 2048 ].real = 0;					//skip dc
//	vfft[ 2048 ].imag = 0;
	}

//vfft[ 10 ].real = 1;
//vfft[ 10 ].imag = 1;

//vfft[ 11 ].real = 0.2;
//vfft[ 11 ].imag = 0.0;



//	unsigned int fftw_size = vfft.size();


if( done_once == 0 )
	{
	//fftw pointers
	fftw_in_c0 = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftw_size );
	fftw_out_c0 = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftw_size );

	//single dimension complex to complex reverse fft
	fftw_p_c0 = fftw_plan_dft_1d( fftw_size, fftw_in_c0, fftw_out_c0, FFTW_BACKWARD, FFTW_ESTIMATE );
	done_once = 1;
	}
	


for( int i = 0; i <	fftw_size; i++ )
	{
//	fftw_in_c0[ i ][ 0 ] = vfft[ i ].real;
//	fftw_in_c0[ i ][ 1 ] = vfft[ i ].imag;
	}

//fftw_execute( fftw_p_c0 );					//reverse fft




double now_real;
double now_imag;

//fftw_out_c0[ 0 ][ 0 ] = last_real;
//fftw_out_c0[ 0 ][ 1 ] = last_imag;

for( int i = 0; i < fftw_size; i++ ) 
	{
	now_real = vfilt[ i ].real;
	now_imag = vfilt[ i ].imag;

	double pcm;

	pcm = polar_discriminant( now_real, now_imag, last_real, last_imag );

//		double d1 = now_real;
//		double d2 = now_imag;
	
//		pcm = sqrt( d1 * d1 + d2 * d2  );
//pcm = now_real;

//if( i == 0 ) pcm = last_pcm;
//if( i == 1 ) pcm = last_pcm;

	st_spect_tag o;
	o.freq = i;
	o.ampl = pcm;
vsp.push_back( o );

	vpcm.push_back( pcm );

	last_real = now_real;
	last_imag = now_imag;
	last_pcm = pcm;
	}


//printf("%lf  ", vpcm[ 0 ] );
/*

for( int i = 0; i <	fftw_size; i++ )
	{
	st_spect_tag o;
	
	o.freq = i;
	o.ampl = fftw_out_c0[ i ][ 0 ];
	vsp.push_back( o );
	}
*/


/*

for( int i = 0; i <	vfft.size(); i++ )
	{
	st_spect_tag o;
	
	o.freq = i;
	
	double d1 = vfilt[ i ].real;
	double d2 = vfilt[ i ].imag;
	
	o.ampl = sqrt( d1 * d1 + d2 * d2  );
//o.ampl = vfft[ i ].imag;

//vsp.push_back( o );
	}




if( wnd_rtl_graph ) wnd_rtl_graph->update_graph( vsp );



//printf("framecnt=%d, vpcm.size()=%d\n", framecnt,  vpcm.size() );

for ( int i = 0; i < framecnt; i++ )
	{

//	if( i < vpcm.size() )
		{
		outbuf1[ i ] = vpcm[ i ] / 1.0;
		outbuf2[ i ] = vpcm[ i ] / 1.0;
		}
	}
	

*/




//audio_card_inject_test_sig( en_ac_test_sig_tone, 400 , 0x1fff );

/*
bool mute = 0;

if ( mute )
	{
	for ( int i = 0; i < framecnt; i++ )	//mute audio
		{
		double whnz = (double) rand() / (double)( RAND_MAX / 2 ) - 1.0;	//RAND_MAX is 2147483647 0x7fffffff

		outbuf1[ i ] = 0.0;
		outbuf2[ i ] = 0.0;
		}

	return;
	}

audio_card_prepare_out( 0 );			//load audio
audio_card_prepare_out( 1 );
*/












/*

//update audio samples, must be completed within: 1.0 / srate * framecnt
//with framecnt = 4096 and srate = 48000, must execute within 85.333mS, 0.085333 Secs
//with framecnt = 4096 and srate = 44100, must execute within 92.880mS, 0.092880 Secs
void update_audio_fft( )
{
printf( "update_audio_fft()\n" );

//return;


if( outbuf1 == 0 )
	{
	if ( my_verbose ) printf( "update_audio() - outbuf1 == 0\n" );
	return;
	}


if( outbuf2 == 0 )
	{
	if ( my_verbose ) printf( "update_audio() - outbuf2 == 0\n" );
	return;
	}


if( inbuf1 == 0 )
	{
//	if ( my_verbose ) printf( "update_audio() - inbuf1 == 0\n" );
//	return;
	}


if( inbuf2 == 0 )
	{
//	if ( my_verbose ) printf( "update_audio() - inbuf2 == 0\n" );
//	return;
	}






bool done_once = 0;




vector <st_spect_tag> vsp;

vector<st_cplex_tag> vfft;

vector<double>vpcm;

rtl.read_fft( vfft );


if( fftw_size != vfft.size() )
	{
	printf("!!!size difference, skipping: fftw_size= %d, vfft.size()= %d\n", fftw_size, vfft.size());
	return;
	}

int half = fftw_size / 2;

for( int i = 1; i < half; i++ )			//box filter the fft
	{
	if( ( i < -1 ) || ( i > 500  ) )
		{
//		if( i == 0 ) continue;					//skip dc
//		if( i == 2048 ) continue;				//skip dc
//		vfft[ i ].real = 0;
//		vfft[ i ].imag = 0;
		
//		vfft[ fftw_size - i ].real = 0;
//		vfft[ fftw_size - i ].imag = 0;
		}


//vfft[ i ].real = 0;
//vfft[ i ].imag = 0;
	}

//printf("!!!size vfft.size()= %d\n", vfft.size());

	{
//	vfft[ 0 ].real = 0;					//skip dc
//	vfft[ 0 ].imag = 0;
	}

	{
//	vfft[ 2048 ].real = 0;					//skip dc
//	vfft[ 2048 ].imag = 0;
	}

//vfft[ 10 ].real = 1;
//vfft[ 10 ].imag = 1;

//vfft[ 11 ].real = 0.2;
//vfft[ 11 ].imag = 0.0;



//	unsigned int fftw_size = vfft.size();


if( done_once == 0 )
	{
	//fftw pointers
	fftw_in_c0 = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftw_size );
	fftw_out_c0 = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftw_size );

	//single dimension complex to complex reverse fft
	fftw_p_c0 = fftw_plan_dft_1d( fftw_size, fftw_in_c0, fftw_out_c0, FFTW_BACKWARD, FFTW_ESTIMATE );
	done_once = 1;
	}
	


for( int i = 0; i <	fftw_size; i++ )
	{
	fftw_in_c0[ i ][ 0 ] = vfft[ i ].real;
	fftw_in_c0[ i ][ 1 ] = vfft[ i ].imag;
	}

fftw_execute( fftw_p_c0 );					//reverse fft




double now_real;
double now_imag;

fftw_out_c0[ 0 ][ 0 ] = last_real;
fftw_out_c0[ 0 ][ 1 ] = last_imag;

for( int i = 0; i < fftw_size; i++ ) 
	{
	now_real = fftw_out_c0[ i ][ 0 ];
	now_imag = fftw_out_c0[ i ][ 1 ];

	double pcm;

	pcm = polar_discriminant( now_real, now_imag, last_real, last_imag );

//		double d1 = now_real;
//		double d2 = now_imag;
	
//		pcm = sqrt( d1 * d1 + d2 * d2  );
//pcm = now_real;

if( i == 0 ) pcm = last_pcm;
if( i == 1 ) pcm = last_pcm;

	st_spect_tag o;
	o.freq = i;
	o.ampl = pcm;
//vsp.push_back( o );

	vpcm.push_back( pcm );

	last_real = now_real;
	last_imag = now_imag;
	last_pcm = pcm;
	}


//printf("%lf  ", vpcm[ 0 ] );




for( int i = 0; i <	vfft.size(); i++ )
	{
	st_spect_tag o;
	
	o.freq = i;
	
	double d1 = vfft[ i ].real;
	double d2 = vfft[ i ].imag;
	
	o.ampl = sqrt( d1 * d1 + d2 * d2  );
//o.ampl = vfft[ i ].imag;

vsp.push_back( o );
	}




if( wnd_rtl_graph ) wnd_rtl_graph->update_graph( vsp );



//printf("framecnt=%d, vpcm.size()=%d\n", framecnt,  vpcm.size() );

for ( int i = 0; i < framecnt; i++ )
	{

//	if( i < vpcm.size() )
		{
		outbuf1[ i ] = vpcm[ i ] * 10.0;
		outbuf2[ i ] = vpcm[ i ] * 10.0;
		}
	}
	






//audio_card_inject_test_sig( en_ac_test_sig_tone, 400 , 0x1fff );


bool mute = 0;

if ( mute )
	{
	for ( int i = 0; i < framecnt; i++ )	//mute audio
		{
		double whnz = (double) rand() / (double)( RAND_MAX / 2 ) - 1.0;	//RAND_MAX is 2147483647 0x7fffffff

		outbuf1[ i ] = 0.0;
		outbuf2[ i ] = 0.0;
		}

	return;
	}

audio_card_prepare_out( 0 );			//load audio
audio_card_prepare_out( 1 );

}

*/
















/*
mystr timer_cb_audio_process_out;
mystr timer1;

//audio card will call this when more audio sample data is needed - audio data to line/spkr o/p, to D/A
int cb_audio_process_out( unsigned int bufid, unsigned int frames, void *arg )
{
timer1.time_start( timer1.ns_tim_start );

//printf(" cb_audio_process\n" );


//just for timing calc and display
if( max_time_cb_audio_process_out == 0 )
	{
	timer_cb_audio_process_out.time_start( timer_cb_audio_process_out.ns_tim_start );
	max_time_cb_audio_process_out = -1;			//set it to something other than zero
	}
else{
	double d = timer_cb_audio_process_out.time_passed( timer_cb_audio_process_out.ns_tim_start );
	timer_cb_audio_process_out.time_start( timer_cb_audio_process_out.ns_tim_start );
	if( d > max_time_cb_audio_process_out ) max_time_cb_audio_process_out = d; 
	}



outbuf1 = audio_card_get_outbuf( 0 );
outbuf2 = audio_card_get_outbuf( 1 );

inbuf1 = audio_card_get_inbuf( 0 );
inbuf2 = audio_card_get_inbuf( 1 );

update_audio();


//printf("max_time_cb_audio_process_out= %lf\n", max_time_cb_audio_process_out );

return 1;
}
*/
















bool start_audio()
{
printf("start_audio()\n" );

if( audio_started ) 
	{
	printf("start_audio() - audio was already started, will do nothing\n" );
	return 1;	
	}

rta.verbose = 1;

//rtaudio_probe();									//dump a list of devices to console


RtAudio::StreamOptions options;

options.streamName = cnsAppName;
options.numberOfBuffers = 2;						//for jack (at least) this is hard set by jack's settings and can't be changed via this parameter

options.flags = 0; 	//0 means interleaved, use oring options, refer 'RtAudioStreamFlags': RTAUDIO_NONINTERLEAVED, RTAUDIO_MINIMIZE_LATENCY, RTAUDIO_HOG_DEVICE, RTAUDIO_SCHEDULE_REALTIME, RTAUDIO_ALSA_USE_DEFAULT, RTAUDIO_JACK_DONT_CONNECT
					// !!! when using RTAUDIO_JACK_DONT_CONNECT, you can create any number of channels, you can't do this if auto connecting as 'openStream()' will fail if there is not enough channel mating ports  

options.priority = 0;								//only used with flag 'RTAUDIO_SCHEDULE_REALTIME'



uint16_t device_num_out = 0;						//use 0 for default device to be used
int channels_out = 2;								//if auto connecting (not RTAUDIO_JACK_DONT_CONNECT), there must be enough mathing ports or 'openStream()' will fail
int first_chan_out = 0;

uint16_t frames = framecnt;
unsigned int audio_format = RTAUDIO_FLOAT32;		//see rtaudio docs 'RtAudioFormat' for supported format types, adj audio proc code to suit

//st_osc_params.freq0 = 200;							//set up some audio proc callback user params
//st_osc_params.gain0 = 0.1;

//st_osc_params.freq1 = 600;
//st_osc_params.gain1 = 0.1;
//st_rta_arg.usr_ptr = (void*)&st_osc_params;



if( !rta.start_stream_out( device_num_out, channels_out, first_chan_out, aud_op_srate, frames, audio_format, &options, &st_rta_arg, (void*)cb_audio_proc_rtaudio ) )		//output only
//if( !rta.start_stream_out( device_num_out, channels_out, first_chan_out, srate, frames, audio_format, &options, &st_rta_arg, (void*)cb_audio_proc_rtaudio_migrate ) )		//output only
	{
	printf("start_audio() - failed to open audio device!!!!\n" );
	return 0;	
	}
else{
	printf("start_audio() - audio out device %d opened, options.flags 0x%02x, srate is: %d, framecnt: %d\n", device_num_out, options.flags, rta.get_srate(), rta.get_framecnt()  );							 //output only	
	}

aud_op_srate = rta.get_srate();
framecnt = rta.get_framecnt();

printf("start_audio() - srate %d, framecnt: %d\n", aud_op_srate, framecnt );


time_per_sample = 1.0/aud_op_srate;

//fftw_init( framecnt );

//filters_create();
//filter_adjust_fir_bw_lower_upper();

audio_started = 1;
return 1;
}










void stop_audio()
{
bool vb = 1;

printf( "stop_audio() - step 0\n" );
audio_started = 0;

mystr m1;
m1.delay_ms( (float)framecnt / aud_op_srate * 1e3 );												//wait one audio proc period


rta.stop_stream();

//fftw_destroy();




if(vb)printf( "stop_audio() - step 1\n" );

//if ( fftw_p_c0 ) fftw_destroy_plan( fftw_p_c0 );
//fftw_p_c0 = 0;

if(vb)printf( "stop_audio() - step 2\n" );

//if ( fftw_in_c0 ) fftw_free( fftw_in_c0 );
//fftw_in_c0 = 0;
if(vb)printf( "stop_audio() - step 3\n" );

//if ( fftw_out_c0 ) fftw_free( fftw_out_c0 );
//fftw_out_c0 = 0;

if(vb)printf( "stop_audio() - step 4\n" );

//if ( fftw_p_c0 ) fftw_destroy_plan( fftw_p_c0 );
//fftw_p_c0 = 0;

//if ( fftw_in_c0 ) fftw_free( fftw_in_c0 );
//if ( fftw_out_c0 ) fftw_free( fftw_out_c0 );
//fftw_in_c0 = 0;
//fftw_out_c0 = 0;
}

























/*
//allocate memory
//make audio files
//start audio
void start_audio( void* obj )
{


printf( "start_audio() - allocating memory (in %d units)\n", framecnt );



ac_buf_ch1 = new double[ framecnt ];
ac_buf_ch2 = new double[ framecnt ];




if ( !audio_card_open_out( srate, 2, 16, framecnt, cb_audio_process_out, (void*)obj ) )
	{
	printf( "start_audio() - can't init audio card for output - failed\n" );
	}


if ( !audio_card_open_in( srate, 2, 16, framecnt, cb_audio_process_in, (void*)obj ) )
	{
	printf( "start_audio() - can't init audio card for input - failed\n" );
	}

unsigned int sr;

if( audio_card_get_srate( sr ) )
	{
	srate = sr;
	}

audio_card_set_vu0( VU0 );

}













void stop_audio( )
{
printf( "stop_audio() - freeing memory\n ");


audio_card_close( );




if( ac_buf_ch1 ) delete ac_buf_ch1;
if( ac_buf_ch2 ) delete ac_buf_ch2;
ac_buf_ch1 = 0;
ac_buf_ch2 = 0;


if ( fftw_p_c0 ) fftw_destroy_plan( fftw_p_c0 );
fftw_p_c0 = 0;

if ( fftw_in_c0 ) fftw_free( fftw_in_c0 );
if ( fftw_out_c0 ) fftw_free( fftw_out_c0 );
fftw_in_c0 = 0;
fftw_out_c0 = 0;

}

*/












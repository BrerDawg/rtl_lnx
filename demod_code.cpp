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



//demod_code.cpp



//v1.01		10-jan-2026



#include "demod_code.h"

bool vb0 = 0;															//verbose control

//bool bdbg_dump_coeff = 1;

extern double g_dev_bw;


bool b_synth_build_start = 0;											//set this and it will set 'b_synth_build_recording = 1' on first demod call
																		//it will record 'cn_synth_rec_time' secs of samples, then replay them, to remove burden on cpu, good for cpu usage calcs

bool b_synth_build_recording = 0;										//DON'T set this, use 'b_synth_build_start', if it's set the synth samples are being recorded
bool b_synth_build_playing = 0;											//DON'T set this, it will be set after 'b_synth_build_recording' is cleared

int16_t *bf_synth = 0;
int synth_ptr_wr = 0;
int synth_ptr_rd = 0;

#define cn_synth_rec_time 10

int synth_sample_cnt = cn_synth_rec_time * g_dev_bw * 2;



extern int g_dbg0;
extern bool b_dbg_skip_demod;
extern int demod_cnt;
extern rtl_graph_wnd *wnd_rtl_graph;
extern cl_aud_spect_wnd* wnd_aud_spect;

extern int sync_wr_rd_pointer;
extern int pref_taper_windowing_for_fft;
extern int pref_zero_padding_for_fft;
extern int pref_zero_padding_for_fft_cnt;
//extern int pref_zero_padding_for_fft_last;
extern int i_fftw_trig_plan_create_state;
extern int demod_cnt_modulo0;
extern float frame_time;
extern unsigned int aud_op_srate;
extern int downsample_srate;											//the downsample_srate will be upsampled to 'aud_op_srate' if required
int downsample_srate_last = downsample_srate;
extern int fm_decimator_srate;

extern float time_per_sample;
float freq_wholecycle_fft;												//see 'init_osc_integrators()' as to why particular freqs are stored in this


extern unsigned int framecnt;
extern int gph_samples_per_sec;
extern bool b_rtlsdr_callback_first;
extern int downsample_factor;
//extern pthread_mutex_t mutex2;
extern std::mutex mutex3;

extern int gph_loc_sel;
extern uint64_t uiwnd_low;
extern uint64_t uiwnd_high;
extern uint64_t uiwnd_low2;
extern uint64_t uiwnd_high2;

extern uint64_t idbg_wr;
extern uint64_t idbg_rd;
extern int64_t delta_wr_rd;
extern float inc_rate;
extern float inc_rate2;
extern int rtl_bf0_rd;
extern float g_gain_iq;
extern float *rtl_bfI0;
extern float *rtl_bfQ0;
extern bool g_b_if_freq;
extern int64_t g_interfreq;
extern vector<st_morse_seq_tag> vmorse0;
extern vector<st_morse_seq_tag> vmorse1;
extern vector<st_morse_seq_tag> vmorse10;
extern float synth_theta00;
extern float synth_tone_theta1;
extern float synth_tone_theta2;
extern float synth_tone_theta3;
extern float synth_tone_theta4;
extern float synth_tone_theta5;
extern float synth_tone_theta6;

extern float synth_theta10;
extern float synth_theta20;
extern float synth_theta30;
extern float synth_theta40;
extern float synth_theta50;
extern float synth_theta60;

extern double fract_ptr00;
extern double fract_ptr000;
extern double fract_ptr10;
extern double fract_ptr20;
extern double fract_ptr30;
extern double fract_ptr40;


extern int synth_iq_ptr0;

float synth_carr_freq0;
int synth_carr_lsb_usb_fm_0 = 1;										//0: off, 1: AM, 2: lower sideband, 3: upper sideband, 4: FM,  5: FM Stereo,  6: FM Stereo (no pilot)
int synth_morse_tone0 = 6;										//0: morse short,  1: morse long,  2: voice0,  3: voice1,  4: tone swept,  5: tone warble,  6: tone adj,  7: stereo tone,  8: stereo voices,  9: off  
float synth_tone_freq0 = 400.0f;
float synth_tone_freq_right_chan0 = 600.0f;

float synth_tone_freq_theta0 = 0;
float synth_tone_freq_theta_inc0 = 0;
float synth_tone_freq_theta_right_chan0 = 0;
float synth_tone_freq_theta_right_chan_inc0 = 0;


float synth_pilot_freq0 = 19e3 + 0.1;											//fm stereo pilot tone
float synth_38K_L_R_mux_freq0 = 38e3;									//fm stereo 38KHz L-R mux osc

float synth_pilot_gain0 = 0.09;											//use 9% deviation
float synth_tone_dcoffs0 = 0.0f;


float theta_fm_19K_pilot = 0;											//used for both 19K pilot and 38K muxer
float theta_fm_19K_pilot_inc;

//float theta_fm_38K_mux = 0;
//float theta_fm_38K_mux_inc;





float synth_carr_freq1;
int synth_carr_lsb_usb_fm_1 = 1;										//0: off, 1: AM, 2: lower sideband, 3: upper sideband, 4: FM,  5: FM Stereo,  6: FM Stereo (no pilot)
int synth_morse_tone1 = 2;												//0: morse short,  1: morse long,  2: voice,  3: voice,  4: tone swept,  5: tone warble,  6: tone adj,  7: stereo tone  8: off
float synth_tone_freq1 = 500.0f;
float synth_tone_freq_theta1 = 0;
float synth_tone_freq_theta_inc1 = 0;


float synth_carr_freq2;
int synth_carr_lsb_usb_fm_2 = 1;										//0: off, 1: AM, 2: lower sideband, 3: upper sideband, 4: FM,  5: FM Stereo,  6: FM Stereo (no pilot)
int synth_morse_tone2 = 3;												//0: morse short,  1: morse long,  2: voice,  3: voice,  4: tone swept,  5: tone warble,  6: tone adj,  7: off
float synth_tone_freq2 = 500.0f;
float synth_tone_freq_theta2 = 0;
float synth_tone_freq_theta_inc2 = 0;


float synth_carr_freq3;
int synth_carr_lsb_usb_fm_3 = 1;										//0: off, 1: AM, 2: lower sideband, 3: upper sideband, 4: FM,  5: FM Stereo,  6: FM Stereo (no pilot)
int synth_morse_tone3 = 5;												//0: morse short,  1: morse long,  2: voice,  3: voice,  4: tone swept,  5: tone warble,  6: tone adj,  7: off
float synth_tone_freq3 = 500.0f;
float synth_tone_freq_theta3 = 0;
float synth_tone_freq_theta_inc3 = 0;


float synth_carr_freq4;
int synth_carr_lsb_usb_fm_4 = 1;										//0: off, 1: AM, 2: lower sideband, 3: upper sideband, 4: FM,  5: FM Stereo,  6: FM Stereo (no pilot)
int synth_morse_tone4 = 5;												//0: morse short,  1: morse long,  2: voice,  3: voice,  4: tone swept,  5: tone warble,  6: tone adj,  7: off
float synth_tone_freq4 = 500.0f;
float synth_tone_freq_theta4 = 0;
float synth_tone_freq_theta_inc4 = 0;

float synth_noise_gain = 0.05f;


extern vector<float> vaudclip0;
extern vector<filter_code::st_cplex_tag>vc_audclip0;

extern vector<float> vaudclip1;
extern vector<filter_code::st_cplex_tag>vc_audclip1;

extern vector<float> vaudclip2;
extern vector<filter_code::st_cplex_tag>vc_audclip2;

extern vector<float> vaudclip3;
extern vector<filter_code::st_cplex_tag>vc_audclip3;

//extern vector<float> vaudclip4;
//extern vector<filter_code::st_cplex_tag>vc_audclip4;

extern vector<float> vaudsynth0;
extern vector<filter_code::st_cplex_tag>vc_audsynth0;

extern vector<float> vaudsynth1;
extern vector<filter_code::st_cplex_tag>vc_audsynth1;


extern int64_t g_freq_tune;
extern int64_t g_freq_tune_offset;
extern int64_t g_freq_sub_tune;

extern bool b_slew_adj_fine;

extern int rec_play_iq_state;

extern float *recply_bf;
extern unsigned long long int play_filesize;
extern int64_t rec_wr_cnt;
extern int64_t rec_rd_cnt;
extern unsigned int rec_bf_sz;
extern float rec_play_time;
extern float play_mode_end_secs;
extern float rec_mode_end_secs;
extern int64_t rec_wr;
extern int64_t rec_rd;
//extern bool b_rec_play_flush;
													
//extern bool b_rec_change_play_pos;
//extern int64_t rec_change_play_pos_to;

extern int filt_prev_idx0;
extern float now_r0;
extern float now_j0;

extern int filt_prev_idx2;
extern float now_r2;
extern float now_j2;

extern int filt_prev_idx14;
extern float now_r14;
extern float now_j14;

extern vector<double>vpcm;
extern vector<float> vpcm_ch0;
extern vector<float> vpcm_ch1;

extern vector <float> vaudio;
extern vector <float> vaudio_ch0;
extern vector <float> vaudio_ch1;

en_demodulator_type_tag demod_type;
extern int down_sample_factor_for_graph;
extern vector<st_spect_tag> vspect_displayble;
extern vector<st_spect_tag> vspect_gph;
extern vector<filter_code::st_cplex_tag> vcplex_gph;



extern float disp_spect_zoom_factor;
extern int gph_loc_sel_tmp;
extern bool b_plot_gph1;
st_plot_probe_tag st_plot;												//used in audio thread
extern vector<float> vgph1_x;
extern vector<float> vgph1_y0;
extern vector<float> vgph1_y1;
extern vector<st_mgraph_user_marker_tag> vgph1_vuser_marker;
extern vector<int>vgph1_user_marker_idx;

extern int plot_gph_trace_cnt;
extern int gph1_b_x_axis_values_derived;
extern float gph1_x_axis_values_derived_left_value;
extern float gph1_x_axis_values_derived_inc_value;

extern float theta_mux;


extern float g_gain_aud;
extern float g_aud_lev_avg;
extern float aud_dim_time;
float aud_dim_gain = 1.0;
extern int aud_gain_plus_minus_db;

extern bool g_b_dwn_aa;


extern bool g_b_user_iir_lpf0;

extern bool g_b_user_iir_lpf1;

extern bool g_b_user_iir_lpf2;


extern bool g_b_user_iir_hpf0;


extern filter_code::st_iir usr_hpf_iir0_I0;									//user adj filter
extern filter_code::st_iir usr_hpf_iir0_Q0;

extern filter_code::st_iir usr_lpf_iir0_I0;									//user adj filter
extern filter_code::st_iir usr_lpf_iir0_Q0;

extern filter_code::st_iir usr_lpf_iir1_I0;									//user adj filter
extern filter_code::st_iir usr_lpf_iir1_Q0;

extern filter_code::st_iir usr_lpf_iir2_I0;									//user adj filter
extern filter_code::st_iir usr_lpf_iir2_Q0;

extern bool g_b_user_iir_notch0;
//extern filter_code::st_iir usr_notch_iir0;									//user adj filter, refer 'g_user_iir_notch_freq0'


extern bool g_b_bw_bpass;
extern filter_code::st_fir usr_demod_bpf_fir_I0;
extern filter_code::st_fir usr_demod_bpf_fir_Q0;


extern st_filter_sdr_tag st_filt[ cn_filters_max ];



extern bool b_dc_block_iq;
extern float dc_block1_dly0;
extern float dc_block1_dly1;
extern float dc_block2_dly0;
extern float dc_block2_dly1;

//float dc_block3_devlp_dly0 = 0;
//float dc_block3_devlp_dly1 = 0;

#define cn_dcblk_sz 64
float dcblk_bf[cn_dcblk_sz];
int dcblk_ptr = 0;

extern float rnd();
extern void low_pass_srconv( vector <filter_code::st_cplex_tag> &vin,  vector <filter_code::st_cplex_tag> &vout, int downsample, int &filt_prev_idx, float &now_r, float &now_j  );
extern void low_pass_srconv_float( vector <float> &vin, vector <float> &vout, int downsample, int &filt_prev_idx, float &now_r );
extern void demod_am( vector<filter_code::st_cplex_tag> vin, vector <double> &vpcm, float output_scale );
extern void demod_am_float( vector<filter_code::st_cplex_float_tag> vin, vector <float> &vpcm, float output_scale );
extern void demod_lsb( vector<filter_code::st_cplex_float_tag> vin, vector <float> &vpcm, float output_scale );
extern void demod_usb( vector<filter_code::st_cplex_float_tag> vin, vector <float> &vpcm, float output_scale );
extern void demod_fm( vector<filter_code::st_cplex_float_tag> vin, vector <float> &vpcm, float output_scale );
extern void demod_wfm( vector<filter_code::st_cplex_float_tag> vin, vector <float> &vpcm, float output_scale );

//extern void low_pass_real_sinc( vector <double> &vin,  vector <double> &vout, int src_rate, int dest_rate, int dest_cnt, float scale, string scaller );
extern void low_pass_real_sinc_float( vector <float> &vin,  vector <float> &vout, int src_rate, int dest_rate, int dest_cnt, float scale, string scaller, float *headbf );
extern void low_pass_real_sinc( vector <double> &vin,  vector <double> &vout, int src_rate, int dest_rate, int dest_cnt, float scale, string scaller );

//extern bool complex_fwd_fft_multi( vector<filter_code::st_cplex_tag> vin, vector<filter_code::st_cplex_tag> &vsp, string user_str, en_fftw_thread_id_tag thrd_id );
//extern bool real_fwd_fft_multi_float( vector<float> vin, vector<filter_code::st_cplex_tag> &vsp, string user_str, en_fftw_thread_id_tag thrd_id );
//extern bool real_fwd_fft_multi( vector<double> vin, vector<filter_code::st_cplex_tag> &vsp, string user_str, en_fftw_thread_id_tag thrd_id );

extern void complex_fft_displayable( int srate_in, int freq_offs, vector<filter_code::st_cplex_tag> vsp_in, vector<st_spect_tag> &vsp, bool show_dc, bool show_nyquist );

extern int64_t g_freq_sub_tune;



bool b_dbg_no_graph_update = 0;											//set this to stop any fft and graph plotting to allow cpu usage tests
bool b_fit_auto_plot = 1;

vector <filter_code::st_cplex_tag> vcplex_sp;
vector<double> vdble_dummy;
vector<float> vfloat_dummy;

extern bool b_clip_enable;
extern int clip_audio_cnt_max;
extern int clip_audio_cnt;
extern float clip_level_audio;
extern bool g_aud_mono;


//st_baseline_remove_tag st_bline_remove;



extern float agc_audio_attack_factor;									// 'agc_audio_attack_time' / frametime;
extern float agc_audio_decay_factor;									// 'agc_audio_decay_time' / frametime;

//extern bool b_agc_enable;
//extern float agc_audio_high_limit;
//extern float agc_audio_low_limit;

//extern float agc_audio_goal;
//extern float agc_audio_low_goal;
extern float agc_audio_attack_time;
extern float agc_audio_decay_time;
extern float agc_audio_peak;
extern float agc_gain_lvl;
extern float agc_audio_peak_charge_time;
extern float agc_audio_peak_discharge_time;

//extern float agc_audio_control_val;

extern bool b_agc;
extern float agc_audio_attack_factor;
extern float agc_audio_decay_factor;


#define cn_moving_avg_siz 8192

//float moving_average(float x);


extern filter_code::st_iir agc_iir0;

extern bool g_fm_19K_pilot_tone;

extern halfband_poly_optimised_float::st_hbpo_tag fm_stereo_decimator;


//#define cn_wndwdth 8
extern float head_bf0[];												//used for resampler inter frame continuity
extern float head_bf1[];


//#define cn_agc_bf0_siz 1024

//float agc_bf0[cn_agc_bf0_siz];
//int agc_wr_bf0 = 0;
//int agc_accum_sz = 128;
//int agc_accum_cnt = 0;

////float agc_accum_sum = 0;


//float agc_led_last = 0;
//float agc_led_integ = 0;

//void agc_init(AGC& a, float fs);
//inline float agc_process(AGC& a, float x);
//AGC agc0;






lin_up2x up2x_ch0;														//linear 2x upsampler, e.g 24K --> 48KHz
lin_up2x up2x_ch1;
float bf_upsample_ch0[2];
float bf_upsample_ch1[2];



//std::atomic<size_t> bf1_rd;
//std::atomic<size_t> bf1_wr;

int bf1_prep_idx = 0;													//0: will fill 'bf1_iq[0][..]'   1: will fill 'bf1_iq[1][..]'  ping pong scheme for thread safe access
alignas(64) atomic<int> bf1_prep_idx_atm{ -1 };							//-1: no data block is prepared, 0: 'bf1_iq[0][..]' has data avail,  1: 'bf1_iq[1][..]' has data avail
alignas(64) filter_code::st_cplex_tag bf1_iq[2][cn_bf1_iq_sz];			//dual buffer ping pong scheme for thread safe access 
alignas(64) atomic<size_t> bf1_cnt_atm{0};								//number of samples in 'bf1_iq[bf1_prep_block_idx][..]'



extern st_audio_spect_avg_tag st_aud_spect_avg;









void stereo_pll_init(stereo_pll_state& s, float adc_srate, int dwnsmple_srate );

extern stereo_pll_state st_fm_decoder;
extern int i_deemphasis;
int i_deemphasis_last = 0;

void stereo_pll_process( stereo_pll_state& s, vector<filter_code::st_cplex_tag> &x, vector<float>& vch0, vector<float>& vch1, float gain_out );





void plot_load_struct_audio_thrd( st_plot_probe_tag &st, void *vv0, void* vv1 );

void plot_using_vectors_eval( en_graph_loc_sel_tag which, unsigned int srate_in, vector <filter_code::st_cplex_tag> vcpx0, vector <filter_code::st_cplex_tag> vcpx1, vector<st_spect_tag> vsp0, vector<double> vdble0, vector<float> vflt0, vector<float> vflt1 );
void plot_using_struct( st_plot_probe_atm_tag &st );
void aud_spect_struct_audio_thrd( vector<float> &vflt0 );


extern int plot_prep_idx;
extern atomic<int> plot_prep_idx_atm;
extern st_plot_probe_atm_tag st_plot_atm[2];


extern bool fftw_build_if_req(en_fftw_index_allocations_tag idx, bool b_destroy_only, const char* szname_in, en_fftw_plans_type_tag typ, unsigned int size );
extern bool fftw_fwd_clpx_cplx( en_fftw_index_allocations_tag idx, vector<filter_code::st_cplex_tag> vin, vector<filter_code::st_cplex_tag> &vsp, float gain_in );
extern bool fftw_fwd_real_cplx( en_fftw_index_allocations_tag idx, vector<float> &vin, vector<filter_code::st_cplex_tag> &vsp, float gain_in );


extern int aud_spect_idx;
extern atomic<int> aud_spect_idx_atm;
extern st_aud_spect_atm_tag st_aud_spect_atm[2];


st_audio_filter_tag st_aud_vu_lpf;



/*
struct cic_state 
{
    int idx = 0;

    // 3-stage integrators
    float i1r = 0, i1i = 0;
    float i2r = 0, i2i = 0;
    float i3r = 0, i3i = 0;

    // 3-stage comb delay registers
    float c1r = 0, c1i = 0;
    float c2r = 0, c2i = 0;
    float c3r = 0, c3i = 0;
    
} st_cic_state;
*/


#define cn_synth_IQ_divisor (32768.0f)


#define cn_dual_integrator_osc_cnt 40
#define cn_di_osc0 0													//carrier 0  SEE also 'osc_active_list_build()' to ACTIVE required oscillators
#define cn_di_osc1 1													//not allocated
#define cn_di_osc2 2													//not allocated
#define cn_di_osc5 5													//carrier 1
#define cn_di_osc7 7													//carrier 2
#define cn_di_osc9 9													//carrier 3
#define cn_di_osc11 11													//carrier 4
#define cn_di_osc13 13													//carrier
#define cn_di_osc15 15
#define cn_di_osc20 20													//tone
#define cn_di_osc21 21													//tone
#define cn_di_osc22 22													//tone
#define cn_di_osc23 23													//tone

//#define cn_di_osc30 30													//19KHz stereo pilot tone
//#define cn_di_osc31 31													//38KHz stereo L-R mux osc


st_dual_integrator_osc_tag st_di_osc[ cn_dual_integrator_osc_cnt ];






//iir_elliptic elliplp_cr, elliplp_ci;

//elliptical_iir_code::iir_elliptic elliplp_cr, elliplp_ci;



extern int aa_dwncnv_fc;
cl_ellip_builder elliplpf_cr;
cl_ellip_builder elliplpf_ci;





void init_osc_integrators( bool calc_initial_freq , bool phase_zero_IQ )
{
int i_interfreq = 0;


float time_per_sample = 1.0f/g_dev_bw;

float freq_offset = 0;


//---
int ii = cn_di_osc0;
if( phase_zero_IQ )
	{
	st_di_osc[ii].I = 1.0f;												//init osc
	st_di_osc[ii].Q = 0.0f;
	}

if( calc_initial_freq )
	{

	freq_wholecycle_fft = 1.0f/( time_per_sample * framecnt );			//calc a freq that puts a whole sine cycle in frame buffer, this ensures fft spectra sit in one bin only, useful for gain checks in the sdr chain
	freq_wholecycle_fft *= 1.0f;										//e.g: at srate: 2496000 framcnt: 2048, one full cycle in 2048 samples is: 1218.75Hz, 2 cycles is: 2437.5Hz
//freq_wholecycle_fft = 1000;
	synth_carr_freq0 = freq_wholecycle_fft;

//	printf("WWWWWWWWWWWWWWWWWWW demod_iso() - time_per_sample %f  time_per_sample %d freq_wholecycle_fft %f\n", time_per_sample, framecnt, freq_wholecycle_fft );
//	exit(0);
	}


float w = twopi * (freq_offset + synth_carr_freq0) / g_dev_bw;			//carrier
st_di_osc[ii].cos = cosf( w );
st_di_osc[ii].sin = sinf( w );
//---




//---
ii = cn_di_osc1;
if( phase_zero_IQ )
	{
	st_di_osc[ii].I = 1.0f;												//carrier
	st_di_osc[ii].Q = 0.0f;
	}

w = twopi * (freq_offset + synth_carr_freq1) / g_dev_bw;
st_di_osc[ii].cos = cosf( w );
st_di_osc[ii].sin = sinf( w );
//---





//---
ii = cn_di_osc5;
if( phase_zero_IQ )
	{
	st_di_osc[ii].I = 1.0f;												//carrier
	st_di_osc[ii].Q = 0.0f;
	}

if( calc_initial_freq )
	{
	synth_carr_freq1 = 5e3;
	}
w = twopi * (freq_offset + synth_carr_freq1) / g_dev_bw;
st_di_osc[ii].cos = cosf( w );
st_di_osc[ii].sin = sinf( w );
//---


//---
ii = cn_di_osc7;
if( phase_zero_IQ )
	{
	st_di_osc[ii].I = 1.0f;												//carrier
	st_di_osc[ii].Q = 0.0f;
	}

if( calc_initial_freq )
	{
	synth_carr_freq2 = 10e3;
	}
w = twopi * (freq_offset + synth_carr_freq2) / g_dev_bw;
st_di_osc[ii].cos = cosf( w );
st_di_osc[ii].sin = sinf( w );
//---


//---
ii = cn_di_osc9;
if( phase_zero_IQ )
	{
	st_di_osc[ii].I = 1.0f;												//carrier
	st_di_osc[ii].Q = 0.0f;
	}

if( calc_initial_freq )
	{
	synth_carr_freq3 = 15e3;
	}
w = twopi * (freq_offset + synth_carr_freq3) / g_dev_bw;
st_di_osc[ii].cos = cosf( w );
st_di_osc[ii].sin = sinf( w );
//---


//---
ii = cn_di_osc11;
if( phase_zero_IQ )
	{
	st_di_osc[ii].I = 1.0f;												//carrier
	st_di_osc[ii].Q = 0.0f;
	}

if( calc_initial_freq )
	{
	synth_carr_freq4 = 20e3;
	}
w = twopi * (freq_offset + synth_carr_freq4) / g_dev_bw;
st_di_osc[ii].cos = cosf( w );
st_di_osc[ii].sin = sinf( w );
//---


//---
ii = cn_di_osc13;
if( phase_zero_IQ )
	{
	st_di_osc[ii].I = 1.0f;												//carrier
	st_di_osc[ii].Q = 0.0f;
	}

w = twopi * (freq_offset + 25e3) / g_dev_bw;
st_di_osc[ii].cos = cosf( w );
st_di_osc[ii].sin = sinf( w );
//---



//---
ii = cn_di_osc15;
if( phase_zero_IQ )
	{
	st_di_osc[ii].I = 1.0f;												//for down mixer
	st_di_osc[ii].Q = 0.0f;
	}

//w = twopi * (g_freq_sub_tune) / g_dev_bw;
w = twopi * (10e6) / g_dev_bw;
st_di_osc[ii].cos = cosf( w );
st_di_osc[ii].sin = sinf( w );
//---



//---
ii = cn_di_osc20;
if( phase_zero_IQ )
	{
	st_di_osc[ii].I = 1.0f;												//tone
	st_di_osc[ii].Q = 0.0f;
	}

w = twopi * (freq_wholecycle_fft * 0.25f) / g_dev_bw;
//w = twopi * (500) / g_dev_bw;
st_di_osc[ii].cos = cosf( w );
st_di_osc[ii].sin = sinf( w );
//---



/*
//---
ii = cn_di_osc30;
if( phase_zero_IQ )
	{
	st_di_osc[ii].I = 1.0f;												//19KHz pilot tone (synthesis test)
	st_di_osc[ii].Q = 0.0f;
	}

w = twopi * (synth_pilot_freq0) / g_dev_bw;
st_di_osc[ii].cos = cosf( w );
st_di_osc[ii].sin = sinf( w );
//---
*/


/*
//---
ii = cn_di_osc31;
if( phase_zero_IQ )
	{
	st_di_osc[ii].I = 1.0f;												//19KHz pilot tone (synthesis test)
	st_di_osc[ii].Q = 0.0f;
	}

w = twopi * (synth_38K_L_R_mux_freq0) / g_dev_bw;
st_di_osc[ii].cos = cosf( w );
st_di_osc[ii].sin = sinf( w );
//---
*/



synth_tone_freq_theta_inc0 = synth_tone_freq0 * twopi / g_dev_bw;		//this is also used for fm stereo left chan

synth_tone_freq_theta_right_chan_inc0 = synth_tone_freq1 * twopi / g_dev_bw;		//this is fm stereo right chan



synth_tone_freq_theta_inc1 = synth_tone_freq1 * twopi / g_dev_bw;
synth_tone_freq_theta_inc2 = synth_tone_freq2 * twopi / g_dev_bw;
synth_tone_freq_theta_inc3 = synth_tone_freq3 * twopi / g_dev_bw;

}




vector<int> vosc_active_idx;											//help avoid 'if' within 'for' loops which helps optimiser in 'inc dual integrator sine oscillators'


void osc_active_list_build()
{
vosc_active_idx.clear();

for( int i = 0; i < cn_dual_integrator_osc_cnt; i++ )
	{

	if( i == cn_di_osc0 )
		{
		if( wnd_rtl_graph->b_synth_voice_on_iq0 ) vosc_active_idx.push_back( i );	//carriers
		}

//	if( i == cn_di_osc1 )
//		{
//		if( wnd_rtl_graph->b_synth_voice_on_iq1 ) vosc_active_idx.push_back( i );
//		}

	if( i == cn_di_osc5 )
		{
		if( wnd_rtl_graph->b_synth_voice_on_iq1 ) vosc_active_idx.push_back( i );
		}

	if( i == cn_di_osc7 )
		{
		if( wnd_rtl_graph->b_synth_voice_on_iq2 ) vosc_active_idx.push_back( i );
		}

	if( i == cn_di_osc9 )
		{
		if( wnd_rtl_graph->b_synth_voice_on_iq3 ) vosc_active_idx.push_back( i );
		}

	if( i == cn_di_osc11 )
		{
		if( wnd_rtl_graph->b_synth_voice_on_iq4 ) vosc_active_idx.push_back( i );
		}

//	if( i == cn_di_osc13 )
//		{
//		if( wnd_rtl_graph->b_synth_voice_on_iq4 ) vosc_active_idx.push_back( i );
//		}

//	if( i == cn_di_osc15 )
//		{
//		if( 0 ) vosc_active_idx.push_back( i );
//		}

	if( i == cn_di_osc20 )												//tone
		{
		if( 1 ) vosc_active_idx.push_back( i );
		}

/*
	if( i == cn_di_osc30 )												//fm stereo 19KHz pilot tone
		{
		if( 1 ) vosc_active_idx.push_back( i );
		}

	if( i == cn_di_osc31 )												//fm stereo 38KHz L-R mux osc
		{
		if( 1 ) vosc_active_idx.push_back( i );
		}
*/
	}
	
}





int64_t g_freq_tune_last = g_freq_tune;
int64_t g_freq_sub_tune_last = g_freq_sub_tune;

double g_dev_bw_last = g_dev_bw;										//used to detect srate has changed an that some items my need to be rebuit

bool demod_iso_first_call = 1;



/*
const float fm_dev = 75e3;												//fm deviation 

const float fm_deviation_gain = 2.0f * M_PI * fm_dev / g_dev_bw;   		// deviation gain

// FM AFC loop parameters
float fm_stepI_ref = st_di_osc[cn_di_osc5].cos; 						// carrier step vector
float fm_stepQ_ref = st_di_osc[cn_di_osc5].sin;

float fm_tau_sec   = 2.0f;                       // AFC averaging time constant
float fm_beta      = 1.0f / (g_dev_bw * fm_tau_sec);      // IIR averaging factor
float fm_trim_gain = 0.02f;                      // AFC correction strength
float fm_slew_alpha = 1e-5f;                     // reference slew factor per sample



// FM Persistent state
float fm_oscI = 1.0f;       // NCO initial I
float fm_oscQ = 0.0f;       // NCO initial Q

float fm_stepI = fm_stepI_ref; // carrier step vector
float fm_stepQ = fm_stepQ_ref;

float fm_prevI = fm_oscI;      // previous oscillator sample
float fm_prevQ = fm_oscQ;

float fm_freq_err_avg = 0.0f;




struct st_fm_mod_tag
{
float dev;
float deviation_gain;
float stepI_ref;
float stepQ_ref;
float tau_sec;
float beta;
float trim_gain;
float slew_alpha;
float oscI;
float oscQ;
float stepI;
float stepQ;
float prevI;
float prevQ;
float freq_err_avg;

} st_fm[2];
*/


#define cn_fm_modulator_siz 5

struct st_fm_tag
{
float fs;        	// sample rate
float fc;        	// carrier freq (Hz)
float fdeviation;	// peak deviation (Hz)

float wc;        // carrier rad/sample
float kf;        // deviation rad/sample/unit

float phase;     // phase accumulator
} st_fm[ cn_fm_modulator_siz ];










bool fm_init( unsigned int idx, unsigned int srate_in, float freq, float deviation )
{
if( idx >= cn_fm_modulator_siz ) return 0;

st_fm_tag *s = st_fm + idx;

s->fs = srate_in;
s->fdeviation = s->fdeviation = deviation;
s->fc = freq;

s->wc = twopi * s->fc / s->fs;
s->kf = twopi * s->fdeviation / s->fs;
s->phase = 0.0f;

return 1;
}






//pre downsampler antialis filter
void elliplpf_init()
{

//-----
int order = 8;				//NOTE: coeffs rounded to float can cause issues with high order ellip filter designer as present here
int ripple = 1;
int stopband = -60;

string s1 = cns_ellip_lpf_zpk_proto_builtin_ini;
if( !elliplpf_cr.load_prototype_from_string( s1, order, ripple, stopband ) )
	{
	printf("demod_iso() - elliplpf_cr.load_prototype_from_string() - failed - no specified filter in ini string: '%s'\n", s1.c_str() );
	}

unsigned int srate = g_dev_bw;
unsigned int fc0 = aa_dwncnv_fc;
unsigned int fc1 = 0;
vector<double> vnumer;
vector<double> vdenom;

if( !elliplpf_cr.calc_iir_coeffs( 0, fc0, fc1, srate, vnumer, vdenom ) )
	{
	printf("demod_iso() - failed - elliplpf_cr.calc_iir_coeffs()\n" );
	}


printf("demod_iso() - setting: elliplpf_cr.calc_iir_coeffs( fc == %d ) \n", aa_dwncnv_fc );


elliplpf_cr.sos_init_float( vnumer, vdenom );			//NOTE: coeffs rounded to float can cause issues with high order ellip filter designer as present above
//-----


//-----
elliplpf_ci.sos_init_float( vnumer, vdenom );			//NOTE: coeffs rounded to float can cause issues with high order ellip filter designer as present above
//-----

}








//--->CALLED by audio proc thread

void demod_iso()
{

bool vb = 0;
bool vb1 = 0;											//verbose control

frame_time = 1.0f/aud_op_srate * framecnt;
int samples_per_frame = aud_op_srate * frame_time;

time_per_sample = 1.0f/g_dev_bw;


st_plot.srate_in = 0;
st_plot.vcpx0 = 0;														//these are pointers, clear them so as to detect if they have been set to a vector
st_plot.vcpx1 = 0;
st_plot.vsp0 = 0;
st_plot.vdble0 = 0;
st_plot.vflt0 = 0;
st_plot.vflt1 = 0;



theta_fm_19K_pilot_inc = synth_pilot_freq0 * twopi / g_dev_bw;





if( demod_iso_first_call )
	{
	demod_iso_first_call = 0;
	
	
	if( bf_synth ) delete[] bf_synth;
	
	synth_sample_cnt = cn_synth_rec_time * g_dev_bw * 2;
	bf_synth = new int16_t[synth_sample_cnt + 16 ];						//add extra bytes as a precaution

	if( b_synth_build_start ) b_synth_build_recording = 1;
	
	init_osc_integrators( 1, 1 );


//	theta_fm_19K_pilot_inc = synth_pilot_freq0 * twopi / g_dev_bw;
//	theta_fm_38K_mux_inc = synth_38K_L_R_mux_freq0 * twopi / g_dev_bw;

/*
	if (!elliplp_cr.make( 10000, 12000, 10, 2496000 ) )		//you CAN'T specify a different ellip filter spec here, coeffs are hardcoded and fixed to this filter shape, numbers just represent the hardcoded spec
		{
		printf("demod_iso() - failed to make elliplp_cr\n" );
		exit(0);
		}

	if (!elliplp_ci.make( 10000, 12000, 10, 2496000 ) )
		{
		printf("demod_iso() - failed to make elliplp_ci\n" );
		exit(0);
		}
*/
	elliplpf_init();


	fm_init( 0, g_dev_bw, synth_carr_freq0, 75000 );					//fm deviation in Hz
	fm_init( 1, g_dev_bw, synth_carr_freq1, 1500 );
	fm_init( 2, g_dev_bw, synth_carr_freq2, 1500 );
	fm_init( 3, g_dev_bw, synth_carr_freq3, 1500 );
	fm_init( 4, g_dev_bw, synth_carr_freq4, 1500 );

	
	stereo_pll_init( st_fm_decoder, g_dev_bw, downsample_srate );

	for( int i = 0; i < 16; i++ )
		{
		dcblk_bf[i] = 0.0f;		
		}	
	
//	dc_block3_devlp_dly0 = 0;
	


//	st_bline_remove.init( 0.3f, downsample_srate );    
	
//	agc_init( agc0, downsample_srate );

/*
	int ifm = 0;
	st_fm[ifm].stepI_ref = st_di_osc[cn_di_osc5].cos;
	st_fm[ifm].stepQ_ref = st_di_osc[cn_di_osc5].sin;

	st_fm[ifm].tau_sec   = 2.0f;                       // AFC averaging time constant
	st_fm[ifm].beta      = 1.0f / (g_dev_bw * fm_tau_sec);      // IIR averaging factor
	st_fm[ifm].trim_gain = 0.02f;                      // AFC correction strength
	st_fm[ifm].slew_alpha = 1e-5f;                     // reference slew factor per sample



	// FM Persistent state
	st_fm[ifm].oscI = 1.0f;       // NCO initial I
	st_fm[ifm].oscQ = 0.0f;       // NCO initial Q

	st_fm[ifm].stepI = st_fm[ifm].stepI_ref; // carrier step vector
	st_fm[ifm].stepQ = st_fm[ifm].stepQ_ref;

	st_fm[ifm].prevI = st_fm[ifm].oscI;      // previous oscillator sample
	st_fm[ifm].prevQ = st_fm[ifm].oscQ;

	st_fm[ifm].freq_err_avg = 0.0f;
*/
	}



if( downsample_srate != downsample_srate_last )							//detect user changed 'downsample_srate'
	{
	printf("demod_iso() - GGGGGGGGGGGGGGGGGG downsample_srate was changed, old: %d -->  new :%d\n", downsample_srate_last, downsample_srate );
	
	downsample_srate_last = downsample_srate;

	stereo_pll_init( st_fm_decoder, g_dev_bw, downsample_srate );
	
	
	float bwidth0 = 500.0f;
	st_aud_vu_lpf.alpha = twopi * bwidth0 / downsample_srate;

	}



if( i_deemphasis != i_deemphasis_last )									//detect user changed 'demphasis'
	{
	printf("demod_iso() - DDDDDDDDDDDDDDDD deemphasis was changed, old: %d -->  new :%d\n", i_deemphasis_last, i_deemphasis );
	stereo_deemphasis_init( st_fm_decoder );

	i_deemphasis_last = i_deemphasis;
	}



bool b_need_osc_all = 0;
if( g_dev_bw_last != g_dev_bw ) b_need_osc_all = 1;						//usr changed dev bandwidth?
if( b_need_osc_all )
	{
	g_dev_bw_last = g_dev_bw;
	init_osc_integrators( 0, 1 );									//adj all osc to correctly set test signal carrier freqs
	elliplpf_init();
	}

bool b_need_osc_adj = 0;

//---

//if( g_freq_tune != g_freq_tune_last ) b_need_osc_adj = 1;				//usr changed tune freq?
//g_freq_tune_last = g_freq_tune;

//if( b_need_osc_adj ) init_osc_integrators( 0 );							//adj osc rotation vals to new freqs
//---


//---

if( g_freq_sub_tune != g_freq_sub_tune_last ) b_need_osc_adj = 1;				//usr changed tune freq?
g_freq_sub_tune_last = g_freq_sub_tune;

//	if(!(demod_cnt%10) ) printf("demod_iso() - ZZZZZZZZZZZZZZzz g_freq_tune_cur %d\n", (int)g_freq_tune_cur);
if( b_need_osc_adj ) 
	{
//	printf("demod_iso() - ZZZZZZZZZZZZZZzz g_freq_tune_cur %d\n", (int)g_freq_tune_cur);
	float w = twopi * (g_freq_sub_tune_last) / g_dev_bw;
	st_di_osc[cn_di_osc15].cos = cosf( w );								//adj rotation rate
	st_di_osc[cn_di_osc15].sin = sinf( w );
	}
//---



////wnd_rtl_graph->b_synth_noise_on_iq = 1;


wnd_rtl_graph->b_synth_voice_on_iq0 = 1;								//morse
wnd_rtl_graph->b_synth_voice_on_iq1 = 1;
wnd_rtl_graph->b_synth_voice_on_iq2 = 1;
wnd_rtl_graph->b_synth_voice_on_iq3 = 1;
wnd_rtl_graph->b_synth_voice_on_iq4 = 1;
wnd_rtl_graph->b_synth_voice_on_iq5 = 0;
wnd_rtl_graph->b_synth_voice_on_iq6 = 0;

//wnd_rtl_graph->b_synth_iq = !wnd_rtl_graph->b_synth_iq;

if( wnd_rtl_graph->b_synth_iq ) osc_active_list_build();


if(b_dbg_skip_demod) 									//SKIP this function's code
	{
	if( !(demod_cnt % 10) ) vb = 1;
	if(vb) printf("demod_iso() - 'b_dbg_skip_demod' is set SKIPPING this function ******************** \n" );
	demod_cnt++;
	return;
	}

if( wnd_rtl_graph == 0 ) return;



if( sync_wr_rd_pointer != 0 )
	{
	if( sync_wr_rd_pointer == 2 )
		{
		sync_wr_rd_pointer = 3;
		}
	}


//if( pref_zero_padding_for_fft != pref_zero_padding_for_fft_last )		//has a change in padding pref occurred?
//	{
//	i_fftw_trig_plan_create_state = 0;	

//	pref_zero_padding_for_fft_last = pref_zero_padding_for_fft;
//	}


if( i_fftw_trig_plan_create_state == 0 ) i_fftw_trig_plan_create_state = 1;		//move to next state transition


if( !(demod_cnt % demod_cnt_modulo0) ) vb0 = 1;
else vb0 = 0;


//int samples_sec0 = g_dev_bw;




if( !(demod_cnt%25) ) gph_samples_per_sec = 0;							//clear periodically

//double d = tim1.time_passed( tim1.ns_tim_start );
//tim1.time_start( tim1.ns_tim_start );

//if(vb0) printf( "demod_iso() - tim1.time_passed = %g\n", d );


//printf( "demod_iso() - here0\n" );

if( ( !b_rtlsdr_callback_first ) && ( !wnd_rtl_graph->b_synth_iq ) ) return;


int demod_type = en_dmt_wfm;

filter_code::st_cplex_tag oc;


vector <filter_code::st_cplex_tag> viq_local;

vector <filter_code::st_cplex_tag> vdownsamp;
vector <filter_code::st_cplex_tag> vcplex_sp_for_demod;
vector <filter_code::st_cplex_tag> vcplex_sp_downsample;
vector <filter_code::st_cplex_tag> vcplex_tdm_for_demod;
vector <double> vpcm_cache;
vector <float> vpcm_cache_ch0;
vector <float> vpcm_cache_ch1;
vector<float> vpcm_ch0;
vector<float> vpcm_ch1;
vector<float> vempty_f0;


if ( wnd_rtl_graph )
	{
	demod_type = wnd_rtl_graph->demodul_mode;
	}


int iq_count_downsample, iq_count_downsample_widefm;
int iq_count = wnd_rtl_graph->iq_count_calc( iq_count_downsample, iq_count_downsample_widefm );


//int iq_count = 159744;
//iq_count = g_device_bw * ( (1.0f/srate) * framecnt); 					//rtl adc srate, e.g: 2496000 Byte/s x audio_proc_period (64mS[32KHz]) = 159744
																		//rtl adc srate, 3200000 Byte/s x audio_proc_period (64mS[32KHz]) = 204800
bool use_fft_filter = 0;

//if( demod_type == en_dmt_wfm ) 
//	{
//	use_fft_filter = 0;
//	downsample_factor = cn_down_sample_factor_small;
//	}
//else{
//	use_fft_filter = 1;
//	downsample_factor = cn_down_sample_factor_large;
//	}


//int downsample_count = iq_count / downsample_factor;

//if(1)printf( "demod_iso() - >>>>>>>>>>>>>>>>>>>> iq_count %d, downsample_factor %d, iq_count_downsample %d\n", iq_count, downsample_factor, iq_count_downsample );


//------------------ process IQ samples ------------------

//pthread_mutex_lock( &mutex2 );					//mutex as viq is being modified

if( gph_loc_sel == 0 )
	{
	}
																		
//if( g_device_bw == 2496000 ) iq_count = 159744;	
//if( g_device_bw == 3200000 ) iq_count = 204800;

//try to keep read ptr at a similar length behind write ptr by slewing read ptr's inc rate
uiwnd_low = (idbg_wr - cn_rtl_buf_size/2) - cn_rtl_buf_size/4;	//these window values will be above of 'cn_rtl_buf_size-1' very quickly as 'idbg_wr' and 'idbg_rd' are allow to increase indefinitly, so thay will pass 'uint64_t' largest poss number
uiwnd_high = (idbg_wr - cn_rtl_buf_size/2) + cn_rtl_buf_size/4;

//uiwnd_low2 = (idbg_wr - cn_rtl_buf_size/2) - cn_rtl_buf_size/8;	//these window values will be above of 'cn_rtl_buf_size-1' very quickly as 'idbg_wr' and 'idbg_rd' are allow to increase indefinitly, so thay will pass 'uint64_t' largest poss number
//uiwnd_high2 = (idbg_wr - cn_rtl_buf_size/2) + cn_rtl_buf_size/8;

float slew_factor = 0.8;

//if( b_rd_wr_recenter )													//request to recenter rd pointer ?
//	{
//	slew_factor *= 2.0f;
//	uiwnd_low = (idbg_wr - cn_rtl_buf_size/2) - cn_rtl_buf_size/16;	//these window values will be above of 'cn_rtl_buf_size-1' very quickly as 'idbg_wr' and 'idbg_rd' are allow to increase indefinitly, so thay will pass 'uint64_t' largest poss number
//	uiwnd_high = (idbg_wr - cn_rtl_buf_size/2) + cn_rtl_buf_size/16;

//	if( fabsf( delta_wr_rd ) < cn_rtl_buf_size/2 ) b_rd_wr_recenter = 0;
//	}

delta_wr_rd = idbg_wr - idbg_rd;										//result stored as an int64_t

if( demod_cnt == 0 )
	{
	printf( "demod_iso() --first call: uidbg_wr %" PRIu64 " idbg_rd %" PRIu64 " delta %" PRIi64 " wnd_low %" PRIu64 " wnd_high %" PRIu64 "\n", idbg_wr, idbg_rd, delta_wr_rd, uiwnd_low, uiwnd_high );
	}


inc_rate = 1.0f;

//if ( dbg_btn2 ) inc_rate -= 10.05;
//if ( dbg_btn3 ) inc_rate += 10.05;



if( idbg_rd < uiwnd_low ) 
	{
	int behind_by = idbg_wr - cn_rtl_buf_size/2 ;
	
	inc_rate += slew_factor;
	float inc_rate = (float)behind_by / iq_count;
	if( !wnd_rtl_graph->b_synth_iq ) printf( "demod_iso() --------------> inc by %f  uidbg_wr %" PRIu64 " idbg_rd %" PRIu64 " delta %" PRIi64 "\n", inc_rate, idbg_wr, idbg_rd, idbg_wr - idbg_rd );

	wnd_rtl_graph->iled_buf_rd_adj = 1000;		//trigger led
	}

if( idbg_rd > uiwnd_high ) 
	{
	inc_rate -= slew_factor;
	if( !wnd_rtl_graph->b_synth_iq ) printf( "demod_iso() --------------> dec by %f  idbg_wr %" PRIu64 " idbg_rd %" PRIu64 " delta %" PRIi64 "\n", inc_rate, idbg_wr, idbg_rd, idbg_wr - idbg_rd );
	wnd_rtl_graph->iled_buf_rd_adj = 1000;		//trigger led
	}

//if ( b_skip_inc ) 
//	{
//	printf( "demod() --------------> skip inc  uidbg_wr %" PRIu64 " uidbg_rd %" PRIu64 " delta %" PRIu64 "\n", uidbg_wr, uidbg_rd, uidbg_wr - uidbg_rd );
//	}

//printf( "demod_iso() - uidbg_wr %d uidbg_rd %" PRIu64 " delta %" PRIu64 "\n", uidbg_wr, uidbg_rd, uidbg_wr - uidbg_rd );

//for( int i = 0; i < 159744; i++ )						//rtl adc srate 2496000

double f_rd = rtl_bf0_rd;
double f_rd2 = idbg_rd;

//inc_rate = 1.0f;
//printf( "demod_iso() - wnd_low %" PRIi64 " wnd_high %" PRIi64 "\n", wnd_low, wnd_high );
if(vb)printf( "demod_iso() - start f_rd %f idbg_rd %" PRIi64 " delta_wr_rd %" PRIi64 "\n", f_rd, idbg_rd, delta_wr_rd );



if( ( iq_count == 0 ) && ( wnd_rtl_graph->b_synth_iq ) )
	{
	iq_count = 1024;	
	}


if( wnd_rtl_graph->b_synth_iq )
	{
	idbg_wr += g_dev_bw * frame_time;
	}



inc_rate2 = 0;

//printf("FFFFFFFFFFFFFFFFF synth_morse_tone2 %d\n", synth_morse_tone2 );


//======================================================================
for( int i = 0; i < iq_count; i++ )						//rtl adc srate, e.g adc rate of 2496000 samples/sec * frame time = 106496 samples per frame
	{
	filter_code::st_cplex_tag o;

	if( g_gain_iq == 0.0f )								//NOTE: see below where specific zeroing is req here, it handles -0.0 result cases, (although the compares here does distinguish between 0.0 and -0.0)
		{
		o.real = 0.0f;
		o.imag = 0.0f;
		}
	else{
		o.real = rtl_bfI0[ (int)f_rd ] * g_gain_iq;		//NOTE: if 'g_gain_iq' is 0.0f, you may get 'o.real and o.imag' being -0.0 or 0.0, which can affact 'atan2' results -
		o.imag = rtl_bfQ0[ (int)f_rd ] * g_gain_iq;		//NOTE: it results in bearly audible FM station when 'g_gain_iq' is 0.0, so the FM demodulator is able to demod audio when -0.0 I/Q vals are allowed to pass -														
														//NOTE: from -0.0 to 0.0 swings refer: https://stackoverflow.com/questions/9657993/how-to-convert-negative-zero-to-positive-zero-in-c
		}



	


//--------- no rtl dev, synthesise it by providing test signals ----------
	if( wnd_rtl_graph->b_synth_iq )
		{
		if(1)															//zero this to turn off modulators, allows checking cpu usage
			{
			if( !b_synth_build_playing )
				{
				int i_interfreq = 0;
			
				if( g_b_if_freq ) i_interfreq = g_interfreq;					//intermediate freq offset for carrier ?
				float freq0 = 10e6 + i_interfreq;								//1st carrier

				float freq1 = wnd_rtl_graph->synth_mod_1st_tone_freq;			//tones
				float freq2 = wnd_rtl_graph->synth_mod_2nd_tone_freq;

				float ampl1 = wnd_rtl_graph->synth_mod_1st_tone_ampl;
				float ampl2 = wnd_rtl_graph->synth_mod_2nd_tone_ampl;
			
	/*
				if( wnd_rtl_graph->morse_play_which != -1 )
					{
					ampl1 = 0.0f;												//turn off 1st tone modulation
					ampl2 = 0.0f;
					if( wnd_rtl_graph->morse_reset )							//user spec a reset?
						{
						if( wnd_rtl_graph->morse_play_which == 0 ) vmorse0 = vmorse1;
						if( wnd_rtl_graph->morse_play_which == 1 ) vmorse0 = vmorse10;

						wnd_rtl_graph->morse_idx = 0;
						wnd_rtl_graph->morse_time_note = vmorse0[0].dur;
						wnd_rtl_graph->morse_time_tot = 0.0f;
						wnd_rtl_graph->morse_reset = 0;
						}

					freq1 = vmorse0[ wnd_rtl_graph->morse_idx ].freq;

					float amp0 = vmorse0[ wnd_rtl_graph->morse_idx ].amp0;		//start ampl
					float amp1 = vmorse0[ wnd_rtl_graph->morse_idx ].amp1;		//stop ampl

					float ctrl_mix = wnd_rtl_graph->morse_time_note / vmorse0[ wnd_rtl_graph->morse_idx ].dur;		//1.0 at start of dur, 0.0 at end of dur
		//			if( wnd_rtl_graph->morse_idx == 0 ) printf("ctrl_mix %f    %f/%f\n", ctrl_mix, wnd_rtl_graph->morse_time_note , vmorse0[ wnd_rtl_graph->morse_idx ].dur );
					ampl1 = amp0*ctrl_mix + (amp1 * (1.0f-ctrl_mix));

		//			if( wnd_rtl_graph->morse_idx == 0 ) printf("ampl1 %f\n", ampl1 );

					wnd_rtl_graph->morse_time_note -= time_per_sample;			//play a musical note seq
					wnd_rtl_graph->morse_time_tot += time_per_sample;
					if( wnd_rtl_graph->morse_time_note <= 0.0f ) 
						{
						wnd_rtl_graph->morse_idx++;
		//				printf( ".........playing music %d   %f\n", wnd_rtl_graph->music_idx, wnd_rtl_graph->music_time_note ); 
						if( wnd_rtl_graph->morse_idx >= vmorse0.size() )
							{
							wnd_rtl_graph->morse_idx = 0;
							wnd_rtl_graph->morse_time_note = vmorse0[ wnd_rtl_graph->morse_idx ].dur;	
							}
						else{
							wnd_rtl_graph->morse_time_note = vmorse0[ wnd_rtl_graph->morse_idx ].dur;	
							}
						}
					}
	*/




				
				st_dual_integrator_osc_tag *osc = st_di_osc + cn_di_osc1;

				float tone_r0 = osc->I;



				float modulation00r = 0;
				float modulation00i = 0;
				float modulation_right_chan00r = 0;
				float modulation_right_chan00i = 0;

				float modulation10r = 0;
				float modulation10i = 0;
				float modulation20r = 0;
				float modulation20i = 0;
				float modulation30r = 0;
				float modulation30i = 0;
				float modulation40r = 0;
				float modulation40i = 0;
//				float modulation50 = 0;	
//				float modulation60 = 0;	


		/*
				if( wnd_rtl_graph->b_synth_voice_on_iq0 )
					{
					//---
					if( fract_ptr0 >= vaudclip0.size() ) fract_ptr0 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip


		//			int samples_per_frame = srate * frame_time;

		//printf("samples_per_frame %d\n", samples_per_frame );

					float interp_inc = (float)samples_per_frame/iq_count;
					
					int iii = floorf(fract_ptr0);
					float lin0 = vaudclip0[ iii ];								//get a sample for interpolation below
					
					int inext = iii + 1;
					if( inext >= vaudclip0.size() ) inext = iii;				//use same sample if past last audio sample
					
					float lin1 = vaudclip0[ inext ];
				
					float fint;
					float fract = modf( fract_ptr0, &fint );
					
					float lin2 = lin0 + (lin1-lin0)*fract;						//linear interpolation

					modulation10 = lin2;

					fract_ptr0 += interp_inc;
					//---
					}
		*/


				//zero order sample and hold srate conversion
				if( wnd_rtl_graph->b_synth_voice_on_iq0 )
					{
					switch( synth_morse_tone0 )
						{
						case 0:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr00 >= vc_audclip0.size() ) fract_ptr00 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip


							modulation00r = vc_audclip0[ (int)fract_ptr00 ].real;
							modulation00i = vc_audclip0[ (int)fract_ptr00 ].imag;

							float interp_inc = (float)samples_per_frame/iq_count;
							fract_ptr00 += interp_inc;
							}
						break;
						
						
						case 1:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr00 >= vc_audclip1.size() ) fract_ptr00 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip


							modulation00r = vc_audclip1[ (int)fract_ptr00 ].real;
							modulation00i = vc_audclip1[ (int)fract_ptr00 ].imag;

							float interp_inc = (float)samples_per_frame/iq_count;
							fract_ptr00 += interp_inc;
							}
						
						break;

						case 2:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr00 >= vc_audclip2.size() ) fract_ptr00 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip


							modulation00r = vc_audclip2[ (int)fract_ptr00 ].real;
							modulation00i = vc_audclip2[ (int)fract_ptr00 ].imag;

							float interp_inc = (float)samples_per_frame/iq_count;
							fract_ptr00 += interp_inc;
							}
						
						break;

						case 3:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr00 >= vc_audclip3.size() ) fract_ptr00 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip


							modulation00r = vc_audclip3[ (int)fract_ptr00 ].real;
							modulation00i = vc_audclip3[ (int)fract_ptr00 ].imag;

							float interp_inc = (float)samples_per_frame/iq_count;
							fract_ptr00 += interp_inc;
							}
						
						break;

						case 4:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr00 >= vc_audsynth0.size() ) fract_ptr00 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip

							float interp_inc = (float)samples_per_frame/iq_count;

							modulation00r = vc_audsynth0[ (int)fract_ptr00 ].real;
							modulation00i = vc_audsynth0[ (int)fract_ptr00 ].imag;

							fract_ptr00 += interp_inc;
							}
						break;

						case 5:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr00 >= vc_audsynth1.size() ) fract_ptr00 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip

							float interp_inc = (float)samples_per_frame/iq_count;

							modulation00r = vc_audsynth1[ (int)fract_ptr00 ].real;
							modulation00i = vc_audsynth1[ (int)fract_ptr00 ].imag;

							fract_ptr00 += interp_inc;
							}
						break;

						case 6:
							{
							float s, c;
							
							sincosf( synth_tone_freq_theta0, &s, &c );
							modulation00r = c;
							modulation00i = s;
							synth_tone_freq_theta0 += synth_tone_freq_theta_inc0;
							
							if( synth_tone_freq_theta0 >= twopi ) synth_tone_freq_theta0 -= twopi;
//							if( synth_tone_freq_theta0 <= -twopi ) synth_tone_freq_theta0 += twopi;

							}
						break;

						case 7:
							{
							float s, c;
							
							sincosf( synth_tone_freq_theta0, &s, &c );
							modulation00r = c;
							modulation00i = s;

							synth_tone_freq_theta0 += synth_tone_freq_theta_inc0;
							if( synth_tone_freq_theta0 >= twopi ) synth_tone_freq_theta0 -= twopi;

							sincosf( synth_tone_freq_theta_right_chan0, &s, &c );
							modulation_right_chan00r = c;
							modulation_right_chan00i = s;
							
							synth_tone_freq_theta_right_chan0 += synth_tone_freq_theta_right_chan_inc0;
							if( synth_tone_freq_theta_right_chan0 >= twopi ) synth_tone_freq_theta_right_chan0 -= twopi;

//							if( synth_tone_freq_theta0 <= -twopi ) synth_tone_freq_theta0 += twopi;
							}
						break;


						case 8:
							{
							float s, c;

							if( fract_ptr00 >= vc_audclip2.size() ) fract_ptr00 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip


							modulation00r = vc_audclip2[ (int)fract_ptr00 ].real;
							modulation00i = vc_audclip2[ (int)fract_ptr00 ].imag;

							float interp_inc = (float)samples_per_frame/iq_count;
							fract_ptr00 += interp_inc;



							if( fract_ptr000 >= vc_audclip3.size() ) fract_ptr000 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip

							modulation_right_chan00r = vc_audclip3[ (int)fract_ptr000 ].real;
							modulation_right_chan00i = vc_audclip3[ (int)fract_ptr000 ].imag;

							fract_ptr000 += interp_inc;
							}
						break;


						case 9:											//so modulation
							{
							}
						break;
					
						}
					}


					switch( synth_morse_tone1 )
						{
						case 0:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr10 >= vc_audclip0.size() ) fract_ptr10 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip


							modulation10r = vc_audclip0[ (int)fract_ptr10 ].real;
							modulation10i = vc_audclip0[ (int)fract_ptr10 ].imag;

							float interp_inc = (float)samples_per_frame/iq_count;
							fract_ptr10 += interp_inc;
							}
						break;
						
						
						case 1:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr10 >= vc_audclip1.size() ) fract_ptr10 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip


							modulation10r = vc_audclip1[ (int)fract_ptr10 ].real;
							modulation10i = vc_audclip1[ (int)fract_ptr10 ].imag;

							float interp_inc = (float)samples_per_frame/iq_count;
							fract_ptr10 += interp_inc;
							}
						
						break;
						

						case 2:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr10 >= vc_audclip2.size() ) fract_ptr10 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip

							float interp_inc = (float)samples_per_frame/iq_count;

							modulation10r = vc_audclip2[ (int)fract_ptr10 ].real;
							modulation10i = vc_audclip2[ (int)fract_ptr10 ].imag;

							fract_ptr10 += interp_inc;
							}
						break;


						case 3:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr10 >= vc_audclip3.size() ) fract_ptr10 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip

							float interp_inc = (float)samples_per_frame/iq_count;

							modulation10r = vc_audclip3[ (int)fract_ptr10 ].real;
							modulation10i = vc_audclip3[ (int)fract_ptr10 ].imag;

							fract_ptr10 += interp_inc;
							}
						break;

						case 4:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr10 >= vc_audsynth0.size() ) fract_ptr10 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip

							float interp_inc = (float)samples_per_frame/iq_count;

							modulation10r = vc_audsynth0[ (int)fract_ptr10 ].real;
							modulation10i = vc_audsynth0[ (int)fract_ptr10 ].imag;

							fract_ptr10 += interp_inc;
							}
						break;

						case 5:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr10 >= vc_audsynth1.size() ) fract_ptr10 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip

							float interp_inc = (float)samples_per_frame/iq_count;

							modulation10r = vc_audsynth1[ (int)fract_ptr10 ].real;
							modulation10i = vc_audsynth1[ (int)fract_ptr10 ].imag;

							fract_ptr10 += interp_inc;
							}
						break;


						case 6:
							{
							float s, c;
							
							sincosf( synth_tone_freq_theta1, &s, &c );
							modulation10r = c;
							modulation10i = s;
							synth_tone_freq_theta1 += synth_tone_freq_theta_inc1;
							
							if( synth_tone_freq_theta1 >= twopi ) synth_tone_freq_theta1 -= twopi;

							}
						break;

						}




					switch( synth_morse_tone2 )
						{
						case 0:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr20 >= vc_audclip0.size() ) fract_ptr20 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip


							modulation20r = vc_audclip0[ (int)fract_ptr20 ].real;
							modulation20i = vc_audclip0[ (int)fract_ptr20 ].imag;

							float interp_inc = (float)samples_per_frame/iq_count;
							fract_ptr20 += interp_inc;
							}
						break;
						
						
						case 1:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr20 >= vc_audclip1.size() ) fract_ptr20 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip


							modulation20r = vc_audclip1[ (int)fract_ptr20 ].real;
							modulation20i = vc_audclip1[ (int)fract_ptr20 ].imag;

							float interp_inc = (float)samples_per_frame/iq_count;
							fract_ptr20 += interp_inc;
							}
						
						break;
						

						case 2:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr20 >= vc_audclip2.size() ) fract_ptr20 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip

							float interp_inc = (float)samples_per_frame/iq_count;

							modulation20r = vc_audclip2[ (int)fract_ptr20 ].real;
							modulation20i = vc_audclip2[ (int)fract_ptr20 ].imag;

							fract_ptr20 += interp_inc;
							}
						break;


						case 3:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr20 >= vc_audclip3.size() ) fract_ptr20 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip

							float interp_inc = (float)samples_per_frame/iq_count;

							modulation20r = vc_audclip3[ (int)fract_ptr20 ].real;
							modulation20i = vc_audclip3[ (int)fract_ptr20 ].imag;

							fract_ptr20 += interp_inc;
							}
						break;

						case 4:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr20 >= vc_audsynth0.size() ) fract_ptr20 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip

							float interp_inc = (float)samples_per_frame/iq_count;

							modulation20r = vc_audsynth0[ (int)fract_ptr20 ].real;
							modulation20i = vc_audsynth0[ (int)fract_ptr20 ].imag;

							fract_ptr20 += interp_inc;
							}
						break;

						case 5:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr20 >= vc_audsynth1.size() ) fract_ptr20 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip

							float interp_inc = (float)samples_per_frame/iq_count;

							modulation20r = vc_audsynth1[ (int)fract_ptr20 ].real;
							modulation20i = vc_audsynth1[ (int)fract_ptr20 ].imag;

							fract_ptr20 += interp_inc;
							}
						break;

						case 6:
							{
							float s, c;
							
							sincosf( synth_tone_freq_theta2, &s, &c );
							modulation20r = c;
							modulation20i = s;
							synth_tone_freq_theta2 += synth_tone_freq_theta_inc2;
							
							if( synth_tone_freq_theta2 >= twopi ) synth_tone_freq_theta2 -= twopi;

							}
						break;

						}







					switch( synth_morse_tone3 )
						{
						case 0:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr30 >= vc_audclip0.size() ) fract_ptr30 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip


							modulation30r = vc_audclip0[ (int)fract_ptr30 ].real;
							modulation20i = vc_audclip0[ (int)fract_ptr30 ].imag;

							float interp_inc = (float)samples_per_frame/iq_count;
							fract_ptr30 += interp_inc;
							}
						break;
						
						
						case 1:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr30 >= vc_audclip1.size() ) fract_ptr30 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip


							modulation30r = vc_audclip1[ (int)fract_ptr30 ].real;
							modulation30i = vc_audclip1[ (int)fract_ptr30 ].imag;

							float interp_inc = (float)samples_per_frame/iq_count;
							fract_ptr30 += interp_inc;
							}
						
						break;
						

						case 2:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr30 >= vc_audclip2.size() ) fract_ptr30 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip

							float interp_inc = (float)samples_per_frame/iq_count;

							modulation30r = vc_audclip2[ (int)fract_ptr30 ].real;
							modulation30i = vc_audclip2[ (int)fract_ptr30 ].imag;

							fract_ptr30 += interp_inc;
							}
						break;


						case 3:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr30 >= vc_audclip3.size() ) fract_ptr30 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip

							float interp_inc = (float)samples_per_frame/iq_count;

							modulation30r = vc_audclip3[ (int)fract_ptr30 ].real;
							modulation30i = vc_audclip3[ (int)fract_ptr30 ].imag;

							fract_ptr30 += interp_inc;
							}
						break;

						case 4:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr30 >= vc_audsynth0.size() ) fract_ptr30 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip

							float interp_inc = (float)samples_per_frame/iq_count;

							modulation30r = vc_audsynth0[ (int)fract_ptr30 ].real;
							modulation30i = vc_audsynth0[ (int)fract_ptr30 ].imag;

							fract_ptr30 += interp_inc;
							}
						break;

						case 5:
							{
							//zero order sample and hold srate conversion
							if( fract_ptr30 >= vc_audsynth1.size() ) fract_ptr30 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip

							float interp_inc = (float)samples_per_frame/iq_count;

							modulation30r = vc_audsynth1[ (int)fract_ptr30 ].real;
							modulation30i = vc_audsynth1[ (int)fract_ptr30 ].imag;

							fract_ptr30 += interp_inc;
							}
						break;

						case 6:
							{
							float s, c;
							
							sincosf( synth_tone_freq_theta3, &s, &c );
							modulation30r = c;
							modulation30i = s;
							synth_tone_freq_theta3 += synth_tone_freq_theta_inc3;
							
							if( synth_tone_freq_theta3 >= twopi ) synth_tone_freq_theta3 -= twopi;

							}
						break;

						
						case 7:
							{
							float s, c;
		
							modulation30r = 0.0f;
							modulation30i = 0.0f;
							}
						break;

						}


/*

				//zero order sample and hold srate conversion
				if( wnd_rtl_graph->b_synth_voice_on_iq1 )
					{
					
					if( fract_ptr10 >= vc_audclip3.size() ) fract_ptr10 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip

					float interp_inc = (float)samples_per_frame/iq_count;

					modulation10r = vc_audclip3[ (int)fract_ptr10 ].real;
					modulation10i = vc_audclip3[ (int)fract_ptr10 ].imag;

					fract_ptr10 += interp_inc;
					}
*/


				//zero order sample and hold srate conversion

/*
				if( wnd_rtl_graph->b_synth_voice_on_iq2 )
					{
					if( fract_ptr20 >= vc_audclip4.size() ) fract_ptr20 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip

					float interp_inc = (float)samples_per_frame/iq_count;

					modulation20r = vc_audclip4[ (int)fract_ptr20 ].real;
					modulation20i = vc_audclip4[ (int)fract_ptr20 ].imag;

					fract_ptr20 += interp_inc;
					}


				//zero order sample and hold srate conversion
				if( wnd_rtl_graph->b_synth_voice_on_iq3 )
					{
					if( fract_ptr30 >= vc_audclip3.size() ) fract_ptr30 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip

					float interp_inc = (float)samples_per_frame/iq_count;

					modulation30r = vc_audclip3[ (int)fract_ptr30 ].real;
					modulation30i = vc_audclip3[ (int)fract_ptr30 ].imag;

					fract_ptr30 += interp_inc;
					}



				//zero order sample and hold srate conversion
				if( wnd_rtl_graph->b_synth_voice_on_iq4 )
					{
					if( fract_ptr40 >= vc_audclip3.size() ) fract_ptr40 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip

					float interp_inc = (float)samples_per_frame/iq_count;

					modulation40r = vc_audclip3[ (int)fract_ptr40 ].real;
					modulation40i = vc_audclip3[ (int)fract_ptr40 ].imag;

					fract_ptr40 += interp_inc;
					}
*/

		/*
				if( wnd_rtl_graph->b_synth_voice_on_iq1 )
					{
					//---
					if( fract_ptr1 >= vaudclip1.size() ) fract_ptr1 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip


					int samples_per_frame = srate * frame_time;

		//printf("samples_per_frame %d\n", samples_per_frame );

					float interp_inc = (float)samples_per_frame/iq_count;
					
					int iii = floorf(fract_ptr1);
					float lin0 = vaudclip1[ iii ];								//get a sample for interpolation below

					int inext = iii + 1;
					if( inext >= vaudclip1.size() ) inext = iii;				//use same sample if past last audio sample
					
					float lin1 = vaudclip1[ inext ];
				
					float fint;
					float fract = modf( fract_ptr1, &fint );
					
					float lin2 = lin0 + (lin1-lin0)*fract;						//linear interpolation

					modulation20 = lin2;

					fract_ptr1 += interp_inc;
					//---
					}


				if( wnd_rtl_graph->b_synth_voice_on_iq2 )
					{
					//---
					if( fract_ptr2 >= vaudsynth0.size() ) fract_ptr2 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip


					int samples_per_frame = srate * frame_time;

		//printf("samples_per_frame %d\n", samples_per_frame );

					float interp_inc = (float)samples_per_frame/iq_count;
					
					int iii = floorf(fract_ptr2);
					float lin0 = vaudsynth0[ iii ];								//get a sample for interpolation below
					
					int inext = iii + 1;
					if( inext >= vaudsynth0.size() ) inext = iii;				//use same sample if past last audio sample
					
					float lin1 = vaudsynth0[ inext ];

					float fint;
					float fract = modf( fract_ptr2, &fint );
					
					float lin2 = lin0 + (lin1-lin0)*fract;						//linear interpolation

					modulation30 = lin2;

					fract_ptr2 += interp_inc;
					//---
					}



				if( wnd_rtl_graph->b_synth_voice_on_iq3 )
					{
					//---
					if( fract_ptr3 >= vaudsynth1.size() ) fract_ptr3 = 0;		//hard rezero here rather than maintaining a fraction start of audio clip


					int samples_per_frame = srate * frame_time;

		//printf("samples_per_frame %d\n", samples_per_frame );

					float interp_inc = (float)samples_per_frame/iq_count;
					
					int iii = floorf(fract_ptr3);
					float lin0 = vaudsynth1[ iii ];								//get a sample for interpolation below
					
					int inext = iii + 1;
					if( inext >= vaudsynth1.size() ) inext = iii;				//use same sample if past last audio sample
					
					float lin1 = vaudsynth1[ inext ];
				
					float fint;
					float fract = modf( fract_ptr3, &fint );
					
					float lin2 = lin0 + (lin1-lin0)*fract;						//linear interpolation

					modulation40 = lin2;

					fract_ptr3 += interp_inc;
					//---
					}




				if( wnd_rtl_graph->b_synth_voice_on_iq4 )
					{
		//			modulation50 = tone_r3 + tone_r4;
		//			modulation60 = tone_r5 + tone_r6;
					}
		*/

		//if(!(i%100))printf("interp_inc %f  fract %f  fidx_af0 %f af0.sizech0 %d\n", interp_inc, fract, fract_ptr1, af0.sizech0 );

					
		//			}
		//		else{
		//			modulation10 = modulation;									//no voice?, apply alternate modulation
		//			modulation20 = modulation;									//no voice?, apply alternate modulation
		//			}



		//		if( wnd_rtl_graph->synth_mod_type == en_mdt_fm )
		//			{
		//			pi*0.50f * modulation2
		//			}


				float osc_sumr0 = 0;
				float osc_sumi0 = 0;
	//			float osc_i00;
	//			float osc_r10;
	//			float osc_i10;
	//			float osc_r20;
	//			float osc_i20;
	//			float osc_r30;
	//			float osc_i30;
	//			float osc_r40;
	//			float osc_i40;
	//			float osc_r50;
	//			float osc_i50;
	//			float osc_r60;
	//			float osc_i60;

				float rfcarr_ampl = 1.0f;
	//			float ampl0 = rfcarr_ampl;									//default carrier ampl
	//			float ampl_mod00r = 0.0f;									//for am modulation
	//			float ampl_mod00i = 0.0f;									//for am modulation

				float ampl10 = rfcarr_ampl;									//default carrier ampl
//				float ampl_mod10r = 0.0f;									//for am modulation
//				float ampl_mod10i = 0.0f;									//for am modulation

				float ampl20 = rfcarr_ampl;									//default carrier ampl
//				float ampl_mod20r = 0.0f;									//for am modulation
//				float ampl_mod20i = 0.0f;									//for am modulation

				float ampl30 = rfcarr_ampl;									//default carrier ampl
//				float ampl_mod30r = 0.0f;									//for am modulation
//				float ampl_mod30i = 0.0f;									//for am modulation

				float ampl40 = rfcarr_ampl;									//default carrier ampl
//				float ampl_mod40 = 0.0f;									//for am modulation

				float ampl50 = rfcarr_ampl;									//default carrier ampl
//				float ampl_mod50 = 0.0f;									//for am modulation

				float ampl60 = rfcarr_ampl;									//default carrier ampl
//				float ampl_mod60 = 0.0f;									//for am modulation

				//am, lsband, usband
				if( ( wnd_rtl_graph->synth_mod_type == en_mdt_am ) || ( wnd_rtl_graph->synth_mod_type == en_mdt_lsb ) || ( wnd_rtl_graph->synth_mod_type == en_mdt_usb ) )
					{
					if( wnd_rtl_graph->synth_mod_type == en_mdt_am )
						{
						float modul_depth = 1.0f;
	//					ampl_mod00r = modul_depth * modulation00r;
	//					ampl_mod00i = modul_depth * modulation00i;
//						ampl_mod10r = modul_depth * modulation10r;
//						ampl_mod10i = modul_depth * modulation10i;
//						ampl_mod20 = modul_depth * modulation20;
//						ampl_mod30 = modul_depth * modulation30;
//						ampl_mod40 = modul_depth * modulation40;
//						ampl_mod50 = modul_depth * modulation50;
//						ampl_mod60 = modul_depth * modulation60;
						
		//				osc_r0 += osc_r0 * 0.3f*(modulation);					//apply am modulation
		//				osc_i0 += osc_i0 * 0.3f*(modulation);


		//				osc_r10 += osc_r10 * 0.3f*(modulation2);				//apply am modulation
		//				osc_i10 += osc_i10 * 0.3f*(modulation2);

	//					if( synth_carr_lsb_usb_fm_0 == 1 )
	//						{
		//					osc_r0 *= 0.3f*(modulation);						//apply am modulation using dsbsc
		//					osc_i0 *= 0.3f*(modulation);

		//					osc_r10 *= 0.3f*(modulation2);						//apply am modulation using dsbsc
		//					osc_i10 *= 0.3f*(modulation2);
	//						}
						}
					}

		//if(!(i%100))printf("wnd_rtl_graph->synth_mod_type %d\n", wnd_rtl_graph->synth_mod_type );

		//---- fm
	//			float fm_mod0 = 0;
	//			float fm_mod10 = 0;
	//			float fm_mod20 = 0;
	//			float fm_mod30 = 0;
	//			float fm_mod40 = 0;
	//			float fm_mod50 = 0;
	//			if( wnd_rtl_graph->synth_mod_type == en_mdt_fm )
	//				{
	//				fm_mod0 = 3000.0f*modulation00;
	//				fm_mod10 = 3000.0f*modulation10;
	//				fm_mod20 = 3000.0f*modulation20;
	//				fm_mod30 = 3000.0f*modulation30;
	//				fm_mod40 = 3000.0f*modulation40;
	//				fm_mod50 = 3000.0f*modulation50;
	//				}
		//----

	//			freq0 -= g_freq_tune;											//remove carrier freq		
	//			float theta_inc0 = (freq0 + fm_mod0) * twopi * time_per_sample;				//carrier0 		


				//apply modulations
	//			if( ( wnd_rtl_graph->synth_mod_type == en_mdt_am ) || ( wnd_rtl_graph->synth_mod_type == en_mdt_lsb ) || ( wnd_rtl_graph->synth_mod_type == en_mdt_usb ) )
	//				{

	//				if( wnd_rtl_graph->synth_mod_type == en_mdt_am )
	//					{
						switch( synth_carr_lsb_usb_fm_0 )
							{
							case 0:																//no carrier
	//							osc_sumr0 += 0;													//build am carriers
	//							osc_sumr0 += 0;
							break;


							case 1:																//AM
								{
								float ampl00 = 1.0f;											//default carrier ampl							
								float modul_depth = 1.0f;

								float ampl_mod00r = modul_depth * modulation00r;				//use real component only
	//							float ampl_mod00i = modul_depth * modulation00i;
//								osc_sumr0 += synth_tone_dcoffs0 + (ampl00 + ampl_mod00r) * st_di_osc[cn_di_osc0].I;		//use 'real' for both i/q
//								osc_sumi0 += synth_tone_dcoffs0 + (ampl00 + ampl_mod00r) * st_di_osc[cn_di_osc0].Q;		//use 'real' for both i/q
								osc_sumr0 += (ampl00 + ampl_mod00r) * st_di_osc[cn_di_osc0].I;		//use 'real' for both i/q
								osc_sumi0 += (ampl00 + ampl_mod00r) * st_di_osc[cn_di_osc0].Q;		//use 'real' for both i/q
								}
							break;
							

							case 2:																//lwrSB
								{
								float modul_depth = 1.0f;
								float ampl_mod00r = modul_depth * modulation00r;
								float ampl_mod00i = modul_depth * modulation00i;
								float r0 = ampl_mod00r * 0.5f;									//keep demod spectral height at same level by having modul level
								float i0 = ampl_mod00i * 0.5f;
								osc_sumr0 += r0 * st_di_osc[cn_di_osc0].I - i0 * st_di_osc[cn_di_osc0].Q;
								osc_sumi0 += r0 * st_di_osc[cn_di_osc0].Q + i0 * st_di_osc[cn_di_osc0].I;
								}
							break;

							case 3:																//uprSB
								{
								float modul_depth = 1.0f;
								float ampl_mod00r = modul_depth * modulation00r;
								float ampl_mod00i = modul_depth * modulation00i;
								float r0 = ampl_mod00r * 0.5f;									//keep demod spectral height at same level by having modul level
								float i0 = ampl_mod00i * 0.5f;
								osc_sumr0 += r0 * st_di_osc[cn_di_osc0].I + i0 * st_di_osc[cn_di_osc0].Q;
								osc_sumi0 += r0 * st_di_osc[cn_di_osc0].Q - i0 * st_di_osc[cn_di_osc0].I;
								}
							break;

							case 4:																//FM
								{
								float modul_depth = 1.0f;
								float ampl_mod00r = modul_depth * modulation00r;
								float ampl_mod00i = modul_depth * modulation00i;

								float modul = ampl_mod00r;

								int ifm = 0;
								st_fm[ifm].phase += st_fm[ifm].wc + st_fm[ifm].kf * modul;		//calc phase update
								
								float s, c;
								sincosf(st_fm[ifm].phase, &s, &c);

								osc_sumr0 += c;
								osc_sumi0 += s;
								
								if(st_fm[ifm].phase >  pi) st_fm[ifm].phase -= twopi;
								if(st_fm[ifm].phase < -pi) st_fm[ifm].phase += twopi;
								}
							break;

							case 5:																//FM Stereo
								{
								


								float modul_depth = 0.9f;
								float mod_l_plus_r = 1.0f * ( modul_depth*modulation00r + modul_depth*modulation_right_chan00r );			//L+R

//								float ampl_mod00i = modul_depth * modulation00i;

//								ampl_mod00r += modul_depth * modulation_right_chan00r;
//								ampl_mod00i += modul_depth * modulation_right_chan00i;

								float mod_l_minus_r = 1.0f * ( modul_depth*modulation00r - modul_depth*modulation_right_chan00r );			//L-R
//								ampl_mod00i += modul_depth * modulation_right_chan00i;

//	if( (!(demod_cnt%50)) && ( i == 0 ))printf( "SSSSSSSSSSSSSSSSSSSSSSSss demod_iso() - st_di_osc[cn_di_osc30].I %f\n", st_di_osc[cn_di_osc30].I );

//								float modul = ampl_mod00r + 1.25f*st_di_osc[cn_di_osc30].I;		//19KHz pilot tone

//								float f19 = cosf( theta_fm_19K_pilot + pi*( 0/100.0f ) );
//								float f38 = cosf( theta_fm_19K_pilot + pi*( 0/100.0f )*2.0f );
								float f19 = cosf( theta_fm_19K_pilot );
								float f38 = cosf( theta_fm_19K_pilot * 2.0 );
								
								float modul = (mod_l_plus_r / 2.0f) + ( synth_pilot_gain0 * f19 );		//L+R and 19KHz pilot tone with 9% deviation when using +/-75KHz deviation max

								modul += (mod_l_minus_r / 2.0f) * f38;									//L-R tone muxed to 38KHz


//								float modul2 = ampl_mod00r + synth_pilot_gain0 * st_di_osc[cn_di_osc31].I;					//38KHz L-R mux with 9% deviation when using +/-75KHz deviation max

								int ifm = 0;
								st_fm[ifm].phase += st_fm[ifm].wc + st_fm[ifm].kf * modul;		//calc phase update
								
								float s, c;
								sincosf(st_fm[ifm].phase, &s, &c);

								
								osc_sumr0 += c;
								osc_sumi0 += s;
								
								if(st_fm[ifm].phase >  pi) st_fm[ifm].phase -= twopi;
								if(st_fm[ifm].phase < -pi) st_fm[ifm].phase += twopi;


								theta_fm_19K_pilot += theta_fm_19K_pilot_inc;
								if( theta_fm_19K_pilot >= twopi) theta_fm_19K_pilot -= twopi;

//								theta_fm_38K_mux += theta_fm_38K_mux_inc;
//								if( theta_fm_38K_mux >= twopi) theta_fm_38K_mux -= twopi;
								}
							break;

							case 6:																//FM Stereo (no pilot)
								{

								float modul_depth = 0.9f;
								float mod_l_plus_r = 1.0f * ( modul_depth*modulation00r + modul_depth*modulation_right_chan00r );			//L+R

//								float ampl_mod00i = modul_depth * modulation00i;

//								ampl_mod00r += modul_depth * modulation_right_chan00r;
//								ampl_mod00i += modul_depth * modulation_right_chan00i;

//								float mod_l_minus_r = 1.0f * ( modul_depth*modulation00r - modul_depth*modulation_right_chan00r );			//L-R
//								ampl_mod00i += modul_depth * modulation_right_chan00i;

//	if( (!(demod_cnt%50)) && ( i == 0 ))printf( "SSSSSSSSSSSSSSSSSSSSSSSss demod_iso() - st_di_osc[cn_di_osc30].I %f\n", st_di_osc[cn_di_osc30].I );

//								float modul = ampl_mod00r + 1.25f*st_di_osc[cn_di_osc30].I;		//19KHz pilot tone

//								float f19 = cosf( theta_fm_19K_pilot + pi*( 0/100.0f ) );
//								float f38 = cosf( theta_fm_19K_pilot + pi*( 0/100.0f )*2.0f );
//								float f19 = cosf( theta_fm_19K_pilot );
//								float f38 = cosf( theta_fm_19K_pilot * 2.0 );
								
								float modul = (mod_l_plus_r / 2.0f);					//L+R

//								modul += (mod_l_minus_r / 2.0f) * f38;									//L-R tone muxed to 38KHz


//								float modul2 = ampl_mod00r + synth_pilot_gain0 * st_di_osc[cn_di_osc31].I;					//38KHz L-R mux with 9% deviation when using +/-75KHz deviation max

								int ifm = 0;
								st_fm[ifm].phase += st_fm[ifm].wc + st_fm[ifm].kf * modul;		//calc phase update
								
								float s, c;
								sincosf(st_fm[ifm].phase, &s, &c);

								
								osc_sumr0 += c;
								osc_sumi0 += s;
								
								if(st_fm[ifm].phase >  pi) st_fm[ifm].phase -= twopi;
								if(st_fm[ifm].phase < -pi) st_fm[ifm].phase += twopi;


								theta_fm_19K_pilot += theta_fm_19K_pilot_inc;
								if( theta_fm_19K_pilot >= twopi) theta_fm_19K_pilot -= twopi;

//								theta_fm_38K_mux += theta_fm_38K_mux_inc;
//								if( theta_fm_38K_mux >= twopi) theta_fm_38K_mux -= twopi;




/*
								float modul_depth = 1.0f;
								float ampl_mod00r = modul_depth * modulation00r;
								float ampl_mod00i = modul_depth * modulation00i;

//	if( (!(demod_cnt%50)) && ( i == 0 ))printf( "SSSSSSSSSSSSSSSSSSSSSSSss demod_iso() - st_di_osc[cn_di_osc30].I %f\n", st_di_osc[cn_di_osc30].I );

//								float modul = ampl_mod00r + 1.25f*st_di_osc[cn_di_osc30].I;		//19KHz pilot tone
								float modul = ampl_mod00r;// + 0.09*st_di_osc[cn_di_osc30].I;		//19KHz pilot tone with 9% deviation when using +/-75KHz deviation max

								int ifm = 0;
								st_fm[ifm].phase += st_fm[ifm].wc + st_fm[ifm].kf * modul;		//calc phase update
								
								float s, c;
								sincosf(st_fm[ifm].phase, &s, &c);

								
								osc_sumr0 += c;
								osc_sumi0 += s;
								
								if(st_fm[ifm].phase >  pi) st_fm[ifm].phase -= twopi;
								if(st_fm[ifm].phase < -pi) st_fm[ifm].phase += twopi;
*/
								}

							break;

							}



						switch( synth_carr_lsb_usb_fm_1 )
							{
							case 0:																//AM
	//							osc_sumr0 += 0;													//build am carriers
	//							osc_sumr0 += 0;
							break;

							case 1:																//AM
								{
								float ampl10 = 1.0f;											//default carrier ampl							
								float modul_depth = 1.0f;

								float ampl_mod10r = modul_depth * modulation10r;				//use real component only
	//							float ampl_mod00i = modul_depth * modulation00i;
								osc_sumr0 += (ampl10 + ampl_mod10r) * st_di_osc[cn_di_osc5].I;		//use 'real' for both i/q
								osc_sumi0 += (ampl10 + ampl_mod10r) * st_di_osc[cn_di_osc5].Q;		//use 'real' for both i/q
								}
							break;
							
							case 2:																//lwrSB
								{
								float modul_depth = 1.0f;
								float ampl_mod10r = modul_depth * modulation10r;
								float ampl_mod10i = modul_depth * modulation10i;
								float r0 = ampl_mod10r * 0.5f;									//keep demod spectral height at same level by having modul level
								float i0 = ampl_mod10i * 0.5f;
								osc_sumr0 += r0 * st_di_osc[cn_di_osc5].I - i0 * st_di_osc[cn_di_osc5].Q;
								osc_sumi0 += r0 * st_di_osc[cn_di_osc5].Q + i0 * st_di_osc[cn_di_osc5].I;
								}
							break;

							case 3:																//uprSB
								{
								float modul_depth = 1.0f;
								float ampl_mod10r = modul_depth * modulation10r;
								float ampl_mod10i = modul_depth * modulation10i;
								float r0 = ampl_mod10r * 0.5f;									//keep demod spectral height at same level by having modul level
								float i0 = ampl_mod10i * 0.5f;
								osc_sumr0 += r0 * st_di_osc[cn_di_osc5].I + i0 * st_di_osc[cn_di_osc5].Q;
								osc_sumi0 += r0 * st_di_osc[cn_di_osc5].Q - i0 * st_di_osc[cn_di_osc5].I;
								}
							break;

							case 4:																//FM
								{
								float modul_depth = 1.0f;
								float ampl_mod10r = modul_depth * modulation10r;
//								float ampl_mod10i = modul_depth * modulation10i;

								float modul = ampl_mod10r;

								int ifm = 1;
								st_fm[ifm].phase += st_fm[ifm].wc + st_fm[ifm].kf * modul;		//calc phase update
								
								float s, c;
								sincosf(st_fm[ifm].phase, &s, &c);

								osc_sumr0 += c;
								osc_sumi0 += s;
								
								if(st_fm[ifm].phase >  pi) st_fm[ifm].phase -= twopi;
								if(st_fm[ifm].phase < -pi) st_fm[ifm].phase += twopi;
								

							/*

								int ifm = 0;
									
								// ----- 1. recursive carrier oscillator step -----
								float I0 = st_di_osc[cn_di_osc5].I;
								float Q0 = st_di_osc[cn_di_osc5].Q;

								float I1 = I0 * st_fm[ifm].stepI - Q0 * st_fm[ifm].stepQ;
								float Q1 = I0 * st_fm[ifm].stepQ + Q0 * st_fm[ifm].stepI;

								// ----- 2. FM vector rotation (2nd-order) -----
								float m = ampl_mod10r;           // MPX composite scalar (stereo + pilot + RDS)
								float dphi = fm_deviation_gain * m;
								float r = 1.0f - 0.5f*dphi*dphi;

								st_fm[ifm].oscI = r*I1 - dphi*Q1;
								st_fm[ifm].oscQ = r*Q1 + dphi*I1;

								// ----- 3. phase-step estimate (AFC detector) -----
								float dtheta_est = st_fm[ifm].oscI * st_fm[ifm].prevQ  -  st_fm[ifm].oscQ * st_fm[ifm].prevI;

								// ----- 4. slow AFC average -----
								st_fm[ifm].freq_err_avg += st_fm[ifm].beta * (dtheta_est - st_fm[ifm].freq_err_avg);

								// ----- 5. trim carrier step toward zero-bias (AFC) -----
								float d = -st_fm[ifm].trim_gain * st_fm[ifm].freq_err_avg;
								float rc = 1.0f - 0.5f*d*d;

								float sI = st_fm[ifm].stepI;
								float sQ = st_fm[ifm].stepQ;

								st_fm[ifm].stepI = rc*sI - d*sQ;
								st_fm[ifm].stepQ = rc*sQ + d*sI;

								// ----- 6. gently slew toward nominal carrier step -----
								st_fm[ifm].stepI += st_fm[ifm].slew_alpha * (st_fm[ifm].stepI_ref - st_fm[ifm].stepI);
								st_fm[ifm].stepQ += st_fm[ifm].slew_alpha * (st_fm[ifm].stepQ_ref - st_fm[ifm].stepQ);

								// ----- 7. normalize carrier step vector -----
								float mag = sqrtf(st_fm[ifm].stepI * st_fm[ifm].stepI + st_fm[ifm].stepQ * st_fm[ifm].stepQ + 1e-20f);
								st_fm[ifm].stepI /= mag;
								st_fm[ifm].stepQ /= mag;

								// ----- 8. save previous oscillator state -----
								st_fm[ifm].prevI = st_fm[ifm].oscI;
								st_fm[ifm].prevQ = st_fm[ifm].oscQ;

								// ----- 9. output FM I/Q -----
								osc_r10 = st_fm[ifm].oscI;
								osc_i10 = st_fm[ifm].oscQ;
	*/
								}

							break;
							}





						switch( synth_carr_lsb_usb_fm_2 )
							{
							case 0:																//AM
	//							osc_sumr0 += 0;													//build am carriers
	//							osc_sumr0 += 0;
							break;

							case 1:																//AM
								{
								float ampl20 = 1.0f;											//default carrier ampl							
								float modul_depth = 1.0f;

								float ampl_mod20r = modul_depth * modulation20r;				//use real component only
	//							float ampl_mod00i = modul_depth * modulation00i;
								osc_sumr0 += (ampl20 + ampl_mod20r) * st_di_osc[cn_di_osc7].I;		//use 'real' for both i/q
								osc_sumi0 += (ampl20 + ampl_mod20r) * st_di_osc[cn_di_osc7].Q;		//use 'real' for both i/q
								}
							break;
							
							case 2:																//lwrSB
								{
								float modul_depth = 1.0f;
								float ampl_mod20r = modul_depth * modulation20r;
								float ampl_mod20i = modul_depth * modulation20i;
								float r0 = ampl_mod20r * 0.5f;									//keep demod spectral height at same level by having modul level
								float i0 = ampl_mod20i * 0.5f;
								osc_sumr0 += r0 * st_di_osc[cn_di_osc7].I - i0 * st_di_osc[cn_di_osc7].Q;
								osc_sumi0 += r0 * st_di_osc[cn_di_osc7].Q + i0 * st_di_osc[cn_di_osc7].I;
								}
							break;

							case 3:																//uprSB
								{
								float modul_depth = 1.0f;
								float ampl_mod20r = modul_depth * modulation20r;
								float ampl_mod20i = modul_depth * modulation20i;
								float r0 = ampl_mod20r * 0.5f;									//keep demod spectral height at same level by having modul level
								float i0 = ampl_mod20i * 0.5f;
								osc_sumr0 += r0 * st_di_osc[cn_di_osc7].I + i0 * st_di_osc[cn_di_osc7].Q;
								osc_sumi0 += r0 * st_di_osc[cn_di_osc7].Q - i0 * st_di_osc[cn_di_osc7].I;
								}
							break;

							case 4:																//FM
								{
								float modul_depth = 1.0f;
								float ampl_mod20r = modul_depth * modulation20r;
								float ampl_mod20i = modul_depth * modulation20i;

								float modul = ampl_mod20r;

								int ifm = 2;
								st_fm[ifm].phase += st_fm[ifm].wc + st_fm[ifm].kf * modul;		//calc phase update
								
								float s, c;
								sincosf(st_fm[ifm].phase, &s, &c);

								osc_sumr0 += c;
								osc_sumi0 += s;
								
								if(st_fm[ifm].phase >  pi) st_fm[ifm].phase -= twopi;
								if(st_fm[ifm].phase < -pi) st_fm[ifm].phase += twopi;
								}

							break;
							}






						switch( synth_carr_lsb_usb_fm_3 )
							{
							case 0:																//AM
	//							osc_sumr0 += 0;													//build am carriers
	//							osc_sumr0 += 0;
							break;

							case 1:																//AM
								{
								float ampl30 = 1.0f;											//default carrier ampl							
								float modul_depth = 1.0f;

								float ampl_mod30r = modul_depth * modulation30r;				//use real component only
	//							float ampl_mod00i = modul_depth * modulation00i;
								osc_sumr0 += (ampl30 + ampl_mod30r) * st_di_osc[cn_di_osc9].I;		//use 'real' for both i/q
								osc_sumi0 += (ampl30 + ampl_mod30r) * st_di_osc[cn_di_osc9].Q;		//use 'real' for both i/q
								}
							break;
							
							case 2:																//lwrSB
								{
								float modul_depth = 1.0f;
								float ampl_mod30r = modul_depth * modulation30r;
								float ampl_mod30i = modul_depth * modulation30i;
								float r0 = ampl_mod30r * 0.5f;									//keep demod spectral height at same level by having modul level
								float i0 = ampl_mod30i * 0.5f;
								osc_sumr0 += r0 * st_di_osc[cn_di_osc9].I - i0 * st_di_osc[cn_di_osc9].Q;
								osc_sumi0 += r0 * st_di_osc[cn_di_osc9].Q + i0 * st_di_osc[cn_di_osc9].I;
								}
							break;

							case 3:																//uprSB
								{
								float modul_depth = 1.0f;
								float ampl_mod30r = modul_depth * modulation30r;
								float ampl_mod30i = modul_depth * modulation30i;
								float r0 = ampl_mod30r * 0.5f;									//keep demod spectral height at same level by having modul level
								float i0 = ampl_mod30i * 0.5f;
								osc_sumr0 += r0 * st_di_osc[cn_di_osc9].I + i0 * st_di_osc[cn_di_osc9].Q;
								osc_sumi0 += r0 * st_di_osc[cn_di_osc9].Q - i0 * st_di_osc[cn_di_osc9].I;
								}
							break;

							case 4:																//FM
								{
								float modul_depth = 1.0f;
								float ampl_mod30r = modul_depth * modulation30r;
								float ampl_mod30i = modul_depth * modulation30i;

								float modul = ampl_mod30r;

								int ifm = 3;
								st_fm[ifm].phase += st_fm[ifm].wc + st_fm[ifm].kf * modul;		//calc phase update
								
								float s, c;
								sincosf(st_fm[ifm].phase, &s, &c);

								osc_sumr0 += c;
								osc_sumi0 += s;
								
								if(st_fm[ifm].phase >  pi) st_fm[ifm].phase -= twopi;
								if(st_fm[ifm].phase < -pi) st_fm[ifm].phase += twopi;
								}

							break;
							}










	//					osc_r10 = (ampl10 + ampl_mod10r) * st_di_osc[cn_di_osc5].I;
	//					osc_i10 = (ampl10 + ampl_mod10i) * st_di_osc[cn_di_osc5].Q;

	//					osc_r20 = (ampl10 + ampl_mod20) * st_di_osc[cn_di_osc7].I;
	//					osc_i20 = (ampl10 + ampl_mod20) * st_di_osc[cn_di_osc7].Q;

	//					osc_r30 = (ampl10 + ampl_mod30) * st_di_osc[cn_di_osc9].I;
	//					osc_i30 = (ampl10 + ampl_mod30) * st_di_osc[cn_di_osc9].Q;

	//					osc_r40 = (ampl10 + ampl_mod40) * st_di_osc[cn_di_osc11].I;
	//					osc_i40 = (ampl10 + ampl_mod40) * st_di_osc[cn_di_osc11].Q;

	//					osc_r50 = (ampl10 + ampl_mod50) * st_di_osc[cn_di_osc13].I;
	//					osc_i50 = (ampl10 + ampl_mod50) * st_di_osc[cn_di_osc13].Q;


		//				osc_r50 = (ampl50 + ampl_mod50) * cosf( synth_theta50 );	//single sideband carrier, when used for Weaver's method of generating single sideband modulation, this is set to desired 'carrier freq' + '0.5*audio baseband bandwidth'
		//				osc_i50 = (ampl50 + ampl_mod50) * sinf( synth_theta50 );
						
	//					}

		/*
					if( wnd_rtl_graph->synth_mod_type == en_mdt_lsb )
						{
						osc_r0 *= 0.3f * cosf( synth_theta0 );					//build lower sideband carriers
						osc_i0 *= 0.3f*(modulation00);

						osc_r10 *= 0.3f*(modulation20);						//apply am modulation using dsbsc
						osc_i10 *= 0.3f*(modulation20);
						}
		*/

	//				}
		 


	//			if( (wnd_rtl_graph->b_synth_voice_on_iq0) )						//combine carriers as req.
	//				{
	//				osc_sumr0 += osc_r00;
	//				osc_sumi0 += osc_i00;
	//				}


	//			if( (wnd_rtl_graph->b_synth_voice_on_iq1) )
	//				{
	//				osc_sumr0 += osc_r10;
	//				osc_sumi0 += osc_i10;
	//				}

	//			if( (wnd_rtl_graph->b_synth_voice_on_iq2) )
	//				{
	//				osc_sumr0 += osc_r20;
	//				osc_sumi0 += osc_i20;
	//				}

	//			if( (wnd_rtl_graph->b_synth_voice_on_iq3) )
	//				{
	//				osc_sumr0 += osc_r30;
	//				osc_sumi0 += osc_i30;
	//				}

	//			if( (wnd_rtl_graph->b_synth_voice_on_iq4) )
	//				{
	//				osc_sumr0 += osc_r40;
	//				osc_sumi0 += osc_i40;
	//				}

	//			if( (wnd_rtl_graph->b_synth_voice_on_iq5) )
	//				{
	//				osc_sumr0 += osc_r50;
	//				osc_sumi0 += osc_i50;
	//				}

				if( wnd_rtl_graph->b_synth_noise_on_iq ) 
					{
					osc_sumr0 += synth_noise_gain * rnd();
					osc_sumi0 += synth_noise_gain * rnd();
					}

				osc_sumr0 += synth_tone_dcoffs0;
				osc_sumi0 += synth_tone_dcoffs0;



				if( b_synth_build_recording )								//store synth signal
					{
					float rf_clip = 1.0f;
					
					if( osc_sumr0 > rf_clip ) osc_sumr0 = rf_clip;
					if( osc_sumr0 < -rf_clip ) osc_sumr0 = -rf_clip;

					if( osc_sumi0 > rf_clip ) osc_sumi0 = rf_clip;
					if( osc_sumi0 < -rf_clip ) osc_sumi0 = -rf_clip;
					
					bf_synth[ synth_ptr_wr ] = osc_sumr0 * cn_synth_IQ_divisor;	//16 bit formatter
					bf_synth[ synth_ptr_wr + 1 ] = osc_sumi0 * cn_synth_IQ_divisor;
					synth_ptr_wr += 2;
					
					if( !(synth_ptr_wr % 10000000) ) printf("demod_iso() - &&&&&&&&&&&& building synth rf, synth_ptr_wr %d  synth_sample_cnt %d &&&&&&&&&&&&\n", synth_ptr_wr, synth_sample_cnt );

					if( synth_ptr_wr >= synth_sample_cnt  ) 
						{
						printf("demod_iso() - &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n" );
						printf("demod_iso() - &&&&&&&&&&&&&&&&&& completed synth rf, synth_ptr_wr %d &&&&&&&&&&&&&&&&\n", synth_ptr_wr );
						printf("demod_iso() - &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n" );
						
						b_synth_build_recording = 0;
						b_synth_build_playing = 1;
						}
					}
		
				o.real = osc_sumr0 * g_gain_iq;
				o.imag = osc_sumi0 * g_gain_iq;
		




				}  //refer: if( !b_synth_build )	//zero this to turn off modulators, allows checking cpu usage
			else{
				//---if here playing synth---
	//			if( b_synth_build == 0 )
					{
					o.real = bf_synth[ synth_ptr_rd ] / cn_synth_IQ_divisor;	//use stored synthesis samples
					o.imag = bf_synth[ synth_ptr_rd + 1 ] / cn_synth_IQ_divisor;
					
					synth_ptr_rd += 2;
					if( synth_ptr_rd >= synth_sample_cnt ) synth_ptr_rd = 0;
					
					
					
					//tuner mixer, complex multiply: x * conj(osc) down-conversion
		
					float freal = o.real * st_di_osc[cn_di_osc15].I   +   o.imag * st_di_osc[cn_di_osc15].Q;
					float fimag = o.imag * st_di_osc[cn_di_osc15].I   -   o.real * st_di_osc[cn_di_osc15].Q;

					o.real = freal * g_gain_iq;
					o.imag = fimag * g_gain_iq;
					}
				}

			}		//if(1)															//zero this to turn off modulators, allows checking cpu usage
			
		} //refer: if( wnd_rtl_graph->b_synth_iq )
//-----------------------------



 

//bf1_iq[0][..]
 
//bf1_ready_block_num;

//---------- thread safe -----------
//        size_t wr = bf1_wr.load(std::memory_order_relaxed);
 //       size_t rd = bf1_rd.load(std::memory_order_acquire);

//		bf1_iq[wr] = 0;

//int wr_next = 0;

//		bf1_wr.store(wr_next, std::memory_order_release);
//-----------------------------------




	viq_local.push_back( o );
	
	f_rd += inc_rate + inc_rate2;
	f_rd2 += inc_rate + inc_rate2;

//	if( f_rd >= cn_rtl_buf_size ) f_rd = 0.0f;



	if( f_rd >= cn_rtl_buf_size ) 
		{
		f_rd -= cn_rtl_buf_size;

		if( f_rd >= cn_rtl_buf_size ) 
			{
			f_rd = 0.0f;
			}
		}

	if( f_rd < 0 ) 
		{
		f_rd += cn_rtl_buf_size;

		if( f_rd < 0 ) 
			{
			f_rd = 0.0f;
			}
		}







	if( ( wnd_rtl_graph->b_synth_iq ) && ( !b_synth_build_playing ) )
		{
		//inc dual integrator sine oscillators
		for( int i = 0; i < vosc_active_idx.size(); i++ )
			{
	//		printf("vosc_active_idx[i] %d\n", vosc_active_idx[i] );
			st_dual_integrator_osc_tag *osc = st_di_osc + vosc_active_idx[i];
			
			float ii = osc->cos * osc->I - osc->sin * osc->Q;
			float qq = osc->sin * osc->I + osc->cos * osc->Q;
			osc->I = ii;
			osc->Q = qq;
			}
		}









	//calc a finer slewing adjusment
	if( b_slew_adj_fine )
		{
		uiwnd_low2 = (idbg_wr - cn_rtl_buf_size/2) - cn_rtl_buf_size/8;	//these window values will be above of 'cn_rtl_buf_size-1' very quickly as 'idbg_wr' and 'idbg_rd' are allowed to increase indefinitly, so they will pass 'uint64_t' largest poss number
		uiwnd_high2 = (idbg_wr - cn_rtl_buf_size/2) + cn_rtl_buf_size/8;


		if( f_rd2 < uiwnd_low2 ) 
			{
			inc_rate2 = 0.009;
			}

		if( f_rd2 > uiwnd_high2 ) 
			{
			inc_rate2 = -0.009;
			}
		}


//	dbg1-= 1;		//i+q count down

	}	//for loop end ----> for( int i = 0; i < iq_count; i++ )						//rtl adc srate, e.g adc rate of 2496000 samples/sec * frame time = 106496 samples per frame

//======================================================================


//printf( "demod_iso() - b_synth_build_start %d  b_synth_build_recording %d  b_synth_build_playing %d\n", b_synth_build_start, b_synth_build_recording, b_synth_build_playing );



//tweek each dual integrator sine oscillator's combined I/Q magnitude, to keep it close to 1.0, otherwise it will decay due to float precision errors
//ONLY need to do this infrequently, such as at frame rate
for( int i = 0; i < cn_dual_integrator_osc_cnt; i++ )
	{
	st_dual_integrator_osc_tag *osc = st_di_osc + i;

    float mag = osc->I * osc->I + osc->Q * osc->Q;
    float corr = 1.5f - 0.5f * mag;  	//1st order Newton/Raphson to calc: '1/sqrt(mag)'
										//e.g: I_tweek = I/sqrt( (I*I) + (Q*Q) ), use newton/raph to numerically simplify calc '1/sqrt()'  this avoids slower call to 'std::sqrt()' and the divide 
	osc->I *= corr;	
    osc->Q *= corr;
	}






	rtl_bf0_rd = f_rd;
	idbg_rd = f_rd2;
	
if(vb)printf( "demod_iso() - end   f_rd %f idbg_rd %" PRIi64 " delta_wr_rd %" PRIi64 "\n", f_rd, idbg_rd, delta_wr_rd );


int test_rec_iq_val = 0;


if( rec_play_iq_state != 0 )
	{
//------------- file recorder ---------------
	if( rec_play_iq_state == 2 )
		{
		rec_play_iq_state = 3;
		test_rec_iq_val = 0;
		}

	if( rec_play_iq_state == 3 )
		{
		int siz = viq_local.size();

		int i2 = 0;
		int i3 = 0;
		for( int i = 0; i < (siz*2); i++ )
			{
			float ff;
			if( i & 0x01 ) 
				{
				ff = viq_local[i2].imag;									//'Q'
				i2++;
				}
			else{
				ff = viq_local[i2].real;									//'I'
				}


	//		ff = -1.0;
	//		ff *= 180.0f;



	// !!!!
	//ff = test_rec_iq_val;
	//test_rec_iq_val++;
	//if( test_rec_iq_val > 255 ) test_rec_iq_val = 0;
	// !!!!


	//		int iv = ff;
			
			recply_bf[rec_wr++] = ff;

			rec_wr_cnt++;

			if( rec_wr >= rec_bf_sz )
				{
	if(vb)printf( "demod_iso() - rec_wr %d, looping back to zero\n", rec_wr );
				rec_wr = 0;
				}

	// !!!!
	//	int iptr = rec_wr - 1;
	//	if(iptr < 0 ) iptr = rec_bf_sz - 1;
	//float test_val = rec_bf[iptr] / 180.0f;

	//if( i & 0x01 ) 
	//	{
		
	//	viq_local[i3].imag = test_val;									//'Q'
	//	i3++;
	//	}
	//else{
	//	viq_local[i3].real = test_val;									//'I'
	//	}
	// !!!!


			}

		rec_play_time += frame_time;
		if( rec_play_time > rec_mode_end_secs )
			{
			rec_play_iq_state = 4;
			}
		}

	if( rec_play_iq_state == 4 )
		{
		rec_play_iq_state = 5;
		}
	//-----------------------------------






	//----------- file play -------------
	if( rec_play_iq_state == 12 )
		{
		if(vb0)printf( "demod_iso() - rec_play_iq_state %d, going to state 13\n", rec_play_iq_state );
		rec_play_iq_state = 13;
		}

	if( rec_play_iq_state == 13 )
		{
		if(0)
			{
			int siz = viq_local.size();
			
			int i2 = 0;
			for( int i = 0; i < (siz*2); i++ )
				{
				float ff;
				if( i & 0x01 ) 
					{
					ff = viq_local[i2].imag;									//'Q'
					i2++;
					}
				else{
					ff = viq_local[i2].real;									//'I'
					}
				
		//		ff = -1.0;
		//		ff *= 180.0f;
				
		//		int iv = ff;
				
				recply_bf[rec_wr++] = ff;

				rec_wr_cnt++;

				if( rec_wr >= rec_bf_sz )
					{
					rec_wr = 0;
					}
				}
			}







		int siz = viq_local.size();
		
		int i2 = 0;
		for( int i = 0; i < (siz*2); i++ )
			{
	//		int iv = recply_bf[rec_rd++];
			float ff = recply_bf[rec_rd++];
			
	//		if( iv > 0x7f ) iv = - (256 - iv);								//neg val ?
			
	//		float ff = iv / 180.0f;
	//if(i == 0) {printf("rec_rd %d\n", rec_rd );  printf("ff %f\n", ff );}
			if( i & 0x01 ) 
				{
				viq_local[i2].imag = ff;									//'Q'
				i2++;
				}
			else{
				ff = viq_local[i2].real = ff;								//'I'
				}


			rec_rd_cnt++;
//			if( b_rec_play_flush ) 
//				{
//				rec_rd_cnt = rec_bf_sz;
//				b_rec_play_flush = 0;
//				}
			
			if( rec_rd >= rec_bf_sz )
				{
	if(vb)printf( "demod_iso() - rec_rd %d, looping back to zero\n", rec_rd );
				rec_rd = 0;
				}
				
			
//			if( b_rec_change_play_pos )									//does user want to change play pos ?
//				{
//				b_rec_change_play_pos = 0;
//				rec_rd = rec_change_play_pos_to;
//				}

			}

//		rec_play_time += frame_time;
//		if( rec_play_time >= play_mode_end_secs ) rec_play_time = 0;
		}


	if( rec_play_iq_state == 14 )
		{
		if(vb0)printf( "demod_iso() - rec_play_iq_state %d, going to state 15\n", rec_play_iq_state );
		rec_play_iq_state = 15;
		}
	//-----------------------------------

	}








//move iq data to gui thread for fft and spectral plotting on inset graticule graph
if( bf1_prep_idx_atm.load(memory_order_acquire) == -1 )			//is gui thread done processing a block and in need of more data?  (also ensures thread safe access)
	{
	int cnt = viq_local.size();
	
//printf( "demod_iso() - loading data for thread\n" );
	for( int i = 0; i < viq_local.size(); i++ )
		{
		bf1_iq[bf1_prep_idx][i] = viq_local[i];
		}
	
	bf1_cnt_atm.store( cnt, memory_order_relaxed );
	bf1_prep_idx_atm.store( bf1_prep_idx, memory_order_release );

	
	bf1_prep_idx ^= 1;
	}







vector<st_spect_tag> vsp;



st_plot.which = en_gls_IQ_timedomain;

st_plot.vcpx0 = &viq_local;
plot_load_struct_audio_thrd( st_plot, (void*)&viq_local, (void*)0 );								//plot



st_plot.which = en_gls_IQ_spect;
plot_load_struct_audio_thrd( st_plot, (void*)&viq_local, (void*)0 );								//plot








//---------- sub tuning, tuning within spectrum provided by rtl ----------
//if( (int)g_freq_tune_cur != 0 )
if( 1 )
	{
	float freq_shift = g_freq_sub_tune;
//	float freq_shift = g_freq_tune_cur;//g_device_bw / 1e6f;
//	printf("demod_iso() - gfreq_sub_tune %d\n", (int)g_freq_sub_tune );
	
	float theta_mux_inc = freq_shift * twopi/g_dev_bw;
	
//	printf("demod_iso() - g_dev_bw %f   g_freq_tune_cur %d  theta_mux_inc %f\n", g_dev_bw, (int)g_freq_tune_cur, theta_mux_inc );
	
	for( int i = 0; i < viq_local.size(); i++ )
		{
		float famp = 2.0f;
		float fr = viq_local[i].real;// * famp * cosf( theta_mux );

		float fi = viq_local[i].imag;// * famp * sinf( theta_mux );

		float I = cosf( theta_mux );
		float Q = -sinf( theta_mux );
		
		float fr2 = fr * I - fi * Q;									//tuner mixdown
		float fi2 = fi * I + fr * Q;


//		if( b_downsampler_aa )
//			{
//			fr2 = elliplp_cr.process(fr2);
//			fi2 = elliplp_ci.process(fi2);
//			}

		
		viq_local[i].real = fr2;
		viq_local[i].imag = fi2;
		
		theta_mux += theta_mux_inc;
		if( theta_mux >= twopi ) theta_mux -= twopi;
		if( theta_mux <= -twopi ) theta_mux += twopi;
		if( theta_mux >= 2*twopi ) theta_mux = 0;
		if( theta_mux <= -2*twopi ) theta_mux = 0;				//handle a large theta, possibly due to high freq usage
		}
	}
//----------------------------------------------------------------------





st_plot.which = en_gls_sub_tuner_spect;

st_plot.vcpx0 = &viq_local;
plot_load_struct_audio_thrd( st_plot, (void*)&viq_local, (void*)0 );								//plot











/*

	if( (!b_dbg_no_graph_update ) )
//	if( (!b_dbg_no_graph_update ) && ( !(demod_cnt%3 ) ) )
//	if( 1 )
		{
		if( i_fftw_trig_plan_create_state == 2 )						//plans created?
			{
	//		else{
				bool b_plan_exists = complex_fwd_fft_multi( viq_local, vcplex_sp, "0: for display", en_ftid_audio_proc );			//go to freq domain using an fft
	//			}
			}




//		down_sample_factor_for_graph = vcplex_sp.size() / 4096;

		down_sample_factor_for_graph = 1;

	//	down_sample_factor_for_graph = 30;
		low_pass_srconv( vcplex_sp,  vcplex_sp_downsample, down_sample_factor_for_graph, filt_prev_idx2, now_r2, now_j2 );			//complex decimate





		if(vb0)printf("demod_iso() - down_sample_factor_for_graph %d, vcplex_sp.size() %d vcplex_sp_downsample.size() %d\n", down_sample_factor_for_graph, vcplex_sp.size(), vcplex_sp_downsample.size() );


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
				
			vspect_displayble = vspect_disp_actual;
			}
		//	complex_rev_fft_multi( vcplex_sp, vtdm );									//go to time domain using a rev fft

		}

*/


























//antialias filter before downsampler
if( g_b_dwn_aa )
	{
	for( int i = 0; i < viq_local.size(); i++ )
		{
		float fr = viq_local[i].real;
		float fi = viq_local[i].imag;

		if(0)
			{
//			fr = elliplp_cr.process(fr);
//			fi = elliplp_ci.process(fi);
			}
		else{
			fr = elliplpf_cr.sos_cascade_process_float( fr );
			fi = elliplpf_ci.sos_cascade_process_float( fi );
			}
		
		viq_local[i].real = fr;
		viq_local[i].imag = fi;
		}
	}






st_plot.which = en_gls_dwncnv_aa_spect;

st_plot.vcpx0 = &viq_local;
plot_load_struct_audio_thrd( st_plot, (void*)&viq_local, (void*)0 );											//plot





//if(1) printf( "demod_iso() ->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>  viq_local.size() %d\n", viq_local.size() );





//if( iq_count == 0 )
//	{
//	for( int i = 0; i < 1000; i++ )						//rtl adc srate 3200000
//		{
//		if(1)printf( "demod_iso() - iq_count is zero %d\n", 0 );
		
//		}
//	}



//if( vgph1_x.size() > 150 )
//	{
//	vgph1_x.erase( vgph1_x.begin() + 0, vgph1_x.begin() + 75 );	
//	vgph1_y0.erase( vgph1_y0.begin() + 0, vgph1_y0.begin() + 75 );	
//	vgph1_y1.erase( vgph1_y1.begin() + 0, vgph1_y1.begin() + 75 );	
//	}
//vgph1_x.push_back( demod_cnt );
//vgph1_y0.push_back( delta_wr_rd );
//vgph1_y1.push_back( delta_wr_rd );

//if(1) printf( "demod() ->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>  dbg1 %d viq.size() %d\n", dbg1, viq.size() );
//pthread_mutex_unlock( &mutex2 );					//mutex as viq is being modified

//---------------------------------







/*
if( 0 ) 	//for cpu usage tests, create silent audio
	{


	for( int i = 0; i < framecnt; i++ )
		{
		
		vaudio.push_back( 00 );												//load vector with audio samples for audiocard o/p
		}

	if( vaudio.size() > 16384 )
		{
		printf( "demod_iso() - vaudio.size() %d is too big, clearing it\n", (int)vaudio.size() );
		vaudio.clear();
		}

	demod_cnt++;
	return;
	}
*/



/*
if( 0 )
	{
	for( int i = 0; i < viq_local.size(); i++ )
		{
		float fr = viq_local[i].real;// * famp * cosf( theta_mux );

		float fi = viq_local[i].imag;// * famp * sinf( theta_mux );

		fr = elliplp_cr.process(fr);
		fi = elliplp_ci.process(fi);
		
		viq_local[i].real = fr;
		viq_local[i].imag = fi;
		
		}

	}
*/



/*
//	if( !b_dbg_no_graph_update )
	if( 0 )
		{
		if( i_fftw_trig_plan_create_state == 2 )							//plans created?
			{
	//		else{
				bool b_plan_exists = complex_fwd_fft_multi( viq_local, vcplex_sp, "0: for display", en_ftid_audio_proc );			//go to freq domain using an fft
	//			}
			}




//		down_sample_factor_for_graph = vcplex_sp.size() / 4096;

		down_sample_factor_for_graph = 1;

	//	down_sample_factor_for_graph = 30;
		low_pass_srconv( vcplex_sp,  vcplex_sp_downsample, down_sample_factor_for_graph, filt_prev_idx2, now_r2, now_j2 );			//complex decimate





		if(vb0)printf("demod_iso() - down_sample_factor_for_graph %d, vcplex_sp.size() %d vcplex_sp_downsample.size() %d\n", down_sample_factor_for_graph, vcplex_sp.size(), vcplex_sp_downsample.size() );


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
				
			vspect_displayble = vspect_disp_actual;
			}
		//	complex_rev_fft_multi( vcplex_sp, vtdm );									//go to time domain using a rev fft


		}
*/


/*

//----- mutex ----
	if( mutex3.try_lock() )												//see 'update_prep_gph0_cnt()' for other lock
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
//				vspect_gph = vspect_displayble;
				}
			}
		mutex3.unlock();
		}

*/



//printf("demod_iso() - dnwsmpl0 %d\n", dnwsmpl0 );



//int pre_samples_sec0 = g_dev_bw;

low_pass_srconv( viq_local,  vdownsamp, downsample_factor, filt_prev_idx0, now_r0, now_j0 );			//complex decimate (WFM uses a lower 'downsample_factor')


//printf("demod_iso() - viq_local %d,  vdownsamp %d  down_sample_factor %d\n", viq_local.size(), vdownsamp.size(), downsample_factor );

/*
//------- CIC decimator function ------------
if( demod_cnt ==  0)
	{
	st_cic_state.idx = 0;

    // 3-stage integrators
    st_cic_state.i1r = 0;
    st_cic_state.i1i = 0;
    st_cic_state.i2r = 0; st_cic_state.i2i = 0;
    st_cic_state.i3r = 0; st_cic_state.i3i = 0;

    // 3-stage comb delay registers
    st_cic_state.c1r = 0; st_cic_state.c1i = 0;
    st_cic_state.c2r = 0; st_cic_state.c2i = 0;
    st_cic_state.c3r = 0; st_cic_state.c3i = 0;
	}






int downsample0 = down_sample_factor;

    vdownsamp.clear();

for (size_t i = 0; i < viq_local.size(); i++)
    {
	// -------- Integrators (run at Fs_in) --------
	st_cic_state.i1r += viq_local[i].real;
	st_cic_state.i1i += viq_local[i].imag;

	st_cic_state.i2r += st_cic_state.i1r;
	st_cic_state.i2i += st_cic_state.i1i;

	st_cic_state.i3r += st_cic_state.i2r;
	st_cic_state.i3i += st_cic_state.i2i;

	st_cic_state.idx++;
	if (st_cic_state.idx < downsample0) continue;

	st_cic_state.idx = 0;

	// -------- Combs (run at Fs_out) --------
	float y1r = st_cic_state.i3r - st_cic_state.c1r;
	float y1i = st_cic_state.i3i - st_cic_state.c1i;
	st_cic_state.c1r = st_cic_state.i3r;
	st_cic_state.c1i = st_cic_state.i3i;

	float y2r = y1r - st_cic_state.c2r;
	float y2i = y1i - st_cic_state.c2i;
	st_cic_state.c2r = y1r;
	st_cic_state.c2i = y1i;

	float y3r = y2r - st_cic_state.c3r;
	float y3i = y2i - st_cic_state.c3i;
	st_cic_state.c3r = y2r;
	st_cic_state.c3i = y2i;

	// -------- Gain normalization --------
	// CIC gain = (R^N)
	float scale = 1.0f / (downsample0 * downsample0 * downsample0);

	filter_code::st_cplex_tag o;
	o.real = y3r * scale;
	o.imag = y3i * scale;

	vdownsamp.push_back(o);
    }

if(1) printf( "demod_iso() - viq_local.size() %d  vdownsamp.size()  %d\n", viq_local.size(), vdownsamp.size() );

//------------------------------

*/




st_plot.which = en_gls_downcnv_timedomain;

st_plot.vcpx0 = &vdownsamp;
plot_load_struct_audio_thrd( st_plot, (void*)&vdownsamp, (void*)0 );											//plot

st_plot.which = en_gls_downcnv_spect;
plot_load_struct_audio_thrd( st_plot, (void*)&vdownsamp, (void*)0 );											//plot











if( g_b_user_iir_hpf0 | g_b_user_iir_lpf0 | g_b_user_iir_lpf1 | g_b_user_iir_lpf2 )				//process user selected iir filters
	{
	for( int i = 0; i < vdownsamp.size(); i++ )	
		{
		float f0 = vdownsamp[i].real;
		float f1 = vdownsamp[i].imag;

	//---
		if( g_b_user_iir_hpf0 )
			{
			f0 = iir_process( usr_hpf_iir0_I0, f0 );
			}
			
		if( g_b_user_iir_lpf0 )
			{
			f0 = iir_process( usr_lpf_iir0_I0, f0 );
			}


		if( g_b_user_iir_lpf1 )
			{
			f0 = iir_process( usr_lpf_iir1_I0, f0 );
			}

		if( g_b_user_iir_lpf2 )
			{
			f0 = iir_process( usr_lpf_iir2_I0, f0 );
			}

		vdownsamp[i].real = f0;
	//---


	//---
		if( g_b_user_iir_hpf0 )
			{
			f1 = iir_process( usr_hpf_iir0_Q0, f1 );
			}
			
		if( g_b_user_iir_lpf0 )
			{
			f1 = iir_process( usr_lpf_iir0_Q0, f1 );
			}

		if( g_b_user_iir_lpf1 )
			{
			f1 = iir_process( usr_lpf_iir1_Q0, f1 );
			}

		if( g_b_user_iir_lpf2 )
			{
			f1 = iir_process( usr_lpf_iir2_Q0, f1 );
			}
	//---

		vdownsamp[i].imag = f1;
		}
	}





//--------------- bpass fir ---------------
if( g_b_bw_bpass ) 
	{	
	for( int i = 0; i < vdownsamp.size(); i++ )				//apply pre demod fir
		{
		float f0 = vdownsamp[i].real;
//usr_demod_bpf_fir_I0.bypass = 1;
		
//		filter_code::fir_in( usr_demod_bpf_fir_I0, f0 );
//		float f1 = filter_code::fir_out( usr_demod_bpf_fir_I0 );

		
		float f1 = filter_code::fir_process( usr_demod_bpf_fir_I0, f0 );

//extern int g_dbg0;
//		if( g_dbg0 ) f1 = 0;

//f1 = f0;
		vdownsamp[i].real = f1;
		
//printf("vcplex_tdm_for_demod.size() %d\n", vcplex_tdm_for_demod.size() );


		f0 = vdownsamp[i].imag;
		
//usr_demod_bpf_fir_Q0.bypass = 1;
//		filter_code::fir_in( usr_demod_bpf_fir_Q0, f0 );
//		f1 = filter_code::fir_out( usr_demod_bpf_fir_Q0 );
		
		f1 = filter_code::fir_process( usr_demod_bpf_fir_Q0, f0 );
//f1 = f0;

//7if( g_dbg0 ) f1 = 0;
		vdownsamp[i].imag = f1;
		}
	}
//----------------------------------------






st_plot.which = en_gls_bandpass_fir_timedomain;

st_plot.vcpx0 = &vdownsamp;
plot_load_struct_audio_thrd( st_plot, (void*)&vdownsamp, (void*)0 );											//plot


st_plot.which = en_gls_bandpass_fir_spect;
plot_load_struct_audio_thrd( st_plot, (void*)&vdownsamp, (void*)0 );											//plot






//--------------------------- demodulation -----------------------------


vcplex_tdm_for_demod = vdownsamp;


if(vb0) printf( "demod_iso() - viq_local.size() %d  vdownsamp.size()  %d   vcplex_tdm_for_demod.size()  %d\n", viq_local.size(), vdownsamp.size(), vcplex_tdm_for_demod.size() );


float unity_gain = 1.0f;





vector <filter_code::st_cplex_float_tag> vcplex_tdm_for_demod_float;
vcplex_tdm_for_demod_float.clear();

for( int i = 0; i < vcplex_tdm_for_demod.size(); i++ )
	{
	filter_code::st_cplex_float_tag oc;
	oc.real = vcplex_tdm_for_demod[i].real;
	oc.imag = vcplex_tdm_for_demod[i].imag;

	vcplex_tdm_for_demod_float.push_back( oc );
	}




bool b_mono_req = 1;

if( demod_type == en_dmt_fm_stereo ) 
	{
	vpcm.clear();
	vector<complex<float>> viq;
	vector <double> vdouble_dummy;

	for( int i = 0; i < vcplex_tdm_for_demod.size(); i++ )				//apply pre demod fir
		{
		complex<float> oc;
		
		oc.real( vcplex_tdm_for_demod[i].real );
		oc.imag( vcplex_tdm_for_demod[i].imag );
		
		viq.push_back( oc );
		
//		vpcm.push_back( 0 );
		
//		en_gls_probe0
		}

	stereo_pll_process( st_fm_decoder, vcplex_tdm_for_demod, vpcm_ch0, vpcm_ch1, 10.0 );
	g_fm_19K_pilot_tone = st_fm_decoder.locked;

	if(vb0) printf( "demod_iso() - st_fm_decoder.locked %d\n", st_fm_decoder.locked );
	
	b_mono_req = 0;
	}
else{
	g_fm_19K_pilot_tone = 0;											//show no pilot lock
	}





if( demod_type == en_dmt_wfm )
	{
	demod_wfm( vcplex_tdm_for_demod_float, vpcm_ch0, unity_gain );		//use demod type that user spec
	}

if( demod_type == en_dmt_fm )
	{
	//demod_am( vcplex_tdm_for_demod, vpcm, unity_gain );
	demod_fm( vcplex_tdm_for_demod_float, vpcm_ch0, unity_gain );
	}

if( demod_type == en_dmt_am )
	{
	demod_am_float( vcplex_tdm_for_demod_float, vpcm_ch0, unity_gain );
	}


if( demod_type == en_dmt_ssb )
	{
	demod_lsb( vcplex_tdm_for_demod_float, vpcm_ch0, unity_gain );
	}

/*
if( demod_type == en_dmt_lsb )
	{
	demod_lsb( vcplex_tdm_for_demod_float, vpcm_ch0, unity_gain );
	}

if( demod_type == en_dmt_usb )
	{
	demod_usb( vcplex_tdm_for_demod_float, vpcm_ch0, unity_gain );
	}
*/

//dsp_utils_code::make_sine( downsample_srate, 300, 0.25, 0, vpcm_ch0.size(), vpcm_ch0 );


//---- audio notcher ------
int ii = en_ftid_iir_notch0_aud;
if( st_filt[ii].flags & en_fflg_on )
	{
	int idx = st_filt[ii].pending_idx.exchange( -1, std::memory_order_acquire );	//thrd safe

	if( idx >= 0 )														//new filter set by gui thrd?
		{
//	printf( "demod_iso() - HHHHHHHHHHHHHHHHHHHHHHHH [audio] switch -> fir[%d]\n", st_filt[ii].active_idx );
		st_filt[ii].active_idx = idx;
		st_filt[ii].iir[idx].dly0 = st_filt[ii].iir[!idx].dly0;			//reduce glitching by copying delay contents over
		st_filt[ii].iir[idx].dly1 = st_filt[ii].iir[!idx].dly1;
		}
	else{
		idx = st_filt[ii].active_idx;									//if no new filter, keep using same filter
		}

//	printf( "demod_iso() - fir[%d]\n", st_filt[ii].active_idx );


	for( int i = 0; i < vpcm_ch0.size(); i++ )
		{
		float f0 = vpcm_ch0[i];
//		f0 = iir_process( usr_notch_iir0, f0 );
		

		
		f0 = iir_process_float( st_filt[ii].iir[idx], f0 );


		vpcm_ch0[i] = f0;
		}
	}


ii = en_ftid_iir_notch1_aud;
if( st_filt[ii].flags & en_fflg_on )
	{
	int idx = st_filt[ii].pending_idx.exchange( -1, std::memory_order_acquire );	//thrd safe

	if( idx >= 0 )														//new filter set by gui thrd?
		{
//	printf( "demod_iso() - HHHHHHHHHHHHHHHHHHHHHHHH [audio] switch -> fir[%d]\n", st_filt[ii].active_idx );
		st_filt[ii].active_idx = idx;
		st_filt[ii].iir[idx].dly0 = st_filt[ii].iir[!idx].dly0;			//reduce glitching by copying delay contents over
		st_filt[ii].iir[idx].dly1 = st_filt[ii].iir[!idx].dly1;
		}
	else{
		idx = st_filt[ii].active_idx;									//if no new filter, keep using same filter
		}

//	printf( "demod_iso() - fir[%d]\n", st_filt[ii].active_idx );


	for( int i = 0; i < vpcm_ch0.size(); i++ )
		{
		float f0 = vpcm_ch0[i];
//		f0 = iir_process( usr_notch_iir0, f0 );
		

		
		f0 = iir_process_float( st_filt[ii].iir[idx], f0 );


		vpcm_ch0[i] = f0;
		}
	}

//------------------------



if( ( b_mono_req ) || ( g_aud_mono ) )
	{
	vpcm_ch1 = vpcm_ch0;
	}


st_plot.which = en_gls_demod_aud_timedomain;

st_plot.vflt0 = &vpcm_ch0;
plot_load_struct_audio_thrd( st_plot, (void*)&vpcm_ch0, (void*)0 );											//plot



vector<float> vpeak;
//vector<float> vpeak_filt;
vector<float> vagc_lev;
//vector<float> vagc_lev1;
vpeak.clear();
//vpeak_filt.clear();
vagc_lev.clear();
//vagc_lev1.clear();

vector<float> vflt_probe;												//for debug plotting

//agc_audio_attack_time = 0.1;
//agc_audio_decay_time = 0.1;

agc_audio_attack_factor = 0.3f / downsample_srate;
//agc_audio_decay_factor = (1e-5 * 24000 ) / (float)downsample_srate;
agc_audio_decay_factor = (2e-5 ) / (downsample_srate / 24000.0f);


//float peak_charge_factor = agc_audio_peak_charge_time / downsample_srate;
//float peak_discharge_factor = agc_audio_peak_discharge_time / downsample_srate;



//printf( "demod_iso() - vpcm.size() %d\n", vpcm.size() );



aud_spect_struct_audio_thrd( vpcm_ch0 );								//for audio notcher


// ---- provide an audio dim feature to slowly bring up audio after major signal disuptions, e.g srate changes or user picking a different demodulator -----
if( aud_dim_time > cn_aud_dim_time_default ) aud_dim_time = cn_aud_dim_time_default;
aud_dim_time -= frame_time;

if( aud_dim_time < 0.0f ) aud_dim_time = 0;
aud_dim_gain = 1.0f - aud_dim_time / cn_aud_dim_time_default;
if( aud_dim_gain > 1.0f ) aud_dim_gain = 1.0f;

//aud_dim_gain *= aud_dim_gain*aud_dim_gain*aud_dim_gain;								//force fade up to be exponential

aud_dim_gain = powf( aud_dim_gain, 4 );								//force fade up to be exponential
//if(vb0) printf( "demod_iso() - aud_dim_time %f   %f\n", aud_dim_time, aud_dim_gain);
//------


float sum2 = 0;


for( int i = 0; i < vpcm_ch0.size(); i++ )
	{
	float aud0 = vpcm_ch0[i];						//grab aud smple incase dc blocker is off
	float aud1 = vpcm_ch1[i];

	
	//--- DC blocking ---
	if( b_dc_block_iq )
		{
		//dc offset calc and its removal
		float sum_dcblk = 0;
		int jj = dcblk_ptr;
		for( int i = 0; i < cn_dcblk_sz; i++ )
			{
			float f0 = dcblk_bf[jj];
			
			sum_dcblk += f0;//*f0;
			
			jj++;
			if( jj >= cn_dcblk_sz ) jj = 0;
			}

		dcblk_bf[ dcblk_ptr ] = aud0;

		dcblk_ptr++;
		if( dcblk_ptr >= cn_dcblk_sz ) dcblk_ptr = 0;
			
		float fdc = sum_dcblk/cn_dcblk_sz;
		aud0 -= fdc;													//remove dc offset
		aud1 -= fdc;
		}
	//-------------------
	
//	vflt_probe.push_back( aud0 );										//for debug plotting

/*
	//--- DC blocking ---
	if( b_dc_block_iq )
		{
			
		
		float alpha = 0.99985f;						//this val is only a guestimate
		
		float sum1 = aud0 - dc_block1_dly0;								//ch0
		sum2 = sum1 + alpha * dc_block1_dly1;

		dc_block1_dly0 = aud0;
		dc_block1_dly1 = sum2;
		aud0 = sum2;




		sum1 = aud1 - dc_block2_dly0;									//ch1
		sum2 = sum1 + alpha * dc_block2_dly1;

		dc_block2_dly0 = aud1;
		dc_block2_dly1 = sum2;
		aud1 = sum2;

//		st_bline_remove.process( aud0, aud1 );
		}

	//-------------------
*/








	if( b_agc )
		{
		float decay_factor = agc_audio_decay_factor;
		if( agc_audio_peak > 1 ) decay_factor = agc_audio_decay_factor * (agc_audio_peak / 0.2f);	//if signal levels are high, increase decay rate
		
		if( fabsf(aud0) > agc_audio_peak ) agc_audio_peak = fabsf( (aud0+aud1)/2 );
		else agc_audio_peak -= decay_factor;

		if( agc_audio_peak < 0.0f ) agc_audio_peak = 0.0f;


		agc_gain_lvl = 0.88f / agc_audio_peak;

		if( agc_gain_lvl > 100.0f ) agc_gain_lvl = 100.0f;
		if( agc_gain_lvl < 0.001f ) agc_gain_lvl = 0.001f;

		vpeak.push_back( agc_audio_peak );								//for plot

//		float mve_avg = moving_average( agc_gain_lvl );

		vagc_lev.push_back( agc_gain_lvl );								//for plot
//		vagc_lev1.push_back( mve_avg );									//for plot

//		float delta = agc_gain_lvl - agc_led_last;
//		if( i == 0 ) agc_led_last = agc_gain_lvl;

//		agc_led_integ += agc_diff;
//		agc_led_integ -= 0.01;
//		agc_led_integ

		aud0 *= agc_gain_lvl;											//apply agc level control
		aud1 *= agc_gain_lvl;											//apply agc level control


//		float swing = agc_gain_lvl - mve_avg;

//		vagc_lev1.push_back( swing );									//for plot

/*		
		if( agc_gain_lvl > 2.0 )
			{
			agc_gain_lvl *= agc_gain_lvl*agc_gain_lvl;
			}

		if( agc_gain_lvl < 0.7 )
			{
			agc_gain_lvl = 1.0 / (agc_gain_lvl*agc_gain_lvl);
			agc_gain_lvl *= agc_gain_lvl;
			}
*/
 
//		agc_bf0[agc_wr_bf0] = agc_gain_lvl;
//		agc_wr_bf0++;
//		if( agc_wr_bf0 >= cn_agc_bf0_siz ) agc_wr_bf0 = 0;
		
		
		
//		printf("agc_gain_lvl %f\n", agc_gain_lvl );
//		agc_gain_lvl = fabsf( log10( agc_gain_lvl) )/2.0f;
		
		
//		printf("agc_gain_lvl2  delta %f  log  %f\n", delta, agc_gain_lvl );

//		float aud_mono = (aud0+aud1)/2;
//		float agc_sum = 0;
//		agc_sum += aud_mono*aud_mono;
		}


//    sum2 = agc_process( agc0, sum2 );

//	if( b_agc_enable )
//	if( 0 )
//		{
		

		//------- agc -------
//		float f0 = fabs( iir_process( agc_iir0, aud ) );
//		vpeak_filt.push_back( fabs(aud) );

/*

		float scaler = downsample_srate / 24000.0f;

		agc_accum_sum += f0;
		agc_accum_cnt++;
		if( agc_accum_cnt >= agc_accum_sz * scaler ) 
			{
			if( agc_accum_sum > 2.0f ) agc_accum_sum = 2.0f;				//FIX THIS at present this happens when changing I/Q gain levels in gui, possibly this is calling freq tuned function which really is req TO BE FIXED
			
			agc_bf0[agc_wr_bf0] = agc_accum_sum;


			int pp = agc_wr_bf0;
			vpeak.clear();
			for( int j = 0; j < cn_agc_bf0_siz; j++ )
				{
				float f1 = agc_bf0[pp];
				pp++;
				if( pp >= cn_agc_bf0_siz ) pp = 0;
				
				vpeak.push_back( f1 );
				}






			agc_wr_bf0++;
			if( agc_wr_bf0 >= cn_agc_bf0_siz ) agc_wr_bf0 = 0;

			agc_accum_cnt = 0;
			agc_accum_sum = 0;
			}
*/


		



//		vpeak.push_back( agc_gain_lvl );
//		sum2 /= agc_gain_lvl;
//		}

//-------------------

float aud_gain_plus_minus = 1.0f;
if(aud_gain_plus_minus_db == 1 )
	{
	aud_gain_plus_minus = 3.16f;
	}
else{
	if(aud_gain_plus_minus_db == 2 )
		{
		aud_gain_plus_minus = 1/3.16f;
		}

	}


aud0 *= g_gain_aud * aud_dim_gain * aud_gain_plus_minus;
aud1 *= g_gain_aud * aud_dim_gain * aud_gain_plus_minus;


/*
	if( 1 )
		{
		bool bclip = 0;
		float clip_level_permanent = 1.3;			//need this as very low DwnSrate causes very high audio level noise for the FM Stereo decoder
		
		//---- clipper -----	
		if( aud0 < -clip_level_permanent ) { aud0 = -clip_level_permanent; bclip = 1; }
		if( aud0 > clip_level_permanent )  { aud0 =  clip_level_permanent; bclip = 1; }

		if( aud1 < -clip_level_permanent ) { aud1 = -clip_level_permanent; bclip = 1; }
		if( aud1 > clip_level_permanent )  { aud1 =  clip_level_permanent; bclip = 1; }
		//------------------


		if( bclip ) clip_audio_cnt = clip_audio_cnt_max;				//turn on clip led
		}vu
*/

	
	if( b_clip_enable )
		{
		bool bclip = 0;
		//---- clipper -----	
		if( aud0 < -clip_level_audio ) { aud0 = -clip_level_audio; bclip = 1; }
		if( aud0 > clip_level_audio ) { aud0 = clip_level_audio; bclip = 1; }

		if( aud1 < -clip_level_audio ) { aud1 = -clip_level_audio; bclip = 1; }
		if( aud1 > clip_level_audio ) { aud1 = clip_level_audio; bclip = 1; }
		//------------------


		if( bclip ) clip_audio_cnt = clip_audio_cnt_max;				//turn on clip led
		}

	vpcm_ch0[i] = aud0;
	vpcm_ch1[i] = aud1;

	float aud_mono = (aud0 + aud1 ) / 2.0f;
	
	st_aud_vu_lpf.z0 += st_aud_vu_lpf.alpha * (aud_mono - st_aud_vu_lpf.z0);	//iir filter

	aud_mono = st_aud_vu_lpf.z0;
	if( fabsf( aud_mono ) > g_aud_lev_avg ) g_aud_lev_avg = fabsf( aud_mono );
	g_aud_lev_avg *= 0.95;
	
//	g_aud_lev_avg += 0.001f * fabsf( aud_mono );
//	g_aud_lev_avg *= 0.99;
	
	vflt_probe.push_back( g_aud_lev_avg );								//for debug plotting

	}





//aud_spect_struct_audio_thrd( vpcm_ch0 );




st_plot.which = en_gls_tdm_probe_vflt0;
st_plot.srate_in = fm_decimator_srate;
st_plot.vflt0 = &vflt_probe;
plot_load_struct_audio_thrd( st_plot, (void*)&vflt_probe, (void*)0 );						//plot


st_plot.which = en_gls_demod_aud_dcblock_timedomain;

st_plot.vflt0 = &vpcm_ch0;
plot_load_struct_audio_thrd( st_plot, (void*)&vpcm_ch0, (void*)0 );						//plot




st_plot.which = en_gls_demod_aud_spect;
//st_plot.vcpx0 = 0;
st_plot.vflt0 = &vpcm_ch0;
plot_load_struct_audio_thrd( st_plot, (void*)&vpcm_ch0, (void*)0 );						//plot


st_plot.which = en_gls_demod_agc_audio_peak_timedomain;
st_plot.vflt0 = &vpeak;
plot_load_struct_audio_thrd( st_plot, (void*)&vpeak, (void*)0 );							//plot


st_plot.which = en_gls_demod_agc_gain_ctrl_timedomain;
st_plot.vflt0 = &vagc_lev;
plot_load_struct_audio_thrd( st_plot, (void*)&vagc_lev, (void*)0 );						//plot



st_plot.which = en_gls_demod_aud_clip_agc_timedomain;
st_plot.vflt0 = &vpcm_ch0;
plot_load_struct_audio_thrd( st_plot, (void*)&vpcm_ch0, (void*)0 );						//plot


st_plot.which = en_gls_filtresp_antialias_iir;
st_plot.vflt0 = &vempty_f0;												//NOTE this will not be used, filter response will be built elsewhere
plot_load_struct_audio_thrd( st_plot, (void*)&vempty_f0, (void*)0 );						//plot


st_plot.which = en_gls_filtresp_demod_hpf_iir;
st_plot.vflt0 = &vempty_f0;												//NOTE this will not be used, filter response will be built elsewhere
plot_load_struct_audio_thrd( st_plot, (void*)&vempty_f0, (void*)0 );						//plot


st_plot.which = en_gls_filtresp_demod_lpf0_iir;
st_plot.vflt0 = &vempty_f0;												//NOTE this will not be used, filter response will be built elsewhere
plot_load_struct_audio_thrd( st_plot, (void*)&vempty_f0, (void*)0 );						//plot


st_plot.which = en_gls_filtresp_demod_bpf_fir;
st_plot.vflt0 = &vempty_f0;												//NOTE this will not be used, filter response will be built elsewhere
plot_load_struct_audio_thrd( st_plot, (void*)&vempty_f0, (void*)0 );						//plot


int audio_srate_in = 1.0f / ( frame_time / vpcm_ch0.size() );	//actual audio srate from demod (note fm stereo has a halfband decimator so rate is: downsample_srate/2 );

if(vb0)printf( "\ndemod_iso() - RRRRRRRRRRR Resampler vpcm_ch0.size() %d  frame_time %f  audio_srate_in %d --> aud_op_srate %d Hz\n", (int)vpcm_ch0.size(), frame_time, audio_srate_in, aud_op_srate );   


// ---- linear upsample 24k -> 48k ----
if( vpcm_ch0.size() == framecnt/2 )										//can do an exact 2x upsample?
	{
	if(vb0)printf( "demod_iso() - LLLLLLLLL LinearInterp  vpcm.size() == framecnt/2   vpcm.size() %d   framecnt %d\n", (int)vpcm.size(), framecnt );
	vpcm_cache_ch0.clear();
	vpcm_cache_ch1.clear();
	for (int i = 0; i < vpcm_ch0.size(); i++)
		{
		float f0 = vpcm_ch0[i];
		float f1 = vpcm_ch1[i];
		for (int j = 0; j < 2; j++)										//get 2 samples, one is halfway between original samples
			{
			
			up2x_ch0.process( f0, bf_upsample_ch0 );
			up2x_ch1.process( f1, bf_upsample_ch1 );

			vpcm_cache_ch0.push_back( bf_upsample_ch0[j] / 2.0f );
			vpcm_cache_ch1.push_back( bf_upsample_ch1[j] / 2.0f );
			}
		}
	if(vb0)printf( "demod_iso() - linear 2x upsample   vpcm_ch0.size() %d   framecnt %d\n", (int)vpcm_ch0.size(), (int)vpcm_cache_ch0.size() );
	//--------------------------------------
	}
else{


	//------ fractional resampler (inefficient)---------
	if( vpcm_ch0.size() != framecnt )
		{
		if(vb0)printf( "demod_iso() - FFFFFFFFF Fractional resample vpcm_ch0.size() != framecnt  %d --> %d\n", (int)vpcm_ch0.size(), framecnt );
		float factor = (float)framecnt / vpcm_ch0.size();

		low_pass_real_sinc_float( vpcm_ch0,  vpcm_cache_ch0, audio_srate_in, aud_op_srate, framecnt, 1.0f, "final pcm audio rate ch0", head_bf0 );
		low_pass_real_sinc_float( vpcm_ch1,  vpcm_cache_ch1, audio_srate_in, aud_op_srate, framecnt, 1.0f, "final pcm audio rate ch1", head_bf1 );
		
		
		if(vb0)printf( "demod_iso() - fractional resample (factor %.4f) vpcm_ch0 %d ---> vpcm_cache_ch0 %d\n", factor, (int)vpcm_ch0.size(), (int)vpcm_cache_ch0.size() );
		}
	else{
		if( audio_srate_in != aud_op_srate )
			{
			if(vb0)printf( "demod_iso() - MMMMMMMMMMM audio_srate_in != aud_op_srate   %d --> %d Hz\n", audio_srate_in, (int)aud_op_srate );
			float factor = (float)aud_op_srate / audio_srate_in;

			low_pass_real_sinc_float( vpcm_ch0,  vpcm_cache_ch0, audio_srate_in, aud_op_srate, framecnt, 1.0f, "final pcm audio rate ch0", head_bf0 );
			low_pass_real_sinc_float( vpcm_ch1,  vpcm_cache_ch1, audio_srate_in, aud_op_srate, framecnt, 1.0f, "final pcm audio rate ch1", head_bf1 );

			if(vb0)printf( "demod_iso() - MMMMMMMMMMM fractional resample (factor %.4f) vpcm_ch0 %d ---> vpcm_cache_ch0 %d\n", factor, (int)vpcm_ch0.size(), (int)vpcm_cache_ch0.size() );
			}
		else{
			if(vb0)printf( "demod_iso() - NO reampling was performed\n" );
			vpcm_cache_ch0 = vpcm_ch0;									//no resampling if here
			vpcm_cache_ch1 = vpcm_ch1;	
			}
		}
	//-------------------------------------
	}






//copy for 'update_audio()'
for( int i = 0; i < vpcm_cache_ch0.size(); i++ )
	{
//	float ff = vpcm_cache_ch0[ i ];
	
//	float clip_limit = 100.0f;
//	if( ff < -clip_limit ) ff = -clip_limit;
//	if( ff > clip_limit ) ff = clip_limit;
		
//	if( std::isnan(ff) )
//		{
//		ff = 0.0f;
//		}
	
	vaudio_ch0.push_back( vpcm_cache_ch0[ i ] );						//load vector with audio samples for audiocard o/p
	vaudio_ch1.push_back( vpcm_cache_ch1[ i ] );
	}


st_plot.which = en_gls_demod_aud_resampler_ch0_timedomain;
st_plot.srate_in = 0;
st_plot.vflt0 = &vaudio_ch0;
plot_load_struct_audio_thrd( st_plot, (void*)&vaudio_ch0, (void*)0 );				//plot


st_plot.which = en_gls_demod_aud_resampler_ch1_timedomain;
st_plot.srate_in = 0;
st_plot.vflt0 = &vaudio_ch1;
plot_load_struct_audio_thrd( st_plot, (void*)&vaudio_ch1, (void*)0 );				//plot


st_plot.which = en_gls_demod_aud_resampler_ch0_spect;
st_plot.srate_in = 0;
st_plot.vflt0 = &vaudio_ch0;
plot_load_struct_audio_thrd( st_plot, (void*)&vaudio_ch0, (void*)0 );				//plot


st_plot.which = en_gls_demod_aud_resampler_ch1_spect;
st_plot.srate_in = 0;
st_plot.vflt0 = &vaudio_ch1;
plot_load_struct_audio_thrd( st_plot, (void*)&vaudio_ch1, (void*)0 );				//plot



if( vaudio_ch0.size() > 16384 )
	{
	printf( "demod_iso() - vaudio_ch0.size() %d is too big, clearing it\n", (int)vaudio_ch0.size() );
	vaudio_ch0.clear();
	vaudio_ch1.clear();
	}

//if(vb0)printf( "demod_iso() - vdownsamp.size() %d, vpcm.size() %d, vaudio.size() %d\n", vdownsamp.size(), vpcm.size(), vaudio.size(), rtl_bf0_wr, rtl_bf0_rd );

vpcm_cache_ch0.clear();	
vpcm_cache_ch1.clear();	



demod_cnt++;
}







/*
float moving_average(float x)
{
static float buffer[ cn_moving_avg_siz ] = {0};
static int index = 0;
static float sum = 0.0f;

sum -= buffer[ index ];				// remove oldest
buffer[ index++ ] = x;				// store new
sum += x;							// add newest

if(index >= cn_moving_avg_siz) index = 0;

return sum * (1.0f / cn_moving_avg_siz);
}
*/




/*
void agc_init(AGC& a, float fs)
{
    const float env_tau  = 0.02f;   // 20 ms envelope
    const float gain_tau = 0.10f;   // 100 ms gain smoothing

    a.alpha = 1.0f - expf(-1.0f / (fs * env_tau));
    a.beta  = 1.0f - expf(-1.0f / (fs * gain_tau));

    a.target   = 0.2f;   // ≈ -14 dBFS
    a.max_gain = 30.0f;  // +30 dB limit
}






inline float agc_process(AGC& a, float x)
{
    const float eps = 1e-12f;

    //
    // Envelope follower
    //
    float mag = fabsf(x);
    a.env += a.alpha * (mag - a.env);

    //
    // Gain computer
    //
    float desired_gain = a.target / (a.env + eps);

    //
    // Gain smoothing
    //
    a.gain += a.beta * (desired_gain - a.gain);

    //
    // Gain limiting (important for SDR noise!)
    //
    if (a.gain > a.max_gain)
        a.gain = a.max_gain;

    //
    // Apply gain
    //
    return x * a.gain;
}
*/






//called by audio proc thread only, loads 'st_aud_spect_atm[]', thread safe
//refer 'aud_spect_gui_thrd()'

void aud_spect_struct_audio_thrd( vector<float> &vflt0 )
{
//printf( "plot_load_struct_audio_thrd() - st.which %d   gph_loc_sel_tmp %d\n", st.which, gph_loc_sel_tmp ); 
	
//if( st.which != 0 ) return;

//move struct for gui thread to access and do fft wfm/spectral plotting
if( aud_spect_idx_atm.load(memory_order_acquire) == -1 )	//is gui thread done processing a 'st_plot_atm[]' and is ready for more data?
	{
//printf( "aud_spect_struct_audio_thrd() - st.vcpx0->size() ); 
	//move struct with thread safety

	st_aud_spect_atm_tag st0;
	st0.srate_in = downsample_srate;
	st0.vflt0 = vflt0;
	
	st_aud_spect_atm[ aud_spect_idx ] = st0;
	
	aud_spect_idx_atm.store( aud_spect_idx, memory_order_release );

//printf( "aud_spect_struct_audio_thrd() - aud_spect_idx %d  st0.srate_in %d  st0.vflt0 %d\n", aud_spect_idx, st0.srate_in, st0.vflt0.size() );
	aud_spect_idx ^= 1;
	}
}











void cb_gph2_left_click( void* args )
{

fast_mgraph *og = (fast_mgraph*)args;

double mx, my;
og->gph[0]->get_mouse_position_relative_to_trc (0, mx, my );

int px, py;
og->gph[0]->get_mouse_pixel_position_on_background( px, py );

printf( "cb_gph2_left_click() px %d %d   mx %f  %f\n", px, py,  mx, my );
}





//fast_mgraph gph2;



//gui thrd reads 'st_aud_spect_atm[]', thread safe
//refer 'aud_spect_struct_audio_thrd()'
void aud_spect_gui_thrd()
{

if( !(wnd_aud_spect->shown() && wnd_aud_spect->visible()) ) return;							//wnd minimized?

if( b_dbg_no_graph_update ) return;


//get data from audio proc thread for fft and wfm/spectral plotting
int idx = aud_spect_idx_atm.load(std::memory_order_acquire);			//this ensures thread safe access


st_aud_spect_atm_tag st;
if( idx != -1 )
	{
	st = st_aud_spect_atm[idx];

	aud_spect_idx_atm.store(-1, std::memory_order_release);				//flag struct processed
	goto got_struct;
	}
else{
	return;
	}

got_struct:


//dsp_utils_code::make_sine( downsample_srate, 1500, 1.0, 0.0, st.vflt0.size(), st.vflt0 ); 
//void make_sine( unsigned int srate, double freq0, double ampl0, double dc_offset, unsigned int cnt, std::vector<T> &vsine )

int dwnsmp_factor = 1;

if( downsample_srate >= 12000 ) dwnsmp_factor = 2;
if( downsample_srate >= 24000 ) dwnsmp_factor = 4;
if( downsample_srate >= 48000 ) dwnsmp_factor = 8;
if( downsample_srate >= 96000 ) dwnsmp_factor = 16;
if( downsample_srate >= 192000 ) dwnsmp_factor = 32;

vector<float> vdwn;
low_pass_srconv_float( st.vflt0,  vdwn, dwnsmp_factor, filt_prev_idx14, now_r14 );			//real only decimate


vector<st_spect_tag> vsp;



//printf( "plot_grph1_gui_thrd() - idx %d  st.vflt0.size() %d\n", idx, st.vflt0.size() );


int b_destroy_only = 0;
fftw_build_if_req( en_fia_aud_spect_notch, b_destroy_only, "fftw fwd real to cplx: for 'en_fia_aud_spect_notch'", en_fpt_fwd_real_to_cplx, vdwn.size() );	//check if fftw needs to be built/resized


float fft_gain = 1;
if( pref_zero_padding_for_fft ) fft_gain = 1.0f;

vector <filter_code::st_cplex_tag> vcpx_sp;
fftw_fwd_real_cplx( en_fia_aud_spect_notch, vdwn, vcpx_sp, fft_gain );		//go to freq domain using an fft


complex_fft_displayable( downsample_srate, 0, vcpx_sp, vsp, 1, 1 );

/*
	vector<float>vgph_x;
	vector<float>vgph_y0;
	for( int i = 0; i < vsp.size(); i++ )
		{
//				float freq = j*(bw_bin);
	
		
		vgph_x.push_back( vsp[i].freq );
		vgph_y0.push_back( vsp[i].ampl );
		
//				j++;
		}

	gph2.plotxy_vfloat( 0, vgph_x, vgph_y0 );
*/




//avegage audio spectrum across frames to show up any consistent whir/whistle spectra so user can see what freqs to notch out
if( 1 )
	{
	st_audio_spect_avg_tag *os = &st_aud_spect_avg;

	os->num_bf_in_use = cn_spect_audio_avg_cnt;


	if( vsp.size() > cn_spect_audio_avg_bf_max )
		{
		printf( "aud_spect_gui_thrd() vsp.size() %d is too big, limit enforced to 'cn_spect_audio_avg_bf_max' %d\n", vsp.size(), cn_spect_audio_avg_bf_max );
		os->spect_size = cn_spect_audio_avg_bf_max;
		}
	else{
		os->spect_size = vsp.size();
		}

	vector<float>vavg;


	//store cur fft
	for( int i = 0; i < os->spect_size; i++ )
		{
		os->bf_spect_ch0[ os->cur_wr_bf ][i] = vsp[i].ampl;
		}

	//average cur and prev ffts
	for( int i = 0; i < os->spect_size; i++ )
		{
		int bfidx = os->cur_wr_bf;

		float sum_ampl = 0;

		for( int j = 0; j < os->num_bf_in_use; j++ )					//for each fft in buf
			{
			sum_ampl += os->bf_spect_ch0[ bfidx ][i];
			bfidx++;		
			if( bfidx >= os->num_bf_in_use ) bfidx -= os->num_bf_in_use;
			}

		vavg.push_back( sum_ampl / cn_spect_audio_avg_cnt );			//build a single avg'd fft
		}


	if( 1 )
		{
		//dc block
		for( int i = 0; i < vavg.size(); i++ )
			{
			float alpha = 0.99985f;										//this val is only a guestimate
			
			float sum1 = vavg[i] - os->z0;	
			float sum2 = sum1 + alpha * os->z1;

			os->z0 = vavg[i];
			os->z1 = sum2;
			vavg[i] = sum2;
			}
		}


	os->cur_wr_bf++;
	if( os->cur_wr_bf >= os->num_bf_in_use ) os->cur_wr_bf = 0;

	vector<float>vgph_x;
	vector<float>vgph_y0;
	
	int siz = vavg.size();
	int j = 0;
	for( int i = siz/2; i < siz; i++ )									//only show positive bins
		{

		int frq = j * (downsample_srate / siz );
		
		vgph_x.push_back( frq );
		vgph_y0.push_back( vavg[i] );
		j++;	
		}

//	if( vgph_y0.size() >= 1 )
//		{
//		vgph_y0[0] = 0.01f;										//force a unity value to limit graph auto scaling
//		}


	wnd_aud_spect->plot( downsample_srate / dwnsmp_factor, vgph_y0 );

//	gph2.plotxy_vfloat( 0, vgph_x, vgph_y0 );

	}

return;
}













/*

void make_sin_complex( unsigned int srate, double freq0, double ampl, double dc_offset, unsigned int cnt, vector<filter_code::st_cplex_tag> &vcpx )
{
vcpx.clear();
vcpx.resize( cnt );

double time_per_sample = 1.0 / (double)srate;

double theta0 = 0.0;
double theta_inc = freq0 * twopi * time_per_sample;

for( int i = 0; i < cnt; i++ )
	{
	filter_code::st_cplex_tag oc;
	
	oc.real = dc_offset + ampl * cos( theta0 );
	oc.imag = dc_offset + ampl * sin( theta0 );
	
	vcpx[i] = oc;

	theta0 += theta_inc;

	// wrap phase for both +ve and -ve freq
	if( theta0 >= twopi ) theta0 -= twopi;
	if( theta0 < 0.0 ) theta0 += twopi;
	}
}

*/










//fftw plot fft items
//refer 'plot_load_struct_audio_thrd()' plot_using_struct()' 'plot_using_vectors_eval()' 'plot_load_struct_audio_thrd()' 'plot_grph1_gui_thrd()'

//int plot_fftwc_siz = 0;
//int plot_fftwc_siz_last = -1;											//used with 'plot_fftwc_siz' to detect if resize of fftw storage and plan is required, occurs when plot vector size has changes
//int plot_fftwr_siz = 0;
//int plot_fftwr_siz_last = -1;											//used with 'plot_fftwr_siz' to detect if resize of fftw storage and plan is required, occurs when plot vector size has changes

fftw_complex *plot_fftwic = 0;											//'plot_load_vectors_eval()' fftw fwd dynamically alloc storage and plans
fftw_complex *plot_fftwoc = 0;
double* plot_fftwir = 0;
double* plot_fftwor = 0;

fftw_plan plot_fftwp_fwdc = 0;
fftw_plan plot_fftwp_fwdr = 0;















int taper_windowing_size_last1 = 0;			//these are used to detect if a newly calc'd taper window is required (due to a signal sample block size change)
vector<float> vtaper_window1;
float taper_window_gain_corr1;


		
void taper_zeropad_complex_1( bool pref_taper_windowing_for_fft_in, bool pref_zero_padding_for_fft_in, int pref_zero_padding_for_fft_cnt_in, vector<filter_code::st_cplex_tag> &vcpx )
{
if( pref_taper_windowing_for_fft_in )
	{
	if( taper_windowing_size_last1 != vcpx.size() )						//reduce excessive calcs
		{
//					printf( "taper_zeropad_complex_1() - calc of 'vtaper_window1', size %d\n", vtaper_window1.size() );
	
		filter_code::window_function_float( filter_code::fwt_hann, vcpx.size(), vtaper_window1 );
		
		taper_window_gain_corr1 = filter_code::window_calc_normalisation_factor_float( vtaper_window1 );
		
		taper_windowing_size_last1 = vcpx.size();
		}
					
	for( int j = 0; j < vcpx.size(); j++ )
		{
		vcpx[j].real *= vtaper_window1[j] * taper_window_gain_corr1;
		vcpx[j].imag *= vtaper_window1[j] * taper_window_gain_corr1;
		}
	}
	

bool bzeropad = pref_zero_padding_for_fft_in;
int zeropad_factor = pref_zero_padding_for_fft_cnt_in;
if( bzeropad )
	{
	int cnt = vcpx.size();
//				printf( "taper_zeropad_complex_1() - zeropad_factor %d,  vcpx00.size() %d\n", zeropad_factor, (int)vcpx00.size() );
	filter_code::st_cplex_tag oc;
	oc.real = 0;
	oc.imag = 0;
	
	for( int j = 0; j < cnt * (zeropad_factor); j++ )
		{
		vcpx.push_back( oc );
		}
	}
}
			







//apply windowing and or zero padding as required
void taper_zeropad_real_1( bool pref_taper_windowing_for_fft_in, bool pref_zero_padding_for_fft_in, int pref_zero_padding_for_fft_cnt_in, vector<float> &vflt )
{
if( pref_taper_windowing_for_fft_in )
	{
	if( taper_windowing_size_last1 != vflt.size() )						//reduce excessive calcs
		{
//					printf( "taper_zeropad_real_1() - calc of 'vtaper_window1', size %d\n", vtaper_window1.size() );
	
		filter_code::window_function_float( filter_code::fwt_hann, vflt.size(), vtaper_window1 );
		
		taper_window_gain_corr1 = filter_code::window_calc_normalisation_factor_float( vtaper_window1 );
		
		taper_windowing_size_last1 = vflt.size();
		}
					
	for( int j = 0; j < vflt.size(); j++ )
		{
		vflt[j]*= vtaper_window1[j] * taper_window_gain_corr1;
		}
	}
	

bool bzeropad = pref_zero_padding_for_fft_in;
int zeropad_factor = pref_zero_padding_for_fft_cnt_in;
if( bzeropad )
	{
	int cnt = vflt.size();
//				printf( "taper_zeropad_real_1() - zeropad_factor %d,  vflt.size() %d\n", zeropad_factor, (int)vflt.size() );
	
	for( int j = 0; j < cnt * (zeropad_factor); j++ )
		{
		vflt.push_back( 0.0f );
		}
	}
}
			













//called by audio proc thread only, loads 'st_plot_atm[]', thread safe
//refer 'plot_load_struct_audio_thrd()' plot_using_struct()' 'plot_using_vectors_eval()' 'plot_load_struct_audio_thrd()' 'plot_grph1_gui_thrd()'

void plot_load_struct_audio_thrd( st_plot_probe_tag &st, void* vv0, void* vv1 )
{
//printf( "plot_load_struct_audio_thrd() - st.which %d   gph_loc_sel_tmp %d\n", st.which, gph_loc_sel_tmp ); 
if( st.which != gph_loc_sel_tmp ) return;
	
//if( st.which != 0 ) return;

//move struct for gui thread to access and do fft wfm/spectral plotting
if( plot_prep_idx_atm.load(memory_order_acquire) == -1 )	//is gui thread done processing a 'st_plot_atm[]' and is ready of more data?
	{
//printf( "plot_load_struct_audio_thrd() - which %d, st_plot.vcpx0  %d\n", st.which, st.vcpx0->size() ); 
	//move struct with thread safety

	st_plot_probe_atm_tag st0;
	st0.which = st.which;
	st0.srate_in = st.srate_in;
/*
	if( st.vcpx0 != 0 ) st0.vcpx0 = *st.vcpx0;
	if( st.vcpx1 != 0 ) st0.vcpx1 = *st.vcpx1;
	if( st.vsp0 != 0 ) st0.vsp0 = *st.vsp0;
	if( st.vdble0 != 0 ) st0.vdble0 = *st.vdble0;
	if( st.vflt0 != 0 ) st0.vflt0 = *st.vflt0;
	if( st.vflt1 != 0 ) st0.vflt1 = *st.vflt1;
*/
	
	if( st.vcpx0 == vv0 ) st0.vcpx0 = *st.vcpx0;							//grab only the passed vector
	if( st.vcpx1 == vv0 ) st0.vcpx1 = *st.vcpx1;
	if( st.vsp0 == vv0 ) st0.vsp0 = *st.vsp0;
	if( st.vdble0 == vv0 ) st0.vdble0 = *st.vdble0;
	if( st.vflt0 == vv0 ) st0.vflt0 = *st.vflt0;

	if( (vv1 != 0 ) && (st.vflt1 == vv1 ) )  st0.vflt1 = *st.vflt1;
	
	st_plot_atm[plot_prep_idx] = st0;
	
	plot_prep_idx_atm.store( plot_prep_idx, memory_order_release );

//printf( "plot_load_struct_audio_thrd() - plot_prep_idx %d\n", plot_prep_idx );
	plot_prep_idx ^= 1;
	}
}







//gui thrd reads 'st_plot_atm[]', thread safe
//refer 'plot_load_struct_audio_thrd()' plot_using_struct()' 'plot_using_vectors_eval()' 'plot_load_struct_audio_thrd()' 'plot_grph1_gui_thrd()'
void plot_grph1_gui_thrd()
{

//get data from audio proc thread for fft and wfm/spectral plotting
int idx = plot_prep_idx_atm.load(std::memory_order_acquire);			//this ensures thread safe access

st_plot_probe_atm_tag st;
if( idx != -1 )
	{
//printf( "plot_grph1_gui_thrd() - idx %d\n", idx );

	//get data from vectors
	
	st = st_plot_atm[idx];

/*	
	if( ( i_fftw_trig_plan_create_state == 0 ) || ( i_fftw_trig_plan_create_state == 1 ) )
		{
printf( "plot_grph1_gui_thrd() - i_fftw_trig_plan_create_state %d\n", i_fftw_trig_plan_create_state );
		wnd_rtl_graph->fftw_adjust_plans();
		i_fftw_trig_plan_create_state = 2;
		}
*/ 
//	printf("temp_code0() - HHHHHHHHHHHHHHHHHHHHHH idx %d cnt %d  iq %f \n", idx, cnt, bf1_iq[idx][0].real );

//	for ( int i = 0; i < cnt; i++ )
//		{
//		viq.push_back( bf1_iq[idx][i] );
//		}

	plot_prep_idx_atm.store(-1, std::memory_order_release);				//flag struct processed
	goto got_struct;
	}
else{
	return;
	}

got_struct:
plot_using_struct( st );

return;
}












//called by gui thread once gui thread has obtained struct in a thread safe manner
//refer 'plot_load_struct_audio_thrd()'  'plot_using_vectors_eval()' 'plot_load_struct_audio_thrd()' 'plot_grph1_gui_thrd()'
void plot_using_struct( st_plot_probe_atm_tag &st )
{
plot_using_vectors_eval( st.which, st.srate_in, st.vcpx0, st.vcpx1, st.vsp0, st.vdble0, st.vflt0, st.vflt1 );
}





//set 'srate_in' to a non zero value to specify a specific srate, else the default coded srate var will be used
//refer 'plot_load_struct_audio_thrd()' plot_using_struct()' 'plot_grph1_gui_thrd()'
void plot_using_vectors_eval( en_graph_loc_sel_tag which, unsigned int srate_in, vector <filter_code::st_cplex_tag> vcpx0, vector <filter_code::st_cplex_tag> vcpx1, vector<st_spect_tag> vsp0, vector<double> vdble0, vector<float> vflt0, vector<float> vflt1 )
{
bool vb = 0;

if( b_dbg_no_graph_update ) return;

if(vb)printf("plot_using_vectors_eval() - which %d\n", which );


if( i_fftw_trig_plan_create_state != 2 ) return;						//plans NOT created?



if( pref_zero_padding_for_fft_cnt > cn_pref_zero_padding_for_fft_cnt_max )
	{
	printf("plot_using_vectors_eval() - 'pref_zero_padding_for_fft_cnt' is %d and out of range, setting max of: %d\n", pref_zero_padding_for_fft_cnt, cn_pref_zero_padding_for_fft_cnt_max );
	pref_zero_padding_for_fft_cnt = cn_pref_zero_padding_for_fft_cnt_max;
	}

if( pref_zero_padding_for_fft_cnt < cn_pref_zero_padding_for_fft_cnt_min )
	{
	printf("plot_using_vectors_eval() - 'pref_zero_padding_for_fft_cnt' is %d and out of range, setting min of: %d\n", pref_zero_padding_for_fft_cnt, cn_pref_zero_padding_for_fft_cnt_min );
	pref_zero_padding_for_fft_cnt = cn_pref_zero_padding_for_fft_cnt_min;
	}


//---------------- graph plot monitoring point -------------------------
if( which == en_gls_IQ_timedomain )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();


			int siz = vcpx0.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
	//				int ifreq = g_freq_tune_offset - siz*bw_bin/2 + bw_bin + i;

				vgph1_x.push_back( i*time_per_sample );
				vgph1_y0.push_back( (vcpx0)[i].real );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 0 - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------








//---------------- graph plot monitoring point -------------------------
if( which == en_gls_IQ_spect )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
			double bw = g_dev_bw;
			if( srate_in != 0 ) bw = srate_in;							//caller specified srate? 

//			double bw_bin = bw / (double)plot_fftwr_siz;


//			int fftsz = vcpx0.size();
//			double bw = downsample_srate;
//			double bw_bin = bw / (double)fftsz;


//			vector <filter_code::st_cplex_tag> vcpx00 = vcpx0;



//			make_sin_complex( g_dev_bw, 46.88*10000, 1.0, 0.5, vcpx0.size(), vcpx00 );
//			make_sin_complex( g_dev_bw, 500, 1.0, 0.5, vcpx0.size(), vcpx00 );


			taper_zeropad_complex_1( pref_taper_windowing_for_fft, pref_zero_padding_for_fft, pref_zero_padding_for_fft_cnt, vcpx0 );
	
/*
			if( pref_taper_windowing_for_fft )
				{
				if( taper_windowing_size_last1 != vcpx00.size() )				//reduce excessive calcs
					{
//					printf( "plot_load_vectors_eval() - calc of 'vtaper_window1', size %d\n", vtaper_window1.size() );
				
					filter_code::window_function_float( filter_code::fwt_hann, vcpx00.size(), vtaper_window1 );
					
					taper_window_gain_corr1 = filter_code::window_calc_normalisation_factor_float( vtaper_window1 );
					
					taper_windowing_size_last1 = vcpx00.size();
					}
								
				for( int j = 0; j < vcpx00.size(); j++ )
					{
					vcpx00[j].real *= vtaper_window1[j] * taper_window_gain_corr1;
					vcpx00[j].imag *= vtaper_window1[j] * taper_window_gain_corr1;
					}

	//			int cnt = viq_local_zero_pad.size();
	//			for( int i = 0; i < cnt; i++ )
	//				{
	//				float f0 = 0.5f - (0.5f * cosf( (twopi * i) / (cnt - 1) ) );		//hann window test
	//				vtaper_window.push_back( f0 );
	//				}
//				b_build_taper_window0 = 0;
				}


			bool bzeropad = pref_zero_padding_for_fft;
			int zeropad_factor = pref_zero_padding_for_fft_cnt;
			if( bzeropad )
				{
				int cnt = vcpx00.size();
//				printf( "plot_using_vectors_eval() - zeropad_factor %d,  vcpx00.size() %d\n", zeropad_factor, (int)vcpx00.size() );
				filter_code::st_cplex_tag oc;
				oc.real = 0;
				oc.imag = 0;
				
//				bw_bin /= (float)zeropad_factor;						//adj bwidth as padding
				for( int j = 0; j < cnt * (zeropad_factor); j++ )
					{
					vcpx00.push_back( oc );
					}
				}

*/


			int b_destroy_only = 0;
			fftw_build_if_req( en_fia_plot_gph1, b_destroy_only, "1: fftw fwd cplx to cplx: for plot gph1 'en_gls_IQ_spect'", en_fpt_fwd_cplx_to_cplx, vcpx0.size() );	//check if fftw needs to be built/resized

			float fft_gain = 1;
			if( pref_zero_padding_for_fft ) fft_gain *= pref_zero_padding_for_fft_cnt+1;

				
			fftw_fwd_clpx_cplx( en_fia_plot_gph1, vcpx0, vcplex_sp, fft_gain );		//go to freq domain using an fft


//printf( " plot_load_vectors_eval() - vcplex_sp[0].real %f %f\n", vcplex_sp[0].real, vcplex_sp[0].imag );


//			complex_fwd_fft_multi( vcpx00, vcplex_sp, "for 'en_gls_IQ_spect' plot", en_ftid_gui );			//go to freq domain using an fft
			vector<st_spect_tag> vsp;
			complex_fft_displayable( bw, g_freq_tune, vcplex_sp, vsp, 1, 1 );

			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

//printf( " plot_load_vectors_eval() - fftsz %d, bw_bin %f\n", fftsz, bw_bin );


/*
			int siz = vsp.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
				int ifreq = -siz*(bw_bin/2) + bw_bin * i;

				vgph1_x.push_back( ifreq );
				vgph1_y0.push_back( vsp[i].ampl );
				}
*/


			int siz = vsp.size();
//			double bw = g_dev_bw;
//			double bw_bin = bw/(siz - 2);								//-2 as DC and nyquist bins reduce neg freq and positive freq bins by 1 each
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			
//			int j = -(siz/2 - 1);										//start from neg most freq, e.g if N=512: 254 neg freqs, DC, +254 pos freq, nyquist (512 total)									
			
			for( int i = 0; i < siz; i++ )
				{
//				float freq = j*(bw_bin);

				vgph1_x.push_back( vsp[i].freq );
				vgph1_y0.push_back( vsp[i].ampl );
				
//				j++;
				}




			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 0 - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------







//---------------- graph plot monitoring point -------------------------
if( which == en_gls_sub_tuner_spect )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
			double bw = g_dev_bw;
			if( srate_in != 0 ) bw = srate_in;							//caller specified srate? 

//			int fftsz = vcpx0.size();
//			double bw = g_dev_bw;
//			double bw_bin = bw / (double)fftsz;


//			vector <filter_code::st_cplex_tag> vcpx00 = vcpx0;


			taper_zeropad_complex_1( pref_taper_windowing_for_fft, pref_zero_padding_for_fft, pref_zero_padding_for_fft_cnt, vcpx0 );


			int b_destroy_only = 0;
			fftw_build_if_req( en_fia_plot_gph1, b_destroy_only, "1: fftw fwd cplx to cplx: for plot gph1 'en_gls_IQ_spect_sub_tuner'", en_fpt_fwd_cplx_to_cplx, vcpx0.size() );	//check if fftw needs to be built/resized


			float fft_gain = 1;
			if( pref_zero_padding_for_fft ) fft_gain *= pref_zero_padding_for_fft_cnt+1;

			fftw_fwd_clpx_cplx( en_fia_plot_gph1, vcpx0, vcplex_sp, fft_gain );		//go to freq domain using an fft

//			complex_fwd_fft_multi( vcpx00, vcplex_sp, "for 'en_gls_IQ_spect' plot", en_ftid_gui );			//go to freq domain using an fft
			vector<st_spect_tag> vsp;
			complex_fft_displayable( bw, g_freq_tune, vcplex_sp, vsp, 1, 1 );

			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

//printf( " plot_load_vectors_eval() - fftsz %d, bw_bin %f\n", fftsz, bw_bin );

			int siz = vsp.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
//				int ifreq = -siz*(bw_bin/2) + bw_bin * i;

				vgph1_x.push_back(  vsp[i].freq );
				vgph1_y0.push_back( vsp[i].ampl );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 0 - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------






//---------------- graph plot monitoring point -------------------------
if( which == en_gls_dwncnv_aa_spect )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
			double bw = g_dev_bw;
			if( srate_in != 0 ) bw = srate_in;							//caller specified srate? 

//			int fftsz = vcpx0.size();
//			double bw = g_dev_bw;
//			double bw_bin = bw / (double)fftsz;


//			vector <filter_code::st_cplex_tag> vcpx00 = vcpx0;


			taper_zeropad_complex_1( pref_taper_windowing_for_fft, pref_zero_padding_for_fft, pref_zero_padding_for_fft_cnt, vcpx0 );


			int b_destroy_only = 0;
			fftw_build_if_req( en_fia_plot_gph1, b_destroy_only, "1: fftw fwd cplx to cplx: for plot gph1 'en_gls_IQ_spect_dwn_aa'", en_fpt_fwd_cplx_to_cplx, vcpx0.size() );	//check if fftw needs to be built/resized


			float fft_gain = 1;
			if( pref_zero_padding_for_fft ) fft_gain *= pref_zero_padding_for_fft_cnt+1;

			fftw_fwd_clpx_cplx( en_fia_plot_gph1, vcpx0, vcplex_sp, fft_gain );		//go to freq domain using an fft

//			complex_fwd_fft_multi( vcpx00, vcplex_sp, "for 'en_gls_IQ_spect' plot", en_ftid_gui );			//go to freq domain using an fft
			vector<st_spect_tag> vsp;
			complex_fft_displayable( bw, g_freq_tune, vcplex_sp, vsp, 1, 1 );

			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

//printf( " plot_load_vectors_eval() - fftsz %d, bw_bin %f\n", fftsz, bw_bin );

			int siz = vsp.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
//				int ifreq = -siz*(bw_bin/2) + bw_bin * i;

				vgph1_x.push_back(  vsp[i].freq );
				vgph1_y0.push_back( vsp[i].ampl );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 0 - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------








//---------------- graph plot monitoring point -------------------------
if( which == en_gls_downcnv_timedomain )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{

			float dt = 1.0f / downsample_srate;
			
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

			int siz = vcpx0.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
	//				int ifreq = g_freq_tune_offset - siz*bw_bin/2 + bw_bin + i;

				vgph1_x.push_back( i * dt );
				vgph1_y0.push_back( (vcpx0)[i].real );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 0 - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------









//---------------- graph plot monitoring point -------------------------
if( which == en_gls_downcnv_spect )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
			double bw = downsample_srate;
			if( srate_in != 0 ) bw = srate_in;							//caller specified srate? 


			taper_zeropad_complex_1( pref_taper_windowing_for_fft, pref_zero_padding_for_fft, pref_zero_padding_for_fft_cnt, vcpx0 );


			int b_destroy_only = 0;
			fftw_build_if_req( en_fia_plot_gph1, b_destroy_only, "1: fftw fwd cplx to cplx: for plot gph1 'en_gls_IQ_spect_downcnv'", en_fpt_fwd_cplx_to_cplx, vcpx0.size() );	//check if fftw needs to be built/resized



			float fft_gain = 1;
			if( pref_zero_padding_for_fft ) fft_gain *= pref_zero_padding_for_fft_cnt+1;

			fftw_fwd_clpx_cplx( en_fia_plot_gph1, vcpx0, vcplex_sp, fft_gain );		//go to freq domain using an fft



//			int fftsz = vcpx0.size();
//			double bw = downsample_srate;
//			double bw_bin = bw / (double)fftsz;

//			complex_fwd_fft_multi( vcpx0, vcplex_sp, "for 'en_gls_IQ_spect_downcnv' plot", en_ftid_gui );			//go to freq domain using an fft
			vector<st_spect_tag> vsp;
			complex_fft_displayable( downsample_srate, g_freq_tune, vcplex_sp, vsp, 1, 1 );

//printf( " plot_load_vectors_eval() - vcpx0.size() %d\n", vcpx0.size() );
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();


			int siz = vsp.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
//				int ifreq = g_freq_tune_offset - siz*bw_bin/2 + bw_bin + i;
//				int ifreq = -siz*(bw_bin/2) + bw_bin * i;

				vgph1_x.push_back( vsp[i].freq );
				vgph1_y0.push_back( vsp[i].ampl );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 0 - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------







//---------------- graph plot monitoring point -------------------------
if( which == en_gls_bandpass_fir_timedomain )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{

			float dt = 1.0f / downsample_srate;
			
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

			int siz = vcpx0.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
	//				int ifreq = g_freq_tune_offset - siz*bw_bin/2 + bw_bin + i;

				vgph1_x.push_back( i * dt );
				vgph1_y0.push_back( (vcpx0)[i].real );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 0 - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------






//---------------- graph plot monitoring point -------------------------
if( which == en_gls_bandpass_fir_spect )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
			double bw = downsample_srate;
			if( srate_in != 0 ) bw = srate_in;							//caller specified srate? 


			taper_zeropad_complex_1( pref_taper_windowing_for_fft, pref_zero_padding_for_fft, pref_zero_padding_for_fft_cnt, vcpx0 );


			int b_destroy_only = 0;
			fftw_build_if_req( en_fia_plot_gph1, b_destroy_only, "1: fftw fwd cplx to cplx: for plot gph1 'en_gls_IQ_spect_downcnv_filter'", en_fpt_fwd_cplx_to_cplx, vcpx0.size() );	//check if fftw needs to be built/resized



			float fft_gain = 1;
			if( pref_zero_padding_for_fft ) fft_gain *= pref_zero_padding_for_fft_cnt+1;

			fftw_fwd_clpx_cplx( en_fia_plot_gph1, vcpx0, vcplex_sp, fft_gain );		//go to freq domain using an fft




//			int b_destroy_only = 0;
//			fftw_build_if_req( en_fia_plot_gph1, b_destroy_only, "1: fftw fwd cplx to cplx: for plot gph1 'en_gls_IQ_spect_downcnv_filter'", en_fpt_fwd_cplx_to_cplx, vcpx0.size() );	//check if fftw needs to be built/resized


//			float fft_gain = 1;
//			fftw_fwd_clpx_cplx( en_fia_plot_gph1, vcpx0, vcplex_sp, fft_gain );		//go to freq domain using an fft


//			int fftsz = vcpx0.size();
//			double bw = downsample_srate;
//			double bw_bin = bw / (double)fftsz;


//			complex_fwd_fft_multi( vcpx0, vcplex_sp, "for 'en_gls_IQ_spect_downcnv_filter' plot", en_ftid_gui );			//go to freq domain using an fft
			vector<st_spect_tag> vsp;
			complex_fft_displayable( downsample_srate, g_freq_tune, vcplex_sp, vsp, 1, 1 );

//printf( " plot_load_vectors_eval() - vcpx0.size() %d\n", vcpx0.size() );
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();


			int siz = vsp.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
//				int ifreq = g_freq_tune_offset - siz*bw_bin/2 + bw_bin + i;
//				int ifreq = -siz*(bw_bin/2) + bw_bin * i;

				vgph1_x.push_back( vsp[i].freq );
				vgph1_y0.push_back( vsp[i].ampl );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 0 - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------




//---------------- graph plot monitoring point -------------------------
if( which == en_gls_demod_aud_timedomain )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
//printf( " plot_load_vectors_eval() - en_gls_timedomain_demod vdble0.size() %d\n", vdble0.size() );

			float dt = 1.0f / downsample_srate;
			
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

			int siz = vflt0.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
	//				int ifreq = g_freq_tune_offset - siz*bw_bin/2 + bw_bin + i;

				vgph1_x.push_back( i * dt );
				vgph1_y0.push_back( (vflt0)[i] );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 10;// - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------




//---------------- graph plot monitoring point -------------------------
if( which == en_gls_demod_aud_dcblock_timedomain )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
//printf( " plot_load_vectors_eval() - en_gls_timedomain_demod vdble0.size() %d\n", vdble0.size() );


			float dt = 1.0f / downsample_srate;
			
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

			int siz = vflt0.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
	//				int ifreq = g_freq_tune_offset - siz*bw_bin/2 + bw_bin + i;

				vgph1_x.push_back( i * dt );
				vgph1_y0.push_back( (vflt0)[i] );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 10;// - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------





//---------------- graph plot monitoring point -------------------------
if( which == en_gls_demod_aud_spect )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
			double bw = downsample_srate;
			if( srate_in != 0 ) bw = srate_in;							//caller specified srate? 

//			plot_fftwr_siz = vflt0.size();	


			taper_zeropad_real_1( pref_taper_windowing_for_fft, pref_zero_padding_for_fft, pref_zero_padding_for_fft_cnt, vflt0 );


			int b_destroy_only = 0;
			fftw_build_if_req( en_fia_plot_gph1, b_destroy_only, "1: fftw fwd real to cplx: for plot gph1 'en_gls_demod_spect'", en_fpt_fwd_real_to_cplx, vflt0.size() );	//check if fftw needs to be built/resized


			float fft_gain = 1;
			if( pref_zero_padding_for_fft ) fft_gain *= pref_zero_padding_for_fft_cnt+1;

			fftw_fwd_real_cplx( en_fia_plot_gph1, vflt0, vcplex_sp, fft_gain );		//go to freq domain using an fft


			vector<st_spect_tag> vsp;
			complex_fft_displayable( downsample_srate, g_freq_tune, vcplex_sp, vsp, 1, 1 );
/*
			for( int i = 0; i < vcplex_sp.size(); i++ )
				{
				st_spect_tag o;
				double d1 = (double)vcplex_sp[ i ].real;
				double d2 = (double)vcplex_sp[ i ].imag;

				o.freq = i;
				o.ampl = d1;//sqrt( d1 * d1 + d2 * d2  );
				vsp.push_back( o );			
				}
*/

//printf( " plot_load_vectors_eval() - en_gls_demod_spect vsp.size()  %d %d\n", vcplex_sp.size(), vsp.size() );

//printf( " plot_load_vectors_eval() - en_gls_demod_spect vdble.size() %d\n", vdble.size() );
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();


			int siz = vsp.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			
			
			for( int i = 0; i < siz; i++ )
				{

				vgph1_x.push_back( vsp[i].freq );
				vgph1_y0.push_back( vsp[i].ampl );
				
//				j++;
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 0 - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------




//---------------- graph plot monitoring point -------------------------
if( which == en_gls_demod_agc_audio_peak_timedomain )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
//printf( " plot_load_vectors_eval() - en_gls_timedomain_demod vdble.size() %d\n", vdble.size() );


			float dt = 1.0f / downsample_srate;
			
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

			int siz = vflt0.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
	//				int ifreq = g_freq_tune_offset - siz*bw_bin/2 + bw_bin + i;

				vgph1_x.push_back( i * dt );
				vgph1_y0.push_back( (vflt0)[i] );
//				vgph1_y1.push_back( vfloat1[i] );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 10;// - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------





//---------------- graph plot monitoring point -------------------------
if( which == en_gls_demod_agc_gain_ctrl_timedomain )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
//printf( " plot_load_vectors_eval() - en_gls_timedomain_demod vdble.size() %d\n", vdble.size() );


			float dt = 1.0f / downsample_srate;
			
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

			int siz = vflt0.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
	//				int ifreq = g_freq_tune_offset - siz*bw_bin/2 + bw_bin + i;

				vgph1_x.push_back( i * dt );
				vgph1_y0.push_back( (vflt0)[i] );
//				vgph1_y1.push_back( vfloat1[i] );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 10;// - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------




/*
//---------------- graph plot monitoring point -------------------------
if( which == en_gls_demod_aud_resampler_ch0_timedomain )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
//printf( " plot_load_vectors_eval() - en_gls_timedomain_demod vdble.size() %d\n", vdble.size() );


			float dt = 1.0f / downsample_srate;
			
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

			int siz = vflt0.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
	//				int ifreq = g_freq_tune_offset - siz*bw_bin/2 + bw_bin + i;

				vgph1_x.push_back( i * dt );
				vgph1_y0.push_back( (vflt0)[i] );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 10;// - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------
*/





//---------------- graph plot monitoring point -------------------------
if( which == en_gls_fm_demod_spect_mpx )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
			double bw = downsample_srate;
			if( srate_in != 0 ) bw = srate_in;							//caller specified srate? 

//			plot_fftwr_siz = vflt0.size();	


			taper_zeropad_real_1( pref_taper_windowing_for_fft, pref_zero_padding_for_fft, pref_zero_padding_for_fft_cnt, vflt0 );


			int b_destroy_only = 0;
			fftw_build_if_req( en_fia_plot_gph1, b_destroy_only, "1: fftw fwd real to cplx: for plot gph1 'en_gls_demod_fm_spect_mpx'", en_fpt_fwd_real_to_cplx, vflt0.size() );	//check if fftw needs to be built/resized


			float fft_gain = 1;
			if( pref_zero_padding_for_fft ) fft_gain *= pref_zero_padding_for_fft_cnt+1;

			fftw_fwd_real_cplx( en_fia_plot_gph1, vflt0, vcplex_sp, fft_gain );		//go to freq domain using an fft






			vector<st_spect_tag> vsp;

			complex_fft_displayable( downsample_srate, g_freq_tune, vcplex_sp, vsp, 1, 1 );
//printf("vsp.size() %d\n",vsp.size() );			

//printf( " plot_load_vectors_eval() - en_gls_spect_probe_*vflt0  vdble.size() %d\n", vdble.size() );
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();


			int siz = vsp.size();										//'real to complex' fftw returns half as many fft o/p samples
			gph_samples_per_sec = siz / frame_time;			//for display purposes
			for( int i = 0; i < siz; i++ )
				{

				vgph1_x.push_back( vsp[i].freq );
				vgph1_y0.push_back( vsp[i].ampl );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 0 - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------




//---------------- graph plot monitoring point -------------------------
if( which == en_gls_fm_demod_mpx_halfband_decimator_spect )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
			double bw = downsample_srate;
			if( srate_in != 0 ) bw = srate_in;							//caller specified srate? 

			taper_zeropad_real_1( pref_taper_windowing_for_fft, pref_zero_padding_for_fft, pref_zero_padding_for_fft_cnt, vflt0 );

			int b_destroy_only = 0;
			fftw_build_if_req( en_fia_plot_gph1, b_destroy_only, "1: fftw fwd real to cplx: for plot gph1 'en_gls_demod_fm_spect_mpx_halfband_decimator'", en_fpt_fwd_real_to_cplx, vflt0.size() );	//check if fftw needs to be built/resized


			float fft_gain = 1;
			if( pref_zero_padding_for_fft ) fft_gain *= pref_zero_padding_for_fft_cnt+1;

			fftw_fwd_real_cplx( en_fia_plot_gph1, vflt0, vcplex_sp, fft_gain );		//go to freq domain using an fft


			vector<st_spect_tag> vsp;
			complex_fft_displayable( downsample_srate/2, g_freq_tune, vcplex_sp, vsp, 1, 1 );

//printf( " plot_load_vectors_eval() - en_gls_spect_probe_*vflt0  vdble.size() %d\n", vdble.size() );
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();


			int siz = vsp.size();										//'real to complex' fftw returns half as many fft o/p samples
			gph_samples_per_sec = siz / frame_time;			//for display purposes
			for( int i = 0; i < siz; i++ )
				{

				vgph1_x.push_back( vsp[i].freq );
				vgph1_y0.push_back( vsp[i].ampl );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 0 - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------




//---------------- graph plot monitoring point -------------------------
if( which == en_gls_fm_demod_pll_osc_compare )
	{
//printf("en_gls_tdm_probe_*vflt0 here  *vflt0.size() %d\n", *vflt0.size() );
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
//printf("en_gls_tdm_probe_*vflt0 here  which %d,  gph_loc_sel_tmp %d\n", which, gph_loc_sel_tmp);
//printf( " plot_load_vectors_eval() - en_gls_timedomain_demod *vflt0.size() %d\n", *vflt0.size() );

			double bw = downsample_srate;
			if( srate_in != 0 ) bw = srate_in;							//caller specified srate? 


			float dt = 1.0f / srate_in;
			
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

			int siz = vflt0.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
	//				int ifreq = g_freq_tune_offset - siz*bw_bin/2 + bw_bin + i;
				vgph1_x.push_back( dt * i );
				vgph1_y0.push_back( (vflt0)[i] );
				vgph1_y1.push_back( (vflt1)[i] );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 10;// - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 2;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------




//---------------- graph plot monitoring point -------------------------
if( which == en_gls_fm_demod_pilot_pll_err )
	{
//printf("en_gls_tdm_probe_*vflt0 here  *vflt0.size() %d\n", *vflt0.size() );
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
//printf("en_gls_tdm_probe_*vflt0 here  which %d,  gph_loc_sel_tmp %d\n", which, gph_loc_sel_tmp);
//printf( " plot_load_vectors_eval() - en_gls_timedomain_demod *vflt0.size() %d\n", *vflt0.size() );


			float dt = 1.0f / downsample_srate;
			
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

			int siz = vflt0.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
	//				int ifreq = g_freq_tune_offset - siz*bw_bin/2 + bw_bin + i;
				vgph1_x.push_back( i  );
				vgph1_y0.push_back( (vflt0)[i] );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 10;// - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------



//---------------- graph plot monitoring point -------------------------
if( which == en_gls_fm_demod_pilot_rectified_peak )
	{
//printf("en_gls_tdm_probe_*vflt0 here  *vflt0.size() %d\n", *vflt0.size() );
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
//printf("en_gls_tdm_probe_*vflt0 here  which %d,  gph_loc_sel_tmp %d\n", which, gph_loc_sel_tmp);
//printf( " plot_load_vectors_eval() - en_gls_timedomain_demod *vflt0.size() %d\n", *vflt0.size() );


			float dt = 1.0f / downsample_srate;
			
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

			int siz = vflt0.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
	//				int ifreq = g_freq_tune_offset - siz*bw_bin/2 + bw_bin + i;
				vgph1_x.push_back( i  );
				vgph1_y0.push_back( (vflt0)[i] );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 10;// - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------





//---------------- graph plot monitoring point -------------------------
if( which == en_gls_fm_demod_pilot_bounce )
	{
//printf("en_gls_tdm_probe_*vflt0 here  *vflt0.size() %d\n", *vflt0.size() );
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
//printf("en_gls_tdm_probe_*vflt0 here  which %d,  gph_loc_sel_tmp %d\n", which, gph_loc_sel_tmp);
//printf( " plot_load_vectors_eval() - en_gls_timedomain_demod *vflt0.size() %d\n", *vflt0.size() );


			float dt = 1.0f / downsample_srate;
			
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

			int siz = vflt0.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
	//				int ifreq = g_freq_tune_offset - siz*bw_bin/2 + bw_bin + i;
				vgph1_x.push_back( i  );
				vgph1_y0.push_back( (vflt0)[i] );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 10;// - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------






//---------------- graph plot monitoring point -------------------------
if( which == en_gls_fm_demod_pll_38K_osc )
	{
//printf("en_gls_tdm_probe_*vflt0 here  *vflt0.size() %d\n", *vflt0.size() );
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
			double bw = downsample_srate;
			if( srate_in != 0 ) bw = srate_in;							//caller specified srate? 

			taper_zeropad_real_1( pref_taper_windowing_for_fft, pref_zero_padding_for_fft, pref_zero_padding_for_fft_cnt, vflt0 );

			int b_destroy_only = 0;
			fftw_build_if_req( en_fia_plot_gph1, b_destroy_only, "1: fftw fwd real to cplx: for plot gph1 'en_gls_demod_fm_spect_mpx_halfband_decimator'", en_fpt_fwd_real_to_cplx, vflt0.size() );	//check if fftw needs to be built/resized


			float fft_gain = 1;
			if( pref_zero_padding_for_fft ) fft_gain *= pref_zero_padding_for_fft_cnt+1;

			fftw_fwd_real_cplx( en_fia_plot_gph1, vflt0, vcplex_sp, fft_gain );		//go to freq domain using an fft


			vector<st_spect_tag> vsp;
			complex_fft_displayable( downsample_srate/2, g_freq_tune, vcplex_sp, vsp, 1, 1 );

//printf( " plot_load_vectors_eval() - en_gls_spect_probe_*vflt0  vdble.size() %d\n", vdble.size() );
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();


			int siz = vsp.size();										//'real to complex' fftw returns half as many fft o/p samples
			gph_samples_per_sec = siz / frame_time;			//for display purposes
			for( int i = 0; i < siz; i++ )
				{

				vgph1_x.push_back( vsp[i].freq );
				vgph1_y0.push_back( vsp[i].ampl );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 0 - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------






//---------------- graph plot monitoring point -------------------------
if( which == en_gls_fm_demod_left_minus_right_bpf_spect )
	{
//printf("en_gls_tdm_probe_*vflt0 here  *vflt0.size() %d\n", *vflt0.size() );
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
			double bw = downsample_srate;
			if( srate_in != 0 ) bw = srate_in;							//caller specified srate? 

			taper_zeropad_real_1( pref_taper_windowing_for_fft, pref_zero_padding_for_fft, pref_zero_padding_for_fft_cnt, vflt0 );

			int b_destroy_only = 0;
			fftw_build_if_req( en_fia_plot_gph1, b_destroy_only, "1: fftw fwd real to cplx: for plot gph1 'en_gls_demod_fm_spect_mpx_halfband_decimator'", en_fpt_fwd_real_to_cplx, vflt0.size() );	//check if fftw needs to be built/resized


			float fft_gain = 1;
			if( pref_zero_padding_for_fft ) fft_gain *= pref_zero_padding_for_fft_cnt+1;

			fftw_fwd_real_cplx( en_fia_plot_gph1, vflt0, vcplex_sp, fft_gain );		//go to freq domain using an fft


			vector<st_spect_tag> vsp;
			complex_fft_displayable( downsample_srate/2, g_freq_tune, vcplex_sp, vsp, 1, 1 );

//printf( " plot_load_vectors_eval() - en_gls_spect_probe_*vflt0  vdble.size() %d\n", vdble.size() );
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();


			int siz = vsp.size();										//'real to complex' fftw returns half as many fft o/p samples
			gph_samples_per_sec = siz / frame_time;			//for display purposes
			for( int i = 0; i < siz; i++ )
				{

				vgph1_x.push_back( vsp[i].freq );
				vgph1_y0.push_back( vsp[i].ampl );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 0 - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------








//---------------- graph plot monitoring point -------------------------
if( which == en_gls_fm_demod_left_minus_right_timedomain )
	{
//printf("en_gls_tdm_probe_*vflt0 here  *vflt0.size() %d\n", *vflt0.size() );
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
//printf("en_gls_tdm_probe_*vflt0 here  which %d,  gph_loc_sel_tmp %d\n", which, gph_loc_sel_tmp);
//printf( " plot_load_vectors_eval() - en_gls_timedomain_demod *vflt0.size() %d\n", *vflt0.size() );


			float dt = 1.0f / downsample_srate;
			
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

			int siz = vflt0.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
	//				int ifreq = g_freq_tune_offset - siz*bw_bin/2 + bw_bin + i;
				vgph1_x.push_back( i  );
				vgph1_y0.push_back( (vflt0)[i] );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 10;// - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------




//---------------- graph plot monitoring point -------------------------
if( which == en_gls_fm_demod_left_minus_right_spect )
	{
//printf("en_gls_tdm_probe_*vflt0 here  *vflt0.size() %d\n", *vflt0.size() );
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
			double bw = downsample_srate;
			if( srate_in != 0 ) bw = srate_in;							//caller specified srate? 

			taper_zeropad_real_1( pref_taper_windowing_for_fft, pref_zero_padding_for_fft, pref_zero_padding_for_fft_cnt, vflt0 );

			int b_destroy_only = 0;
			fftw_build_if_req( en_fia_plot_gph1, b_destroy_only, "1: fftw fwd real to cplx: for plot gph1 'en_gls_demod_fm_spect_mpx_halfband_decimator'", en_fpt_fwd_real_to_cplx, vflt0.size() );	//check if fftw needs to be built/resized


			float fft_gain = 1;
			if( pref_zero_padding_for_fft ) fft_gain *= pref_zero_padding_for_fft_cnt+1;

			fftw_fwd_real_cplx( en_fia_plot_gph1, vflt0, vcplex_sp, fft_gain );		//go to freq domain using an fft


			vector<st_spect_tag> vsp;
			complex_fft_displayable( downsample_srate/2, g_freq_tune, vcplex_sp, vsp, 1, 1 );

//printf( " plot_load_vectors_eval() - en_gls_spect_probe_*vflt0  vdble.size() %d\n", vdble.size() );
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();


			int siz = vsp.size();										//'real to complex' fftw returns half as many fft o/p samples
			gph_samples_per_sec = siz / frame_time;			//for display purposes
			for( int i = 0; i < siz; i++ )
				{

				vgph1_x.push_back( vsp[i].freq );
				vgph1_y0.push_back( vsp[i].ampl );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 0 - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------



//---------------- graph plot monitoring point -------------------------
if( which == en_gls_fm_ster_demux_aud_ch0_timedomain )
	{
//printf("en_gls_timedomain_fm_ster_demux_aud_ch0  vflt0 here  vflt0.size() %d\n", vflt0.size() );
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
//printf("en_gls_tdm_probe_*vflt0 here  which %d,  gph_loc_sel_tmp %d\n", which, gph_loc_sel_tmp);
//printf( " plot_load_vectors_eval() - en_gls_timedomain_demod *vflt0.size() %d\n", *vflt0.size() );


			float dt = 1.0f / downsample_srate;
			
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

			int siz = vflt0.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
	//				int ifreq = g_freq_tune_offset - siz*bw_bin/2 + bw_bin + i;
				vgph1_x.push_back( i  );
				vgph1_y0.push_back( (vflt0)[i] );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 10;// - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------






//---------------- graph plot monitoring point -------------------------
if( which == en_gls_fm_ster_demux_aud_ch1_timedomain )
	{
//printf("en_gls_tdm_probe_*vflt0 here  vflt0.size() %d\n", vflt0.size() );
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
//printf("en_gls_tdm_probe_*vflt0 here  which %d,  gph_loc_sel_tmp %d\n", which, gph_loc_sel_tmp);
//printf( " plot_load_vectors_eval() - en_gls_timedomain_demod *vflt0.size() %d\n", *vflt0.size() );


			float dt = 1.0f / downsample_srate;
			
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

			int siz = vflt0.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
	//				int ifreq = g_freq_tune_offset - siz*bw_bin/2 + bw_bin + i;
				vgph1_x.push_back( i  );
				vgph1_y0.push_back( (vflt0)[i] );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 10;// - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------





//---------------- graph plot monitoring point -------------------------
if( which == en_gls_fm_ster_demux_aud_ch0_spect )
	{
//printf("en_gls_spect_fm_ster_demux_aud_ch0   vflt0.size() %d\n", vflt0.size() );
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
			double bw = downsample_srate;
			if( srate_in != 0 ) bw = srate_in;							//caller specified srate? 

			taper_zeropad_real_1( pref_taper_windowing_for_fft, pref_zero_padding_for_fft, pref_zero_padding_for_fft_cnt, vflt0 );

			int b_destroy_only = 0;
			fftw_build_if_req( en_fia_plot_gph1, b_destroy_only, "1: fftw fwd real to cplx: for plot gph1 'en_gls_spect_fm_ster_demux_aud_ch0'", en_fpt_fwd_real_to_cplx, vflt0.size() );	//check if fftw needs to be built/resized


			float fft_gain = 1;
			if( pref_zero_padding_for_fft ) fft_gain *= pref_zero_padding_for_fft_cnt+1;

			fftw_fwd_real_cplx( en_fia_plot_gph1, vflt0, vcplex_sp, fft_gain );		//go to freq domain using an fft



//			vector<st_spect_tag> vsp;
//			complex_fft_displayable( downsample_srate/2, g_freq_tune, vcplex_sp, vsp, 1, 1 );

			vector<st_spect_tag> vsp;
			complex_fft_displayable( bw, g_freq_tune, vcplex_sp, vsp, 1, 1 );

//printf( " plot_load_vectors_eval() - en_gls_spect_probe_*vflt0  vdble.size() %d\n", vdble.size() );
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();


			int siz = vsp.size();										//'real to complex' fftw returns half as many fft o/p samples
			gph_samples_per_sec = siz / frame_time;			//for display purposes

			for( int i = 0; i < siz; i++ )
				{

				vgph1_x.push_back( vsp[i].freq );
				vgph1_y0.push_back( vsp[i].ampl );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 0 - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------










//---------------- graph plot monitoring point -------------------------
if( which == en_gls_fm_ster_demux_aud_ch1_spect )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
			double bw = downsample_srate;
			if( srate_in != 0 ) bw = srate_in;							//caller specified srate? 

			taper_zeropad_real_1( pref_taper_windowing_for_fft, pref_zero_padding_for_fft, pref_zero_padding_for_fft_cnt, vflt0 );

			int b_destroy_only = 0;
			fftw_build_if_req( en_fia_plot_gph1, b_destroy_only, "1: fftw fwd real to cplx: for plot gph1 'en_gls_spect_fm_ster_demux_aud_ch1'", en_fpt_fwd_real_to_cplx, vflt0.size() );	//check if fftw needs to be built/resized


			float fft_gain = 1;
			if( pref_zero_padding_for_fft ) fft_gain *= pref_zero_padding_for_fft_cnt+1;

			fftw_fwd_real_cplx( en_fia_plot_gph1, vflt0, vcplex_sp, fft_gain );		//go to freq domain using an fft



//			vector<st_spect_tag> vsp;
//			complex_fft_displayable( downsample_srate/2, vcplex_sp, vsp, 1, 1 );

			vector<st_spect_tag> vsp;
			complex_fft_displayable( bw, g_freq_tune, vcplex_sp, vsp, 1, 1 );

//printf( " plot_load_vectors_eval() - en_gls_spect_probe_*vflt0  vdble.size() %d\n", vdble.size() );
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();


			int siz = vsp.size();										//'real to complex' fftw returns half as many fft o/p samples
			gph_samples_per_sec = siz / frame_time;					//for display purposes
			for( int i = 0; i < siz; i++ )
				{

				vgph1_x.push_back( vsp[i].freq );
				vgph1_y0.push_back( vsp[i].ampl );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 0 - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------





//---------------- graph plot monitoring point -------------------------
if( which == en_gls_demod_aud_resampler_ch0_timedomain )
	{
//printf("en_gls_tdm_probe_*vflt0 here  *vflt0.size() %d\n", *vflt0.size() );
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
//printf("en_gls_tdm_probe_*vflt0 here  which %d,  gph_loc_sel_tmp %d\n", which, gph_loc_sel_tmp);
//printf( " plot_load_vectors_eval() - en_gls_timedomain_demod *vflt0.size() %d\n", *vflt0.size() );

			double bw = aud_op_srate;
			if( srate_in != 0 ) bw = srate_in;							//caller specified srate? 

			float dt = 1.0f / bw;
			
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

			int siz = vflt0.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
	//				int ifreq = g_freq_tune_offset - siz*bw_bin/2 + bw_bin + i;
				vgph1_x.push_back( i*dt  );
				vgph1_y0.push_back( (vflt0)[i] );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 10;// - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------






//---------------- graph plot monitoring point -------------------------
if( which == en_gls_demod_aud_resampler_ch1_timedomain )
	{
//printf("en_gls_tdm_probe_*vflt0 here  *vflt0.size() %d\n", *vflt0.size() );
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
//printf("en_gls_tdm_probe_*vflt0 here  which %d,  gph_loc_sel_tmp %d\n", which, gph_loc_sel_tmp);
//printf( " plot_load_vectors_eval() - en_gls_timedomain_demod *vflt0.size() %d\n", *vflt0.size() );


			double bw = aud_op_srate;
			if( srate_in != 0 ) bw = srate_in;							//caller specified srate? 

			float dt = 1.0f / bw;
			
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

			int siz = vflt0.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
	//				int ifreq = g_freq_tune_offset - siz*bw_bin/2 + bw_bin + i;
				vgph1_x.push_back( i*dt );
				vgph1_y0.push_back( (vflt0)[i] );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 10;// - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------






//---------------- graph plot monitoring point -------------------------
if( which == en_gls_demod_aud_resampler_ch0_spect )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
			double bw = aud_op_srate;
			if( srate_in != 0 ) bw = srate_in;							//caller specified srate? 

			taper_zeropad_real_1( pref_taper_windowing_for_fft, pref_zero_padding_for_fft, pref_zero_padding_for_fft_cnt, vflt0 );

			int b_destroy_only = 0;
			fftw_build_if_req( en_fia_plot_gph1, b_destroy_only, "1: fftw fwd real to cplx: for plot gph1 'en_gls_spect_fm_ster_demux_aud_ch1'", en_fpt_fwd_real_to_cplx, vflt0.size() );	//check if fftw needs to be built/resized


			float fft_gain = 1;
			if( pref_zero_padding_for_fft ) fft_gain *= pref_zero_padding_for_fft_cnt+1;

			fftw_fwd_real_cplx( en_fia_plot_gph1, vflt0, vcplex_sp, fft_gain );		//go to freq domain using an fft



//			vector<st_spect_tag> vsp;
//			complex_fft_displayable( downsample_srate/2, vcplex_sp, vsp, 1, 1 );

			vector<st_spect_tag> vsp;
			complex_fft_displayable( bw, g_freq_tune, vcplex_sp, vsp, 1, 1 );

//printf( " plot_load_vectors_eval() - en_gls_spect_probe_*vflt0  vdble.size() %d\n", vdble.size() );
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();


			int siz = vsp.size();										//'real to complex' fftw returns half as many fft o/p samples
			gph_samples_per_sec = siz / frame_time;					//for display purposes
			for( int i = 0; i < siz; i++ )
				{

				vgph1_x.push_back( vsp[i].freq );
				vgph1_y0.push_back( vsp[i].ampl );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 0 - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------








//---------------- graph plot monitoring point -------------------------
if( which == en_gls_demod_aud_resampler_ch1_spect )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
			double bw = aud_op_srate;
			if( srate_in != 0 ) bw = srate_in;							//caller specified srate? 

			taper_zeropad_real_1( pref_taper_windowing_for_fft, pref_zero_padding_for_fft, pref_zero_padding_for_fft_cnt, vflt0 );

			int b_destroy_only = 0;
			fftw_build_if_req( en_fia_plot_gph1, b_destroy_only, "1: fftw fwd real to cplx: for plot gph1 'en_gls_spect_fm_ster_demux_aud_ch1'", en_fpt_fwd_real_to_cplx, vflt0.size() );	//check if fftw needs to be built/resized


			float fft_gain = 1;
			if( pref_zero_padding_for_fft ) fft_gain *= pref_zero_padding_for_fft_cnt+1;

			fftw_fwd_real_cplx( en_fia_plot_gph1, vflt0, vcplex_sp, fft_gain );		//go to freq domain using an fft



//			vector<st_spect_tag> vsp;
//			complex_fft_displayable( downsample_srate/2, vcplex_sp, vsp, 1, 1 );

			vector<st_spect_tag> vsp;
			complex_fft_displayable( bw, g_freq_tune, vcplex_sp, vsp, 1, 1 );

//printf( " plot_load_vectors_eval() - en_gls_spect_probe_*vflt0  vdble.size() %d\n", vdble.size() );
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();


			int siz = vsp.size();										//'real to complex' fftw returns half as many fft o/p samples
			gph_samples_per_sec = siz / frame_time;					//for display purposes
			for( int i = 0; i < siz; i++ )
				{

				vgph1_x.push_back( vsp[i].freq );
				vgph1_y0.push_back( vsp[i].ampl );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 0 - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------









//---------------- graph plot monitoring point -------------------------
if( which == en_gls_tdm_probe_vflt0 )
	{
//printf("en_gls_tdm_probe_*vflt0 here  *vflt0.size() %d\n", *vflt0.size() );
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
//printf("en_gls_tdm_probe_*vflt0 here  which %d,  gph_loc_sel_tmp %d\n", which, gph_loc_sel_tmp);
//printf( " plot_load_vectors_eval() - en_gls_timedomain_demod *vflt0.size() %d\n", *vflt0.size() );


			double bw = downsample_srate;
			if( srate_in != 0 ) bw = srate_in;							//caller specified srate? 

			float dt = 1.0f / bw;
			
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

			int siz = vflt0.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
	//				int ifreq = g_freq_tune_offset - siz*bw_bin/2 + bw_bin + i;
				vgph1_x.push_back( i*dt  );
				vgph1_y0.push_back( (vflt0)[i] );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 10;// - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------



//---------------- graph plot monitoring point -------------------------
if( which == en_gls_spect_probe_vflt0 )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
			double bw = downsample_srate;
			if( srate_in != 0 ) bw = srate_in;							//caller specified srate? 

			taper_zeropad_real_1( pref_taper_windowing_for_fft, pref_zero_padding_for_fft, pref_zero_padding_for_fft_cnt, vflt0 );

			int b_destroy_only = 0;
			fftw_build_if_req( en_fia_plot_gph1, b_destroy_only, "1: fftw fwd real to cplx: for plot gph1 'en_gls_spect_probe_vflt0'", en_fpt_fwd_real_to_cplx, vflt0.size() );	//check if fftw needs to be built/resized


			float fft_gain = 1;
			if( pref_zero_padding_for_fft ) fft_gain *= pref_zero_padding_for_fft_cnt+1;

			fftw_fwd_real_cplx( en_fia_plot_gph1, vflt0, vcplex_sp, fft_gain );		//go to freq domain using an fft



//			vector<st_spect_tag> vsp;
//			complex_fft_displayable( downsample_srate/2, g_freq_tune, vcplex_sp, vsp, 1, 1 );

			vector<st_spect_tag> vsp;
			complex_fft_displayable( bw, g_freq_tune, vcplex_sp, vsp, 1, 1 );

//printf( " plot_load_vectors_eval() - en_gls_spect_probe_*vflt0  vdble.size() %d\n", vdble.size() );
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();


			int siz = vsp.size();										//'real to complex' fftw returns half as many fft o/p samples
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{

				vgph1_x.push_back( vsp[i].freq );
				vgph1_y0.push_back( vsp[i].ampl );
				}


			gph1_b_x_axis_values_derived = 0;
//			gph1_x_axis_values_derived_left_value = 0 - siz*bw_bin/2;
//			gph1_x_axis_values_derived_inc_value = bw_bin;

	//			plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------





//---------------- graph plot monitoring point -------------------------
if( which == en_gls_spect_probe_vcpx0 )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
			double bw = g_dev_bw;
			if( srate_in != 0 ) bw = srate_in;							//caller specified srate? 

			taper_zeropad_complex_1( pref_taper_windowing_for_fft, pref_zero_padding_for_fft, pref_zero_padding_for_fft_cnt, vcpx0 );


			int b_destroy_only = 0;
			fftw_build_if_req( en_fia_plot_gph1, b_destroy_only, "1: fftw fwd cplx to cplx: for plot gph1 'en_gls_spect_probe_vcpx0'", en_fpt_fwd_cplx_to_cplx, vcpx0.size() );	//check if fftw needs to be built/resized


			float fft_gain = 1;
			if( pref_zero_padding_for_fft ) fft_gain *= pref_zero_padding_for_fft_cnt+1;

			fftw_fwd_clpx_cplx( en_fia_plot_gph1, vcpx0, vcplex_sp, fft_gain );		//go to freq domain using an fft

			vector<st_spect_tag> vsp;
			complex_fft_displayable( bw, g_freq_tune, vcplex_sp, vsp, 1, 1 );

			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

//printf( " plot_load_vectors_eval() - fftsz %d, bw_bin %f\n", fftsz, bw_bin );

			int siz = vsp.size();
			gph_samples_per_sec = siz / frame_time;						//for display purposes
			for( int i = 0; i < siz; i++ )
				{
				vgph1_x.push_back(  vsp[i].freq );
				vgph1_y0.push_back( vsp[i].ampl );
				}


			gph1_b_x_axis_values_derived = 0;

	//		plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------



//---------------- graph plot monitoring point -------------------------
if( which == en_gls_filtresp_antialias_iir )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

			vector<double> vnumer;
			vector<double> vdenom;
			float fsrate = g_dev_bw;
			float ampl_out;



//float b0 = elliplp_cr.sec[0].b0;

/*
			for( int i = 0; i < 5; i++ )
				{
				vnumer.push_back( elliplp_cr.sec[i].b0 );
				vnumer.push_back( elliplp_cr.sec[i].b1 );
				vnumer.push_back( elliplp_cr.sec[i].b2 );

				vdenom.push_back( 1.0 );
				vdenom.push_back( elliplp_cr.sec[i].a1 );
				vdenom.push_back( elliplp_cr.sec[i].a2 );
				}
*/


vnumer = elliplpf_cr.st0.vnumer;
vdenom = elliplpf_cr.st0.vdenom;

			int cnt = 512;
			
			float freq_start = 1000;
			float freq_stop = elliplpf_cr.st0.fc0*1.125f;
			float freq = freq_start;
			
			float freq_step = (freq_stop - freq_start) / (float)cnt;
			
			for( int i = 0; i < cnt; i++ )
				{
				if( dsp_utils_code::freqz_sos_inline_single_freq( vnumer, vdenom, fsrate, freq, ampl_out ) )
					{
					vgph1_x.push_back( freq );
					vgph1_y0.push_back( ampl_out );
					}
				
				freq += freq_step;
				}

//printf( " plot_load_vectors_eval() - en_gls_filtresp_aa   vgph1_x.size() %d\n", vgph1_x.size() );
				
/*

			vgph1_x.push_back( 0.0f );
			vgph1_y0.push_back( 0.0f );

			vgph1_x.push_back( 1.0f );
			vgph1_y0.push_back( 1.0f );

			vgph1_x.push_back( 2.0f );
			vgph1_y0.push_back( 1.0f );

			vgph1_x.push_back( 3.0f );
			vgph1_y0.push_back( -1.0f );
			
			vgph1_x.push_back( 4.0f );
			vgph1_y0.push_back( -1.0f );
*/


			gph1_b_x_axis_values_derived = 0;


	//		plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------




//---------------- graph plot monitoring point -------------------------
if( which == en_gls_filtresp_demod_lpf0_iir )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

			vector<double> vnumer;
			vector<double> vdenom;
			float fsrate = downsample_srate;
			float ampl_out;

			printf( " plot_load_vectors_eval() - en_gls_filtresp_demod_lpf0_iir  \n" );


			vnumer.push_back( usr_lpf_iir0_I0.b0 );
			vnumer.push_back( usr_lpf_iir0_I0.b1 );
			vnumer.push_back( usr_lpf_iir0_I0.b2 );

			vdenom.push_back( 1.0f );
			vdenom.push_back( usr_lpf_iir0_I0.a1 );
			vdenom.push_back( usr_lpf_iir0_I0.a2 );

			int cnt = 512;
			
			float freq_start = 10;
			float freq_stop = downsample_srate / 2;//usr_hpf_iir0_I0.fc * 10.0f;
			float freq = freq_start;
			
			float freq_step = (freq_stop - freq_start) / (float)cnt;
			
			for( int i = 0; i < cnt; i++ )
				{
				if( dsp_utils_code::freqz_sos_inline_single_freq( vnumer, vdenom, fsrate, freq, ampl_out ) )
					{
					vgph1_x.push_back( freq );
					vgph1_y0.push_back( ampl_out );
					}
				
				freq += freq_step;
				}

			gph1_b_x_axis_values_derived = 0;


	//		plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------





//---------------- graph plot monitoring point -------------------------
if( which == en_gls_filtresp_demod_hpf_iir )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();

			vector<double> vnumer;
			vector<double> vdenom;
			float fsrate = downsample_srate;
			float ampl_out;

//			printf( " plot_load_vectors_eval() - en_gls_filtresp_demod_hpf_iir  \n" );


			vnumer.push_back( usr_hpf_iir0_I0.b0 );
			vnumer.push_back( usr_hpf_iir0_I0.b1 );
			vnumer.push_back( usr_hpf_iir0_I0.b2 );

			vdenom.push_back( 1.0f );
			vdenom.push_back( usr_hpf_iir0_I0.a1 );
			vdenom.push_back( usr_hpf_iir0_I0.a2 );

			int cnt = 512;
			
			float freq_start = 10;
			float freq_stop = downsample_srate / 2;//usr_hpf_iir0_I0.fc * 10.0f;
			float freq = freq_start;
			
			float freq_step = (freq_stop - freq_start) / (float)cnt;
			
			for( int i = 0; i < cnt; i++ )
				{
				if( dsp_utils_code::freqz_sos_inline_single_freq( vnumer, vdenom, fsrate, freq, ampl_out ) )
					{
					vgph1_x.push_back( freq );
					vgph1_y0.push_back( ampl_out );
					}
				
				freq += freq_step;
				}

			gph1_b_x_axis_values_derived = 0;


	//		plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------






string s1, st;
mystr m1;

//---------------- graph plot monitoring point -------------------------
if( which == en_gls_filtresp_demod_bpf_fir )
	{
	if( which == gph_loc_sel_tmp )
		{
		if( b_plot_gph1 == 0 )
			{
			vgph1_x.clear();
			vgph1_y0.clear();
			vgph1_y1.clear();


			vector<double> vcoeff;

			for( int i = 0; i < usr_demod_bpf_fir_I0.coeff_cnt; i++ )
				{
				vcoeff.push_back( usr_demod_bpf_fir_I0.coeff_ptr[i] );

//				if( bdbg_dump_coeff )
//					{
//					strpf( s1, "%f\n", usr_demod_bpf_fir_I0.coeff_ptr[i] );
//					st += s1;
//					printf("coeff[%d] %f\n", i, usr_demod_bpf_fir_I0.coeff_ptr[i] );
//					}
				}

//			if( bdbg_dump_coeff )
//					{
//					m1 = st;
//					m1.writefile( "zzz_coeff_dump.txt" );
//					}
//bdbg_dump_coeff = 0;


			//analyse an fir response
			unsigned int num_points = 512;
//			vector<double> vomega;
//			vector<double> vamp;
//			dsp_utils_code::freqz_fir( vcoeff, num_points, vomega, vamp );

			vector<double> vfreq_out;
			vector<double> vamp_out;

			dsp_utils_code::freqz_fir_between_freqs( vcoeff, num_points, downsample_srate, 10, downsample_srate/2.0f, vfreq_out, vamp_out );


//			for( int i = 0; i < vomega.size(); i++ )
//				{
//				vgph1_x.push_back( vomega[i] );
//				vgph1_y0.push_back( vamp[i] );
//				}


			for( int i = 0; i < vfreq_out.size(); i++ )
				{
//				vgph1_x.push_back( 2 * vomega[i] * pi * (downsample_srate/2) );
				vgph1_x.push_back( vfreq_out[i] );
				vgph1_y0.push_back( vamp_out[i] );

//printf( "vomega[%d]   %f\n", i, vomega[i] );
				}


/*

			vgph1_x.push_back( 0.0f );
			vgph1_y0.push_back( 0.0f );

			vgph1_x.push_back( 1.0f );
			vgph1_y0.push_back( 1.0f );

			vgph1_x.push_back( 2.0f );
			vgph1_y0.push_back( 1.0f );

			vgph1_x.push_back( 3.0f );
			vgph1_y0.push_back( -1.0f );
			
			vgph1_x.push_back( 4.0f );
			vgph1_y0.push_back( -1.0f );
*/


			gph1_b_x_axis_values_derived = 0;


	//		plot_gph_type = 0;
			plot_gph_trace_cnt = 1;
			b_plot_gph1 = 1;
			}
		}
	}
//----------------------------------------------------------------------








}

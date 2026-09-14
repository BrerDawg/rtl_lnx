/*
Copyright (C) 2026 BrerDawg, et. al.

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

//fm_demode_code.cpp

//v1.01		27-feb-2026		//some code is via chatgpt



#include "fm_demode_code.h"

extern int g_dbg0;
extern int g_dbg1;

extern int gph_samples_per_sec;

extern bool g_fm_19K_pilot_tone;

extern void plot_load_struct_audio_thrd( st_plot_probe_tag &st, void* vv0, void* vv1 );

//extern void plot_load_vectors_eval( en_graph_loc_sel_tag which, unsigned int srate_in, vector <filter_code::st_cplex_tag> *vcpx0, vector <filter_code::st_cplex_tag> *vcpx1, vector<st_spect_tag> *vsp0, vector<double> *vdble0, vector<float> *vfloat0, vector<float> *vflt1 );
//extern void plot_load_vectors( st_plot_probe_tag &st );
extern st_plot_probe_tag st_plot;

//extern cl_halfband_decimator fm_stereo_decimator;
extern iir_sos::st_iir_sos_tag fm_stereo_dwnsmpl_aa;

extern halfband_poly_optimised_float::st_hbpo_tag fm_stereo_decimator;
extern filter_code::st_fir fm_ster_fir_15k_lpf0;
extern filter_code::st_fir fm_ster_fir_15k_lpf1;
extern filter_code::st_fir fm_ster_fir_23_53k_bpf;
extern filter_code::st_iir fm_ster_iir_19k_notch;

extern st_filter_sdr_tag st_filt[cn_filters_max];

extern int downsample_srate;
extern int fm_decimator_srate;
extern int clip_audio_cnt;
extern int clip_audio_cnt_max;

int plot_srate;


float fm_delay[ cn_fm_stereo_bpf_taps+50 ];								//the delay code uses only roughly tap/2 samples, so half of this delay is not used
int fmd_wr = 0;


#define cn_deemphasis_tau_75us (75e-6f)									//USA
#define cn_deemphasis_tau_50us (50e-6f)

extern int i_deemphasis;


/*
//code obtained via chatgpt5
//'vf0' holds sine wfm samples to be measured, >100 sine cycles for accuracy are needed
//'f0' is the freq to be measured, make SURE that is this exact freq the 'vf0' samples were recorded with, ELSE magn will be INCORRECT
//'fs' is the samplerate
//needs more than >100 sine cycles to average out spurious values and focus in on the actual 'f0' to be measured
//returns magnitude of calc'd at freq 'f0'
float goertzel_mag_real_float( vector<float> &vf0, float fs, float f0 )
{
float w = 2.0f * M_PI * f0 / fs;
float coeff = 2.0f * cosf(w);

float s0 = 0.0f;
float s1 = 0.0f;
float s2 = 0.0f;

int cnt = vf0.size();
 
for (int i = 0; i < cnt; i++) 
	{
	s0 = vf0[i] + coeff * s1 - s2;
	s2 = s1;
	s1 = s0;
	}

float re = s1 - s2 * cosf( w );
float im = s2 * sinf( w );

float mag = sqrtf( re*re + im*im );

//scale to recover sine amplitude
return ( 2.0f / cnt ) * mag;
}
*/












// ---------------------------------
// PLL initialization
// ---------------------------------
void stereo_pll_init(stereo_pll_state& s, float adc_rate, int dwnsmple_srate )
{

s.fs = adc_rate;
s.dwn_srate = dwnsmple_srate;										//e.g: 240KHz
s.mpx_srate = s.dwn_srate/2;										//halfband decimator's o/p srate, 120KHz

s.prev.real = 0.0f;
s.prev.imag = 0.0f;
s.first_sample = true;

float w0 = twopi * cn_pilot_hz / s.mpx_srate;
float alpha = sinf(w0) / (2.0f * cn_pilot_q);
float cosw = cosf(w0);

float b0 =  alpha;
float b1 =  0.0f;
float b2 = -alpha;

float a0 =  1.0f + alpha;
float a1 = -2.0f * cosw;
float a2 =  1.0f - alpha;

s.b0 = b0 / a0;
s.b1 = b1 / a0;
s.b2 = b2 / a0;
s.a1 = a1 / a0;
s.a2 = a2 / a0;

s.z1 = 0.0f;
s.z2 = 0.0f;

// --------------------------
// normalize biquad DC gain
// --------------------------
float dc_gain = (s.b0 + s.b1 + s.b2) / (1.0f + s.a1 + s.a2);

if(fabsf(dc_gain) > 1e-12f)
{
	s.b0 /= dc_gain;
	s.b1 /= dc_gain;
	s.b2 /= dc_gain;
}
//--------------------------------------------------------

float wn = twopi * cn_pll_bw_hz / s.mpx_srate;

s.kp = 2.0f * wn;
s.ki = wn * wn;

s.phase = 0.0f;

s.freq_nominal = twopi * cn_pilot_hz / s.mpx_srate;
s.freq = s.freq_nominal;

float pull = twopi * cn_pll_pull_hz / s.mpx_srate;

s.freq_min = s.freq_nominal - pull;
s.freq_max = s.freq_nominal + pull;

s.integrator = 0.0f;

s.err_avg = 0.0f;
s.locked = false;

s.osc_19_cos = 1.0f;
s.osc_19_sin = 0.0f;
s.osc_38_cos = 1.0f;
//    s.osc_38_sin = 0.0f;
s.osc_57_cos = 1.0f;
s.osc_57_sin = 0.0f;

s.i_lp = 0.0f;
s.q_lp = 0.0f;

//float lp_bw = 20.0f;
//s.lp_alpha = twopi * lp_bw / s.mpx_srate;

//s.pilot_power_lp = 0.0f;

//float power_bw = 5.0f;
//s.power_alpha = twopi * power_bw / s.mpx_srate;


//float pilot_presence_bw = 5.0f;
//s.pilot_presence_alpha = twopi * pilot_presence_bw / s.mpx_srate;


//float pilot_swing_bw = 1.0f;
//s.pilot_swing_alpha = twopi * pilot_swing_bw / s.mpx_srate;


stereo_deemphasis_init( s );
}







void stereo_deemphasis_init( stereo_pll_state& s )
{
s.deemph_ch0 = 0.0f;
s.deemph_ch1 = 0.0f;

float tau = cn_deemphasis_tau_50us;										//Non USA
if( i_deemphasis == 2 ) tau = cn_deemphasis_tau_75us;

float dt = 1.0f / s.mpx_srate;

s.deemph_alpha = dt / ( tau + dt );

}









// ---------------------------------
// FM discriminator
// ---------------------------------
static inline float fm_discriminator(stereo_pll_state& s, const filter_code::st_cplex_tag &x)
{
if(s.first_sample)
    {
    s.prev = x;
    s.first_sample = false;
    return 0.0f;
    }

//    std::complex<float> d = x * std::conj(s.prev);
filter_code::st_cplex_tag d = x * s.prev.conj();
s.prev = x;

return atan2f(d.imag, d.real);
}



// ---------------------------------
// Biquad lowpass process
// ---------------------------------
static inline float biquad_process(stereo_pll_state& s, float x)
{
    float y = s.b0 * x + s.z1;
    s.z1 = s.b1 * x - s.a1 * y + s.z2;
    s.z2 = s.b2 * x - s.a2 * y;
    return y;
}







float pilot_rctfy = 0;
//float pilot_swing_min = 1e9;
float pilot_swing_max = 0;
int pilot_rctfy_cnt = 0;
float pilot_rctfy_delayed = 0.0f;
float pilot_rctfy_delta;
int pilot_bounce_cnt = 0;





// ---------------------------------
// PLL process
// ---------------------------------

void stereo_pll_process(
    stereo_pll_state& s,
    vector<filter_code::st_cplex_tag> &x,
    std::vector<float>& vch0,
    std::vector<float>& vch1, float gain_out )

{
std::vector<filter_code::st_cplex_tag> vcplex_dummy;
std::vector<st_spect_tag> vspect_dummy;
std::vector<double> vdble_dummy;
std::vector<float> vfloat_dummy;

float rate_factor = (320000.0f / downsample_srate);						//used for simplictic pilot tone presence detection


//vector<filter_code::st_cplex_tag> x2;


//   if( s.fs != downsample_srate / 2 ) stereo_pll_init( s, g_dev_bw, downsample_srate / 2 );	//init for mpx srate in use if req (halfband decimator o/p srate)


    size_t n = x.size();
//    size_t n = x2.size();


    std::vector<filter_code::st_cplex_tag> vcplx;    // for plotting
    std::vector<float> vmpx; 
    std::vector<float> vfloat_probe;


//    std::vector<filter_code::st_cplex_tag> vcplx0;
 //   std::vector<filter_code::st_cplex_tag> vcplx1;

//--- FM discriminator ---
for(size_t i = 0; i < n; i++)
    {
    float mpx = fm_discriminator(s, x[i]);

//	mpx = fm_stereo_dwnsmpl_aa.process( mpx );

	vmpx.push_back( mpx );


	
//	filter_code::st_cplex_tag oc;

//	oc.real = mpx;
//	oc.imag = 0.0f;
//
//	vcplx0.push_back( oc );
	}


st_plot.which = en_gls_fm_demod_spect_mpx;
st_plot.srate_in = downsample_srate;
st_plot.srate_in = 0;

st_plot.vflt0 = &vmpx;
plot_load_struct_audio_thrd( st_plot, (void*)&vmpx, (void*)0 );							//plot


std::vector<float> vmpx_dwn; 
st_plot.srate_in = fm_decimator_srate;
fm_stereo_decimator.process( vmpx, vmpx_dwn );							//halfband decimator, halves srate, e.g: 240K-->120KHz (fir stopband at around 60KHz)



//plot_srate = fm_decimator_srate;										//fm decimator srate, for display purposes only


st_plot.which = en_gls_fm_demod_mpx_halfband_decimator_spect;
st_plot.srate_in = fm_decimator_srate;
st_plot.vflt0 = &vmpx_dwn;
plot_load_struct_audio_thrd( st_plot, (void*)&vmpx_dwn, (void*)0 );				//plot

//vmpx_dwn = vmpx;

//	vcplx1 = vcplx0;
	
n = vmpx_dwn.size();
vch0.resize(n);
vch1.resize(n);

std::vector<float> vpilot_rectified_peak; 
std::vector<float> vpilot_bounce; 
//std::vector<float> vpilot_power; 
std::vector<float> vpilot_pll_err; 
std::vector<float> vleft_minus_right_bpf; 
std::vector<float> vleft_minus_right; 
std::vector<float> vpll_38Kosc; 

std::vector<float> vpll_pilot_recvr;
std::vector<float> vpll_pilot_local;


//---- get filter index to 'en_ftid_iir_notch_fm_19k_pilot' ------
int idx_iir_fmst_19k_pilot_notch = 0;
int ii = en_ftid_iir_fmst_19k_pilot_notch;
if( st_filt[ii].flags & en_fflg_on )
	{
	int idx = st_filt[ii].pending_idx.exchange( -1, std::memory_order_acquire );	//thrd safe
	
	if( idx >= 0 )								//new filter set by gui thrd?
		{
		st_filt[ii].active_idx = idx;
//		st_filt[ii].iir[idx].dly0 = st_filt[ii].iir[!idx].dly0;			//reduce glitching by copying delay contents over
//		st_filt[ii].iir[idx].dly1 = st_filt[ii].iir[!idx].dly1;
		}
	else{
		idx = st_filt[ii].active_idx;									//if no new filter, keep using same filter
		}
	
	idx_iir_fmst_19k_pilot_notch = idx;
	}
//------------------------------




//---- get filter index to 'en_ftid_fir_fmst_15k_lpf0' ------
int idx_fir_fmst_15k_lpf0 = 0;
ii = en_ftid_fir_fmst_15k_lpf0;
if( st_filt[ii].flags & en_fflg_on )
	{
	int idx = st_filt[ii].pending_idx.exchange( -1, std::memory_order_acquire );	//thrd safe
	
	if( idx >= 0 )								//new filter set by gui thrd?
		{
		st_filt[ii].active_idx = idx;
//		st_filt[ii].iir[idx].dly0 = st_filt[ii].iir[!idx].dly0;			//reduce glitching by copying delay contents over
//		st_filt[ii].iir[idx].dly1 = st_filt[ii].iir[!idx].dly1;
		}
	else{
		idx = st_filt[ii].active_idx;									//if no new filter, keep using same filter
		}
	
	idx_fir_fmst_15k_lpf0 = idx;
	}
//------------------------------






//---- get filter index to 'en_ftid_fir_fmst_15k_lpf1' ------
int idx_fir_fmst_15k_lpf1 = 0;
ii = en_ftid_fir_fmst_15k_lpf1;
if( st_filt[ii].flags & en_fflg_on )
	{
	int idx = st_filt[ii].pending_idx.exchange( -1, std::memory_order_acquire );	//thrd safe
	
	if( idx >= 0 )								//new filter set by gui thrd?
		{
		st_filt[ii].active_idx = idx;
//		st_filt[ii].iir[idx].dly0 = st_filt[ii].iir[!idx].dly0;			//reduce glitching by copying delay contents over
//		st_filt[ii].iir[idx].dly1 = st_filt[ii].iir[!idx].dly1;
		}
	else{
		idx = st_filt[ii].active_idx;									//if no new filter, keep using same filter
		}
	
	idx_fir_fmst_15k_lpf1 = idx;
	}
//------------------------------






//---- get filter index to 'en_ftid_fir_fmst_23_53k_bpf' ------
int idx_fir_fmst_23_53k_bpf = 0;
ii = en_ftid_fir_fmst_23_53k_bpf;
if( st_filt[ii].flags & en_fflg_on )
	{
	int idx = st_filt[ii].pending_idx.exchange( -1, std::memory_order_acquire );	//thrd safe
	
	if( idx >= 0 )								//new filter set by gui thrd?
		{
		st_filt[ii].active_idx = idx;
//		st_filt[ii].iir[idx].dly0 = st_filt[ii].iir[!idx].dly0;			//reduce glitching by copying delay contents over
//		st_filt[ii].iir[idx].dly1 = st_filt[ii].iir[!idx].dly1;
		}
	else{
		idx = st_filt[ii].active_idx;									//if no new filter, keep using same filter
		}
	
	idx_fir_fmst_23_53k_bpf = idx;
	}
//------------------------------







    for(size_t i = 0; i < n; i++)
		{

//       float mpx = fm_discriminator(s, x[i]);
		
		float mpx_dwn = vmpx_dwn[i];									//e.g: 120KHz





        // --- narrowband pilot BPF ---
        float pilot = biquad_process(s, mpx_dwn);



//------------ pilot presence test -----------------
		if( fabsf(pilot) > pilot_rctfy ) pilot_rctfy = fabsf(pilot);		//peak rectify

		if( pilot_rctfy > 10.0f) pilot_rctfy = 10.0f;
		
//		vfloat_probe.push_back( pilot_rctfy_delta );
		
		pilot_rctfy *= 0.9998f;												//discharge
//		if( pilot_rctfy < pilot_swing_min ) pilot_swing_min = pilot_rctfy;
		if( pilot_rctfy > pilot_swing_max ) pilot_swing_max = pilot_rctfy;	//2nd peak top up
		
		
//		pilot_swing_min += 1e-7;
//		pilot_swing_max *= 0.99995 * (320000.0f / downsample_srate);
		pilot_swing_max -= 1e-5 * rate_factor;								//2nd discharge

//		if( pilot_swing_min > 10.0f ) pilot_swing_min = 10.0f;
		if( pilot_swing_max < 0.0f ) pilot_swing_max = 0.0f;


		vpilot_rectified_peak.push_back( pilot_swing_max );

		if( pilot_rctfy_cnt > 200 ) 										//wait a number of samples
			{
			pilot_rctfy_cnt = 0;
			pilot_rctfy_delta = ( pilot_swing_max - pilot_rctfy_delayed );	//calc how much the peak recitified pilot level fluctuates over a num of samples
			
			pilot_rctfy_delayed = pilot_swing_max;		
			}
		pilot_rctfy_cnt++;


//		s.pilot_swing_lp += s.pilot_swing_alpha * (pilot_rctfy_delta - s.pilot_swing_lp);


		if( fabs( pilot_rctfy_delta ) > 75e-3 )							//check if pilot level has fluctuated passed a set limit 
			{
			pilot_bounce_cnt = 10000;									//reload a counter to show pilot is suspect
			}


		pilot_bounce_cnt--;
		if( pilot_bounce_cnt < 0 ) pilot_bounce_cnt = 0;

		vpilot_bounce.push_back( pilot_bounce_cnt );

        bool pilot_present;
		pilot_present = ( ( pilot_swing_max > 100e-3 ) && ( pilot_swing_max < 280e-3) && (pilot_bounce_cnt == 0) );	 //check if pilot is within a range and has limited bounce


//		vfloat_probe.push_back( pilot_swing_max );


//--------------------------------------------------




//vfloat_probe.push_back( pilot_swing_max );
		
        // --- track pilot power ---
//        float pilot_sq = pilot * pilot;
//        float pilot_sq = pilot;
//        s.pilot_power_lp += s.power_alpha * (pilot_sq - s.pilot_power_lp);



        
//        s.pilot_presence_lp += s.pilot_presence_alpha * (pilot_present - s.pilot_presence_lp);



//-------------- pilot tone pll -----------------
        float sinp, cosp;
        sincosf(s.phase, &sinp, &cosp);

        // --- HARD LIMIT PILOT (kept before LP) ---
//        float pilot_lim = (pilot >= 1e-3) ? 1.0f : -1.0f;

        float pilot_lim = 0.0f;
        
//		if( pilot >= 20e-3) pilot_lim = 1.0f;
//		if( pilot <= -20e-3) pilot_lim = -1.0f;
		if( pilot >= 20e-3) pilot_lim = 20e-3;							//clip incomming pilot to a known limit, if req, keeps 'error' with range below
		if( pilot <= -20e-3) pilot_lim = -20e-3;

//pilot_lim = pilot;

		vpll_pilot_recvr.push_back( cosp );
		vpll_pilot_local.push_back( pilot );

        // --- COHERENT DETECTION ---
        float i_mix = pilot_lim * cosp;
        float q_mix = pilot_lim * sinp;

        // --- increase LP bandwidth for broadcast-grade PLL ---
 //       float lp_bw = 300.0f;          // ~300 Hz for fast locking
        float lp_bw = 100.0f;          // ~100 Hz
        s.lp_alpha = twopi * lp_bw / s.mpx_srate;

        s.i_lp += s.lp_alpha * (i_mix - s.i_lp);
        s.q_lp += s.lp_alpha * (q_mix - s.q_lp);

//		if( s.i_lp > 5e-3f ) s.i_lp -= 1e-3;							//force a limit of swing
//		if( s.i_lp < -5e-3f ) s.i_lp += 1e-3;

        // --- phase detector (Costas-style) ---
		float error = -s.i_lp * s.q_lp;									//swings between -80e-6 --> +80e-6  when wheeling

        s.integrator += s.ki * error;
//vfloat_probe.push_back( s.integrator + s.kp * error );

//        float multp = 1.0;
//        if( fabs.integrator >  ) multp = 1.0;

//        s.freq = s.freq_nominal + multp * (s.integrator + s.kp * error);
       
//		if( s.integrator > 0.0 )  s.integrator -= 40e-9;
//		if( s.integrator < 0.0 )  s.integrator += 40e-9;

       s.freq = s.freq_nominal + 300.0f * (s.integrator + s.kp * error);	//adj local osc running freq

       
//s.freq = s.freq_nominal;												//break pll feedback to debug (freewheel)

		vfloat_probe.push_back( s.integrator + s.kp * error );

        if(s.freq > s.freq_max) s.freq = s.freq_max;
        if(s.freq < s.freq_min) s.freq = s.freq_min;

        s.phase += s.freq;
        if(s.phase >= twopi) s.phase -= twopi;
        else if(s.phase < 0.0f) s.phase += twopi;

        // --- generate oscillators ---
        sincosf(s.phase, &s.osc_19_sin, &s.osc_19_cos);					//19KHz

//        float p2 = 2.0f * s.phase + pi*( g_dbg1/100.0f );
//        float p2 = 2.0f * s.phase + pi*( 36/100.0f );	//this '+pi*factor' fudge was adjusted manually (via 'g_dbg1') to determine best factor for increasing left right seperation
        float p2 = 2.0f * s.phase;
       
		s.osc_38_cos = cosf( p2 );										//38KHz

		vpll_38Kosc.push_back( s.osc_38_cos );


//        float p3 = 3.0f * s.phase;
//        sincosf(p3, &s.osc_57_sin, &s.osc_57_cos);					//57KHz RDS, poss feature in future

        // --- lock detect ---
        float abs_err = fabsf(error);
        s.err_avg = 0.9995f * s.err_avg + 0.0005f * abs_err;

//        bool freq_ok = (fabsf(s.freq - s.freq_nominal) < (twopi * 5.0f / s.fs));

//       float i_power = s.i_lp * s.i_lp;
//       float q_power = s.q_lp * s.q_lp;
//        float corr_power = i_power + q_power;

//		float coherence = 0.0f;
//        if(s.pilot_power_lp > 1e-12f)
//        coherence = corr_power / s.pilot_power_lp;

//        bool phase_dominant = (i_power > 10.0f * q_power);
//        bool snr_ok = (i_power > 1e-8f);

//        bool pilot_present = (coherence > 0.2f) && phase_dominant && snr_ok;
//        bool pilot_present = (coherence > 0.2f);// && snr_ok;

//        bool pilot_present;// = (coherence > 0.2f);

//		pilot_present = ( s.pilot_power_lp >= 12e-3 );					//set a min lev for presence


//		vpilot_power.push_back( s.pilot_power_lp );

		vpilot_pll_err.push_back( s.err_avg );

//        s.locked = (s.err_avg < 0.009f) && freq_ok;// && pilot_present;
        s.locked = (s.err_avg < 50e-3) && pilot_present;

//-----------------------------------------------






//------------------ stereo channel decoding ---------------------

//		float mpx_notch = filter_code::iir_process( fm_ster_iir_19k_notch, mpx_dwn );		//remove pilot tone

		ii = en_ftid_iir_fmst_19k_pilot_notch;
		int idx = idx_iir_fmst_19k_pilot_notch;
		float mpx_notch = iir_process_float( st_filt[ii].iir[ idx ], mpx_dwn );		//remove pilot tone


//if( g_dbg0 == 1 ) mpx_notch = mpx_dwn;


//		filter_code::fir_in( fm_ster_fir_15k_lpf0, mpx_notch );			//L+R 0-->15KHz
//		float mpx_LplusR = filter_code::fir_out( fm_ster_fir_15k_lpf0 );




//		filter_code::fir_in( st_filt[ii].fir[ idx ], mpx_notch );				//L+R 0-->15KHz
//		float mpx_LplusR = filter_code::fir_out( st_filt[ii].fir[ idx ] );

		ii = en_ftid_fir_fmst_15k_lpf0;
		idx = idx_fir_fmst_15k_lpf0;
		float mpx_LplusR = filter_code::fir_process( st_filt[ii].fir[ idx ], mpx_notch );			//L+R 0-->15KHz
		
		
		
		
//		filter_code::fir_in( fm_ster_fir_23_53k_bpf, mpx_notch );									//L-R, 23-->53KHz
//		float mpx_LminusR = filter_code::fir_out( fm_ster_fir_23_53k_bpf );

		ii = en_ftid_fir_fmst_23_53k_bpf;
		idx = idx_fir_fmst_23_53k_bpf;
		float mpx_LminusR_38K = filter_code::fir_process( st_filt[ii].fir[ idx ], mpx_notch );		//L-R, 23-->53KHz
		
		

		vleft_minus_right_bpf.push_back( mpx_LminusR_38K );

		
		float mpx_LminusR_demux = mpx_LminusR_38K * s.osc_38_cos/* * ( g_dbg1/100.0f )*/;			//38KHz mix down to baseband
	
	

//		filter_code::fir_in( fm_ster_fir_15k_lpf1, mpx_LminusR_1 );		//L-R, 0-->15KHz clean up, removes 38KHz L-R images
//		float mpx_LminusR_2 = filter_code::fir_out( fm_ster_fir_15k_lpf1 );

		ii = en_ftid_fir_fmst_15k_lpf1;
		idx = idx_fir_fmst_15k_lpf1;

		float mpx_LminusR_bband = filter_code::fir_process( st_filt[ii].fir[ idx ], mpx_LminusR_demux );	//L-R, 0-->15KHz clean up, removes 38KHz L-R images



		vleft_minus_right.push_back( mpx_LminusR_bband );

		mpx_LminusR_bband *= 2.0f;										//double L-R here, req as lpf stripped the DSB-SC image around 76KHz which halved levels


		//---non bpf path's delay---
		float mpx_LplusR_dly = fm_delay[fmd_wr];						//delayed L+R to match mpx_LminusR_bband's additional bpf filter delay
		fm_delay[fmd_wr] = mpx_LplusR;									//delay L+R to match mpx_LminusR_bband's additional bpf filter delay
		fmd_wr++;
//		if( fmd_wr >= (( cn_fm_stereo_bpf_taps - 1 )/2) - g_dbg1  ) fmd_wr = 0;			//delay req is ((taps-1) / 2) sample

		if( fmd_wr >= ( ( cn_fm_stereo_bpf_taps - 1 )/2) ) fmd_wr = 0;	//delay req is ((taps-1) / 2) sample
		//--------------------------
  
  
        float ch0 = 0.5f * ( mpx_LplusR_dly + mpx_LminusR_bband );		//matrix
        float ch1 = 0.5f * ( mpx_LplusR_dly - mpx_LminusR_bband );
 
 
		if( i_deemphasis != 0 ) 
			{
			s.deemph_ch0 += s.deemph_alpha * ( ch0 - s.deemph_ch0 );	//deemphasis iir
			ch0 = s.deemph_ch0;

			s.deemph_ch1 += s.deemph_alpha * ( ch1 - s.deemph_ch1 );
			ch1 = s.deemph_ch1;
			}


		if( 1 )
			{
			bool bclip = 0;
			float clip_level_audio_fm_stereo = 1.3;			//need this as very low DwnSrate causes very high audio level noise for the FM Stereo decoder
			
			//---- clipper -----	
			if( ch0 < -clip_level_audio_fm_stereo ) { ch0 = -clip_level_audio_fm_stereo; bclip = 1; }
			if( ch0 > clip_level_audio_fm_stereo )  { ch0 =  clip_level_audio_fm_stereo; bclip = 1; }

			if( ch1 < -clip_level_audio_fm_stereo ) { ch1 = -clip_level_audio_fm_stereo; bclip = 1; }
			if( ch1 > clip_level_audio_fm_stereo )  { ch1 =  clip_level_audio_fm_stereo; bclip = 1; }
			//------------------


			if( bclip ) clip_audio_cnt = clip_audio_cnt_max;			//turn on clip led
			}



        vch0[i] = ch0 * gain_out;													
        vch1[i] = ch1 * gain_out;
//-----------------------------------------------





//        vfloat_probe.push_back( mpx );
//        vfloat_probe.push_back( s.err_avg );
        
//        vfloat_probe.push_back( mpx_LplusR );
        
//        vfloat_probe.push_back(pilot);
//		vfloat_probe.push_back(s.pilot_power_lp);

        // --- debug complex vector ---
        filter_code::st_cplex_tag oc;
        oc.real = x[i].real;
        oc.imag = x[i].imag;
        vcplx.push_back(oc);
		}



	st_plot.which = en_gls_fm_demod_pll_osc_compare;
	st_plot.srate_in = fm_decimator_srate;
	st_plot.vflt0 = &vpll_pilot_recvr;
	st_plot.vflt1 = &vpll_pilot_local;
	plot_load_struct_audio_thrd( st_plot, (void*)&vpll_pilot_recvr, (void*)&vpll_pilot_local );			//plot 


		
	st_plot.which = en_gls_fm_demod_pilot_pll_err;
	st_plot.srate_in = fm_decimator_srate;
	st_plot.vflt0 = &vpilot_pll_err;
	plot_load_struct_audio_thrd( st_plot, (void*)&vpilot_pll_err, (void*)0 );			//plot


	st_plot.which = en_gls_fm_demod_pilot_rectified_peak;
	st_plot.srate_in = fm_decimator_srate;
	st_plot.vflt0 = &vpilot_rectified_peak;
	plot_load_struct_audio_thrd( st_plot, (void*)&vpilot_rectified_peak, (void*)0 );	//plot

	st_plot.which = en_gls_fm_demod_pilot_bounce;
	st_plot.srate_in = fm_decimator_srate;
	st_plot.vflt0 = &vpilot_bounce;
	plot_load_struct_audio_thrd( st_plot, (void*)&vpilot_bounce, (void*)0 );			//plot


	st_plot.which = en_gls_fm_demod_pll_38K_osc;
	st_plot.srate_in = fm_decimator_srate;
	st_plot.vflt0 = &vpll_38Kosc;
	plot_load_struct_audio_thrd( st_plot, (void*)&vpll_38Kosc, (void*)0 );				//plot


	st_plot.which = en_gls_fm_demod_left_minus_right_bpf_spect;
	st_plot.srate_in = fm_decimator_srate;
	st_plot.vflt0 = &vleft_minus_right_bpf;
	plot_load_struct_audio_thrd( st_plot, (void*)&vleft_minus_right_bpf, (void*)0 );	//plot


	st_plot.which = en_gls_fm_demod_left_minus_right_timedomain;
	st_plot.srate_in = fm_decimator_srate;
	st_plot.vflt0 = &vleft_minus_right;
	plot_load_struct_audio_thrd( st_plot, (void*)&vleft_minus_right, (void*)0 );		//plot

	st_plot.which = en_gls_fm_demod_left_minus_right_spect;
	st_plot.srate_in = fm_decimator_srate;
	st_plot.vflt0 = &vleft_minus_right;
	plot_load_struct_audio_thrd( st_plot, (void*)&vleft_minus_right, (void*)0 );		//plot


	st_plot.which = en_gls_fm_ster_demux_aud_ch0_timedomain;
	st_plot.srate_in = fm_decimator_srate;
	st_plot.vflt0 = &vch0;
	plot_load_struct_audio_thrd( st_plot, (void*)&vch0, (void*)0 );						//plot


	st_plot.which = en_gls_fm_ster_demux_aud_ch1_timedomain;
	st_plot.srate_in = fm_decimator_srate;
	st_plot.vflt0 = &vch1;
	plot_load_struct_audio_thrd( st_plot, (void*)&vch1, (void*)0  );					//plot



	st_plot.which = en_gls_fm_ster_demux_aud_ch0_spect;
	st_plot.srate_in = fm_decimator_srate;
	st_plot.vflt0 = &vch0;
	plot_load_struct_audio_thrd( st_plot, (void*)&vch0, (void*)0 );						//plot


	st_plot.which = en_gls_fm_ster_demux_aud_ch1_spect;
	st_plot.srate_in = fm_decimator_srate;
	st_plot.vflt0 = &vch1;
	plot_load_struct_audio_thrd( st_plot, (void*)&vch1, (void*)0 );						//plot


    // --- plotting ---
    if(1)
    {

	st_plot.which = en_gls_tdm_probe_vflt0;
	st_plot.srate_in = fm_decimator_srate;
	st_plot.vflt0 = &vfloat_probe;
	plot_load_struct_audio_thrd( st_plot, (void*)&vfloat_probe, (void*)0 );				//plot


	st_plot.which = en_gls_spect_probe_vflt0;
	st_plot.srate_in = fm_decimator_srate;
	st_plot.vflt0 = &vfloat_probe;
	plot_load_struct_audio_thrd( st_plot, (void*)&vfloat_probe, (void*)0 );				//plot



	st_plot.which = en_gls_spect_probe_vcpx0;
	st_plot.srate_in = fm_decimator_srate;
	st_plot.vcpx0 = &vcplx;
	plot_load_struct_audio_thrd( st_plot, (void*)&vcplx, (void*)0 );					//plot
    }
}

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

//fm_demode_code.h

//v1.01




#ifndef fm_demode_code_h
#define fm_demode_code_h



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
#include <complex>


#include "globals.h"
#include "GCProfile.h"
#include "filter_code.h"
#include "rtl_graph.h"
#include "iir_sos_code.h"


using namespace std;



#define cn_pilot_hz        19000.0f
#define cn_min_srate       192000.0f
#define cn_pll_bw_hz       50.0f		//100.0f
#define cn_pll_pull_hz     100.0f		//200.0f
#define cn_pilot_q         80.0f
#define pi                 (3.14159265358979323846f)
#define twopi              (2.0f*pi)


#define cn_fm_stereo_bpf_taps 55										//this is used to delay fm stereo L+R, it must be the tap count of the BPF used for L-R
																		//refer 'fm_delay'    rem fir bpf delay is ((taps-1) / 2) samples

struct stereo_pll_state
{
    float fs;															//this is the srate of the rtl adc  'DevBw' ('g_dev_bw')
    float dwn_srate;													//e.g: downsampler o/p srate: 240000

	int mpx_srate;						//mpx srate out of halfband decimator, eg: downsampler o/p srate: 240000, fm stereo halfband decimator o/p srate: 120000 Hz

    filter_code::st_cplex_tag prev;
    bool first_sample;

    float b0, b1, b2;
    float a1, a2;
    float z1, z2;

    float phase;
    float freq;
    float integrator;

    float kp;
    float ki;

    float freq_nominal;
    float freq_min;
    float freq_max;

    float err_avg;
    bool locked;

    float osc_19_cos;
    float osc_19_sin;

    float osc_38_cos;
//    float osc_38_sin;

    float osc_57_cos;
    float osc_57_sin;

    float i_lp;
    float q_lp;
    float lp_alpha;

//    float pilot_power_lp;
//    float power_alpha;
  
//    float pilot_presence_lp;
//    float pilot_presence_alpha;
 
//    float pilot_swing_lp;
//    float pilot_swing_alpha;
 
    
    float deemph_alpha;													//deemphasis
    float deemph_ch0;													//one sample delay storage for deemphasis iir
    float deemph_ch1;

};



extern void stereo_deemphasis_init( stereo_pll_state& s );



#endif	//#ifndef fm_demode_code_h

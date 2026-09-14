/*
Copyright (C) 2026 BrerDawg

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


//v1.01

//demod_code.h

#ifndef demod_code_h
#define demod_code_h



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
#include "rtl_graph.h"
#include "halfband_poly_optimised_float.h"
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


#define cn_bf1_iq_sz 262144												//e.g:  3168000 * (2048/48000) ) = 135168  ,     1920000 * (2048/48000) ) = 81920


using namespace std;


extern void demod_iso();


/*
struct AGC
{
    float env      = 0.0f;   // envelope state
    float gain     = 1.0f;   // smoothed gain
    float alpha;             // envelope coefficient
    float beta;              // gain smoothing coefficient
    float target;            // linear target level
    float max_gain;
};
*/

/*
struct st_baseline_remove_tag
{
	float b0;     // baseline estimate ch0
	float b1;     // baseline estimate ch1
	float alpha;  // same coefficient for both

	void init( float fc, float fs )
	{
		b0 = 0.0f;
		b1 = 0.0f;

		alpha = expf( -2.0f * (float)M_PI * fc / fs );
	}

	void process( float &ch0, float &ch1 )
	{
		// --- ch0 ---
		b0 = (1.0f - alpha) * ch0 + alpha * b0;
		ch0 = ch0 - b0;

		// --- ch1 ---
		b1 = (1.0f - alpha) * ch1 + alpha * b1;
		ch1 = ch1 - b1;
	}
};
*/




struct st_audio_filter_tag												//simple iir used for metering
{
float z0;
float alpha;
};




//linear upsampler, fast
struct lin_up2x
{
    float prev = 0.0f;
    bool  first = true;

    inline void process(float in, float* out)			//'float* out' needs to point to buf that holds 2 floats,  'float out[2];'
		{
		if (first)
			{
			out[0] = out[1] = in;
			prev = in;
			first = false;
			return;
			}

		float d = (in - prev) * 0.5f;  // half step

		out[0] = prev + d;  // halfway between prev and in
		out[1] = in;

		prev = in;
		}
};





//-------------------------------------------------------
//efficient recursive sinewave quadrature oscillator, similar to analogue dual integrators feeding each other, one of the integrators is also inverting its o/p
//NOTE, you MUST tweek combined I/Q magnitude back to 1.0 (infrequently), otherwise it will DECAY to zero due to float precision errors,
//e.g: I_tweeked = I/sqrt( (I*I) + (Q*Q) ), use newton/raph to numerically simplify calc '1/sqrt()'  this avoids slower call to 'std::sqrt()' and the divide 
//SEE demo example code below:
struct st_dual_integrator_osc_tag
{ 
//bool active;
float sin;																//load this with sinf(w), this is a fixed value and defines the freq to o/p, in effect they are elements of a rotation matrix, w is angular vel, w = twopi * freq / srate;
float cos;																//load this with cosf(w)
float I;																//cur o/p level, initially set these to I = 1.0f,  Q is 0.0
float Q;	
};

/* -- EXAMPLE of use --

#define cn_dual_integrator_osc_cnt 1

st_dual_integrator_osc_tag st_di_osc[ cn_dual_integrator_osc_cnt ];


void osc_build()
{
float time_per_sample = 1.0f/g_dev_bw;

st_di_osc[0].I = 0.1f;													//init osc's quadrature o/ps
st_di_osc[0].Q = 0.0f;

float w = twopi * 400 / srate;											//400Hz
st_di_osc[0].cos = cosf( w );											//holds the fixed vals used in differential calc, these are used when inc'ing to next val
st_di_osc[0].sin = sinf( w );

}




//inc dual integrator sine oscillators
for( int i = 0; i < cn_dual_integrator_osc_cnt; i++ )
	{
	st_dual_integrator_osc_tag *osc = st_di_osc + i;
	
	float ii = osc->cos * osc->I - osc->sin * osc->Q;
	float qq = osc->sin * osc->I + osc->cos * osc->Q;
	osc->I = ii;
	osc->Q = qq;
	}




//tweek each sine oscillator's I/Q magn, so to keep it close to 1.0, otherwise it will decay due to float precision errors
//ONLY need to do this infrequently, such as at frame rate

void osc_tweek_mag()
{
for( int i = 0; i < cn_dual_integrator_osc_cnt; i++ )
	{
	st_dual_integrator_osc_tag *osc = st_di_osc + i;

    float mag = osc->I * osc->I + osc->Q * osc->Q;
    float corr = 1.5f - 0.5f * mag;  	//1st order Newton/Raphson to approximate: '1/sqrt(mag)'
										//e.g: I_tweeked = I/sqrt( (I*I) + (Q*Q) ), use newton/raph to numerically simplify calc '1/sqrt()'  this avoids slower call to 'std::sqrt()' and the divide 
	osc->I *= corr;	
    osc->Q *= corr;
	}
}
//---------------------
*/

//-------------------------------------------------------




#endif



/*
Copyright (C) 2019 BrerDawg

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



//rt_code.cpp
//1v1.01	2020-nov-07



#include "rt_code.h"



#define twopi (2.0*M_PI)



extern mystr tim1;
extern rtaud rta;																//rtaudio
extern st_rtaud_arg_tag st_rta_arg;											//this is used in audio proc callback to work out chan in/out counts

extern int aud_op_srate;
extern int framecnt;

extern float time_per_sample;
extern bool audio_started;
extern int audio_source;
//extern st_iir_2nd_order_tag iir0;

//extern audio_formats af0;
extern uint64_t audptr;
extern bool aud_loaded;
extern double dfract_audptr;
extern float play_speed;

extern bool grph_bf_loaded;	
extern float grph_bf0[];
extern float grph_bf1[];
extern float grph_bf2[];



mystr tim5;

int proc_cnt = 0;
float freq0 =  framecnt * 1.0;
float freq1 = aud_op_srate / 24;
float theta00 = 0;
float theta01 = 0;
float theta00_inc;
float theta01_inc;

extern void update_audio( float *bf0, float *bf1, int frmcnt );


//generates a rnd num between -1.0 -> 1.0
float rnd()
{
float frnd =  (float)( RAND_MAX / 2 - rand() ) / (float)( RAND_MAX / 2 );

return frnd;
}



float aud_bf0[ 8192 ];
float aud_bf1[ 8192 ];


//-------------------- realtime audio proc -----------------------------
int cb_audio_proc_rtaudio( void *bf_out, void *bf_in, int frames, double streamTime, RtAudioStreamStatus status, void *arg_in )
{
bool vb = 1;

double dt = tim5.time_passed( tim5.ns_tim_start );
tim5.time_start( tim5.ns_tim_start );

if( !(proc_cnt % 400) )
	{
	if(vb) printf( "cb_audio_proc_rtaudio() - dt: %f, frames: %d\n", dt, frames );
	}
proc_cnt++;
	

if ( status ) std::cout << "cb_audio_proc_rtaudio() - Stream over/underflow detected." << std::endl;



float *bfin = (float *) bf_in;											//depends on which 'audio_format' data type, refer: 'RtAudioFormat'
float *bfout = (float *) bf_out;


st_rtaud_arg_tag *arg = (st_rtaud_arg_tag*) arg_in;

//st_osc_params_tag *usr = (st_osc_params_tag*) arg->usr_ptr;


float freq_fit_in_framecnt = 1.0 / (framecnt * time_per_sample );		//freq that gives 1 exact cycle in framecnt

freq0 = freq_fit_in_framecnt * 100;
freq1 = freq_fit_in_framecnt * 20;

theta00_inc = freq0 * twopi / aud_op_srate;
theta01_inc = freq1 * twopi / aud_op_srate;


float f0 = 0;
float f1 = 0;


update_audio( aud_bf0, aud_bf1, framecnt );								//get audio samples
 
int pinterleve = 0;
for ( int i = 0; i < framecnt; i++ )
	{
audio_source = 3;

	if( audio_source == 0 )
		{
		float famp = 0.5;
		f0 = famp * cosf( theta00 );
		f1 = famp * cosf( theta01 );

		theta00 += theta00_inc;
		if( theta00 >= twopi ) theta00 -= twopi;

		theta01 += theta01_inc;
		if( theta01 >= twopi ) theta01 -= twopi;
		}

	if( audio_source == 1 )
		{
		float famp = 0.2;
		f0 = famp * rnd();
		f1 = famp * rnd();

		theta00 += theta00_inc;
		if( theta00 >= twopi ) theta00 -= twopi;

		theta01 += theta01_inc;
		if( theta01 >= twopi ) theta01 -= twopi;
		}


	if( audio_source == 2 )
		{
		float famp = 0.9;
		f0 = famp * rnd();
		f1 = famp * rnd();
		
//		filter_iir_2nd_order_2ch( f0, f1, iir0 );
//		filter_iir_2nd_order( f1, iir0 );
		}


	if( audio_source == 3 )
		{
		f0 = aud_bf0[i];
		f1 = aud_bf1[i];
		
//		if( aud_loaded )
//			{
//			float famp = 0.2;
//			f0 = famp * af0.pch0[audptr];
//			f1 = famp * af0.pch1[audptr];
//			audptr++;
//			if( audptr >= af0.sizech0 ) audptr = 0;
//			}
		}


//	float play_speed = 0.5;

/*	
	if( audio_source == 4 )
		{
		if( aud_loaded )
			{
			float bw = srate / 2;
			
			bw *= play_speed;
			
			f0 = qdss_resample( dfract_audptr, af0.pch0, af0.sizech0, bw, srate, 16 );
			float famp = 0.2;
			f0 = famp * f0;
			f1 = f0;
			
			dfract_audptr += play_speed;
//			float smp0 = af0.pch0[dfract_audptr];
			
//			uint64_t ptr1 = dfract_audptr + 1;
//			if( ptr1 >= af0.sizech0 ) ptr1 = af0.sizech0 - 1;
			
//			float smp1 = af0.pch0[ ptr1 ];
			
//			f0 = 0.2 * af0.pch0[audptr];
//			f1 = 0.2 * af0.pch1[audptr];
//			audptr++;
			if( dfract_audptr >= af0.sizech0 ) dfract_audptr = 0;
			}
		}
*/

	if( audio_source == 9 )
		{
		f0 = 0;
		f1 = 0;
		}
/*
	if( grph_bf_loaded == 0 )
		{
		grph_bf0[ i ] = f0 + 0.5;
		grph_bf1[ i ] = f1 + 0.5;
		}
*/
	bfout[ pinterleve ] = f0;
	pinterleve++;
	bfout[ pinterleve ] = f1;
	pinterleve++;

	}

//if( grph_bf_loaded == 0 ) grph_bf_loaded = 1;

return 0;
}


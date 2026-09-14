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
#include <complex>

#pragma once

//halfband_poly_optimised_float.h

//v1.01		19-mar-2026		//some code is via chatgpt, developed in  ../MyPrj/halfband_polyphase_symmetry_optimised/, 
							//use 'filter_code::make_kaiser_window( 127, 6, st_halfband_poly_optimised_float_tag.vcoeff )' to make coeffs suitable for halfband use (must be odd length), 
							//see below for coeff organisation that is suitable for halfband use


//halfband polyphase decimator, halves srate_in, fir has cutoff at (srate_in/4), exploits sequence reordering to reduce MAC count, code needs further work to increase performance,
//uses a local buffer to store input sample history so the next call to function: 'process()' is allowed access to past samples, 
//ensure vcoeff size is less than 'coeff_max' and has ODD count, coeffs must hold a symetrical impulse, coeffs must have odd entries with a val of zero,
//except center odd indexed coeff which must be 0.5 in value
//e.g 15 coeffs: -0.000676, 0.000000, 0.012719, -0.000000, -0.062740, 0.000000, 0.300939, 0.500000, 0.300939, 0.000000, -0.062740, -0.000000, 0.012719, 0.000000, -0.000676
namespace halfband_poly_optimised_float
{
constexpr int coeff_max = 2048;

struct st_hbpo_tag
{

string suser_name = { "not_named" };									//store a debug identifier here
unsigned cnt_in;
int tap_cnt = 0;
int wr_idx = 0;

vector<float> vcoeff;													//load coeffs here, try 'filter_code::make_kaiser_window( 127, 6, st_halfband_poly_optimised_float_tag.vcoeff )'


float bf_loc[coeff_max];												//hold some of the prev call's samples (from 'vin')

bool created = 0;

int mac_cnt = 0;

int phase = 0;    														//decimation phase (toggles 0/1)


//coeff count must be odd
//returns 1 on success, else 0
bool load_coeffs( vector<float> &vcef )
{
int taps_in = vcef.size();

created = 0;

if( (taps_in % 2) == 0 )
	{
	printf("st_halfband_poly_optimised_float_tag.process() - '%s': coeffs must have an odd count: %d\n", suser_name.c_str(), taps_in );
	return 0;
	}

if( (taps_in == 0) || (taps_in >= coeff_max) )
	{
	printf("st_halfband_poly_optimised_float_tag.process() - '%s':  zero, or too many coeffs, given %d, limit is %d\n", suser_name.c_str(), taps_in, coeff_max - 1);
	return 0;
	}

created = 1;
vcoeff = vcef;
tap_cnt = taps_in;

for( int i = 0; i < coeff_max; i++ )									//clear 
	{
	bf_loc[i] = 0;
	}

return 1;
}






//require 'vin.size()' >= 1
//returns 1 on success, else 0
bool process( vector<float> &vin, vector<float> &vout )
{
vout.clear();

if (!created ) return 0;

int cnt = vin.size();

if( cnt == 0 )
	{
	return 0;
	}

int middle = tap_cnt / 2;

for( int n = 0; n < cnt; n++ )
	{
	// write new sample into circular buffer
	bf_loc[wr_idx] = vin[n];

//	if( n == 0 ) bf_loc[wr_idx] = 10;									//insert a glitch for test purposes

	// only compute every 2nd sample (decimation by 2)
	if( phase == 0 )
		{
		float sum = 0;

		// even phase (symmetry optimised)
		for( int j = 0; j < middle; j += 2 )
			{
			int idx1 = wr_idx - j;
			if( idx1 < 0 ) idx1 += tap_cnt;

			int idx2 = wr_idx - (tap_cnt - 1 - j);
			if( idx2 < 0 ) idx2 += tap_cnt;

			sum += ( bf_loc[idx1] + bf_loc[idx2] ) * vcoeff[j];			//symmetry optimise, adds 2 input samples, then multiplies coeff once, halves multiplies req
			mac_cnt++;
			}

		// centre tap
		int idxm = wr_idx - middle;
		if( idxm < 0 ) idxm += tap_cnt;

		sum += bf_loc[idxm] * vcoeff[middle];
		mac_cnt++;

		vout.push_back( sum );
		}

	// advance circular buffer index
	wr_idx++;
	if( wr_idx >= tap_cnt ) wr_idx = 0;

	// toggle decimation phase
	phase ^= 1;
	}

return 1;
}

};	//struct st_hbpo_tag


}	//namespace halfband_poly_optimised_float



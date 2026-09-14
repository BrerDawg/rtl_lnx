/*
Copyright (C) 2023 BrerDawg

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



//bmp_code.h
//v1.04

#ifndef bmp_code_h
#define bmp_code_h

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <locale.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <algorithm>

#include "GCProfile.h"



using namespace std;


namespace bmp_code
	{


	//holds one pixel
	struct st_bmp_code_pixel
	{
	int x;
	int y;
	int r;
	int g;
	int b;
	};


	//holds a number of pixels to make up one horiz line in an image
	struct st_bmp_code_pix_line
	{
	vector<st_bmp_code_pixel> vpixel;
	};



	//holds a number horiz lines to make up an image
	struct st_img
	{
	vector<st_bmp_code_pix_line> vline;
	};



	bool save_bmp_integer_24bit_colour( string fname, unsigned int *buf, int dx, int dy, int linedx );
	bool save_bmp_uchar_24bit_colour( string fname, unsigned char *buf, int dx, int dy, int linedx );
	bool save_bmp( string fname, st_img &img );


	unsigned int* loadbmp_3_integer_rgb( const char* fname, int &dx, int &dy );									// code that uses Malcolm McLean's bmp routines
	bool savebmp_3_integer_rgb( const char* fname, unsigned int *bufsrc, int dx, int dy );						// code that uses Malcolm McLean's bmp routines
	bool loadbmp_3_int16_t( const char* fname, int16_t **bf_r, int16_t **bf_g, int16_t **bf_b, int &dx, int &dy );	// code that uses Malcolm McLean's bmp routines
	bool savebmp_3_int16_t( const char* fname, int16_t *bf_r, int16_t *bf_g, int16_t *bf_b, int dx, int dy );	// code that uses Malcolm McLean's bmp routines



	// !!!!! seems that 'loadbmp()' is in big endian format, so loaded colour format (on intel) is actually: bb,gg,rr
	// !!!!! when 'savebmp()' is called the colours have to be reversed manually before the call, so they are back in regular format: rr,gg,bb
	// !!!!! suspect the swap is required as code was written for big endian architecture
	//refer https://raw.githubusercontent.com/MalcolmMcLean/babyxrc/master/src/bmp.c

	unsigned char *loadbmp(char *fname, int *width, int *height, bool b_rgb_ordering = 1 );		//MAKE SURE you free the malloc pointer supplied with: 'loadbmp()'
	int savebmp( char *fname, unsigned char *rgb, int width, int height );


	}		//namespace 'bmp_code::'

#endif



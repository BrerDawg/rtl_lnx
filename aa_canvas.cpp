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



//aa_canvas.cpp

//v1.01		25-jul-2023		//simple graphics drawing class with an antialias filtering feature
//v1.02		11-feb-2025		//fixed crash due to incorrect 'cnt' value being used to loading bitmap from 'pp[]' (was including 'kernel_size_in')
//v1.03		07-mar-2026		//changed 'delete pp' entries to 'free(pp)' in 'create_layer_from_bmp_file()', suspect this was causing spurious crash in 'rtl_scan' prj, found by using gcc - flags '-fsanitize=address -fno-omit-frame-pointer'

#include "aa_canvas.h"


aa_canvas::aa_canvas()
{

for( int i = 0; i < cn_aa_canvas_layer_max; i++ )
	{
	st_aa[i].bf = 0;
	st_aa[i].lineclp = new line_clip();
	}

}






aa_canvas::~aa_canvas()
{
bool vb = 0;

for( int i = 0; i < cn_aa_canvas_layer_max; i++ )
	{
if(vb)printf("aa_canvas::~aa_canvas() - deleting layer %d\n", i );
	if( st_aa[i].bf != 0 ) delete[] st_aa[i].bf;
//	st_aa[i].bf = 0;
	}
}






//load an rgb bitmap file and create suitable layer to hold it
//MAKE SURE bitmap is a 3 colour type, even if its monochrome (i.e: 3 bytes per pixel, 24 bit colour)
bool aa_canvas::create_layer_from_bmp_file( string fname, unsigned int layer, en_aa_filter_kernal_type filt_kern_type, unsigned int kernel_size_in )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;


int ww, hh;
unsigned char *pp = bmp_code::loadbmp( (char*)fname.c_str(), &ww, &hh );			// !!! YOU MUST free 'loadbmp()' supplied malloc pointer

printf("aa_wnd::create_pixel_block_from_bmp_file() - '%s'   layer %d  ww %d  hh %d  kernel_size_in %d\n", fname.c_str(), layer, ww, hh, kernel_size_in );

if( pp == 0 )
	{
	printf("aa_wnd::create_pixel_block_from_bmp_file() - failed to read bitmap: '%s'\n", fname.c_str() );
	return 0;
	}

int bytes_per_pixel_in = 3;

st_aa[layer].ww = ww;
st_aa[layer].hh = hh;
st_aa[layer].bytes_per_pixel = bytes_per_pixel_in;
st_aa[layer].linedx = ww;

st_aa[layer].lineclp->clip_vportleft = 8;					//NOTE: upward dir is positive, for: 'line_clip_int()'
st_aa[layer].lineclp->clip_vportright = ww - 8;

st_aa[layer].lineclp->clip_vporttop = hh - 8;				//top limit of plot area, NOTE: upward dir is positive, for: 'line_clip_int()'
st_aa[layer].lineclp->clip_vportbot = 10;					//bottom limit of plot area


st_aa[layer].filter_kernel_type = filt_kern_type;
st_aa[layer].kern_size0 = kernel_size_in;

filter_code::en_filter_window_type_tag wnd_type = filter_code::fwt_blackman_harris;

//printf( "aa_canvas::create_layer_from_bmp_file() - here 2\n" );

if( st_aa[layer].bf != 0 ) delete[] st_aa[layer].bf;

//printf( "aa_canvas::create_layer_from_bmp_file() - here 4\n" );


//ww 1859  hh 1097  kernel_size_in 15

st_aa[layer].bfsiz = (ww + kernel_size_in) * (hh + kernel_size_in) * bytes_per_pixel_in;
//st_aa[layer].bfsiz = (100 + kernel_size_in) * (500 + kernel_size_in) * bytes_per_pixel_in;
st_aa[layer].bf = new unsigned char[ st_aa[layer].bfsiz ];

//unsigned char *bffix = new unsigned char[ st_aa[layer].bfsiz ];

//printf( "aa_canvas::create_layer_from_bmp_file() - here 6\n" );


if( st_aa[layer].bf == 0 )
	{
	printf( "aa_canvas::create_pixel_block_from_bmp_file() - failed to alloc pixel mem for layer %d:  ww %d  h %d  bytes_per_pixel %d\n", layer, ww, hh, bytes_per_pixel_in );

//	delete pp;
	free( pp );			//v1.03	
	return 0;	
	}


int cnt = (ww) * (hh) * bytes_per_pixel_in;								//v1.02, does not include 'kernel_size_in'

//printf( "aa_canvas::create_layer_from_bmp_file() - here 8 cnt %d\n", cnt );

memcpy( st_aa[layer].bf, pp, cnt );


//for( int i = 0; i < cnt; i++ )
//	{
//	st_aa[layer].bf[i] = pp[i];
//	}

//printf( "aa_canvas::create_layer_from_bmp_file() - here 10\n" );

//delete pp;
free( pp );			//v1.03

return 1;
}






//makes a graphics layer using memory storage
bool aa_canvas::create_layer( unsigned int layer, unsigned int ww, unsigned int hh, unsigned int bytes_per_pixel_in, en_aa_filter_kernal_type filt_kern_type, unsigned int kernel_size_in )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;


if( ww > cn_aa_canvas_max_ww ) ww = cn_aa_canvas_max_ww;
if( hh > cn_aa_canvas_max_hh ) hh = cn_aa_canvas_max_hh;

st_aa[layer].ww = ww;
st_aa[layer].hh = hh;
st_aa[layer].bytes_per_pixel = bytes_per_pixel_in;
st_aa[layer].linedx = ww;

st_aa[layer].lineclp->clip_vportleft = 8;					//NOTE: upward dir is positive, for: 'line_clip_int()'
st_aa[layer].lineclp->clip_vportright = ww - 8;

st_aa[layer].lineclp->clip_vporttop = hh - 8;				//top limit of plot area, NOTE: upward dir is positive, for: 'line_clip_int()'
st_aa[layer].lineclp->clip_vportbot = 10;					//bottom limit of plot area


st_aa[layer].filter_kernel_type = filt_kern_type;
st_aa[layer].kern_size0 = kernel_size_in;

filter_code::en_filter_window_type_tag wnd_type = filter_code::fwt_blackman_harris;

//printf( "aa_canvas::create_pixel_block() - here 2\n" );

if( st_aa[layer].bf != 0 ) delete[] st_aa[layer].bf;


//printf( "aa_canvas::create_pixel_block() - here 3\n" );

//printf( "aa_canvas::create_pixel_block() - layer %d:  ww %d  h %d  bytes_per_pixel %d\n", layer, ww, hh, bytes_per_pixel_in );

st_aa[layer].bfsiz = (ww + kernel_size_in) * (hh + kernel_size_in) * bytes_per_pixel_in; 
st_aa[layer].bf = new unsigned char[ st_aa[layer].bfsiz ];

//printf( "aa_canvas::create_pixel_block() - here 5\n" );

if( st_aa[layer].bf == 0 )
	{
	printf( "aa_canvas::create_pixel_block() - failed to alloc pixel mem for layer %d:  ww %d  h %d  bytes_per_pixel %d\n", layer, ww, hh, bytes_per_pixel_in );
	return 0;	
	}

return 1;
}












bool aa_canvas::get_layer_details( unsigned int layer, unsigned int &ww, unsigned int &hh, unsigned int &linedx, unsigned int &bytes_per_pixel )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;

ww = st_aa[layer].ww;
hh = st_aa[layer].hh;
linedx = st_aa[layer].linedx;
bytes_per_pixel = st_aa[layer].bytes_per_pixel;

return 1;
}








bool aa_canvas::set_layer_details( unsigned int layer, unsigned int ww, unsigned int hh, unsigned int linedx, unsigned int bytes_per_pixel )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;

st_aa[layer].ww = ww;
st_aa[layer].hh = hh;
st_aa[layer].linedx = linedx;
st_aa[layer].bytes_per_pixel = bytes_per_pixel;


st_aa[layer].lineclp->clip_vportleft = 8;					//NOTE: upward dir is positive, for: 'line_clip_int()'
st_aa[layer].lineclp->clip_vportright = ww - 8;

st_aa[layer].lineclp->clip_vporttop = hh - 8;				//top limit of plot area, NOTE: upward dir is positive, for: 'line_clip_int()'
st_aa[layer].lineclp->clip_vportbot = 8;					//bottom limit of plot area


return 1;
}






//copy data in supplied buf to layer's buf
bool aa_canvas::block_set( unsigned int layer, unsigned char *bfin, unsigned int xx, unsigned int yy, unsigned int ww, unsigned int hh )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;

if( (ww + xx) > st_aa[layer].ww ) return 0;
if( (hh + yy) > st_aa[layer].hh ) return 0;

if( st_aa[layer].bf == 0 ) return 0;

int bpp = st_aa[layer].bytes_per_pixel;

int linedx = st_aa[layer].linedx * bpp;

/*
int iy0 = 0;
for( int iy = yy; iy < (yy+hh); iy++ )
	{
	int ix0 = 0;
	for( int ix = xx; ix < (xx+ww); ix++ )
		{
		int p0 = iy0 * linedx + ix0*bpp;
		int p1 = iy * linedx + ix*bpp;
		 
		st_aa[layer].bf[ p1 ] = bfin[ p0 ];
		st_aa[layer].bf[ p1+1 ] = bfin[ p0+1 ];
		st_aa[layer].bf[ p1+2 ] = bfin[ p0+2 ];

		ix0 += 1;
		}
	iy0 += 1;
//	printf("ix0 %04d %04d\n", ix0, iy0 );
	}
*/




int p1 = yy * linedx + xx*bpp;			//make offset to src buf

int cnt = ww*hh*bpp;

memcpy( st_aa[layer].bf + p1, bfin, cnt );

return 1;
}











//copy data in supplied buf to layer's buf
bool aa_canvas::block_set_old( unsigned int layer, unsigned char *bfin, unsigned int xx, unsigned int yy, unsigned int ww, unsigned int hh )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;

if( (ww + xx) > st_aa[layer].ww ) return 0;
if( (hh + yy) > st_aa[layer].hh ) return 0;

if( st_aa[layer].bf == 0 ) return 0;

int bpp = st_aa[layer].bytes_per_pixel;

int linedx = st_aa[layer].linedx * bpp;

int iy0 = 0;
for( int iy = yy; iy < (yy+hh); iy++ )
	{
	int ix0 = 0;
	for( int ix = xx; ix < (xx+ww); ix++ )
		{
		int p0 = iy0 * linedx + ix0*bpp;
		int p1 = iy * linedx + ix*bpp;
		 
		st_aa[layer].bf[ p1 ] = bfin[ p0 ];
		st_aa[layer].bf[ p1+1 ] = bfin[ p0+1 ];
		st_aa[layer].bf[ p1+2 ] = bfin[ p0+2 ];

		ix0 += 1;
		}
	iy0 += 1;
//	printf("ix0 %04d %04d\n", ix0, iy0 );
	}

return 1;
}
















bool aa_canvas::block_fill( unsigned int layer, unsigned int xx, unsigned int yy, unsigned int ww, unsigned int hh, uint8_t rr, uint8_t gg, uint8_t bb )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;

if( (ww + xx) > st_aa[layer].ww ) return 0;
if( (hh + yy) > st_aa[layer].hh ) return 0;

if( st_aa[layer].bf == 0 ) return 0;

int bpp = st_aa[layer].bytes_per_pixel;

int linedx = st_aa[layer].linedx * bpp;

for( int iy = yy; iy < (yy+hh); iy++ )
	{
	for( int ix = xx; ix < (xx+ww); ix++ )
		{
		int p1 = iy * linedx + ix*bpp;
		 
		st_aa[layer].bf[ p1 ] = rr;
		st_aa[layer].bf[ p1+1 ] = gg;
		st_aa[layer].bf[ p1+2 ] = bb;
		}
	}

return 1;
}









bool aa_canvas::block_get( unsigned int layer, unsigned char *bfout, unsigned int xx, unsigned int yy, unsigned int ww, unsigned int hh )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;

if( (ww + xx) > st_aa[layer].ww ) return 0;
if( (hh + yy) > st_aa[layer].hh ) return 0;

if( st_aa[layer].bf == 0 ) return 0;

int bpp = st_aa[layer].bytes_per_pixel;

int linedx = st_aa[layer].linedx * bpp;

/*
int iy0 = 0;
for( int iy = yy; iy < (yy+hh); iy++ )
	{
	int ix0 = 0;
	for( int ix = xx; ix < (xx+ww); ix++ )
		{
		int p0 = iy0 * linedx + ix0*bpp;
		int p1 = iy * linedx + ix*bpp;
		 
//		bfout[ p0 ] = st_aa[layer].bf[ p1 ];
//		bfout[ p0+1 ] = st_aa[layer].bf[ p1+1 ];
//		bfout[ p0+2 ] = st_aa[layer].bf[ p1+2 ];

		ix0++;
		}
	iy0++;
	}
*/


int p1 = yy * linedx + xx*bpp;			//make offset to src buf

int cnt = ww*hh*bpp;

memcpy( bfout, st_aa[layer].bf + p1, cnt );

return 1;
}

















bool aa_canvas::block_get_old( unsigned int layer, unsigned char *bfout, unsigned int xx, unsigned int yy, unsigned int ww, unsigned int hh )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;

if( (ww + xx) > st_aa[layer].ww ) return 0;
if( (hh + yy) > st_aa[layer].hh ) return 0;

if( st_aa[layer].bf == 0 ) return 0;

int bpp = st_aa[layer].bytes_per_pixel;

int linedx = st_aa[layer].linedx * bpp;

int iy0 = 0;
for( int iy = yy; iy < (yy+hh); iy++ )
	{
	int ix0 = 0;
	for( int ix = xx; ix < (xx+ww); ix++ )
		{
		int p0 = iy0 * linedx + ix0*bpp;
		int p1 = iy * linedx + ix*bpp;
		 
		bfout[ p0 ] = st_aa[layer].bf[ p1 ];
		bfout[ p0+1 ] = st_aa[layer].bf[ p1+1 ];
		bfout[ p0+2 ] = st_aa[layer].bf[ p1+2 ];

		ix0++;
		}
	iy0++;
	}

return 1;
}







//dimensions are relative to source image
//ENUSRE dest layer is big enough to hold copied block
//dest dimensions are NOT adj
//DOES NOT reshape
bool aa_canvas::block_copy_no_reshape( unsigned int layer_src, unsigned int layer_dest, unsigned int xx, unsigned int yy, unsigned int ww, unsigned int hh, unsigned int destxx, unsigned int destyy )
{
if( layer_src >= cn_aa_canvas_layer_max ) return 0;
if( layer_dest >= cn_aa_canvas_layer_max ) return 0;


if( st_aa[layer_src].bf == 0 ) return 0;
if( st_aa[layer_dest].bf == 0 ) return 0;

int linedx_src = st_aa[layer_src].linedx;
int linedx_dest = st_aa[layer_dest].linedx;

int bytes_pixel_src = st_aa[layer_src].bytes_per_pixel;
int bytes_pixel_dest = st_aa[layer_dest].bytes_per_pixel;

/*
//reshape to suit dest dimensions
int iy0 = destyy;
for( int iy = yy; iy < (yy+hh); iy++ )
	{
	int ix0 = destxx;
	for( int ix = xx; ix < (xx+ww); ix++ )
		{
		int p0 = iy * linedx_src * bytes_pixel_src  +  ix * bytes_pixel_src;
		int p1 = iy0 * linedx_dest * bytes_pixel_dest  +  ix0 * bytes_pixel_dest;
		
		                            st_aa[layer_dest].bf[ p1 ] = st_aa[layer_src].bf[p0];
		if( bytes_pixel_dest >= 2 ) st_aa[layer_dest].bf[ p1+1 ] = st_aa[layer_src].bf[p0+1];
		if( bytes_pixel_dest >= 3 ) st_aa[layer_dest].bf[ p1+2 ] = st_aa[layer_src].bf[p0+2];
		ix0++;
		}
	iy0++;
	}
*/



int p0 = yy * linedx_src + xx*bytes_pixel_src;							//make offset to src buf
int p1 = destyy * linedx_dest + destxx*bytes_pixel_dest;				//make offset to dest buf

int cnt = ww*hh*bytes_pixel_dest;

unsigned char *bfsrc = st_aa[layer_src].bf;
unsigned char *bfdest = st_aa[layer_dest].bf;

memcpy( bfdest + p1, bfsrc + p0, cnt );


return 1;
}










//dimensions are relative to source image
//ENUSRE dest layer is big enough to hold copied block
//dest dimensions are NOT adj
bool aa_canvas::block_copy_old( unsigned int layer_src, unsigned int layer_dest, unsigned int xx, unsigned int yy, unsigned int ww, unsigned int hh, unsigned int destxx, unsigned int destyy )
{
if( layer_src >= cn_aa_canvas_layer_max ) return 0;
if( layer_dest >= cn_aa_canvas_layer_max ) return 0;


if( st_aa[layer_src].bf == 0 ) return 0;
if( st_aa[layer_dest].bf == 0 ) return 0;

int linedx_src = st_aa[layer_src].linedx;
int linedx_dest = st_aa[layer_dest].linedx;

int bytes_pixel_src = st_aa[layer_src].bytes_per_pixel;
int bytes_pixel_dest = st_aa[layer_dest].bytes_per_pixel;

//reshape to suit dest dimensions
int iy0 = destyy;
for( int iy = yy; iy < (yy+hh); iy++ )
	{
	int ix0 = destxx;
	for( int ix = xx; ix < (xx+ww); ix++ )
		{
		int p0 = iy * linedx_src * bytes_pixel_src  +  ix * bytes_pixel_src;
		int p1 = iy0 * linedx_dest * bytes_pixel_dest  +  ix0 * bytes_pixel_dest;
		
		                            st_aa[layer_dest].bf[ p1 ] = st_aa[layer_src].bf[p0];
		if( bytes_pixel_dest >= 2 ) st_aa[layer_dest].bf[ p1+1 ] = st_aa[layer_src].bf[p0+1];
		if( bytes_pixel_dest >= 3 ) st_aa[layer_dest].bf[ p1+2 ] = st_aa[layer_src].bf[p0+2];
		ix0++;
		}
	iy0++;
	}


return 1;
}













//reallocates dest buf, copies pixels and duplicates all details, so it's identical to source
bool aa_canvas::block_duplicate( unsigned int layer_src, unsigned int layer_dest )
{
if( layer_src >= cn_aa_canvas_layer_max ) return 0;
if( layer_dest >= cn_aa_canvas_layer_max ) return 0;


if( st_aa[layer_src].bf == 0 ) return 0;


create_layer( layer_dest, st_aa[layer_src].ww, st_aa[layer_src].hh, st_aa[layer_src].bytes_per_pixel, st_aa[layer_src].filter_kernel_type, st_aa[layer_src].kern_size0 );
block_copy_no_reshape( layer_src, layer_dest, 0, 0, st_aa[layer_src].ww, st_aa[layer_src].hh, 0, 0 );

return 1;
}













/*

//'idx' to cached pixel details within 'vpixpnt[]'
//'scale', e.g: 0.25f for downsampling
//make sure 'st_aa[].bytes_per_pixel' are same value for src and dest
bool aa_canvas::pixel_filter( unsigned int layer_src, unsigned int layer_dest, unsigned int idx, float scale_in, unsigned int kernel_size )
{
if( layer_src >= cn_aa_canvas_layer_max ) return 0;
if( layer_dest >= cn_aa_canvas_layer_max ) return 0;


if( st_aa[layer_src].bf == 0 ) return 0;
if( st_aa[layer_dest].bf == 0 ) return 0;

if( idx >= vpixpnt.size() )  return 0;

st_aa_canvas_pixel_point_tag o = vpixpnt[idx];

//float scale_inv = 1.0f/scale_in;
float fx = o.x0 - kernel_size/2;
float fy = o.y0 - kernel_size/2;
if( fx < 0 ) fx = 0;
if( fy < 0 ) fy = 0;

//printf("o.x0 %03d  %03d\n", o.x0, o.y0 );

int size_srcx;
int size_srcy;
//int size_destx;
//int size_desty;

//float renorm = kernel_size;


unsigned char *bfsrc = st_aa[layer_src].bf;


int wid = st_aa[layer_src].ww;
int hei = st_aa[layer_src].hh;

int linedx_src = st_aa[layer_src].linedx;
int linedx_dest = st_aa[layer_dest].linedx;

int bytes_per_pixel = st_aa[layer_src].bytes_per_pixel;


float *kern00 = st_aa[layer_src].kern00;

if( 1 )
	{
	int pdest = 0;
	bool at_edge_x;
	bool at_edge_y;

	bool done_whgt_sum = 0;
	float wght_sum = 0;

	size_srcx = wid - kernel_size;						//don't scan rightmost and bottom most pixels so as to avoid convolution going past end of src image dimensions
	size_srcy = hei - kernel_size;
	
	int destx = o.x0 * scale_in;
	int desty = o.y0 * scale_in;

int sizex = size_srcx;
int sizey = size_srcy;

	//image scaling - convolve filter and image  (note: the kernel has not been centred over src pixels, so dest would be shifted slightly to the right by 'kern_size')
//	size_desty = 0;


	float sumr = 0;
	float sumg = 0;
	float sumb = 0;
	for( int ky = 0; ky < kernel_size; ky++ )
		{
//fx = 170 - nearbyint(kernel_size/2);

//fy = 170 - nearbyint(kernel_size/2);
		int srcy = nearbyint(fy) + ky;
		
		if (srcy >= sizey) at_edge_y = 1;
		else at_edge_y = 0;

		float fky = kern00[ ky ];

		for( int kx = 0; kx < kernel_size; kx++ )
			{				
			int srcx = nearbyint(fx) + kx;
			
			if (srcx >= sizex) at_edge_x = 1;
			else at_edge_x = 0;

			int psrc = ( srcy*linedx_src*bytes_per_pixel )  +  srcx*bytes_per_pixel;
			
			float fkx = kern00[ kx ];

			if( !done_whgt_sum ) wght_sum += fky*fkx;
			
			float fk = fky*fkx;
			
			unsigned char uc;

//printf("kx %02d  %02d   srcx %d %d  fkx %f  %f rr %d\n", kx, ky, srcx, srcy, fkx, fky, st_aa[layer_src].bf[ psrc ] );
//			psrc = ( fy*linedx_src*bytes_per_pixel )  +  fx*bytes_per_pixel;

//			uc = bfsrc[ psrc + 0 ];								
			bool at_edge = at_edge_x | at_edge_y;
			
			uc = st_aa[layer_src].bf[ psrc ];
			if( at_edge ) uc = 0;				
			sumr += uc*fk;											//mac

//			uc = bfsrc[ psrc + 1 ];				
			uc = st_aa[layer_src].bf[ psrc + 1];
			if( at_edge ) uc = 0;				
			sumg += uc*fk;
			
//			uc = bfsrc[ psrc + 2 ];				
			uc = st_aa[layer_src].bf[ psrc + 2 ];
			if( at_edge ) uc = 0;				
			sumb += uc*fk;

//printf("at_edge %d, sumr %f  %f  %f\n", at_edge, sumr, sumg, sumb );
			}
		
		}

//wght_sum = 1;
	
//	destx *= 0.1;
//	desty *= 0.1;

	pdest = ( desty * linedx_dest * bytes_per_pixel    +    destx * bytes_per_pixel );

//if( pdest >= st_aa[layer_dest].bfsiz )
//	{
//	printf("stopped st_aa[layer_dest].bfsize, st_aa[layer_dest].ww %d %d   destx %d %d\n", st_aa[layer_dest].ww, st_aa[layer_dest].hh, destx, desty );
//	getchar();
//	}

	if( (destx < st_aa[layer_dest].ww ) && (desty < st_aa[layer_dest].hh ) )
		{
		st_aa[layer_dest].bf[ pdest++ ] = sumr / wght_sum;							//place new pixel	
		st_aa[layer_dest].bf[ pdest++ ] = sumg / wght_sum;	
		st_aa[layer_dest].bf[ pdest ] = sumb / wght_sum;
		}
	else{
		printf("aa_canvas::pixel_filter() - exceeded dimensions allocate, ignoring pixel, destx: %03d  %03d, limit is st_aa[layer_dest].ww %d %d\n", destx, desty, st_aa[layer_dest].ww, st_aa[layer_dest].hh );
		}

//printf("destx %03d  %03d  rr %d %d %d\n", destx, desty, (int)(sumr / wght_sum), (int)(sumg / wght_sum), (int)(sumb / wght_sum) );

//if( destx >= 300 )
	{
//	printf("stopped x\n");
//	getchar();
	}

//if( desty >= 140 )
	{
//	printf("stopped y\n");
//	getchar();
	}

	done_whgt_sum = 1;

//	size_destx++;
	}

return 1;
}
*/





/*
bool aa_canvas::filter_kernel_float( en_filter_window_type_tag wnd_type, float *w, int N )
{
int ilow;



for( float n = 0; n < N; n++ )
	{
	int i = (int)n;
	
    
	switch( wnd_type )
		{
		case fwt_rect:
			w[ i ] = 1.0f;                              										//rect window impulse resp
		break;

		case fwt_bartlett:
            ilow = ( N - 1.0f ) / 2.0f ;                        //work out which formula to use (what side of the triangle peak we are in)
            
            if( i <= ilow ) w[ i ] = 2.0f * n / ( N - 1.0f ) ;                                //bartlett/triangle window impulse resp
            else w[ i ] =  2.0f - 2.0f * n / ( N - 1.0f ) ;
		break;

		case fwt_hann:
           w[ i ] = 0.5f * ( 1.0f - cosf( ( twopi * n / ( N - 1.0f ) ) ) );                     //hann window impulse resp
		break;

		case fwt_bartlett_hanning:
            w[ i ] = 0.62f - 0.48f * fabsf( n / ( N - 1.0f ) - 0.5f ) + 0.38f * cosf( ( twopi * ( n / ( N - 1.0f ) - 0.5f ) ) );        //bartlett-hanning window impulse resp
		break;

		case fwt_hamming:
            w[ i ] = 0.54f - 0.46f * cosf( twopi * n / ( N - 1.0f ) );            //hamming window impulse resp, note the text in  http://www.mikroe.com.. listed above is slightly incorrect, used wikipedia version
		break;

		case fwt_blackman:
            w[ i ] = 0.42f - 0.5f * cosf( twopi * n / ( N - 1.0 ) ) + 0.08f * cosf( 2.0f * twopi * n / ( N - 1.0f ) );        //blackman window impulse resp
		break;

		case fwt_blackman_harris:
            w[ i ] = 0.35875f - 0.48829f * cosf( twopi * n / ( N - 1.0f ) ) + 0.14128f * cosf( 2.0f * twopi * n / ( N - 1.0f ) ) - 0.01168f * cosf( 3.0f * twopi * n / ( N - 1.0f ) );        //blackman-harris window impulse resp
		break;

		default:
			printf( "aa_canvas::filter_kernel() - unknown filter window type(wnd): %u\n", wnd_type );
			return 0;
		break;
        }
	}
	
return 1;
}
*/





/*
//See also 'filter_scale_block_layer()'
bool aa_canvas::filter_scale_block( int layer, unsigned char *bfsrc, int srcx_in, int srcy_in, int wid, int hei, int linedx, unsigned char *bfdest, int destx, int desty, int destlinedx, float scale_in, unsigned int kernel_size, int bytes_per_pixel_in, int &rendered_wid, int &rendered_hei )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;

kern_size0 = kernel_size;
int kern_size = kern_size0;

en_filter_window_type_tag wnd_type = fwt_blackman_harris;

filter_kernel_float( wnd_type, kern00, kern_size );

float scale_inv = 1.0f/scale_in;
float fx = 0;
float fy = 0;
int size_srcx;
int size_srcy;
int size_destx;
int size_desty;

float renorm = kern_size;




if( 1 )
	{
	int pdest = 0;
	bool at_edge;

	bool done_whgt_sum = 0;
	float wght_sum = 0;

	size_srcx = wid - kern_size;						//don't scan rightmost and bottom most pixels so as to avoid convolution going past end of src image dimensions
	size_srcy = hei - kern_size;
	size_destx = 0;
	size_desty = 0;

int sizex = size_srcx;
int sizey = size_srcy;

	//image scaling - convolve filter and image  (note: the kernel has not been centred over src pixels, so dest would be shifted slightly to the right by 'kern_size')
	size_desty = 0;
	for( fy = 0; fy < size_srcy; fy += scale_inv )
		{
		size_destx = 0;
		for( fx = 0; fx < size_srcx; fx += scale_inv )
			{
			float sumr = 0;
			float sumg = 0;
			float sumb = 0;
			for( int ky = 0; ky < kern_size; ky++ )
				{
				int srcy = nearbyint(fy) + ky;
				
				if (srcy >= sizey) at_edge = 1;
				else at_edge = 0;

				float fky = kern00[ ky ];

				for( int kx = 0; kx < kern_size; kx++ )
					{				
					int srcx = nearbyint(fx) + kx;
					
					if (srcx >= sizex) at_edge = 1;
					else at_edge = 0;

					int psrc = ( (srcy_in + srcy)*linedx*bytes_per_pixel_in )  +  (srcx_in + srcx)*bytes_per_pixel_in;
					
					float fkx = kern00[ kx ];

					if( !done_whgt_sum ) wght_sum += fky*fkx;
					
					float fk = fky*fkx;
					
					unsigned char uc;

					uc = bfsrc[ psrc + 0 ];								
					if( at_edge ) uc = 0;				
					sumr += uc*fk;											//mac

					uc = bfsrc[ psrc + 1 ];				
					if( at_edge ) uc = 0;				
					sumg += uc*fk;
					
					uc = bfsrc[ psrc + 2 ];				
					if( at_edge ) uc = 0;				
					sumb += uc*fk;
					}
				
				}
			
			pdest = ( ( desty + size_desty ) * ( destlinedx ) * bytes_per_pixel_in    +    ( destx + size_destx )* bytes_per_pixel_in );

			bfdest[ pdest++ ] = sumr / wght_sum;							//place new pixel	
			bfdest[ pdest++ ] = sumg / wght_sum;	
			bfdest[ pdest ] = sumb / wght_sum;

			done_whgt_sum = 1;

			size_destx++;
			}

		rendered_wid = size_destx;
		
//printf( "size_destx: %d\n", size_destx );

		size_desty++;
		}
	
	rendered_hei = size_desty;
	}
return 1;
}
*/









float aa_canvas::bitmap_sinx_on_x(float x)
{
if (x != 0)
	{
	x *= ((float)M_PI);
	return ( sinf(x)/x);
	}
return 1;
}











/*
//LESS efficient, convolves dest pixels many times - DON'T USE
//scales down the WHOLE of the src image, 'scale_in' < 1.0   - SEE ALSO 'block_filter_partial_scale_layer()'
//this WILL change the geometry of the dest image
bool aa_canvas::block_filter_scale_layer_old( unsigned int layer_src, int srcx_in, int srcy_in, unsigned int layer_dest, int destx, int desty, float scale_in, unsigned int kernel_size, int bytes_per_pixel_in, int &rendered_wid, int &rendered_hei )
{
if( layer_src >= cn_aa_canvas_layer_max ) return 0;
if( layer_dest >= cn_aa_canvas_layer_max ) return 0;

if( st_aa[layer_src].bf == 0 ) return 0;
if( st_aa[layer_dest].bf == 0 ) return 0;

if( scale_in > 1.0f ) return 0;

int kern_size0 = st_aa[layer_src].kern_size0;
int kern_size = kern_size0;

//en_filter_window_type_tag wnd_type = fwt_blackman_harris;

//filter_kernel_float( wnd_type, kern00, kern_size );

float scale_inv = 1.0f/scale_in;
float fx = 0;
float fy = 0;
int size_srcx;
int size_srcy;
int size_destx;
int size_desty;

float renorm = kern_size;

int wid = st_aa[layer_src].ww;
int hei = st_aa[layer_src].hh;

int linedx = st_aa[layer_src].linedx;

unsigned char *bfsrc = st_aa[layer_src].bf;
unsigned char *bfdest = new unsigned char[ wid * hei * bytes_per_pixel_in];		//make a tmp buf to render into


float* kern00 = st_aa[layer_src].kern00;

if( 1 )
	{
	int pdest = 0;
	bool at_edge;

	bool done_whgt_sum = 0;
	float wght_sum = 0;

	size_srcx = wid - kern_size;						//don't scan rightmost and bottom most pixels so as to avoid convolution going past end of src image dimensions
	size_srcy = hei - kern_size;
	size_destx = 0;
	size_desty = 0;

int sizex = size_srcx;
int sizey = size_srcy;

	//image scaling - convolve filter and image  (note: the kernel has not been centred over src pixels, so dest would be shifted slightly to the right by 'kern_size')
	size_desty = 0;
	for( fy = 0; fy < size_srcy; fy += scale_inv )
		{
		size_destx = 0;
		for( fx = 0; fx < size_srcx; fx += scale_inv )
			{
			float sumr = 0;
			float sumg = 0;
			float sumb = 0;
			for( int ky = 0; ky < kern_size; ky++ )
				{
				int srcy = nearbyint(fy) + ky;
				
				if (srcy >= sizey) at_edge = 1;
				else at_edge = 0;

				float fky = kern00[ ky ];

				for( int kx = 0; kx < kern_size; kx++ )
					{				
					int srcx = nearbyint(fx) + kx;
					
					if (srcx >= sizex) at_edge = 1;
					else at_edge = 0;

					int psrc = ( (srcy_in + srcy)*linedx*bytes_per_pixel_in )  +  (srcx_in + srcx)*bytes_per_pixel_in;
					
					float fkx = kern00[ kx ];

					if( !done_whgt_sum ) wght_sum += fky*fkx;
					
					float fk = fky*fkx;
					
					unsigned char uc;

					uc = bfsrc[ psrc + 0 ];								
					if( at_edge ) uc = 0;				
					sumr += uc*fk;											//mac

					uc = bfsrc[ psrc + 1 ];				
					if( at_edge ) uc = 0;				
					sumg += uc*fk;
					
					uc = bfsrc[ psrc + 2 ];				
					if( at_edge ) uc = 0;				
					sumb += uc*fk;
					}
				}
			
			pdest = ( ( desty + size_desty ) * ( linedx ) * bytes_per_pixel_in    +    ( destx + size_destx )* bytes_per_pixel_in );

			bfdest[ pdest++ ] = sumr / wght_sum;							//place new pixel	
			bfdest[ pdest++ ] = sumg / wght_sum;	
			bfdest[ pdest ] = sumb / wght_sum;

			done_whgt_sum = 1;

			size_destx++;
			}

		rendered_wid = size_destx;
		
//printf( "size_destx: %d\n", size_destx );

		size_desty++;
		}
	
	rendered_hei = size_desty;
	}



//reshape to suit new size of image
int iy0 = 0;
for( int iy = 0; iy < rendered_hei; iy++ )
	{
	int ix0 = 0;
	for( int ix = 0; ix < rendered_wid; ix++ )
		{
		int p0 = iy * linedx * bytes_per_pixel_in  +  ix * bytes_per_pixel_in;
		int p1 = iy0 * rendered_wid * bytes_per_pixel_in  +  ix0 * bytes_per_pixel_in;
		st_aa[layer_dest].bf[ p1 ] = bfdest[p0];
		st_aa[layer_dest].bf[ p1+1 ] = bfdest[p0+1];
		st_aa[layer_dest].bf[ p1+2 ] = bfdest[p0+2];
		ix0++;
		}
	iy0++;
	}

st_aa[layer_dest].ww = rendered_wid;
st_aa[layer_dest].hh = rendered_hei;
st_aa[layer_dest].linedx = rendered_wid;

delete[] bfdest;
return 1;
}
*/









#pragma GCC optimize("O3")		//this improves below code's performance, speeds up execution


//more EFFICIENT, convolves dest pixels only once
//MAKE SURE 'layer_dest' has a big enough buffer if scaling up, as no 'layer_dest' memory is allocated
//scales the whole of the src image, - SEE ALSO 'block_filter_partial_scale_layer()'
//this WILL change the geometry of the dest image
bool aa_canvas::block_filter_scale_layer( unsigned int layer_src, int srcx_in, int srcy_in, unsigned int layer_dest, float destx, float desty, float scale_in, int bytes_per_pixel_in, unsigned int &rendered_wid, unsigned int &rendered_hei )
{
if( layer_src >= cn_aa_canvas_layer_max ) return 0;
if( layer_dest >= cn_aa_canvas_layer_max ) return 0;

if( st_aa[layer_src].bf == 0 ) return 0;
if( st_aa[layer_dest].bf == 0 ) return 0;

//if( scale_in > 1.0f ) return 0;

int filt_kernel_type = st_aa[layer_dest].filter_kernel_type;

int kern_size = st_aa[layer_dest].kern_size0;;


//kern_size = 15;


float scale = scale_in;
float scale_inv = 1.0f/scale;
float fx = 0;
float fy = 0;
int size_srcx;
int size_srcy;
int size_destx;
int size_desty;


int wid = st_aa[layer_src].ww;
int hei = st_aa[layer_src].hh;

//printf( "aa_canvas::block_filter_partial_scale_layer) srcx_in %d %d   ww %d %d   destx %f %f\n", srcx_in, srcy_in, wid, hei, destx, desty );

float dint;
float dest_fract_x = modff( destx, &dint );								//get fractional and integer parts of dest pos
int idestx = dint;

float dest_fract_y = modff( desty, &dint );
int idesty = dint;


int linedx = st_aa[layer_src].linedx;
int destlinedx = st_aa[layer_dest].linedx;
int bytes_per_pixel_src = st_aa[layer_src].bytes_per_pixel;
int bytes_per_pixel_dest = st_aa[layer_dest].bytes_per_pixel;

int dest_linedx;

unsigned char *bfsrc;
unsigned char *bfdest;

float* kern00 = st_aa[layer_src].kern00;

rendered_wid = 0;
rendered_hei = 0;




//float rot_theta = 0.0f;

//float theta = twopi * (-rot_theta/360.0f);

//float cos0 = cosf( theta );
//float sin0 = sinf( theta );


if( 1 )
	{
	int pdest = 0;
	bool at_edgex, at_edgey;

	bool done_whgt_sum = 0;
	float wght_sum = 0;

	size_srcx = wid - kern_size;						//don't scan rightmost and bottom most pixels so as to avoid convolution going past end of src image dimensions
	size_srcy = hei - kern_size;

	size_srcx = wid;
	size_srcy = hei;
	size_destx = 0;
	size_desty = 0;

//	int sizex = size_srcx + kern_size;	//scan past wid/hei dim by size of kernel, assuming wid/hei are less than src image dim, see directly below which handles possibility of going past src image dim
//	int sizey = size_srcy + kern_size;

//	if( sizex >= (st_aa[layer_src].ww - kern_size) ) sizex = st_aa[layer_src].ww - kern_size; //don't scan rightmost and bottom most pixels so as to avoid convolution going past end of src image dimensions
//	if( sizey >= (st_aa[layer_src].hh - kern_size) ) sizey = st_aa[layer_src].hh - kern_size;


	float fractiony = 0.0;


	size_destx = (size_srcx * scale);
	size_desty = (size_srcy * scale);

dest_linedx = size_destx;

	bfsrc = st_aa[layer_src].bf;
	bfdest = new unsigned char[ size_destx * size_desty * bytes_per_pixel_src ];		//make a tmp buf to render into, set it to a max of src image size even though only a portion of it gets used by scaled down block (only uses: wid*scale, hei*scale)


	bool kern_x_built = 0;
	float *kern_x = new float[ size_destx * kern_size ];				//used to cache kernel vals for first line of x pixels



//printf( "aa_canvas::block_filter_partial_scale_layer) size_destx %d %d\n", size_destx, size_desty );

//return 1;

//	float kern_fraction;
	
//	size_desty = 0;	
	//image scaling - convolve filter and image  (note: the kernel may not be centered over src pixels, so dest would maybe be shifted slightly to the right by 'kern_size')
	for( int iy = 0; iy < size_desty; iy++ )
		{
		float fy = srcy_in + iy * scale_inv;// - kern_size/2.0f;			//form src coord using dest coord

//if( fy < 0 ) fy = 0;

		double dintx, dinty;



		
//		size_destx = 0;


		int kern_x_idx = 0;
		for( int ix = 0; ix < size_destx; ix++ )
			{
			float fx = srcx_in + ix * scale_inv;// - kern_size/2.0f;		//form src coord using dest coord
//printf("fx %f %f\n", fx, fy );
//if( fx < 0 ) fx = 0;

			float fry = fy;
			float frx = fx;

/*
float theta = 0.0f;

//if( fabsf(theta) > 0.001 )
if( 0 )
	{

	float fy0 = fy - size_srcy/2;
	float fx0 = fx - size_srcx/2;

	frx = fx0 * cos0 - fy0 * sin0;
	fry = fy0 * cos0 + fx0 * sin0;




	fry = fry + size_srcy/2;
	frx = frx + size_srcx/2;

	if( fry >= (size_srcy-1) ) fry = size_srcy-1;
	if( fry < 0 ) fry = 0;

	if( frx >= (size_srcy-1) ) frx = size_srcy-1;
	if( frx < 0 ) frx = 0;
	}	
*/


//			float fractiony = modf( fry, &dinty );

//			float fractionx = modf( frx, &dintx );

			float fractiony = modf( fry, &dinty ) - dest_fract_y;		//include the fractional position of dest

			float fractionx = modf( frx, &dintx ) - dest_fract_x;
	
//	dintx = round( dintx );
//			fraction = fraction;


//			int filter_subsample_offsetx = fractionx*pixel_subsample_factor;			//slip filter impulse response to sit over a fractional pixel position (allows subpixel positioning)





//		printf("size_srcx %d, fx %f\n", size_srcx, fx );
			wght_sum = 0;
			float sumr = 0;
			float sumg = 0;
			float sumb = 0;
			int kx = 0;
			for( int ky = 0; ky < kern_size; ky++ )
				{
//				float fky = bitmap_rotate_kern0[ filter_center_idx + filter_subsample_offsety ];

				float res;
//				float radius, x;
//				radius = 1.0f;

//				x = ( ((ky-kern_size/2)/(float)kern_size - fractiony*(1.0f/kern_size) ) ) * radius;//(float)n/(N-1)*(2*radius);
//				if (x < 0) x = -x;

					
//					if( filt_kernel_type == 0 )
//						{
//						float radius = 1.0f;
//						float x = 0.0f;
//						if (x < (radius) ) res = ( bitmap_sinx_on_x( x ) * bitmap_sinx_on_x( x / radius ) );	//lanczos with radius
//						else res = 0.0f;
//						}
					if( filt_kernel_type == en_afkt_linear )
						{
						res  = fractiony;
						}


					if( filt_kernel_type == en_afkt_blackman_harris )
						{
						int n = ky;
						int N = kern_size;
						
						float kern_fraction = n / ( N - 1.0f ) - fractiony * (1.0f/(kern_size-1));
						float x = twopi * kern_fraction;
						res = 0.35875f - 0.48829f * cosf( x ) + 0.14128f * cosf( 2.0f * x ) - 0.01168f * cosf( 3.0f * x );        //blackman-harris window impulse resp
						}

				float fky = res;

//				printf("fx %f %f ky %d fraction %f filtx %f filter_subsample_offsety %d  fky %f\n", fx, fy, ky, fractiony, x, filter_subsample_offsety, fky );

				int srcy = dinty + ky;
				if (srcy >= size_srcy) at_edgey = 1;
				else at_edgey = 0;


//printf( " frx %f  dintx %f  fraction %f  filter_subsample_offsetx %d\n", frx, dintx, fraction, filter_subsample_offsetx );
				for( kx = 0; kx < kern_size; kx++ )
					{
//					int srcx = nearbyint(ix * scale_inv) + kx;

//					float fkx = bitmap_rotate_kern0[ filter_center_idx + filter_subsample_offsetx ];

//					x = ( ((kx-kern_size/2)/(float)kern_size - fractionx*(1.0f/kern_size) ) ) * radius;//(float)n/(N-1)*(2*radius);
//					if (x < 0) x = -x;

					
//					if( filt_kernel_type == 0 )
//						{
//						if (x < (radius) ) res = ( bitmap_sinx_on_x( x ) * bitmap_sinx_on_x( x / radius ) );	//lanczos with radius
//						else res = 0.0f;
//						}




//	if( ( fractionx > 0.6f ) && ( fractionx < 0.9f) )  res = 2.0;
//if( res < 0.0f ) res = 0;

					
					
						
					if( !kern_x_built )								//first x line of pixels?, build 'x' kernel values for a horiz line of pixels, put into array for all other x pixel line calcs
						{
						if( filt_kernel_type == en_afkt_linear )
							{
							res  = fractionx;
							kern_x[ ix * kern_size + kx ] = res;		//store kernel val for successive x lines
							}
							
						if( filt_kernel_type == en_afkt_blackman_harris )
							{
							int n = kx;
							int N = kern_size;

							float kern_fraction = n / ( N - 1.0f ) - fractionx * (1.0f/(kern_size-1));
							float x = twopi * kern_fraction;

							res = 0.35875f - 0.48829f * cosf( x ) + 0.14128f * cosf( 2.0f * x ) - 0.01168f * cosf( 3.0f * x );        //blackman-harris window impulse resp
							
//printf( "kern_x_idx %d (%d)\n", kern_x_idx, size_desty * size_destx * kern_size );
							kern_x[ ix * kern_size + kx ] = res;		//store kernel val for successive x lines
							}		
						}
					else{
						res = kern_x[ ix * kern_size + kx ];		//if already built kernel vals, use them
						}
							
	//					if (n==1 ) res *= 0.25;
	//					if (n==3 ) res *= 0.25;

	//res = bitmap_rotate_kern0[n];
						

//					kern_x_idx++;
					float fkx = res;



//		if( iy == 0 )printf("fx %f %f kx %d fraction %f filtx %f filter_subsample_offsetx %d  fkx %f\n", fx, fy, kx, fractionx, x, filter_subsample_offsetx, fkx );
					
					int srcx = dintx + kx;

					if (srcx >= size_srcx) at_edgex = 1;
					else at_edgex = 0;

					bool at_edge = at_edgey | at_edgex;

					int psrc = (srcy * linedx*bytes_per_pixel_src)  +  srcx*bytes_per_pixel_src;
					

//		printf("size_srcx %d %d, srcx %d %d fkx %f %f\n", size_srcx, size_srcy, srcx, srcy, fkx, fky );
//					if( !done_whgt_sum ) wght_sum += fky*fkx;
					wght_sum += fky*fkx;
					float fk = fky*fkx;
//if( iy == 0 )printf("%d: fk %f\n", kx, fk );					
					unsigned char uc;

					uc = bfsrc[ psrc++ ];								
					if( at_edge ) uc = 0;				
					sumr += uc*fk;										//mac

					uc = bfsrc[ psrc++ ];				
					if( at_edge ) uc = 0;				
					sumg += uc*fk;
					
					uc = bfsrc[ psrc ];				
					if( at_edge ) uc = 0;				
					sumb += uc*fk;
					}
				}

			sumr = sumr / wght_sum;
			sumg = sumg / wght_sum;	
			sumb = sumb / wght_sum;

/*			
			if( sumr < 0 ) sumr = 0;									//need this for kernels that go below zero, such as lanczos
			if( sumg < 0 ) sumg = 0;
			if( sumb < 0 ) sumb = 0;
			if( sumr > 255 ) sumr = 255;
			if( sumg > 255 ) sumg = 255;
			if( sumb > 255 ) sumb = 255;
*/

			pdest = ( iy ) * ( dest_linedx ) * bytes_per_pixel_src    +    ( ix * bytes_per_pixel_src );
			
			bfdest[ pdest++ ] = sumr;							//place new pixel	
			bfdest[ pdest++ ] = sumg;	
			bfdest[ pdest ] = sumb;
//printf("wght_sum %f\n", wght_sum );				
//			pdest += cn_bytes_per_pixel;
//			done_whgt_sum = 1;

//			size_destx++;

//			rendered_wid = size_destx;
			}
		kern_x_built = 1;												//flag that have done x kernal calcs for first x line of pixels

//		size_desty++;
//		rendered_hei = size_desty;
		}

	delete[] kern_x;
	}
	
rendered_wid = size_destx;
rendered_hei = size_desty;


//if( rendered_wid >= st_aa[layer_dest].ww ) rendered_wid = st_aa[layer_dest].ww;
//if( rendered_hei >= st_aa[layer_dest].hh ) rendered_hei = st_aa[layer_dest].hh;



//reshape to suit new size of image
int iy0 = 0;
for( int iy = 0; iy < rendered_hei; iy++ )
	{
	int ix0 = 0;
	for( int ix = 0; ix < rendered_wid; ix++ )
		{
		int p0 = iy * dest_linedx * bytes_per_pixel_in  +  ix * bytes_per_pixel_in;
		int p1 = iy0 * rendered_wid * bytes_per_pixel_in  +  ix0 * bytes_per_pixel_in;
		st_aa[layer_dest].bf[ p1++ ] = bfdest[p0++];
		st_aa[layer_dest].bf[ p1++ ] = bfdest[p0++];
		st_aa[layer_dest].bf[ p1 ] = bfdest[p0];
		ix0++;
		}
	iy0++;
	}

st_aa[layer_dest].ww = rendered_wid;
st_aa[layer_dest].hh = rendered_hei;
st_aa[layer_dest].linedx = rendered_wid;

delete[] bfdest;
return 1;
}

#pragma GCC pop_options












/*
 
//LESS efficient, convolves dest pixels many times - DON'T USE
//scales down a PORTION of the src image, 'scale_in' < 1.0     - SEE ALSO 'block_filter_scale_layer()'
//does NOT change dest layer geometry
//MAKE SURE dest geometry can hold scaled down image bock, particulary its 'linedx' should equal scaled down image block's effective linedx
bool aa_canvas::block_filter_partial_scale_layer_old( unsigned int layer_src, int srcx_in, int srcy_in, int wid, int hei, unsigned int layer_dest, int destx, int desty, float scale_in )
{
if( layer_src >= cn_aa_canvas_layer_max ) return 0;
if( layer_dest >= cn_aa_canvas_layer_max ) return 0;

if( st_aa[layer_src].bf == 0 ) return 0;
if( st_aa[layer_dest].bf == 0 ) return 0;

if( scale_in > 1.0f ) return 0;

int kern_size0 = st_aa[layer_src].kern_size0;
int kern_size = kern_size0;


//return 0;



//printf( "aa_canvas::block_filter_partial_scale_layer) x0 %d %d   ww %d %d   destx %d %d\n", srcx_in, srcy_in, wid, hei, destx, desty );

//en_filter_window_type_tag wnd_type = fwt_blackman_harris;

//filter_kernel_float( wnd_type, kern00, kern_size );

float scale_inv = 1.0f/scale_in;
float fx = 0;
float fy = 0;
int size_srcx;
int size_srcy;
int size_destx;
int size_desty;

float renorm = kern_size;

int linedx = st_aa[layer_src].linedx;
int destlinedx = st_aa[layer_dest].linedx;
int bytes_per_pixel_src = st_aa[layer_src].bytes_per_pixel;
int bytes_per_pixel_dest = st_aa[layer_dest].bytes_per_pixel;


unsigned char *bfsrc = st_aa[layer_src].bf;
unsigned char *bfdest = new unsigned char[ st_aa[layer_src].ww * st_aa[layer_src].hh * bytes_per_pixel_src ];		//make a tmp buf to render into, set it to a max of src image size even though only a portion of it gets used by scaled down block (only uses: wid*scale, hei*scale)


float* kern00 = st_aa[layer_src].kern00;

int rendered_wid = 0;
int rendered_hei = 0;

if( 1 )
	{
	int pdest = 0;
	bool at_edge;

	bool done_whgt_sum = 0;
	float wght_sum = 0;

	size_srcx = wid;
	size_srcy = hei;
	size_destx = 0;
	size_desty = 0;

int sizex = size_srcx + kern_size;	//scan past wid/hei dim by size of kernel, assuming wid/hei are less than src image dim, see directly below which handles possibility of going past src image dim
int sizey = size_srcy + kern_size;

if( sizex >= (st_aa[layer_src].ww - kern_size) ) sizex = st_aa[layer_src].ww - kern_size; //don't scan rightmost and bottom most pixels so as to avoid convolution going past end of src image dimensions
if( sizey >= (st_aa[layer_src].hh - kern_size) ) sizey = st_aa[layer_src].hh - kern_size;


float fractiony = 0.0;

	//image scaling - convolve filter and image  (note: the kernel has not been centred over src pixels, so dest would be shifted slightly to the right by 'kern_size')
	size_desty = 0;
	for( fy = 0; fy < size_srcy; fy += scale_inv )
		{
		size_destx = 0;
		for( fx = 0; fx < size_srcx; fx += scale_inv )
			{
			float sumr = 0;
			float sumg = 0;
			float sumb = 0;
			for( int ky = 0; ky < kern_size; ky++ )
				{
				int srcy = nearbyint(fy) + ky;
				
				if (srcy >= sizey) at_edge = 1;
				else at_edge = 0;

				float fky = kern00[ ky ];

				for( int kx = 0; kx < kern_size; kx++ )
					{				


//-----
//int filt_kernel_type = 1;
//
//					float res = 0.0f;
//					
//					if( filt_kernel_type == 0 )
//						{
//						float radius = 1.0f;
//						float x = 0.0f;
//						if (x < (radius) ) res = ( bitmap_sinx_on_x( x ) * bitmap_sinx_on_x( x / radius ) );	//lanczos with radius
//						else res = 0.0f;
//						}


//					if( filt_kernel_type == 1 )
//						{
//						int n = ky;
//						int N = kern_size0;
						
//						float kern_fraction = n / ( N - 1.0 ) - fractiony * (1.0f/(kern_size0-1));
//						float x = twopi * kern_fraction;
//						res = 0.35875 - 0.48829 * cos( x ) + 0.14128 * cos( 2.0 * x ) - 0.01168 * cos( 3.0 * x );        //blackman-harris window impulse resp
//						}
//-----



					int srcx = nearbyint(fx) + kx;
					
					if (srcx >= sizex) at_edge = 1;
					else at_edge = 0;

					int psrc = ( (srcy_in + srcy)*linedx*bytes_per_pixel_src )  +  (srcx_in + srcx)*bytes_per_pixel_src;
					
					float fkx = kern00[ kx ];

					if( !done_whgt_sum ) wght_sum += fky*fkx;
					
					float fk = fky*fkx;
					
					unsigned char uc;
					
					uc = bfsrc[ psrc++ ];								
					if( at_edge ) uc = 0;				
					sumr += uc*fk;										//mac

					uc = bfsrc[ psrc++ ];				
					if( at_edge ) uc = 0;				
					sumg += uc*fk;
					
					uc = bfsrc[ psrc ];				
					if( at_edge ) uc = 0;				
					sumb += uc*fk;
					}
				}
			


			pdest = ( size_desty ) * ( linedx ) * bytes_per_pixel_src    +    ( size_destx * bytes_per_pixel_src );

			bfdest[ pdest++ ] = sumr / wght_sum;						//place new pixel	
			bfdest[ pdest++ ] = sumg / wght_sum;	
			bfdest[ pdest ] = sumb / wght_sum;

			done_whgt_sum = 1;

			size_destx++;
			}

		rendered_wid = size_destx;
		

		size_desty++;
		}
	
	rendered_hei = size_desty;
	}


if( rendered_wid >= st_aa[layer_dest].ww ) rendered_wid = st_aa[layer_dest].ww;
if( rendered_hei >= st_aa[layer_dest].hh ) rendered_hei = st_aa[layer_dest].hh;


//reshape geometery to suit dest geometry
int iy0 = 0;
for( int iy = 0; iy < rendered_hei; iy++ )
	{
	int ix0 = 0;
	for( int ix = 0; ix < rendered_wid; ix++ )
		{
		int p0 = iy * linedx * bytes_per_pixel_src  +  ix * bytes_per_pixel_src;
		int p1 = (desty + iy0) * destlinedx * bytes_per_pixel_dest  +  (destx + ix0) * bytes_per_pixel_dest;

//printf( "reshape geometery   %d %d p0 p1 %d %d \n", st_aa[layer_src].bfsiz + p0, st_aa[layer_dest].bfsiz + p1, p0, p1 );
		st_aa[layer_dest].bf[ p1++ ] = bfdest[p0++];
		st_aa[layer_dest].bf[ p1++ ] = bfdest[p0++];
		st_aa[layer_dest].bf[ p1 ] = bfdest[p0];
		ix0++;
		}
	iy0++;
	}

//st_aa[layer_dest].ww = rendered_wid;
//st_aa[layer_dest].hh = rendered_hei;
//st_aa[layer_dest].linedx = rendered_wid;

delete[] bfdest;
return 1;
}
*/


























bool aa_canvas::block_filter_partial_scale_layer_rect( unsigned int layer_src, st_aa_canvas_bounding_rect_tag &vr, unsigned int layer_dest, float destx, float desty, float scale_in, unsigned int &rendered_wid, unsigned int &rendered_hei )
{

int srcx_in = vr.x0;
int srcy_in = vr.y0;

int wid = vr.x1 - vr.x0;
int hei = vr.y1 - vr.y0;

return block_filter_partial_scale_layer( layer_src, srcx_in, srcy_in, wid, hei, layer_dest, destx, desty, scale_in, rendered_wid, rendered_hei );
}











#pragma GCC optimize("O3")		//this improves below code's performance, speeds up execution

 
//more EFFICIENT, convolves dest pixels only once
//MAKE SURE 'layer_dest' has a big enough buffer if scaling up, as no 'layer_dest' memory is allocated     - SEE ALSO 'block_filter_scale_layer()'
//does NOT change dest layer geometry
//does NOT check if convolution of src pixels with kernel will overshoot end of available src pixels, i.e. don't allow 'srcx_in+wid' and 'srcy_in+hei' to get to close right and bottom edges of avail src pixel block, allow a 'kern_size' margin
//MAKE SURE dest geometry can hold scaled down image bock, particulary its 'linedx' should equal scaled down image block's effective linedx
bool aa_canvas::block_filter_partial_scale_layer( unsigned int layer_src, int srcx_in, int srcy_in, int wid, int hei, unsigned int layer_dest, float destx, float desty, float scale_in, unsigned int &rendered_wid, unsigned int &rendered_hei )
{
if( layer_src >= cn_aa_canvas_layer_max ) return 0;
if( layer_dest >= cn_aa_canvas_layer_max ) return 0;

if( st_aa[layer_src].bf == 0 ) return 0;
if( st_aa[layer_dest].bf == 0 ) return 0;

if( scale_in > 1.0f ) return 0;

int kern_size0 = st_aa[layer_src].kern_size0;
int kern_size = kern_size0;


//kern_size = 15;



//printf( "aa_canvas::block_filter_partial_scale_layer) x0 %d %d   ww %d %d   destx %d %d\n", srcx_in, srcy_in, wid, hei, destx, desty );

//en_filter_window_type_tag wnd_type = fwt_blackman_harris;

//filter_kernel_float( wnd_type, kern00, kern_size );


float scale = scale_in;
float scale_inv = 1.0f/scale;
float fx = 0;
float fy = 0;
int size_srcx;
int size_srcy;
int size_destx;
int size_desty;


float dint;
float dest_fract_x = modff( destx, &dint );								//get fractional and integer parts of dest pos
int idestx = dint;

float dest_fract_y = modff( desty, &dint );
int idesty = dint;


int linedx = st_aa[layer_src].linedx;
int destlinedx = st_aa[layer_dest].linedx;
int bytes_per_pixel_src = st_aa[layer_src].bytes_per_pixel;
int bytes_per_pixel_dest = st_aa[layer_dest].bytes_per_pixel;


unsigned char *bfsrc = st_aa[layer_src].bf;
unsigned char *bfdest = new unsigned char[ st_aa[layer_src].ww * st_aa[layer_src].hh * bytes_per_pixel_src ];		//make a tmp buf to render into, set it to a max of src image size even though only a portion of it gets used by scaled down block (only uses: wid*scale, hei*scale)


float* kern00 = st_aa[layer_src].kern00;

rendered_wid = 0;
rendered_hei = 0;


//float rot_theta = 0.0f;

//float theta = twopi * (-rot_theta/360.0f);

//float cos0 = cosf( theta );
//float sin0 = sinf( theta );

int filt_kernel_type = 1;

if( 1 )
	{
	int pdest = 0;
//	bool at_edgex, at_edgey;

	bool done_whgt_sum = 0;
	float wght_sum = 0;

	size_srcx = wid;
	size_srcy = hei;
	size_destx = 0;
	size_desty = 0;

	float fractiony = 0.0;


	size_destx = size_srcx * scale;
	size_desty = size_srcy * scale;

//dest_linedx = size_destx;

	bool kern_x_built = 0;
	float *kern_x = new float[ size_destx * kern_size ];				//used to cache kernel vals for first line of x pixels




	dest_fract_x = dest_fract_x / scale;// + tmp0;								//scale up fractional part of dest pos
	dest_fract_y = dest_fract_y / scale;// + tmp1;


//	float kern_fraction;
	
//	size_desty = 0;	
	//image scaling - convolve filter and image  (note: the kernel may not be centered over src pixels, so dest would maybe be shifted slightly to the right by 'kern_size')
	for( int iy = 0; iy < size_desty; iy++ )
		{
		float fy = srcy_in + iy * scale_inv;// - kern_size/2.0f;							//form src coord using dest coord

		double dintx, dinty;


//		size_destx = 0;


		int kern_x_idx = 0;
		for( int ix = 0; ix < size_destx; ix++ )
			{
			float fx = srcx_in + ix * scale_inv;						//form src coord using dest coord

			float fry = fy;
			float frx = fx;

/*
float theta = 0.0f;

//if( fabsf(theta) > 0.001 )
if( 0 )
	{

	float fy0 = fy - size_srcy/2;
	float fx0 = fx - size_srcx/2;

	frx = fx0 * cos0 - fy0 * sin0;
	fry = fy0 * cos0 + fx0 * sin0;




	fry = fry + size_srcy/2;
	frx = frx + size_srcx/2;

	if( fry >= (size_srcy-1) ) fry = size_srcy-1;
	if( fry < 0 ) fry = 0;

	if( frx >= (size_srcy-1) ) frx = size_srcy-1;
	if( frx < 0 ) frx = 0;
	}	
*/


			float fractiony = modf( fry, &dinty ) - dest_fract_y;		//include the fractional position of dest

			float fractionx = modf( frx, &dintx ) - dest_fract_x;
	
//	dintx = round( dintx );
//			fraction = fraction;


//			int filter_subsample_offsetx = fractionx*pixel_subsample_factor;			//slip filter impulse response to sit over a fractional pixel position (allows subpixel positioning)





//		printf("size_srcx %d, fx %f\n", size_srcx, fx );
			wght_sum = 0;
			float sumr = 0;
			float sumg = 0;
			float sumb = 0;
			int kx = 0;
			for( int ky = 0; ky < kern_size; ky++ )
				{
//				float fky = bitmap_rotate_kern0[ filter_center_idx + filter_subsample_offsety ];

				float res;
//				float radius, x;
//				radius = 1.0f;

//				x = ( ((ky-kern_size/2)/(float)kern_size - fractiony*(1.0f/kern_size) ) ) * radius;//(float)n/(N-1)*(2*radius);
//				if (x < 0) x = -x;

					
//					if( filt_kernel_type == 0 )
//						{
//						float radius = 1.0f;
//						float x = 0.0f;
//						if (x < (radius) ) res = ( bitmap_sinx_on_x( x ) * bitmap_sinx_on_x( x / radius ) );	//lanczos with radius
//						else res = 0.0f;
//						}


					if( 1 )
						{
						int n = ky;
						int N = kern_size;
						
						float kern_fraction = n / ( N - 1.0f ) - fractiony * (1.0f/(kern_size-1));
						float x = twopi * kern_fraction;
						res = 0.35875f - 0.48829f * cosf( x ) + 0.14128f * cosf( 2.0f * x ) - 0.01168f * cosf( 3.0f * x );        //blackman-harris window impulse resp
						}

				float fky = res;

//				printf("fx %f %f ky %d fraction %f filtx %f filter_subsample_offsety %d  fky %f\n", fx, fy, ky, fractiony, x, filter_subsample_offsety, fky );

				int srcy = dinty + ky;
//				if (srcy >= size_srcy) at_edgey = 1;
//				else at_edgey = 0;



//printf( " frx %f  dintx %f  fraction %f  filter_subsample_offsetx %d\n", frx, dintx, fraction, filter_subsample_offsetx );
				for( kx = 0; kx < kern_size; kx++ )
					{
//					int srcx = nearbyint(ix * scale_inv) + kx;

//					float fkx = bitmap_rotate_kern0[ filter_center_idx + filter_subsample_offsetx ];

//					x = ( ((kx-kern_size/2)/(float)kern_size - fractionx*(1.0f/kern_size) ) ) * radius;//(float)n/(N-1)*(2*radius);
//					if (x < 0) x = -x;

					
//					if( filt_kernel_type == 0 )
//						{
//						if (x < (radius) ) res = ( bitmap_sinx_on_x( x ) * bitmap_sinx_on_x( x / radius ) );	//lanczos with radius
//						else res = 0.0f;
//						}




//	if( ( fractionx > 0.6f ) && ( fractionx < 0.9f) )  res = 2.0;
//if( res < 0.0f ) res = 0;

					if( 1 )
						{
						
						if( !kern_x_built )								//first x line of pixels?, build 'x' kernel values for a horiz line of pixels, put into array for all other x pixel line calcs
							{
							int n = kx;
							int N = kern_size;

							float kern_fraction = n / ( N - 1.0f ) - fractionx * (1.0f/(kern_size-1));
							float x = twopi * kern_fraction;

							res = 0.35875f - 0.48829f * cosf( x ) + 0.14128f * cosf( 2.0f * x ) - 0.01168f * cosf( 3.0f * x );        //blackman-harris window impulse resp
							
//printf( "kern_x_idx %d (%d)\n", kern_x_idx, size_desty * size_destx * kern_size );
							kern_x[ ix * kern_size + kx ] = res;		//store kernel val for successive x lines		
							}
						else{
							res = kern_x[ ix * kern_size + kx ];		//if already built kernel vals, use them
							}
							
	//					if (n==1 ) res *= 0.25;
	//					if (n==3 ) res *= 0.25;

	//res = bitmap_rotate_kern0[n];
						}

//					kern_x_idx++;
					float fkx = res;



//		if( iy == 0 )printf("fx %f %f kx %d fraction %f filtx %f filter_subsample_offsetx %d  fkx %f\n", fx, fy, kx, fractionx, x, filter_subsample_offsetx, fkx );
					
					int srcx = dintx + kx;

//					if (srcx >= size_srcx) at_edgex = 1;
//					else at_edgex = 0;

//					bool at_edge = at_edgey | at_edgex;

					int psrc = (srcy * linedx*bytes_per_pixel_src)  +  srcx*bytes_per_pixel_src;
					

//		printf("size_srcx %d %d, srcx %d %d fkx %f %f\n", size_srcx, size_srcy, srcx, srcy, fkx, fky );
//					if( !done_whgt_sum ) wght_sum += fky*fkx;
					wght_sum += fky*fkx;
					float fk = fky*fkx;
//if( iy == 0 )printf("%d: fk %f\n", kx, fk );					
					unsigned char uc;

					uc = bfsrc[ psrc++ ];								
//					if( at_edge ) uc = 0;				
					sumr += uc*fk;										//mac

					uc = bfsrc[ psrc++ ];				
//					if( at_edge ) uc = 0;				
					sumg += uc*fk;
					
					uc = bfsrc[ psrc ];				
//					if( at_edge ) uc = 0;				
					sumb += uc*fk;
					}
				}

			sumr = sumr / wght_sum;
			sumg = sumg / wght_sum;	
			sumb = sumb / wght_sum;

/*			
			if( sumr < 0 ) sumr = 0;									//need this for kernels that go below zero, such as lanczos
			if( sumg < 0 ) sumg = 0;
			if( sumb < 0 ) sumb = 0;
			if( sumr > 255 ) sumr = 255;
			if( sumg > 255 ) sumg = 255;
			if( sumb > 255 ) sumb = 255;
*/

			pdest = ( iy ) * ( linedx ) * bytes_per_pixel_src    +    ( ix * bytes_per_pixel_src );
			
			bfdest[ pdest++ ] = sumr;							//place new pixel	
			bfdest[ pdest++ ] = sumg;	
			bfdest[ pdest ] = sumb;
//printf("wght_sum %f\n", wght_sum );				
//			pdest += cn_bytes_per_pixel;
//			done_whgt_sum = 1;

//			size_destx++;

//			rendered_wid = size_destx;
			}
		kern_x_built = 1;												//flag that have done x kernal calcs for first x line of pixels

//		size_desty++;
//		rendered_hei = size_desty;
		}

	delete[] kern_x;
	}


rendered_wid = size_destx;
rendered_hei = size_desty;

//int sizex2 = size_destx;
//int sizey2 = size_desty;


if( rendered_wid >= st_aa[layer_dest].ww ) rendered_wid = st_aa[layer_dest].ww;
if( rendered_hei >= st_aa[layer_dest].hh ) rendered_hei = st_aa[layer_dest].hh;




//reshape geometery to suit dest geometry while copying across
int iy0 = 0;
for( int iy = 0; iy < rendered_hei; iy++ )
	{
	int ix0 = 0;
	for( int ix = 0; ix < rendered_wid; ix++ )
		{
		int p0 = iy * linedx * bytes_per_pixel_src  +  ix * bytes_per_pixel_src;
		int p1 = (idesty + iy0) * destlinedx * bytes_per_pixel_dest  +  (idestx + ix0) * bytes_per_pixel_dest;

//printf( "reshape geometery   %d %d p0 p1 %d %d \n", st_aa[layer_src].bfsiz + p0, st_aa[layer_dest].bfsiz + p1, p0, p1 );
		st_aa[layer_dest].bf[ p1++ ] = bfdest[p0++];
		st_aa[layer_dest].bf[ p1++ ] = bfdest[p0++];
		st_aa[layer_dest].bf[ p1 ] = bfdest[p0];
		ix0++;
		}
	iy0++;
	}

//st_aa[layer_dest].ww = rendered_wid;
//st_aa[layer_dest].hh = rendered_hei;
//st_aa[layer_dest].linedx = rendered_wid;


delete[] bfdest;
return 1;
}

#pragma GCC pop_options




















void aa_canvas::set_pixel_internal( unsigned char *bf, int xx, int yy, int rr, int gg, int bb, int linedx, int bytes_per_pixel )
{
int ix = xx;
int iy = yy;

int psrc = ( (iy) * (linedx)*bytes_per_pixel )  +  (ix)*bytes_per_pixel;

bf[ psrc++ ] = rr;
bf[ psrc++ ] = gg;
bf[ psrc ] = bb;

}






bool aa_canvas::plot_pixel( unsigned int layer, int xx, int yy, int rr, int gg, int bb )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;
if( st_aa[layer].bf == 0 ) return 0;

int ix = xx;
//int iy = st_aa[layer].hh - 1 - yy;				//an increasing 'y' moves up screen
int iy = yy;

int p0 = iy * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  ix * st_aa[layer].bytes_per_pixel;

st_aa[layer].bf[ p0++ ] = rr;
st_aa[layer].bf[ p0++ ] = gg;
st_aa[layer].bf[ p0 ] = bb;
return 1;
}







//'num_pixels' AT MOMENT must be 3,5,7
//loads a vector with one rect of 'rect_size' that bounds the pixel
bool aa_canvas::plot_pixel_multiple_with_rect( unsigned int layer, int xx, int yy, int num_pixels, int rr, int gg, int bb, int rect_size, vector<st_aa_canvas_bounding_rect_tag> &vr )
{
vr.clear();

if( layer >= cn_aa_canvas_layer_max ) return 0;
if( st_aa[layer].bf == 0 ) return 0;


plot_pixel_multiple( layer, xx, yy, rr, gg, bb, num_pixels );



st_aa_canvas_bounding_rect_tag o;

int rect = rect_size/2;

o.x0 = xx - rect;
o.x1 = xx + rect;
o.y0 = yy - rect;
o.y1 = yy + rect;

vr.push_back( o );
return 1;
}






//'num_pixels' AT MOMENT must be 3,5,7
//'num_pixels' 3, 5, 7
//plots an approx circle of pixels
bool aa_canvas::plot_pixel_multiple( unsigned int layer, int xx, int yy, int rr, int gg, int bb, int num_pixels )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;
if( st_aa[layer].bf == 0 ) return 0;

int ix = xx ;
//int iy = st_aa[layer].hh - 1 - yy;				//an increasing 'y' moves up screen
int iy = yy;


switch( num_pixels )
	{
	case 7:
		{
		int offset = -1;
		int y = -3;
		int p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;

		for( int x = 0; x < 3; x++ )
			{
			st_aa[layer].bf[ p0++ ] = rr;
			st_aa[layer].bf[ p0++ ] = gg;
			st_aa[layer].bf[ p0++ ] = bb;
			}

		offset = -2;
		y = -2;
		p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;
		
		for( int x = 0; x < 5; x++ )
			{
			st_aa[layer].bf[ p0++ ] = rr;
			st_aa[layer].bf[ p0++ ] = gg;
			st_aa[layer].bf[ p0++ ] = bb;
			}

		offset = -3;
		y = -1;
		p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;
		for( int x = 0; x < 7; x++ )
			{
			st_aa[layer].bf[ p0++ ] = rr;
			st_aa[layer].bf[ p0++ ] = gg;
			st_aa[layer].bf[ p0++ ] = bb;
			}

		offset = -3;
		y = 0;
		p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;
		for( int x = 0; x < 7; x++ )
			{
			st_aa[layer].bf[ p0++ ] = rr;
			st_aa[layer].bf[ p0++ ] = gg;
			st_aa[layer].bf[ p0++ ] = bb;
			}

		offset = -3;
		y = 1;
		p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;
		for( int x = 0; x < 7; x++ )
			{
			st_aa[layer].bf[ p0++ ] = rr;
			st_aa[layer].bf[ p0++ ] = gg;
			st_aa[layer].bf[ p0++ ] = bb;
			}

		offset = -2;
		y = 2;
		p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;
		
		for( int x = 0; x < 5; x++ )
			{
			st_aa[layer].bf[ p0++ ] = rr;
			st_aa[layer].bf[ p0++ ] = gg;
			st_aa[layer].bf[ p0++ ] = bb;
			}

		offset = -1;
		y = 3;
		p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;
		
		for( int x = 0; x < 3; x++ )
			{
			st_aa[layer].bf[ p0++ ] = rr;
			st_aa[layer].bf[ p0++ ] = gg;
			st_aa[layer].bf[ p0++ ] = bb;
			}
		
		break;
		}





	case 5:
		{
		int offset = 0;
		int y = -2;
		int p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;
		
		st_aa[layer].bf[ p0++ ] = rr;
		st_aa[layer].bf[ p0++ ] = gg;
		st_aa[layer].bf[ p0++ ] = bb;
			

		offset = -1;
		y = -1;
		p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;
		for( int x = 0; x < 3; x++ )
			{
			st_aa[layer].bf[ p0++ ] = rr;
			st_aa[layer].bf[ p0++ ] = gg;
			st_aa[layer].bf[ p0++ ] = bb;
			}

		offset = -2;
		y = 0;
		p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;
		for( int x = 0; x < 5; x++ )
			{
			st_aa[layer].bf[ p0++ ] = rr;
			st_aa[layer].bf[ p0++ ] = gg;
			st_aa[layer].bf[ p0++ ] = bb;
			}

		offset = -1;
		y = 1;
		p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;
		for( int x = 0; x < 3; x++ )
			{
			st_aa[layer].bf[ p0++ ] = rr;
			st_aa[layer].bf[ p0++ ] = gg;
			st_aa[layer].bf[ p0++ ] = bb;
			}

		offset = 0;
		y = 2;
		p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;
		
		st_aa[layer].bf[ p0++ ] = rr;
		st_aa[layer].bf[ p0++ ] = gg;
		st_aa[layer].bf[ p0++ ] = bb;
		break;
		}



	case 3:
		{
		int offset = 0;
		int y = -1;
		int p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;

		st_aa[layer].bf[ p0++ ] = rr;
		st_aa[layer].bf[ p0++ ] = gg;
		st_aa[layer].bf[ p0 ] = bb;
			

		offset = -1;
		y = 0;
		p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;
		
		for( int x = 0; x < 3; x++ )
			{
			st_aa[layer].bf[ p0++ ] = rr;
			st_aa[layer].bf[ p0++ ] = gg;
			st_aa[layer].bf[ p0++ ] = bb;
			}

		offset = 0;
		y = 1;
		p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;

		st_aa[layer].bf[ p0++ ] = rr;
		st_aa[layer].bf[ p0++ ] = gg;
		st_aa[layer].bf[ p0 ] = bb;
		}
	default:
	break;
	}






/*

if( num_pixels == 5 )
	{

	int offset = 0;
	int y = -2;
	int p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;
	
	st_aa[layer].bf[ p0++ ] = rr;
	st_aa[layer].bf[ p0++ ] = gg;
	st_aa[layer].bf[ p0++ ] = bb;
		

	offset = -1;
	y = -1;
	p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;
	for( int x = 0; x < 3; x++ )
		{
		st_aa[layer].bf[ p0++ ] = rr;
		st_aa[layer].bf[ p0++ ] = gg;
		st_aa[layer].bf[ p0++ ] = bb;
		}

	offset = -2;
	y = 0;
	p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;
	for( int x = 0; x < 5; x++ )
		{
		st_aa[layer].bf[ p0++ ] = rr;
		st_aa[layer].bf[ p0++ ] = gg;
		st_aa[layer].bf[ p0++ ] = bb;
		}

	offset = -1;
	y = 1;
	p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;
	for( int x = 0; x < 3; x++ )
		{
		st_aa[layer].bf[ p0++ ] = rr;
		st_aa[layer].bf[ p0++ ] = gg;
		st_aa[layer].bf[ p0++ ] = bb;
		}

	offset = 0;
	y = 2;
	p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;
	
	st_aa[layer].bf[ p0++ ] = rr;
	st_aa[layer].bf[ p0++ ] = gg;
	st_aa[layer].bf[ p0++ ] = bb;
	}









if( num_pixels == 3 )
	{

	int offset = 0;
	int y = -1;
	int p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;

	st_aa[layer].bf[ p0++ ] = rr;
	st_aa[layer].bf[ p0++ ] = gg;
	st_aa[layer].bf[ p0 ] = bb;
		

	offset = -1;
	y = 0;
	p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;
	
	for( int x = 0; x < 3; x++ )
		{
		st_aa[layer].bf[ p0++ ] = rr;
		st_aa[layer].bf[ p0++ ] = gg;
		st_aa[layer].bf[ p0++ ] = bb;
		}

	offset = 0;
	y = 1;
	p0 = (iy + (y)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;

	st_aa[layer].bf[ p0++ ] = rr;
	st_aa[layer].bf[ p0++ ] = gg;
	st_aa[layer].bf[ p0 ] = bb;
	}
*/



/*
int offset = -num_pixels/2;

for( int y = 0; y < num_pixels; y++ )
	{
	int p0 = (iy + (y+offset)) * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  (ix+offset) * st_aa[layer].bytes_per_pixel;

	for( int x = 0; x < num_pixels; x++ )
		{
		st_aa[layer].bf[ p0++ ] = rr;
		st_aa[layer].bf[ p0++ ] = gg;
		st_aa[layer].bf[ p0++ ] = bb;
		}
	}
*/


return 1;
}








//just adds rgb values, does not clamp result
bool aa_canvas::plot_pixel_transparent( unsigned int layer, int xx, int yy, int rr, int gg, int bb )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;
if( st_aa[layer].bf == 0 ) return 0;

int ix = xx ;
//int iy = st_aa[layer].hh - 1 - yy;				//an increasing 'y' moves up screen
int iy = yy;

int p0 = iy * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  ix * st_aa[layer].bytes_per_pixel;


st_aa[layer].bf[ p0++ ] += rr;
st_aa[layer].bf[ p0++ ] += gg;
st_aa[layer].bf[ p0 ] += bb;
return 1;
}







//just adds rgb values, clampa result
bool aa_canvas::plot_pixel_transparent_clamp( unsigned int layer, int xx, int yy, int rr, int gg, int bb )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;
if( st_aa[layer].bf == 0 ) return 0;

int ix = xx ;
//int iy = st_aa[layer].hh - 1 - yy;				//an increasing 'y' moves up screen
int iy = yy;

int p0 = iy * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  ix * st_aa[layer].bytes_per_pixel;

int iv = st_aa[layer].bf[ p0 ];
iv += rr;
if( iv < 0 ) iv = 0;
if( iv > 255 ) iv = 255;
st_aa[layer].bf[ p0++ ] = iv;

iv = st_aa[layer].bf[ p0 ];
iv += gg;
if( iv < 0 ) iv = 0;
if( iv > 255 ) iv = 255;
st_aa[layer].bf[ p0++ ] = iv;

iv = st_aa[layer].bf[ p0 ];
iv += bb;
if( iv < 0 ) iv = 0;
if( iv > 255 ) iv = 255;
st_aa[layer].bf[ p0++ ] = iv;

return 1;
}










bool aa_canvas::set_pixel9_layer( unsigned int layer, int xx, int yy, int rr, int gg, int bb )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;
if( st_aa[layer].bf == 0 ) return 0;


//yy = st_aa[layer].hh - yy - 1;												//an increasing 'y' moves up screen


int px = xx - 1;
int py = yy - 1;


for( int iy = 0; iy < 3; iy++ )
	{
	for( int ix = 0; ix < 3; ix++ )
		{
		int x0 = px + ix;
		int y0 = py + iy;

		int p0 = y0 * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  x0 * st_aa[layer].bytes_per_pixel;

		st_aa[layer].bf[ p0++ ] = rr;
		st_aa[layer].bf[ p0++ ] = gg;
		st_aa[layer].bf[ p0 ] = bb;
		}
	}

return 1;
}




//pixels surrounding 'xx, 'yy' are set also ---> 'xx', 'yy' is at center 
bool aa_canvas::set_pixel25_layer( unsigned int layer, int xx, int yy, int rr, int gg, int bb, bool b_cache_pixels )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;
if( st_aa[layer].bf == 0 ) return 0;


//yy = st_aa[layer].hh - yy - 1;												//an increasing 'y' moves up screen

int px = xx - 2;
int py = yy - 2;

for( int iy = 0; iy < 5; iy++ )
	{
	for( int ix = 0; ix < 5; ix++ )
		{
		int x0 = px + ix;
		int y0 = py + iy;
//printf("x0 %02d  %02d  \n", x0, y0 );
if( y0 > 1 )
	{
//	printf("stopped\n");
//	getchar();
	}
if( x0 < 0 )
	{
//	getchar();
	}
		int p0 = y0 * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  x0 * st_aa[layer].bytes_per_pixel;

		st_aa[layer].bf[ p0++ ] = rr;
		st_aa[layer].bf[ p0++ ] = gg;
		st_aa[layer].bf[ p0 ] = bb;
		
		if( b_cache_pixels )
			{
			st_aa_canvas_pixel_point_tag o;									//cache pixel details
			o.layer = layer;
			
			o.x0 = x0;
			o.y0 = y0;
			o.rr = rr;
			o.gg = gg;
			o.bb = bb;
			
			vpixpnt.push_back( o );
			}
		}
	}


//		int p0 = y0 * st_aa[layer].linedx * st_aa[layer].bytes_per_pixel   +  x0 * st_aa[layer].bytes_per_pixel;


return 1;
}







//draw a line in spec layer
bool aa_canvas::plot_line_25( unsigned int layer, int x1, int y1, int x2, int y2, int rr, int gg, int bb, int linedx, bool b_cache_pixels )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;
if( st_aa[layer].bf == 0 ) return 0;


printf("aa_canvas::plot_line_25()000 x1 %d %d %d %d\n", x1, y1, x2, y2 );
if( st_aa[layer].lineclp->line_clip_int( x1, y1, x2, y2 ) );


float xx1 = x1;
float yy1 = y1;

float xx2 = x2;
float yy2 = y2;

printf("aa_canvas::plot_line_25()111 x1 %d %d %d %d  x1, y1, x2, y2\n", x1, y1, x2, y2 );
if( 1 )
	{
	float dx = xx2 - xx1;
	float dy = yy2 - yy1;

	if( fabsf( dx ) >= fabsf( dy ) )
		{
		if( xx1 > xx2 )
			{
			float swap = xx1;
			xx1 = xx2;
			xx2 = swap;
			
			swap = yy1;	
			yy1 = yy2;
			yy2 = swap;
			
			dx = -dx;
			dy = -dy;
			}

		float m = ( yy2 - yy1 ) / ( xx2 - xx1 );

		float b = -(m*xx1 - yy1);

//printf("%03d dx %f %f b %f m %f\n", dbg_line_idx, dx, dy, b, m );	
		
		int sub = 0;
		for( int i = xx1; i <= xx2; i+=1 )
			{
			int xx = i;
			int yy = m*i + b;

//			fl_point( xx, yy );

//			get_pixel( ot.bf, xx + ot.srcx, yy + ot.srcy, rr, gg, bb, ot.linedx );				//src pixel

//if( b_cache_pixels ) 


			if( 1 )
				{
				set_pixel25_layer( layer, xx, yy, rr, gg, bb, b_cache_pixels );
				}

/*
//			set_pixel_internal( st_aa[layer].bf, xx, yy, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
//			set_pixel_internal( st_aa[layer].bf, xx - 1, yy, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
//			set_pixel_internal( st_aa[layer].bf, xx + 1, yy, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
//			set_pixel_internal( st_aa[layer].bf, xx - 2, yy, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
//			set_pixel_internal( st_aa[layer].bf, xx + 2, yy, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
//			set_pixel_internal( st_aa[layer].bf, xx - 3, yy, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
//			set_pixel_internal( st_aa[layer].bf, xx + 3, yy, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel


			if( 1 )
				{
				set_pixel_internal( st_aa[layer].bf, xx - 1, yy - 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 1, yy - 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx - 2, yy - 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 2, yy - 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx - 3, yy - 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 3, yy - 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				}

			if( 1 )
				{
				set_pixel_internal( st_aa[layer].bf, xx - 1, yy - 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 1, yy - 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx - 2, yy - 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 2, yy - 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx - 3, yy - 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 3, yy - 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				}

			if( 1 )
				{
				set_pixel_internal( st_aa[layer].bf, xx - 1, yy + 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 1, yy + 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx - 2, yy + 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 2, yy + 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx - 3, yy + 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 3, yy + 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				}

			if( 1 )
				{
				set_pixel_internal( st_aa[layer].bf, xx - 1, yy + 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 1, yy + 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx - 2, yy + 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 2, yy + 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx - 3, yy + 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 3, yy + 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				}
*/
			}	
		}
	else{
		//dy > dx
		if( yy1 > yy2 )
			{
			float swap = xx1;
			xx1 = xx2;
			xx2 = swap;
			
			swap = yy1;	
			yy1 = yy2;
			yy2 = swap;
			
			dx = -dx;
			dy = -dy;
			}

		float m = ( xx2 - xx1 ) / ( yy2 - yy1 );

		float b = -(m*yy1 - xx1);

//printf("%03d dx %f %f b %f m %f\n", dbg_line_idx, dx, dy, b, m );	
		int sub = 0;
		for( int i = yy1; i <= yy2; i+=1 )
			{
			int yy = i;
			int xx = m*i + b;
			
			if( 1 )
				{
				set_pixel25_layer( layer, xx, yy, rr, gg, bb, b_cache_pixels );
				}
/*
			set_pixel_internal( st_aa[layer].bf, xx, yy, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
			set_pixel_internal( st_aa[layer].bf, xx - 1, yy, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
			set_pixel_internal( st_aa[layer].bf, xx + 1, yy, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
			set_pixel_internal( st_aa[layer].bf, xx - 2, yy, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
			set_pixel_internal( st_aa[layer].bf, xx + 2, yy, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
			set_pixel_internal( st_aa[layer].bf, xx - 3, yy, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
			set_pixel_internal( st_aa[layer].bf, xx + 3, yy, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel


			if( 1 )
				{
				set_pixel_internal( st_aa[layer].bf, xx - 1, yy - 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 1, yy - 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx - 2, yy - 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 2, yy - 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx - 3, yy - 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 3, yy - 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				}

			if( 1 )
				{
				set_pixel_internal( st_aa[layer].bf, xx - 1, yy - 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 1, yy - 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx - 2, yy - 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 2, yy - 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx - 3, yy - 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 3, yy - 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				}

			if( 1 )
				{
				set_pixel_internal( st_aa[layer].bf, xx - 1, yy + 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 1, yy + 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx - 2, yy + 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 2, yy + 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx - 3, yy + 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 3, yy + 1, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				}

			if( 1 )
				{
				set_pixel_internal( st_aa[layer].bf, xx - 1, yy + 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 1, yy + 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx - 2, yy + 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 2, yy + 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx - 3, yy + 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				set_pixel_internal( st_aa[layer].bf, xx + 3, yy + 2, rr, gg, bb, st_aa[layer].linedx, st_aa[layer].bytes_per_pixel );		//dest pixel
				}
*/
			}	
		}
	}
return 1;
}









//draw a line in spec layer
bool aa_canvas::plot_line( unsigned int layer, int x1, int y1, int x2, int y2, int rr, int gg, int bb )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;
if( st_aa[layer].bf == 0 ) return 0;


if( !st_aa[layer].lineclp->line_clip_int( x1, y1, x2, y2 ) )
	{
	return 1;
	}

//int linedx = st_aa[layer].linedx;

float xx1 = x1;
float yy1 = y1;

float xx2 = x2;
float yy2 = y2;

if( 1 )
	{
	float dx = xx2 - xx1;
	float dy = yy2 - yy1;

	if( fabsf( dx ) >= fabsf( dy ) )
		{
		if( xx1 > xx2 )
			{
			float swap = xx1;
			xx1 = xx2;
			xx2 = swap;
			
			swap = yy1;	
			yy1 = yy2;
			yy2 = swap;
			
			dx = -dx;
			dy = -dy;
			}

		float m = ( yy2 - yy1 ) / ( xx2 - xx1 );

		float b = -(m*xx1 - yy1);

//printf("%03d dx %f %f b %f m %f\n", dbg_line_idx, dx, dy, b, m );	
		
		int sub = 0;
		for( int i = xx1; i <= xx2; i+=1 )
			{
			int xx = i;
			int yy = m*i + b;


//			if( 1 )
				{
				plot_pixel( layer, xx, yy, rr, gg, bb );
				}

			}	
		}
	else{
		//dy > dx
		if( yy1 > yy2 )
			{
			float swap = xx1;
			xx1 = xx2;
			xx2 = swap;
			
			swap = yy1;	
			yy1 = yy2;
			yy2 = swap;
			
			dx = -dx;
			dy = -dy;
			}

		float m = ( xx2 - xx1 ) / ( yy2 - yy1 );

		float b = -(m*yy1 - xx1);

		int sub = 0;
		for( int i = yy1; i <= yy2; i+=1 )
			{
			int yy = i;
			int xx = m*i + b;
			
//			if( 1 )
				{
				plot_pixel( layer, xx, yy, rr, gg, bb );
				}

			}	
		}
	}
return 1;
}









//draw a line in spec layer,
//builds a vector of overlaying rectangles in the direction of line's greatest deflection, 
//these rects (or tiles) will bound the line, thicker lines need 'rect_size' to be larger,
//the tiles overlap more when lines approach 45 degrees, so 45 degree lines have the most tiles, 
//the rect tiles can be used by filter downsampling algorithm to minimize the size and number of filter block calcs required,
//'num_pixels' is thickness of line, must be either  3, 5, 7
//'rect_size' is size of each bounding rect, adj this till you see no 'dropouts' in filter downsampled block tiling, try val of 27
bool aa_canvas::plot_line_thick_bound_rect( unsigned int layer, int x1, int y1, int x2, int y2, int rr, int gg, int bb, int num_pixels, int rect_size, vector<st_aa_canvas_bounding_rect_tag> &vr )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;
if( st_aa[layer].bf == 0 ) return 0;

vr.clear();

if( !st_aa[layer].lineclp->line_clip_int( x1, y1, x2, y2 ) )
	{
	return 1;
	}

st_aa_canvas_bounding_rect_tag o;


//find if line is close to a 45 degree angle
float theta0 = atan2f( (y1 - y2), ( x2 - x1 ) );

float theta1 = fabsf( theta0 );
if( theta1 > pi/2 ) theta1 -= pi/2;
if( theta1 > pi ) theta1 -= pi;


//adj how much overlap the encompassing rects will have, 45 degree lines need the most overlap 
int proximity_45 = 0;
if( ( theta1 > 0.485 ) && ( theta1 < 1.085 ) ) proximity_45 = 2;
if( ( theta1 > 0.585 ) && ( theta1 < 0.985 ) ) proximity_45 = 3;
if( ( theta1 > 0.685 ) && ( theta1 < 0.885 ) ) proximity_45 = 4;		//most overlap

int modulo_val = 25;													//least overlap
if( proximity_45 == 4 ) modulo_val = 15;								//most overlap
if( proximity_45 == 3 ) modulo_val = 17;
if( proximity_45 == 2 ) modulo_val = 20;


//printf( "aa_canvas::plot_line_thick_bound_rect() - theta %f  %f\n", theta0, theta1 );

//int linedx = st_aa[layer].linedx;

float xx1 = x1;
float yy1 = y1;

float xx2 = x2;
float yy2 = y2;




if( 1 )
	{
	float dx = xx2 - xx1;
	float dy = yy2 - yy1;

	bool bforce_one_rect = 0;
	int len = sqrtf( dx*dx + dy*dy );									//if line length < 'rect_size', then make only one triangle

	if( len <= rect_size ) bforce_one_rect = 1;
	
	if( bforce_one_rect )
		{
		//add one encompassing rect
		int minx, miny, maxx, maxy;
		
		if( xx1 <= xx2 ) { minx = xx1; maxx = xx2; }
		else { minx = xx2; maxx = xx1; }
		
		if( yy1 <= yy2 ) { miny = yy1; maxy = yy2; }
		else { miny = yy2; maxy = yy1; }
		
		o.x0 = minx - rect_size/2;
		o.y0 = miny - rect_size/2;

		if( o.x0 < 0 ) o.x0 = 0; 
		if( o.x0 >= st_aa[layer].ww - 1 ) o.x0 = st_aa[layer].ww - 1;

		if( o.y0 < 0 ) o.y0 = 0; 
		if( o.y0 >= st_aa[layer].hh - 1 ) o.y0 = st_aa[layer].hh - 1;

		o.x1 = maxx + rect_size/2;
		o.y1 = maxy + rect_size/2; 

		if( o.x1 < 0 ) o.x1 = 0; 
		if( o.x1 >= st_aa[layer].ww - 1 ) o.x1 = st_aa[layer].ww - 1;

		if( o.y1 < 0 ) o.y1 = 0; 
		if( o.y1 >= st_aa[layer].hh - 1 ) o.y1 = st_aa[layer].hh - 1;


		vr.push_back( o );
		}


	if( !bforce_one_rect )
		{
		//add endpoint rects
		o.x0 = xx1 - rect_size/2;
		o.y0 = yy1 - rect_size/2;

		if( o.x0 < 0 ) o.x0 = 0; 
		if( o.x0 >= st_aa[layer].ww - 1 ) o.x0 = st_aa[layer].ww - 1;

		if( o.y0 < 0 ) o.y0 = 0; 
		if( o.y0 >= st_aa[layer].hh - 1 ) o.y0 = st_aa[layer].hh - 1;

	//o.x0 = o.x0&0xfffffffc;					//round to nearest multiple of 4 (or zero), helps ensure filter kernel sits in over same pixels when compared to a full image downsample, stops partial filter block compositing shifts
	//o.y0 = o.y0&0xfffffffc;
		o.x1 = xx1 + rect_size/2;
		o.y1 = yy1 + rect_size/2; 

		if( o.x1 < 0 ) o.x1 = 0; 
		if( o.x1 >= st_aa[layer].ww - 1 ) o.x1 = st_aa[layer].ww - 1;

		if( o.y1 < 0 ) o.y1 = 0; 
		if( o.y1 >= st_aa[layer].hh - 1 ) o.y1 = st_aa[layer].hh - 1;


		vr.push_back( o );

		o.x0 = xx2 - rect_size/2;
		o.y0 = yy2 - rect_size/2;

		if( o.x0 < 0 ) o.x0 = 0; 
		if( o.x0 >= st_aa[layer].ww - 1 ) o.x0 = st_aa[layer].ww - 1;

		if( o.y0 < 0 ) o.y0 = 0; 
		if( o.y0 >= st_aa[layer].hh - 1 ) o.y0 = st_aa[layer].hh - 1;

	//o.x0 = o.x0&0xfffffffc;
	//o.y0 = o.y0&0xfffffffc;
		o.x1 = xx2 + rect_size/2;
		o.y1 = yy2 + rect_size/2; 

		if( o.x1 < 0 ) o.x1 = 0; 
		if( o.x1 >= st_aa[layer].ww - 1 ) o.x1 = st_aa[layer].ww - 1;

		if( o.y1 < 0 ) o.y1 = 0; 
		if( o.y1 >= st_aa[layer].hh - 1 ) o.y1 = st_aa[layer].hh - 1;
		vr.push_back( o );
		}


	if( fabsf( dx ) >= fabsf( dy ) )
		{
		if( xx1 > xx2 )
			{
			float swap = xx1;
			xx1 = xx2;
			xx2 = swap;
			
			swap = yy1;	
			yy1 = yy2;
			yy2 = swap;
			
			dx = -dx;
			dy = -dy;
			}


		float m = ( yy2 - yy1 ) / ( xx2 - xx1 );

		float b = -(m*xx1 - yy1);

//printf("%03d dx %f %f b %f m %f\n", dbg_line_idx, dx, dy, b, m );	
		
		int modulo = 0;
		for( int i = xx1; i <= xx2; i+=1 )
			{
			int xx = i;
			int yy = m*i + b;


			if( !bforce_one_rect )
				{
				//add rect
				if( modulo == modulo_val )
					{
					o.x0 = xx - rect_size/2;
					o.y0 = yy - rect_size/2;

					if( o.x0 < 0 ) o.x0 = 0; 
					if( o.x0 >= st_aa[layer].ww - 1 ) o.x0 = st_aa[layer].ww - 1;

					if( o.y0 < 0 ) o.y0 = 0; 
					if( o.y0 >= st_aa[layer].hh - 1 ) o.y0 = st_aa[layer].hh - 1;

	//o.x0 = o.x0&0xfffffffc;
	//o.y0 = o.y0&0xfffffffc;
					o.x1 = xx + rect_size/2;
					o.y1 = yy + rect_size/2;
					
					
					if( o.x1 < 0 ) o.x1 = 0; 
					if( o.x1 >= st_aa[layer].ww - 1 ) o.x1 = st_aa[layer].ww - 1;

					if( o.y1 < 0 ) o.y1 = 0; 
					if( o.y1 >= st_aa[layer].hh - 1 ) o.y1 = st_aa[layer].hh - 1;

					vr.push_back( o );
					modulo = 0;
					}
				else{
					modulo++;
					}
				}


			plot_pixel_multiple( layer, xx, yy, rr, gg, bb, num_pixels );
		
			}	
		}
	else{
		//dy > dx
		if( yy1 > yy2 )
			{
			float swap = xx1;
			xx1 = xx2;
			xx2 = swap;
			
			swap = yy1;	
			yy1 = yy2;
			yy2 = swap;
			
			dx = -dx;
			dy = -dy;
			}

		float m = ( xx2 - xx1 ) / ( yy2 - yy1 );

		float b = -(m*yy1 - xx1);

		int modulo = 0;		
		for( int i = yy1; i <= yy2; i+=1 )
			{
			int yy = i;
			int xx = m*i + b;
			
			if( !bforce_one_rect )
				{
				//add rect
				if( modulo == modulo_val )
					{
					o.x0 = xx - rect_size/2;
					o.y0 = yy - rect_size/2;


					if( o.x0 < 0 ) o.x0 = 0; 
					if( o.x0 >= st_aa[layer].ww - 1 ) o.x0 = st_aa[layer].ww - 1;

					if( o.y0 < 0 ) o.y0 = 0; 
					if( o.y0 >= st_aa[layer].hh - 1 ) o.y0 = st_aa[layer].hh - 1;

	//o.x0 = o.x0&0xfffffffc;
	//o.y0 = o.y0&0xfffffffc;
					o.x1 = xx + rect_size/2;
					o.y1 = yy + rect_size/2; 
					
					if( o.x1 < 0 ) o.x1 = 0; 
					if( o.x1 >= st_aa[layer].ww - 1 ) o.x1 = st_aa[layer].ww - 1;

					if( o.y1 < 0 ) o.y1 = 0; 
					if( o.y1 >= st_aa[layer].hh - 1 ) o.y1 = st_aa[layer].hh - 1;
					vr.push_back( o );
				
					modulo = 0;
					}
				else{
					modulo++;
					}
				}

			plot_pixel_multiple( layer, xx, yy, rr, gg, bb, num_pixels );
			}	
		}
	}
return 1;
}











//'num_pixels' must be either  3, 5, 7
//draw a line in spec layer
bool aa_canvas::plot_line_thick( unsigned int layer, int x1, int y1, int x2, int y2, int rr, int gg, int bb, int num_pixels )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;
if( st_aa[layer].bf == 0 ) return 0;


if( !st_aa[layer].lineclp->line_clip_int( x1, y1, x2, y2 ) )
	{
	return 1;
	}

//int linedx = st_aa[layer].linedx;

float xx1 = x1;
float yy1 = y1;

float xx2 = x2;
float yy2 = y2;

if( 1 )
	{
	float dx = xx2 - xx1;
	float dy = yy2 - yy1;

	if( fabsf( dx ) >= fabsf( dy ) )
		{
		if( xx1 > xx2 )
			{
			float swap = xx1;
			xx1 = xx2;
			xx2 = swap;
			
			swap = yy1;	
			yy1 = yy2;
			yy2 = swap;
			
			dx = -dx;
			dy = -dy;
			}

		float m = ( yy2 - yy1 ) / ( xx2 - xx1 );

		float b = -(m*xx1 - yy1);

//printf("%03d dx %f %f b %f m %f\n", dbg_line_idx, dx, dy, b, m );	
		
		int sub = 0;
		for( int i = xx1; i <= xx2; i+=1 )
			{
			int xx = i;
			int yy = nearbyint(m*i + b);


//			if( 1 )
				{
				plot_pixel_multiple( layer, xx, yy, rr, gg, bb, num_pixels );
				}

			}	
		}
	else{
		//dy > dx
		if( yy1 > yy2 )
			{
			float swap = xx1;
			xx1 = xx2;
			xx2 = swap;
			
			swap = yy1;	
			yy1 = yy2;
			yy2 = swap;
			
			dx = -dx;
			dy = -dy;
			}

		float m = ( xx2 - xx1 ) / ( yy2 - yy1 );

		float b = -(m*yy1 - xx1);

		int sub = 0;
		for( int i = yy1; i <= yy2; i+=1 )
			{
			int yy = i;
			int xx = nearbyint(m*i + b);
			
//			if( 1 )
				{
				plot_pixel_multiple( layer, xx, yy, rr, gg, bb, num_pixels );
				}

			}	
		}
	}
return 1;
}










//draw a line in spec layer
bool aa_canvas::plot_line_transparent( unsigned int layer, int x1, int y1, int x2, int y2, int rr, int gg, int bb )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;
if( st_aa[layer].bf == 0 ) return 0;


if( !st_aa[layer].lineclp->line_clip_int( x1, y1, x2, y2 ) )
	{
	return 1;
	}
//int linedx = st_aa[layer].linedx;

float xx1 = x1;
float yy1 = y1;

float xx2 = x2;
float yy2 = y2;

if( 1 )
	{
	float dx = xx2 - xx1;
	float dy = yy2 - yy1;

	if( fabsf( dx ) >= fabsf( dy ) )
		{
		if( xx1 > xx2 )
			{
			float swap = xx1;
			xx1 = xx2;
			xx2 = swap;
			
			swap = yy1;	
			yy1 = yy2;
			yy2 = swap;
			
			dx = -dx;
			dy = -dy;
			}

		float m = ( yy2 - yy1 ) / ( xx2 - xx1 );

		float b = -(m*xx1 - yy1);

//printf("%03d dx %f %f b %f m %f\n", dbg_line_idx, dx, dy, b, m );	
		
		int sub = 0;
		for( int i = xx1; i <= xx2; i+=1 )
			{
			int xx = i;
			int yy = m*i + b;


//			if( 1 )
				{
				plot_pixel_transparent( layer, xx, yy, rr, gg, bb );
				}

			}	
		}
	else{
		//dy > dx
		if( yy1 > yy2 )
			{
			float swap = xx1;
			xx1 = xx2;
			xx2 = swap;
			
			swap = yy1;	
			yy1 = yy2;
			yy2 = swap;
			
			dx = -dx;
			dy = -dy;
			}

		float m = ( xx2 - xx1 ) / ( yy2 - yy1 );

		float b = -(m*yy1 - xx1);

		int sub = 0;
		for( int i = yy1; i <= yy2; i+=1 )
			{
			int yy = i;
			int xx = m*i + b;
			
//			if( 1 )
				{
				plot_pixel_transparent( layer, xx, yy, rr, gg, bb );
				}

			}	
		}
	}
return 1;
}















//draw a line in spec layer
bool aa_canvas::plot_line_transparent_clamp( unsigned int layer, int x1, int y1, int x2, int y2, int rr, int gg, int bb )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;
if( st_aa[layer].bf == 0 ) return 0;


if( !st_aa[layer].lineclp->line_clip_int( x1, y1, x2, y2 ) )
	{
	return 1;
	}

//int linedx = st_aa[layer].linedx;

float xx1 = x1;
float yy1 = y1;

float xx2 = x2;
float yy2 = y2;

if( 1 )
	{
	float dx = xx2 - xx1;
	float dy = yy2 - yy1;

	if( fabsf( dx ) >= fabsf( dy ) )
		{
		if( xx1 > xx2 )
			{
			float swap = xx1;
			xx1 = xx2;
			xx2 = swap;
			
			swap = yy1;	
			yy1 = yy2;
			yy2 = swap;
			
			dx = -dx;
			dy = -dy;
			}

		float m = ( yy2 - yy1 ) / ( xx2 - xx1 );

		float b = -(m*xx1 - yy1);

//printf("%03d dx %f %f b %f m %f\n", dbg_line_idx, dx, dy, b, m );	
		
		int sub = 0;
		for( int i = xx1; i <= xx2; i+=1 )
			{
			int xx = i;
			int yy = m*i + b;


//			if( 1 )
				{
				plot_pixel_transparent_clamp( layer, xx, yy, rr, gg, bb );
				}

			}	
		}
	else{
		//dy > dx
		if( yy1 > yy2 )
			{
			float swap = xx1;
			xx1 = xx2;
			xx2 = swap;
			
			swap = yy1;	
			yy1 = yy2;
			yy2 = swap;
			
			dx = -dx;
			dy = -dy;
			}

		float m = ( xx2 - xx1 ) / ( yy2 - yy1 );

		float b = -(m*yy1 - xx1);

		int sub = 0;
		for( int i = yy1; i <= yy2; i+=1 )
			{
			int yy = i;
			int xx = m*i + b;
			
//			if( 1 )
				{
				plot_pixel_transparent_clamp( layer, xx, yy, rr, gg, bb );
				}

			}	
		}
	}
return 1;
}







bool aa_canvas::plot_rect( unsigned int layer, st_aa_canvas_bounding_rect_tag &orect, int rr, int gg, int bb )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;
if( st_aa[layer].bf == 0 ) return 0;


plot_line( layer, orect.x0, orect.y0, orect.x0, orect.y1, rr, gg, bb );

plot_line( layer, orect.x0, orect.y1, orect.x1, orect.y1, rr, gg, bb );

plot_line( layer, orect.x1, orect.y1, orect.x1, orect.y0, rr, gg, bb );

plot_line( layer, orect.x1, orect.y0, orect.x0, orect.y0, rr, gg, bb );

return 1;
}









bool aa_canvas::plot_rect_using_coords( unsigned int layer, int x0, int y0, int ww, int hh, int rr, int gg, int bb )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;
if( st_aa[layer].bf == 0 ) return 0;


//if( x0 < 0 ) x0 = 0;
//if( y0 < 0 ) y0 = 0;

//if( (x0 + ww) >= st_aa[layer].ww ) ww = st_aa[layer].ww - x0 - 1;

//if( (y0 + hh) >= st_aa[layer].hh ) hh = st_aa[layer].hh - y0 - 1;


plot_line( layer, x0, y0, x0, y0 + hh, rr, gg, bb );

plot_line( layer, x0, y0 + hh, x0 + ww, y0 + hh, rr, gg, bb );

plot_line( layer, x0 + ww, y0 + hh, x0 + ww, y0, rr, gg, bb );

plot_line( layer, x0 + ww, y0, x0, y0, rr, gg, bb );

return 1;
}









/*
//fast arc draw, using line vertex points, works well when radius is large, fl_arc() is very slow with large radii,
//the cull flag is not useful as arc is drawn as one complete line (with many vertices), so cull is not visually correct
void captr::draw_arc_fast( flts forgx, flts forgy, flts frad, flts start_ang, flts stop_ang, bool filled, bool cull )
{
while( start_ang >= 360.0 )  { start_ang -= 360.0;  stop_ang -= 360.0; }
while( start_ang <= -360.0 ) { start_ang += 360.0;  stop_ang += 360.0; }
while( stop_ang >= 360.0 )   { start_ang -= 360.0;  stop_ang -= 360.0; }
while( stop_ang <= -360.0 )  { start_ang += 360.0;  stop_ang += 360.0; }

double theta = ( start_ang ) * cn_deg2rad;

flts fx1 = forgx + frad * cos ( theta );
flts fy1 = forgy + frad * sin ( theta ); ;

flts fx2, fy2;

flts dir;
if( ( start_ang ) <= ( stop_ang ) ) dir = 1.0;
else dir = -1.0;


flts step_val = 50.0 / frad;
if ( step_val < 1.0 ) step_val = 1.0;

int saftey_count = 0;

flts angle = start_ang;

if( filled ) fl_begin_complex_polygon();
else fl_begin_line();

while( 1 )
	{
	double theta = angle * cn_deg2rad;

	fx2 = frad * cos ( theta ); 
	fy2 = frad * sin ( theta ); 
	fx2 += forgx;
	fy2 += forgy;

	bool offscreen = 0;

	if( ( midx + fx1 < 0 ) && ( midx + fx2 < w() ) ) offscreen = 1;
	if( ( midx + fx1 > w() ) && ( midx + fx2 > w() ) ) offscreen = 1;

	if( ( midy - fy1 < 0 ) && ( midy - fy2 < h() ) ) offscreen = 1;
	if( ( midy - fy1 > 0 ) && ( midy - fy2 > h() ) ) offscreen = 1;


//	if( !offscreen ) fl_line( nearbyint( wndx + midx + fx1 ), nearbyint( wndy + midy - fy1 ), nearbyint( wndx + midx + fx2 ), nearbyint( wndy + midy - fy2 ) );

	if( ( !offscreen ) | ( filled ) | (!cull) )			//don't cull if 'filled' required
		{
		fl_vertex( nearbyint( wndx + midx + fx1 ), nearbyint( wndy + midy - fy1 ) );
		fl_vertex( nearbyint( wndx + midx + fx2 ), nearbyint( wndy + midy - fy2 ) );
		}

	angle += step_val * dir;

	if( dir == 1.0 )
		{
		if( angle > ( stop_ang ) ) break;
		}
	else{
		if( angle < ( stop_ang ) ) break;
		}

	if( angle >= 360.0 ) angle -=360.0;
	if( angle <= -360.0 ) angle +=360.0;

	if( saftey_count > 500 ) break;
	saftey_count++;
	fx1 = fx2;
	fy1 = fy2;
	}

//printf( "saftey_count: %d\n", saftey_count ); 

if( filled ) fl_end_complex_polygon();
else fl_end_line();

}
*/












//'num_pixels' must be either  3, 5, 7
//'rect_size' sets the size of individual rectangles bounding each arc line segment (when 'one_rect_only' = 0 )
//'rect_size' sets the oversizing of single rectangle covering whole of arc curve (when 'one_rect_only' = 1 ), oversizing is performed on top,bott,left,right edges
//MAKE sure you ADD half of 'num_pixels' to 'rect_size' also, to encompass line thickness as well
bool aa_canvas::plot_arc_thick_rect( unsigned int layer, int px, int py, float radius, float start_ang, float stop_ang, int segments, int num_pixels, int rect_size, bool one_rect_only, int rr, int gg, int bb, vector<st_aa_canvas_bounding_rect_tag> &vr )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;
if( st_aa[layer].bf == 0 ) return 0;

vr.clear();

if( start_ang == stop_ang ) return 0;

vector<st_aa_canvas_bounding_rect_tag> vr1;

while( start_ang >= twopi )  { start_ang -= twopi;  stop_ang -= twopi; }
while( start_ang <= -twopi ) { start_ang += twopi;  stop_ang += twopi; }
while( stop_ang >= twopi )   { start_ang -= twopi;  stop_ang -= twopi; }
while( stop_ang <= -twopi )  { start_ang += twopi;  stop_ang += twopi; }

int dir;
if( start_ang <= stop_ang ) dir = 1;
else dir = -1;




if( segments < 2 ) segments = 2;
if( segments > 500 ) segments = 500;


//int dir;
//if( start_ang <= stop_ang ) dir = 1;
//else dir = -1;



float step_val = fabsf(stop_ang - start_ang ) / segments;

step_val *= dir;


int minx = 1e6;
int maxx = -1e6;
int miny = 1e6;
int maxy = -1e6;



float theta = start_ang;

int x1 = nearbyint( radius * cosf ( theta ) ) + px; 
int y1 = nearbyint( radius * -sinf ( theta ) ) + py; 								//invert so positive 'y' goes up the screen, i.e. pi/2 is top of the clock


int cnt = 0;
while( 1 )
	{
	theta += step_val;

	if( theta >= twopi ) theta -= twopi;
	if( theta <= -twopi ) theta += twopi;


//	if( dir == 1 )
//		{
//		if( theta > stop_ang ) break;
//		}
//	else{
//		if( theta < stop_ang ) break;
//		}

	int x2 = nearbyint( radius * cosf( theta ) ) + px; 
	int y2 = nearbyint( radius * -sinf( theta ) ) + py; 								//invert so positive 'y' goes up the screen, i.e. pi/2 is top of the clock

//printf("aa_canvas::plot_arc_thick_rect() - x1 y1 %d %d %d %d\n", x1, y1, x2, y2 );

	if( !one_rect_only ) plot_line_thick_bound_rect( layer, x1, y1, x2, y2, rr, gg, bb, num_pixels, rect_size, vr1 );
	else plot_line_thick( layer, x1, y1, x2, y2, rr, gg, bb, num_pixels );


if( one_rect_only )
	{
	if( x1 < minx ) minx = x1;
	if( x1 > maxx ) maxx = x1;
	if( y1 < miny ) miny = y1;
	if( y1 > maxy ) maxy = y1;

	if( x2 < minx ) minx = x2;
	if( x2 > maxx ) maxx = x2;
	if( y2 < miny ) miny = y2;
	if( y2 > maxy ) maxy = y2;
	}

	x1 = x2;
	y1 = y2;

	if( !one_rect_only) vr.insert(vr.end(), vr1.begin(), vr1.end());

//printf("saftey_count %d\n", saftey_count );
	cnt++;
	if( cnt >= segments ) break;
	}

if( one_rect_only )
	{
	st_aa_canvas_bounding_rect_tag orect;


	orect.x0 = minx - rect_size;	
	orect.y0 = miny - rect_size;	
	orect.x1 = maxx + rect_size;
	orect.y1 = maxy + rect_size;
	vr.push_back( orect );
	}




//plot_line( layer, x0, y0, x0, y0 + hh, rr, gg, bb );

//plot_line( layer, x0, y0 + hh, x0 + ww, y0 + hh, rr, gg, bb );

//plot_line( layer, x0 + ww, y0 + hh, x0 + ww, y0, rr, gg, bb );

//plot_line( layer, x0 + ww, y0, x0, y0, rr, gg, bb );

return 1;
}









//build an arc polygon shape
//e.g: 'rect_oversizex' enlarges the bounding rectangle on either side of the final polygon's shape (in x direction), i.e 2x 'rect_oversizex'
void aa_canvas::ploygon_arc_build(  int px, int py, float radius, float start_ang, float stop_ang, int segments, int rect_oversizex, int rect_oversizey, st_aa_canvas_polygon_tag &op )
{
op.vcrd.clear();

op.posx = px;
op.posy = py;
op.scale = 1.0f;
op.rotate_theta = 0.0f;

if( start_ang == stop_ang ) return;


while( start_ang >= twopi )  { start_ang -= twopi;  stop_ang -= twopi; }
while( start_ang <= -twopi ) { start_ang += twopi;  stop_ang += twopi; }
while( stop_ang >= twopi )   { start_ang -= twopi;  stop_ang -= twopi; }
while( stop_ang <= -twopi )  { start_ang += twopi;  stop_ang += twopi; }

int dir;
if( start_ang <= stop_ang ) dir = 1;
else dir = -1;




if( segments < 2 ) segments = 2;
if( segments > 500 ) segments = 500;


//int dir;
//if( start_ang <= stop_ang ) dir = 1;
//else dir = -1;



float step_val = fabsf(stop_ang - start_ang ) / segments;

step_val *= dir;


int minx = 1e6;
int maxx = -1e6;
int miny = 1e6;
int maxy = -1e6;



float theta = start_ang;

int x1 = nearbyint( radius * cosf ( theta ) ); 
int y1 = nearbyint( radius * -sinf ( theta ) ); 						//invert so positive 'y' goes up the screen, i.e. pi/2 is top of the clock

st_aa_canvas_coord_tag oc;

oc.x0 = x1;
oc.y0 = y1;
op.vcrd.push_back( oc );

int cnt = 0;
while( 1 )
	{
	theta += step_val;

	if( theta >= twopi ) theta -= twopi;
	if( theta <= -twopi ) theta += twopi;


//	if( dir == 1 )
//		{
//		if( theta > stop_ang ) break;
//		}
//	else{
//		if( theta < stop_ang ) break;
//		}

	int x2 = nearbyint( radius * cosf( theta ) ); 
	int y2 = nearbyint( radius * -sinf( theta ) ); 								//invert so positive 'y' goes up the screen, i.e. pi/2 is top of the clock

//printf("aa_canvas::plot_arc_thick_rect() - x1 y1 %d %d %d %d\n", x1, y1, x2, y2 );

//	if( !one_rect_only ) plot_line_thick_bound_rect( layer, x1, y1, x2, y2, rr, gg, bb, num_pixels, rect_size, vr1 );
//	else plot_line_thick( layer, x1, y1, x2, y2, rr, gg, bb, num_pixels );

	oc.x0 = x2;
	oc.y0 = y2;
	op.vcrd.push_back( oc );

	x1 = x2;
	y1 = y2;

	cnt++;
	if( cnt >= segments ) break;
	}


//make encompassing rect
for( int i = 0; i < op.vcrd.size(); i++ )
	{
	oc = op.vcrd[i];
	
	if( oc.x0 < minx ) minx = oc.x0;
	if( oc.x0 > maxx ) maxx = oc.x0;
	if( oc.y0 < miny ) miny = oc.y0;
	if( oc.y0 > maxy ) maxy = oc.y0;


//printf("aa_canvas::ploygon_arc_build() - i %d  x1 y1 %d %d\n", i, oc.x0, oc.y0 );

	}



op.rectx = minx - rect_oversizex;	
op.recty = miny - rect_oversizey;	
op.ww = maxx - minx + 2*rect_oversizex;
op.hh = maxy - miny + 2*rect_oversizey;

op.cntr_x = op.rectx + op.ww / 2;
op.cntr_y = op.recty + op.hh / 2;


printf("aa_canvas::ploygon_arc_build() - op.rectx %d %d  ww %d %d\n", op.rectx, op.recty, op.ww, op.hh );

}














//text based file, one point coord per line
//'bkeep_coords_positive' if set, will shift polygon so its minx,miny coord is at: 0,0
//'rotate_theta' in radians, positive is anti-clockwise
//e.g: 'rect_oversizex' enlarges the bounding rectangle on either side of the final polygon's shape (in x direction), i.e 2x 'rect_oversizex'
bool aa_canvas::polygon_file_load( string sfname, st_aa_canvas_polygon_tag &op, float scle, float rotate_theta, bool bkeep_coords_positive, bool binvert_x, bool binvert_y, int rr, int gg, int bb, int rect_oversizex, int rect_oversizey )
{
bool vb = 0;
string s1;
mystr m1, m2;


bool b_point_or_line = 0;		//0: is one point per line (2 numbers per line), 1: is one line segment per line (4 numbers per line )


int minx = 1e6;
int maxx = -1e6;
int miny = 1e6;
int maxy = -1e6;

//read file
st_aa_canvas_coord_tag oc;
oc.rr = rr;
oc.gg = gg;
oc.bb = bb;

float c0 = cosf( rotate_theta );
float s0 = sinf( rotate_theta );

if( m1.readfile( sfname, 5000 ) )
	{
	op.vcrd.clear();
	vector<string> vstr0;
	m1.LoadVectorStrings( vstr0, '\n' );	
	int cnt = 0;
	for( int i = 0; i < vstr0.size(); i++ )
		{
		if(vb)printf("polygon_file_load() - vstr0[%d] '%s'\n", i, vstr0[i].c_str() );
		
		m2 = vstr0[i];
		
		m2.FindReplace( s1, "\n", "", 0 );
		m2 = s1;
		m2.FindReplace( s1, "\r", "", 0 );
		
		if( s1.length() == 0 ) continue;
		
		float x0 = 0;
		float y0 = 0;
		float x1 = 0;
		float y1 = 0;

		if( b_point_or_line == 0 )										//get points
			{
			sscanf( s1.c_str(), "%f %f", &x0, &y0 );

			if( binvert_x )
				{
				x0 = -x0;
				}
			if( binvert_y )
				{
				y0 = -y0;
				}

			oc.x0 = x0*scle*c0 + y0*scle*s0;
			oc.y0 = -x0*scle*s0 + y0*scle*c0;
			op.vcrd.push_back( oc );
			}

		if( b_point_or_line == 1 )										//get lines
			{
			sscanf( s1.c_str(), "%f %f %f %f", &x0, &y0, &x1, &y1 );
			if( binvert_x )
				{
				x0 = -x0;
				}
			if( binvert_y )
				{
				y0 = -y0;
				}
			oc.x0 = x0*scle*c0 + y0*scle*s0;
			oc.y0 = -x0*scle*s0 + y0*scle*c0;
			op.vcrd.push_back( oc );
			
			if( i == vstr0.size() - 1 )									//last line, read last point of last line ?
				{
				if( binvert_x )
					{
					x1 = -x1;
					}

				if( binvert_y )
					{
					y1 = -y1;
					}
				oc.x0 = x1*scle*c0 + y1*scle*s0;
				oc.y0 = -x1*scle*s0 + y1*scle*c0;
				op.vcrd.push_back( oc );
				}
			}
		cnt++;
		}

	if(vb)printf("polygon_file_load() - loaded %d points\n", cnt );

	//make encompassing rect
	for( int i = 0; i < op.vcrd.size(); i++ )
		{
		st_aa_canvas_coord_tag oc = op.vcrd[i];
		
		if( oc.x0 < minx ) minx = oc.x0;
		if( oc.x0 > maxx ) maxx = oc.x0;
		if( oc.y0 < miny ) miny = oc.y0;
		if( oc.y0 > maxy ) maxy = oc.y0;
		}
	
//	iww = maxx - minx;
//	ihh = maxy - miny;
	
//	op.ww = iww;
//	op.hh = ihh;

	op.rectx = minx - rect_oversizex;	
	op.recty = miny - rect_oversizey;	
	op.ww = maxx - minx + 2*rect_oversizex;
	op.hh = maxy - miny + 2*rect_oversizey;

	op.cntr_x = op.rectx + op.ww / 2;
	op.cntr_y = op.recty + op.hh / 2;


	if(vb)printf("polygon_file_load() - encompassing rect iww %d  ihh %d\n", op.ww, op.hh );

	if( bkeep_coords_positive )
		{
		for( int i = 0; i < op.vcrd.size(); i++ )
			{
			st_aa_canvas_coord_tag oc = op.vcrd[i];
			
			oc.x0 -= op.rectx;
			oc.y0 -= op.recty;

//			minx = 0;
//			miny = 0;
			op.vcrd[i] = oc;

			if(vb)printf("polygon_file_load() - final coord vals [%d]  x0 %d   y0 %d\n", i, oc.x0, oc.y0 );
			}
		op.rectx = 0;	
		op.recty = 0;	
		op.cntr_x = op.rectx + op.ww / 2;
		op.cntr_y = op.recty + op.hh / 2;
		}

	if(vb)
		{
		if(vb)printf("polygon_file_load() - op.rectx %d %d   op.ww %d %d   cntr_x %d %d\n", op.rectx, op.recty, op.ww, op.hh, op.cntr_x, op.cntr_y );
		for( int i = 0; i < op.vcrd.size(); i++ )
			{
			st_aa_canvas_coord_tag oc = op.vcrd[i];
			if(vb)printf("polygon_file_load() - final coord vals [%d]  x0 %d   y0 %d\n", i, oc.x0, oc.y0 );
			}
		}
	}
else{
	return 0;
	}

return 1;
}














void aa_canvas::rect_adj( st_aa_canvas_rect_tag &orct, int delta_posx, int delta_posy, int delta_ww, int delta_hh )
{
orct.x0 += delta_posx;
orct.y0 += delta_posy;
orct.ww += delta_ww;
orct.hh += delta_hh;
}








void aa_canvas::polygon_copy( st_aa_canvas_polygon_tag &op_src, st_aa_canvas_polygon_tag &op_dest )
{
op_dest = op_src;
/*
op_dest.posx = op_src.posx;
op_dest.posy = op_src.posy;
op_dest.ww = op_src.ww;
op_dest.hh = op_src.hh;
op_dest.scale = op_src.scale;
op_dest.rotate_theta = op_src.rotate_theta;
op_dest.vcrd = op_src.vcrd;
*/
}


//e.g: 'rect_oversizex' enlarges the bounding rectangle on either side of the final polygon's shape (in x direction), i.e 2x 'rect_oversizex'
void aa_canvas::polygon_rotate( st_aa_canvas_polygon_tag &op, float rotate_theta, float scle, int cntr_x, int cntr_y, int rect_oversizex, int rect_oversizey )
{

float c0 = cosf( rotate_theta );
float s0 = sinf( rotate_theta );

int minx = 1e6;
int maxx = -1e6;
int miny = 1e6;
int maxy = -1e6;

for( int i = 0; i < op.vcrd.size(); i++ )
	{
	st_aa_canvas_coord_tag oc = op.vcrd[i];
	
	float x0 = op.vcrd[i].x0 - cntr_x;
	float y0 = op.vcrd[i].y0 - cntr_y;

	oc.x0 = x0*scle*c0 + y0*scle*s0;
	oc.y0 = -x0*scle*s0 + y0*scle*c0;
	
	op.vcrd[i] = oc;

	if( oc.x0 < minx ) minx = oc.x0;
	if( oc.x0 > maxx ) maxx = oc.x0;
	if( oc.y0 < miny ) miny = oc.y0;
	if( oc.y0 > maxy ) maxy = oc.y0;
	}

op.rectx = minx - rect_oversizex;	
op.recty = miny - rect_oversizey;	
op.ww = maxx - minx + 2*rect_oversizex;
op.hh = maxy - miny + 2*rect_oversizey;
op.cntr_x = op.rectx + op.ww / 2;
op.cntr_y = op.recty + op.hh / 2;
}











//e.g: 'rect_oversizex' enlarges the bounding rectangle on either side of the final polygon's shape (in x direction), i.e 2x 'rect_oversizex'
void aa_canvas::polygon_update_rect( st_aa_canvas_polygon_tag &op, int rect_oversizex, int rect_oversizey )
{

int minx = 1e6;
int maxx = -1e6;
int miny = 1e6;
int maxy = -1e6;

for( int i = 0; i < op.vcrd.size(); i++ )
	{
	st_aa_canvas_coord_tag oc = op.vcrd[i];
		
	if( oc.x0 < minx ) minx = oc.x0;
	if( oc.x0 > maxx ) maxx = oc.x0;
	if( oc.y0 < miny ) miny = oc.y0;
	if( oc.y0 > maxy ) maxy = oc.y0;
	}

op.rectx = minx - rect_oversizex;	
op.recty = miny - rect_oversizey;	
op.ww = maxx - minx + 2*rect_oversizex;
op.hh = maxy - miny + 2*rect_oversizey;
op.cntr_x = op.rectx + op.ww / 2;
op.cntr_y = op.recty + op.hh / 2;



//printf("aa_canvas::polygon_update_rect() -  op.rectx %d %d op.ww %d %d\n", op.rectx,  op.recty, op.ww, op.hh );

}














//rect coords will include polygon position offset
//see also 'polygon_update_rect()'
void aa_canvas::polygon_get_bounding_rect_absolute( st_aa_canvas_polygon_tag &op, int rect_oversizex, int rect_oversizey, st_aa_canvas_bounding_rect_tag &orect )
{
orect.x0 = op.rectx + op.posx - rect_oversizex;	
orect.y0 = op.recty + op.posy - rect_oversizey;	
orect.x1 = orect.x0 + op.ww + 2*rect_oversizex;
orect.y1 = orect.y0 + op.hh + 2*rect_oversizey;
//printf("aa_canvas::polygon_get_bounding_rect_absolute() -  orect.x0 %d %d  orect.x1 %d %d\n", orect.x0,  orect.y0, orect.x1, orect.y1 );
}











bool aa_canvas::plot_rect_transparent_clamp( unsigned int layer, int x0, int y0, int ww, int hh, int rr, int gg, int bb )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;
if( st_aa[layer].bf == 0 ) return 0;


if( x0 < 0 ) x0 = 0;
if( y0 < 0 ) y0 = 0;

if( (x0 + ww) >= st_aa[layer].ww ) ww = st_aa[layer].ww - x0 - 1;

if( (y0 + hh) >= st_aa[layer].hh ) hh = st_aa[layer].hh - y0 - 1;


plot_line_transparent_clamp( layer, x0, y0, x0, y0 + hh, rr, gg, bb );

plot_line_transparent_clamp( layer, x0, y0 + hh, x0 + ww, y0 + hh, rr, gg, bb );

plot_line_transparent_clamp( layer, x0 + ww, y0 + hh, x0 + ww, y0, rr, gg, bb );

plot_line_transparent_clamp( layer, x0 + ww, y0, x0, y0, rr, gg, bb );

return 1;
}









bool aa_canvas::filter_boundary_gen( unsigned int layer, int* x0, int *x1, int lim_idx0, int lim_idx1, int kernel_size )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;
if( st_aa[layer].bf == 0 ) return 0;


return 1;
}








/*
bool aa_canvas::outline_build( unsigned int layer )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;
if( st_aa[layer].bf == 0 ) return 0;

st_aa_canvas_tag *o = st_aa + layer;

o->voutline.clear();


return 1;
}
*/





/*
//draw a region in spec layer
bool aa_canvas::plot_region( unsigned int layer, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, int rr, int gg, int bb )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;
if( st_aa[layer].bf == 0 ) return 0;


//printf("aa_canvas::line_plot()000 x1 %d %d %d %d\n", x1, y1, x2, y2 );
//if( st_aa[layer].lineclp->line_clip_int( x1, y1, x2, y2 ) );



float m0 = ((float)(x1 - x0))/(y1-y0); 
float b0 = x0 - m0*y0;							//x-intercept

float m1 = ((float)(x3 - x2))/(y3 - y2); 
float b1 = x2 - m1*y2;							//x-intercept

st_aa_canvas_tag *o = st_aa + layer;

o->voutline.clear();

//clear edge array
for( int yy = 0; yy < st_aa[layer].hh; yy++ )
	{
	o->a_x0[yy] = -1;
	o->a_x1[yy] = -1;
	}

int idx0 = -1;
int idx1 = -1;

//gen pixel edge location pairs
for( int yy = 0; yy < st_aa[layer].hh; yy++ )
	{
	if( ( yy >= y0 ) && ( yy <= y1 ) )
		{
		int x = m0*yy + b0;
		
		if( ( x >= 0 ) && ( x < st_aa[layer].ww ) )
			{
//			set_pixel_layer( layer, x, yy, rr, gg, bb );
			
			if( o->a_x0[yy] == -1 )
				{
				o->a_x0[yy] = x;											//make a record of how much of this raster line is to be set
				}
			else{
				o->a_x1[yy] = x;	
				}
			}

		x = m1*yy + b1;

		if( ( x >= 0 ) && ( x < st_aa[layer].ww ) )
			{
//			set_pixel_layer( layer, x, yy, rr, gg, bb );
			
			if( o->a_x0[yy] == -1 )
				{
				o->a_x0[yy] = x;											//make a record of how much of this raster line is to be set
				}
			else{
				o->a_x1[yy] = x;	
				}
			}

		}
	}

//set pixels using a horiz line for each edge pixel pair
for( int yy = 0; yy < st_aa[layer].hh; yy++ )
	{
	if( ( o->a_x0[yy] != -1 ) && ( o->a_x1[yy] != -1 ) )
		{
		
		int p0 = st_aa[layer].linedx * (st_aa[layer].hh - yy) * st_aa[layer].bytes_per_pixel   +  o->a_x0[yy] * st_aa[layer].bytes_per_pixel;

		st_aa_canvas_outline_pnt_tag op;
		op.x = o->a_x0[yy];
		op.y = yy;
		o->voutline.push_back( op );

		op.x = o->a_x1[yy];
		o->voutline.push_back( op );

		for( int i = o->a_x0[yy]; i <= o->a_x1[yy]; i++ )
			{
			st_aa[layer].bf[ p0++ ] = rr;
			st_aa[layer].bf[ p0++ ] = gg;
			st_aa[layer].bf[ p0++ ] = bb;

//			line_plot( layer, a_x0[yy], yy, a_x1[yy], yy, rr, gg, bb, st_aa[layer].linedx, 0 );
			}
		}

	}

	

//return 1;

//unordered_map<int, int> umap;


for( int i = 0; i < o->voutline.size(); i++ )
	{
	st_aa_canvas_outline_pnt_tag op = o->voutline[i];

//umap[i] = 0;
//	plot_pixel( 1, op.x, op.y, 255,255,255 );
	}
	
//printf("ZZZZZZZZZZZZZZZZZZZZ umap %d %d\n", umap.size(),  o->voutline.size() );

//outline_build( layer );

return 1;
}
*/

































//thick line plot
bool aa_canvas::plot_polygon_thick_with_rect( unsigned int layer, st_aa_canvas_polygon_tag &op, int offsx, int offsy, float scale, int num_pixels, int rr, int gg, int bb, int rect_size, bool one_rect_only, vector<st_aa_canvas_bounding_rect_tag> &vr )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;
if( st_aa[layer].bf == 0 ) return 0;

vr.clear();

if( op.vcrd.size() < 2 )  return 0;

if( rect_size < 4 ) rect_size = 4;

int minx = 1e6;
int maxx = -1e6;
int miny = 1e6;
int maxy = -1e6;


int x0 = nearbyint( op.vcrd[0].x0 * scale + op.posx + offsx );
int y0 = nearbyint( op.vcrd[0].y0 * scale + op.posy + offsy );

vector<st_aa_canvas_bounding_rect_tag> vr1;

for( int i = 1; i < op.vcrd.size(); i++ )
	{
	
	int x1 = nearbyint( op.vcrd[i].x0 * scale + op.posx + offsx );
	int y1 = nearbyint( op.vcrd[i].y0 * scale + op.posy + offsy );
	
	plot_line_thick_bound_rect( layer, x0, y0, x1, y1, rr, gg, bb, num_pixels, rect_size, vr1 );



	if( one_rect_only )
		{
		if( x0 < minx ) minx = x0;
		if( x0 > maxx ) maxx = x0;
		if( y0 < miny ) miny = y0;
		if( y0 > maxy ) maxy = y0;

		if( x1 < minx ) minx = x1;
		if( x1 > maxx ) maxx = x1;
		if( y1 < miny ) miny = y1;
		if( y1 > maxy ) maxy = y1;
		}

		
	if( !one_rect_only) vr.insert(vr.end(), vr1.begin(), vr1.end());
	
	x0 = x1;
	y0 = y1;
	}


if( one_rect_only )
	{
	st_aa_canvas_bounding_rect_tag orect;


	orect.x0 = minx - rect_size;	
	orect.y0 = miny - rect_size;	
	orect.x1 = maxx + rect_size;
	orect.y1 = maxy + rect_size;
	vr.push_back( orect );
	}

return 1;
}










//polygon rasterizer with fill
//refer: https://alienryderflex.com/polygon_fill/
//  public-domain code by Darel Rex Finley, 2007

bool aa_canvas::plot_polygon_filled( unsigned int layer, st_aa_canvas_polygon_tag &op, int offsx, int offsy, float scale, int rr, int gg, int bb )
{
if( layer >= cn_aa_canvas_layer_max ) return 0;
if( st_aa[layer].bf == 0 ) return 0;

#define MAX_POLY_CORNERS 1024

int  nodes, nodeX[MAX_POLY_CORNERS], pixelX, pixelY, i, j, swap ;

float polyX[MAX_POLY_CORNERS];
float polyY[MAX_POLY_CORNERS];

int IMAGE_LEFT = 0;
int IMAGE_RIGHT = st_aa[layer].ww;

int IMAGE_TOP = 0;
int IMAGE_BOT = st_aa[layer].hh;


if( op.vcrd.size() <= 1 ) return 0;

bool b_scle = 0;
if( scale != 1.0f ) b_scle = 1;

if( b_scle )
	{
	for( int i = 0; i < op.vcrd.size(); i++ )
		{
		if( i >= MAX_POLY_CORNERS ) break;
		
		polyX[i] = op.vcrd[i].x0 * scale + op.posx + offsx;
		polyY[i] = op.vcrd[i].y0 * scale + op.posy + offsy;
		}
	}
else{
	for( int i = 0; i < op.vcrd.size(); i++ )
		{
		if( i >= MAX_POLY_CORNERS ) break;
		
		polyX[i] = op.vcrd[i].x0 + op.posx + offsx;
		polyY[i] = op.vcrd[i].y0 + op.posy + offsx;
		}
	}

int polyCorners = op.vcrd.size();

/*
int polyCorners = 4;
polyX[0] = 20;
polyY[0] = 20;

polyX[1] = 20;
polyY[1] = 120;

polyX[2] = 200;
polyY[2] = 200;

polyX[3] = 300;
polyY[3] = 25;
*/

//  Loop through the rows of the image.
for (pixelY=IMAGE_TOP; pixelY<IMAGE_BOT; pixelY++) {

  //  Build a list of nodes.
  nodes=0; j=polyCorners-1;
  for (i=0; i<polyCorners; i++) {
    if (polyY[i]<(double) pixelY && polyY[j]>=(double) pixelY
    ||  polyY[j]<(double) pixelY && polyY[i]>=(double) pixelY) {
      nodeX[nodes++]=(int) (polyX[i]+(pixelY-polyY[i])/(polyY[j]-polyY[i])
      *(polyX[j]-polyX[i])); }
    j=i; }

  //  Sort the nodes, via a simple “Bubble” sort.
  i=0;
  while (i<nodes-1) {
    if (nodeX[i]>nodeX[i+1]) {
      swap=nodeX[i]; nodeX[i]=nodeX[i+1]; nodeX[i+1]=swap; if (i) i--; }
    else {
      i++; }}

  //  Fill the pixels between node pairs.
  for (i=0; i<nodes; i+=2) {
    if   (nodeX[i  ]>=IMAGE_RIGHT) break;
    if   (nodeX[i+1]> IMAGE_LEFT ) {
      if (nodeX[i  ]< IMAGE_LEFT ) nodeX[i  ]=IMAGE_LEFT ;
      if (nodeX[i+1]> IMAGE_RIGHT) nodeX[i+1]=IMAGE_RIGHT;
      for (pixelX=nodeX[i]; pixelX<nodeX[i+1]; pixelX++) /*fillPixel(pixelX,pixelY)*/plot_pixel( layer, pixelX, pixelY, rr, gg, bb ); 
      }
      }
      }
 return 1;
}


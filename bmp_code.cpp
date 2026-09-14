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


//bmp_code.cpp

//v1.01		10-may-2018		//refer https://raw.githubusercontent.com/MalcolmMcLean/babyxrc/master/src/bmp.c'

//v1.02		21-jul-2023		//fixed some bugs involving how bmp padding bytes at end of each line were being skipped over in 'loadbmp()', was coded incorrectly and noticable only
							//if loading a bmp who's image width was not at multiples of 4, was causing a skewed image being place into returned memory buf ptr in 'loadbmp()'
							//the bmp padding bytes are used to keep the first pixel of each line on a 4 byte boundary if it's not by default at that point,
							//added: param 'b_rgb_ordering' to 'loadbmp()', it's set to '1' by default, so 'r,g'b' ordering is stored in memory, set to '0' for 'b,g,r' packing

//v1.03		22-jul-2023		//added namespace 'bmp_code::'				
//v1.04		11-feb-2025		//debugging, added more 'printf()' info	to show image details and 'malloc()' size allocation		

#include "bmp_code.h"


//this code has some of my code (first, after example code) and some of Malcolm McLean's bmp loading/saving code,
//BE AWARE that Malcolm McLean's would load bmp into memory with 'b,g,r' packing, added default flag 'b_rgb_ordering' set to 1, so packing is now 'r,g,b' 

//MAKE SURE you free the malloc pointer supplied with: 'loadbmp()'


//BE AWARE this code is NOT a full implementation, it ONLY handles bmp headers that are 12 or 40 bytes in size,


//also an example or a blur filter for bitmap images: 'blur_bitmap()'




/* 

// ------------------ EXAMPLE code for Malcolm McLean's bmp loading/saving code -------------------------------------
string fname = "zzzanvil_2.bmp";

int dx, dy;
unsigned char *pp = loadbmp( (char*)fname.c_str(), &dx, &dy );			// !!! YOU MUST free 'loadbmp()' supplied malloc pointer


if( pp == 0 )
	{
	printf("loadbmp() - failed to read bitmap: '%s'\n", fname.c_str() );
	return;
	}
printf( "bmp image size: dx: %d, dy: %d, '%s'\n", dx, dy, fname.c_str() );




//simple bpm load and save

unsigned char* pp2 = new unsigned char[ dx * dy * 3 ];

// ---- just copies r,g,b from buf to buf ----
for( int y = 0; y < dy; y++ )
	{
	for( int x = 0; x < dx; x++ )
		{
		int ps = y * dx * 3 + x * 3;
		int pd = y * dx * 3 + x * 3;

		int r = pp[ ps++ ];
		int g = pp[ ps++ ];
		int b = pp[ ps ];

		pp2[ pd++ ] = r;
		pp2[ pd++ ] = g;
		pp2[ pd ] = b;
		}
	}

fname = "zzzzanvil_2.bmp";

if( savebmp( (char*)fname.c_str(), pp2, dx, dy ) == -1 )
	{
	printf("savebmp() - failed to save bitmap: '%s'\n", fname.c_str() );
	}

//--------------------------------------------------------------------------



//------------------ spins image 90 degrees clockwise -----------------------
for( int y = 0; y < dy; y++ )
	{
	for( int x = 0; x < dx; x++ )
		{
		int ps = y * dx * 3 + x * 3;
		int pd = (dy - y) * 3   +  x * dy * 3;				// !!!!spin dest buf addressing 90 degrees

		int r = pp[ ps++ ];
		int g = pp[ ps++ ];
		int b = pp[ ps ];

		pp2[ pd++ ] = r;
		pp2[ pd++ ] = g;
		pp2[ pd ] = b;
		}
	}

fname = "zzzzzanvil_2.bmp";

if( savebmp( (char*)fname.c_str(), pp2, dy, dx ) == -1 )	// !!!!spin dest buf addressing 90 degrees
	{
	printf("savebmp() - failed to save bitmap: '%s'\n", fname.c_str() );
	}
//--------------------------------------------------------------------------




free( pp );				// !!! YOU MUST free 'loadbmp()' supplied malloc pointer

delete[] pp2;

return;
//----------------------------------------------------------------------------------------------------------------------
*/





/*
//-------------------------- simple bitmap blur filter------------------------------------------------------------------

double bitmap_sinx_on_x(double x)
{
if (x != 0)
	{
	x *= M_PI;
	return ( sin(x)/x);
	}
return 1;
}



//lanczos radius 3.0
float bitmap_calc_filter_val( int filter_type, double x )
{
double ret = 0;
double temp;
const double C = 1.0/3.0;

switch( filter_type )
	{

	case 0:
		if (x < 0) x = - x;
		if (x < 3) { ret= ( bitmap_sinx_on_x( x ) * bitmap_sinx_on_x( x / 3.0 ) ); break;}
		ret = 0;
	break;

	default:
		printf("calc_filter_val() - filter type not found: %d\n" , filter_type );
	}

return ret;
}







//simple blur filter convolution example, MUST be a bitmap formatted image file
//use -O3 optimisation
//use this to see asm dump of code:  objdump -d -M intel -S cache_code.o

bool blur_bitmap( string fname_src, string fname_dest )
{
mystr m1;

int dx;
int dy;

unsigned int *bufsrc = loadbmp_3_integer_rgb( fname_src.c_str(), dx, dy );				// !!!  DONT forget to free allocated buf

if( bufsrc == 0 )
	{
	printf("loadbmp_3_integer_rgb() - failed to load bitmap: '%s'\n", fname_src.c_str() );
	return 0;
	}

int *bufdest = new int[ dx * dy * 3 ];

int kern_size = 16;

int *kern = new int[ kern_size ];


float filter_rad = 3 / 3.0;			// divide 3 for more blur, it stretches the sinc waveform, so more of surrounding pixel add to interpolated pixel

float x = -filter_rad;
float step = filter_rad * 2.0 / kern_size;

int wght_sum = 0;
for( int i = 0; i < kern_size; i++ )
	{
	kern[ i ] = nearbyint( bitmap_calc_filter_val( 0, x ) * 0x7ff );

 	wght_sum += kern[ i ];

	printf("kern[ %d ]: x: %.6f, coeff: %d   %.6f\n", i, x, kern[ i ], bitmap_calc_filter_val( 0, x ) );
	x += step;
	}

printf("wght_sum: %d\n", wght_sum );


m1.time_start( m1.ns_tim_start );


//horiz filter
for( int y = 0; y < dy; y++ )
	{
	for( int x = 0; x < dx; x++ )
		{
		int sum_r = 0;
		int sum_g = 0;
		int sum_b = 0;
		int pd = y * dx * 3 + x * 3;

		int offs = -kern_size / 2;
		for( int k = 0; k < kern_size; k++ )
			{
			int xoffs = x + offs;
			if (xoffs < 0) xoffs = 0;
			if (xoffs >= dx) xoffs = dx - 1;

			int ps = y * dx * 3 + xoffs * 3;

			int kk = kern[ k ];

			sum_r += bufsrc[ ps++ ] * kk;
			sum_g += bufsrc[ ps++ ] * kk;
			sum_b += bufsrc[ ps ] * kk;

			offs++;
			}

		sum_r /= wght_sum;
		sum_g /= wght_sum;
		sum_b /= wght_sum;


		if ( sum_r > 0xff ) sum_r = 0xff;
		if ( sum_r < 0 ) sum_r = 0;

		if ( sum_g > 0xff ) sum_g = 0xff;
		if ( sum_g < 0 ) sum_g = 0;

		if ( sum_b > 0xff ) sum_b = 0xff;
		if ( sum_b < 0 ) sum_b = 0;


		bufdest[ pd++ ] = sum_r;
		bufdest[ pd++ ] = sum_g;
		bufdest[ pd ] = sum_b;
		}
	}

//vert filter
for( int y = 0; y < dy; y++ )
	{
	for( int x = 0; x < dx; x++ )
		{
		int sum_r = 0;
		int sum_g = 0;
		int sum_b = 0;
		int pd = y * dx * 3 + x * 3;

		int offs = -kern_size / 2;
		for( int k = 0; k < kern_size; k++ )
			{
			int yoffs = y + offs;
			if (yoffs < 0) yoffs = 0;
			if (yoffs >= dy) yoffs = dy - 1;

			int ps = yoffs * dx * 3 + x * 3;

			int kk = kern[ k ];

			sum_r += bufdest[ ps++ ] * kk;
			sum_g += bufdest[ ps++ ] * kk;
			sum_b += bufdest[ ps ] * kk;
			offs++;
//	printf("kern[ %d ]: %d\n", k, kern[ kern_idx * kern_size + k ] );
			}

		sum_r /= wght_sum;
		sum_g /= wght_sum;
		sum_b /= wght_sum;

		if ( sum_r > 0xff ) sum_r = 0xff;
		if ( sum_r < 0 ) sum_r = 0;

		if ( sum_g > 0xff ) sum_g = 0xff;
		if ( sum_g < 0 ) sum_g = 0;

		if ( sum_b > 0xff ) sum_b = 0xff;
		if ( sum_b < 0 ) sum_b = 0;


//			sum_r /= 0x7ff;
//			sum_g /= 0x7ff;
//			sum_b /= 0x7ff;

		bufdest[ pd++ ] = sum_r;
		bufdest[ pd++ ] = sum_g;
		bufdest[ pd ] = sum_b;
		}
	}


double dt = m1.time_passed( m1.ns_tim_start );

printf("timing: %.6f secs\n", dt );




int ret = 1;
if( savebmp_3_integer_rgb( (char*)fname_dest.c_str(), (unsigned int*)bufdest, dx, dy ) == 0 )
	{
	printf("savebmp_3_integer_rgb() - failed to save bitmap: '%s'\n", fname_dest.c_str() );
	ret = 0;
	}


free( bufsrc );
delete[] bufdest;
delete[] kern;

return ret;
}

//----------------------------------------------------------------------------------------------------------------------
*/






namespace bmp_code
{









//---------------- additional code to MalcolmMcLean's routines ---------------

//make a bitmap image file from supplied pixel array
bool save_bmp( string fname, st_img &img )
{
bool retval = 1;
mystr m1;

int px_cnt = 0;
int fsize = 0;


if( img.vline.size() == 0 ) return 0;		//simple checking
if( img.vline[ 0 ].vpixel.size() == 0 ) return 0;

//BITMAPFILEHEADER is 14 bytes
//BITMAPINFOHEADER is 40 bytes

unsigned char bmp_hdr[ 14 + 40 ];

for( int i = 0; i < sizeof( bmp_hdr ); i++ ) bmp_hdr[ i ] = 0;	//clear hdr



vector<unsigned char>vpxlarray;			//vector holding pixel in seq order

int sizex = img.vline[ 0 ].vpixel.size();		//get x size from 1st line;
int sizey = img.vline.size();

//work out how many bytes to add to each line to keep on 4 byte boundary
int bytes_line = 3 * sizex;
int low_bits = bytes_line  & 0x3;
int add_bytes = 0;
if( low_bits != 0 ) add_bytes = 4 - low_bits;



//make seq pixel array, last line goes first
for( int y = 0; y < img.vline.size(); y++ )
	{
	
	for( int x = 0; x < img.vline[ y ].vpixel.size(); x++ )
		{
		//last line goes first
		vpxlarray.push_back( img.vline[ sizey - y - 1 ].vpixel[ x ].b ); 	//store blue pixel val
		vpxlarray.push_back( img.vline[ sizey - y - 1 ].vpixel[ x ].g ); 	//store green
		vpxlarray.push_back( img.vline[ sizey - y - 1 ].vpixel[ x ].r ); 	//store red

		}

	for ( int i = 0; i < add_bytes; i++ )  vpxlarray.push_back( 0 ); //add bytes to put on 4 byte boundary
	}


fsize = 14 + 40 +  vpxlarray.size();		//filesize = BITMAPFILEHEADER + BITMAPINFOHEADER + pixelarray


//printf("save_bmp() - sizex= %d, sizey= %d\n", sizex, sizey );

bmp_hdr[ 0 ] = 'B';
bmp_hdr[ 1 ] = 'M';
bmp_hdr[ 2 ] = fsize & 0xff;
bmp_hdr[ 3 ] = ( fsize >> 8 ) & 0xff;
bmp_hdr[ 4 ] = ( fsize >> 16 ) & 0xff;
bmp_hdr[ 5 ] = ( fsize >> 24 ) & 0xff;
bmp_hdr[ 0x0a ] = 54;						//offset to pixel array

bmp_hdr[ 0x0e ] = 40;						//length of  BITMAPINFOHEADER

bmp_hdr[ 0x12 ] = sizex & 0xff;				//sizex
bmp_hdr[ 0x13 ] = ( sizex >> 8 ) & 0xff;
bmp_hdr[ 0x14 ] = ( sizex >> 16 ) & 0xff;
bmp_hdr[ 0x15 ] = ( sizex >> 24 ) & 0xff;


bmp_hdr[ 0x16 ] = sizey & 0xff;				//sizey
bmp_hdr[ 0x17 ] = ( sizey >> 8 ) & 0xff;
bmp_hdr[ 0x18 ] = ( sizey >> 16 ) & 0xff;
bmp_hdr[ 0x19 ] = ( sizey >> 24 ) & 0xff;

bmp_hdr[ 0x1a ] = 1;						//colour planes
bmp_hdr[ 0x1c ] = 24;						//bits per pixel


int rawsize = vpxlarray.size();

bmp_hdr[ 0x22 ] = rawsize & 0xff;			//size of raw bitmap data count
bmp_hdr[ 0x23 ] = ( rawsize >> 8 ) & 0xff;
bmp_hdr[ 0x24 ] = ( rawsize >> 16 ) & 0xff;
bmp_hdr[ 0x25 ] = ( rawsize >> 24 ) & 0xff;

int wrcnt = 0;
int written;
bool done;
unsigned char ucbuf[ 8192 ];
int i = 0;

int test = 0;

FILE *fpw = m1.mbc_fopen( fname, "wb" );
if( fpw == 0 )
	{
	printf( "save_bmp() - failed to create write file: '%s'\n", fname.c_str()  );
	retval = 0;
	goto failed_1;
	}

//write headers
written = fwrite( bmp_hdr, 1, sizeof( bmp_hdr ), fpw );
if( written == 0 )
	{
	printf( "save_bmp() - fwrite failed at: %d\n", wrcnt );
	retval = 0;
	goto failed_1;
	}

wrcnt += written;

done = 0;


//write pixel array

while( 1 )
	{
	int cnt = 0;
	for( ; i < vpxlarray.size(); i++ )
		{
		ucbuf[ cnt ] = vpxlarray[ i ];
		
		if( cnt >=  ( sizeof( ucbuf ) - 1 ) ) goto do_write;
		cnt++;
		}
	done = 1;

do_write:
//printf( "test:  %d bytes, i=%d\n", test, i );

	written = fwrite( ucbuf, 1, cnt, fpw );
	if( written == 0 )
		{
		printf( "save_bmp() - fwrite failed at: %d\n", wrcnt );
		retval = 0;
		goto failed_1;
		}
	wrcnt += written;

	if ( done ) break;
	}

//printf( "wrote file: '%s',  %d bytes\n",  fname.c_str(), wrcnt );

//printf( "test:  %d bytes, i=%d\n", test, i );

failed_1:


if( fpw ) fclose( fpw );


return retval;

}





//e.g: xxbbggrr - 32 bit integer colour, alpha is NOT saved
//'linedx' is the number of pixels in a horiz line, most cases this will be the same as 'dx'
bool save_bmp_integer_24bit_colour( string fname, unsigned int *buf, int dx, int dy, int linedx )
{
st_img img;
st_bmp_code_pixel pxl;
st_bmp_code_pix_line line;

for( unsigned int y = 0; y < dy; y++ )
	{
	line.vpixel.clear();

	for( unsigned int x = 0; x < dx; x++ )
		{
		int ps = y * linedx + x;

		int pixel = buf[ ps ];
		int r = pixel & 0x000000ff;
		int g = ( pixel >> 8 ) & 0x000000ff;
		int b = pixel >> 16;

		pxl.r = r;
		pxl.g = g;
		pxl.b = b;
		line.vpixel.push_back( pxl );
		}
	img.vline.push_back( line );
	}

if( save_bmp( fname, img ) ) return 1;
return 0;
}







//e.g: rr,gg,bb	- unsigned char colour
//'linedx' is the number of pixels in a horiz line, most cases this will be the same as 'dx'
bool save_bmp_uchar_24bit_colour( string fname, unsigned char *buf, int dx, int dy, int linedx )
{
st_img img;
st_bmp_code_pixel pxl;
st_bmp_code_pix_line line;

for( unsigned int y = 0; y < dy; y++ )
	{
	line.vpixel.clear();

	for( unsigned int x = 0; x < dx; x++ )
		{
		int ps = y * linedx * 3 + x * 3;

		int r = buf[ ps++ ];
		int g = buf[ ps++ ];
		int b = buf[ ps ];

		pxl.r = r;
		pxl.g = g;
		pxl.b = b;
		line.vpixel.push_back( pxl );
		}
	img.vline.push_back( line );
	}

if( save_bmp( fname, img ) ) return 1;
return 0;
}
//-----------------------------------------------------------------------------








//load a windows BMP formatted image file, uses Malcolm McLean's bmp routines,
//an malloc'ated buffer will returned holding pixels in 3 integer colour format: 000000rr, 000000gg, 000000bb,    !!!! DONT forget to free buffer
//returns malloc'ated buffer on success, else 0
unsigned int* loadbmp_3_integer_rgb( const char* fname, int &dx, int &dy )
{


unsigned char *pp = loadbmp( (char*)fname, &dx, &dy );			// !!! YOU MUST free 'loadbmp()' supplied malloc pointer

if( pp == 0 ) return 0;

unsigned int* buf = (unsigned int*) malloc( dx * dy * sizeof(int) * 3 );

if( buf == 0 ) return 0;

//go from unsigned char colour: rr, gg, bb, to 3 integer colour: 000000rr, 000000gg, 000000bb  
for( int y = 0; y < dy; y++ )
	{
	for( int x = 0; x < dx; x++ )
		{
		int ps = y * dx * 3 + x * 3;
		int pd = y * dx * 3 + x * 3;

		int r = pp[ ps++ ];
		int g = pp[ ps++ ];
		int b = pp[ ps ];

		
		buf[ pd++ ] = r;
		buf[ pd++ ] = g;
		buf[ pd ] = b;

		}
	}

return buf;
}






//save a windows BMP formatted image file, uses Malcolm McLean's bmp routines,
//src buffer colour needs to be in 3 integer format: 000000rr, 000000gg, 000000bb
//returns 1 on success, else 0
bool savebmp_3_integer_rgb( const char* fname, unsigned int *bufsrc, int dx, int dy )
{

unsigned char* bufdest = new unsigned char[ dx * dy * 3 ];

if( bufdest == 0 ) return 0;


//go from 3 integer colour: 000000rr, 000000gg, 000000bb, to unsigned char colour: rr, gg, bb
for( int y = 0; y < dy; y++ )
	{
	for( int x = 0; x < dx; x++ )
		{
		int ps = y * dx * 3 + x * 3;
		int pd = y * dx * 3 + x * 3;

		int r = bufsrc[ ps++ ];
		int g = bufsrc[ ps++ ];
		int b = bufsrc[ ps ];

		bufdest[ pd++ ] = r;
		bufdest[ pd++ ] = g;
		bufdest[ pd ] = b;
		}
	}

if( savebmp( (char*)fname, bufdest, dx, dy ) == -1 )
	{
	free( bufdest );
	return 0;
	}

free( bufdest );

return 1;
}







//load a windows BMP formatted image file, uses Malcolm McLean's bmp routines,
//it 'aligned_alloc'ates 3x 'int16_t' buffers holding pixels for each colour:    				!!!! DONT forget to free buffer
//the buffers are aligned_alloc'ted on a 16 byte boundary suitable for simd/sse processsing, don't forget to free buffers
//returns 1 on success, else 0
bool loadbmp_3_int16_t( const char* fname, int16_t **bf_r, int16_t **bf_g, int16_t **bf_b, int &dx, int &dy )
{

unsigned char *pp = loadbmp( (char*)fname, &dx, &dy );			// !!! YOU MUST free 'loadbmp()' supplied malloc pointer

if( pp == 0 ) return 0;

*bf_r = (int16_t*) aligned_alloc( 16, dx * dy * sizeof(int16_t) );
*bf_g = (int16_t*) aligned_alloc( 16, dx * dy * sizeof(int16_t) );
*bf_b = (int16_t*) aligned_alloc( 16, dx * dy * sizeof(int16_t) );

if( ( bf_r == 0 ) | ( bf_g == 0 ) | ( bf_b == 0 ) )
	{
	if( bf_r != 0 ) free( bf_r );
	if( bf_g != 0 ) free( bf_g );
	if( bf_b != 0 ) free( bf_b );
	return 0;
	}

//go from unsigned char colour: rr, gg, bb,   to   3 buffers hold 3 colours 
for( int y = 0; y < dy; y++ )
	{
	for( int x = 0; x < dx; x++ )
		{
		int ps = y * dx * 3 + x * 3;
		int pd = y * dx + x;

		int r = pp[ ps++ ];
		int g = pp[ ps++ ];
		int b = pp[ ps ];

		(*bf_r)[ pd ] = r;
		(*bf_g)[ pd ] = g;
		(*bf_b)[ pd ] = b;
		}
	}

free( pp );
return 1;
}








//save a windows BMP formatted image file, uses Malcolm McLean's bmp routines,
//3 supplied 'int16_t' buffers must hold 3 colours,
//returns 1 on success, else 0
bool savebmp_3_int16_t( const char* fname, int16_t *bf_r, int16_t *bf_g, int16_t *bf_b, int dx, int dy )
{

unsigned char* bufdest = new unsigned char[ dx * dy * 3 ];

if( bufdest == 0 ) return 0;

//go from 3 integer colour: 000000rr, 000000gg, 000000bb, to unsigned char colour: rr, gg, bb
for( int y = 0; y < dy; y++ )
	{
	for( int x = 0; x < dx; x++ )
		{
		int ps = y * dx + x;
		int pd = y * dx * 3 + x * 3;

		int r = bf_r[ ps ];
		int g = bf_g[ ps ];
		int b = bf_b[ ps ];

		bufdest[ pd++ ] = r;
		bufdest[ pd++ ] = g;
		bufdest[ pd ] = b;
		}
	}

if( savebmp( (char*)fname, bufdest, dx, dy ) == -1 )
	{
	free( bufdest );
	return 0;
	}

free( bufdest );

return 1;
}








//-----------------------------------------------------------------------------

//refer https://raw.githubusercontent.com/MalcolmMcLean/babyxrc/master/src/bmp.c

/*********************************************************
* bmp.c - Microsoft bitmap loading functions.            *
* By Malcolm McLean                                      *
* This is a file from the book Basic Algorithms by       *
*   Malcolm McLean                                       *
*********************************************************/
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
  int width;     
  int height;
  int bits;
  int upside_down; /* set for a bottom-up bitmap */
  int core;        /* set if bitmap is old version */
  int palsize;     /* number of palette entries */
  int compression; /* type of compression in use */
} BMPHEADER;

int bmpgetinfo(char *fname, int *width, int *height);
unsigned char *loadbmp(char *fname, int *width, int *height);
unsigned char *loadbmp8bit(char *fname, int *width, int *height, unsigned char *pal);
unsigned char *loadbmp4bit(char *fname, int *width, int *height, unsigned char *pal);

static void loadpalette(FILE *fp, unsigned char *pal, int entries);
static void loadpalettecore(FILE *fp, unsigned char *pal, int entries);
static void savepalette(FILE *fp, unsigned char *pal, int entries);
static int loadheader(FILE *fp, BMPHEADER *hdr);
static void saveheader(FILE *fp, int width, int height, int bits);
static void loadraster8(FILE *fp, unsigned char *data, int width, int height);
static void loadraster4(FILE *fp, unsigned char *data, int width, int height);
static void loadraster1(FILE *fp, unsigned char *data, int width, int height);
static long getfilesize(int width, int height, int bits);
static void swap(void *x, void *y, int len);
static void fput32le(long x, FILE *fp);
static void fput16le(int x, FILE *fp);
static long fget32le(FILE *fp);
static int fget16le(FILE *fp);


/********************************************************		
* bmpgetinfo() - get information about a BMP file.      *
* Params: fname - name of the .bmp file.                *
*         width - return pointer for image width.       *
*         height - return pointer for image height.     *
* Returns: bitmap type.                                 *
*           0 - not a valid bitmap.                     *
*           1 - monochrome paletted (2 entries).        *
*           4 - 4-bit paletted.                         *
*           8 - 8 bit paletted.                         *
*           16 - 16 bit rgb.                            *
*           24 - 24 bit rgb.                            *
*           32 - 24 bit rgb with 1 byte wasted.         *
********************************************************/                             
int bmpgetinfo(char *fname, int *width, int *height)
{
  FILE *fp;
  BMPHEADER bmphdr;
  fp = fopen(fname, "rb");
  if(!fp)
	return 0;
  if(loadheader(fp, &bmphdr) == -1)
  {
    fclose(fp);
	return 0;
  }
  fclose(fp);
  if(width)
	*width = bmphdr.width;
  if(height)
	*height = bmphdr.height;
  return bmphdr.bits;
}







// !!!!! gc - by default 'b_rgb_ordering' flag is set to 1, so 'r,g,b' packing is used when storing into memory, set this flag to 0 for 'b,g,r' ordering

//MAKE SURE you free the malloc pointer supplied with: 'loadbmp()'

/************************************************************
* loadbmp() - load any bitmap                               *
* Params: fname - pointer to file path                      *
*         width - return pointer for image width            *
*         height - return pointer for image height          *
* Returns: malloced pointer to image, 0 on fail             *
************************************************************/


unsigned char *loadbmp(char *fname, int *width, int *height, bool b_rgb_ordering )
{
bool vb = 1;

  FILE *fp;
  BMPHEADER bmpheader;
  unsigned char *answer;
  unsigned char *raster = 0;
  unsigned char pal[256 * 3];
  int index;
  int col;
  int target;
  int i;
  int ii;


  fp = fopen(fname, "rb");
  if(!fp)
	{
if(vb)printf( "bmp_code.cpp loadbmp() - failed to open file: '%s'\n", fname );
	return 0;
	}

  if(loadheader(fp, &bmpheader) == -1)
  {
if(vb)printf( "bmp_code.cpp loadbmp() - call to loadheader() failed\n" );
    fclose(fp);
	return 0;
  }
  if(bmpheader.bits == 0 || bmpheader.compression != 0)
  {
if(vb)printf( "bmp_code.cpp loadbmp() - failed, bmpheader.bits==0 (was %d) or bmpheader.compression!=0 (was %d)\n", bmpheader.bits,  bmpheader.compression );
	fclose(fp);
	return 0;
  }

  answer = (unsigned char *)malloc(bmpheader.width * bmpheader.height * 3);
  if(!answer)
  {
if(vb)printf( "bmp_code.cpp loadbmp() - failed, malloc()\n" );
   fclose(fp);
	return 0;
  }
else{
if(vb)printf( "bmp_code.cpp loadbmp() -  malloc() size is %d x %d (3 bytes per pixel):  %d\n", bmpheader.width,  bmpheader.height, bmpheader.width * bmpheader.height * 3 );	//v1.04
	}

  if(bmpheader.bits < 16)
  {
    raster = (unsigned char *)malloc(bmpheader.width * bmpheader.height);
	if(!raster)
	{
if(vb)printf( "bmp_code.cpp loadbmp() - failed, malloc()\n" );
	  free(answer);
	  return 0;
	}
else{
if(vb)printf( "bmp_code.cpp loadbmp() -  malloc() size is %d x %d (1 byte per pixel):  %d\n", bmpheader.width, bmpheader.height, bmpheader.width * bmpheader.height );	//v1.04
	}
	
  }


//printf( "loadbmp() - bmpheader.bits %d\n", bmpheader.bits );


int ptr_rd = 0;															//v1.02, used to skip over padding bytes, see below	

  switch(bmpheader.bits)
  {
    case 1:
if(vb)printf( "loadbmp() - num of bits: %d\n", bmpheader.bits );
	  if(bmpheader.core)
		loadpalettecore(fp, pal, 2);
	  else
		loadpalette(fp, pal, bmpheader.palsize);

	  loadraster1(fp, raster, bmpheader.width, bmpheader.height);
	  break;
	case 4:
if(vb)printf( "loadbmp() - num of bits: %d\n", bmpheader.bits );
	  if(bmpheader.core)
		loadpalettecore(fp, pal, 256);
	  else
		loadpalette(fp, pal, bmpheader.palsize);

	  loadraster4(fp, raster, bmpheader.width, bmpheader.height);
	  break;
	 
	case 8:
if(vb)printf( "loadbmp() - num of bits: %d\n", bmpheader.bits );

	  if(bmpheader.core)
        loadpalettecore(fp, pal, 256);
      else
	    loadpalette(fp, pal, bmpheader.palsize);
		
	  loadraster8(fp, raster, bmpheader.width, bmpheader.height);
	  break;
	  
	case 16:
if(vb)printf( "loadbmp() - num of bits: %d\n", bmpheader.bits );

      for(i=0;i<bmpheader.height;i++)
	  {
	    for(ii=0;ii<bmpheader.width;ii++)
		{
		  target = (i * bmpheader.width * 3) + ii * 3;
		  col = fget16le(fp);
//		  answer[target] = (col & 0x001F) << 3;
//		  answer[target+1] = (col & 0x03E0) >> 2;
//		  answer[target+2] = (col & 0x7A00) >> 7;

		if( b_rgb_ordering )											//v1.02
			{
			answer[target+2] = (col & 0x001F) << 3;						//v1.02
			answer[target+1] = (col & 0x03E0) >> 2;						//v1.02
			answer[target] = (col & 0x7A00) >> 7;						//v1.02
			}
		else{
			answer[target++] = (col & 0x001F) << 3;
			answer[target++] = (col & 0x03E0) >> 2;
			answer[target] = (col & 0x7A00) >> 7;
			}

		  ptr_rd += 2;													//v1.02

		}
//		while(ii < (bmpheader.width + 1)/2 * 4)
		while( ptr_rd % 4 )												//v1.02, inc read ptr up to a 4 byte boundary if req (to get passed any padding bytes at end of each line)
		{
	    fgetc(fp);
//		  ii++;
		ptr_rd++;														//v1.02
		}
	  }
	  break;

	case 24:

if(vb)printf( "loadbmp() - num of bits: %d\n", bmpheader.bits );


	  for(i=0;i<bmpheader.height;i++)
	  {
		for(ii=0;ii<bmpheader.width;ii++)
		{
		  target = (i * bmpheader.width * 3) + ii * 3;
//	      answer[target] = fgetc(fp);
//		  answer[target+1] = fgetc(fp);
//		  answer[target+2] = fgetc(fp);

		if( b_rgb_ordering )											//v1.02
			{
			answer[target+2] = fgetc(fp);								//v1.02
			answer[target+1] = fgetc(fp);								//v1.02
			answer[target] = fgetc(fp);									//v1.02
			}
		else{
			answer[target++] = fgetc(fp);								//v1.02
			answer[target++] = fgetc(fp);								//v1.02
			answer[target] = fgetc(fp);									//v1.02
			}
		  ptr_rd += 3;													//v1.02
		}
	
//		while(ii < (bmpheader.width + 3)/4 * 4)
		while( ptr_rd%4 )												//v1.02, inc read ptr up to a 4 byte boundary if req (to get passed any padding bytes at end of each line)
		{
		fgetc(fp);
//		ii++;
		ptr_rd++;														//v1.02
		}
	  }
	  break;

	case 32:
if(vb)printf( "loadbmp() - num of bits: %d\n", bmpheader.bits );
	  for(i=0;i<bmpheader.height;i++)
		for(ii=0;ii<bmpheader.width;ii++)
		{
		  target = (i * bmpheader.width * 3) + ii * 3;

		if( b_rgb_ordering )											//v1.02
			{
			answer[target + 2] = fgetc(fp);
			answer[target + 1] = fgetc(fp);
			answer[target] = fgetc(fp);
			}
		else{
			answer[target] = fgetc(fp);
			answer[target+1] = fgetc(fp);
			answer[target+2] = fgetc(fp);
			}

		  fgetc(fp);
		}
	  break;
  }

  if(bmpheader.bits < 16)
  {
    for(i=0;i<bmpheader.height;i++)
	  for(ii=0;ii<bmpheader.width;ii++)
	  {
	    target = (i * bmpheader.width * 3) + ii * 3;
		index = raster[i * bmpheader.width + ii] * 3;

		if( b_rgb_ordering )											//v1.02
			{
			answer[target + 2] = pal[ index ];
			answer[target+1] = pal[ index + 1 ];
			answer[target] = pal[ index + 2 ];
			}
		else{
			answer[target] = pal[ index ];
			answer[target+1] = pal[ index + 1 ];
			answer[target+2] = pal[ index + 2 ];
			}
	  }  

	free(raster);
  }

  if(bmpheader.upside_down)
  {
    for(i=0;i<bmpheader.height/2;i++)
	  swap( answer + i * bmpheader.width * 3, 
	    answer + (bmpheader.height - i - 1) * bmpheader.width * 3,
		bmpheader.width * 3);
  }

  if(ferror(fp))
  {
    free(answer);
	answer = 0;
  }



if(vb)printf( "loadbmp() - bmpheader.width: %d\n", bmpheader.width );	//v1.04
if(vb)printf( "loadbmp() - bmpheader.height: %d\n", bmpheader.height );	//v1.04

  *width = bmpheader.width;
  *height = bmpheader.height;

  fclose(fp);

  return answer;
}

/************************************************************
* loadbmp8bit() - load an 8-bit bitmap.                     *
* Params: fname - pointer to file path.                     *
*         width - return pointer for image width.           *
*         height - return pointer for image height.         *
*         pal - return pointer to 256 rgb palette entries.  *
* Returns: malloced pointer to image data, 0 on fail.       *
************************************************************/
unsigned char *loadbmp8bit(char *fname, int *width, int *height, unsigned char *pal)
{
  BMPHEADER bmphdr;
  FILE *fp;
  unsigned char *answer;
  int i;

  fp = fopen(fname, "rb");
  if(!fp)
	return 0;
  
  if(loadheader(fp, &bmphdr) == -1)
  {
    fclose(fp);
	return 0;
  }
  if(bmphdr.bits != 8)
  {
    fclose(fp);
	return 0;
  }
  if(bmphdr.compression != 0)
  {
    fclose(fp);
	return 0;
  }
  if(bmphdr.core)
    loadpalettecore(fp, pal, 256);
  else
	loadpalette(fp, pal, bmphdr.palsize);
  answer = (unsigned char *) malloc(bmphdr.width * bmphdr.height);
  if(!answer)
  {
    fclose(fp);
	return 0;
  }

  loadraster8(fp, answer, bmphdr.width, bmphdr.height);
  if(bmphdr.upside_down)
  {
    for(i=0;i<bmphdr.height/2;i++)
	  swap(answer + i * bmphdr.width, 
	    answer + (bmphdr.height - i - 1) * bmphdr.width, 
		bmphdr.width);
  }

  if(ferror(fp))
  {
    free(answer);
	answer = 0;
  }

  fclose(fp);
  *width = bmphdr.width;
  *height = bmphdr.height;

  return answer;
}

/************************************************************
* loadbmp4bit() - load a 4-bit bitmap from disk.            *
* Params: fname - pointer to the file path.                 *
*         width - return pointer for image width.           *
*         height - return pointer for image height.         *
*         pal - return pointer for 16 rgb palette entries.  *
* Returns: malloced pointer to 4-bit image data.            *
************************************************************/
unsigned char *loadbmp4bit(char *fname, int *width, int *height, unsigned char *pal)
{
  BMPHEADER bmphdr;
  FILE *fp;
  unsigned char *answer;
  int i;

  fp = fopen(fname, "rb");
  if(!fp)
	return 0;
 
  if(loadheader(fp, &bmphdr) == -1)
  {
    fclose(fp);
	return 0;
  }
  if(bmphdr.bits != 4)
  {
    fclose(fp);
	return 0;
  }
  if(bmphdr.compression != 0)
  {
    fclose(fp);
	return 0;
  }

  if(bmphdr.core)
    loadpalettecore(fp, pal, 16);
  else
	loadpalette(fp, pal, bmphdr.palsize);
  answer = (unsigned char *) malloc(bmphdr.width * bmphdr.height);
  if(!answer)
  {
    fclose(fp);
	return 0;
  }
  loadraster4(fp, answer, bmphdr.width, bmphdr.height);

  if(bmphdr.upside_down)
  {
    for(i=0;i<bmphdr.height/2;i++)
	{
	  swap(answer + i * bmphdr.width,
		answer + (bmphdr.height - i - 1) * bmphdr.width,
		bmphdr.width);
    }
  }

  if(ferror(fp))
  {
    free(answer);
	answer = 0;
  }
  
  fclose(fp);
  *width = bmphdr.width;
  *height = bmphdr.height;
  return answer;
}









/***********************************************************
* save a24-bit bmp file.                                   *
* Params: fname - name of file to save.                    *
*         rgb - raster data in rgb format                  *
*         width - image width                              *
*         height - image height                            *
* Returns: 0 on success, -1 on fail                        *
***********************************************************/             
int savebmp(char *fname, unsigned char *rgb, int width, int height)
{
  FILE *fp;
  int i;
  int ii;

  fp = fopen(fname, "wb");
  if(!fp)
	return -1;

  saveheader(fp, width, height, 24);
  for(i=0;i<height;i++)
  {
	for(ii=0;ii<width;ii++)
	{
      fputc(rgb[2], fp);												//store blue first
	  fputc(rgb[1], fp);
	  fputc(rgb[0], fp);
	  rgb += 3;
	}
	if(( width * 3) % 4)
	{
	  for(ii=0;ii< 4 - ( (width * 3) % 4); ii++)
	  {
	    fputc(0, fp);
	  }
	}
  }

  if(ferror(fp))
  {
    fclose(fp);
	return -1;
  }

  return fclose(fp);
}



/**********************************************************
* save an 8-bit palettised bitmap .                       *
* Params: fname - the name of the file.                   *
*         data - the raster data                          *
*         width - image width                             *
*         height - image height                           *
*         pal - palette (RGB format)                      *
* Returns: 0 on success, -1 on failure                    *
**********************************************************/
int savebmp8bit(char *fname, unsigned char *data, int width, int height, unsigned char *pal)
{
  FILE *fp;
  int i;
  int ii;

  fp = fopen(fname, "wb");
  if(!fp)
	return -1;

  saveheader(fp, width, height, 8);
  savepalette(fp, pal, 256);
  
  for(i=0;i<height;i++)
	for(ii=0;ii< (width + 3)/4;ii++)
	{
	  fputc(data[i*width + ii * 4], fp);
	  if(ii * 4 + 1 < width)
		fputc(data[i*width + ii * 4 + 1], fp);
	  else
		fputc(0, fp);
	  if(ii * 4 + 2 < width)
		fputc(data[i*width + ii * 4 + 2], fp);
	  else
		fputc(0, fp);
	  if(ii * 4 + 3 < width)
		fputc(data[i*width + ii * 4 + 3], fp);
	  else
		fputc(0, fp);
	}

  if(ferror(fp))
  {
    fclose(fp);
	return -1;
  }

  return fclose(fp);
}

/*******************************************************
* save a 4-bit palettised bitmap.                      *
* Params: fname - the name of the file.                *
*         data - raster data                           *
*         width - image width                          *
*         height - image height                        *
*         pal - the palette (RGB format)               *
* Returns: 0 on success, -1 on failure                 *
*******************************************************/ 
int savebmp4bit(char *fname, unsigned char *data, int width, int height, unsigned char *pal)
{
  FILE *fp;
  int i;
  int ii;
  int pix;

  fp = fopen(fname, "wb");
  if(!fp)
	return -1;

  saveheader(fp, width, height, 4);
  savepalette(fp, pal, 16);

  for(i=0;i<height;i++)
    for(ii=0;ii< (width + 7)/8;ii++)
	{
      pix = data[i * width + ii * 8] << 4;
	  if(ii * 8 + 1 < width)
		pix |= data[i * width + ii * 8 + 1];
	  fputc(pix, fp);

	  pix = 0;
	  if(ii * 8 + 2 < width)
		pix = data[ i * width + ii * 8 + 2] << 4;
	  if(ii * 8 + 3 < width)
		pix |= data[ i * width + ii * 8 + 3];
	  fputc(pix, fp);

	  pix = 0;
	  if(ii * 8 + 4 < width)
		pix = data[ i * width + ii * 8 + 4] << 4;
	  if(ii * 8 + 5 < width)
		pix |= data[i * width + ii * 8 + 5];
	  fputc(pix, fp);

	  pix = 0;
	  if(ii * 8 + 6 < width)
		pix = data[ i * width + ii * 8 + 6] << 4;
	  if(ii * 8 + 7 < width)
		pix |= data[i * width + ii * 8 + 7];
	  fputc(pix, fp);
	}

  if(ferror(fp))
  {
    fclose(fp);
	return -1;
  }

  return fclose(fp);
}

/***************************************************************
* save a 2-bit palettised bitmap.                              *
* Params: fname - name of file to write.                       *
*         data - raster data, one byte per pixel.              *
*         width - image width.                                 *
*         height - image height.                               *
*         pal - the palette (0 = black/white)                  *
* Returns: 0 on success, -1 on fail.                           *
***************************************************************/ 
int savebmp2bit(char *fname, unsigned char *data, int width, int height, unsigned char *pal)
{
  FILE *fp;
  unsigned char defpal[6] = {0, 0, 0, 255, 255, 255 };
  int i;
  int ii;
  int iii;
  int pix;

  fp = fopen(fname, "wb");
  if(!fp)
	return -1;
  
  saveheader(fp, width, height, 1);
  if(pal)
	savepalette(fp, pal, 2);
  else
	savepalette(fp, defpal, 2);

  for(i=0;i<height;i++)
	for(ii=0;ii<width;ii+=32)
	{
	  pix = 0;
      for(iii=0;iii<8;iii++)
		if(ii + iii < width)
			pix |= data[i * width + ii + iii] ? (1 << (7-iii) ) : 0;
	  fputc(pix, fp);

      pix = 0;
	  for(iii=0;iii<8;iii++)
		if(ii + iii + 8 < width)
			pix |= data[i * width + ii + iii + 8] ? (1 << (7 - iii)) : 0;
	  fputc(pix, fp);

	  pix = 0;
	  for(iii=0;iii<8;iii++)
		if(ii + iii + 16 < width)
			pix |= data[i * width + ii + iii + 16] ? (1 << (7 - iii)) : 0;
      fputc(pix, fp);

	  pix = 0;
	  for(iii=0;iii<8;iii++)
		if(ii + iii + 24 < width)
			pix |= data[i * width + ii + iii + 24] ? (1 << (7 - iii)) : 0;

      fputc(pix, fp);
	}
  
  if(ferror(fp))
  {
    fclose(fp);
	return -1;
  }

  return fclose(fp);
}

/**************************************************************
* loadpalette() - load palette for a new format BMP.          *
* Params: fp - pointer to an open file.                       *
*         pal - return pointer for palette entries.           *
*         entries - number of entries in palette.             *
**************************************************************/
static void loadpalette(FILE *fp, unsigned char *pal, int entries)
{
  int i;
  for(i=0;i<entries;i++)
  {
    pal[2] = fgetc(fp);
	pal[1] = fgetc(fp);
	pal[0] = fgetc(fp);
	fgetc(fp);
	pal += 3;
  }
}

/******************************************************
* loadpalettecore() - load a palette for a core BMP   *
* Params: fp - pointer to an open file.               *
*         pal - return pointer for palette entries.   *
*         entries - number of entries to read.        *
******************************************************/
static void loadpalettecore(FILE *fp, unsigned char *pal, int entries)
{
  int i;
  for(i=0;i<entries;i++)
  {
    pal[2] = fgetc(fp);
	pal[1] = fgetc(fp);
	pal[0] = fgetc(fp);
	pal += 3;
  }
}

/*************************************************************
* saves a palette                                            *
* Params: fp - pointer to an open file                       *
*         pal - the palette                                  *
*         entries - number of palette entries.               *
*************************************************************/
static void savepalette(FILE *fp, unsigned char *pal, int entries)
{
  int i;

  for(i=0;i<entries;i++)
  {
    fputc(pal[2], fp);
	fputc(pal[1], fp);
	fputc(pal[0], fp);
	fputc(0, fp);
	pal += 3;
  }
}

/*
typedef struct tagBITMAPFILEHEADER { // bmfh 
    WORD    bfType; 
    DWORD   bfSize; 
    WORD    bfReserved1; 
    WORD    bfReserved2; 
    DWORD   bfOffBits; 
} BITMAPFILEHEADER; 

typedef struct tagBITMAPINFOHEADER{ // bmih 
    DWORD  biSize; 
    LONG   biWidth; 
    LONG   biHeight; 
    WORD   biPlanes; 
    WORD   biBitCount 
    DWORD  biCompression; 
    DWORD  biSizeImage; 
    LONG   biXPelsPerMeter; 
    LONG   biYPelsPerMeter; 
    DWORD  biClrUsed; 
    DWORD  biClrImportant; 
} BITMAPINFOHEADER;

*/

/******************************************************
* loadheader() - load the bitmap header information.  *
* Params: fp - pinter to an opened file.              *
*         hdr - return pointer for header information.*
* Returns: 0 on success, -1 on fail.                   *
******************************************************/
static int loadheader(FILE *fp, BMPHEADER *hdr)
{
  int size;
  int hdrsize;
  int id;
  int i;

  id = fget16le(fp);
  /* is it genuinely a BMP ? */
  if(id != 0x4D42)
	return -1;
  /* skip rubbish */
  fget32le(fp);
  fget16le(fp);
  fget16le(fp);
  /* offset to bitmap bits */
  size = fget32le(fp);
  hdrsize = fget32le(fp);
 printf("bmp_code.cpp  loadheader() hdrsize %d\n", hdrsize );
 if(hdrsize == 40)
  {
    hdr->width = fget32le(fp);
	hdr->height = fget32le(fp);
//printf("bmp_code.cpp  loadheader() hdr->height %d\n", hdr->height );
	fget16le(fp);
    hdr->bits = fget16le(fp);
//printf("bmp_code.cpp  loadheader() hdr->bits %d\n", hdr->bits );
	hdr->compression = fget32le(fp);
	/* skip rubbish */
    for(i=0;i<12;i++)
	  fgetc(fp);
	hdr->palsize = fget32le(fp);
	if(hdr->palsize == 0 && hdr->bits < 16)
	  hdr->palsize = 1 << hdr->bits;
	fget32le(fp);
	if(hdr->height < 0)
	{
	  hdr->upside_down = 0;
	  hdr->height = -hdr->height;
	}
	else
	  hdr->upside_down = 1;
    hdr->core = 0;
  }
  else if(hdrsize == 12)
  {
    hdr->width = fget16le(fp);
	hdr->height = fget16le(fp);
	fget16le(fp);
	hdr->bits = fget16le(fp);
	hdr->compression = 0;
	hdr->upside_down = 1;
	hdr->core = 1;
	hdr->palsize = 1 << hdr->bits;
  }
  else
	{
printf("bmp_code.cpp  loadheader() unsupported header size: %d\n", hdrsize );
	return 0;
	}
  if(ferror(fp))
	return -1;
  return 0;
}

/****************************************************************
* write a bitmap header.                                        *
* Params: fp - pointer to an open file.                         *
*         width - bitmap width                                  *
*         height - bitmap height                                *
*         bit - bit depth (1, 4, 8, 16, 24, 32)                 *
****************************************************************/
static void saveheader(FILE *fp, int width, int height, int bits)
{
  long sz;
  long offset;

  /* the file header */
  /* "BM" */
  fputc(0x42, fp);
  fputc(0x4D, fp);

  /* file size */
  sz = getfilesize(width, height, bits) + 40 + 14;
  fput32le(sz, fp);

  /* reserved */
  fput16le(0, fp);
  fput16le(0, fp);
  /* offset of raster data from header */
  if(bits < 16)
    offset = 40 + 14 + 4 * (1 << bits);
  else
	offset = 40 + 14;
  fput32le(offset, fp);

  /* the infoheader */

  /* size of structure */
  fput32le(40, fp);
  fput32le(width, fp);
  /* height negative because top-down */
  fput32le(-height, fp);
  /* bit planes */
  fput16le(1, fp);
  fput16le(bits, fp);
  /* compression */
  fput32le(0, fp);
  /* size of image (can be zero) */
  fput32le(0, fp);
  /* pels per metre */
  fput32le(600000, fp);
  fput32le(600000, fp);
  /* colours used */
  fput32le(0, fp);
  /* colours important */
  fput32le(0, fp);
}

/*********************************************************************
* load 8-bit raster data                                             *
* Params: fp - pointer to an open file.                              *
*         data - return pointer for data (one byte per pixel)        *
*         width - image width                                        *
*         height - image height                                      *
*********************************************************************/
static void loadraster8(FILE *fp, unsigned char *data, int width, int height)
{
  int linewidth;
  int i;
  int ii;

  linewidth = (width + 3)/4 * 4;

  for(i=0;i<height;i++)
  {
    for(ii=0;ii<width;ii++)
	  *data++ = fgetc(fp);
	while(ii < linewidth)
	{
	  fgetc(fp);
	  ii++;
	}
  }
}

/**********************************************************************
* load 4-bit raster data                                              *
* Params: fp - pointer to an open file.                               *
*         data - return pointer for data (one byte per pixel)         *
*         width - image width                                         *
*         height - iamge height                                       *
**********************************************************************/ 
static void loadraster4(FILE *fp, unsigned char *data, int width, int height)
{
  int linewidth;
  int i;
  int ii;
  int pix;

  linewidth = (((width + 1)/2) + 3)/4 * 4;
  
  for(i=0;i<height;i++)
  {
    for(ii=0;ii<linewidth;ii++)
	{
	  pix = fgetc(fp);
	  if(ii * 2 < width)
		*data++ = pix >> 4;
	  if(ii * 2 + 1 < width)
		*data++ = pix & 0x0F;
    }
  }
}

/*******************************************************************
* load 1 bit raster data                                           *
* Params: fp - pointer to an open file.                            *
*         data - return pointer for data (one byte per pixel)      *
*         width - image width.                                     *
*         height - image height.                                   *
*******************************************************************/
static void loadraster1(FILE *fp, unsigned char *data, int width, int height)
{
  int linewidth;
  int i;
  int ii;
  int iii;
  int pix;

  linewidth = ((width + 7)/8 + 3)/4 * 4;

  for(i=0;i<height;i++)
  {
	for(ii=0;ii<linewidth;ii++)
	{
	  pix = fgetc(fp);
	  if(ii * 8 < width)
	  {
	    for(iii=0;iii<8;iii++)
	      if(ii * 8 + iii < width)
		    *data++ = (pix & (1 << (7 - iii))) ? 1 : 0;
	  }
	}
  }

}

/*****************************************************
*  get the size of the file to be written.           *
* Params: width - image width                        *
*         height - image height                      *
*         bits - image type                          *
* Returns: size of image data (excluding headers)    *
*****************************************************/
static long getfilesize(int width, int height, int bits)
{
  long answer = 0;
  switch(bits)
  {
    case 1:
	  answer = (width + 7)/8;
	  answer = (answer + 3)/4 * 4;
	  answer *= height;
	  answer += 2 * 4;
	  break;
    case 4:
	  answer = 16 * 4 + (width + 1)/2;
	  answer = (answer + 3)/4 * 4;
	  answer *= height;
	  answer += 16 * 4;
	  break;
	case 8:
	  answer = (width + 3)/4 * 4;
	  answer *= height;
	  answer += 256 * 4;
	  break;
	case 16:
	  answer = (width * 2 + 3)/4 * 4;
	  answer *= height;
	  break;
	case 24:
	  answer = (width * 3 + 3)/4 * 4;
	  answer *= height;
	  break;
	case 32:
	  answer = width * height * 4;
	  break;
	default:
	  return 0;
  }

  return answer;
}

/***************************************************************
* swap an area of memory                                       *
* Params: x - pointer to first buffer                          *
*         y - pointer to second buffer                         *
*         len - length of memory to swap                       *
***************************************************************/
static void swap(void *x, void *y, int len)
{
  unsigned char *ptr1 = (unsigned char *)x;
  unsigned char *ptr2 = (unsigned char *)y;
  unsigned char temp;
  int i;

  for(i=0;i<len;i++)
  {
    temp = ptr1[i];
	ptr1[i] = ptr2[i];
	ptr2[i] = temp;
  }
}

/***************************************************************
* write a 32-bit little-endian number to a file.               *
* Params: x - the number to write                              *
*         fp - pointer to an open file.                        *
***************************************************************/
static void fput32le(long x, FILE *fp)
{
  fputc(x & 0xFF, fp);
  fputc( (x >> 8) & 0xFF, fp);
  fputc( (x >> 16) & 0xFF, fp);
  fputc( (x >> 24) & 0xFF, fp);
}

/***************************************************************
* write a 16-bit little-endian number to a file.               *
* Params: x - the nmuber to write                              *
*         fp - pointer to an open file                         *
***************************************************************/                
static void fput16le(int x, FILE *fp)
{
  fputc(x & 0xFF, fp);
  fputc( (x >> 8) & 0xFF, fp);
}

/***************************************************************
* fget32le() - read a 32 bit little-endian number from a file. *
* Params: fp - pointer to an open file.                        *
* Returns: value read as a signed integer.                     *
***************************************************************/
static long fget32le(FILE *fp)
{
  long answer;
  answer = fgetc(fp);
  answer |= (fgetc(fp) << 8);
  answer |= (fgetc(fp) << 16);
  answer |= (fgetc(fp) << 24);
  /* check for negative */
  if(answer & 0x80000000)
	answer |= ((-1) << 31);
  return answer;
}

/***************************************************************
* fget16le() - read a 16 bit little-endian number from a file. *
* Params: fp - pointer to an open file.                        *
* Returns: value read as a signed integer.                     *
***************************************************************/
static int fget16le(FILE *fp)
{
  int answer;
  answer = fgetc(fp);
  answer |= (fgetc(fp) << 8);
  /* check for negative */
  if(answer & 0x8000)
	answer |= ((-1) << 16);
  return answer;
}
//-----------------------------------------------------------------------------

}//namespace 'bmp_code::'		

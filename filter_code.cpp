/*
Copyright (C) 2018--2026 BrerDawg, et. al.

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

//filter_code.cpp

//v1.01		01-sep-2018
//v1.02		07-nov-2020													//changed 'twopi' to a constant, commented out some iir code
//v1.03		09-oct-2022		//added function: 'iir_process(..)' which is similar to: 'filter_iir_2nd_order(..)' and 'filter_iir_2nd_order_2ch(..)'
							//added function prototypes to: 'filter_code.h' ---> 'bool create_iir_filter_from_coeffs( st_iir &iir, vector<double> &vcoeff );'
							//added function prototypes to: 'filter_code.h' ---> 'void iir_delete_filter( st_iir &iir );'  and  'void iir_init( st_iir &iir );'
							//added extra filter types to 'en_filter_window_type_tag' and 'filter_fir_windowed()':  'fwt_lanczos1', 'fwt_lanczos1_5', 'fwt_lanczos2', 'fwt_lanczos3'
							//added 'filter_fir_sinc(..)'

//v1.04		22-jul-2023		//added namespace 'filter_code::'				
//v1.05		18-sep-2023		//added 'create_filter_iir_using_q()'
//v1.06		26-feb-2024		//remove include(s) of 'globals.h' 'mgraph.h'
							//changed 'delete' to 'delete[]'
							//added 'st_fir.verb'  'st_fir.suser0'  'st_fir.suser1'  'st_fir.user_id0'  'st_fir.user_id1'

//v1.07		08-mar-2025		//added to 'st_iir'  'user0', 'user_id0' etc, for debug purposes

//v1.08		28-apr-2025		//added 'window_calc_and_apply_float()', 'window_function_float()', also changed mem deletes, eg changed: 'delete w;'  to  'delete[] w;'
							//completed code for type: 'fwt_kaiser' used a by various functions, NOTE:  you may need to ADJUST as req: 'kaiser_alpha' 'kaiser_k'
							

//v1.09		13-jun-2025		//added 'st_cplex_float_tag'

//v1.10		22-dec-2025		//added 'load_coeffs_from_file_float()' 'load_coeffs_from_file_double()'
//v1.11		07-jan-2026		//added 'polyphase_filter_downsampler_complex()'    'polyphase_filter_downsampler_for_fft()'    'polyphase_filter_upsampler_complex()'   'polyphase_filter_upsampler_complex_vector()'
//v1.12		23-jan-2026		//added 'load_coeffs_from_string_float()'  'load_coeffs_from_string_double()'
							//added 'save_coeffs_vector_float()'  'save_coeffs_vector_double()'
							//added 'make_string_from_coeffs_float()'  'make_string_from_coeffs_double()' 

//v1.13		05-mar-2026		//added complex cojugate function and complex multiply operator  to 'st_cplex_tag' and 'st_cplex_float_tag'

//v1.14		14-mar-2026		//moded 'load_coeffs_from_string_double()'  'load_coeffs_from_string_float()'  to strip commas and some c code formatting when pasting coeffs from a c code struct
							//removed code in 'iir_init()' which stopped iir being cleared if it was not in a 'created' state, this helps if 'iir.dly0' or 'iir.dly1' has a stuck NAN value in either of them
//v1.15		23-mar-2026		//added 'window_kaiser_float()' ' window_kaiser_double()'
							//added 'make_halfband_coeffs_float()'  'make_halfband_coeffs_double()'
							//added fixed kaiser code to match python's version of numpy.kaiser(), now use 'filter_code::kaiser_beta', removed: 'filter_code::kaiser_alpha'  and 'filter_code::kaiser_k'
							//added 'create_iir_filter_from_coeffs_b0_first()'

//v1.16		03-apr-2026		//fixed DC offset in notch fir response in 'filter_fir_windowed()'
							//added 'iir_process_float()' and internal float variables to check if coeff rounding has any effect on freq response, refer 'st_iir' 'f_a1', 'f_a2', 'f_dly0'

//v1.17		29-apr-2026		//added 'window_calc_normalisation_factor_float()' 'window_calc_normalisation_factor_double()'
//v1.18		31-may-2026		//added 'fir_process()'   'fir_process_vector_double()'
//v1.19		26-aug-2026		//added 'class cl_ellip_builder' this NOT is namespace 'filter_code::'
//v1.20		12-sep-2026		//added added to 'st_iir'  'fc' and 'q'  'db_gain'  handy vars for user to make note of iir specs, not actually set by any code avail here or used in calcs


#include "filter_code.h"

namespace filter_code
	{
	#define pi (M_PI)
	#define twopi (2.0*M_PI)

float iir2nd_coeffs[5];													//second order iir




//double kaiser_alpha = 1.27;				//v1.08, alpha of '1.27' has a beta value of 4  (4/pi)
//int kaiser_k = 5;						//v1.08, defines iteration count to use when calculating Io (I0), the zeroth order of the modified Bessel function of the first kind

double kaiser_beta = 6;


//reads an iowa hills coeffients text file and get filter coeffs for one specified section,
//only parses the 2'nd Order Sections that have headings: 'Sect xx',
//specify required section starting at index 0,
//returns 1 on success, else 0
bool read_iowa_hills_coeffs( string fname, int section, double &a1, double &a2, double &b0, double &b1, double &b2 )
{
string s1, ssection;
mystr m1;


if( !m1.readfile( fname, 20000 ) )			//saftey margin
	{
	printf("read_iowa_hills_coeffs() - failed to read file: '%s'\n", fname.c_str() );
	return 0;
	}

printf("read_iowa_hills_coeffs() ---------------------------------\n" );

strpf( ssection, "Sect %d", section );

bool in_section = 0;
int section_line = 0;
int check_count = 0;

char c0, c1;
double dd;
int ii = 0;
for( ii = 0; ii < 20000; ii++ )				//saftey margin
	{
	if( !m1.gets( s1 ) ) break;
	if( s1.compare( ssection ) == 0 ) in_section = 1;

	if( in_section )
		{
		printf("got %03d: line: %03d, '%s'\n", ii, section_line, s1.c_str() );

		if( section_line == 2 ) { sscanf( s1.c_str(), "%c%c %lf", &c0, &c1, &dd );  a1 = dd; printf("check %c%c  %.17f\n", c0, c1, a1 ); check_count++; }
		if( section_line == 3 ) { sscanf( s1.c_str(), "%c%c %lf", &c0, &c1, &dd );  a2 = dd; printf("check %c%c  %.17f\n", c0, c1, a2 ); check_count++; }
		if( section_line == 4 ) { sscanf( s1.c_str(), "%c%c %lf", &c0, &c1, &dd );  b0 = dd; printf("check %c%c  %.17f\n", c0, c1, b0 ); check_count++; }
		if( section_line == 5 ) { sscanf( s1.c_str(), "%c%c %lf", &c0, &c1, &dd );  b1 = dd; printf("check %c%c  %.17f\n", c0, c1, b1 ); check_count++; }
		if( section_line == 6 ) { sscanf( s1.c_str(), "%c%c %lf", &c0, &c1, &dd );  b2 = dd; printf("check %c%c  %.17f\n", c0, c1, b2 ); check_count++; }
 
		section_line++;
		if( section_line >= 7 ) break;
		}
	}

//printf("read_iowa_hills_coeffs() - done - section_line %d, check_count: %d\n", section_line, check_count );
if( section_line != 7 )
	{
	printf("read_iowa_hills_coeffs() - 'section_line' expected to reach 7, only reached: %d, check coeffs read correctly\n", section_line );
	return 0;
	}

if( check_count != 5 )
	{
	printf("read_iowa_hills_coeffs() - 'check_count' expected to reach 5, only reached: %d, check coeffs read correctly\n", check_count );
	return 0;
	}

printf("read_iowa_hills_coeffs() ---------------------------------\n" );
return 1;
}










//note a0 is not used (it would be one), the other coeffs should be divided by the a0 you calc'd
// -- X --> add ------->------+-----> b0 ----> add --- Y ----> 
//           ^                |                 ^
//           |                v                 |
//           |                |                 |
//           ^               xn0                ^
//           |                |                 |
//           |                |                 |
//           |                v                 |
//          add <...-a1 <... xn0 ....> b1 ...> add
//           ^                |                 ^
//           |                v                 |
//           |                |                 |
//           |               xn1                |
//           ^                |                 ^
//           |                |                 |
//           |                v                 |
//           |<....-a2 <...  add ....> b2 ....> |


//NOTE this is SIMILAR to 'iir_process()'

//2nd order direct form 2 iir filter

bool filter_iir_2nd_order( vector <st_cplex_tag> &viq, double a1, double a2, double b0,  double b1, double b2, double &d0r, double &d1r, double &d0i, double &d1i )
{

for ( int i = 0; i < viq.size(); i++ )					//for every sample
	{

    double sum_rev = viq[ i ].real - a1 * d0r - a2 * d1r;

    double sum_fwd = sum_rev * b0 + b1 * d0r + b2 * d1r;

    viq[ i ].real = sum_fwd;

    d1r = d0r;
    d0r = sum_rev;
	}


for ( int i = 0; i < viq.size(); i++ )					//for every sample
	{

    double sum_rev = viq[ i ].imag - a1 * d0i - a2 * d1i;

    double sum_fwd = sum_rev * b0 + b1 * d0i + b2 * d1i;

    viq[ i ].imag = sum_fwd;

    d1i = d0i;
    d0i = sum_rev;
	}
}











void filter_iir_2nd_order( float &fsignal, st_iir_2nd_order_tag &of )
{
																		//a1 = coeff[0]
																		//a1 = coeff[1]
																		//b0 = coeff[2]
																		//b1 = coeff[3]
																		//b2 = coeff[4]


float sum_rev = fsignal - of.coeff[0] * of.delay0[0] - of.coeff[1] * of.delay0[1];

float sum_fwd = sum_rev * of.coeff[2] + of.coeff[3] * of.delay0[0] + of.coeff[4] * of.delay0[1];

fsignal = sum_fwd;

of.delay0[1] = of.delay0[0];
of.delay0[0] = sum_rev;
}





void filter_iir_2nd_order_2ch( float &fsig0, float &fsig1, st_iir_2nd_order_tag &of )
{
																		//a1 = coeff[0]
																		//a1 = coeff[1]
																		//b0 = coeff[2]
																		//b1 = coeff[3]
																		//b2 = coeff[4]


float sum_rev0 = fsig0 - of.coeff[0] * of.delay0[0] - of.coeff[1] * of.delay0[1];
float sum_rev1 = fsig1 - of.coeff[0] * of.delay1[0] - of.coeff[1] * of.delay1[1];						//ch1

float sum_fwd0 = sum_rev0 * of.coeff[2] + of.coeff[3] * of.delay0[0] + of.coeff[4] * of.delay0[1];
float sum_fwd1 = sum_rev1 * of.coeff[2] + of.coeff[3] * of.delay1[0] + of.coeff[4] * of.delay1[1];		//ch1

fsig0 = sum_fwd0;
fsig1 = sum_fwd1;														//ch1

of.delay0[1] = of.delay0[0];
of.delay0[0] = sum_rev0;

of.delay1[1] = of.delay1[0];
of.delay1[0] = sum_rev1;												//ch1
}












//----------------------------------------------------------------------
void iir_init( st_iir &iir )
{
//if( iir.created == 0 ) return;										//v1.14

iir.dly0 = 0;
iir.dly1 = 0;

iir.f_dly0 = 0;
iir.f_dly1 = 0;
}








//delete filter
void iir_delete_filter( st_iir &iir )
{
if ( iir.created == 0 ) return;

iir.created = 0;
}


















//note a0 is not used (it would be one), the other coeffs should be divided by the a0 you calc'd
// -- X --> add ------->------+-----> b0 ----> add --- Y ----> 
//           ^                |                 ^
//           |                v                 |
//           |                |                 |
//           ^               xn0                ^
//           |                |                 |
//           |                |                 |
//           |                v                 |
//          add <...-a1 <... xn0 ....> b1 ...> add
//           ^                |                 ^
//           |                v                 |
//           |                |                 |
//           |               xn1                |
//           ^                |                 ^
//           |                |                 |
//           |                v                 |
//           |<....-a2 <...  add ....> b2 ....> |


//NOTE this is SIMILAR to 'filter_iir_2nd_order()'

//2nd order direct form 2 iir filter

//see also 'iir_process_float()'
double iir_process( st_iir &iir, double in )
{
if( iir.created == 0 ) return 0;

if ( iir.bypass )
	{
	return in;
	}

double sum_rev = in - iir.a1 * iir.dly0 - iir.a2 * iir.dly1;

double sum_fwd = sum_rev * iir.b0 + iir.b1 * iir.dly0 + iir.b2 * iir.dly1;

iir.dly1 = iir.dly0;
iir.dly0 = sum_rev;

return sum_fwd;
}






//see also 'iir_process()'
float iir_process_float( st_iir &iir, float in )						//v1.16
{
if( iir.created == 0 ) return 0;

if ( iir.bypass )
	{
	return in;
	}

float sum_rev = in - iir.f_a1 * iir.f_dly0 - iir.f_a2 * iir.f_dly1;		//use float precision to test if coeff rounding affects freq response

float sum_fwd = sum_rev * iir.f_b0 + iir.f_b1 * iir.f_dly0 + iir.f_b2 * iir.f_dly1;

iir.f_dly1 = iir.f_dly0;
iir.f_dly0 = sum_rev;

return sum_fwd;
}









//build a filter using 'vcoeff' coeffs, assumes ordering is 'a1,a2,b0,b1,b2' --- denominator first
bool create_iir_filter_from_coeffs( st_iir &iir, vector<double> &vcoeff )
{

if( iir.created ) iir_delete_filter( iir );

//if( vcoeff.size() == 0 ) return 0;
if( vcoeff.size() < 5  ) return 0;


iir.a1 = vcoeff[0];									//load coeffs
iir.a2 = vcoeff[1];
iir.b0 = vcoeff[2];
iir.b1 = vcoeff[3];
iir.b2 = vcoeff[4];

iir.f_a1 = vcoeff[0];									//v1.16
iir.f_a2 = vcoeff[1];
iir.f_b0 = vcoeff[2];
iir.f_b1 = vcoeff[3];
iir.f_b2 = vcoeff[4];

iir.coeff_cnt = vcoeff.size();
iir.bypass = 0;

iir_init( iir );									//clear iir delays

iir.created = 1;

return 1;
}






//build a filter using 'vcoeff' coeffs, assumes ordering is 'b0,b1,b2,a1,a2' --- numerator first
bool create_iir_filter_from_coeffs_b0_first( st_iir &iir, vector<double> &vcoeff )
{

if( iir.created ) iir_delete_filter( iir );

//if( vcoeff.size() == 0 ) return 0;
if( vcoeff.size() < 5  ) return 0;


iir.b0 = vcoeff[0];
iir.b1 = vcoeff[1];
iir.b2 = vcoeff[2];
iir.a1 = vcoeff[3];									//load coeffs
iir.a2 = vcoeff[4];

iir.f_b0 = vcoeff[0];
iir.f_b1 = vcoeff[1];
iir.f_b2 = vcoeff[2];
iir.f_a1 = vcoeff[3];								//v1.16
iir.f_a2 = vcoeff[4];

iir.coeff_cnt = vcoeff.size();
iir.bypass = 0;

iir_init( iir );									//clear iir delays

iir.created = 1;

return 1;
}
//----------------------------------------------------------------------






void create_filter_iir_using_q( filter_code::en_filter_pass_type_tag filt_type, float filt_freq_in, float filt_q_in, int srate_in, filter_code::st_iir_2nd_order_tag &iir )
{
vector<double> vfilt_coeff;
float db_gain = 0;

if( !calc_filter_iir_2nd_order( filt_type, filt_freq_in, filt_q_in, db_gain, srate_in, vfilt_coeff ) )
	{
	printf( "create_filter_iir_using_q() - failed to calc filter coeffs, freq %f, q %f\n", filt_freq_in, filt_q_in );
	return;
	}

//printf( "create_filter0() - iir freq %f, q %f   %f %f %f %f %f\n", filt_freq0, filt_q0, iir0.coeff[0], iir0.coeff[1], iir0.coeff[2], iir0.coeff[3], iir0.coeff[4] );

iir.bypass = 0;
iir.coeff[0] = vfilt_coeff[0];		//a1
iir.coeff[1] = vfilt_coeff[1];		//a2
iir.coeff[2] = vfilt_coeff[2];		//b0
iir.coeff[3] = vfilt_coeff[3];		//b1
iir.coeff[4] = vfilt_coeff[4];		//b2

iir.delay0[0] = 0;
iir.delay0[1] = 0;

iir.delay1[0] = 0;
iir.delay1[1] = 0;
iir.bypass = 1;

}








/*
void filter_iir_2nd_order_slow( double &dsignal, double a1, double a2, double b0,  double b1, double b2, double &d0, double &d1 )
{
double sum_rev = dsignal - a1 * d0 - a2 * d1;

double sum_fwd = sum_rev * b0 + b1 * d0 + b2 * d1;

dsignal = sum_fwd;

d1 = d0;
d0 = sum_rev;
}
*/






//calc spec filter coeffs suitable for a Direct Form 1 - IIR, 2nd Order

//see ...
//  http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt



//Direct Form 1 - IIR, 2nd Order
// -X-----+---> b0 ----> add --------------+----Y----> 
//        |               ^                |
//       xn0              |               yn0
//        |               |                |
//        |...> b1 ....> add <.... -a1 <...|
//        |               ^                |
//       xn1              |               yn1
//        |               |                |
//        |...> b2 ....> add <.... -a2 <...|


//can be used with 'st_iir' or 'st_iir_2nd_order_tag' and associated functions:  iir_process()' or 'filter_iir_2nd_order()'
//'db_gain' only affects filt_types: 'fpt_peakeq' 'fpt_lowshelf' 'fpt_highshelf'

bool calc_filter_iir_2nd_order( en_filter_pass_type_tag filt_type, double fc1, double in_Q, double db_gain, double srate, vector<double> &vcoeff )
{
bool vb = 1;

if(vb)printf( "filter_iir_2nd_order() - type: %u, fc1: %f, q: %f, db_gain: %f, srate: %f\n", filt_type, fc1, in_Q, db_gain, srate );

if( fc1 >= srate / 2.0 )
	{
	printf( "filter_iir_2nd_order() - fc1: %f violates Nyquist, limiting to: %f\n", fc1, srate / 2.0 - 1 );
	fc1 = srate / 2.0 - 1;
	}

double Fs = srate;
double f0 = fc1;
double w0 = twopi * f0 / Fs;

double Q = in_Q;
//double BW = in_bw / fc1;

double dBgain = db_gain;

double A = pow ( 10.0 , ( dBgain / 40.0 ) );                   // A = sqrt( 10.0 ^( dBgain / 20.0 ) ) (for peaking and shelving EQ filters only)

double gain = pow ( 10.0 , ( db_gain / 20.0 ) );

double alpha = sin( w0 ) / ( 2.0 * Q );                                                         //(case: Q)
//double alpha = sin( w0 ) * sinh( log( 2.0 ) / 2.0 * BW * w0 / sin( w0 ) );                    //(case: BW)
//double alpha = sin( w0 ) / 2.0 * sqrt( ( A + 1.0 / A ) * ( 1.0 / S - 1 ) + 2.0 );             //(case: S)

double a0, a1, a2;
double b0, b1, b2;

bool ok = 0;

if(vb)printf( "filter_iir_2nd_order() - w0: %f, alpha: %f, A: %f\n", w0, alpha, A );

if( filt_type == fpt_lowpass )
    {
    b0 =  ( 1.0 - cos( w0 ) ) / 2.0;
    b1 =   1.0 - cos( w0 );
    b2 =  ( 1.0 - cos( w0 ) ) / 2.0;
    a0 =   1.0 + alpha;
    a1 =  -2.0 * cos( w0 );
    a2 =   1.0 - alpha;
    ok = 1;
    }


if( filt_type == fpt_highpass )
    {
    b0 =  ( 1.0 + cos( w0 ) ) / 2.0;
    b1 = - ( 1.0 + cos( w0 ) );
    b2 =  ( 1.0 + cos( w0 ) ) / 2.0;
    a0 =   1.0 + alpha;
    a1 =  -2.0 * cos( w0 );
    a2 =   1.0 - alpha;
    ok = 1;
    }


if( filt_type == fpt_bandpass )           //constant skirt gain, peak gain = Q
    {
    b0 =   sin(w0)/2.0;             //Q*alpha;
    b1 =   0.0;
    b2 =  -sin(w0)/2.0;             //-Q*alpha;
    a0 =   1.0 + alpha;
    a1 =  -2.0*cos(w0);
    a2 =   1.0 - alpha;
    ok = 1;
    }

if( filt_type == fpt_bandpass2 )           //constant 0 dB peak gain
    {
    b0 =   alpha;
    b1 =   0.0;
    b2 =  -alpha;
    a0 =   1.0 + alpha;
    a1 =  -2.0*cos(w0);
    a2 =   1.0 - alpha;
    ok = 1;
    }

if( filt_type == fpt_notch )
    {
    b0 =   1.0;
    b1 =  -2.0*cos(w0);
    b2 =   1.0;
    a0 =   1.0 + alpha;
    a1 =  -2.0*cos(w0);
    a2 =   1.0 - alpha;
    ok = 1;
    }

if( filt_type == fpt_apf )
    {
    b0 =   1.0 - alpha;
    b1 =  -2.0*cos(w0);
    b2 =   1.0 + alpha;
    a0 =   1.0 + alpha;
    a1 =  -2.0*cos(w0);
    a2 =   1 - alpha;
    ok = 1;
    }

if( filt_type == fpt_peakeq )
    {
    b0 =   1.0 + alpha*A;
    b1 =  -2.0*cos(w0);
    b2 =   1.0 - alpha*A;
    a0 =   1.0 + alpha/A;
    a1 =  -2.0*cos(w0);
    a2 =   1.0 - alpha/A;
    ok = 1;
    }

if( filt_type == fpt_lowshelf )
    {
    b0 =    A*( (A+1.0) - (A-1.0)*cos(w0) + 2.0*sqrt(A)*alpha );
    b1 =  2.0*A*( (A-1.0) - (A+1.0)*cos(w0)                   );
    b2 =    A*( (A+1.0) - (A-1.0)*cos(w0) - 2.0*sqrt(A)*alpha );
    a0 =        (A+1.0) + (A-1.0)*cos(w0) + 2.0*sqrt(A)*alpha;
    a1 =   -2.0*( (A-1.0) + (A+1.0)*cos(w0)                   );
    a2 =        (A+1.0) + (A-1.0)*cos(w0) - 2.0*sqrt(A)*alpha;
    ok = 1;
    }

if( filt_type == fpt_highshelf )
    {
    b0 =    A*( (A+1.0) + (A-1.0)*cos(w0) + 2.0*sqrt(A)*alpha );
    b1 = -2.0*A*( (A-1.0) + (A+1.0)*cos(w0)                   );
    b2 =    A*( (A+1.0) + (A-1.0)*cos(w0) - 2.0*sqrt(A)*alpha );
    a0 =        (A+1.0) - (A-1.0)*cos(w0) + 2.0*sqrt(A)*alpha;
    a1 =    2.0*( (A-1.0) - (A+1.0)*cos(w0)                  );
    a2 =        (A+1.0) - (A-1.0)*cos(w0) - 2.0*sqrt(A)*alpha;
    ok = 1;
    }

if( !ok )
    {
    printf( "filter_iir_2nd_order() - type: %u, is not supported\n", filt_type );
    return 0;
    }

vcoeff.clear();

vcoeff.push_back( a1 / a0 );
vcoeff.push_back( a2 / a0 );
vcoeff.push_back( b0 / a0 );
vcoeff.push_back( b1 / a0 );
vcoeff.push_back( b2 / a0 );


if( vb )
	{
	printf( "filter_iir_2nd_order():\n" );
	printf( "a1: %f, a2: %f\n", a1 / a0, a2 / a0 );
	printf( "b0: %f, b1: %f, b2: %f\n", b0 / a0, b1 / a0, b2 / a0 );
	}
//conv_vcoeff_to_scoeff( vcoeff, scoeff );

return 1;
}






//v1.03
double filter_fir_sinc(double x)
{
if (x != 0)
	{
	x *= M_PI;
	return ( sin(x)/x);
	}
return 1;
}










//calc spec filter coeffs using spec window type(e.g. blackman) and spec filter type(e.g: hpf, notch)
//refer http://www.mikroe.com/chapters/view/72/chapter-2-fir-filters/
bool filter_fir_windowed( en_filter_window_type_tag   wnd_type, en_filter_pass_type_tag   filt_type, unsigned int taps, double fc1, double fc2, double srate, vector<double> &vcoeff )
{
int ilow;
float x;

if( ( taps & 0x1 ) == 1 )
	{
	taps++;
	printf( "filter_fir_windowed() - to keep window impulse response symetrical, taps count will be slighlty increased\n" );
	}

if( taps >= cn_filter_tap_limit )
	{
	printf( "filter_fir_windowed() - tap count is too large: %u, limit is: %u\n", taps, cn_filter_tap_limit - 1 );
	return 0;
	}

printf( "filter_fir_windowed() - srate: %f, fc1: %f, fc2: %f\n", srate, fc1, fc2 );
//printf( "filter_fir_windowed() - wnd_type: %d, filt_type: %d\n", wnd_type, filt_type );


double fs = srate;

//    fs = 20000;
//    fc1 = 2500;

double *w = new double[ taps + 10 ];            //add some extra space, even though only one extra space is required (as N = taps + 1)
double *hd = new double[ taps + 10 ];
double *hcoeff = new double[ taps + 10 ];

//filt_type = fpt_highpass;


vcoeff.clear();

//double twopi = 2.0 * M_PI;

double wc1, wc2;

wc1 = twopi * fc1 / fs;
wc2 = twopi * fc2 / fs;

int Nf = taps;
double N = Nf + 1;

printf( "filter_fir_windowed() - coeff count will be: %u\n", (unsigned int)N );


double M = (double)Nf / 2.0;

//cslpf( 0, "filter_fir_windowed() - wnd_type: %d, filt_type: %d, taps: %d, wc1: %g, wc2: %g\n", wnd_type, filt_type, taps, wc1, wc2 );



//once only calc done here to reduce workload in loop
//float kaiser_pi_a = pi * kaiser_alpha;									//beta param
//float kaiser_denom = kaiser_Io( kaiser_pi_a );

float I0_beta = std::cyl_bessel_i(0, kaiser_beta );						//do once only


for( double n = 0; n < N; n++ )
	{
	int i = (int)n;
	
    
	switch( wnd_type )
		{
		case fwt_rect:
			w[ i ] = 1.0;                              										//rect window impulse resp
		break;

		case fwt_bartlett:
            ilow = ( N - 1.0 ) / 2.0 ;                        //work out which formula to use (what side of the triangle peak we are in)
            
            if( i <= ilow ) w[ i ] = 2.0 * n / ( N - 1.0 ) ;                                //bartlett/triangle window impulse resp
            else w[ i ] =  2.0 - 2.0 * n / ( N - 1.0 ) ;
		break;

		case fwt_hann:
           w[ i ] = 0.5 * ( 1.0 - cos( ( twopi * n / ( N - 1.0 ) ) ) );                     //hann window impulse resp
		break;

		case fwt_bartlett_hanning:
            w[ i ] = 0.62 - 0.48 * fabs( n / ( N - 1.0 ) - 0.5 ) + 0.38 * cos( ( twopi * ( n / ( N - 1.0 ) - 0.5 ) ) );        //bartlett-hanning window impulse resp
		break;

		case fwt_hamming:
            w[ i ] = 0.54 - 0.46 * cos( twopi * n / ( N - 1.0 ) );            //hamming window impulse resp, note the text in  http://www.mikroe.com.. listed above is slightly incorrect, used wikipedia version
		break;

		case fwt_blackman:
            w[ i ] = 0.42 - 0.5 * cos( twopi * n / ( N - 1.0 ) ) + 0.08 * cos( 2.0 * twopi * n / ( N - 1.0 ) );        //blackman window impulse resp
		break;

		case fwt_blackman_harris:
            w[ i ] = 0.35875 - 0.48829 * cos( twopi * n / ( N - 1.0 ) ) + 0.14128 * cos( 2.0 * twopi * n / ( N - 1.0 ) ) - 0.01168 * cos( 3.0 * twopi * n / ( N - 1.0 ) );        //blackman-harris window impulse resp
		break;


		case fwt_lanczos1:							//v1.03
			x = 2.0f * (float)n/(N-1);
			x -= 1;
			if (x < 0) x = - x;
			if (x < 1) w[ i ] = ( filter_fir_sinc( x ) * filter_fir_sinc( x / 1.0 ) );
			else w[ i ] = 0;
		break;

		case fwt_lanczos1_5:						//v1.03
			x = 3.0f * (float)n/(N-1);
			x -= 1.5f;
			if (x < 0) x = - x;
			if (x < 1.5) w[ i ] = ( filter_fir_sinc( x ) * filter_fir_sinc( x / 1.5 ) );
			else w[ i ] = 0;
		break;


		case fwt_lanczos2:							//v1.03
			x = 4.0f * (float)n/(N-1);
			x -= 2;
			if (x < 0) x = - x;
			if (x < 2) w[ i ] = ( filter_fir_sinc( x ) * filter_fir_sinc( x / 2.0 ) );
			else w[ i ] = 0;
		break;

		case fwt_lanczos3:							//v1.03
			x = 6.0f * (float)n/(N-1);
			x -= 3;
			if (x < 0) x = - x;
			if (x < 3) w[ i ] = ( filter_fir_sinc( x ) * filter_fir_sinc( x / 3.0 ) );
			else w[ i ] = 0;
		break;

/*
		case fwt_kaiser:							//v1.08				//make SURE you set 'filter_code::kaiser_alpha' to value you require
			{
			{
			float kaiser_w0 = kaiser_Io( kaiser_pi_a * sqrtf( 1 - powf( (2.0f*((float)i/N) - 1 ), 2 ) ) );	//refer: https://en.wikipedia.org/wiki/Kaiser_window
			kaiser_w0 /= kaiser_denom;

			w[ i ] = kaiser_w0;
			}
			}
		break;
*/

		case fwt_kaiser:												//v1.15, make SURE you set 'filter_code::kaiser_beta' to value you require
			{
			float x = 2.0f * i / (N - 1.0f) - 1.0f;          			//map to [-1,1]
			float t = kaiser_beta * std::sqrtf(1.0f - x * x);     		//argument for I0
			w[ i ] = std::cyl_bessel_i(0, t) / I0_beta;    				//normalise kaiser wnd

//printf( "filter_fir_windowed() - fwt_kaiser w[%03d] %f\n", i, w[ i ] );
			}
		break;




		default:
			printf( "filter_fir_windowed() - unknown filter window type(wnd): %u\n", wnd_type );
			return 0;
		break;
        }


	switch( filt_type )
		{
		case fpt_lowpass:

			//ideal lowpass impulse response
			if( (int)n != (int)M ) hd[ i ] = ( ( sin( wc1 * ( n - M ) ) ) / (  M_PI * ( n - M ) ) );        
			else hd[ i ] = wc1 / M_PI;
		break;

		case fpt_highpass:

			//ideal highpass impulse response
			if( (int)n != (int)M ) hd[ i ] = -( ( sin( wc1 * ( n - M ) ) ) / (  M_PI * ( n - M ) ) );        
			else hd[ i ] = 1.0 - wc1 / M_PI;
		break;


		case fpt_bandpass:

			//ideal bandpass impulse response
			if( (int)n != (int)M ) hd[ i ] =   ( sin( wc2 * ( n - M ) )  /  (  M_PI * ( n - M ) ) )   -   ( sin( wc1 * ( n - M ) ) /  (  M_PI * ( n - M ) ) );        
			else hd[ i ] = ( wc2 - wc1 ) / M_PI;
		break;


		case fpt_notch:

		// ideal notch impulse response
		if( (int)n != (int)M )
			hd[i] = ( sin( wc1 * ( n - M ) ) / ( M_PI * ( n - M ) ) )	//v1.16	
				  - ( sin( wc2 * ( n - M ) ) / ( M_PI * ( n - M ) ) );
		else
			hd[i] = 1.0 - ( wc2 - wc1 ) / M_PI;

			//ideal notch impulse response
// *** this caused a dc offset of +0.25 **
//			if( (int)n != (int)M ) hd[ i ] =   ( sin( wc1 * ( n - M ) )  /  (  M_PI * ( n - M ) ) )   -   ( sin( wc2 * ( n - M ) ) /  (  M_PI * ( n - M ) ) );        
//			else hd[ i ] = 1.0 - ( wc1 - wc2 ) / M_PI;

		break;
		
		default:
			printf( "filter_fir_windowed() - unknown filter type: %u\n", filt_type );
			return 0;

		break;
		}

    hcoeff[ i ] = w[ i ] * hd[ i ];             //window * impulse response
//	printf( "filter_fir_windowed() - w[%d]: %g, hd[]: %g, h[]: %g\n", (int)n, w[ i ], hd[ i ], hcoeff[ i ] );

//printf( "filter_fir_windowed() - w[%d]: %g, hd[]: %g, h[]: %g\n", (int)n, w[ i ], hd[ i ], hcoeff[ i ] );
 
    vcoeff.push_back( hcoeff[ i ] );
	}



delete[] w;																//v1.08
delete[] hd;
delete[] hcoeff;


return 1;
}



















//just clears a created filter's bufs, make sure you set various 'st_fir' elements manually, this function does not alter 'st_fir'
//MAKE SURE you set 'fir.bypass = 0', see 'create_filter_from_coeffs()' for example
void fir_init( st_fir &fir )
{
if( fir.created == 0 ) return;

fir.prev_idx = 0;

for( int i = 0; i < fir.coeff_cnt; i++ ) fir.prev[ i ] = 0;
}










//not for user use
void fir_init_internal_use( st_fir &fir )
{
if( fir.created == 0 ) return;

fir.prev_idx = 0;

for( int i = 0; i < fir.coeff_cnt; i++ ) fir.prev[ i ] = 0;
}








//EXAMPLE of HOW to set up 'st_fir' before 'filter_code::create_filter_from_coeffs()'

//filter_code::st_fir fir00;
//fir00.verb = 1;
//fir00.suser0 = "fir00";
//fir00.suser1 = "in function xxxx";
//fir00.user_id0 = 0;
//fir00.user_id1 = 0;
//fir00.bypass = 0;
//fir00.created = 0;


//build an fir filter from a string loaded with coeffs, allocates memory so it must be deleted
//see ABOVE for example usage
bool create_filter_from_coeffs( st_fir &fir, vector<double> &vcoeff )
{
bool vb = fir.verb;														//v1.06


if( fir.created ) delete_filter( fir );

if( vcoeff.size() == 0 )
	{
	if(vb)printf( "create_filter_from_coeffs() - filter not created, size requested was zero, '%s' '%s'\n", fir.suser0.c_str(), fir.suser1.c_str() );	
	
	return 0;
	}


if(vb) printf( "create_filter_from_coeffs() - alloc 2 filter bufs, size each %d, '%s' '%s'\n", (int)vcoeff.size(), fir.suser0.c_str(), fir.suser1.c_str() );	

fir.coeff_ptr = new double [ vcoeff.size() ];				//alloc space
fir.prev = new double [ vcoeff.size() ];

if(vb) printf( "create_filter_from_coeffs() - about to load filter coeffs, '%s' '%s'\n", fir.suser0.c_str(), fir.suser1.c_str() );	


for( int i = 0; i < vcoeff.size(); i++ )					//load coeffs
	{
	fir.coeff_ptr[ i ] = vcoeff[ i ];
	}

fir.coeff_cnt = vcoeff.size();
fir.created = 1;
//fir.bypass = 0;

fir_init_internal_use( fir );											//clear fir buf

if(vb) printf( "create_filter_from_coeffs() - filter created, '%s' '%s'\n", fir.suser0.c_str(), fir.suser1.c_str() );	
if(vb) if( fir.bypass ) printf( "create_filter_from_coeffs() - filter is in BYPASS, '%s' '%s'\n", fir.suser0.c_str(), fir.suser1.c_str() );	

return 1;
}







//delete filter and free any memory allocated
void delete_filter( st_fir &fir )
{
bool vb = fir.verb;														//v1.06


if ( fir.created == 0 ) 
	{
	if(vb)printf( "delete_filter() - can't delete filter, it's not created, '%s' '%s'\n", fir.suser0.c_str(), fir.suser1.c_str() );	
	return;
	}

if(vb)printf( "delete_filter() - about to delete filter, '%s' '%s'\n", fir.suser0.c_str(), fir.suser1.c_str() );	

fir.created = 0;


delete[] fir.coeff_ptr;													//v1.06
delete[] fir.prev;
if(vb)printf( "delete_filter() - deleted filter, '%s' '%s'\n", fir.suser0.c_str(), fir.suser1.c_str() );	

fir.coeff_ptr = 0;
fir.prev = 0;
}



















//see also 'fir_process()'
void fir_in( st_fir &fir, double in )
{
if( fir.created == 0 ) return;

fir.prev[ fir.prev_idx++ ] = in;
if( fir.prev_idx == fir.coeff_cnt ) fir.prev_idx = 0;

}







//see also 'fir_process()'
double fir_out( st_fir &fir )
{
if( fir.created == 0 ) return 0;

int idx = fir.prev_idx;

if ( fir.bypass )							//bypass, just o/p last fir_in value
	{
	return fir.prev[ idx ];
	}


//convolve
double sum = 0;
for( int i = 0; i < fir.coeff_cnt; i++ )
	{
	if( idx != 0 ) idx--;
	else idx = fir.coeff_cnt - 1;

	sum += fir.prev[ idx ] * fir.coeff_ptr[ i ];						//mac
	}

return sum;
}








//combined input/output function
double fir_process( st_fir &fir, double in )							//v1.18
{
//input
if( fir.created == 0 ) return 0;

fir.prev[ fir.prev_idx++ ] = in;
if( fir.prev_idx == fir.coeff_cnt ) fir.prev_idx = 0;



//output
int idx = fir.prev_idx;

if ( fir.bypass )							//bypass, just o/p last fir_in value
	{
	return fir.prev[ idx ];
	}



//convolve
double sum = 0;
for( int i = 0; i < fir.coeff_cnt; i++ )
	{
	if( idx != 0 ) idx--;
	else idx = fir.coeff_cnt - 1;

	sum += fir.prev[ idx ] * fir.coeff_ptr[ i ];						//mac
	}

return sum;
}







//TO BE TESTED
//combined input/output function
bool fir_process_vector_double( st_fir &fir, vector<double> &vv )		//v1.18
{

if( fir.created == 0 ) return 0;
if ( fir.bypass ) return 1;	

for( int i = 0; i < vv.size(); i++ )
	{
	//input
	fir.prev[ fir.prev_idx++ ] = vv[i];
	if( fir.prev_idx == fir.coeff_cnt ) fir.prev_idx = 0;


	//output
	int idx = fir.prev_idx;
	double sum = 0;
	for( int i = 0; i < fir.coeff_cnt; i++ )							//convolve
		{
		if( idx != 0 ) idx--;
		else idx = fir.coeff_cnt - 1;

		sum += fir.prev[ idx ] * fir.coeff_ptr[ i ];					//mac
		}
	vv[i] = sum;
	}

return 1;
}







/*

//some in-line code to speed things up, coeff count must be a factor of 4
double fir_out_inline_4( st_fir &fir )
{
if( fir.created == 0 ) return 0;

int idx = fir.prev_idx;

if ( fir.bypass )							//bypass, just o/p last fir_in value
	{
	return fir.prev[ idx ];
	}

double sum = 0;


//for( int i = 0; i < fir.coeff_cnt; i++ )
//	{
//	if( idx != 0 ) idx--;
//	else idx = fir.coeff_cnt - 1;

//	sum += fir.prev[ idx ] * fir.coeff_ptr[ i ];
//	};


//int cnt = fir.coeff_cnt / 4;

for( int i = 0; i < fir.coeff_cnt; i++ )
	{
	if( idx != 0 ) idx--;
	else idx = fir.coeff_cnt - 1;
	sum += fir.prev[ idx ] * fir.coeff_ptr[ i++ ];


	if( idx != 0 ) idx--;
	else idx = fir.coeff_cnt - 1;
	sum += fir.prev[ idx ] * fir.coeff_ptr[ i++ ];


	if( idx != 0 ) idx--;
	else idx = fir.coeff_cnt - 1;
	sum += fir.prev[ idx ] * fir.coeff_ptr[ i++ ];


	if( idx != 0 ) idx--;
	else idx = fir.coeff_cnt - 1;
	sum += fir.prev[ idx ] * fir.coeff_ptr[ i ];
	};


return sum;
}
*/








/*
//some in-line code to speed things up, coeff count must be a factor of 8
double fir_out_inline_8( st_fir &fir )
{
if( fir.created == 0 ) return 0;

int idx = fir.prev_idx;

if ( fir.bypass )							//bypass, just o/p last fir_in value
	{
	return fir.prev[ idx ];
	}

double sum = 0;


//for( int i = 0; i < fir.coeff_cnt; i++ )
//	{
//	if( idx != 0 ) idx--;
//	else idx = fir.coeff_cnt - 1;

//	sum += fir.prev[ idx ] * fir.coeff_ptr[ i ];

//	};


//int cnt = fir.coeff_cnt / 8;

for( int i = 0; i < fir.coeff_cnt; )
	{
	if( idx != 0 ) idx--;
	else idx = fir.coeff_cnt - 1;
	sum += fir.prev[ idx ] * fir.coeff_ptr[ i++ ];


	if( idx != 0 ) idx--;
	else idx = fir.coeff_cnt - 1;
	sum += fir.prev[ idx ] * fir.coeff_ptr[ i++ ];


	if( idx != 0 ) idx--;
	else idx = fir.coeff_cnt - 1;
	sum += fir.prev[ idx ] * fir.coeff_ptr[ i++ ];


	if( idx != 0 ) idx--;
	else idx = fir.coeff_cnt - 1;
	sum += fir.prev[ idx ] * fir.coeff_ptr[ i++ ];


	if( idx != 0 ) idx--;
	else idx = fir.coeff_cnt - 1;
	sum += fir.prev[ idx ] * fir.coeff_ptr[ i++ ];


	if( idx != 0 ) idx--;
	else idx = fir.coeff_cnt - 1;
	sum += fir.prev[ idx ] * fir.coeff_ptr[ i++ ];


	if( idx != 0 ) idx--;
	else idx = fir.coeff_cnt - 1;
	sum += fir.prev[ idx ] * fir.coeff_ptr[ i++ ];


	if( idx != 0 ) idx--;
	else idx = fir.coeff_cnt - 1;
	sum += fir.prev[ idx ] * fir.coeff_ptr[ i++ ];

	}


return sum;
}
*/




/*
// !!! this function is not useful, as iir filters are so simple, may as well code as req, v1.02
void iir_init( st_iir &iir )
{
if( iir.created == 0 ) return;

iir.prev_idx = 0;

for( int i = 0; i < iir.coeff_cnt; i++ ) iir.prev[ i ] = 0;
}









// !!! this function is not useful, as iir filters are so simple, may as well code as req, v1.02

//delete filter and free any memory allocated
void iir_delete_filter( st_iir &iir )
{
if ( iir.created == 0 ) return;

iir.created = 0;

delete iir.coeff_ptr;
delete iir.prev;
}









// !!! this function is not useful, as iir filters are so simple, may as well code as req, v1.02

//build a filter from a vector loaded with coeffs, allocates memory so it must be deleted
bool create_iir_filter_from_coeffs( st_iir &iir, vector<double> &vcoeff )
{

if( iir.created ) iir_delete_filter( iir );

if( vcoeff.size() == 0 ) return 0;

iir.coeff_ptr = new double [ vcoeff.size() ];				//alloc space
iir.prev = new double [ vcoeff.size() ];



for( int i = 0; i < vcoeff.size(); i++ )					//load coeffs
	{
	iir.coeff_ptr[ i ] = vcoeff[ i ];
	}

iir.coeff_cnt = vcoeff.size();
iir.created = 1;
//fir.bypass = 0;

iir_init( iir );											//clear fir buf

return 1;
}

*/









//build an fir filter from file, allocates memory so it must be deleted
//MAKE SURE you set 'fir.bypass = 0', see 'create_filter_from_coeffs()' for example
bool create_filter_from_file( st_fir &fir, string fname )
{
mystr m1, m2;
string s1;
double dd;
vector<double> vd;

bool vb = fir.verb;														//v1.06

if( fir.created ) delete_filter( fir );


if( m1.readfile( fname, 10000 ) == 0 )
	{
	printf( "create_filter_from_file() - failed to open file: '%s'\n", fname.c_str()  );
	}

s1 = m1.str();
s1 += "\r\n";							//ensure ending newline
m1 = s1;

while( 1 )
	{
	if( m1.gets( s1 ) == 0 ) break;

	m2 = s1;
	m2.cut_at_first_find( s1, "//", 0 );		// cut off any comments

	m2 = s1;
	m2.to_lower( s1 );								//change to lowercase

	if( s1.find( "end" ) != string::npos ) break;	//end found on this line?

		
	int ret = sscanf( s1.c_str(), "%lf", &dd );

	if( ( ret != 0 ) && ( ret != EOF ) )		//conv to double
		{
		vd.push_back( dd );
		}
	}


if( vd.size() == 0 )
	{
	printf( "create_filter_from_file() - no coeff read: '%s'\n", fname.c_str()  );
	return 0;
	}


if(vb)printf( "create_filter_from_file() - %d filter coeffs read from: '%s'\n", (int)vd.size(), fname.c_str()  );

if( vd.size() >=  10000 )
	{
	printf( "create_filter_from_file() - too many filter coeffs (%d) read from: '%s'\n", (int)vd.size(), fname.c_str()  );
	return 0;
	}

fir.coeff_ptr = new double [ vd.size() ];				//alloc space
fir.prev = new double [ vd.size() ];



for( int i = 0; i < vd.size(); i++ )				//loed coeffs
	{
	fir.coeff_ptr[ i ] = vd[ i ];

//	cslpf( "coeff %d: %g\n", i, vd[ i ] );
	}

fir.coeff_cnt = vd.size();
fir.created = 1;
//fir.bypass = 0;

fir_init_internal_use( fir );									//clear fir buf

return 1;
}










//read coeffs from spec file into 'vcoeff', must have one coeff per text line
bool load_coeffs_from_file_float( string fname, vector<float> &vcoeff )					//v1.10
{
mystr m1, m2;
string s1;
double dd;
vector<double> vd;

vcoeff.clear();


if( m1.readfile( fname, 10000 ) == 0 )
	{
	printf( "load_coeffs_from_file_float() - failed to open file: '%s'\n", fname.c_str()  );
	return 0;
	}

s1 = m1.str();
s1 += "\r\n";							//ensure ending newline
m1 = s1;


while( 1 )
	{
	if( m1.gets( s1 ) == 0 ) break;

	m2 = s1;
	m2.cut_at_first_find( s1, "//", 0 );		// cut off any comments

	m2 = s1;
	m2.to_lower( s1 );								//change to lowercase

	if( s1.find( "end" ) != string::npos ) break;	//end found on this line?

		
	int ret = sscanf( s1.c_str(), "%lf", &dd );

	if( ( ret != 0 ) && ( ret != EOF ) )		//conv to double
		{
		vcoeff.push_back( dd );
		}
	}


if( vcoeff.size() == 0 )
	{
	printf( "load_coeffs_from_file_float() - no coeff read: '%s'\n", fname.c_str()  );
	return 0;
	}

return 1;
}









//read coeffs from spec file into 'vcoeff', must have one coeff per text line
bool load_coeffs_from_file_double( string fname, vector<double> &vcoeff )				//v1.10
{
mystr m1, m2;
string s1;
double dd;
vector<double> vd;

vcoeff.clear();


if( m1.readfile( fname, 10000 ) == 0 )
	{
	printf( "load_coeffs_from_file_double() - failed to open file: '%s'\n", fname.c_str()  );
	return 0;
	}

s1 = m1.str();
s1 += "\r\n";							//ensure ending newline
m1 = s1;


while( 1 )
	{
	if( m1.gets( s1 ) == 0 ) break;

	m2 = s1;
	m2.cut_at_first_find( s1, "//", 0 );		// cut off any comments

	m2 = s1;
	m2.to_lower( s1 );								//change to lowercase

	if( s1.find( "end" ) != string::npos ) break;	//end found on this line?

		
	int ret = sscanf( s1.c_str(), "%lf", &dd );

	if( ( ret != 0 ) && ( ret != EOF ) )		//conv to double
		{
		vcoeff.push_back( dd );
		}
	}


if( vcoeff.size() == 0 )
	{
	printf( "load_coeffs_from_file_double() - no coeff read: '%s'\n", fname.c_str()  );
	return 0;
	}

return 1;
}











//read coeffs from string 'scoeff' into 'vcoeff', will to reformat to have one coeff per text line, allows for some c code formatting to be cleaned off
bool load_coeffs_from_string_float( string scoeff, vector<float> &vcoeff )
{
mystr m1, m2;
string s1;
double dd;
vector<double> vd;

m1 = scoeff;

m1.FindReplace( s1, "\r", "", 0 );										//clean up to one coeff per line
m1 = s1;

m1.FindReplace( s1, "{", "", 0 );										//clean up to one coeff per line
m1 = s1;

m1.FindReplace( s1, "}", "", 0 );										//clean up to one coeff per line
m1 = s1;

m1.FindReplace( s1, ",\n", "\n", 0 );									//clean up to one coeff per line
m1 = s1;

m1.FindReplace( s1, ",", "\n", 0 );
m1 = s1;

m1.FindReplace( s1, "f", "", 0 );										//remove possible c float suffix, like 1.23f
m1 = s1;

scoeff = s1;

if( scoeff.length() == 0 )
	{
	printf( "load_coeffs_from_string() - 'scoeff' is empty\n"  );
	return 0;
	}

vcoeff.clear();


s1 = scoeff;

s1 += "\r\n";							//ensure ending newline
m1 = s1;


while( 1 )
	{
	if( m1.gets( s1 ) == 0 ) break;

	m2 = s1;
	m2.cut_at_first_find( s1, "//", 0 );		// cut off any comments

	m2 = s1;
	m2.to_lower( s1 );								//change to lowercase

	if( s1.find( "end" ) != string::npos ) break;	//end found on this line?

		
	int ret = sscanf( s1.c_str(), "%lf", &dd );

	if( ( ret != 0 ) && ( ret != EOF ) )		//conv to float
		{
		vcoeff.push_back( dd );
		}
	}


if( vcoeff.size() == 0 )
	{
	printf( "load_coeffs_from_file_double() - no coeffs read\n" );
	return 0;
	}

return 1;
}






//read coeffs from string 'scoeff' into 'vcoeff', will to reformat to have one coeff per text line, allows for some c code formatting to be cleaned off
bool load_coeffs_from_string_double( string scoeff, vector<double> &vcoeff )
{
mystr m1, m2;
string s1;
double dd;
vector<double> vd;

m1 = scoeff;

m1.FindReplace( s1, "\r", "", 0 );										//clean up to one coeff per line
m1 = s1;

m1.FindReplace( s1, "{", "", 0 );										//clean up to one coeff per line
m1 = s1;

m1.FindReplace( s1, "}", "", 0 );										//clean up to one coeff per line
m1 = s1;

m1.FindReplace( s1, ",\n", "\n", 0 );									//clean up to one coeff per line
m1 = s1;

m1.FindReplace( s1, ",", "\n", 0 );
m1 = s1;

m1.FindReplace( s1, "f", "", 0 );										//remove possible c float suffix, like 1.23f
m1 = s1;

scoeff = s1;

if( scoeff.length() == 0 )
	{
	printf( "load_coeffs_from_string() - 'scoeff' is empty\n"  );
	return 0;
	}

vcoeff.clear();


s1 = scoeff;

s1 += "\r\n";							//ensure ending newline
m1 = s1;


while( 1 )
	{
	if( m1.gets( s1 ) == 0 ) break;

	m2 = s1;
	m2.cut_at_first_find( s1, "//", 0 );		// cut off any comments

	m2 = s1;
	m2.to_lower( s1 );								//change to lowercase

	if( s1.find( "end" ) != string::npos ) break;	//end found on this line?

		
	int ret = sscanf( s1.c_str(), "%lf", &dd );

	if( ( ret != 0 ) && ( ret != EOF ) )		//conv to double
		{
		vcoeff.push_back( dd );
		}
	}


if( vcoeff.size() == 0 )
	{
	printf( "load_coeffs_from_file_double() - no coeffs read\n" );
	return 0;
	}

return 1;
}







void make_string_from_coeffs_float( vector<float>vcef, string &sout )
{
sout = "";


string s1, st;

for( int i = 0; i < vcef.size(); i++ )
	{
	strpf( s1, "%.8f", vcef[i] );
	sout += s1;
	sout +="\n";
	}	
}







void make_string_from_coeffs_double( vector<double>vcef, string &sout )
{
sout = "";


string s1, st;

for( int i = 0; i < vcef.size(); i++ )
	{
	strpf( s1, "%.17f", vcef[i] );
	sout += s1;
	sout +="\n";
	}	
	
}







//saves 'vcoeff', one coeff per text line
bool save_coeffs_vector_float( string fname, vector<float> &vcoeff )
{
mystr m1;
string s1, st;

if( vcoeff.size() == 0 )
	{
	printf( "save_coeffs_vector_float() - 'vcoeff' is empty\n"  );
	return 0;
	}

for( int i = 0; i < vcoeff.size(); i++ )
	{
	strpf( s1, "%.17f", vcoeff[i] ); 
	st += s1;
	st += "\n" ;
	}

m1 = st;

if( !m1.writefile( fname ) )
	{
	printf( "save_coeffs_vector_float() - failed to save file '%s'\n", fname.c_str() );
	return 0;
	}
	
return 1;
}









//saves 'vcoeff', one coeff per text line
bool save_coeffs_vector_double( string fname, vector<double> &vcoeff )
{
mystr m1;
string s1, st;

if( vcoeff.size() == 0 )
	{
	printf( "save_coeffs_vector_double() - 'vcoeff' is empty\n"  );
	return 0;
	}

for( int i = 0; i < vcoeff.size(); i++ )
	{
	strpf( s1, "%.17f", vcoeff[i] ); 
	st += s1;
	st += "\n" ;
	}

m1 = st;

if( !m1.writefile( fname ) )
	{
	printf( "save_coeffs_vector_double() - failed to save file '%s'\n", fname.c_str() );
	return 0;
	}
	
return 1;
}












//build a filter from a string loaded with coeffs, allocates memory so it must be deleted
// coeff should be seperated by cr\lf
//MAKE SURE you set 'fir.bypass = 0', see 'create_filter_from_coeffs()' for example
bool create_filter_from_string( st_fir &fir, string scoeff )
{
mystr m1, m2;
string s1;
double dd;
vector<double> vd;

bool vb = fir.verb;														//v1.06

if( fir.created ) delete_filter( fir );



s1 = scoeff;
s1 += "\r\n";							//ensure ending newline
m1 = s1;

while( 1 )
	{
	if( m1.gets( s1 ) == 0 ) break;

	m2 = s1;
	m2.cut_at_first_find( s1, "//", 0 );		// cut off any comments

	m2 = s1;
	m2.to_lower( s1 );								//change to lowercase

	if( s1.find( "end" ) != string::npos ) break;	//end found on this line?

		
	int ret = sscanf( s1.c_str(), "%lf", &dd );

	if( ( ret != 0 ) && ( ret != EOF ) )		//conv to double
		{
		vd.push_back( dd );
		}
	}


if( vd.size() == 0 )
	{
	printf( "create_filter_from_string() - no coeff read\n"  );
	return 0;
	}


if(vb)printf( "create_filter_from_string() - %d filter coeffs read\n", vd.size() );

if( vd.size() >=  10000 )
	{
	printf( "create_filter_from_string() - too many filter coeffs (%d) read \n", vd.size() );
	return 0;
	}

fir.coeff_ptr = new double [ vd.size() ];				//alloc space
fir.prev = new double [ vd.size() ];



for( int i = 0; i < vd.size(); i++ )				//load coeffs
	{
	fir.coeff_ptr[ i ] = vd[ i ];

//	cslpf( "coeff %d: %g\n", i, vd[ i ] );
	}

fir.coeff_cnt = vd.size();
fir.created = 1;
//fir.bypass = 0;

fir_init_internal_use( fir );									//clear fir buf

return 1;
}






//refer: https://www.geeksforgeeks.org/one-line-function-for-factorial-of-a-number/
int factorial( int n ) 
{ 

//single line to find factorial 
return (n==1 || n==0) ? 1: n * factorial(n - 1);  
} 











/*	//v1.15, removed

//calc zeroth order of the modified Bessel function of the first kind: Io(x), uses 'kaiser_k' which is user adjustable

//refer: https://medium.com/@savinihemachandra/fir-filter-design-bandstop-filter-17bdace6a54e
float kaiser_Io( float x )
{
if( kaiser_k <= 1 ) return 0;


float Io = 0;

for( int k = 1; k <= kaiser_k; k++ )
	{
	Io += powf( 1.0f/factorial(k) * powf(x/2.0f,k), 2.0f);
	}
	
Io += 1.0f;

//kaiser_I0_last = Io;

return Io;
}
*/












//v1.08
//apply a window function to 'vsmpl[]' samples, calcs window shape as specified by 'wnd_type'
//'vwnd[]' is loaded with the window sample points, for debug or display purposes
bool window_calc_and_apply_float( en_filter_window_type_tag wnd_type, vector<float> &vsmpl, vector<float> &vwnd )
{
bool vb = 1;

int ilow;
float x;

vwnd.clear();

if( vsmpl.size() < 5 ) 
	{
	printf( "window_calc_and_apply_float() - not enough samples to work with, vsmpl.size(): %d\n", (int)vsmpl.size() );
	return 0;
	}



int N = vsmpl.size();


int cnt = N + 10;            //add some extra space

float *w = new float[ cnt ];






//once only calc done here to reduce workload in loop
//float kaiser_pi_a = pi * kaiser_alpha;									//beta param
//float kaiser_denom = kaiser_Io( kaiser_pi_a );

float I0_beta = std::cyl_bessel_i(0, kaiser_beta );						//do once only

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
           w[ i ] = 0.5f * ( 1.0f - cos( ( twopi * n / ( N - 1.0f ) ) ) );                     //hann window impulse resp
		break;

		case fwt_bartlett_hanning:
            w[ i ] = 0.62f - 0.48f * fabs( n / ( N - 1.0f ) - 0.5f ) + 0.38f * cos( ( twopi * ( n / ( N - 1.0f ) - 0.5f ) ) );        //bartlett-hanning window impulse resp
		break;

		case fwt_hamming:
            w[ i ] = 0.54f - 0.46f * cos( twopi * n / ( N - 1.0f ) );            //hamming window impulse resp, note the text in  http://www.mikroe.com.. listed above is slightly incorrect, used wikipedia version
		break;

		case fwt_blackman:
            w[ i ] = 0.42f - 0.5f * cos( twopi * n / ( N - 1.0f ) ) + 0.08f * cos( 2.0f * twopi * n / ( N - 1.0f ) );        //blackman window impulse resp
		break;

		case fwt_blackman_harris:
            w[ i ] = 0.35875f - 0.48829f * cos( twopi * n / ( N - 1.0f ) ) + 0.14128f * cos( 2.0f * twopi * n / ( N - 1.0f ) ) - 0.01168f * cos( 3.0f * twopi * n / ( N - 1.0f ) );        //blackman-harris window impulse resp
		break;


		case fwt_lanczos1:							//v1.03
			x = 2.0f * (float)n/(N-1);
			x -= 1;
			if (x < 0) x = - x;
			if (x < 1) w[ i ] = ( filter_fir_sinc( x ) * filter_fir_sinc( x / 1.0f ) );
			else w[ i ] = 0;
		break;

		case fwt_lanczos1_5:						//v1.03
			x = 3.0f * (float)n/(N-1);
			x -= 1.5f;
			if (x < 0) x = - x;
			if (x < 1.5f) w[ i ] = ( filter_fir_sinc( x ) * filter_fir_sinc( x / 1.5f ) );
			else w[ i ] = 0;
		break;


		case fwt_lanczos2:							//v1.03
			x = 4.0f * (float)n/(N-1);
			x -= 2;
			if (x < 0) x = - x;
			if (x < 2) w[ i ] = ( filter_fir_sinc( x ) * filter_fir_sinc( x / 2.0f ) );
			else w[ i ] = 0;
		break;

		case fwt_lanczos3:							//v1.03
			x = 6.0f * (float)n/(N-1);
			x -= 3;
			if (x < 0) x = - x;
			if (x < 3) w[ i ] = ( filter_fir_sinc( x ) * filter_fir_sinc( x / 3.0f ) );
			else w[ i ] = 0;
		break;

/*
		case fwt_kaiser:							//v1.08				//make SURE you set 'filter_code::kaiser_alpha' to value you require
		
			{
			float kaiser_w0 = kaiser_Io( kaiser_pi_a * sqrtf( 1 - powf( (2.0f*((float)i/N) - 1 ), 2 ) ) );	//refer: https://en.wikipedia.org/wiki/Kaiser_window
			kaiser_w0 /= kaiser_denom;

			w[ i ] = kaiser_w0;
			}
		break;
*/

		case fwt_kaiser:												//v1.15, make SURE you set 'filter_code::kaiser_beta' to value you require
			{
			float x = 2.0f * i / (N - 1.0f) - 1.0f;          			//map to [-1,1]
			float t = kaiser_beta * std::sqrtf(1.0f - x * x);     		//argument for I0
			w[ i ] = std::cyl_bessel_i(0, t) / I0_beta;    				//normalise kaiser wnd
			}
		break;

		default:
			printf( "window_calc_and_apply_float() - unknown filter window type(wnd): %u\n", wnd_type );
			return 0;
		break;
        }

 	}



for( int i = 0; i < vsmpl.size(); i++ )
	{
	vsmpl[i] *= w[ i ];
	}



for( int i = 0; i < N; i++ )
	{
	vwnd.push_back( w[ i ] );
	}





delete[] w;

return 1;
}











//v1.08
//build a tapering window of shape: 'wnd_type' and 'len' samples
bool window_function_float( en_filter_window_type_tag wnd_type, unsigned len, vector<float> &vwnd )
{
bool vb = 1;

int ilow;
float x;

vwnd.clear();

if( len < 5 ) 
	{
	printf( "window_function_float() - not enough samples to work with, 'len': %d\n", len );
	return 0;
	}

if( len > 1024*1024*16 ) 
	{
	printf( "window_function_float() - too many samples specified, 'len': %d (limit is 16 megasamples)\n", 1024*1024*16 );
	return 0;
	}


int N = len;



float *w = new float[ N ];





//once only calc done here to reduce workload in loop
//float kaiser_pi_a = pi * kaiser_alpha;									//beta param
//float kaiser_denom = kaiser_Io( kaiser_pi_a );

float I0_beta = std::cyl_bessel_i(0, kaiser_beta );						//do once only

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
           w[ i ] = 0.5f * ( 1.0f - cos( ( twopi * n / ( N - 1.0f ) ) ) );                     //hann window impulse resp
		break;

		case fwt_bartlett_hanning:
            w[ i ] = 0.62f - 0.48f * fabs( n / ( N - 1.0f ) - 0.5f ) + 0.38f * cos( ( twopi * ( n / ( N - 1.0f ) - 0.5f ) ) );        //bartlett-hanning window impulse resp
		break;

		case fwt_hamming:
            w[ i ] = 0.54f - 0.46f * cos( twopi * n / ( N - 1.0f ) );            //hamming window impulse resp, note the text in  http://www.mikroe.com.. listed above is slightly incorrect, used wikipedia version
		break;

		case fwt_blackman:
            w[ i ] = 0.42f - 0.5f * cos( twopi * n / ( N - 1.0f ) ) + 0.08f * cos( 2.0f * twopi * n / ( N - 1.0f ) );        //blackman window impulse resp
		break;

		case fwt_blackman_harris:
            w[ i ] = 0.35875f - 0.48829f * cos( twopi * n / ( N - 1.0f ) ) + 0.14128f * cos( 2.0f * twopi * n / ( N - 1.0f ) ) - 0.01168f * cos( 3.0f * twopi * n / ( N - 1.0f ) );        //blackman-harris window impulse resp
		break;

		case fwt_lanczos1:							//v1.03
			x = 2.0f * (float)n/(N-1);
			x -= 1;
			if (x < 0) x = - x;
			if (x < 1) w[ i ] = ( filter_fir_sinc( x ) * filter_fir_sinc( x / 1.0 ) );
			else w[ i ] = 0;
		break;

		case fwt_lanczos1_5:						//v1.03
			x = 3.0f * (float)n/(N-1);
			x -= 1.5f;
			if (x < 0) x = - x;
			if (x < 1.5) w[ i ] = ( filter_fir_sinc( x ) * filter_fir_sinc( x / 1.5 ) );
			else w[ i ] = 0;
		break;


		case fwt_lanczos2:							//v1.03
			x = 4.0f * (float)n/(N-1);
			x -= 2;
			if (x < 0) x = - x;
			if (x < 2) w[ i ] = ( filter_fir_sinc( x ) * filter_fir_sinc( x / 2.0 ) );
			else w[ i ] = 0;
		break;

		case fwt_lanczos3:							//v1.03
			x = 6.0f * (float)n/(N-1);
			x -= 3;
			if (x < 0) x = - x;
			if (x < 3) w[ i ] = ( filter_fir_sinc( x ) * filter_fir_sinc( x / 3.0 ) );
			else w[ i ] = 0;
		break;

/*
		case fwt_kaiser:							//v1.08				//make SURE you set 'filter_code::kaiser_alpha' to value you require
		
			{
			
			float kaiser_w0 = kaiser_Io( kaiser_pi_a * sqrtf( 1 - powf( (2.0f*((float)i/N) - 1 ), 2 ) ) );	//refer: https://en.wikipedia.org/wiki/Kaiser_window
			kaiser_w0 /= kaiser_denom;

			w[ i ] = kaiser_w0;
			}
		break;
*/

		case fwt_kaiser:												//v1.15, make SURE you set 'filter_code::kaiser_beta' to value you require
			{

			float x = 2.0f * i / (N - 1.0f) - 1.0f;          			//map to [-1,1]
			float t = kaiser_beta * std::sqrtf(1.0f - x * x);     		//argument for I0
			w[ i ] = std::cyl_bessel_i(0, t) / I0_beta;    				//normalise kaiser wnd
			}
		break;




		default:
			printf( "window_function_float() - unknown filter window type(wnd): %u\n", wnd_type );
			return 0;
		break;
        }

 	}


for( int i = 0; i < N; i++ )
	{
	vwnd.push_back( w[ i ] );
	}



delete[] w;

return 1;
}









//v1.17	
//calc the gain correction factor of spec window 'vwnd', useful when using a tapering window on a signal before a fwd fft,
//to use the calc'd factor returned, multiply each signal sample value with this factor before feeding samples to fwd fft,
//e.g: a hann wnd will produce a returned value of 2.0
float window_calc_normalisation_factor_float( vector<float> &vwnd )
{
float sum = 0;

int N = vwnd.size();

for( int i = 0; i < N; i++ )
	{
	sum += vwnd[ i ];
	}

sum /= (float)N;

return 1.0f / sum;
}









//v1.17	
//calc the gain correction factor of spec window 'vwnd', useful when using a tapering window on a signal before a fwd fft,
//to use the calc'd factor returned, multiply each signal sample value with this factor before feeding samples to fwd fft,
//e.g: a hann wnd will produce a returned value of 2.0
double window_calc_normalisation_factor_double( vector<double> &vwnd )
{
double sum = 0;

int N = vwnd.size();

for( int i = 0; i < N; i++ )
	{
	sum += vwnd[ i ];
	}

sum /= (double)N;

return 1.0 / sum;
}












//refer: https://www.dsprelated.com/showarticle/1113.php
//try 'kaiser_beta' = 6
void window_kaiser_float( int ntaps, double kaiser_beta, vector<float> &vkaiser )
{

vkaiser.resize( ntaps );

double denom = std::cyl_bessel_i( 0.0, kaiser_beta );

for( int n = 0; n < ntaps; n++ )
	{
	double x = ( 2.0 * n ) / ( ntaps - 1.0 ) - 1.0;

	double arg = kaiser_beta * sqrt( 1.0 - x * x );

	vkaiser[ n ] = std::cyl_bessel_i( 0.0, arg ) / denom;
	}
}





//refer: https://www.dsprelated.com/showarticle/1113.php
//try 'kaiser_beta' = 6
void window_kaiser_double( int ntaps, double kaiser_beta, vector<double> &vkaiser )
{

vkaiser.resize( ntaps );

double denom = std::cyl_bessel_i( 0.0, kaiser_beta );

for( int n = 0; n < ntaps; n++ )
	{
	double x = ( 2.0 * n ) / ( ntaps - 1.0 ) - 1.0;

	double arg = kaiser_beta * sqrt( 1.0 - x * x );

	vkaiser[ n ] = std::cyl_bessel_i( 0.0, arg ) / denom;
	}
}





//refer: https://www.dsprelated.com/showarticle/1113.php
//'odd_tap_cnt ' must be odd
//center of is 0.5
//NOTE: odd indexed coeffs (1,3,5...) are zero, this reduces fir macs by half when decimating
void make_halfband_coeffs_float( unsigned int odd_tap_cnt, vector<float> &vcef )
{
vcef.clear();

if( !(odd_tap_cnt%2) )
	{
	odd_tap_cnt++;
	printf("make_halfband_coeffs() - adding a tap to keep tap count odd, taps: %d\n", odd_tap_cnt );
	}

double kaiser_beta = 6;
vector<double> vkaiser;
filter_code::window_kaiser_double( odd_tap_cnt, kaiser_beta, vkaiser );

for( int i = 0; i < odd_tap_cnt; i++ )
	{
	double N = odd_tap_cnt - 1;
	double n = -((double)N)/2.0 + i;
	double d0 = sin(n * M_PI/2.0 )/( n*M_PI + std::numeric_limits<double>::epsilon() );	//sinc
	
	d0 *= vkaiser[i];													//apply kaiser wnd
	if( i == odd_tap_cnt/ 2 ) d0 = 0.5;									//for middle sample force to 0.5, this is the limit of: sin(πn/2)​/πn 
	vcef.push_back( d0 );
	}

/* //octave code
N= ntaps-1;
n= -N/2:N/2;
sinc= sin(n*pi/2)./(n*pi+eps);      % truncated impulse response; eps= 2E-16
sinc(N/2 +1)= 1/2;                  % value for n = 0, force it to 0.5
win= kaiser(ntaps,6);               % window function
b= sinc.*win';                      % apply window function
*/
}










//refer: https://www.dsprelated.com/showarticle/1113.php
//'odd_tap_cnt ' must be odd
//center of is 0.5
//NOTE: odd indexed coeffs (1,3,5...) are zero, this reduces fir macs by half when decimating
void make_halfband_coeffs_double( unsigned int odd_tap_cnt, vector<double> &vcef )
{
vcef.clear();

if( !(odd_tap_cnt%2) )
	{
	odd_tap_cnt++;
	printf("make_halfband_coeffs() - adding a tap to keep tap count odd, taps: %d\n", odd_tap_cnt );
	}

double kaiser_beta = 6;
vector<double> vkaiser;
filter_code::window_kaiser_double( odd_tap_cnt, kaiser_beta, vkaiser );

for( int i = 0; i < odd_tap_cnt; i++ )
	{
	double N = odd_tap_cnt - 1;
	double n = -((double)N)/2.0 + i;
	double d0 = sin(n * M_PI/2.0 )/( n*M_PI + std::numeric_limits<double>::epsilon() );	//sinc
	
	d0 *= vkaiser[i];													//apply kaiser wnd
	if( i == odd_tap_cnt/ 2 ) d0 = 0.5;									//for middle sample force to 0.5, this is the limit of: sin(πn/2)​/πn 
	vcef.push_back( d0 );
	}

/* //octave code
N= ntaps-1;
n= -N/2:N/2;
sinc= sin(n*pi/2)./(n*pi+eps);      % truncated impulse response; eps= 2E-16
sinc(N/2 +1)= 1/2;                  % value for n = 0, force it to 0.5
win= kaiser(ntaps,6);               % window function
b= sinc.*win';                      % apply window function
*/
}









//this code is NOT optimised, refer: ..MyPrj/polyphase_0_dwnsampler/
//'cin[]' holds input samples, it expects 'cin' 'phase_cnt * phase_tap_cnt' worth of samples from prev call at its head to allow repeated call continuity,
//ie. it accesses samples in the past so 'cin' should NOT be at the head of your sample block
//'phase_cnt' is the downsampler factor
//'phase_tap_cnt' number of taps per phase, i.e. ( 'vpolycoeff.size()' / 'phase_cnt' )
//'insize' is how many input samples to process in this call
//the input data index is inc'd by 'phase_cnt' count after all phases are processed only if 'insize' is a  multiple of 'phase_cnt'
//'vpolycoeff' filter coeffs (designed for input srate) in normal linear ordering (does not need a banked layout), it must be 'phase_cnt * phase_tap_size' in length
void polyphase_filter_downsampler_complex( filter_code::st_cplex_float_tag *cin, unsigned int insize, vector<filter_code::st_cplex_float_tag> &vcout, vector<float> &vpolycoeff, unsigned int phase_cnt, unsigned int phase_tap_cnt )
{
bool vb = 0;

vcout.clear();

if( insize < (phase_cnt * phase_tap_cnt ) )
	{
	printf( "polyphase_filter_downsampler_complex() - insize %d is too small, need min of %d\n", insize, phase_cnt * phase_tap_cnt );
	return;	
	}

//if( vcin.size() == 0 )
//	{
//	printf( "frme_polyphase() - vcin.size() is zero \n" );
//	return;
//	}




if(vb)printf( "polyphase_filter_downsampler_complex() - insize %d\n", insize );
if(vb)printf( "polyphase_filter_downsampler_complex() - phase_cnt %d\n", phase_cnt );
if(vb)printf( "polyphase_filter_downsampler_complex() - phase_tap_cnt %d\n", phase_tap_cnt );
if(vb)printf( "\n" );

int ii = 0;
int loop_cnt = 0;
//while( ii <= (insize-M) )
for( int ii = 0; ii < insize; ii += phase_cnt )							//move fwd in 'cin[]' after all phase coeffs have been convovled with past samples
	{
	float sum0 = 0.0f;
	int jj = 0;
	for( int phas = 0; phas < phase_cnt; phas++ )						//cycle phase/banks and apply filter coeffs (note last phase/bank is accessed first in code below)
		{
		float f0;
		for( int tap = 0; tap < phase_tap_cnt; tap++ )					//cycle coeffs in cur phase bank and convolve
			{
			int cin_idx = ii - tap*phase_cnt - phas;					//use a prev input sample for each successive phase bank
			f0 = cin[ cin_idx ].real;


			int coeff_idx = tap*phase_cnt + phas; 						//

			sum0 += f0 * vpolycoeff[ coeff_idx ];
if(vb)printf( "polyphase_filter_downsampler_complex() - %02d: in[%03d] phas %02d tap %02d  coef[%03d] %f   out[%04d]\n", jj, cin_idx, phas, tap, coeff_idx, vpolycoeff[ coeff_idx ], (int)vcout.size()  );
			jj++;
			}
		 if(vb)printf( "polyphase_filter_downsampler_complex()\n" );
		}

	if(vb)printf( "----------\n");
	filter_code::st_cplex_float_tag oc;									//store o/p sample (at downsample rate)
	oc.real = sum0;
	oc.imag = 0.0f;
	vcout.push_back( oc );

//	ii += M;
//	if( ii >= insize ) break;

loop_cnt++;
//if( loop_cnt == 4 ) getchar();
//if( ii == 2 ) getchar();
	}


if(vb)printf( "polyphase_filter_downsampler_complex() - insize %d\n", insize );

if(vb)printf( "polyphase_filter_downsampler_complex() - vcout.size() %d\n", (int)vcout.size() );
}











//this code is NOT optimised, refer: ..MyPrj/polyphase_1_channelizer/
//creates 'phase_cnt' worth of data suitable to be fed into a fwd fft, the fft would then create 'phase_cnt' number of baseband channels which are band limited by the filter impulse response in 'vpolycoeff'
//'phase_cnt' is the number of phase to use, it is also the downsample factor
//'phase_tap_cnt' is the number of filter coeffs (taps) in a phase
//'vpolycoeff.size()' needs to be of length: 'phase_cnt' * 'phase_tap_cnt', the coeffs are in normal linear ordering (does not need a banked layout)
//'vpolycoeff' impulse response is calculating using the input srate (not the downsampled srate)
//'*cin' points to input samples, make sure it's avdvanced by at least 'phase_cnt' * 'phase_tap_cnt' samples as,
//this ptr is accessed in a decrementing order, you need at least 'phase_cnt' * 'phase_tap_cnt' number of samples to preceed 'cin[]' (i.e. including 'cin[0]')

//'insize' is how many input samples to process in this call, make sure this value is either 'phase_cnt' or a multiple of this
//the internal input data index is inc'd at 'phase_cnt' steps after all phases are processed only if 'insize' is a  multiple of 'phase_cnt'
//'vcout[]' holds the output of each phase nad be of length: 'phase_cnt'
//NOTE this code is DIFFERENT to a polyphase downsampler in that the 'data out' is picked off after each phase is processed, it is NOT the sum of all the phases as in a downsampler

//example 
//assuming input srate: 2496000
//'phase_cnt': 512
//'phase_tap_cnt': 10
//'vpolycoeff.size()' = 5120, the impulse response is a lowpass filter giving a bw of 4.875 kHz (Has a −6 dB point around ±2.44 kHz)
//an input 1MHz sinewave appears in fft bin 205 (if you are displaying fft bins with bin 255 as 0Hz, then 1MHz sine appears in the minus part of spectrum at bin 50)

void polyphase_filter_downsampler_for_fft( filter_code::st_cplex_float_tag *cin, unsigned int insize, vector<filter_code::st_cplex_float_tag> &vcout, vector<float> &vpolycoeff, unsigned int phase_cnt, unsigned int phase_tap_cnt )
{
bool vb = 0;

vcout.clear();
//vphaseIQ0.clear();

//if( insize < (dwnsample_factor * phase_tap_size ) )
//	{
//	printf( "frme_polyphase_filter_for_fft() - insize %d is too small, need min of %d\n", insize, dwnsample_factor * phase_tap_size );
//	return;	
//	}

//if( vcin.size() == 0 )
//	{
//	printf( "frme_polyphase_filter_for_fft() - vcin.size() is zero \n" );
//	return;
//	}


unsigned int M = phase_cnt;												//e.g: 48000-->8000 = 6


if(vb)printf( "polyphase_filter_downsampler_for_fft() - insize %d\n", insize );
if(vb)printf( "polyphase_filter_downsampler_for_fft() - phase_cnt %d\n", phase_cnt );
if(vb)printf( "polyphase_filter_downsampler_for_fft() - phase_tap_cnt %d\n", phase_tap_cnt );
if(vb)printf( "\n" );

int ii = 0;
int loop_cnt = 0;
//while( ii <= (insize-M) )

//getchar();

int idx_min = 0;
int idx_max = 0;

int last_idx = 0;


for( int ii = 0; ii < insize; ii += phase_cnt )									//move fwd in 'cin[]' after all phase coeffs have been convovled with past samples
	{
	int jj = 0;
	for( int phas = 0; phas < phase_cnt; phas++ )						//cycle phase/banks and apply filter coeffs (note last phase/bank is accessed first in code below)
		{
		float sum0 = 0.0f;
		float sum1 = 0.0f;
		float f0;
		float f1;
		for( int tap = 0; tap < phase_tap_cnt; tap++ )					//cycle coeffs in cur phase bank and convolve
			{
			int cin_idx = ii - tap*phase_cnt - phas;					//use a prev input sample for each successive phase bank

if( cin_idx < idx_min ) idx_min = cin_idx;
if( cin_idx > idx_max ) idx_max = cin_idx;

			f0 = cin[ cin_idx ].real;
			f1 = cin[ cin_idx ].imag;

last_idx = cin_idx;

			int coeff_idx = tap*phase_cnt + phas; 						//

			float phas0 = f0 * vpolycoeff[ coeff_idx ];
			float phas1 = f1 * vpolycoeff[ coeff_idx ];
			
//			if( phas == phase_monitor )
//				{
//				filter_code::st_cplex_float_tag oc;

//				oc.real = phas0;
//				oc.imag = phas1;
//				vchanIQ0.push_back( oc );
//				}
				
			
			sum0 += phas0;
			sum1 += phas1;

			if(vb)printf( "polyphase_filter_downsampler_for_fft() - %02d: in[%03d]  phas %02d tap %02d  coef[%03d] %f   out[%04d]\n", jj, cin_idx, phas, tap, coeff_idx, vpolycoeff[ coeff_idx ], (int)vcout.size()  );
			jj++;

//			idbg0++;
//			if( idbg0 >= phase_cnt )idbg0 = 0;			
			}
		
		 if(vb)printf( "polyphase_filter_downsampler_for_fft()\n" );
		 
		if(vb)printf( "----------\n");
		
		filter_code::st_cplex_float_tag oc;									//store the processed phase resultant sample for fft to use elsewhere
		oc.real = sum0;
		oc.imag = sum1;
		vcout.push_back( oc );
		}
		
		
if(vb)printf( "polyphase_filter_downsampler_for_fft() - last_idx %d  ii %d\n", last_idx, ii );

//getchar();

//	ii += M;
//	if( ii >= insize ) break;

loop_cnt++;
//if( loop_cnt == 4 ) getchar();
//if( ii == 2 ) getchar();
	}


//if(vb)printf( "polyphase_filter_downsampler_for_fft() - insize %d vcout.size() %d  vchanIQ0 %d  \n", insize, (int)vcout.size(), (int)vchanIQ0.size() );

if(vb)printf( "polyphase_filter_downsampler_for_fft() - vcout.size() %d\n", (int)vcout.size() );

//if(1)printf( "polyphase_filter_downsampler_for_fft() - insize %d vcout.size() %d  vchanIQ0 %d  \n", insize, (int)vcout.size(), (int)vchanIQ0.size() );

if(vb)printf( "polyphase_filter_downsampler_for_fft() - idx_min %d %d  \n", idx_min, idx_max );
//getchar();
}















//this code is NOT optimised, REFER prj: 'polyphase_2_dsbsc_ssb/' for usage
//upsamples data in 'cin' by factor: 'phase_cnt'
//coded for streaming purposes, therefore requires samples from the past as described for 'cin' below,
//'phase_cnt' is the number of phase to use, it is also the upsample factor,
//'phase_tap_cnt' is the number of filter coeffs (taps) in a phase,
//'vpolycoeff.size()' needs to be of length: 'phase_cnt' * 'phase_tap_cnt', the coeffs are in normal linear ordering (does not need a banked layout),
//'vpolycoeff' impulse response is calculating using the input srate (not the upsampled srate),
//'*cin' points to input samples, make sure it's avdanced by at least 'phase_cnt' samples,
//i.e. this ptr is accessed in a decrementing order (as well as incrementing order if 'insize' is > 1), you NEED at least 'phase_tap_cnt' number of samples to preceed '*cin' (i.e. including 'cin[0]')

//'insize' is how many fwd input samples to process in this call, make sure 'cin[]' has this many sample from of 'cin[0]' and onwards i.e. 1 means the newest and last sample accessed is 'cin[0]', 2 likewise means 'cin[1]' is last sample required
//NOTE: the internal input data index is inc'd by 1 after all phases are processed (if 'insize' > 1)
//'vcout[]' holds the upsampled data and its length becomes: 'phase_cnt' * 'insize'

//example 
//assuming input srate: 48000Hz
//'cin[]' points to input sample, make SURE there is 'phase_tap_cnt' samples preceeding 'cin[]' (i.e. including 'cin[0]')
//'insize' = 1	means cin[0] is the lastest sample, 2 means 'cin[1]' is a future sample (2 will produce 80 samples in 'vout')
//'phase_cnt': 40
//'phase_tap_cnt': 16
//'vpolycoeff.size()' = 640, the impulse response is a lowpass filter
//'vout' receives 40 samples, at srate of 1920000Hz
void polyphase_filter_upsampler_complex( filter_code::st_cplex_float_tag *cin, unsigned int insize, vector<filter_code::st_cplex_float_tag> &vcout, vector<float> &vpolycoeff, unsigned int phase_cnt, unsigned int phase_tap_cnt, bool correct_gain )
{
bool vb = 0;

vcout.clear();




if(vb)printf( "polyphase_filter_upsampler_complex() - insize %d\n", insize );
if(vb)printf( "polyphase_filter_upsampler_complex() - phase_cnt %d\n", phase_cnt );
if(vb)printf( "polyphase_filter_upsampler_complex() - phase_tap_cnt %d\n", phase_tap_cnt );
if(vb)printf( "\n" );

//int ii = 0;
//int loop_cnt = 0;
//while( ii <= (insize-M) )

//getchar();

int idx_min = 0;
int idx_max = 0;

//int last_idx = 0;


for( int ii = 0; ii < insize; ii += 1 )									//move fwd in 'cin[]' after all phase coeffs have been convovled with past samples
	{
	int jj = 0;
	for( int phas = 0; phas < phase_cnt; phas++ )						//cycle phase/banks and apply filter coeffs (note last phase/bank is accessed first in code below)
		{
		float sum0 = 0.0f;
		float sum1 = 0.0f;
		float f0;
		float f1;
		for( int tap = 0; tap < phase_tap_cnt; tap++ )					//cycle coeffs in cur phase bank and convolve
			{
			int cin_idx = ii - tap;										//use a prev input sample for each successive phase bank

//if( cin_idx < idx_min ) idx_min = cin_idx;
//if( cin_idx > idx_max ) idx_max = cin_idx;

			f0 = cin[ cin_idx ].real;
			f1 = cin[ cin_idx ].imag;

//last_idx = cin_idx;

			int coeff_idx = tap*phase_cnt + phas; 						//

			float phas0 = f0 * vpolycoeff[ coeff_idx ];
			float phas1 = f1 * vpolycoeff[ coeff_idx ];

//if( phas0 > 10 ) 
//	{
//if(vb)printf( "polyphase_filter_upsampler_complex() - phas0 sum %f    f0[%d] %f  coeff[%d] %f\n", phas0, f0, cin_idx, coeff_idx, vpolycoeff[ coeff_idx ] );
//	getchar();
//	}

//			if( phas == phase_monitor )
//				{
//				filter_code::st_cplex_float_tag oc;

//				oc.real = phas0;
//				oc.imag = phas1;
//				vchanIQ0.push_back( oc );
//				}
				
			sum0 += phas0;
			sum1 += phas1;

			if(vb)printf( "polyphase_filter_upsampler_complex() - %02d: in[%03d]  phas %02d tap %02d  coef[%03d] %f   out[%04d]\n", jj, cin_idx, phas, tap, coeff_idx, vpolycoeff[ coeff_idx ], (int)vcout.size()  );
			jj++;

//			idbg0++;
//			if( idbg0 >= phase_cnt )idbg0 = 0;			
			}
//getchar();		
//		 if(vb)printf( "polyphase_filter_upsampler_complex()\n" );
		 
		if(vb)printf( "----------\n");
		
		filter_code::st_cplex_float_tag oc;									//store the processed phase resultant sample for fft to use elsewhere
		oc.real = sum0;
		oc.imag = sum1;
		vcout.push_back( oc );
		}
		
		
//if(vb)printf( "polyphase_filter_upsampler_complex() - last_idx %d  ii %d\n", last_idx, ii );

//getchar();

//	ii += M;
//	if( ii >= insize ) break;

//loop_cnt++;
//if( loop_cnt == 4 ) getchar();
//if( ii == 2 ) getchar();
	}


//if(vb)printf( "polyphase_filter_upsampler_complex() - insize %d vcout.size() %d  vchanIQ0 %d  \n", insize, (int)vcout.size(), (int)vchanIQ0.size() );

if(vb)printf( "polyphase_filter_upsampler_complex() - vcout.size() %d\n", (int)vcout.size() );

//if(1)printf( "polyphase_filter_upsampler_complex() - insize %d vcout.size() %d  vchanIQ0 %d  \n", insize, (int)vcout.size(), (int)vchanIQ0.size() );

if(vb)printf( "polyphase_filter_upsampler_complex() - idx_min %d %d  \n", idx_min, idx_max );
//getchar();
}













//this code is NOT optimised, REFER prj: 'polyphase_2_dsbsc_ssb/' for usage
//upsamples data in 'vcin' by factor: 'phase_cnt'
//code requires samples from the past, so will load zeros till 'phase_tap_cnt' num samples in 'vcin[]' is reached,
//'phase_cnt' is the number of phase to use, it is also the upsample factor,
//'phase_tap_cnt' is the number of filter coeffs (taps) in a phase,
//'vpolycoeff.size()' needs to be of length: 'phase_cnt' * 'phase_tap_cnt', the coeffs are in normal linear ordering (does not need a banked layout),
//'vpolycoeff' impulse response is calculating using the input srate (not the upsampled srate),
//'vcin' holds to input samples, make sure has at least 'phase_tap_cnt' samples in it
// vcout' is loaded with this amount of samples: ( 'vcin.size()' - 'phase_tap_cnt' + 1 ) * 'phase_cnt'
//returns 1 on success, else 0
 
//example 
//assuming input srate: 48000Hz
// 'vcin' has 'phase_tap_cnt' number of samples (minimum num, ensures convolution is not starved of input samples)
//'phase_cnt': 40
//'phase_tap_cnt': 16
//'vpolycoeff.size()' = 640, the impulse response is a lowpass filter (at input srate)
//'vout' receives 40 samples, at srate of 1920000Hz
bool polyphase_filter_upsampler_complex_vector( vector<filter_code::st_cplex_float_tag> &vcin, vector<filter_code::st_cplex_float_tag> &vcout, vector<float> &vpolycoeff, unsigned int phase_cnt, unsigned int phase_tap_cnt, bool correct_gain )
{
bool vb = 0;

vcout.clear();


if( vcin.size() < phase_tap_cnt )
	{
	printf( "polyphase_filter_upsampler_complex_vector() - insufficient input data, min count is 'phase_tap_cnt' %d, vcin.size() was: %d\n", (int)vcin.size(), phase_tap_cnt );
	return 0;
	}

if(vb)printf( "polyphase_filter_upsampler_complex_vector() - vcin.size( %d\n", (int)vcin.size() );
if(vb)printf( "polyphase_filter_upsampler_complex_vector() - phase_cnt %d\n", phase_cnt );
if(vb)printf( "polyphase_filter_upsampler_complex_vector() - phase_tap_cnt %d\n", phase_tap_cnt );
if(vb)printf( "\n" );

//int ii = 0;
//int loop_cnt = 0;
//while( ii <= (insize-M) )

//getchar();

int idx_min = 0;
int idx_max = 0;

//int last_idx = 0;


int insize = vcin.size() - phase_tap_cnt;

for( int ii = 0; ii <= insize; ii++ )									//move fwd in 'vcin[]' after all phase coeffs have been convovled with past samples
	{
	int jj = 0;
	for( int phas = 0; phas < phase_cnt; phas++ )						//cycle phase/banks and apply filter coeffs (note last phase/bank is accessed first in code below)
		{
		float sum0 = 0.0f;
		float sum1 = 0.0f;
		float f0 = 0;
		float f1 = 0;
		for( int tap = 0; tap < phase_tap_cnt; tap++ )					//cycle coeffs in cur phase bank and convolve
			{
			int in_idx = ii - tap;										//use a prev input sample for each successive phase bank

			
//if( cin_idx < idx_min ) idx_min = cin_idx;
//if( cin_idx > idx_max ) idx_max = cin_idx;

			if( in_idx < 0 )
				{
				sum0 = 0;
				sum0 = 0;
				}
			else{
				f0 = vcin[ in_idx ].real;
				f1 = vcin[ in_idx ].imag;
//last_idx = cin_idx;

				int coeff_idx = tap*phase_cnt + phas; 						//

				float phas0 = f0 * vpolycoeff[ coeff_idx ];
				float phas1 = f1 * vpolycoeff[ coeff_idx ];

				sum0 += phas0;
				sum1 += phas1;
				if(vb)printf( "polyphase_filter_upsampler_complex_vector() - %02d: in[%03d]  phas %02d tap %02d  coef[%03d] %f   out[%04d]\n", jj, in_idx, phas, tap, coeff_idx, vpolycoeff[ coeff_idx ], (int)vcout.size()  );
				}

			jj++;

//			idbg0++;
//			if( idbg0 >= phase_cnt )idbg0 = 0;			
			}
//getchar();		
//		 if(vb)printf( "polyphase_filter_upsampler_complex_vector()\n" );
		 
		if(vb)printf( "----------\n");
		
		filter_code::st_cplex_float_tag oc;									//store the processed phase resultant sample for fft to use elsewhere
		oc.real = sum0;
		oc.imag = sum1;
		vcout.push_back( oc );
		}

		
//if(vb)printf( "polyphase_filter_upsampler_complex_vector() - last_idx %d  ii %d\n", last_idx, ii );

//getchar();

//	ii += M;
//	if( ii >= insize ) break;

//loop_cnt++;
//if( loop_cnt == 4 ) getchar();
//if( ii == 2 ) getchar();
	}


//if(vb)printf( "polyphase_filter_upsampler_complex_vector() - insize %d vcout.size() %d  vchanIQ0 %d  \n", insize, (int)vcout.size(), (int)vchanIQ0.size() );

if(vb)printf( "polyphase_filter_upsampler_complex_vector() - vcout.size() %d\n", (int)vcout.size() );

//if(1)printf( "polyphase_filter_upsampler_complex_vector() - insize %d vcout.size() %d  vchanIQ0 %d  \n", insize, (int)vcout.size(), (int)vchanIQ0.size() );

if(vb)printf( "polyphase_filter_upsampler_complex_vector() - idx_min %d %d  \n", idx_min, idx_max );
//getchar();

return 1;
}


}		//namespace 'filter_code'









//---------- elliptical iir coeff gen using analogue lpf zero/pole/gain (zpk) prototypes stored in a table ------------
//translated from python code where chatgpt was used, refer: 'test_ellip_zpk_sos_v2.py' commented out code below
//analogue zero/pole(complex) values must be stored in 64 bit double precision

//the analogue lpf prototypes can be loaded using 'load_prototype_from_string()' or from similar formatted 'load_prototype_from_file()' ini file,
//formatting of ini table examples can found be in: '#define cns_ellip_lpf_zpk_proto_builtin_ini'
//you can add your own analogue zero/pole/gain (zpk) prototypes to table for above methods to use

//keep 'order' in multiples of 2, such as 4, 8 or 10, 12, 
//generated digital coeffs (vnumer/vdenom) req use of a number of cascaded sos biquad iirs to implement the filter,
//e.g: a 10th order filter, requires 5 biquad sos cascaded stages for implementation

//NOTE: if you intend to round down the calc'd 64 bit double precision digital coeffs(vnumer/vdenom) to 32 bit floats, be aware your filter may not work as expected,
//particularly if the filter order is high

//use 'sos_init_double()' and then 'sos_cascade_process_double()' to implement the filter on a per sample basis


cl_ellip_builder::cl_ellip_builder()									//v1.19
{
verbose = 0;
st0.valid = 0;
}





//ini file formatted, holds analogue prototype ellip values such as: gain, zeros(complex), poles(complex) for a specific: order (e.g: 8), ripple (e.g: 1.0 dB), stopband (e.g: -60 dB)
//see ALSO 'cl_ellip_builder::load_prototype_from_string()'
bool cl_ellip_builder::load_prototype_from_file( string sfname, unsigned int order_in, float ripple_in, float stopband_in )
{
bool vb0 = verbose;

st0.valid = 0;

if(vb0)printf("cl_ellip_builder::load_prototype_from_file() - '%s'   order %d  ripple %f  stopband %f\n", sfname.c_str(), order_in, ripple_in, stopband_in );


string s1, ssection, skey;

GCProfile p( sfname );
if( !p.Exists() ) return 0;

for( int i = 0; i < cn_ellip_builder_proto_max; i++ )
	{
	strpf( ssection, "ellip_lpf_proto%d", i );

	unsigned int order = p.GetPrivateProfileLONG( ssection, "order", 0 );
	if( !p.check_get_success() ) break;

	float ripple = p.GetPrivateProfileDOUBLE( ssection, "ripple", 0 );
	if( !p.check_get_success() ) break;

	float stopband = p.GetPrivateProfileDOUBLE( ssection, "stopband", 0 );
	if( !p.check_get_success() ) break;

	if(vb0)printf("cl_ellip_builder::load_prototype_from_file() - checking entry[%d]: order %d ripple %f stopband %f\n", i, order, ripple, stopband );
	
	if( order != order_in ) continue;
	if( ripple != ripple_in ) continue;
	if( stopband != stopband_in ) continue;

	if(vb0)printf("cl_ellip_builder::load_prototype_from_file() - found match entry[%d]: order %d ripple %f stopband %f\n", i, order, ripple, stopband );

	float gain = p.GetPrivateProfileDOUBLE( ssection, "gain", 0 );
	if( !p.check_get_success() )
		{
		printf("cl_ellip_builder::load_prototype_from_file() - FAILED to read 'gain' from entry[%d]\n", i );
		return 0;
		}


	if(vb0)printf("cl_ellip_builder::load_prototype_from_file() - read gain %f\n", gain );


	st_ellip_builder_protoype os;
	os.order = order_in;	
	os.ripple = ripple_in;	
	os.stopband = stopband_in;	
	os.gain = gain;

	//read complex zeros
	for( int j = 0; j < cn_ellip_builder_proto_zero_pole_max; j++ )
		{
		strpf( skey, "z%d", j );
		p.GetPrivateProfileStr( ssection, skey, "0,0", &s1 );
		
		if( !p.check_get_success() ) break;
		if(vb0)printf("cl_ellip_builder::load_prototype_from_file() - zero string [%d]'%s'\n", j, s1.c_str() );

		double zr, zi;
		sscanf( s1.c_str(), "%lf,%lf", &zr, &zi );
		
		complex<double> oc( zr, zi );
		os.vzeros.push_back( oc );

		if(vb0)printf("cl_ellip_builder::load_prototype_from_file() - zero complex [%d] '%f + %fi'\n", j, os.vzeros[j].real(), os.vzeros[j].imag() );
		}


	//read complex poles
	for( int j = 0; j < cn_ellip_builder_proto_zero_pole_max; j++ )
		{
		strpf( skey, "p%d", j );
		p.GetPrivateProfileStr( ssection, skey, "0,0", &s1 );
		
		if( !p.check_get_success() ) break;
		if(vb0)printf("cl_ellip_builder::load_prototype_from_file() - pole string [%d]'%s'\n", j, s1.c_str() );

		double pr, pimag;
		sscanf( s1.c_str(), "%lf,%lf", &pr, &pimag );
		
		complex<double> oc( pr, pimag );
		os.vpoles.push_back( oc );

		if(vb0)printf("cl_ellip_builder::load_prototype_from_file() - pole complex [%d] '%f + %fi'\n", j, os.vpoles[j].real(), os.vpoles[j].imag() );
		}

	if( os.vzeros.size() != os.vpoles.size() )
		{
		printf("cl_ellip_builder::load_prototype_from_file() - FAILED, zeros (%d) count does not match poles (%d) count\n", (int)os.vzeros.size(), (int)os.vpoles.size() );
		return 0;
		}
	os.valid = 1;
	
	st0 = os;
	}

return 1;
}









//ini formatted strring, holds analogue prototype ellip values such as: gain, zeros(complex), poles(complex) for a specific: order (e.g: 8), ripple (e.g: 1.0 dB), stopband (e.g: -60 dB)
//see ALSO 'cl_ellip_builder::load_prototype_from_file()'
bool cl_ellip_builder::load_prototype_from_string( string &ss, unsigned int order_in, float ripple_in, float stopband_in )
{
bool vb0 = verbose;

st0.valid = 0;

if(vb0)printf("cl_ellip_builder::load_prototype_from_string() - '%s'   order %d  ripple %f  stopband %f\n", ss.c_str(), order_in, ripple_in, stopband_in );


string s1, ssection, skey;

GCProfile p( "" );
p.Index_from_string( ss );
//if( !p.Exists() ) return 0;

for( int i = 0; i < cn_ellip_builder_proto_max; i++ )
	{
	strpf( ssection, "ellip_lpf_proto%d", i );

	unsigned int order = p.GetPrivateProfileLONG( ssection, "order", 0 );
	if( !p.check_get_success() ) break;

	float ripple = p.GetPrivateProfileDOUBLE( ssection, "ripple", 0 );
	if( !p.check_get_success() ) break;

	float stopband = p.GetPrivateProfileDOUBLE( ssection, "stopband", 0 );
	if( !p.check_get_success() ) break;

	if(vb0)printf("cl_ellip_builder::load_prototype_from_string() - checking entry[%d]: order %d ripple %f stopband %f\n", i, order, ripple, stopband );
	
	if( order != order_in ) continue;
	if( ripple != ripple_in ) continue;
	if( stopband != stopband_in ) continue;

	if(vb0)printf("cl_ellip_builder::load_prototype_from_string() - found match entry[%d]: order %d ripple %f stopband %f\n", i, order, ripple, stopband );

	float gain = p.GetPrivateProfileDOUBLE( ssection, "gain", 0 );
	if( !p.check_get_success() )
		{
		printf("cl_ellip_builder::load_prototype_from_string() - FAILED to read 'gain' from entry[%d]\n", i );
		return 0;
		}


	if(vb0)printf("cl_ellip_builder::load_prototype_from_string() - read gain %f\n", gain );


	st_ellip_builder_protoype os;
	os.order = order_in;	
	os.ripple = ripple_in;	
	os.stopband = stopband_in;	
	os.gain = gain;

	//read complex zeros
	for( int j = 0; j < cn_ellip_builder_proto_zero_pole_max; j++ )
		{
		strpf( skey, "z%d", j );
		p.GetPrivateProfileStr( ssection, skey, "0,0", &s1 );

		if( !p.check_get_success() ) break;
		if(vb0)printf("cl_ellip_builder::load_prototype_from_string() - zero string [%d]'%s'\n", j, s1.c_str() );

		double zr, zi;
		sscanf( s1.c_str(), "%lf,%lf", &zr, &zi );

		complex<double> oc( zr, zi );
		os.vzeros.push_back( oc );

		if(vb0)printf("cl_ellip_builder::load_prototype_from_string() - zero complex [%d] '%f + %fi'\n", j, os.vzeros[j].real(), os.vzeros[j].imag() );
		}


	//read complex poles
	for( int j = 0; j < cn_ellip_builder_proto_zero_pole_max; j++ )
		{
		strpf( skey, "p%d", j );
		p.GetPrivateProfileStr( ssection, skey, "0,0", &s1 );
		
		if( !p.check_get_success() ) break;
		if(vb0)printf("cl_ellip_builder::load_prototype_from_string() - pole string [%d]'%s'\n", j, s1.c_str() );

		double pr, pimag;
		sscanf( s1.c_str(), "%lf,%lf", &pr, &pimag );
		
		complex<double> oc( pr, pimag );
		os.vpoles.push_back( oc );

		if(vb0)printf("cl_ellip_builder::load_prototype_from_string() - pole complex [%d] '%f + %fi'\n", j, os.vpoles[j].real(), os.vpoles[j].imag() );
		}

	if( os.vzeros.size() != os.vpoles.size() )
		{
		printf("cl_ellip_builder::load_prototype_from_string() - FAILED, zeros (%d) count does not match poles (%d) count\n", (int)os.vzeros.size(), (int)os.vpoles.size() );
		return 0;
		}
	os.valid = 1;
	
	st0 = os;
	}

return 1;
}





//'type'  0: lpf  1: hpf  2: bpf  3: notch								//filter shape
//'vnumer' will be loaded with: b0, b1, b2, b0, b1, b2  etc				//use in second order sections (SOS)
//'vdenom' will be loaded with: 1.0, a1, a2, 1.0, a1, a2  etc			//a0 is always 1.0, use in second order sections (SOS)

bool cl_ellip_builder::calc_iir_coeffs( unsigned int type, unsigned int fc0, unsigned int fc1, unsigned int srate, vector<double> &vnumer_out, vector<double> &vdenom_out )
{
bool vb0 = verbose;

if( !st0.valid )
	{
	if(vb0)printf("cl_ellip_builder::calc_iir_coeffs() - 'st0' is not valid, did you succeed in loading a matched prototype ?\n" );
	return 0;
	}

if(vb0)printf("cl_ellip_builder::calc_iir_coeffs() - 'st0' is not valid, did you succeed in loading a matched prototype ?\n" );


vnumer_out.clear();
vdenom_out.clear();

vector< complex<double> > vdz;
vector< complex<double> > vdp;
double kd;

st0.type = type;
st0.srate = srate;
st0.fc0 = fc0;
st0.fc1 = fc1;


analog_to_digital( st0.type, st0.vzeros, st0.vpoles, st0.gain, st0.fc0, st0.fc1, st0.srate, vdz, vdp, kd );


zpk2sos( type, vdz, vdp, kd, vnumer_out, vdenom_out );

st0.vnumer = vnumer_out;
st0.vdenom = vdenom_out;
return 1;
}








double cl_ellip_builder::sos_gain_dc(
	vector<double> &vnumer,
	vector<double> &vdenom)
{
double gain = 1.0;

int sections = vnumer.size() / 3;

for( int i = 0; i < sections; i++ )
	{
	double b =
		vnumer[i*3 + 0] +
		vnumer[i*3 + 1] +
		vnumer[i*3 + 2];

	double a =
		vdenom[i*3 + 0] +
		vdenom[i*3 + 1] +
		vdenom[i*3 + 2];

	gain *= b / a;
	}

return fabs(gain);
}









void cl_ellip_builder::zpk2sos(
	unsigned int type,
	vector< complex<double> > &vdz,
	vector< complex<double> > &vdp,
	double kd,
	vector<double> &vnumer,
	vector<double> &vdenom)
{
bool vb0 = verbose;
	
//extract the upper half-plane poles/zeros
//(those above the real axis, i.e. imaginary part is positive)

vector< complex<double> > vzpostv;
vector< complex<double> > vppostv;

for( int i = 0; i < vdz.size(); i++ )
	{
	if( vdz[i].imag() > 0.0 )
		{
		vzpostv.push_back(vdz[i]);
		}

	if( vdp[i].imag() > 0.0 )
		{
		vppostv.push_back(vdp[i]);
		}
	}


if( vb0 )
	{
	for( int i = 0; i < vzpostv.size(); i++ )
		{
		printf("cl_ellip_builder::zpk2sos() - vzpostv[%d] %.17f + %.17fi\n",
			i, vzpostv[i].real(), vzpostv[i].imag());
		}

	for( int i = 0; i < vppostv.size(); i++ )
		{
		printf("\ncl_ellip_builder::zpk2sos() - vppostv[%d] %.17f + %.17fi\n",
			i, vppostv[i].real(), vppostv[i].imag());
		}
	}



vector<double> vb, va;

for( int i = 0; i < vzpostv.size(); i++ )
	{
	complex<double> z2, p2;
	
	z2 = conj(vzpostv[i]);
	p2 = conj(vppostv[i]);
	
	vb.push_back(1.0 * kd);
	vb.push_back(-(vzpostv[i] + z2).real() * kd);
	vb.push_back((vzpostv[i] * z2).real() * kd);

	kd = 1.0;
	
	va.push_back(1.0);
	va.push_back(-(vppostv[i] + p2).real());
	va.push_back((vppostv[i] * p2).real());
	}


//
// Notch gain correction.
//
// Normalize the complete SOS filter at DC.
// For a band-stop filter the low-frequency
// and high-frequency passbands have the same
// limiting gain, so DC is a convenient reference.
//
if( type == 3 )
	{
	double gain = sos_gain_dc(vb, va);

	if( gain > 0.0 )
		{
		double correction = 1.0 / gain;

		vb[0] *= correction;
		vb[1] *= correction;
		vb[2] *= correction;

		if( vb0 )
			{
			printf("cl_ellip_builder::zpk2sos() - notch DC gain = %.17f\n", gain);
			printf("cl_ellip_builder::zpk2sos() - notch correction = %.17f\n", correction);
			}
		}
	}

if( vb0 )
	{
	int sections = vb.size() / 3;

	printf("cl_ellip_builder::zpk2sos() - sections total: %d\n\n", sections );
	
	for( int i = 0; i < sections; i++ )
		{
		printf("cl_ellip_builder::zpk2sos() - section %d\n", i );
		printf("cl_ellip_builder::zpk2sos() - b0 = %.17f\n", vb[i*3 + 0]);
		printf("cl_ellip_builder::zpk2sos() - b1 = %.17f\n", vb[i*3 + 1]);
		printf("cl_ellip_builder::zpk2sos() - b2 = %.17f\n", vb[i*3 + 2]);
		printf("cl_ellip_builder::zpk2sos() - a1 = %.17f\n", va[i*3 + 1]);
		printf("cl_ellip_builder::zpk2sos() - a2 = %.17f\n\n", va[i*3 + 2]);
		}
	}

vnumer = vb;
vdenom = va;
}









void cl_ellip_builder::lp_to_bs_zpk(
	vector< complex<double> > &vz,
	vector< complex<double> > &vp,
	double k,
	double wlow,
	double whigh,
	double &kbs,
	vector< complex<double> > &bz,
	vector< complex<double> > &bp)
{
double bw = whigh - wlow;
double w0 = sqrt(wlow * whigh);

bz.clear();
bp.clear();

//
// Low-pass -> band-stop:
//
//		B*s
//	q = -----------
//		s^2 + w0^2
//
// Therefore:
//
//	q*s^2 - bw*s + q*w0^2 = 0
//
// s = (bw +/- sqrt(bw^2 - 4*q^2*w0^2)) / (2*q)
//

//
// Transform finite zeros
//
for( int i = 0; i < vz.size(); i++ )
	{
	complex<double> q = vz[i];

	complex<double> disc =
		sqrt(bw*bw - 4.0*q*q*w0*w0);

	bz.push_back((bw + disc) / (2.0*q));
	bz.push_back((bw - disc) / (2.0*q));
	}

//
// Transform poles
//
for( int i = 0; i < vp.size(); i++ )
	{
	complex<double> q = vp[i];

	complex<double> disc =
		sqrt(bw*bw - 4.0*q*q*w0*w0);

	bp.push_back((bw + disc) / (2.0*q));
	bp.push_back((bw - disc) / (2.0*q));
	}

//
// LP zeros at infinity become notch zeros
// at +/-j*w0.
//
int nz_extra = vp.size() - vz.size();

for( int i = 0; i < nz_extra; i++ )
	{
	bz.push_back(complex<double>(0.0,  w0));
	bz.push_back(complex<double>(0.0, -w0));
	}

//
// Gain deliberately left for later gain correction.
//
kbs = k;
}







void cl_ellip_builder::lp_to_bp_zpk(
	vector< complex<double> > &vz,
	vector< complex<double> > &vp,
	double k,
	double wc0,
	double wc1,
	double &kbp,
	vector< complex<double> > &bz,
	vector< complex<double> > &bp)
{
double bw = wc1 - wc0;
double w0 = sqrt(wc0 * wc1);

bz.clear();
bp.clear();

//
// Low-pass -> band-pass:
//
//		s^2 + w0^2
//	q = ----------------
//		     bw * s
//
// For each prototype pole/zero q:
//
//	s^2 - bw*q*s + w0^2 = 0
//

//
// Transform finite zeros
//
for( int i = 0; i < vz.size(); i++ )
	{
	complex<double> q = vz[i];

	complex<double> disc =
		sqrt((bw*q)*(bw*q) - 4.0*w0*w0);

	bz.push_back((bw*q + disc) / 2.0);
	bz.push_back((bw*q - disc) / 2.0);
	}

//
// Transform poles
//
for( int i = 0; i < vp.size(); i++ )
	{
	complex<double> q = vp[i];

	complex<double> disc =
		sqrt((bw*q)*(bw*q) - 4.0*w0*w0);

	bp.push_back((bw*q + disc) / 2.0);
	bp.push_back((bw*q - disc) / 2.0);
	}

//
// Every prototype zero at infinity becomes a zero at s = 0.
//
// For an elliptic prototype this is normally:
//
//	number of poles - number of finite zeros
//
int nz_extra = vp.size() - vz.size();

for( int i = 0; i < nz_extra; i++ )
	{
	bz.push_back(complex<double>(0.0, 0.0));
	}

//
// Gain.
//
// For the substitution
//
//	s_lp = (s^2 + w0^2)/(bw*s)
//
// the leading coefficient introduces the bandwidth
// factor for each zero at infinity.
//
kbp = k * pow(bw, nz_extra);
}






void cl_ellip_builder::analog_to_digital(
	unsigned int type,
	vector< complex<double> > &vz,
	vector< complex<double> > &vp,
	double k,
	double fc0,
	double fc1,
	double fs,
	vector< complex<double> > &vdz,
	vector< complex<double> > &vdp,
	double &kd)
{
bool vb0 = verbose;


double wc0 = 2.0 * fs * tan(M_PI * fc0/fs);
double wc1 = 2.0 * fs * tan(M_PI * fc1/fs);

int shape = type;	// 0: lpf  1: hpf  2: bpf  3: notch

if( shape == 1 )
	{
	// high pass conversion

	for( int i = 0; i < vz.size(); i++ )
		{
		vz[i] = 1.0 / vz[i];
		}

	for( int i = 0; i < vp.size(); i++ )
		{
		vp[i] = 1.0 / vp[i];
		}

	k = 1.0;
	}

else if( shape == 2 )
	{
	// band pass conversion
	//
	// fc0 = lower cutoff
	// fc1 = upper cutoff

	vector< complex<double> > vbz;
	vector< complex<double> > vbp;

	double kb;

	lp_to_bp_zpk(vz, vp, k, wc0, wc1, kb, vbz, vbp);

	vz = vbz;
	vp = vbp;
	k = kb;
	}

else if( shape == 3 )
	{
	// band stop / notch conversion
	//
	// fc0 = lower edge of notch
	// fc1 = upper edge of notch

	vector< complex<double> > vbz;
	vector< complex<double> > vbp;

	double kb;

	lp_to_bs_zpk(vz, vp, k, wc0, wc1, kb, vbz, vbp);

	vz = vbz;
	vp = vbp;
	k = kb;
	}


vector< complex<double> > vzwc;
vector< complex<double> > vpwc;

if( shape == 2 || shape == 3 )
	{
	// BPF / notch transformation has already
	// produced actual analogue-frequency
	// poles and zeros.

	vzwc = vz;
	vpwc = vp;
	}
else
	{
	// LPF / HPF: scale normalized prototype by wc0.

	for( int i = 0; i < vz.size(); i++ )
		{
		vzwc.push_back(wc0 * vz[i]);
		}

	for( int i = 0; i < vp.size(); i++ )
		{
		vpwc.push_back(wc0 * vp[i]);
		}
	}


if(vb0)
	printf("cl_ellip_builder::analog_to_digital() - wc0 %f\n", wc0);

bilinear_zpk(vzwc, vpwc, k, fs, vdz, vdp, kd);
}









//convert to zero/poles to digital
void cl_ellip_builder::bilinear_zpk(vector< complex<double> > &vz, vector< complex<double> > &vp, double k, double fs, vector< complex<double> > &vdz, vector< complex<double> > &vdp, double &k_d )
{
bool vb0 = verbose;

double fs2 = 2.0 * fs;

vdz.clear();
vdp.clear();

if( vb0 )
	{
	for( int i = 0; i < vz.size(); i++ )
		{
		printf("cl_ellip_builder::analog_to_digital() - vz[%d]  %f + %fj\n", i, vz[i].real(), vz[i].imag() );
		}

	for( int i = 0; i < vp.size(); i++ )
		{
		printf("cl_ellip_builder::analog_to_digital() - vp[%d]  %f + %fj\n", i, vp[i].real(), vp[i].imag() );
		}

	}

//map finite zeros
for( int i = 0; i < vz.size(); i++ )
	{
	vdz.push_back( (fs2 + vz[i]) / (fs2 - vz[i]) );
	if(vb0)printf("cl_ellip_builder::analog_to_digital() - vdz[%d]  %f + %fj\n", i, vdz[i].real(), vdz[i].imag() );
	}

//map poles
for( int i = 0; i < vp.size(); i++ )
	{
	vdp.push_back( (fs2 + vp[i]) / (fs2 - vp[i]) );
	if(vb0)printf("cl_ellip_builder::analog_to_digital() - vdp[%d]  %f + %fj\n", i, vdp[i].real(), vdp[i].imag() );
	}


//add zeros at z=-1 for analogue zeros at infinity, if more poles than zeroes exist
int degree = vp.size() - vz.size();

//degree = 5;

for( int i = 0; i < degree; i++ )
	{
	complex<double> oc(1,0);											//1 + 0i
	
	vdz.push_back( -oc );												//store -1 + 0i
	}

if( degree > 0 )
	{
	if(vb0)printf( "cl_ellip_builder::analog_to_digital() - added %d digital zeros at z=-1 for analogue zeros at infinity (if had more analogue poles than zeroes )\n", degree );
	for( int i = 0; i < vdz.size(); i++ )
		{
		printf("cl_ellip_builder::analog_to_digital() - vdz_ones[%d]  %f + %fj\n", i, vdz[i].real(), vdz[i].imag() );
		}
	}


//gain correction
complex<double> oc0( 0, 0 );
complex<double> oc1( 0, 0 );
for( int i = 0; i < vz.size(); i++ )
	{
	if( i == 0 )														//initial product setup
		{
		if(vb0)printf("product vz[%d] %f +%fi\n", i, vz[i].real(), vz[i].imag() );
		oc0 = fs2 - vz[i];
		oc1 = fs2 - vp[i];
		}
	else{
		if(vb0)printf("product vz[%d] %f +%fi\n", i, vz[i].real(), vz[i].imag() );
		oc0 *= fs2 - vz[i];												//implement product 
		oc1 *= fs2 - vp[i];
		}
	}

complex<double> oc3;

oc3 = oc0 / oc1;

k_d = k * oc3.real();

if(vb0) printf( "cl_ellip_builder::analog_to_digital() - fs2 %f   k %f  k_d %.17f )\n", fs2, k, k_d );


//correct gain for added zeros at infinity
k_d *= pow( fs2, degree );

if(vb0) printf( "cl_ellip_builder::analog_to_digital() - k_d %.17f (if zeros were added at infinity)\n", k_d );
}






// iir biquad sos - process (Direct Form II Transposed)
float cl_ellip_builder::sos_biquad_process_float(biquad_float& q, float x )
{
    float y = q.b0 * x + q.z1;
    q.z1 = q.b1 * x - q.a1 * y + q.z2;
    q.z2 = q.b2 * x - q.a2 * y;
    return y;
}


// iir cascaded biquad sos sections - filter process (per sample)
float cl_ellip_builder::sos_cascade_process_float( float x )
{
for(int i = 0; i < sections; i++)
	{
	x = sos_biquad_process_float( sec_float[i], x);
	}

return x;
}




// iir biquad sos - process (Direct Form II Transposed)
double cl_ellip_builder::sos_biquad_process_double( biquad_double& q, double x )
{
    double y = q.b0 * x + q.z1;
    q.z1 = q.b1 * x - q.a1 * y + q.z2;
    q.z2 = q.b2 * x - q.a2 * y;
    return y;
}


// iir cascaded biquad sos sections - filter process (per sample)
double cl_ellip_builder::sos_cascade_process_double( double x )
{
for(int i = 0; i < sections; i++)
	{
	x = sos_biquad_process_double( sec_double[i], x );
	}

return x;
}




// load coeffs into cascaded biquad sos sections
//'vnumer' loaded with: b0, b1, b2, b0, b1, b2  etc			(3 ceoffs per sos)
//'vdenom' loaded with:  1.0, a1, a2, 1.0, a1, a2  etc	    a0 is always 1.0 (3 ceoffs per sos)
bool cl_ellip_builder::sos_init_float( vector<double> &vnumer, vector<double> &vdenom )
{
bool vb0 = verbose;

sections = 0;

if( vnumer.size() == 0 )
	{
	printf("cl_ellip_builder::sos_init_float() - vnumer.size() is empty\n" );
	return 0;
	}

if( (vnumer.size() % 3) != 0 )
	{
	printf("cl_ellip_builder::sos_init_float() - vnumer.size() %d is not a factor of 3\n", (int)vnumer.size() );
	return 0;
	}

if( vnumer.size() != vdenom.size() )
	{
	printf("cl_ellip_builder::sos_init_float() - FAILED, zeros (%d) count does not match poles (%d) count\n", (int)vnumer.size(), (int)vdenom.size() );
	return 0;
	}

if( (vnumer.size() / 3) >= cn_ellip_builder_sos_sections_max )
	{
	printf("cl_ellip_builder::sos_init_float() - too many sections, max is %d, found: %d \n", cn_ellip_builder_sos_sections_max, ((int)vnumer.size() / 3) );
	return 0;
	}

sections = vnumer.size() / 3;

if(vb0)printf("cl_ellip_builder::sos_init_float() - loaded %d sections\n", sections );


for (int i = 0; i < sections; i++)
	{
	sec_float[i].b0 = vnumer[i*3+0];
	sec_float[i].b1 = vnumer[i*3+1];
	sec_float[i].b2 = vnumer[i*3+2];
	sec_float[i].a1 = vdenom[i*3+1];
	sec_float[i].a2 = vdenom[i*3+2];
	sec_float[i].z1 = 0.0f;
	sec_float[i].z2 = 0.0f;
	
if(vb0)printf("cl_ellip_builder::sos_init_float() - sec_float[%d].b0 %f     sec[%d].a0 %f\n", i, sec_float[i].b0, i, 1.0  );
if(vb0)printf("cl_ellip_builder::sos_init_float() - sec_float[%d].b1 %f     sec[%d].a1 %f\n", i, sec_float[i].b1, i, sec_float[i].a1  );
if(vb0)printf("cl_ellip_builder::sos_init_float() - sec_float[%d].b2 %f     sec[%d].a2 %f\n", i, sec_float[i].b2, i, sec_float[i].a2  );

	}



for (int i = 0; i < sections; i++)										//load sos
	{
	sec_float[i].b0 = vnumer[i*3+0];
	sec_float[i].b1 = vnumer[i*3+1];
	sec_float[i].b2 = vnumer[i*3+2];
	sec_float[i].a1 = vdenom[i*3+1];
	sec_float[i].a2 = vdenom[i*3+2];
	sec_float[i].z1 = 0.0f;
	sec_float[i].z2 = 0.0f;
	}

if(vb0)
	{
	printf( "\n-------- cl_ellip_builder::sos_init_float() - b0,b1,b2,a1,a2 ordering --------\n" );

	for (int i = 0; i < sections; i++)
		{
		printf( "%.17f\n", sec_float[i].b0 );
		printf( "%.17f\n", sec_float[i].b1 );
		printf( "%.17f\n", sec_float[i].b2 );
		printf( "%.17f\n", sec_float[i].a1 );
		printf( "%.17f\n", sec_float[i].a2 );

		}
	printf( "--------------------------------------------------------------------------\n" );
	}

return 1;
}







// load coeffs into cascaded biquad sos sections
//'vnumer' loaded with: b0, b1, b2, b0, b1, b2  etc			(3 ceoffs per sos)
//'vdenom' loaded with:  1.0, a1, a2, 1.0, a1, a2  etc	    a0 is always 1.0 (3 ceoffs per sos)
bool cl_ellip_builder::sos_init_double( vector<double> &vnumer, vector<double> &vdenom )
{
bool vb0 = verbose;

sections = 0;

if( vnumer.size() == 0 )
	{
	printf("cl_ellip_builder::sos_init_double() - vnumer.size() is empty\n" );
	return 0;
	}

if( (vnumer.size() % 3) != 0 )
	{
	printf("cl_ellip_builder::sos_init_double() - vnumer.size() %d is not a factor of 3\n", (int)vnumer.size() );
	return 0;
	}

if( vnumer.size() != vdenom.size() )
	{
	printf("cl_ellip_builder::sos_init_double() - FAILED, zeros (%d) count does not match poles (%d) count\n", (int)vnumer.size(), (int)vdenom.size() );
	return 0;
	}

if( (vnumer.size() / 3) >= cn_ellip_builder_sos_sections_max )
	{
	printf("cl_ellip_builder::sos_init_double() - too many sections, max is %d, found: %d \n", cn_ellip_builder_sos_sections_max, ((int)vnumer.size() / 3) );
	return 0;
	}

sections = vnumer.size() / 3;

if(vb0)printf("cl_ellip_builder::sos_init_double() - loaded %d sections\n", sections );


for (int i = 0; i < sections; i++)
	{
	sec_double[i].b0 = vnumer[i*3+0];
	sec_double[i].b1 = vnumer[i*3+1];
	sec_double[i].b2 = vnumer[i*3+2];
	sec_double[i].a1 = vdenom[i*3+1];
	sec_double[i].a2 = vdenom[i*3+2];
	sec_double[i].z1 = 0.0f;
	sec_double[i].z2 = 0.0f;
	
if(vb0)printf("cl_ellip_builder::sos_init_double() - sec_double[%d].b0 %f     sec_double[%d].a0 %f\n", i, sec_double[i].b0, i, 1.0  );
if(vb0)printf("cl_ellip_builder::sos_init_double() - sec_double[%d].b1 %f     sec_double[%d].a1 %f\n", i, sec_double[i].b1, i, sec_double[i].a1  );
if(vb0)printf("cl_ellip_builder::sos_init_double() - sec_double[%d].b2 %f     sec_double[%d].a2 %f\n", i, sec_double[i].b2, i, sec_double[i].a2  );

	}



for (int i = 0; i < sections; i++)										//load sos
	{
	sec_double[i].b0 = vnumer[i*3+0];
	sec_double[i].b1 = vnumer[i*3+1];
	sec_double[i].b2 = vnumer[i*3+2];
	sec_double[i].a1 = vdenom[i*3+1];
	sec_double[i].a2 = vdenom[i*3+2];
	sec_double[i].z1 = 0.0f;
	sec_double[i].z2 = 0.0f;
	}

if(vb0)
	{
	printf( "\n-------- cl_ellip_builder::sos_init() - b0,b1,b2,a1,a2 ordering --------\n" );

	for (int i = 0; i < sections; i++)
		{
		printf( "%.17f\n", sec_double[i].b0 );
		printf( "%.17f\n", sec_double[i].b1 );
		printf( "%.17f\n", sec_double[i].b2 );
		printf( "%.17f\n", sec_double[i].a1 );
		printf( "%.17f\n", sec_double[i].a2 );

		}
	printf( "--------------------------------------------------------------------------\n" );
	}

return 1;
}



/* PYTHON3 CODE
# test_ellip_zpk_sos_v2.py
#
# Enhanced version of the validated prototype:
#  - Writes/reads ZPK table with // comments
#  - Analog LP -> Digital (prewarp + bilinear)
#  - my_zpk2sos()
#  - Compares against SciPy
#  - Plots response comparison
#  - Dumps SOS coefficients one per line for C
#

# !!!!!!!!!!!!!!!! DOES HP as well JUST UNCOMMENT below at: #convert to a HPF !!!!!!!!!!!!!!!!

import numpy as np
import matplotlib.pyplot as plt
from scipy import signal

order=10
rp=1.0
rs=40.0
fc=10000.0
fs=96000.0
table_filename="ellip_lpf_zpk_proto_non_ini.txt"
table_filename_ini="ellip_lpf_zpk_proto_one_entry.txt"

vb = 0


# [ellip_lpf_proto0]
# order=8
# ripple=1.0
# stopband=-60.0
# gain=0.001
# z0=0,3.9165773784910107
# p0=0,3.9165773784910107

def write_zpk_table_ini_format(fn,z,p,k):
    print("====write_zpk_table_ini_format()====")
    with open(fn,"w") as f:
        f.write("[ellip_lpf_protoXXXXXXXXXXXX]\n" )
        f.write("order=%d\n"%order )
        f.write("ripple=%f\n"%rp )
        f.write("stopband=%f\n"%-rs )
        f.write("gain=%f\n"%k )
        i=0
        for v in z:
            f.write("z%d=%.17g,%.17g\n"%(i,v.real,v.imag) )
            i+=1
        i=0
        for v in p:
            f.write("p%d=%.17g,%.17g\n"%(i,v.real,v.imag) )
            i+=1
        print("\nsaved analogue zero/pole/gain prototype as an ini in file: %s\n"%fn)

def write_zpk_table(fn,z,p,k):
    print("====write_zpk_table()====")
    print("order %d   ripple %f   stopband %d\n"%(order,rp,rs) )
    with open(fn,"w") as f:
        f.write("// Elliptic LP prototype\n")
        f.write(f"// Order: {order}\n// Ripple: {rp}\n// Stopband: {rs}\n\n")
        f.write("gain\n%.17g\n\nzeros\n"%k)
        for v in z:
            f.write("%.17g %.17g\n"%(v.real,v.imag))
        f.write("\npoles\n")
        for v in p:
            f.write("%.17g %.17g\n"%(v.real,v.imag))

def read_zpk_table(fn):
    z=[];p=[];k=1.0;mode=""
    for line in open(fn):
        s=line.strip()
        if not s or s.startswith("//"): continue
        if s in ("gain","zeros","poles"):
            mode=s; continue
        if mode=="gain":
            k=float(s); mode=""
        elif mode=="zeros":
            a,b=s.split(); z.append(complex(float(a),float(b)))
        elif mode=="poles":
            a,b=s.split(); p.append(complex(float(a),float(b)))
    return np.array(z),np.array(p),k




def analog_to_digital(z,p,k,fc,fs):
    wc=2*fs*np.tan(np.pi*fc/fs)
    return signal.bilinear_zpk(z*wc,p*wc,k,fs)





def my_bilinear_zpk(z, p, k, fs):
    if vb: 
        print("==== my_bilinear_zpk() =====")

        print("-----------")
        print("z=", z)
        print("-----------")

        print("-----------")
        print("p=", p)
        print("-----------")
    
    fs2 = 2.0 * fs

    z = np.asarray(z, dtype=complex)
    p = np.asarray(p, dtype=complex)

    if vb: 
        print("-----------")
        print("zarray=", z)
        print("-----------")
 
        print("-----------")
        print("parray=", p)
        print("-----------")

    # Map finite zeros
    z_d = (fs2 + z) / (fs2 - z)

    # Map poles
    p_d = (fs2 + p) / (fs2 - p)
 
    if vb: 
        print("-----------")
        print("z_d =", z_d)
        print("-----------")
        
        print("-----------")
        print("p_d =", p_d)
        print("-----------")

    # Add zeros at z=-1 for analogue zeros at infinity
    degree = len(p) - len(z)
#    degree = 5;

    if vb: 
        print("degree", degree)

    if degree > 0:
        z_d = np.concatenate((z_d, -np.ones(degree)))

#    if vb: 
#        print("-----------")
#        print("z_d2_ones =", z_d2)
#        print("-----------")


    # Gain correction
    k_d = k * np.real( np.prod(fs2 - z) / np.prod(fs2 - p) )

    if vb: 
        print("fs2  k-->k_d", fs2, k, k_d)


    # Correct gain for added zeros at infinity
    k_d *= fs2 ** (len(p) - len(z))

    if vb: 
        print("k_d (if zeros were added at infinity)", k_d)

    return z_d, p_d, k_d




def analog_to_digital_discrete(z,p,k,fc,fs):
    wc=2*fs*np.tan(np.pi*fc/fs)
    if vb: 
        print("==== analog_to_digital_discrete() ====")

        print("wc=", wc)        
    return my_bilinear_zpk(z*wc,p*wc,k,fs)






def my_zpk2sos(z,p,k):
    if vb: 
        print("==== my_zpk2sos() ====")
    zp=[v for v in z if v.imag>0]
    pp=[v for v in p if v.imag>0]

    if vb: 
        print("zp[]", zp)
        print("pp[]", pp)
    
    sos=[]
    for z1,p1 in zip(zp,pp):
        z2=np.conj(z1); p2=np.conj(p1)
        b=[1.0,-(z1+z2).real,(z1*z2).real]
        a=[1.0,-(p1+p2).real,(p1*p2).real]

        if vb:
            print("b", b)
            print("a", a)
    
        sos.append(b+a)

    sos=np.array(sos)
    sos[0,0:3]*=k

    if vb: 
        print("sos[]", sos)
    
    return sos




def dump_c_coeffs(sos):
    print("\n===== COEFFICIENTS FOR C =====")
    for sec,s in enumerate(sos):
        print(f"\n// section {sec}")
        print(f"b0 = {s[0]:.17g}")
        print(f"b1 = {s[1]:.17g}")
        print(f"b2 = {s[2]:.17g}")
        print(f"a1 = {s[4]:.17g}")
        print(f"a2 = {s[5]:.17g}")
    print("\n===== COPY/PASTE FORMAT b0,b1,b2,a1,a2 =====")
    for s in sos:
        for i in (0,1,2,4,5):
            print(f"{s[i]:.17g}")



print("==== main code ====")

z,p,k=signal.ellipap(order,rp,rs)

write_zpk_table(table_filename,z,p,k)
z,p,k=read_zpk_table(table_filename)

write_zpk_table_ini_format(table_filename_ini,z,p,k)



if vb: 
     print("---- analogue zeros/poles -----")
     print("za0:",z)
     print("-----------")
     print("pa0:",p )
     print("-----------")

#zd,pd,kd=analog_to_digital(z,p,k,fc,fs)
#print("zd:",zd )


#---------------------------------
#z = 1.0/z;		#convert to a HPF
#p = 1.0/p;		#convert to a HPF
#k = 1.0;		#convert to a HPF
#---------------------------------

zd,pd,kd=analog_to_digital_discrete(z,p,k,fc,fs)		#see also scipy version above 'analog_to_digital()'

if vb: 
    print("---digial poles/zeros -----")
    print("zd0:",zd )
    print("-----------")
    print("pd0:",pd )
    print("-----------")

mine=my_zpk2sos(zd,pd,kd)
ref=signal.zpk2sos(zd,pd,kd)

print("Max SOS coefficient error:",np.max(np.abs(mine-ref)))

w,h1=signal.sosfreqz(mine,worN=8192,fs=fs)
_,h2=signal.sosfreqz(ref,worN=8192,fs=fs)
m1=20*np.log10(np.maximum(np.abs(h1),1e-12))
m2=20*np.log10(np.maximum(np.abs(h2),1e-12))
print("Max response error (dB):",np.max(np.abs(m1-m2)))

dump_c_coeffs(mine)

print("\nsaved analogue zero/pole/gain prototype as an ini in file: %s\n"%table_filename_ini)

plt.figure()
plt.plot(w,m1,label="my_zpk2sos")
plt.plot(w,m2,"--",label="SciPy")
plt.grid(True)
plt.legend()
plt.title("SOS Response Comparison")
plt.xlabel("Frequency (Hz)")
plt.ylabel("Magnitude (dB)")
plt.show(block=False)
input("Hit enter key to quit and close any plots:")
plt.close('all')
*/

//------------------------------------------------------------------------------------------------------------




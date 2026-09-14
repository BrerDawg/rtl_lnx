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


//filter_code.h
//v1.19

#ifndef filter_code_h
#define filter_code_h

#include <stdio.h>
#include <string.h>
#include <string>
#include <wchar.h>
#include <math.h>
#include <vector>
#include <complex>
#include <cmath>														//for std::cyl_bessel_i();

//#include "globals.h"					//v1.06
#include "GCProfile.h"
//#include "mgraph.h"					//v1.06
//#include "audio_formats.h"


using namespace std;



#define cn_filter_tap_limit 16384


namespace filter_code
	{
		
//	extern double kaiser_alpha;
	extern double kaiser_beta;											//v1.15
	
	enum en_filter_pass_type_tag
	{
	fpt_undefined,
	fpt_lowpass,
	fpt_highpass,
	fpt_bandpass,

	fpt_notch,

	fpt_bandpass2,                  //use by: filter_iir_2nd_order(..)
	fpt_apf,
	fpt_peakeq,
	fpt_lowshelf,
	fpt_highshelf,
	};








	//refer http://www.mikroe.com/chapters/view/72/chapter-2-fir-filters/
	enum en_filter_window_type_tag
	{
	fwt_undefined,
	fwt_rect,
	fwt_kaiser,                     // ADJUST ALSO as req: 'kaiser_beta'
	fwt_bartlett,
	fwt_hann,
	fwt_bartlett_hanning,
	fwt_hamming,
	//fwt_bohman,                   //not yet supported
	fwt_blackman,
	fwt_blackman_harris,

	fwt_lanczos1,					//v1.03
	fwt_lanczos1_5,					//v1.03
	fwt_lanczos2,					//v1.03
	fwt_lanczos3,					//v1.03
	};





	struct st_cplex_tag
	{
	double real;
	double imag;

    //conjugate
    st_cplex_tag conj() const											//v1.13
		{
		return { real, -imag };
		}

    //complex multiply
    st_cplex_tag operator*(const st_cplex_tag& rhs) const
		{
		return 
			{
			real * rhs.real - imag * rhs.imag,
			real * rhs.imag + imag * rhs.real
			};
		}
	};





	struct st_cplex_float_tag
	{
	float real;
	float imag;

    //conjugate
    st_cplex_float_tag conj() const										//v1.13
		{
		return { real, -imag };
		}

    //complex multiply
    st_cplex_float_tag operator*(const st_cplex_float_tag& rhs) const
		{
		return 
			{
			real * rhs.real - imag * rhs.imag,
			real * rhs.imag + imag * rhs.real
			};
		}
	};





	typedef struct 														//see 'create_filter_from_coeffs()' function for EXAMPLE of HOW to set this structure up
	{
	bool verb;															//verbose for debugging, v1.06
	string suser0;														//user string for debugging	v1.06
	string suser1;
	int user_id0;														//user num storage
	int user_id1;
	bool created;
	int coeff_cnt;
	double *coeff_ptr;
	double *prev;
	unsigned int prev_idx;
	bool bypass;

	} st_fir;




	//v1.03, see also, see also 'st_iir_2nd_order_tag'
	typedef struct
	{
	bool verb;															//verbose for debugging, v1.07
	string suser0;														//user string for debugging	v1.07
	string suser1;
	int user_id0;														//user num storage v1.07
	int user_id1;

	bool bypass;

	bool created;
	int coeff_cnt;
//	double *coeff_ptr;													//v1.07

	double a1, a2;
	double b0, b1, b2;

	double dly0;														//z0
	double dly1;														//z1

	double fc;											//v1.20, handy var for user to make note of iir specs, not actually set by any code avail here or used in calcs
	double q;											//v1.20, handy var for user to make note of iir specs, not actually set by any code avail here or used in calcs
	double db_gain;										//v1.20, e.g: for shelving iir, handy var for user to make note of iir specs, not actually set by any code avail here or used in calcs
	
	float f_a1, f_a2;													//v1.16, float precision
	float f_b0, f_b1, f_b2;

	float f_dly0;														//z0
	float f_dly1;														//z1

	} st_iir;











	//v1.02, see also 'st_iir'
	struct st_iir_2nd_order_tag
	{
	bool verb;															//verbose for debugging, v1.06
	string suser0;														//user string for debugging	v1.08
	string suser1;
	int user_id0;														//user num storage
	int user_id1;
	bool bypass;
	float coeff[5];														//a1, a2, b0, b1, b2
	float delay0[2];													//for ch0
	float delay1[2];													//for ch1
	};







	//function prototypes
	bool calc_filter_iir_2nd_order( en_filter_pass_type_tag filt_type, double fc1, double in_Q, double db_gain, double srate, vector<double> &vcoeff );
	bool filter_iir_2nd_order( vector <st_cplex_tag> &viq, double a1, double a2, double b0,  double b1, double b2, double &d0r, double &d1r, double &d0i, double &d1i );
	bool filter_fir_windowed( en_filter_window_type_tag   wnd_type, en_filter_pass_type_tag   filt_type, unsigned int taps, double fc1, double fc2, double srate, vector<double> &vcoeff );
	bool read_iowa_hills_coeffs( string fname, int section, double &a1, double &a2, double &b0, double &b1, double &b2 );


	void fir_init( st_fir &fir );
	void fir_in( st_fir &fir, double in );								//see alternate method 'fir_process()'
	double fir_out( st_fir &fir );										//see alternate method 'fir_process()'
	double fir_process( st_fir &fir, double in );						//v1.18
	bool fir_process_vector_double( st_fir &fir, vector<double> &vv );	//v1.18

	//double fir_out_inline_4( st_fir &fir );
	//double fir_out_inline_8( st_fir &fir );
	bool create_filter_from_coeffs( st_fir &fir, vector<double> &vcoeff );
	bool create_iir_filter_from_coeffs_b0_first( st_iir &iir, vector<double> &vcoeff );
	void delete_filter( st_fir &fir );

	bool create_filter_from_string( st_fir &fir, string scoeff );
	bool create_filter_from_file( st_fir &fir, string fname );

	//void filter_iir_2nd_order_slow( double &dsignal, double a1, double a2, double b0,  double b1, double b2, double &d0, double &d1 );
	void filter_iir_2nd_order( float &fsignal, st_iir_2nd_order_tag &of );	//v1.02			//NOTE this is SIMILAR to 'iir_process()'
	void filter_iir_2nd_order_2ch( float &fsig0, float &fsig1, st_iir_2nd_order_tag &of );	//v1.02

	bool create_iir_filter_from_coeffs( st_iir &iir, vector<double> &vcoeff ); //v1.03
	void iir_init( st_iir &iir ); //v1.03
	void iir_delete_filter( st_iir &iir ); //v1.03
	double iir_process( st_iir &iir, double in ); //v1.03	//NOTE this is SIMILAR to 'filter_iir_2nd_order()'
	float iir_process_float( st_iir &iir, float in ); 					//v1.16
	void create_filter_iir_using_q( filter_code::en_filter_pass_type_tag filt_type, float filt_freq_in, float filt_q_in, int srate_in, filter_code::st_iir_2nd_order_tag &iir );

	int factorial( int n ); 											//v1.08
	float kaiser_Io( float x );											//v1.08

	bool window_calc_and_apply_float( en_filter_window_type_tag wnd_type, vector<float> &vsmpl, vector<float> &vwnd );	//v1.08
	bool window_function_float( en_filter_window_type_tag wnd_type, unsigned len, vector<float> &vwnd );				//v1.08
	float window_calc_normalisation_factor_float( vector<float> &vwnd );	//v1.17
	double window_calc_normalisation_factor_double( vector<double> &vwnd );	//v1.17

	bool load_coeffs_from_file_float( string fname, vector<float> &vcoeff );			//v1.10
	bool load_coeffs_from_file_double( string fname, vector<double> &vcoeff );			//v1.10

	void polyphase_filter_downsampler_complex( filter_code::st_cplex_float_tag *cin, unsigned int insize, vector<filter_code::st_cplex_float_tag> &vcout, vector<float> &vpolycoeff, unsigned int phase_cnt, unsigned int phase_tap_cnt );
	void polyphase_filter_downsampler_for_fft( filter_code::st_cplex_float_tag *cin, unsigned int insize, vector<filter_code::st_cplex_float_tag> &vcout, vector<float> &vpolycoeff, unsigned int phase_cnt, unsigned int phase_tap_cnt );
	void polyphase_filter_upsampler_complex( filter_code::st_cplex_float_tag *cin, unsigned int insize, vector<filter_code::st_cplex_float_tag> &vcout, vector<float> &vpolycoeff, unsigned int phase_cnt, unsigned int phase_tap_cnt, bool correct_gain );
	bool polyphase_filter_upsampler_complex_vector( vector<filter_code::st_cplex_float_tag> &vcin, vector<filter_code::st_cplex_float_tag> &vcout, vector<float> &vpolycoeff, unsigned int phase_cnt, unsigned int phase_tap_cnt, bool correct_gain );
	bool load_coeffs_from_string_float( string scoeff, vector<double> &vcoeff );
	bool load_coeffs_from_string_double( string scoeff, vector<double> &vcoeff );
	bool save_coeffs_vector_float( string fname, vector<float> &vcoeff );
	bool save_coeffs_vector_double( string fname, vector<double> &vcoeff );
	void make_string_from_coeffs_float( vector<float>vcef, string &sout );
	void make_string_from_coeffs_double( vector<double>vcef, string &sout );
	void window_kaiser_float( int ntaps, double kaiser_beta, vector<float> &vkaiser );
	void window_kaiser_double( int ntaps, double kaiser_beta, vector<double> &vkaiser );
	void make_halfband_coeffs_float( unsigned int odd_tap_cnt, vector<float> &vcef );
	void make_halfband_coeffs_double( unsigned int odd_tap_cnt, vector<double> &vcef );



	}	//namespace 'filter_code'

//end of namespace
//----------------------------------------------------------------------



#define cn_ellip_builder_proto_max 16
#define cn_ellip_builder_proto_zero_pole_max 64
#define cn_ellip_builder_sos_sections_max ( cn_ellip_builder_proto_zero_pole_max / 2 )

#define cns_ellip_builder_lpf_zpk_proto_fname "ellip_lpf_zpk_proto.txt"	//holds a number of ellip filter precalc'd lpf gain/zero/pole analogue prototypes, refer to python code 'test_ellip_zpk_sos_v2.py'



//this struct can be loaded from an ini file with 'load_prototype_from_file()'
struct st_ellip_builder_protoype										//v1.19
{

int id0;																//user id
string sname0;															//user name

int ini_idx;															//e.g: '0' for ini entry: [ellip_lpf_proto0]
bool valid;						//possibly this struct is intact, not a thorough confirmation if complex zero/poles are not correctly formatted in ini file
unsigned int order;														//e.g: 8
float ripple;															//e.g: -1.0 for 1.0dB
float stopband;															//e.g: -60.0 for -60.0dB

double gain;
vector< complex<double> > vzeros;										//ellip analogue prototype iir zeros
vector< complex<double> > vpoles;										//ellip analogue prototype iir poles

unsigned int type;
unsigned int fc0;
unsigned int fc1;
unsigned int srate;

vector<double>vnumer;		//b0, b1, b2								//digital iir coeffs for use by second order section (SOS)
vector<double>vdenom;		//1.0, a1, a2

};



//---------- elliptical iir coeff gen using analogue zero/pole/gain (zpk) prototypes stored in a table ------------
//refer 'cl_ellip_builder::cl_ellip_builder()' for more details

//use 64 bit double precision numbers - zeros and poles are complex (real,imag)

//below is a built-in prototype table in an ini file format, expand to this by adding further '[ellip_lpf_protoX]' sections
#define cns_ellip_lpf_zpk_proto_builtin_ini R"(
[ellip_lpf_proto0]
order=4
ripple=1.000000
stopband=-40.000000
gain=0.010000
z0=0,3.5252874329960022
z1=0,1.6095504012251538
z2=0,-3.5252874329960022
z3=0,-1.6095504012251538
p0=-0.3642905958734215,-0.47860276764064974
p1=-0.10528126462117136,-0.99371081120877214
p2=-0.3642905958734215,0.47860276764064974
p3=-0.10528126462117136,0.99371081120877214


[ellip_lpf_proto1]
order=6
ripple=2.000000
stopband=-60.000000
gain=0.001000
z0=0,4.0638305449123795
z1=0,1.6427535098741952
z2=0,1.3150329533760208
z3=0,-4.0638305449123795
z4=0,-1.6427535098741952
z5=0,-1.3150329533760208
p0=-0.20857730158554458,-0.32878306298546689
p1=-0.11010791842113045,-0.80052761280457263
p2=-0.029510654853827079,-0.98958244622451663
p3=-0.20857730158554458,0.32878306298546689
p4=-0.11010791842113045,0.80052761280457263
p5=-0.029510654853827079,0.98958244622451663



[ellip_lpf_proto2]
order=8
ripple=1.0
stopband=-60.0
gain=0.001
z0=0,3.9165773784910107
z1=0,1.5517895699804845
z2=0,1.1992599115796481
z3=0,1.1115158386282038
z4=0,-3.9165773784910107
z5=0,-1.5517895699804845
z6=0,-1.1992599115796481
z7=0,-1.1115158386282038
p0=-0.24757755767570402,-0.2986822344640841
p1=-0.14002275951473211,-0.73542529665989842
p2=-0.055308138156739628,-0.93336352487701901
p3=-0.013600821923738692,-0.99923757129566093
p4=-0.24757755767570402,0.2986822344640841
p5=-0.14002275951473211,0.73542529665989842
p6=-0.055308138156739628,0.93336352487701901
p7=-0.013600821923738692,0.99923757129566093


[ellip_lpf_proto3]
order=8
ripple=0.250000
stopband=-60.000000
gain=0.001000
z0=0,4.3092843646215622
z1=0,1.6713385410534747
z2=0,1.2634018921706593
z3=0,1.1573597169399692
z4=0,-4.3092843646215622
z5=0,-1.6713385410534747
z6=0,-1.2634018921706593
z7=0,-1.1573597169399692
p0=-0.35564754235110041,-0.29800435825858229
p1=-0.21173141642074997,-0.73590220707540688
p2=-0.089767226535868092,-0.93851788962738292
p3=-0.023250541197890068,-1.0083001069956004
p4=-0.35564754235110041,0.29800435825858229
p5=-0.21173141642074997,0.73590220707540688
p6=-0.089767226535868092,0.93851788962738292
p7=-0.023250541197890068,1.0083001069956004


[ellip_lpf_proto4]
order=8
ripple=1.000000
stopband=-80.000000
gain=0.000100
z0=0,5.2573279342487691
z1=0,1.9724984347229491
z2=0,1.4363504548365289
z3=0,1.2880184721856083
z4=0,-5.2573279342487691
z5=0,-1.9724984347229491
z6=0,-1.4363504548365289
z7=0,-1.2880184721856083
p0=-0.2155936471385935,-0.25171264297410062
p1=-0.14736155323732883,-0.6633743269413731
p2=-0.073572524061334382,-0.90058659561766674
p3=-0.021153569664180298,-0.99838693048412297
p4=-0.2155936471385935,0.25171264297410062
p5=-0.14736155323732883,0.6633743269413731
p6=-0.073572524061334382,0.90058659561766674
p7=-0.021153569664180298,0.99838693048412297


[ellip_lpf_proto5]
order=10
ripple=2.000000
stopband=-40.000000
gain=0.010000
z0=0,2.6709831551080816
z1=0,1.2091873560644053
z2=0,1.0411782981853974
z3=0,1.0096480033138449
z4=0,1.0035143157020758
z5=0,-2.6709831551080816
z6=0,-1.2091873560644053
z7=0,-1.0411782981853974
z8=0,-1.0096480033138449
z9=0,-1.0035143157020758
p0=-0.23726296798844845,-0.40031632325313732
p1=-0.083200735567070694,-0.84870323310240814
p2=-0.019448948779927807,-0.96856794821369874
p3=-0.0040907517874174048,-0.99458604073631607
p4=-0.00068016822689536912,-0.99979491057601344
p5=-0.23726296798844845,0.40031632325313732
p6=-0.083200735567070694,0.84870323310240814
p7=-0.019448948779927807,0.96856794821369874
p8=-0.0040907517874174048,0.99458604073631607
p9=-0.00068016822689536912,0.99979491057601344



[ellip_lpf_proto6]
order=10
ripple=1.000000
stopband=-40.000000
gain=0.010000
z0=0,2.8311980685292677
z1=0,1.2465830177933253
z2=0,1.0532524724540215
z3=0,1.0139107427903418
z4=0,1.0055964003005116
z5=0,-2.8311980685292677
z6=0,-1.2465830177933253
z7=0,-1.0532524724540215
z8=0,-1.0139107427903418
z9=0,-1.0055964003005116
p0=-0.30200177697452013,-0.39233274735397472
p1=-0.11478808720107861,-0.83840360905154598
p2=-0.029679958185096569,-0.96397806737304381
p3=-0.0068791545051012553,-0.9935425697814031
p4=-0.0012314886069465739,-0.99998405112865918
p5=-0.30200177697452013,0.39233274735397472
p6=-0.11478808720107861,0.83840360905154598
p7=-0.029679958185096569,0.96397806737304381
p8=-0.0068791545051012553,0.9935425697814031
p9=-0.0012314886069465739,0.99998405112865918


[ellip_lpf_proto7]
order=10
ripple=1.000000
stopband=-60.000000
gain=0.001000
z0=0,3.7881079914248046
z1=0,1.4996484754424135
z2=0,1.155770606428778
z3=0,1.0622653532267758
z4=0,1.0357995118716679
z5=0,-3.7881079914248046
z6=0,-1.4996484754424135
z7=0,-1.155770606428778
z8=0,-1.0622653532267758
z9=0,-1.0357995118716679
p0=-0.23977748700713741,-0.2891847644117011
p1=-0.13614392032148293,-0.71255716840024386
p2=-0.055537556247658854,-0.9065347971354184
p3=-0.019205788496320621,-0.97770110594268034
p4=-0.0045255362338639978,-0.99976624127227942
p5=-0.23977748700713741,0.2891847644117011
p6=-0.13614392032148293,0.71255716840024386
p7=-0.055537556247658854,0.9065347971354184
p8=-0.019205788496320621,0.97770110594268034
p9=-0.0045255362338639978,0.99976624127227942




[ellip_lpf_proto8]
order=12
ripple=0.500000
stopband=-80.000000
gain=0.000100
z0=0,4.9124009968484534
z1=0,1.8251861542038013
z2=0,1.3061636492533792
z3=0,1.1381920655245217
z4=0,1.0751349922467042
z5=0,1.0532008567088682
z6=0,-4.9124009968484534
z7=0,-1.8251861542038013
z8=0,-1.3061636492533792
z9=0,-1.1381920655245217
z10=0,-1.0751349922467042
z11=0,-1.0532008567088682
p0=-0.23848404152946456,-0.22606513555431615
p1=-0.16785076307610325,-0.59878508666340646
p2=-0.092147074309701316,-0.82229461067047216
p3=-0.043763536196738208,-0.93319461940592863
p4=-0.018418743353327899,-0.98246718491962248
p5=-0.0049854207655082635,-1.0007676640346947
p6=-0.23848404152946456,0.22606513555431615
p7=-0.16785076307610325,0.59878508666340646
p8=-0.092147074309701316,0.82229461067047216
p9=-0.043763536196738208,0.93319461940592863
p10=-0.018418743353327899,0.98246718491962248
p11=-0.0049854207655082635,1.0007676640346947


[ellip_lpf_proto9]
order=12
ripple=0.100000
stopband=-80.000000
gain=0.000100
z0=0,5.3050421846467515
z1=0,1.9462371655836672
z2=0,1.3694692847183447
z3=0,1.176802068193753
z4=0,1.1018039234615995
z5=0,1.0748987845467366
z6=0,-5.3050421846467515
z7=0,-1.9462371655836672
z8=0,-1.3694692847183447
z9=0,-1.176802068193753
z10=0,-1.1018039234615995
z11=0,-1.0748987845467366
p0=-0.33364475734944393,-0.2240235112006301
p1=-0.24171701836600834,-0.59423453551612659
p2=-0.1391298543315897,-0.81858713632392066
p3=-0.069677727666604072,-0.93252762253946542
p4=-0.030746844176367763,-0.98483210558803935
p5=-0.0085683989276071695,-1.0048533729555937
p6=-0.33364475734944393,0.2240235112006301
p7=-0.24171701836600834,0.59423453551612659
p8=-0.1391298543315897,0.81858713632392066
p9=-0.069677727666604072,0.93252762253946542
p10=-0.030746844176367763,0.98483210558803935
p11=-0.0085683989276071695,1.0048533729555937

)"





class cl_ellip_builder
{
private:

public:
bool verbose;
st_ellip_builder_protoype st0;


//----
struct biquad_float
{
float b0, b1, b2;
float a1, a2;
float z1, z2;
};

biquad_float sec_float[ cn_ellip_builder_proto_zero_pole_max ];
//----


//----
struct biquad_double
{
double b0, b1, b2;
double a1, a2;
double z1, z2;
};

biquad_double sec_double[ cn_ellip_builder_proto_zero_pole_max ];
//----



int sections;															//number of cascaded sos biquads

public:
cl_ellip_builder();
bool load_prototype_from_file( string sfname, unsigned int order_in, float ripple_in, float stopband_in );
bool load_prototype_from_string( string &ss, unsigned int order_in, float ripple_in, float stopband_in );
bool calc_iir_coeffs( unsigned int type, unsigned int fc0, unsigned int fc1, unsigned int srate, vector<double> &vnumer_out, vector<double> &vdenom_out );
void analog_to_digital( unsigned int type, vector< complex<double> > &vz, vector< complex<double> > &vp, double k, double fc0, double fc1, double fs, vector< complex<double> > &vdz, vector< complex<double> > &vdp, double &kd );
void bilinear_zpk(vector< complex<double> > &vz, vector< complex<double> > &vp, double k, double fs, vector< complex<double> > &vdz, vector< complex<double> > &vdp, double &k_d );

void cl_ellip_builder::zpk2sos(
	unsigned int type,
	vector< complex<double> > &vdz,
	vector< complex<double> > &vdp,
	double kd,
	vector<double> &vnumer,
	vector<double> &vdenom);


void lp_to_bp_zpk(
	vector< complex<double> > &vz,
	vector< complex<double> > &vp,
	double k,
	double wc0,
	double wc1,
	double &kbp,
	vector< complex<double> > &bz,
	vector< complex<double> > &bp);


void lp_to_bs_zpk(
	vector< complex<double> > &vz,
	vector< complex<double> > &vp,
	double k,
	double wlow,
	double whigh,
	double &kbs,
	vector< complex<double> > &bz,
	vector< complex<double> > &bp);
	
double cl_ellip_builder::sos_gain_dc(
	vector<double> &vnumer,
	vector<double> &vdenom);


bool sos_init_float( vector<double> &vnumer, vector<double> &vdenom );
float sos_cascade_process_float( float x);

bool sos_init_double( vector<double> &vnumer, vector<double> &vdenom );
double sos_cascade_process_double( double x);


private:
float sos_biquad_process_float(biquad_float& q, float x);
double sos_biquad_process_double(biquad_double& q, double x);

};
//------------------------------------------------------------------------------------------------------------




#endif

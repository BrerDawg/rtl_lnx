/*
Copyright (C) 2010-2026 BrerDawg, et. al.

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

//rtlobj.h
//v1.04

#ifndef rtlobj_h
#define rtlobj_h


#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <locale.h>
#include <string>
#include <vector>
#include <wchar.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <vector>


//#include <libusb.h>
#include <libusb-1.0/libusb.h>


#include <fftw3.h>

//#include </usr/include/rtl-sdr.h>										//v1.04
#include <rtl-sdr.h>													//v1.04
#include "convenience.h"

#include "globals.h"
#include "filter_code.h"			//just for 'st_cplex_tag'



using namespace std;


#define cn_rtl_e4000_sample_bandwith_min 225027 		//this number was from internet, e4000 chip's lowest srate
#define cn_rtl_e4000_sample_bandwith_max 3200000
#define cn_rtl_sample_bandwith_max 3168000


extern bool rtl_scan( double freq_start, double freq_stop, double freq_step, double gain_in );






class gcrtl
{
private:
rtlsdr_dev_t *rtl_dev;
uint8_t *buf8;
unsigned int srate;
unsigned int rtl_buf_size;

unsigned int fftw_size;

fftw_complex *fftw_in_c0;						//complex pointers
fftw_complex *fftw_out_c0;

fftw_plan fftw_p_c0;

double cur_freq;

pthread_mutex_t read_mutex;

bool b_is_open;



public:
char szname[256];
char szmanufact[256];
char szproduct[256];
char szserial[256];


private:


public:
gcrtl();
~gcrtl();
bool set_srate( unsigned int srate_in );
unsigned int get_srate();
bool set_buf_size( unsigned int fft_size_in );
unsigned int get_buf_size();

int get_device_count();
const char* get_device_name_using_index( uint32_t index );
bool get_get_device_usb_strings_using_index( uint32_t index, string &s_manufact, string &s_product, string &s_serial );


const char* get_device_name();
bool get_get_device_usb_strings( string &s_manufact, string &s_product, string &s_serial );


bool open( int dev_index );
bool close();
bool open_status();
int nearest_gain( int target_gain );
bool set_agc_mode( bool b_on );
bool set_gain_mode( bool b_manual );
bool set_gain( double gain_in );
float get_gain();
bool set_center_freq( double freq_in );
int get_center_freq();
bool set_tuner_if_gain( int stage, double gain_in );
bool set_ppm( int ppm_error );
int get_ppm();
bool read( vector <filter_code::st_cplex_tag> &vbuf );
bool read_fft( vector <filter_code::st_cplex_tag> &vfft );
bool read_fft_graph( vector <st_spect_tag> &vsp, bool b_suppress_zero_ifreq );
bool read_async( rtlsdr_read_async_cb_t cb, void *arg, unsigned int count );
bool reset_buffer();
bool cancel_async();
int set_direct_sampling( int Off_Ibranch_Qbranch );
int get_direct_sampling();
bool set_offset_tuning( bool b_on );
bool get_offset_tuning();
bool set_bandwidth( unsigned int bwidth );
bool set_bias_tee( bool on );

};




#endif


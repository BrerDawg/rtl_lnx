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


/*
 * rtl-sdr, turns your Realtek RTL2832 based DVB dongle into a SDR receiver
 * Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>
 * Copyright (C) 2012 by Hoernchen <la@tfc-server.de>
 * Copyright (C) 2012 by Kyle Keen <keenerd@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */



//this cpp obj is derived from 'rtl_power.c' 




//rtlobj.cpp
//v1.01		15-Feb-2014			//
//v1.02		26-aug-2023			//added further functions, e.g: 'get_device_count()', 'get_device_name( uint32_t index )', get_get_device_usb_strings()'
//v1.03		05-sep-2023			//added 'b_is_open'  'get_device_name_using_index()'   'get_get_device_usb_strings_using_index()'
//v1.04		03-jun-2025			//changed: '#include </usr/include/rtl-sdr.h>'  to   '#include <rtl-sdr.h>'





#include "rtlobj.h"



bool use_file = 0;






//#define usleep(x) Sleep(x/1000)
#define round(x) (x > 0.0 ? floor(x + 0.5): ceil(x - 0.5))


#define MAX(x, y) (((x) > (y)) ? (x) : (y))

#define DEFAULT_BUF_LENGTH		(1 * 16384)
#define AUTO_GAIN			-100
#define BUFFER_DUMP			(1<<12)

#define MAXIMUM_RATE			2800000
#define MINIMUM_RATE			1000000


static volatile int do_exit;
static rtlsdr_dev_t *dev;
FILE *file;

int16_t* Sinewave;
double* power_table;
int N_WAVE, LOG2_N_WAVE;
int next_power;
int16_t *fft_buf;
int *window_coefs;

struct tuning_state
/* one per tuning range */
{
	int freq;
	int rate;
	int bin_e;
	long *avg;  /* length == 2^bin_e */
	int samples;
	int downsample;
	int downsample_passes;  /* for the recursive filter */
	double crop;
	//pthread_rwlock_t avg_lock;
	//pthread_mutex_t avg_mutex;
	/* having the iq buffer here is wasteful, but will avoid contention */
	uint8_t *buf8;
	int buf_len;
	//int *comp_fir;
	//pthread_rwlock_t buf_lock;
	//pthread_mutex_t buf_mutex;
};

/* 3000 is enough for 3GHz b/w worst case */
#define MAX_TUNES	3000
struct tuning_state tunes[MAX_TUNES];
int tune_count;

int boxcar;
int comp_fir_size;
int peak_hold;






#include "rtlobj.h"







void multi_bail(void)
{
	if (do_exit == 1)
	{
		fprintf(stderr, "Signal caught, finishing scan pass.\n");
	}
	if (do_exit >= 2)
	{
		fprintf(stderr, "Signal caught, aborting immediately.\n");
	}
}

#ifdef _WIN32
BOOL WINAPI
sighandler(int signum)
{
	if (CTRL_C_EVENT == signum) {
		do_exit++;
		multi_bail();
		return TRUE;
	}
	return FALSE;
}
#else
static void sighandler(int signum)
{
	do_exit++;
	multi_bail();
}
#endif

/* more cond dumbness */
#define safe_cond_signal(n, m) pthread_mutex_lock(m); pthread_cond_signal(n); pthread_mutex_unlock(m)
#define safe_cond_wait(n, m) pthread_mutex_lock(m); pthread_cond_wait(n, m); pthread_mutex_unlock(m)

/* {length, coef, coef, coef}  and scaled by 2^15
   for now, only length 9, optimal way to get +85% bandwidth */
#define CIC_TABLE_MAX 10
int cic_9_tables[][10] = {
	{0,},
	{9, -156,  -97, 2798, -15489, 61019, -15489, 2798,  -97, -156},
	{9, -128, -568, 5593, -24125, 74126, -24125, 5593, -568, -128},
	{9, -129, -639, 6187, -26281, 77511, -26281, 6187, -639, -129},
	{9, -122, -612, 6082, -26353, 77818, -26353, 6082, -612, -122},
	{9, -120, -602, 6015, -26269, 77757, -26269, 6015, -602, -120},
	{9, -120, -582, 5951, -26128, 77542, -26128, 5951, -582, -120},
	{9, -119, -580, 5931, -26094, 77505, -26094, 5931, -580, -119},
	{9, -119, -578, 5921, -26077, 77484, -26077, 5921, -578, -119},
	{9, -119, -577, 5917, -26067, 77473, -26067, 5917, -577, -119},
	{9, -199, -362, 5303, -25505, 77489, -25505, 5303, -362, -199},
};

#ifdef _MSC_VER
double log2(double n)
{
	return log(n) / log(2.0);
}
#endif

/* FFT based on fix_fft.c by Roberts, Slaney and Bouras
   http://www.jjj.de/fft/fftpage.html
   16 bit ints for everything
   -32768..+32768 maps to -1.0..+1.0
*/

void sine_table(int size)
{
	int i;
	double d;
	LOG2_N_WAVE = size;
	N_WAVE = 1 << LOG2_N_WAVE;
	Sinewave = (int16_t*)malloc(sizeof(int16_t) * N_WAVE*3/4);
	power_table = (double*)malloc(sizeof(double) * N_WAVE);
	for (i=0; i<N_WAVE*3/4; i++)
	{
		d = (double)i * 2.0 * M_PI / N_WAVE;
		Sinewave[i] = (int)round(32767*sin(d));
		//printf("%i\n", Sinewave[i]);
	}
}

inline int16_t FIX_MPY(int16_t a, int16_t b)
/* fixed point multiply and scale */
{
	int c = ((int)a * (int)b) >> 14;
	b = c & 0x01;
	return (c >> 1) + b;
}

int fix_fft(int16_t iq[], int m)
/* interleaved iq[], 0 <= n < 2**m, changes in place */
{
	int mr, nn, i, j, l, k, istep, n, shift;
	int16_t qr, qi, tr, ti, wr, wi;
	n = 1 << m;
	if (n > N_WAVE)
		{return -1;}
	mr = 0;
	nn = n - 1;
	/* decimation in time - re-order data */
	for (m=1; m<=nn; ++m) {
		l = n;
		do
			{l >>= 1;}
		while (mr+l > nn);
		mr = (mr & (l-1)) + l;
		if (mr <= m)
			{continue;}
		// real = 2*m, imag = 2*m+1
		tr = iq[2*m];
		iq[2*m] = iq[2*mr];
		iq[2*mr] = tr;
		ti = iq[2*m+1];
		iq[2*m+1] = iq[2*mr+1];
		iq[2*mr+1] = ti;
	}
	l = 1;
	k = LOG2_N_WAVE-1;
	while (l < n) {
		shift = 1;
		istep = l << 1;
		for (m=0; m<l; ++m) {
			j = m << k;
			wr =  Sinewave[j+N_WAVE/4];
			wi = -Sinewave[j];
			if (shift) {
				wr >>= 1; wi >>= 1;}
			for (i=m; i<n; i+=istep) {
				j = i + l;
				tr = FIX_MPY(wr,iq[2*j]) - FIX_MPY(wi,iq[2*j+1]);
				ti = FIX_MPY(wr,iq[2*j+1]) + FIX_MPY(wi,iq[2*j]);
				qr = iq[2*i];
				qi = iq[2*i+1];
				if (shift) {
					qr >>= 1; qi >>= 1;}
				iq[2*j] = qr - tr;
				iq[2*j+1] = qi - ti;
				iq[2*i] = qr + tr;
				iq[2*i+1] = qi + ti;
			}
		}
		--k;
		l = istep;
	}
	return 0;
}

double rectangle(int i, int length)
{
	return 1.0;
}

double hamming(int i, int length)
{
	double a, b, w, N1;
	a = 25.0/46.0;
	b = 21.0/46.0;
	N1 = (double)(length-1);
	w = a - b*cos(2*i*M_PI/N1);
	return w;
}

double blackman(int i, int length)
{
	double a0, a1, a2, w, N1;
	a0 = 7938.0/18608.0;
	a1 = 9240.0/18608.0;
	a2 = 1430.0/18608.0;
	N1 = (double)(length-1);
	w = a0 - a1*cos(2*i*M_PI/N1) + a2*cos(4*i*M_PI/N1);
	return w;
}

double blackman_harris(int i, int length)
{
	double a0, a1, a2, a3, w, N1;
	a0 = 0.35875;
	a1 = 0.48829;
	a2 = 0.14128;
	a3 = 0.01168;
	N1 = (double)(length-1);
	w = a0 - a1*cos(2*i*M_PI/N1) + a2*cos(4*i*M_PI/N1) - a3*cos(6*i*M_PI/N1);
	return w;
}

double hann_poisson(int i, int length)
{
	double a, N1, w;
	a = 2.0;
	N1 = (double)(length-1);
	w = 0.5 * (1 - cos(2*M_PI*i/N1)) * \
	    pow(M_E, (-a*(double)abs((int)(N1-1-2*i)))/N1);
	return w;
}

double youssef(int i, int length)
/* really a blackman-harris-poisson window, but that is a mouthful */
{
	double a, a0, a1, a2, a3, w, N1;
	a0 = 0.35875;
	a1 = 0.48829;
	a2 = 0.14128;
	a3 = 0.01168;
	N1 = (double)(length-1);
	w = a0 - a1*cos(2*i*M_PI/N1) + a2*cos(4*i*M_PI/N1) - a3*cos(6*i*M_PI/N1);
	a = 0.0025;
	w *= pow(M_E, (-a*(double)abs((int)(N1-1-2*i)))/N1);
	return w;
}

double kaiser(int i, int length)
// todo, become more smart
{
	return 1.0;
}

double bartlett(int i, int length)
{
	double N1, L, w;
	L = (double)length;
	N1 = L - 1;
	w = (i - N1/2) / (L/2);
	if (w < 0) {
		w = -w;}
	w = 1 - w;
	return w;
}

void rms_power(struct tuning_state *ts)
/* for bins between 1MHz and 2MHz */
{
	int i, s;
	uint8_t *buf = ts->buf8;
	int buf_len = ts->buf_len;
	long p, t;
	double dc, err;

	p = t = 0L;
	for (i=0; i<buf_len; i++) {
		s = (int)buf[i] - 127;
		t += (long)s;
		p += (long)(s * s);
	}
	/* correct for dc offset in squares */
	dc = (double)t / (double)buf_len;
	err = t * 2 * dc - dc * dc * buf_len;
	p -= (long)round(err);

	if (!peak_hold) {
		ts->avg[0] += p;
	} else {
		ts->avg[0] = MAX(ts->avg[0], p);
	}
	ts->samples += 1;
}

bool frequency_range( double fstart_in, double fstop_in, double fstep_in, double crop)
/* flesh out the tunes[] for scanning */
// do we want the fewest ranges (easy) or the fewest bins (harder)?
{
	char *start, *stop, *step;
	int i, j, upper, lower, max_size, bw_seen, bw_used, bin_e, buf_len;
	int downsample, downsample_passes;
	double bin_size;
	struct tuning_state *ts;

/*
	// hacky string parsing
	start = arg;
	stop = strchr(start, ':') + 1;
printf("here0.0\n" );
	stop[-1] = '\0';
	step = strchr(stop, ':') + 1;
	step[-1] = '\0';
	lower = (int)atofs(start);
printf("here0.1\n" );
	upper = (int)atofs(stop);
	max_size = (int)atofs(step);
	stop[-1] = ':';
	step[-1] = ':';
*/

lower = fstart_in;
upper = fstop_in;
max_size = fstep_in;

	downsample = 1;

	downsample_passes = 0;
	/* evenly sized ranges, as close to MAXIMUM_RATE as possible */
	// todo, replace loop with algebra
	for (i=1; i<1500; i++) {
		bw_seen = (upper - lower) / i;
		bw_used = (int)((double)(bw_seen) / (1.0 - crop));
		if (bw_used > MAXIMUM_RATE) {
			continue;}
		tune_count = i;
		break;
	}
	/* unless small bandwidth */
	if (bw_used < MINIMUM_RATE) {
		tune_count = 1;
		downsample = MAXIMUM_RATE / bw_used;
		bw_used = bw_used * downsample;
	}

	if (!boxcar && downsample > 1) {
		downsample_passes = (int)log2(downsample);
		downsample = 1 << downsample_passes;
		bw_used = (int)((double)(bw_seen * downsample) / (1.0 - crop));
	}
	/* number of bins is power-of-two, bin size is under limit */
	// todo, replace loop with log2
	for (i=1; i<=21; i++) {
		bin_e = i;
		bin_size = (double)bw_used / (double)((1<<i) * downsample);
		if (bin_size <= (double)max_size) {
			break;}
	}
	/* unless giant bins */
	if (max_size >= MINIMUM_RATE) {
		bw_seen = max_size;
		bw_used = max_size;
		tune_count = (upper - lower) / bw_seen;
		bin_e = 0;
		crop = 0;
	}
	if (tune_count > MAX_TUNES) {
		fprintf(stderr, "Error: bandwidth too wide.\n");
		return 0;
//		exit(1);
	}

	buf_len = 2 * (1<<bin_e) * downsample;
	if (buf_len < DEFAULT_BUF_LENGTH) {
		buf_len = DEFAULT_BUF_LENGTH;
	}
	/* build the array */
	for (i=0; i<tune_count; i++) {
		ts = &tunes[i];
		ts->freq = lower + i*bw_seen + bw_seen/2;
		ts->rate = bw_used;
		ts->bin_e = bin_e;
		ts->samples = 0;
		ts->crop = crop;
		ts->downsample = downsample;
		ts->downsample_passes = downsample_passes;
		ts->avg = (long*)malloc((1<<bin_e) * sizeof(long));
		if (!ts->avg) {
			fprintf(stderr, "Error: malloc.\n");
			exit(1);
		}
		for (j=0; j<(1<<bin_e); j++) {
			ts->avg[j] = 0L;
		}
		ts->buf8 = (uint8_t*)malloc(buf_len * sizeof(uint8_t));
		if (!ts->buf8) {
			fprintf(stderr, "Error: malloc.\n");
			exit(1);
		}
		ts->buf_len = buf_len;
	}
	/* report */
	fprintf(stderr, "Number of frequency hops: %i\n", tune_count);
	fprintf(stderr, "Dongle bandwidth: %iHz\n", bw_used);
	fprintf(stderr, "Downsampling by: %ix\n", downsample);
	fprintf(stderr, "Cropping by: %0.2f%%\n", crop*100);
	fprintf(stderr, "Total FFT bins: %i\n", tune_count * (1<<bin_e));
	fprintf(stderr, "Logged FFT bins: %i\n", \
	  (int)((double)(tune_count * (1<<bin_e)) * (1.0-crop)));
	fprintf(stderr, "FFT bin size: %0.2fHz\n", bin_size);
	fprintf(stderr, "Buffer size: %i bytes (%0.2fms)\n", buf_len, 1000 * 0.5 * (float)buf_len / (float)bw_used);
return 1;
}


void retune(rtlsdr_dev_t *d, int freq)
{
	uint8_t dump[BUFFER_DUMP];
	int n_read;
	rtlsdr_set_center_freq(d, (uint32_t)freq);
	/* wait for settling and flush buffer */
	usleep(5000);
	rtlsdr_read_sync(d, &dump, BUFFER_DUMP, &n_read);
	if (n_read != BUFFER_DUMP) {
		fprintf(stderr, "Error: bad retune.\n");}
}

void fifth_order(int16_t *data, int length)
/* for half of interleaved data */
{
	int i;
	int a, b, c, d, e, f;
	a = data[0];
	b = data[2];
	c = data[4];
	d = data[6];
	e = data[8];
	f = data[10];
	/* a downsample should improve resolution, so don't fully shift */
	/* ease in instead of being stateful */
	data[0] = ((a+b)*10 + (c+d)*5 + d + f) >> 4;
	data[2] = ((b+c)*10 + (a+d)*5 + e + f) >> 4;
	data[4] = (a + (b+e)*5 + (c+d)*10 + f) >> 4;
	for (i=12; i<length; i+=4) {
		a = c;
		b = d;
		c = e;
		d = f;
		e = data[i-2];
		f = data[i];
		data[i/2] = (a + (b+e)*5 + (c+d)*10 + f) >> 4;
	}
}

void remove_dc(int16_t *data, int length)
/* works on interleaved data */
{
	int i;
	int16_t ave;
	long sum = 0L;
	for (i=0; i < length; i+=2) {
		sum += data[i];
	}
	ave = (int16_t)(sum / (long)(length));
	if (ave == 0) {
		return;}
	for (i=0; i < length; i+=2) {
		data[i] -= ave;
	}
}

void generic_fir(int16_t *data, int length, int *fir)
/* Okay, not at all generic.  Assumes length 9, fix that eventually. */
{
	int d, temp, sum;
	int hist[9] = {0,};
	/* cheat on the beginning, let it go unfiltered */
	for (d=0; d<18; d+=2) {
		hist[d/2] = data[d];
	}
	for (d=18; d<length; d+=2) {
		temp = data[d];
		sum = 0;
		sum += (hist[0] + hist[8]) * fir[1];
		sum += (hist[1] + hist[7]) * fir[2];
		sum += (hist[2] + hist[6]) * fir[3];
		sum += (hist[3] + hist[5]) * fir[4];
		sum +=            hist[4]  * fir[5];
		data[d] = (int16_t)(sum >> 15) ;
		hist[0] = hist[1];
		hist[1] = hist[2];
		hist[2] = hist[3];
		hist[3] = hist[4];
		hist[4] = hist[5];
		hist[5] = hist[6];
		hist[6] = hist[7];
		hist[7] = hist[8];
		hist[8] = temp;
	}
}

void downsample_iq(int16_t *data, int length)
{
	fifth_order(data, length);
	//remove_dc(data, length);
	fifth_order(data+1, length-1);
	//remove_dc(data+1, length-1);
}

long real_conj(int16_t real, int16_t imag)
/* real(n * conj(n)) */
{
	return ((long)real*(long)real + (long)imag*(long)imag);
}

void scanner( vector<st_spect_tag> &vspct )
{
	int i, j, j2, f, n_read, offset, bin_e, bin_len, buf_len, ds, ds_p;
	int32_t w;
	struct tuning_state *ts;
	bin_e = tunes[0].bin_e;
	bin_len = 1 << bin_e;
	buf_len = tunes[0].buf_len;
	for (i=0; i<tune_count; i++) {
		if (do_exit >= 2)
			{return;}
		ts = &tunes[i];
		f = (int)rtlsdr_get_center_freq(dev);
		if (f != ts->freq) {
			retune(dev, ts->freq);}
		rtlsdr_read_sync(dev, ts->buf8, buf_len, &n_read);
		if (n_read != buf_len) {
			fprintf(stderr, "Error: dropped samples.\n");}
		/* rms */
		if (bin_len == 1) {
			rms_power(ts);
			continue;
		}
		/* prep for fft */
		for (j=0; j<buf_len; j++) {
			fft_buf[j] = (int16_t)ts->buf8[j] - 127;
		}
		ds = ts->downsample;
		ds_p = ts->downsample_passes;
		if (boxcar && ds > 1) {
			j=2, j2=0;
			while (j < buf_len) {
				fft_buf[j2]   += fft_buf[j];
				fft_buf[j2+1] += fft_buf[j+1];
				fft_buf[j] = 0;
				fft_buf[j+1] = 0;
				j += 2;
				if (j % (ds*2) == 0) {
					j2 += 2;}
			}
		} else if (ds_p) {  /* recursive */
			for (j=0; j < ds_p; j++) {
				downsample_iq(fft_buf, buf_len >> j);
			}
			/* droop compensation */
			if (comp_fir_size == 9 && ds_p <= CIC_TABLE_MAX) {
				generic_fir(fft_buf, buf_len >> j, cic_9_tables[ds_p]);
				generic_fir(fft_buf+1, (buf_len >> j)-1, cic_9_tables[ds_p]);
			}
		}
		remove_dc(fft_buf, buf_len / ds);
		remove_dc(fft_buf+1, (buf_len / ds) - 1);
		/* window function and fft */
		for (offset=0; offset<(buf_len/ds); offset+=(2*bin_len)) {
			// todo, let rect skip this
			for (j=0; j<bin_len; j++) {
				w =  (int32_t)fft_buf[offset+j*2];
				w *= (int32_t)(window_coefs[j]);
				//w /= (int32_t)(ds);
				fft_buf[offset+j*2]   = (int16_t)w;
				w =  (int32_t)fft_buf[offset+j*2+1];
				w *= (int32_t)(window_coefs[j]);
				//w /= (int32_t)(ds);
				fft_buf[offset+j*2+1] = (int16_t)w;
			}
			fix_fft(fft_buf+offset, bin_e);
			if (!peak_hold) {
				for (j=0; j<bin_len; j++) {
					ts->avg[j] += real_conj(fft_buf[offset+j*2], fft_buf[offset+j*2+1]);
				}
			} else {
				for (j=0; j<bin_len; j++) {
					ts->avg[j] = MAX(real_conj(fft_buf[offset+j*2], fft_buf[offset+j*2+1]), ts->avg[j]);
				}
			}
			ts->samples += ds;
		}
	}
}

double last_tune = 0;

void csv_dbm(struct tuning_state *ts, vector<st_spect_tag> &vspct )
{
	int i, len, ds, i1, i2, bw2, bin_count;
	long tmp;
	double dbm;
	len = 1 << ts->bin_e;
	ds = ts->downsample;
	/* fix FFT stuff quirks */
	if (ts->bin_e > 0) {
		/* nuke DC component (not effective for all windows) */
		ts->avg[0] = ts->avg[1];
		/* FFT is translated by 180 degrees */
		for (i=0; i<len/2; i++) {
			tmp = ts->avg[i];
			ts->avg[i] = ts->avg[i+len/2];
			ts->avg[i+len/2] = tmp;
		}
	}
	/* Hz low, Hz high, Hz step, samples, dbm, dbm, ... */
	bin_count = (int)((double)len * (1.0 - ts->crop));
	bw2 = (int)(((double)ts->rate * (double)bin_count) / (len * 2 * ds));
	if( use_file )
		{
		fprintf(file, "%i, %i, %.2f, %i, ", ts->freq - bw2, ts->freq + bw2, (double)ts->rate / (double)(len*ds), ts->samples);
		}

last_tune = ts->freq - bw2;

	// something seems off with the dbm math
	i1 = 0 + (int)((double)len * ts->crop * 0.5);
	i2 = (len-1) - (int)((double)len * ts->crop * 0.5);
	for (i=i1; i<=i2; i++) {
		dbm  = (double)ts->avg[i];
		dbm /= (double)ts->rate;
		dbm /= (double)ts->samples;
		dbm  = 10 * log10(dbm);
	if( use_file )
		{
		fprintf(file, "%.2f, ", dbm);
		}

	st_spect_tag o;
	o.freq = last_tune;
	last_tune += (double)ts->rate / (double)( len * ds );
	o.ampl = dbm;
	vspct.push_back( o );
	}

	dbm = (double)ts->avg[i2] / ((double)ts->rate * (double)ts->samples);
	if (ts->bin_e == 0) {
		dbm = ((double)ts->avg[0] / \
		((double)ts->rate * (double)ts->samples));}
	dbm  = 10 * log10(dbm);
	if( use_file )
		{
		fprintf(file, "%.2f\n", dbm);
		}
	for (i=0; i<len; i++) {
		ts->avg[i] = 0L;
	}
	ts->samples = 0;
}









gcrtl::gcrtl()
{
b_is_open = 0;
rtl_dev = 0;
buf8 = 0;

fftw_in_c0 = 0;
fftw_out_c0 = 0;

fftw_p_c0 = 0;

rtl_buf_size = 0;
fftw_size = 0;
}







gcrtl::~gcrtl()
{
this->close();
}










bool gcrtl::open_status()
{
return b_is_open;
}








int gcrtl::get_device_count()
{
return rtlsdr_get_device_count();
}





const char *gcrtl::get_device_name_using_index( uint32_t index )
{
return rtlsdr_get_device_name( index );
}





//returns 0 if index not valid
bool gcrtl::get_get_device_usb_strings_using_index( uint32_t index, string &s_manufact, string &s_product, string &s_serial )
{
char manufact[256];
char product[256];
char serial[256];


if( rtlsdr_get_device_usb_strings( index, manufact, product, serial ) == 0 ) 
	{
	s_manufact = manufact;
	s_product = product;
	s_serial = serial;
	return 1;
	}
else return 0;
}





const char* gcrtl::get_device_name()
{
if( !b_is_open ) return 0;

return szname;															//use cached string obtained when device was opened (using an index)
}





bool gcrtl::get_get_device_usb_strings( string &s_manufact, string &s_product, string &s_serial )
{
if( !b_is_open ) return 0;

s_manufact = szmanufact;													//use cached strings obtained when device was opened (using an index)
s_product = szproduct;
s_serial = szserial;

//if( rtlsdr_get_device_usb_strings( index, manufact, product, serial ) == 0 ) return 1;
//else return 0;

return 1;
}






/*
RTLSDR_API int rtlsdr_get_device_usb_strings(uint32_t index,
					     char *manufact,
					     char *product,
					     char *serial);
*/


bool gcrtl::open( int dev_index )
{
int ppm_error = 0;
string s1;

if( b_is_open ) return 0;



int res = rtlsdr_open( &rtl_dev, (uint32_t)dev_index );
if ( res < 0 ) {
	fprintf( stderr, "Failed to open rtlsdr device #%d.\n", dev_index );
	rtl_dev = 0;
	b_is_open = 0;
	return 0;
	}

s1 = get_device_name_using_index( dev_index );	//cache this string while we have the device defined by an index (indexes change when a further devices are plugged in, so can't use calls that spec a device index, such as 'rtlsdr_get_device_name()', this has been changed in later versions of librtlsdr, to be fixed)
strncpy( szname, s1.c_str(), s1.length() );

rtlsdr_get_device_usb_strings( dev_index, szmanufact, szproduct, szserial );	//cache these strings while we have the device defined by an index (indexes change when a further devices are plugged in, so can't use calls that spec a device index, such as 'rtlsdr_get_device_usb_strings()', this has been changed in later versions of librtl, to be fixed)


set_srate( 2000000 );

pthread_mutex_init( &read_mutex, NULL );

b_is_open = 1;


/*
//fftw pointers
fftw_in_c0 = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftw_size );
fftw_out_c0 = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftw_size );

//single dimension complex to complex fwd fft
fftw_p_c0 = fftw_plan_dft_1d( fftw_size, fftw_in_c0, fftw_out_c0, FFTW_FORWARD, FFTW_ESTIMATE );


buff8 = (uint8_t*)malloc( rtl_buf_size * sizeof( uint8_t) );
if ( !buff8 )
	{
	fprintf( stderr, "Error: malloc.\n" );
	exit(1);
	}
*/

//rtlsdr_set_sample_rate( rtl_dev, ( uint32_t)a_d_srate );

//int ret = rtlsdr_reset_buffer( rtl_dev );

return 1;

}








bool gcrtl::set_bandwidth( unsigned int bwidth )
{
printf( "gcrtl::set_bandwidth() - bwidth %d\n", bwidth );

if( !b_is_open ) return 0;

int r;
r = rtlsdr_set_tuner_bandwidth( rtl_dev, bwidth );


if( r < 0 ) 
	{
	fprintf( stderr, "gcrtl::set_bandwidth() - failed to set bandwidth, retval: %d\n", r );
	return 0;
	}

return 1;
}






bool gcrtl::set_srate( unsigned int srate_in )
{
printf( "gcrtl::set_srate() - srate_in %d\n", srate_in );

if( !b_is_open ) return 0;

if( srate_in < cn_rtl_e4000_sample_bandwith_min ) return 0;	//this number was from internet, e4000 chip's lowest srate

srate = srate_in;

rtlsdr_set_sample_rate( rtl_dev, ( uint32_t)srate );

//int ret = rtlsdr_reset_buffer( rtl_dev );

return 1;
}




bool gcrtl::reset_buffer()
{
if( !b_is_open ) return 0;

int ret = rtlsdr_reset_buffer( rtl_dev );
return 1;
}







unsigned int gcrtl::get_srate()
{
if( !b_is_open ) return 0;

return rtlsdr_get_sample_rate( rtl_dev );
}







//must provide number >=512 and must be powers of 2, sets buff8 to this size,
//note: fftw size is set to half this size as buff8 holds I,Q,I,Q samples sequentially whereas
//fftw complex array has each location holding both IQ eg: fftw_in_c0[x][0]= I, fftw_in_c0[x][1]= Q

bool gcrtl::set_buf_size( unsigned int size_in )
{
if( size_in < 512 ) return 0;

rtl_buf_size = size_in;							//I,Q,I,Q     'IQ are sequential'
fftw_size = rtl_buf_size / 2;					//IQ, IQ, IQ  'IQ are parallel'


if( buf8 ) free ( buf8 );
buf8 = 0;

buf8 = (uint8_t*)malloc( rtl_buf_size * sizeof( uint8_t) );
if ( !buf8 )
	{
	fprintf( stderr, "gcrtl::set_buf_size() - malloc failed size was: %u\n", rtl_buf_size * sizeof( uint8_t) );
	exit( 1 );
	}



if ( fftw_p_c0 ) fftw_destroy_plan( fftw_p_c0 );
fftw_p_c0 = 0;

if ( fftw_in_c0 ) fftw_free( fftw_in_c0 );
if ( fftw_out_c0 ) fftw_free( fftw_out_c0 );
fftw_in_c0 = 0;
fftw_out_c0 = 0;

//fftw pointers
fftw_in_c0 = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftw_size );
fftw_out_c0 = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftw_size );

//single dimension complex to complex fwd fft
fftw_p_c0 = fftw_plan_dft_1d( fftw_size, fftw_in_c0, fftw_out_c0, FFTW_FORWARD, FFTW_ESTIMATE );

rtlsdr_reset_buffer( rtl_dev );

return 1;
}




unsigned int gcrtl::get_buf_size()
{
return rtl_buf_size;
}










bool gcrtl::close()
{

if( b_is_open ) rtlsdr_close( rtl_dev );
rtl_dev = 0;

if( buf8 ) free ( buf8 );
buf8 = 0;

if ( fftw_p_c0 ) fftw_destroy_plan( fftw_p_c0 );
fftw_p_c0 = 0;

if ( fftw_in_c0 ) fftw_free( fftw_in_c0 );
if ( fftw_out_c0 ) fftw_free( fftw_out_c0 );
fftw_in_c0 = 0;
fftw_out_c0 = 0;

pthread_mutex_destroy( &read_mutex );

b_is_open = 0;

return 1;
}










//e.g 'target_gain' of 115, means 11.5dB internal to tuner
int gcrtl::nearest_gain( int target_gain )
{
if( !b_is_open ) return 0;

int r, err1, err2, count, nearest;
int* gains;

r = rtlsdr_set_tuner_gain_mode( rtl_dev, 1 );

if( r < 0 ) 
	{
	fprintf( stderr, "gcrtl::nearest_gain() - failed to enable manual gain, retval: %d\n", r );
	return 0;
	}

count = rtlsdr_get_tuner_gains( rtl_dev, NULL );
if (count <= 0) 
	{
	return 0;
	}

gains = (int*) malloc( sizeof(int) * count );
count = rtlsdr_get_tuner_gains( rtl_dev, gains );
	
nearest = gains[ 0 ];
//printf( "gcrtl::nearest_gain() - lowest gain avail (in 1/10 ths of a dB) %d\n", nearest );

for ( int i = 0; i < count; i++ ) 
	{
//	printf( "gcrtl::nearest_gain() - checking gain %d\n", gains[i] );
	err1 = abs(target_gain - nearest);
	err2 = abs(target_gain - gains[i]);
	if ( err2 < err1 ) 
		{
		nearest = gains[i];
		}
	}

//printf( "gcrtl::nearest_gain() - nearest avail gain has been set: %d (%.2f dB)\n", nearest, (float)nearest/10.0f );

free( gains );
return nearest;
}







//'b_manual' = 1 for manual,
//else auto gain is set
bool gcrtl::set_gain_mode( bool b_manual )
{

if( !b_is_open ) return 0;

int r;
r = rtlsdr_set_tuner_gain_mode( rtl_dev, b_manual );

if( r < 0 ) 
	{
	fprintf( stderr, "gcrtl::set_gain_mode() - failed to set gain mode, retval: %d\n", r );
	return 0;
	}
return 1;
}





//will take your 'gain_in' and find 'near_gain()' that is valid for E4000, such as:

//internal values (in tenths of a dB) for the E4000 tuner:
//-10, 15, 40, 65, 90, 115, 140, 165, 190,
//215, 240, 290, 340, 420, 430, 450, 470, 490

//(Note your value is scaled up by 10, as internally 115 means 11.5dB )

bool gcrtl::set_gain( double gain_in )
{

if( !b_is_open ) return 0;


int r;
r = rtlsdr_set_tuner_gain_mode( rtl_dev, 1 );

if( r < 0 ) 
	{
	fprintf( stderr, "gcrtl::set_gain() - failed to enable manual gain, retval: %d\n", r );
	return 0;
	}
	
	
	

int near_gain = nearest_gain( gain_in*10 );

fprintf( stderr, "gcrtl::set_gain() - nearest avail gain has been set: %d (%.2f dB)\n", near_gain, (float)near_gain/10.0f );

r = rtlsdr_set_tuner_gain( rtl_dev, near_gain );

if ( r != 0 ) 
	{
	fprintf( stderr, "gcrtl::set_gain() - failed to set tuner gain, retval: %d\n", r );
	return 0;
	} 


return 1;
}




//returns gain in dB (e.g: internally 115 means 11.5dB )
float gcrtl::get_gain()
{
if( !b_is_open ) return 0;
	
int r = rtlsdr_get_tuner_gain( rtl_dev );
if( r == 0 )
	{
	fprintf( stderr, "gcrtl::get_gain() - rtlsdr_get_tuner_gain() - failed to get tuner gain, retval: %d\n", r );
	return 0;
	}

return  (float)r / 10.0f;
}








//param dev the device handle given by rtlsdr_open()
//param stage intermediate frequency gain stage number (1 to 6 for E4000)
//param gain in tenths of a dB, -30 means -3.0 dB.
//return 0 on success

bool gcrtl::set_tuner_if_gain( int stage, double gain_in )
{

if( !b_is_open ) return 0;


int r;
r = rtlsdr_set_tuner_gain_mode( rtl_dev, 1 );
if( r < 0 ) 
	{
	fprintf( stderr, "gcrtl::set_tuner_if_gain() - failed to enable manual gain, retval: %d\n", r );
	return 0;
	}


r = rtlsdr_set_tuner_if_gain( rtl_dev, stage, gain_in );

if( r < 0 ) 
	{
	fprintf( stderr, "gcrtl::set_tuner_if_gain() - failed to set tuner if gain, retval: %d\n", r );
	return 0;
	}
	
	
return 1;
}









bool gcrtl::set_agc_mode( bool b_on )
{

if( !b_is_open ) return 0;

int r;
r = rtlsdr_set_agc_mode( rtl_dev, b_on );
if( r < 0 ) 
	{
	fprintf( stderr, "gcrtl::set_agc_mode() - failed to set tuner if gain, retval: %d\n", r );
	return 0;
	}
return 1;
}










bool gcrtl::set_ppm( int ppm_error )
{
if( !b_is_open ) return 0;

int r;
r = rtlsdr_set_freq_correction( rtl_dev, ppm_error );
	
if ( r < 0 ) 
	{
	fprintf( stderr, "gcrtl::set_ppm_set() - failed to set ppm error\n");
	return 0;
	} 

return 1;
}











//returns -1 on error
int gcrtl::get_ppm()
{
if( !b_is_open ) return 0;

int r;
r = rtlsdr_get_freq_correction( rtl_dev );
	
return r;
}











//0: off
//1: I branch
//2: Q branch

//NOTE the gain NEEDS to set after 'set_dev_direct_sampling()' call
int gcrtl::set_direct_sampling( int Off_Ibranch_Qbranch )
{

if( !b_is_open ) return 0;

int r;
r = rtlsdr_set_direct_sampling( rtl_dev, Off_Ibranch_Qbranch );
if( r < 0 ) 
	{
	fprintf( stderr, "gcrtl::set_direct_sampling() - failed to set direct sampling, retval: %d\n", r );
	return 0;
	}
return 1;
}












//returns:
//0: off
//1: I branch
//2: Q branch
//-1 on error
int gcrtl::get_direct_sampling()
{
if( !b_is_open ) return -1;

int r;
r = rtlsdr_get_direct_sampling( rtl_dev );
if( r < 0 ) 
	{
	fprintf( stderr, "gcrtl::get_direct_sampling() - failed to get direct sampling, retval: %d\n", r );
	return -1;
	}
	
return r;
}











bool gcrtl::set_bias_tee( bool on )
{

if( !b_is_open ) return 0;

int r;
r = rtlsdr_set_bias_tee( rtl_dev, on );
if( r < 0 ) 
	{
	fprintf( stderr, "gcrtl::set_bias_tee() - failed to set bias tee, retval: %d\n", r );
	return 0;
	}
return 1;
}










//enable or disable offset tuning for zero-IF tuners, which may avoid
//problems caused by the DC offset of the ADCs and 1/f noise.
bool gcrtl::set_offset_tuning( bool b_on )
{
if( !b_is_open ) return 0;

int r = rtlsdr_set_offset_tuning( rtl_dev, b_on );
if ( r < 0 ) 
	{
	fprintf( stderr, "gcrtl::set_offset_tuning() - failed to set offset tuning state\n");
	return 0;
	} 

return 1;
}







bool gcrtl::get_offset_tuning()
{
if( !b_is_open ) return 0;

int r;
r = rtlsdr_get_offset_tuning( rtl_dev );
if( r < 0 ) 
	{
	fprintf( stderr, "gcrtl::get_direct_sampling() - failed to get offset_tuning, retval: %d\n", r );
	return 0;
	}

return (bool)r;
}













bool gcrtl::set_center_freq( double freq_in )
{
if( !b_is_open ) return 0;

cur_freq = freq_in;

int r = rtlsdr_set_center_freq( rtl_dev, cur_freq );
if ( r < 0 ) 
	{
	fprintf( stderr, "gcrtl::set_center_freq() - failed to set tuning freq\n");
	return 0;
	} 

r = (int)rtlsdr_get_center_freq( rtl_dev );
printf("gcrtl::set_center_freq() - actual center freq set: %d\n", r );

return 1;
}












//returns 0 on error
int gcrtl::get_center_freq()
{
if( !b_is_open ) return 0;

int r = rtlsdr_get_center_freq( rtl_dev );
if ( r == 0 ) 
	{
	fprintf( stderr, "gcrtl::get_center_freq() - failed to get tuning freq\n");
	return 0;
	} 

return r;
}





//NOTE: this function call does not return, call it via a second thread,
//only call once to start a continual async callback at a rate determined by bandwidth and buffer size,
//e.g: if bw: 2400000 and buffer size: 524288, the callback will get called every ~0.1093 Secs
//calc is: 2400000 samples/sec (i.e: 4800000 IQ samples/sec),  524288/ 4800000 = 0.10922 Secs

//e.g: if bw: 1200000 and buffer 524288 the callback will get called every: 524288 / 2400000 = 0.2185 Secs
//e.g: if bw: 1200000 and buffer 65536 the callback will get called every: 65536 / 2400000 = 0.02731 Secs

//call cancel_async(); to stop callback
//NOTE: this function call does not return, call it via a second thread
bool gcrtl::read_async( rtlsdr_read_async_cb_t cb, void *arg, unsigned int count )
{
if( !b_is_open ) return 0;

#define DEFAULT_ASYNC_BUF_NUMBER 32

rtlsdr_read_async( rtl_dev, cb, arg, DEFAULT_ASYNC_BUF_NUMBER, count );

printf("gcrtl::read_async()");
return 1;
}


















//stop what read_async() started
bool gcrtl::cancel_async()
{
if( !b_is_open ) return 0;

rtlsdr_cancel_async( rtl_dev );

return 1;
}









double theta1 = 0;
double theta2 = 0;
double theta3 = 0;
double theta4 = 0;

double freq_test1 = 20000;


//read rtl dongle data, adjusted to provide levels between +/- 127, 
//I,Q sequential data are seperated into spec vbuf container,
//does not clear vbuf
bool gcrtl::read( vector <filter_code::st_cplex_tag> &vbuf )
{
int ret, n_read;

if( !b_is_open ) return 0;
if( !buf8 ) return 0;


//printf("grrrr rtl_buf_size=%u\n", rtl_buf_size );


pthread_mutex_lock( &read_mutex );

//reset endpoint before we start reading from it (mandatory)
verbose_reset_buffer( rtl_dev );
rtlsdr_read_sync( rtl_dev, buf8, rtl_buf_size, &n_read );

pthread_mutex_unlock( &read_mutex );

if ( n_read != rtl_buf_size )
	{
	fprintf( stderr, "Error: dropped samples, needed %d, got %d\n", rtl_buf_size, n_read );
	return 0;
	}


fprintf( stderr, "read: %d\n", n_read );


//test sinewaves

double time_per_sample = 1.0 / srate;
double phase1 = 0;

double freq_test2 = 860000;
double theta2_inc = freq_test2 * twopi * time_per_sample;
double phase2 = 0;


double freq_test3 = 10000;
double theta3_inc = freq_test3 * twopi * time_per_sample;

double freq_test4 = 960000;
double theta4_inc = freq_test4 * twopi * time_per_sample;

filter_code::st_cplex_tag o;

int n = 0;
for( int i = 0; i < ( n_read ); i += 2 )
	{

	double carrier_fm = freq_test1 + 10000.0 * cosf( theta3 );// + 10000.0 * cos( theta4 );		//modulate carrier

	double theta1_inc = carrier_fm * twopi * time_per_sample;


	o.real = buf8[ i ] - 127;		//sample is 127 for zero signal,so 127 +/-127
	o.imag = buf8[ i + 1 ] - 127;

bool tone = 0;

	if( tone )
		{
		o.real = 0;
		o.imag = 0;
	
		o.real += 127.0 * cos( theta1 + phase1 );						//test signal
		o.imag += 127.0 * sin( theta1 + phase1 );						//test signal

		}

//	fprintf( stderr, "got buf8: %g %g\n", fftw_in_c0[ n ][ 0 ], fftw_in_c0[ n ][ 1 ] );

	n++;

	theta1 += theta1_inc;
	if( theta1 >= twopi ) theta1 -= twopi;

	theta2 += theta2_inc;
	if( theta2 >= twopi ) theta2 -= twopi;

	theta3 += theta3_inc;
	if( theta3 >= twopi ) theta3 -= twopi;

	theta4 += theta4_inc;
	if( theta4 >= twopi ) theta4 -= twopi;

	vbuf.push_back( o );
	}

return 1;
}



















//read rtl dongle and do an fft
bool gcrtl::read_fft( vector <filter_code::st_cplex_tag> &vfft )
{
int ret, n_read;

if( !b_is_open ) return 0;
if( !buf8 ) return 0;

if( !fftw_in_c0 ) return 0;
if( !fftw_out_c0 ) return 0;
if( !fftw_p_c0 ) return 0;

//printf("grrrr rtl_buf_size=%u\n", rtl_buf_size );


pthread_mutex_lock( &read_mutex );

//reset endpoint before we start reading from it (mandatory)
verbose_reset_buffer( rtl_dev );
rtlsdr_read_sync( rtl_dev, buf8, rtl_buf_size, &n_read );

pthread_mutex_unlock( &read_mutex );

if ( n_read != rtl_buf_size )
	{
	fprintf( stderr, "Error: dropped samples, needed %d, got %d\n", rtl_buf_size, n_read );
	return 0;
	}


//fprintf( stderr, "read: %d\n", n_read );


//test sinewaves

double time_per_sample = 1.0 /srate;
double phase1 = 0;

double freq_test2 = 860000;
double theta2_inc = freq_test2 * twopi * time_per_sample;
double phase2 = 0;


double freq_test3 = 20000;
double theta3_inc = freq_test3 * twopi * time_per_sample;

double freq_test4 = 960000;
double theta4_inc = freq_test4 * twopi * time_per_sample;


int n = 0;
for( int i = 0; i < ( n_read ); i += 2 )
	{

	double carrier_fm = freq_test1 + 10000.0 * cos( theta3 );// + 10000.0 * cos( theta4 );		//modulate carrier

	double theta1_inc = carrier_fm * twopi * time_per_sample;


	fftw_in_c0[ n ][ 0 ] = buf8[ i ] - 127;		//sample is 127 for zero signal,so 127 +/-127
	fftw_in_c0[ n ][ 1 ] = buf8[ i + 1 ] - 127;

bool tone = 0;

	if( tone )
		{
		fftw_in_c0[ n ][ 0 ] = 0;
		fftw_in_c0[ n ][ 1 ] = 0;

		fftw_in_c0[ n ][ 0 ] += 127.0 * cos( theta1 + phase1 );			//test signal
		fftw_in_c0[ n ][ 1 ] += 127.0 * sin( theta1 + phase1 );			//test signal

//		fftw_in_c0[ n ][ 0 ] += 60.0 * cos( theta2 + phase2 );			//test signal
//		fftw_in_c0[ n ][ 1 ] += 60.0 * sin( theta2 + phase2 );			//test signal
		}

//	fftw_in_c0[ n ][ 0 ] = 1.234;
//	fftw_in_c0[ n ][ 1 ] = 567.8;

//	fprintf( stderr, "got buf8: %g %g\n", fftw_in_c0[ n ][ 0 ], fftw_in_c0[ n ][ 1 ] );

	n++;

	theta1 += theta1_inc;
	if( theta1 >= twopi ) theta1 -= twopi;

	theta2 += theta2_inc;
	if( theta2 >= twopi ) theta2 -= twopi;

	theta3 += theta3_inc;
	if( theta3 >= twopi ) theta3 -= twopi;

	theta4 += theta4_inc;
	if( theta4 >= twopi ) theta4 -= twopi;
	}

//fprintf( stderr, "n= %d\n", n );


fftw_execute( fftw_p_c0 );						//calc complex to complex fwd fft

//st_spect_tag o;




//int half = fftw_size / 2;


int freq_offset = 0;

//int fft_bin_freq_width = a_d_srate / fftw_size;


for( int i = 0; i < fftw_size; i++ )
	{
//	double d1;
//	double d2;

/*
	if( i < half )
		{
		//negative spectrum
		d1 = (double)fftw_out_c0[ half + i ][ 0 ] / (double)fftw_size;
		d2 = (double)fftw_out_c0[ half + i ][ 1 ] / (double)fftw_size;
		}
	else{
		d1 = (double)fftw_out_c0[ i - half ][ 0 ] / (double)fftw_size;
		d2 = (double)fftw_out_c0[ i - half ][ 1 ] / (double)fftw_size;
		}
*/
//	o.freq = cur_freq + ( -half + i ) * fft_bin_freq_width;

	filter_code::st_cplex_tag o;

	o.real = fftw_out_c0[ i ][ 0 ] / (double)fftw_size;
	o.imag = fftw_out_c0[ i ][ 1 ] / (double)fftw_size;
	
	vfft.push_back( o );
//	printf(" o.freq = %lf\n", o.freq );

	}

//exit(0);

return 1;
}






//int kill_off = 0;



//read rtl dongle and do an fft suitable for graphing, spectra are in correct order (not fftw order)
//negative freqs are first then positive
//does not clear vsp
bool gcrtl::read_fft_graph( vector <st_spect_tag> &vsp, bool b_suppress_zero_ifreq )
{
int ret, n_read;

if( !b_is_open ) return 0;
if( !buf8 ) return 0;

if( !fftw_in_c0 ) return 0;
if( !fftw_out_c0 ) return 0;
if( !fftw_p_c0 ) return 0;

//printf("grrrr rtl_buf_size=%u\n", rtl_buf_size );


pthread_mutex_lock( &read_mutex );

//reset endpoint before we start reading from it (mandatory)
verbose_reset_buffer( rtl_dev );
rtlsdr_read_sync( rtl_dev, buf8, rtl_buf_size, &n_read );

pthread_mutex_unlock( &read_mutex );

if ( n_read != rtl_buf_size )
	{
	fprintf( stderr, "Error: dropped samples, needed %d, got %d\n", rtl_buf_size, n_read );
	return 0;
	}


//fprintf( stderr, "read: %d\n", n_read );


//test sinewave
double freq_test = 80000;
double time_per_sample = 1.0 / srate;
double theta_inc = freq_test * twopi * time_per_sample;
double theta = 0;
double phase = 0;

int n = 0;
for( int i = 0; i < ( n_read ); i += 2 )
	{


	fftw_in_c0[ n ][ 0 ] = buf8[ i ] - 127;		//sample is 127 for zero signal,so 127 +/-127
	fftw_in_c0[ n ][ 1 ] = buf8[ i + 1 ] - 127;

//	fftw_in_c0[ n ][ 0 ] = 0;
//	fftw_in_c0[ n ][ 1 ] = 0;

//	fftw_in_c0[ n ][ 0 ] += 170.0 * cos( theta + phase );			//test signal
//	fftw_in_c0[ n ][ 1 ] += 170.0 * sin( theta + phase );			//test signal

//	fftw_in_c0[ n ][ 0 ] = 1.234;
//	fftw_in_c0[ n ][ 1 ] = 567.8;

//	fprintf( stderr, "got buf8: %g %g\n", fftw_in_c0[ n ][ 0 ], fftw_in_c0[ n ][ 1 ] );

	n++;

	theta += theta_inc;
	if( theta >= twopi ) theta -= twopi;
	}

//fprintf( stderr, "n= %d\n", n );


fftw_execute( fftw_p_c0 );						//calc complex to complex fwd fft

st_spect_tag o;




int half = fftw_size / 2;


int freq_offset = 0;

int fft_bin_freq_width = srate / fftw_size;

//int negs = 0;
//int poss= 0;


for( int i = 0; i < ( fftw_size ); i++ )
	{
	double d1;
	double d2;
	
	if( i == 0 ) continue;					//skip dc term
	if( i == 2048 ) continue;				//skip dc term


	if( i < half )
		{
		//negative spectrum
		d1 = (double)fftw_out_c0[ half + i ][ 0 ] / (double)fftw_size;
		d2 = (double)fftw_out_c0[ half + i ][ 1 ] / (double)fftw_size;
//printf(" neg = %d   ", half + i );
//negs++;
		}
	else{
		d1 = (double)fftw_out_c0[ i - half ][ 0 ] / (double)fftw_size;
		d2 = (double)fftw_out_c0[ i - half ][ 1 ] / (double)fftw_size;
//printf(" pos = %d", i - half );
//poss++;
		}


	int supress_width = 5 / 2;
	
	if( b_suppress_zero_ifreq )
		{
		if( ( i >= ( half - supress_width ) ) && ( i <= ( half + supress_width ) ) )
			{
			d1 = 0;
			d2 = 0;
			}
		}

	o.freq = cur_freq + ( -half + i ) * fft_bin_freq_width;
	o.ampl = sqrt( d1 * d1 + d2 * d2  );

//if( i == 0 )  o.ampl = -5;
//if( i == 2048 )  o.ampl = -3;
	vsp.push_back( o );
//	printf(" o.freq = %lf\n", o.freq );

	}
	
//kill_off++;
//if( kill_off == 5 )
//	{
//	printf( "\n negs= %d, poss=%d\n", negs, poss );
//	printf( "\n 0th= %lf, 2047th=%lf, 2048th=%lf\n", (double)fftw_out_c0[ 0 ][ 0 ], (double)fftw_out_c0[ 2047 ][ 0 ], (double)fftw_out_c0[ 2048 ][ 0 ] );
//	printf( "\n 1th= %lf, 2049th=%lf\n", (double)fftw_out_c0[ 1 ][ 0 ], (double)fftw_out_c0[ 2049 ][ 0 ] );
//	for( ;; )
//		{
//		sleep( 1 );
//		}
//	}


//exit(0);

return 1;
}







/*
//read rtl dongle and do an fft suitable for graphing, spectra are in correct order (not fftw order)
//negative freqs are first then positive
//does not clear vsp
bool gcrtl::read_fft_graph( vector <st_spect_tag> &vsp, bool b_suppress_zero_ifreq )
{
int ret, n_read;

if( !b_is_open ) return 0;
if( !buff8 ) return 0;

if( !fftw_in_c0 ) return 0;
if( !fftw_out_c0 ) return 0;
if( !fftw_p_c0 ) return 0;

//printf("grrrr rtl_buf_size=%u\n", rtl_buf_size );


pthread_mutex_lock( &read_mutex );

//reset endpoint before we start reading from it (mandatory)
verbose_reset_buffer( rtl_dev );
rtlsdr_read_sync( rtl_dev, buff8, rtl_buf_size, &n_read );

pthread_mutex_unlock( &read_mutex );

																													return 1;

if ( n_read != rtl_buf_size )
	{
	fprintf( stderr, "Error: dropped samples, needed %d, got %d\n", rtl_buf_size, n_read );
	return 0;
	}


//fprintf( stderr, "read: %d\n", n_read );


//test sinewave
double freq_test = 80000;
double time_per_sample = 1.0 / srate;
double theta_inc = freq_test * twopi * time_per_sample;
double theta = 0;
double phase = 0;

int n = 0;
for( int i = 0; i < ( n_read ); i += 2 )
	{


	fftw_in_c0[ n ][ 0 ] = buff8[ i ] - 127;		//sample is 127 for zero signal,so 127 +/-127
	fftw_in_c0[ n ][ 1 ] = buff8[ i + 1 ] - 127;

//	fftw_in_c0[ n ][ 0 ] = 0;
//	fftw_in_c0[ n ][ 1 ] = 0;

//	fftw_in_c0[ n ][ 0 ] += 170.0 * cos( theta + phase );			//test signal
//	fftw_in_c0[ n ][ 1 ] += 170.0 * sin( theta + phase );			//test signal

//	fftw_in_c0[ n ][ 0 ] = 1.234;
//	fftw_in_c0[ n ][ 1 ] = 567.8;

//	fprintf( stderr, "got buf8: %g %g\n", fftw_in_c0[ n ][ 0 ], fftw_in_c0[ n ][ 1 ] );

	n++;

	theta += theta_inc;
	if( theta >= twopi ) theta -= twopi;
	}

//fprintf( stderr, "n= %d\n", n );


fftw_execute( fftw_p_c0 );						//calc complex to complex fwd fft

st_spect_tag o;




int half = fftw_size / 2;


int freq_offset = 0;

int fft_bin_freq_width = srate / fftw_size;

//int negs = 0;
//int poss= 0;


for( int i = 0; i < ( fftw_size ); i++ )
	{
	double d1;
	double d2;
	
	if( i == 0 ) continue;					//skip dc term
	if( i == 2048 ) continue;				//skip dc term


	if( i < half )
		{
		//negative spectrum
		d1 = (double)fftw_out_c0[ half + i ][ 0 ] / (double)fftw_size;
		d2 = (double)fftw_out_c0[ half + i ][ 1 ] / (double)fftw_size;
//printf(" neg = %d   ", half + i );
//negs++;
		}
	else{
		d1 = (double)fftw_out_c0[ i - half ][ 0 ] / (double)fftw_size;
		d2 = (double)fftw_out_c0[ i - half ][ 1 ] / (double)fftw_size;
//printf(" pos = %d", i - half );
//poss++;
		}


	int supress_width = 5 / 2;
	
	if( b_suppress_zero_ifreq )
		{
		if( ( i >= ( half - supress_width ) ) && ( i <= ( half + supress_width ) ) )
			{
			d1 = 0;
			d2 = 0;
			}
		}

	o.freq = cur_freq + ( -half + i ) * fft_bin_freq_width;
	o.ampl = sqrt( d1 * d1 + d2 * d2  );

//if( i == 0 )  o.ampl = -5;
//if( i == 2048 )  o.ampl = -3;
	vsp.push_back( o );
//	printf(" o.freq = %lf\n", o.freq );

	}
	
//kill_off++;
//if( kill_off == 5 )
//	{
//	printf( "\n negs= %d, poss=%d\n", negs, poss );
//	printf( "\n 0th= %lf, 2047th=%lf, 2048th=%lf\n", (double)fftw_out_c0[ 0 ][ 0 ], (double)fftw_out_c0[ 2047 ][ 0 ], (double)fftw_out_c0[ 2048 ][ 0 ] );
//	printf( "\n 1th= %lf, 2049th=%lf\n", (double)fftw_out_c0[ 1 ][ 0 ], (double)fftw_out_c0[ 2049 ][ 0 ] );
//	for( ;; )
//		{
//		sleep( 1 );
//		}
//	}


//exit(0);

return 1;
}
*/















bool rtl_scan2( double freq_start, double freq_stop, double freq_step, double gain_in )
{


//if( !rtl.open( 2048000, 512 ) );

//rtl.set_gain( gain_in );

//rtl.tune( freq_start, freq_stop, freq_step );

//rtl.read( );

return 1;
}



/*

bool rtl_scan22( double freq_start, double freq_stop, double freq_step, double gain_in )
{
int dev_index = 0;
int dev_given = 0;
int ppm_error = 0;
int ret, f, n_read, offset, bin_e, bin_len, buf_len, ds, ds_p;
int a_d_srate = 2048000;


unsigned int fftw_size = 256;

unsigned int rtl_buf_siz = fftw_size * 2;

fftw_complex *fftw_in_c0 = 0;						//complex pointers
fftw_complex *fftw_out_c0 = 0;

fftw_plan fftw_p_c0 = 0;


//fftw pointers
fftw_in_c0 = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftw_size );
fftw_out_c0 = ( fftw_complex* ) fftw_malloc( sizeof( fftw_complex ) * fftw_size );

//single dimension complex to complex fwd fft
fftw_p_c0 = fftw_plan_dft_1d( fftw_size, fftw_in_c0, fftw_out_c0, FFTW_FORWARD, FFTW_ESTIMATE );



int r = rtlsdr_open( &dev, (uint32_t)dev_index );
if (r < 0) {
	fprintf( stderr, "Failed to open rtlsdr device #%d.\n", dev_index );
	return 0;
	}

double gain = nearest_gain( dev, gain_in );
verbose_gain_set( dev, gain );


verbose_ppm_set( dev, ppm_error );

//reset endpoint before we start reading from it (mandatory)
verbose_reset_buffer( dev );

//actually do stuff
rtlsdr_set_sample_rate( dev, ( uint32_t)a_d_srate );

ret = rtlsdr_set_center_freq( dev, freq_start );

f = (int)rtlsdr_get_center_freq( dev );
printf("centre freq: %d\n", f );

ret = rtlsdr_set_tuner_gain_mode( dev, 1 );


uint8_t *buf8 = (uint8_t*)malloc( rtl_buf_siz * sizeof( uint8_t) );

if ( !buf8 )
	{
	fprintf(stderr, "Error: malloc.\n");
	exit(1);
	}


ret = rtlsdr_reset_buffer( dev );

rtlsdr_read_sync( dev, buf8, rtl_buf_siz, &n_read );

if ( n_read != rtl_buf_siz) fprintf( stderr, "Error: dropped samples\n" );


fprintf(stderr, "read: %d\n", n_read );


//test sinewave
double freq_test = a_d_srate - 100000;
double time_per_sample = 1.0 / a_d_srate;
double theta_inc = freq_test * twopi * time_per_sample;
double theta = 0;
double phase = 0;

int n = 0;
for( int i = 0; i < ( n_read ); i += 2 )
	{


	fftw_in_c0[ n ][ 0 ] = buf8[ i ] - 127;		//sample is 127 for zero signal,so 127 +/-127
	fftw_in_c0[ n ][ 1 ] = buf8[ i + 1 ] - 127;

//	fftw_in_c0[ n ][ 0 ] = 0;
//	fftw_in_c0[ n ][ 1 ] = 0;

	fftw_in_c0[ n ][ 0 ] += 128.0 * cos( theta + phase );			//test signal
	fftw_in_c0[ n ][ 1 ] += 128.0 * sin( theta + phase );			//test signal

//	fftw_in_c0[ n ][ 0 ] = 1.234;
//	fftw_in_c0[ n ][ 1 ] = 567.8;

//	fprintf( stderr, "got buf8: %g %g\n", fftw_in_c0[ n ][ 0 ], fftw_in_c0[ n ][ 1 ] );

	n++;

	theta += theta_inc;
	if( theta >= twopi ) theta -= twopi;
	}

//fprintf( stderr, "n= %d\n", n );


fftw_execute( fftw_p_c0 );						//calc complex to complex fwd fft

st_spect_tag o;




for( int i = 0; i < fftw_size; i++ )
	{
//	fftw_out_c0[ i ][ 0 ] = 1.234;
//	fftw_out_c0[ i ][ 1 ] = 567.8;

	o.freq = i;
	double d1 = (double)fftw_out_c0[ i ][ 0 ] / (double)fftw_size;
	double d2 = (double)fftw_out_c0[ i ][ 1 ] / (double)fftw_size;

	o.ampl = sqrt( d1 * d1 + d2 * d2  );
	vspect.push_back( o );

//	fprintf( stderr, "got buf8: %g %g, out: %g,  %g %g\n", (double)fftw_in_c0[ i ][ 0 ], (double)fftw_in_c0[ i ][ 1 ], (double)o.ampl, (double)fftw_out_c0[ i ][ 0 ], (double)fftw_out_c0[ i ][ 1 ] );

//	fprintf( stderr, "got: %g,  %g %g\n", o.ampl, d1, d2 );
//	fprintf( stderr, "got: %lg,  %lg %lg\n", o.ampl, (double)fftw_out_c0[ i ][ 0 ], (double)fftw_out_c0[ i ][ 1 ] );
	}


if ( fftw_p_c0 ) fftw_destroy_plan( fftw_p_c0 );
fftw_p_c0 = 0;

if ( fftw_in_c0 ) fftw_free( fftw_in_c0 );
if ( fftw_out_c0 ) fftw_free( fftw_out_c0 );
fftw_in_c0 = 0;
fftw_out_c0 = 0;

rtlsdr_close( dev );

free ( buf8 );

return 1;
}

*/





/*

//#!/bin/sh
//#./rtl_fm -M am -f 120.5M -s 200000 -r 48000 -A std -g 100 | aplay -r 48k -f S16_LE -t raw -c 1
//#./rtl_fm -M am -f 126.1M -s 200000 -r 48000 -A std -g 100 | aplay -r 48k -f S16_LE -t raw -c 1
//#./rtl_fm -M am -f 126.25M -s 200000 -r 48000 -A std -g 100 | aplay -r 48k -f S16_LE -t raw -c 1
//./rtl_fm -M wfm -f 92.9M -s 200000 -r 48000 -A std -g 1 | aplay -r 48k -f S16_LE -t raw -c 1


bool rtl_scan( double freq_start, double freq_stop, double freq_step, double gain_in )
{

return rtl_scan2( freq_start, freq_stop, freq_step , gain_in );


struct sigaction sigact;
char *filename = NULL;
int i, length, r, opt, wb_mode = 0;
int f_set = 0;
int gain = AUTO_GAIN; // tenths of a dB
int dev_index = 0;
int dev_given = 0;
int ppm_error = 0;
int interval = 10;
int fft_threads = 1;
int smoothing = 0;
int single = 0;
int direct_sampling = 0;
int offset_tuning = 0;
double crop = 0.0;
char *freq_optarg;
time_t next_tick;
time_t time_now;
time_t exit_time = 0;
char t_str[50];
struct tm *cal_time;
double (*window_fn)(int, int) = rectangle;
freq_optarg = (char*)"";

do_exit = 0;
dev = NULL;
tune_count = 0;
boxcar = 1;
comp_fir_size = 0;
peak_hold = 0;


frequency_range( freq_start, freq_stop, freq_step, crop );
exit_time = (time_t) 1;

filename = (char*)"zzampl.csv";
interval = 1;
//gain = (int)(atof(optarg) * 10);


r = rtlsdr_open( &dev, (uint32_t)dev_index );
if (r < 0) {
	fprintf( stderr, "Failed to open rtlsdr device #%d.\n", dev_index );
	return 0;
	}

sigact.sa_handler = sighandler;
sigemptyset(&sigact.sa_mask);
sigact.sa_flags = 0;
sigaction(SIGINT, &sigact, NULL);
sigaction(SIGTERM, &sigact, NULL);
sigaction(SIGQUIT, &sigact, NULL);
sigaction(SIGPIPE, &sigact, NULL);


printf("tune_count= %d\n", tune_count );

//Set the tuner gain
//if (gain == AUTO_GAIN) {
//	verbose_auto_gain(dev);
//} else {
	gain = nearest_gain( dev, gain_in );
	verbose_gain_set(dev, gain);
//}

verbose_ppm_set(dev, ppm_error);

if( use_file )
	{
		file = fopen(filename, "wb");
		if (!file) {
			fprintf(stderr, "Failed to open %s\n", filename);
			return 0;
		}
	}

	// Reset endpoint before we start reading from it (mandatory)
	verbose_reset_buffer(dev);

	// actually do stuff
	rtlsdr_set_sample_rate(dev, (uint32_t)tunes[0].rate);
	sine_table(tunes[0].bin_e);
	next_tick = time(NULL) + interval;
	if (exit_time) {
		exit_time = time(NULL) + exit_time;}
	fft_buf = (int16_t*)malloc(tunes[0].buf_len * sizeof(int16_t));
	length = 1 << tunes[0].bin_e;
	window_coefs = (int*)malloc(length * sizeof(int));
	for (i=0; i<length; i++) {
		window_coefs[i] = (int)(256*window_fn(i, length));
	}
	while (!do_exit) {
		scanner( vspect );
		time_now = time(NULL);
		if (time_now < next_tick) {
			continue;}
		// time, Hz low, Hz high, Hz step, samples, dbm, dbm, ...
		cal_time = localtime(&time_now);
		strftime(t_str, 50, "%Y-%m-%d, %H:%M:%S", cal_time);
		for (i=0; i<tune_count; i++) {
			if( use_file )
				{
				fprintf(file, "%s, ", t_str);
				}
			csv_dbm( &tunes[i], vspect );
		}
		if( use_file ) fflush(file);
		while (time(NULL) >= next_tick) {
			next_tick += interval;}
		if (single) {
			do_exit = 1;}
		if (exit_time && time(NULL) >= exit_time) {
			do_exit = 1;}
	}

	// clean up

	if (do_exit) {
		fprintf(stderr, "\nUser cancel, exiting...\n");}
	else {
		fprintf(stderr, "\nLibrary error %d, exiting...\n", r);}

	if( use_file )
		{
		if (file != stdout) {
			fclose(file);}
		}

	rtlsdr_close(dev);
	free(fft_buf);
	free(window_coefs);

return 1;
}


*/





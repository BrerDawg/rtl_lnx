

#ifndef globals_h
#define globals_h



//linux code
#ifndef compile_for_windows
#define LLU "llu"
#endif


//windows code
#ifdef compile_for_windows
#define LLU "I64u"
#endif

#define max_16bit 32767.0
#define def_VU0 max_16bit * 0.9				//value to show 0 VU, similar to -20dBFS 

#define pi ((float)(M_PI))
#define pi2 ((float)(M_PI*2))
#define twopi (2.0 * M_PI)


#define cnsAppName "rtl_lnx"
#define cnsAppWndName "rtl_lnx app"
#define cns_version "v1.06"
#define cns_tops_thread_text_fname "tops_thread.txt"



typedef double flts;				//define numerical calc precision: float/double
//typedef double flts;			//define numerical calc precision: float/double

/*
struct st_cplex_tag
{
double real;
double imag;
};
*/


struct col_tag
{
int r;
int g;
int b;
};



struct colref
{
int r, g, b;
};




enum en_mode
{
en_mode_silence,
en_mode_stop,
en_mode_scan_mode,
en_mode_scan_mode_single,
en_mode_listen,
en_mode_squelch,
};




//IF YOU ADD to this also modify 'sz_demodulator_type[]', ALSO MOD 'rtl_graph_wnd::demod_type_idx_from_str()'
//MOD ALSO 'pulldown_demod_mode'

enum en_demodulator_type_tag
{
en_dmt_fm_stereo,
en_dmt_wfm,
en_dmt_fm,
en_dmt_am,
en_dmt_ssb,
//en_dmt_lsb,
//en_dmt_usb,
};



enum en_modulation_type_tag
{
en_mdt_wfm,
en_mdt_fm,
en_mdt_am,
en_mdt_lsb,
en_mdt_usb,
//en_mdt_raw,
};



enum en_arc_tangest
{
en_at_std,
en_at_fast,
en_at_lut
};


struct st_spect_tag
{
double freq;															//freq within the current dev bandwidth
double freq_actual;														//actual received freq as opposed to defined freq defined in 'freq'			
double ampl;
};



#endif

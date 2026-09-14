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

//aa_canvas.h

//v1.03


#ifndef aa_canvas_h
#define aa_canvas_h



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
#include <mutex>
#include <bits/stdc++.h>		//for std::unordered_map

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Enumerations.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_JPEG_Image.H>
#include <FL/Fl_File_Input.H>
#include <FL/Fl_Input_Choice.H>
#include <FL/Fl_Table_Row.H>

//#include "globals.h"
#include "GCProfile.h"
#include "filter_code.h"
#include "line_clip_code.h"
#include "bmp_code.h"


//linux code
#ifndef compile_for_windows

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>

#define _FILE_OFFSET_BITS 64			//large file handling
//#define _LARGE_FILES
#include <dirent.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <syslog.h>		//MakeIniPathFilename(..) needs this
#endif


//windows code
#ifdef compile_for_windows
#include <windows.h>
#include <process.h>
#include <winnls.h>
#include <share.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>
#include <conio.h>

#define WC_ERR_INVALID_CHARS 0x0080		//missing from gcc's winnls.h
#endif



#define pi ((float)M_PI)
#define twopi ((float)M_PI*2.0f)


using namespace std;

#define cn_aa_canvas_layer_max 128

#define cn_aa_canvas_max_ww 16384
#define cn_aa_canvas_max_hh 16384



struct st_aa_canvas_pixel_point_tag
{
unsigned int layer;
int x0, y0;
int rr, gg, bb;
};






struct st_aa_canvas_coord_tag
{
int x0, y0;

int rr, gg, bb;	
};





struct st_aa_canvas_polygon_tag
{
int posx, posy;
int rectx, recty;
int ww, hh;

int cntr_x, cntr_y;														//center of obj, affects rotations, (0,0) is top left of obj
float scale;
float rotate_theta;

vector<st_aa_canvas_coord_tag> vcrd;
};





/*
struct st_aa_canvas_outline_pnt_tag
{
int x;
int y;

};
*/



struct st_aa_canvas_rect_tag
{
int x0, y0;
int ww, hh;	
};


struct st_aa_canvas_bounding_rect_tag
{
int x0, y0;
int x1, y1;

};



enum en_aa_filter_kernal_type
{
en_afkt_linear,
en_afkt_blackman_harris,	
};


struct st_aa_canvas_tag
{
int xx;
int yy;
int ww;
int hh;
int bytes_per_pixel;
int linedx;																//number of bytes in a line of pixels, e.g:  ww * bytes_per_pixel

line_clip *lineclp;

unsigned int bfsiz;
unsigned char *bf;

en_aa_filter_kernal_type filter_kernel_type;
unsigned int kern_size0;												//kernel size for downsample filter, odd gives a 1.0 peak
float kern00[ 256 ];


int a_x0[1080];
int a_x1[1080];
//vector<st_aa_canvas_outline_pnt_tag>voutline;

};









class aa_canvas
{
private:

public:
st_aa_canvas_tag st_aa[cn_aa_canvas_layer_max];
vector<st_aa_canvas_pixel_point_tag> vpixpnt;							//holds pixel location that were individualy set by pixel plot functions (i.e. excluding bitmap block operations)


public:
aa_canvas();
~aa_canvas();

bool create_layer_from_bmp_file( string fname, unsigned int layer, en_aa_filter_kernal_type filt_kern_type, unsigned int kernel_size_in );
bool create_layer( unsigned int layer, unsigned int ww, unsigned int hh, unsigned int bytes_per_pixel_in, en_aa_filter_kernal_type filt_kern_type, unsigned int kernel_size_in );
bool get_layer_details( unsigned int layer, unsigned int &ww, unsigned int &hh, unsigned int &linedx, unsigned int &bytes_per_pixel );
bool set_layer_details( unsigned int layer, unsigned int ww, unsigned int hh, unsigned int linedx, unsigned int bytes_per_pixel );
bool block_set( unsigned int layer, unsigned char *bfin, unsigned int xx, unsigned int yy, unsigned int ww, unsigned int hh );
bool block_set_old( unsigned int layer, unsigned char *bfin, unsigned int xx, unsigned int yy, unsigned int ww, unsigned int hh );
bool block_get( unsigned int layer, unsigned char *bfout, unsigned int xx, unsigned int yy, unsigned int ww, unsigned int hh );
bool block_get_old( unsigned int layer, unsigned char *bfout, unsigned int xx, unsigned int yy, unsigned int ww, unsigned int hh );
bool block_duplicate( unsigned int layer_src, unsigned int layer_dest );
bool block_copy_no_reshape( unsigned int layer_src, unsigned int layer_dest, unsigned int xx, unsigned int yy, unsigned int ww, unsigned int hh, unsigned int destxx, unsigned int destyy );
bool block_copy_old( unsigned int layer_src, unsigned int layer_dest, unsigned int xx, unsigned int yy, unsigned int ww, unsigned int hh, unsigned int destxx, unsigned int destyy );
bool block_fill( unsigned int layer, unsigned int xx, unsigned int yy, unsigned int ww, unsigned int hh, uint8_t rr, uint8_t gg, uint8_t bb );
//bool filter_kernel_float( en_filter_window_type_tag wnd_type, float *w, int N );
//bool filter_scale_block( int layer, unsigned char *bfsrc, int srcx_in, int srcy_in, int wid, int hei, int linedx, unsigned char *bfdest, int destx, int desty, int destlinedx, float scale_in, unsigned int kernel_size, int bytes_per_pixel_in, int &rendered_wid, int &rendered_hei );

bool block_filter_scale_layer( unsigned int layer_src, int srcx_in, int srcy_in, unsigned int layer_dest, float destx, float desty, float scale_in, int bytes_per_pixel_in, unsigned int &rendered_wid, unsigned int &rendered_hei );
//bool block_filter_scale_layer_old( unsigned int layer_src, int srcx_in, int srcy_in, unsigned int layer_dest, int destx, int desty, float scale_in, unsigned int kernel_size, int bytes_per_pixel_in, int &rendered_wid, int &rendered_hei );
bool block_filter_partial_scale_layer( unsigned int layer_src, int srcx_in, int srcy_in, int wid, int hei, unsigned int layer_dest, float destx, float desty, float scale_in, unsigned int &rendered_wid, unsigned int &rendered_hei );
//bool block_filter_partial_scale_layer_old( unsigned int layer_src, int srcx_in, int srcy_in, int wid, int hei, unsigned int layer_dest, int destx, int desty, float scale_in );
bool block_filter_partial_scale_layer_rect( unsigned int layer_src, st_aa_canvas_bounding_rect_tag &vr, unsigned int layer_dest, float destx, float desty, float scale_in, unsigned int &rendered_wid, unsigned int &rendered_hei );

bool plot_pixel( unsigned int layer, int xx, int yy, int rr, int gg, int bb );
bool plot_pixel_transparent( unsigned int layer, int xx, int yy, int rr, int gg, int bb );
bool plot_pixel_transparent_clamp( unsigned int layer, int xx, int yy, int rr, int gg, int bb );
bool plot_pixel_multiple( unsigned int layer, int xx, int yy, int rr, int gg, int bb, int num_pixels );
bool plot_pixel_multiple_with_rect( unsigned int layer, int xx, int yy, int num_pixels, int rr, int gg, int bb, int rect_size, vector<st_aa_canvas_bounding_rect_tag> &vr );

bool set_pixel9_layer( unsigned int layer, int xx, int yy, int rr, int gg, int bb );
bool set_pixel25_layer( unsigned int layer, int xx, int yy, int rr, int gg, int bb, bool b_cache_pixels );
//bool pixel_filter( unsigned int layer_src, unsigned int layer_dest, unsigned int idx, float scale_in, unsigned int kernel_size );

bool plot_line_25( unsigned int layer, int x1, int y1, int x2, int y2, int rr, int gg, int bb, int linedx, bool b_cache_pixels );
bool plot_line( unsigned int layer, int x1, int y1, int x2, int y2, int rr, int gg, int bb );

bool plot_line_thick( unsigned int layer, int x1, int y1, int x2, int y2, int rr, int gg, int bb, int num_pixels );
bool plot_line_thick_bound_rect( unsigned int layer, int x1, int y1, int x2, int y2, int rr, int gg, int bb, int num_pixels, int rect_size, vector<st_aa_canvas_bounding_rect_tag> &vr );

bool plot_line_transparent( unsigned int layer, int x1, int y1, int x2, int y2, int rr, int gg, int bb );
bool plot_line_transparent_clamp( unsigned int layer, int x1, int y1, int x2, int y2, int rr, int gg, int bb );

bool plot_rect( unsigned int layer, st_aa_canvas_bounding_rect_tag &orect, int rr, int gg, int bb );
bool plot_rect_using_coords( unsigned int layer, int x0, int y0, int ww, int hh, int rr, int gg, int bb );
bool plot_rect_transparent_clamp( unsigned int layer, int x0, int y0, int ww, int hh, int rr, int gg, int bb );

//bool plot_region( unsigned int layer, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, int rr, int gg, int bb );
bool filter_boundary_gen( unsigned int layer, int* x0, int *x1, int lim_idx0, int lim_idx1, int kernel_size );
bool plot_polygon_thick_with_rect( unsigned int layer, st_aa_canvas_polygon_tag &op, int offsx, int offsy, float scale, int num_pixels, int rr, int gg, int bb, int rect_size, bool one_rect_only, vector<st_aa_canvas_bounding_rect_tag> &vr );
bool plot_polygon_filled( unsigned int layer, st_aa_canvas_polygon_tag &op, int offsx, int offsy, float scale, int rr, int gg, int bb );
bool polygon_file_load( string sfname, st_aa_canvas_polygon_tag &op, float scle, float rotate_theta, bool bkeep_coords_positive, bool binvert_x, bool binvert_y, int rr, int gg, int bb, int rect_oversizex, int rect_oversizey );
void polygon_copy( st_aa_canvas_polygon_tag &op_src, st_aa_canvas_polygon_tag &op_dest );
void polygon_rotate( st_aa_canvas_polygon_tag &op, float rotate_theta, float scle, int cntr_x, int cntr_y, int oversizex, int oversizey );
void rect_adj( st_aa_canvas_rect_tag &orct, int delta_posx, int delta_posy, int delta_ww, int delta_hh );
float bitmap_sinx_on_x(float x);
bool plot_arc_thick_rect( unsigned int layer, int px, int py, float radius, float start_ang, float stop_ang, int segments, int num_pixels, int rect_size, bool one_rect_only, int rr, int gg, int bb, vector<st_aa_canvas_bounding_rect_tag> &vr );
void ploygon_arc_build(  int px, int py, float radius, float start_ang, float stop_ang, int segments, int rect_oversizex, int rect_oversizey, st_aa_canvas_polygon_tag &op );
void polygon_update_rect( st_aa_canvas_polygon_tag &op, int rect_oversizex, int rect_oversizey );
void polygon_get_bounding_rect_absolute( st_aa_canvas_polygon_tag &op, int rect_oversizex, int rect_oversizey, st_aa_canvas_bounding_rect_tag &orect );

private:
void set_pixel_internal( unsigned char *bf, int xx, int yy, int rr, int gg, int bb, int linedx, int bytes_per_pixel );

//bool outline_build( unsigned int layer );


};


#endif 

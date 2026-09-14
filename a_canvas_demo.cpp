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



//a_canvas_demo.cpp

//v1.01		25-jul-2023		//

#include "a_canvas_demo.h"






aa_canvas *cnvs;


cl_a_canvas_demo::cl_a_canvas_demo( int xx, int yy, int wid, int hei, const char *label ) : Fl_Double_Window( xx, yy, wid ,hei, label )
{
//bx_image = new Fl_Box( 2, 2, w() - 4, h() - 4  );

ctrl_key = shift_key = 0;

idbg0 = idbg1 = 0;
idbg2 = idbg3 = 0;

timer_avg = 0.0f;
timer_avg_cnt = 0;

bmp0 = 0;
bmp1 = 0;


Fl_Scroll *scl_bmp0 = new Fl_Scroll( 1, 1, w() - 4, h()/2 - 2, "" );
	bx_image0 = new Fl_Box( 2, 2, w() - 4, h() - 4 );
scl_bmp0->end();
	
bx_image1 = new Fl_Box( 2,  h()/2 + 2 , w() - 4, h()/2 - 4 );
bx_image1->box( FL_BORDER_BOX );




cnvs = new aa_canvas();												//make antialias canvas obj

layer0 = 0;											//assign some unique layer ids, must be less than 'cn_aa_canvas_layer_max'
layer1 = 1;
layer2 = 2;
layer3 = 3;

bytes_per_pixel = cn_bytes_per_pixel;
unsigned int linedx;
vector<st_aa_canvas_bounding_rect_tag> vr;

string fname = "zzznice_meter0.bmp";

scale_factor = 0.25;
kern_size = 15;
float scle0 = scale_factor;
theta_hr_hand = 0.0f;

en_aa_filter_kernal_type filter_kernel_type = en_afkt_blackman_harris;

cnvs->create_layer_from_bmp_file( fname, layer0, filter_kernel_type, kern_size );	//load an rgb bitmap file and create suitable layer to hold it, this is the master background copy (fullsize)

cnvs->block_duplicate( layer0, layer1 );											//copy to layer 1 as a working copy, clock hands will be drawn to this layer

cnvs->get_layer_details( layer0, bkgd_ww, bkgd_hh, linedx, bytes_per_pixel );

printf("aa_wnd::aa_wnd() - Image dimensions w %d  h %d  depth %d  linedx %d\n", bkgd_ww, bkgd_hh, bytes_per_pixel, linedx );


//	jpg = new Fl_JPEG_Image( "s-meter2.jpg" );      // load jpeg image into ram
//	jpg = new Fl_JPEG_Image( "zzznice_meter0.jpg" );      // load jpeg image into ram
//	printf("aa_wnd::aa_wnd() - Image dimensions w %d  h %d  depth %d  ld %d\n", jpg->w(), jpg->h(), jpg->d(), jpg->ld() );

//	int ww = jpg->w();
//	int hh = jpg->h();
//	int bytes_per_pixel = jpg->d();


bf0 = new unsigned char[ bkgd_ww * bkgd_hh * bytes_per_pixel ];
bf1 = new unsigned char[ bkgd_ww * bkgd_hh * bytes_per_pixel ];

//bmp0 = new Fl_RGB_Image( (unsigned char*)bf0, bkgd_ww, bkgd_hh, bytes_per_pixel );





cnvs->create_layer( layer1, bkgd_ww, bkgd_hh, bytes_per_pixel, filter_kernel_type, kern_size );				//alloc mem for layers, some layers will have more memory alloc than is actually used, as they hold scaled down bitmaps
cnvs->create_layer( layer2, bkgd_ww, bkgd_hh, bytes_per_pixel, filter_kernel_type, kern_size );
cnvs->create_layer( layer3, bkgd_ww, bkgd_hh, bytes_per_pixel, filter_kernel_type, kern_size );






bool bshow_bound_rect = 1;


int srcx_in = 0;
int srcy_in = 0;
int destx = 0;
int desty = 0;

unsigned int rendered_wid;
unsigned int rendered_hei;
unsigned int dest_wid, dest_hei;


linedx = bkgd_ww;
cnvs->set_layer_details( layer2, bkgd_ww*scle0, bkgd_hh*scle0, linedx*scle0, bytes_per_pixel );


cnvs->block_filter_scale_layer( layer0, srcx_in, srcy_in, layer2, destx, desty, scle0, bytes_per_pixel, dwnsmpl_wid, dwnsmpl_hei );	//downsample to layer2
//cnvs->block_filter_partial_scale_layer( layer0, srcx_in, srcy_in, bkgd_ww, bkgd_hh, layer2, destx, desty, scle0, rendered_wid, rendered_hei );




cnvs->block_duplicate( layer2, layer3 );								//copy downsampled master to layer3 as a working copy 


//cnvs->get_layer_details( layer0, rendered_wid, rendered_hei, linedx, bytes_per_pixel );


unsigned int filtered_wid = rendered_wid;
unsigned int filtered_hei = rendered_hei;








//---- load clock hands
//string sname = "fancy_clock_hands_pnt00.txt";
string sname = "fancy_clock_hands_simpler_pnt00.txt";
//	string sname = "gnomon3_point.txt";
poly0.posx = 250;
poly0.posy = 200;

float poly_scle = 0.5f;
int bkeep_coords_positive = 1;
int binvert_x = 0;
int binvert_y = 1;
float initial_theta = 0;//pi/4;
int cntr_x = 0;
int cntr_y = 0;

if( !cnvs->polygon_file_load( sname, poly0, poly_scle, initial_theta, bkeep_coords_positive, binvert_x, binvert_y, 255, 255, 255, 0, 0 ) )
	{
	printf("failed to load polygon file '%s'\n", sname.c_str() );
	}
else{
	}

//---- 



//---- plot clock hands

	cnvs->polygon_copy( poly0, poly1 );		

	printf("poly0 %d %d\n", poly1.cntr_x, poly1.cntr_y );


//	float dest_fx = (poly1.rectx + poly1.posx)*scle0;					//the scale downed dest pos which keep fractional part as well
//	float dest_fy = (poly1.recty + poly1.posy)*scle0;

//	st_aa_canvas_rect_tag orct;


//	orct.x0 = px;
//	orct.y0 = py;
//	orct.ww = poly1.ww;
//	orct.hh = poly1.hh;

//	cnvs->rect_adj( orct, -7, -7, 7+7, 7+7 );

float scle1 = 1.0f;

//cnvs->polygon_rotate( poly1, theta_hr_hand, scle1, poly1.cntr_x, poly1.cntr_y, kern_size/2, kern_size/2  );



//cnvs->plot_polygon_filled( layer1, poly1, 0, 0, 1.0, 0, 0, 0 );

st_aa_canvas_bounding_rect_tag orct0;
//cnvs->polygon_get_bounding_rect_absolute( poly1, 0, 0, orct0 );

//	float fx2 = orct.x0*scle0;				//scale down dest pos keeping fractional part as well
//	float fy2 = orct.y0*scle0;

//cnvs->block_filter_partial_scale_layer( layer1, orct.x0, orct.y0, orct.ww, orct.hh, layer3, fx2, fy2, scle0, dest_wid, dest_hei );
//cnvs->block_filter_partial_scale_layer_rect( layer1, orct0, layer3, dest_fx, dest_fy, scle0, dest_wid, dest_hei );


//if( bshow_bound_rect )
//	{
//	cnvs->plot_rect_using_coords( layer1, orct0.x0, orct0.y0, orct0.x1 - orct0.x0, orct0.y1 - orct0.y0, 100, 100, 255 );
//	}

//---- 

//cnvs->block_get( layer1, bf0, 0, 0, bkgd_ww, bkgd_hh );

//bx_image0->size( bkgd_ww, bkgd_hh );
//bx_image0->image( bmp0 );


//cnvs->block_get( layer3, bf1, 0, 0, filtered_wid, filtered_hei );		//move layer2 pixels into bf1

//bmp1 = new Fl_RGB_Image( (unsigned char*)bf1, rendered_wid, rendered_hei, bytes_per_pixel );
//bx_image1->size( rendered_wid, rendered_hei );
//bx_image1->image( bmp1 );
//bx_image1->position( 300, h()/2 + 100 );

}














void cl_a_canvas_demo::tick( float dt )
{
mtimer.time_start(mtimer.ns_tim_start);



bool bshow_bound_rect = 1;
	
int destx = 0;
int desty = 0;
unsigned int dest_wid, dest_hei;

vector<st_aa_canvas_bounding_rect_tag> vr;


cnvs->block_duplicate( layer0, layer1 );								//refresh working copy from master (fullsize) 
cnvs->block_duplicate( layer2, layer3 );								//refresh working copy from master (downsampled) 


poly0.posx = 250 + idbg0;
poly0.posy = 200 + idbg1;

cnvs->polygon_copy( poly0, poly1 );										//reload the working copy	

//printf("cl_a_canvas_demo::tick() - idbg0 %d %d\n", idbg0, idbg1 );

float scle0 = scale_factor;




//printf("poly1.cntr_x %d %d\n", poly1.cntr_x, poly1.cntr_y );


float scle1 = 1.0f;

theta_hr_hand += twopi*0.01f;
cnvs->polygon_rotate( poly1, theta_hr_hand, scle1, poly1.cntr_x+idbg2, poly1.cntr_y+idbg3, kern_size/2, kern_size/2  );



cnvs->plot_polygon_filled( layer1, poly1, 0, 0, 1.0, 0, 0, 0 );

st_aa_canvas_bounding_rect_tag orct0;
cnvs->polygon_get_bounding_rect_absolute( poly1, 0, 0, orct0 );			//rect coords are in actual pixel positions, i.e. not relative to polygon's position


float dest_fx = (poly1.rectx + poly1.posx)*scle0;					//the scale downed dest pos which keep fractional part as well
float dest_fy = (poly1.recty + poly1.posy)*scle0;



//cnvs->block_filter_partial_scale_layer( layer1, orct.x0, orct.y0, orct.ww, orct.hh, layer3, fx2, fy2, scle0, dest_wid, dest_hei );
cnvs->block_filter_partial_scale_layer_rect( layer1, orct0, layer3, dest_fx, dest_fy, scle0, dest_wid, dest_hei );




if( bshow_bound_rect )
	{
	cnvs->plot_rect( layer1, orct0, 100, 100, 255 );
//	cnvs->plot_rect_using_coords( layer1, orct0.x0, orct0.y0, orct0.x1 - orct0.x0, orct0.y1 - orct0.y0, 100, 100, 255 );
	}

//----
float dtime = mtimer.time_passed(mtimer.ns_tim_start);

timer_avg += dtime; 
timer_avg_cnt++;

//printf("cl_a_canvas_demo::tick() - time passed: %f, timer_avg %f\n" , dtime, timer_avg/timer_avg_cnt );
//----

//	printf("rect %d %d %d %d\n", orct0.x0, orct0.y0, orct0.x1 - orct0.x0, orct0.y1 - orct0.y0 );



cnvs->block_get( layer1, bf0, 0, 0, bkgd_ww, bkgd_hh );


bx_image0->image( 0 );
if( bmp0 ) delete bmp0;

bmp0 = new Fl_RGB_Image( (unsigned char*)bf0, bkgd_ww, bkgd_hh, bytes_per_pixel );


bx_image0->image( bmp0 );
bx_image0->size( bkgd_ww, bkgd_hh );
bx_image0->redraw();



//unsigned int filtered_wid = dest_wid;
//unsigned int filtered_hei = dest_hei;

cnvs->block_get( layer3, bf1, 0, 0, dwnsmpl_wid, dwnsmpl_hei );			//move layer3 pixels into bf1



bx_image1->image( 0 );
if( bmp1 ) delete bmp1;
bmp1 = new Fl_RGB_Image( (unsigned char*)bf1, dwnsmpl_wid, dwnsmpl_hei, bytes_per_pixel );


bx_image1->size( dwnsmpl_wid, dwnsmpl_hei );
bx_image1->image( bmp1 );
bx_image1->position( 300, h()/2 + 100 );
bx_image1->redraw();

}















cl_a_canvas_demo::~cl_a_canvas_demo()
{
}












int cl_a_canvas_demo::handle(int e)
{
string s1;
int len;
char *szTmp;
bool need_redraw = 0;
bool dont_pass_on = 0;
int mousewheel;


if ( e == FL_MOUSEWHEEL )
	{
	mousewheel = Fl::event_dy();
    int ii;

	int step = 1;
		
	if( ctrl_key ) step = 2;
	if( shift_key ) step = 5;
	if( ctrl_key & shift_key ) step = 10;

	int step_dir;
	if( mousewheel > 0 ) step_dir = step;
	if( mousewheel < 0 ) step_dir = -step;

	need_redraw = 1;
//	dont_pass_on = 1;
	}


//if ( ( e == FL_KEYDOWN ) || ( e == FL_SHORTCUT ) )	//key pressed? -- removed for v1.19, caused parent app shortcuts to not work?
if ( e == FL_KEYDOWN )									//key pressed?
	{
	int key = Fl::event_key();

	if( ( key == FL_Enter ) | ( key == FL_KP_Enter ) )		                        //is it CR ?
		{
		}
	
	if( ( key == FL_Control_L ) || (  key == FL_Control_R ) ) ctrl_key = 1;
	if( ( key == FL_Shift_L ) || (  key == FL_Shift_R ) ) shift_key = 1;
	


	if( key == FL_Left ) idbg0 -= 1;
	if( key == FL_Right ) idbg0 += 1;
	
	if( key == FL_Up ) idbg1 -= 1;
	if( key == FL_Down ) idbg1 += 1;
	
	
	if( key == 'a' ) idbg2 -= 1;
	if( key == 's' ) idbg2 += 1;

	if( key == 'w' ) idbg3 -= 1;
	if( key == 'z' ) idbg3 += 1;

	need_redraw = 1;
//    dont_pass_on = 1;
	}



if ( e == FL_KEYUP )  				                    //key release?
	{
	int key = Fl::event_key();
	if( ( key == FL_Control_L ) || ( key == FL_Control_R ) ) ctrl_key = 0;
	if( ( key == FL_Shift_L ) || (  key == FL_Shift_R ) ) shift_key = 0;

	need_redraw = 1;
 //   dont_pass_on = 1;
	}


//if ( need_redraw ) redraw();

if( dont_pass_on ) return 1;

return Fl_Double_Window::handle(e);
}

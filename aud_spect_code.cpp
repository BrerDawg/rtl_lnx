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


//aud_spect_code.cpp
//v1.01		8-mar-2026			//


#include "aud_spect_code.h"


extern bool filter_iir_notch_adjust( en_filter_idx_tag filt_idx, float freq_cutoff,  float Q_in );


extern rtl_graph_wnd *wnd_rtl_graph;

extern cl_aud_spect_wnd* wnd_aud_spect;


st_audio_spect_avg_tag st_aud_spect_avg;

extern st_filter_sdr_tag st_filt[cn_filters_max];
extern st_filt_bode_tag st_bode[ cn_filters_max ];

st_aud_notch_gui_ctrls_tag st_aud_notch_gui_ctrls[  cn_aud_spect_notch_max  ];














void cb_aud_notch_gui_gainy_posy( Fl_Widget *w, void *v )
{
int which = (intptr_t)v;

My_Input_Wheel *miw = (My_Input_Wheel*)w;
	

if( which == 0 )
	{
	wnd_aud_spect->gainy = miw->get_value_as_double();
	}


if( which == 1 )
	{
	wnd_aud_spect->posy = miw->get_value_as_double();
	}
}

















void cl_aud_spect_wnd::update_gph_user_obj( int srate_in )
{
string s1, st;
mystr m1;

st_mgraph_draw_obj_tag mdo;

gph->vdrwobj.clear();


//int ww, hh;
//gph->border_left;

int ww, hh;
gph->get_background_dimensions( ww, hh );

mdo.type = (en_mgraph_draw_obj_type)en_dobt_polygon;



mdo.visible = 1;
mdo.draw_ordering = 2;				//draw before graticle, 0 = before grid,  1 = after graticle but before traces,   2 = after traces

mdo.use_pos_x = -1;					//if not set to -1: then use this trace id to get a specific x position
mdo.use_scale_x = -1;				//if not set to -1: then use this trace id to get a specific x scale

mdo.use_pos_y = -1;                 //if not set to -1: then use this trace id to get a specific y position
mdo.use_scale_y = -1;               //if not set to -1: then use this trace id to get a specific y scale

mdo.clip_left = 0;					//same as background
mdo.clip_right = 0;
mdo.clip_top = 0;
mdo.clip_bottom = 0;

//strpf( s1, "press h, x:%f y:%.2f", 1.234, 5.678 );

mdo.arc1 = 0;
mdo.arc2 = 0;
mdo.stext = "";
mdo.r = 200;
mdo.g = 200;
mdo.b = 200;
mdo.justify = en_tj_none;
mdo.font = 4;
mdo.font_size = 10;
mdo.line_style = (en_mgraph_line_style)FL_SOLID;
mdo.line_thick = 1;


//mdo.vpolyx.push_back( 10 );   mdo.vpolyy.push_back( 20 );
//mdo.vpolyx.push_back( 30 );   mdo.vpolyy.push_back( 50 );
//mdo.vpolyx.push_back( 5 );   mdo.vpolyy.push_back( 80 );

int yoffs = hh - 12;


mdo.vpolyx.clear();
mdo.vpolyy.clear();

mdo.vpolyx.push_back( 0 );			mdo.vpolyy.push_back( yoffs);
mdo.vpolyx.push_back( ww );			mdo.vpolyy.push_back( yoffs );
gph->vdrwobj.push_back( mdo );


string snum, sunits, scombined;
int fractional_digits = 1;




int xoffs = gph->bkgd_border_left;

int tick_cnt = 10;
int step = ww / tick_cnt;
for( int i = 0; i < tick_cnt; i++ )
	{

	//---- horiz ticks ----
	mdo.type = (en_mgraph_draw_obj_type)en_dobt_polygon;
	int xx = i*step;
	mdo.vpolyx.clear();
	mdo.vpolyy.clear();

	mdo.vpolyx.push_back( xoffs + xx );			mdo.vpolyy.push_back( yoffs );
	mdo.vpolyx.push_back( xoffs + xx );			mdo.vpolyy.push_back( yoffs+2 );

	gph->vdrwobj.push_back( mdo );
	//--------------------


	//------- tick num --------
	mdo.type = (en_mgraph_draw_obj_type)en_dobt_text;
	mdo.x1 = xoffs + i*step;
	mdo.y1 = yoffs+11;
	mdo.vpolyy.clear();

	mdo.justify = en_tj_horiz_center;
	mdo.font = 4;
	mdo.font_size = 10;

//void mystr::make_engineering_str( string &snum, string &sunits, string &scombined, int fractional_digits, double dvalue, string sappend_num, string sappend_units )

//	strpf( s1, "%.1fK", (float)i*0.001 );
	float frq = 0.0f;
	if( i != 0 )
		{
		frq = (srate_in/2.0f) * ((float)i/tick_cnt);
		}
	m1.make_engineering_str( snum, sunits, scombined, fractional_digits, frq, "", "" );
	strpf( s1, "%s", scombined.c_str(), "" );

//	if( i == 1 ) printf( "i == 1: %f\n", frq );

	if( i == 0 ) 
		{
		s1 = "0";
		mdo.x1 += 2;				//move '0' Hz text away from left edge
		}
	mdo.stext = s1;

	gph->vdrwobj.push_back( mdo );

	//--------------------
	}




for( int i = 0; i < cn_aud_spect_notch_max; i++ )
	{

	int filt_idx = en_ftid_iir_notch0_aud;
	if( i == 1 ) filt_idx = en_ftid_iir_notch1_aud;
	
//	st_aud_notch_gui_ctrls_tag oa;
//	oa = st_aud_notch_gui_ctrls[ filt_idx ];


	st_filter_sdr_tag *of = st_filt + filt_idx;

	if( of->flags & en_fflg_on )
		{
		//--- vertical needle  ----
		mdo.type = (en_mgraph_draw_obj_type)en_dobt_polygon;
		mdo.draw_ordering = 1;				//draw before graticle, 0 = before grid,  1 = after graticle but before traces,   2 = after traces
		mdo.use_scale_x = 0;               	//if not set to -1: then use this trace id to get a specific x scale
		mdo.use_scale_y = -1;               //if not set to -1: then use this trace id to get a specific x scale

		mdo.clip_left = 0;					//same as background
		mdo.clip_right = 0;
		mdo.clip_top = 5;
		mdo.clip_bottom = 9;


		if( i == wnd_aud_spect->notch_sel_idx ) 
			{
			mdo.line_style = (en_mgraph_line_style)FL_SOLID;

			mdo.r = 70;
			mdo.g = 180;
			mdo.b = 70;
			}
		else{
			mdo.line_style = (en_mgraph_line_style)FL_DOT;

			mdo.r = 70;
			mdo.g = 150;
			mdo.b = 70;
			}

		mdo.vpolyx.clear();
		mdo.vpolyy.clear();
		
		float fneedle_x = of->fc0;

		mdo.vpolyx.push_back( fneedle_x );			mdo.vpolyy.push_back( 0 );
		mdo.vpolyx.push_back( fneedle_x );   		mdo.vpolyy.push_back( hh );

		gph->vdrwobj.push_back( mdo );
		//------------------
		}




	//------bode response ------
	if( of->flags & en_fflg_on )										//refer 'filter_build_bode()'
		{
		st_filt_bode_tag *ob = st_bode + i;
		ob->vpx.clear();
		ob->vpy.clear();

		mdo.vpolyx.clear();
		mdo.vpolyy.clear();
		
		mdo.clip_left = 0;
		mdo.clip_right = 0;
		mdo.clip_top = 5;
		mdo.clip_bottom = 9;
		
		mdo.line_thick = 1;

//		mdo.use_scale_x = -1;               	//if not set to -1: then use this trace id to get a specific x scale
		
		if( i == wnd_aud_spect->notch_sel_idx ) 
			{
			mdo.line_style = (en_mgraph_line_style)FL_SOLID;

			mdo.r = 180;
			mdo.g = 180;
			mdo.b = 70;
			}
		else{
			mdo.line_style = (en_mgraph_line_style)FL_DOT;

			mdo.r = 140;
			mdo.g = 140;
			mdo.b = 70;
			}

//		double xmin, xmax, ymin, ymax;
//		gph->get_trace_min_max( 0, xmin, xmax, ymin, ymax );
//		float hz_per_pixelx = xmax / ww;

		for( int i = 0; i < ob->vx.size(); i++ )
			{

//printf( "cl_aud_spect_wnd::update_gph_user_obj( ) - minx %f %f\n", xmin, xmax );


//			float fx = ob->vx[i] / twopi * (of->srate);					//hz
			float fx = ob->vx[i];// / twopi * (of->srate);					//hz
//			fx /= hz_per_pixelx;										//conv to pixels

			float fy = (hh-10) - ob->vy[i] * (hh-10);


//strpf( s1, "%f %f\n", fx, fy );
//st += s1;


	//if( i == 1 ) printf("%03d: vx %f   fx %f  %f\n", i, ob->vx[i], fx, fy );		
	//if( i == 20 )printf("  %03d: vx %f   fx %f  %f\n", i, ob->vx[i], fx, fy );		
	//if( i == 63 )printf("    %03d: vx %f   fx %f  %f\n", i, ob->vx[i], fx, fy );	
	//		ob->vpx.push_back( px );
	//		ob->vpy.push_back( 20 );


			//--- line seg  ----
			mdo.type = (en_mgraph_draw_obj_type)en_dobt_polyline;
			
			mdo.draw_ordering = 1;				//draw before graticle, 0 = before grid,  1 = after graticle but before traces,   2 = after traces
			mdo.use_scale_x = 0;               //if not set to -1: then use this trace id to get a specific x scale
			mdo.use_scale_y = -1;               //if not set to -1: then use this trace id to get a specific x scale

			
			mdo.vpolyx.push_back( fx );						mdo.vpolyy.push_back( fy );
	//		mdo.vpolyx.push_back( 600 );					mdo.vpolyy.push_back( 20 );

			//------------------
			}

		gph->vdrwobj.push_back( mdo );
		}

//m1 = st;
//m1.writefile( "zzznotchplot.txt" );


	//show letter 'Q' and bar if req	
	if( i == wnd_aud_spect->notch_sel_idx ) 
		{
		if( wnd_aud_spect->b_Q_adj )
			{
			//letter 'Q'
			mdo.type = (en_mgraph_draw_obj_type)en_dobt_text;
			mdo.draw_ordering = 1;				//draw before graticle, 0 = before grid,  1 = after graticle but before traces,   2 = after traces
			mdo.use_scale_x = -1;               	//if not set to -1: then use this trace id to get a specific x scale
			mdo.use_scale_y = -1;               //if not set to -1: then use this trace id to get a specific x scale

			mdo.clip_left = 0;
			mdo.clip_right = 0;
			mdo.clip_top = 0;
			mdo.clip_bottom = 9;

			mdo.justify = en_tj_horiz_center;
			mdo.font = 4;
			mdo.font_size = 14;

			mdo.stext = "Q";

			mdo.x1 = ww - 20;
			mdo.x2 = mdo.x1;
			
			mdo.y1 = 13;
			mdo.y2 = mdo.y1;	
			mdo.line_style = (en_mgraph_line_style)FL_SOLID;

			mdo.r = 255;
			mdo.g = 120;
			mdo.b = 120;

			gph->vdrwobj.push_back( mdo );



			//bar
			mdo.type = (en_mgraph_draw_obj_type)en_dobt_polyline;
			mdo.line_thick = 2;

			mdo.vpolyx.clear();
			mdo.vpolyy.clear();

			mdo.vpolyx.push_back( 0 );						mdo.vpolyy.push_back( 1 );
			mdo.vpolyx.push_back( ww );						mdo.vpolyy.push_back( 1 );

			gph->vdrwobj.push_back( mdo );
			}
		}
	
	}

}
















void cb_aud_notch_gui( Fl_Widget *w, void *v )
{
unsigned int which = (unsigned int)v;

unsigned int gui_idx = which >> 16;

which = which & 0xffff;

int filt_idx = en_ftid_iir_notch0_aud;
if( gui_idx == 1 ) filt_idx = en_ftid_iir_notch1_aud;


wnd_aud_spect->notch_sel_idx = gui_idx;

st_aud_notch_gui_ctrls_tag oa;
oa = st_aud_notch_gui_ctrls[ gui_idx ]; 


for( int i = 0; i < cn_aud_spect_notch_max; i++ )						//clear all radio btns
	{
	st_aud_notch_gui_ctrls[ i ].bt_sel->clear();
	}


//printf( "UUUUUUUUUUUUUUUUUUUUUUUUUUU cb_aud_notch_gui() - gui_idx %d  which %d\n", gui_idx, which );

if( which == 0 )
	{
	Fl_Round_Button *bt = (Fl_Round_Button*)w;
	
	oa.bt_sel->value(1);
	
//	for( int i = 0; i < cn_aud_spect_notch_max; i++ )
//		{
//		st_aud_notch_gui_ctrls_tag ooa;
//		ooa = st_aud_notch_gui_ctrls[ i ];
		
		
//		if( i == gui_idx )
//			{
//			ooa.bt_sel->value(1); 										//toggle on the req radio button
//			wnd_aud_spect->notch_sel_idx = i;
//			}
		
//		st_aud_notch_gui_ctrls[ i ] = ooa;
//		}
	
//	st_aud_notch_gui_ctrls[i].bt_sel = notch_sel_idx;	
	
//	notch_sel_idx	
	}
	

/*
if( which == 1 )
	{
	for( int i = 0; i < cn_aud_spect_notch_max; i++ )
		{
		
		}
	}
*/


if( which == 1 )
	{
	Fl_Check_Button *bt = (Fl_Check_Button*)w;
	
	oa.bt_sel->value(1);
	
	if( bt->value() )
		{
		st_aud_notch_gui_ctrls[ gui_idx ].active = 1;
		st_filt[filt_idx].flags |= en_fflg_on;
		}
	else{
		st_aud_notch_gui_ctrls[ gui_idx ].active = 0;
		st_filt[filt_idx].flags &= ~en_fflg_on; 
		}
	}


if( which == 130 )
	{
	My_Input_Wheel *miw = (My_Input_Wheel*)w;
	
	oa.bt_sel->value(1); 
	
	float frq = miw->get_value_as_double();								//cutoff freq

	float Q = st_filt[filt_idx].Q;
	
	filter_iir_notch_adjust( filt_idx,  frq,  Q );
	}


if( which == 131 )
	{
	My_Input_Wheel *miw = (My_Input_Wheel*)w;

	oa.bt_sel->value(1);
		
	float Q = miw->get_value_as_double();								//Q

	float frq = st_filt[filt_idx].fc0;
	
	filter_iir_notch_adjust( filt_idx,  frq,  Q );
	}

}








#define cn_aud_spect_Q_mousewheel_region_y 5




void cb_aud_spect_gph_mousemove( void *o, int int0, int int1, double dble0, double dble1 )
{
//string s1;

int px, py;

wnd_aud_spect->gph->get_mouse_pixel_position_on_background( px, py );


int ii = wnd_aud_spect->notch_sel_idx;

int filt_idx = en_ftid_iir_notch0_aud;
if( ii == 1 ) filt_idx = en_ftid_iir_notch1_aud;


st_aud_notch_gui_ctrls_tag oa;
oa = st_aud_notch_gui_ctrls[ ii ];

if( oa.miw_fc == 0 ) return;											//no controls created as yet?

if( py > cn_aud_spect_Q_mousewheel_region_y )
	{
	wnd_aud_spect->b_Q_adj = 0;
	}
else{
	wnd_aud_spect->b_Q_adj = 1;
	}

wnd_aud_spect->redraw();
}










void cb_aud_spect_gph_mousewheel( void *o, int dir )
{



int px, py;

wnd_aud_spect->gph->get_mouse_pixel_position_on_background( px, py );


int ii = wnd_aud_spect->notch_sel_idx;

int filt_idx = en_ftid_iir_notch0_aud;
if( ii == 1 ) filt_idx = en_ftid_iir_notch1_aud;


st_aud_notch_gui_ctrls_tag oa;
oa = st_aud_notch_gui_ctrls[ ii ];

if( oa.miw_fc == 0 ) return;											//no controls created as yet?

if( py > cn_aud_spect_Q_mousewheel_region_y )
	{
	int frq = oa.miw_fc->get_value_as_double();

	int step = 10 * dir;
	frq = frq + step;													//adj frq
	st_filt[ filt_idx ].fc0 = frq;
	oa.miw_fc->set_value_from_double( frq );
	}
else{
	float Q = oa.miw_Q->get_value_as_double();

	float fstep = 0.2f * dir;
	Q = Q + fstep;														//adj Q

	oa.miw_Q->set_value_from_double( Q );
	Q = oa.miw_Q->get_value_as_double();								//this will enforce value limits

	st_filt[ filt_idx ].Q = Q;
	}


st_aud_notch_gui_ctrls[ ii ] = oa;


filter_iir_notch_adjust( filt_idx,   st_filt[ filt_idx ].fc0,    st_filt[  filt_idx ].Q );

}






void cb_aud_spect_gph_left_click_anywhere_cb( void *o )
{
string s1;



if( !wnd_aud_spect ) return;



double mx, my;

wnd_aud_spect->gph->get_mouse_position_relative_to_trc( 0, mx, my );

printf( "cb_aud_spect_gph_left_click_anywhere_cb() mx %f %f\n", mx, my );

//mtim_graph_left_click.time_start( mtim_graph_left_click.ns_tim_start );	//start timer for release callback 'cb_graph_left_click_release_cb' to check


//mgraph_grabx = wnd_aud_spect->gph->mousex;
//mgraph_graby = wnd_aud_spect->gph->mousey;

//mgraph_freq_tune = wnd_rtl_graph->miwp_tune->miw->get_value_as_double();
//mgraph_freq_sub_tune = wnd_rtl_graph->miw_freq_sub_tune->get_value_as_double();






//wnd_rtl_graph->miw_dbg1->set_value_from_double( mx );


/*

int yy = wnd_aud_spect->gph->y() + wnd_aud_spect->gph->h() + 20;



//once off build of gui controls
for( unsigned int i = 0; i < cn_aud_spect_notch_max; i++ )
	{
	st_aud_notch_gui_ctrls_tag oa;
	oa = st_aud_notch_gui_ctrls[i];
	
	int filt_idx_init = en_ftid_iir_notch0_aud;
	if( i == 1 ) filt_idx_init = en_ftid_iir_notch1_aud;
	
	if( oa.gp == 0 )
		{
		oa.active = 1;
		oa.filt_idx = filt_idx_init;
		
		oa.gp = new Fl_Group( 5 + 85 * i, yy, 80, 80, "gp_notch");

		oa.gp->labelsize(7);
		oa.gp->box( FL_BORDER_BOX );


		if( oa.bt_sel == 0  )
			{
			oa.bt_sel = new Fl_Round_Button( oa.gp->x(), oa.gp->y(), 70, 17, "sel" );
			oa.bt_sel->type(FL_RADIO_BUTTON);
			oa.bt_sel->labelsize(10);
			oa.bt_sel->tooltip("set if this notch filter is the currently adjustable filter on graph" );
			oa.bt_sel->callback( (void*)cb_aud_notch_gui, ((unsigned int) (i<<16) | 0) );			//upper 16 bits are the filter index
			
			if( i == 0 ) oa.bt_sel->value(1);
			}

		if( oa.ck_en == 0  )
			{
			oa.ck_en = new Fl_Check_Button( oa.gp->x(), oa.gp->y() + 20, 80, 15, "enable");
			oa.ck_en->labelsize(10);
			oa.ck_en->tooltip("enable notch filter" );
			oa.ck_en->value( st_filt[i].flags & en_fflg_on );
			
			oa.ck_en->callback( (void*)cb_aud_notch_gui, ((unsigned int) (i<<16) | 1) );			//upper 16 bits are the gui ctrl index
			}

		if( oa.miw_fc == 0  )
			{
			My_Input_Wheel *omiw = new My_Input_Wheel( oa.gp->x() + 20, oa.gp->y() + 40, 45, 15, "fc" );
			oa.miw_fc = omiw;
			omiw->labelsize(10);
			omiw->textsize(9);
			omiw->align(FL_ALIGN_LEFT);
			omiw->tooltip( "notch filter cutoff freq" );
			omiw->b_show_modified = 1;
			omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
			omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
			omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
			omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
			omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
			omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
			omiw->col_update();
			omiw->b_take_focus_on_inside = 0;
			omiw->b_use_limit_min = 1;
			omiw->limit_min = 10;
			omiw->b_use_limit_max = 1;
			omiw->limit_max = 96000;
			omiw->b_invert_wheel = 0;
			omiw->std_wheel_step = 25;
			omiw->ctrl_wheel_step = 5;
			omiw->shift_wheel_step = 100;
			omiw->ctrl_shift_wheel_step = 250;

			omiw->s_printf_format = "%d";
			omiw->force_integer = 1;
			omiw->set_value_from_double( st_filt[filt_idx_init].fc0 );
			omiw->set_callback( (void*)cb_aud_notch_gui, omiw, (void*) ((unsigned int) (i<<16) | 130) );	//upper 16 bits are the gui ctrl index
			omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
			omiw->id2 = 1;
			omiw->allow_right_but_drag = 1;
			omiw->right_drag_x_val_change_factor = 0;
			omiw->right_drag_y_val_change_factor = omiw->limit_max / 100.0f;
			}



		if( oa.miw_Q == 0  )
			{
			My_Input_Wheel *omiw = new My_Input_Wheel( oa.gp->x() + 20, oa.gp->y() + 60, 45, 15, "Q" );
			oa.miw_Q = omiw;
			omiw->labelsize(10);
			omiw->textsize(9);
			omiw->align(FL_ALIGN_LEFT);
			omiw->tooltip( "notch filter Q, higher narrows notch and attenuates a smaller band of frequencies" );
			omiw->b_show_modified = 1;
			omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
			omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
			omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
			omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
			omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
			omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
			omiw->col_update();
			omiw->b_take_focus_on_inside = 0;
			omiw->b_use_limit_min = 1;
			omiw->limit_min = 1.01;
			omiw->b_use_limit_max = 1;
			omiw->limit_max = 20.0f;
			omiw->b_invert_wheel = 0;
			omiw->std_wheel_step = 0.25;
			omiw->ctrl_wheel_step = 0.0125;
			omiw->shift_wheel_step = 0.5;
			omiw->ctrl_shift_wheel_step = 1;

			omiw->s_printf_format = "%.4f";
			omiw->force_integer = 0;
			omiw->set_value_from_double( st_filt[filt_idx_init].Q );
			omiw->set_callback( (void*)cb_aud_notch_gui, omiw, (void*) ((unsigned int) (i<<16) | 131) );	//upper 16 bits are the gui ctrl index
			omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
			omiw->id2 = 1;
			omiw->allow_right_but_drag = 1;
			omiw->right_drag_x_val_change_factor = 0;
			omiw->right_drag_y_val_change_factor = omiw->limit_max / 100.0f;
			}

		oa.gp->end();
		
		}


	wnd_aud_spect->remove( oa.gp );
	wnd_aud_spect->add( oa.gp );


	st_aud_notch_gui_ctrls[i] = oa;
	}
*/

//adjust sel filter
st_aud_notch_gui_ctrls_tag oa = st_aud_notch_gui_ctrls[ wnd_aud_spect->notch_sel_idx ];


int filt_idx = en_ftid_iir_notch0_aud;
if( wnd_aud_spect->notch_sel_idx == 1 ) filt_idx = en_ftid_iir_notch1_aud;



if( ( oa.bt_sel->value() ) || ( oa.bt_sel->value() ) )
	{
	st_filt[ filt_idx ].flags |= en_fflg_on;
	oa.ck_en->value( st_filt[ filt_idx ].flags & en_fflg_on );
	
	oa.active = 1;
	}

st_filt[ filt_idx ].fc0 = mx;
oa.miw_fc->set_value_from_double( st_filt[ filt_idx ].fc0 );

oa.miw_Q->set_value_from_double( st_filt[ filt_idx ].Q );


st_aud_notch_gui_ctrls[ wnd_aud_spect->notch_sel_idx ] = oa;

filter_iir_notch_adjust( filt_idx,   st_filt[ filt_idx ].fc0,    st_filt[  filt_idx ].Q );

wnd_aud_spect->redraw();
}








void cl_aud_spect_wnd::plot( unsigned int srate_in, vector<float> &vf0 )
{
int grat_pixels_x = 25;							//pxls per graticule
int grat_pixels_y = 25;

int ii = gph->w();
int graticle_count_x = ii / grat_pixels_x;

ii = gph->h();
int graticle_count_y = ii / grat_pixels_y;

int border = 0;

int wfm_wid = graticle_count_x * grat_pixels_x; //calc size of wnd using graticule details
wfm_wid += 2 * border;

int wfm_hei = graticle_count_y * grat_pixels_y;
wfm_hei += 2 * border;

gph->bkgd_border_left = border;
gph->bkgd_border_top = border;
gph->bkgd_border_right = border;
gph->bkgd_border_bottom = border;

gph->graticule_border_left = border;
gph->graticule_border_top = border;
gph->graticule_border_right = border;
gph->graticule_border_bottom = border;

gph->graticle_count_x = graticle_count_x;
gph->grat_pixels_x = grat_pixels_x;

gph->graticle_count_y = graticle_count_y;
gph->grat_pixels_y = grat_pixels_y;
gph->cro_graticle = 1;

gph->col_graticle_center.r = gph->col_graticle.r;
gph->col_graticle_center.g = gph->col_graticle.g;
gph->col_graticle_center.b = gph->col_graticle.b;

mg_col_tag col;
trace_tag tr1;


col.r = 150;
col.g = 150;
col.b = 210;



tr1.id = 0;                      //identify which trace this is, helps when traces are push_back'd in unknown order
tr1.vis = 1;
tr1.col = col;
tr1.line_thick = 1;
tr1.lineplot = 1;
tr1.line_style = (en_mgraph_line_style) en_mls_solid;
tr1.border_left = 0;
tr1.border_right = 0;
tr1.border_top = 0;
tr1.border_bottom = 0;

tr1.show_as_spectrum = 0;                           //not a spectra plot
tr1.spectrum_baseline_y = 0;

tr1.b_limit_auto_scale_min_for_y = 0;
tr1.b_limit_auto_scale_max_for_y = 0;

//graticle_count_y = gph->graticle_count_y;					//this should be an even number
//graticle_count_x = gph->graticle_count_x;					//this should be an even number
//grat_pixels_x = gph->grat_pixels_x;						//pxls per graticule
//grat_pixels_y = gph->grat_pixels_y;

tr1.xunits_perpxl = -1;//1.0/grat_pixels_x;
tr1.yunits_perpxl = 0.001;//0.1 / grat_pixels_y;


tr1.posx = 0;
tr1.posy = posy;

tr1.plot_offsx = 0; 								//not affected by override: 'use_pos_y', still allows independent trace offsets at pixel level
tr1.plot_offsy = -20;//15;

tr1.use_pos_y = -1;				//if not -1, use this trace's id as a reference for this val
tr1.use_scale_y = -1;			//if not -1, use this trace's id as a reference for this val


tr1.scalex = 1;
tr1.scaley = 2.0f;

tr1.single_sel_col.r = 255;
tr1.single_sel_col.g = 0;
tr1.single_sel_col.b = 255;

tr1.group_sel_trace_col.r = 230;
tr1.group_sel_trace_col.g = 230;
tr1.group_sel_trace_col.b = 230;

tr1.group_sel_rect_col.r = 255;
tr1.group_sel_rect_col.g = 153;
tr1.group_sel_rect_col.b = 0;

tr1.sample_rect_hints_double_click = 1;

tr1.sample_rect_hints = 1;
tr1.sample_rect_hints_col.r = 255;
tr1.sample_rect_hints_col.g = 120;
tr1.sample_rect_hints_col.b = 150;
tr1.sample_rect_hints_distancex = 12;						//helps stop over hinting on x-axis
tr1.sample_rect_hints_distancey = 0;						//disable over hinting test/clearing for y-axis 

tr1.clip_left = tr1.border_left;
tr1.clip_right = tr1.border_right;
tr1.clip_top = tr1.border_top;
tr1.clip_bottom = tr1.border_bottom;


tr1.sample_rect_flicker = 0;

//tr1.minx = 0;

pnt_tag pnt1;
//int count = 700;





for( int i = 0; i <	vf0.size(); i++ )
	{
	float frq = (srate_in/2.0f) * ( (float)i/vf0.size() );
	pnt1.x = frq;
	pnt1.y = vf0[ i ] * gainy;
	pnt1.sel = 0;
	
	tr1.pnt.push_back( pnt1 );											//load graph points
	}




//printf("update_graph() - vspct.size() %d\n", vspct.size() );

/*
int count = 700;
for( int i = 0; i <	count; i++ )
	{
	pnt1.x = i;

	double theta = 2.0 * M_PI * (double)i / (double)count;

	pnt1.y = 250 * sin( theta );

	tr1.pnt.push_back( pnt1 );						//load graph points
	}
*/



//gph->get_selected_idx( 0, gph0_last_sel_idx );

gph->clear_traces();
gph->add_trace( tr1 );
gph->set_left_click_anywhere_cb( cb_aud_spect_gph_left_click_anywhere_cb, (void*)this );
//gph->set_middle_click_anywhere_cb( cb_graph_middle_click_anywhere_cb, (void*)this );
//gph->set_right_click_anywhere_cb( cb_graph_right_click_anywhere_cb, (void*)this );

//gph->set_left_click_release_cb( cb_graph_left_click_release_cb, (void*)this );

gph->set_mousemove_cb( 0, (void*)cb_aud_spect_gph_mousemove, (void*)this );
gph->set_mousewheel_cb( 0, (void*)cb_aud_spect_gph_mousewheel, (void*)this );
//gph->set_keydown_cb( 0, cb_graph_keydown, this );
//gph->set_keyup_cb( 0, cb_graph_keyup, this );

col.r = 64;
col.g = 64;
col.b = 64;
gph->background = col;


//gph->set_selected_sample( 0, gph0_last_sel_idx, 0 );

//update_gph_add_text();													//this clears 'vdrwobj'

//gph->sample_rect_showing[0] = 1;

update_gph_user_obj( srate_in );

gph->render( -1 );


//if( start_up_state == 4 ) update_gph0_cnt++;

//double x0,y0,x1,y1;
//gph0->get_trace_min_max( 0, x0,x1,y0,y1);


//printf( "update_gph0() - x0, x1 %f %f   y0 y1 %f %f\n",  x0,x1,y0,y1 );
}



















cl_aud_spect_wnd::cl_aud_spect_wnd( int xx, int yy, int ww, int hh, const char *label ) : Fl_Double_Window( xx, yy, ww, hh, label )
{

gph = new mgraph( 2, 2, ww - 4, 100, "" );



gainy = 0.2f;
posy = 0.0f;
notch_sel_idx = 0;
b_Q_adj = 0;


Fl_Group *gp0 = new Fl_Group( ww - 50, gph->h() + 10, 80, 40, "" );

gp0->labelsize(7);
//gp0->box( FL_BORDER_BOX );

My_Input_Wheel *omiw = new My_Input_Wheel( gp0->x() + 1, gp0->y() + 1, 45, 15, "y-gain" );
omiw->labelsize(10);
omiw->textsize(9);
omiw->align(FL_ALIGN_LEFT);
omiw->tooltip( "graph y gain" );
omiw->b_show_modified = 1;
omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
omiw->col_update();
omiw->b_take_focus_on_inside = 0;
omiw->b_use_limit_min = 1;
omiw->limit_min = 0.00001;
omiw->b_use_limit_max = 1;
omiw->limit_max = 100.0;
omiw->b_invert_wheel = 0;
omiw->std_wheel_step = 0.025;
omiw->ctrl_wheel_step = 0.1;
omiw->shift_wheel_step = 0.25;
omiw->ctrl_shift_wheel_step = 2;
omiw->s_printf_format = "%.3f";
omiw->force_integer = 0;
omiw->set_value_from_double( gainy );
omiw->set_callback( (void*)cb_aud_notch_gui_gainy_posy, omiw, (void*) 0 );
omiw->id = 0;															//additional value for callback, useful for matrix arrays of ctrls
omiw->id2 = 1;
omiw->allow_right_but_drag = 1;
omiw->right_drag_x_val_change_factor = 0;
omiw->right_drag_y_val_change_factor = omiw->limit_max / 100.0f;




omiw = new My_Input_Wheel( gp0->x() + 1, gp0->y() + 21, 45, 15, "y-offs" );
omiw->labelsize(10);
omiw->textsize(9);
omiw->align(FL_ALIGN_LEFT);
omiw->tooltip( "graph y offset" );
omiw->b_show_modified = 1;
omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
omiw->col_update();
omiw->b_take_focus_on_inside = 0;
omiw->b_use_limit_min = 1;
omiw->limit_min = -100;
omiw->b_use_limit_max = 1;
omiw->limit_max = 100.0;
omiw->b_invert_wheel = 0;
omiw->std_wheel_step = 1;
omiw->ctrl_wheel_step = 2;
omiw->shift_wheel_step = 5;
omiw->ctrl_shift_wheel_step = 10;
omiw->s_printf_format = "%.3f";
omiw->force_integer = 0;
omiw->set_value_from_double( posy );
omiw->set_callback( (void*)cb_aud_notch_gui_gainy_posy, omiw, (void*) 1 );
omiw->id = 0;															//additional value for callback, useful for matrix arrays of ctrls
omiw->id2 = 1;
omiw->allow_right_but_drag = 1;
omiw->right_drag_x_val_change_factor = 0;
omiw->right_drag_y_val_change_factor = omiw->limit_max / 100.0f;

gp0->end();

gp0->resizable( 0 );





int yoffs = gph->h() + 20;



//once off build of gui controls
for( unsigned int i = 0; i < cn_aud_spect_notch_max; i++ )
	{
	st_aud_notch_gui_ctrls_tag oa;
	oa = st_aud_notch_gui_ctrls[i];
	
	int filt_idx_init = en_ftid_iir_notch0_aud;
	if( i == 1 ) filt_idx_init = en_ftid_iir_notch1_aud;
	
//	if( oa.gp == 0 )
		{
		oa.active = 0;
		oa.filt_idx = filt_idx_init;
		
		oa.gp = new Fl_Group( 5 + 85 * i, yoffs, 80, 80, "");
		if( i == 0 )
			{
			oa.gp->label( "gp_notch0" );
			}
		if( i == 1 )
			{
			oa.gp->label( "gp_notch1" );
			}
		
		oa.gp->labelsize(7);
		oa.gp->box( FL_BORDER_BOX );


		if( oa.bt_sel == 0  )
			{
			oa.bt_sel = new Fl_Round_Button( oa.gp->x(), oa.gp->y(), 70, 17, "sel" );
			oa.bt_sel->type(FL_RADIO_BUTTON);
			oa.bt_sel->labelsize(10);
			oa.bt_sel->tooltip("shows if this notch filter is the currently adjustable filter on graph" );
			oa.bt_sel->callback( (void*)cb_aud_notch_gui, ((unsigned int) (i<<16) | 0) );			//upper 16 bits are the filter index
			
			if( i == 0 ) oa.bt_sel->value(1);
			}

		if( oa.ck_en == 0  )
			{
			oa.ck_en = new Fl_Check_Button( oa.gp->x(), oa.gp->y() + 20, 80, 15, "enable");
			oa.ck_en->labelsize(10);
			oa.ck_en->tooltip("enable notch filter" );
			oa.ck_en->value( st_filt[i].flags & en_fflg_on );
			
			oa.ck_en->callback( (void*)cb_aud_notch_gui, ((unsigned int) (i<<16) | 1) );			//upper 16 bits are the gui ctrl index
			}

//		if( oa.miw_fc == 0  )
			{
			My_Input_Wheel *omiw = new My_Input_Wheel( oa.gp->x() + 20, oa.gp->y() + 40, 45, 15, "fc" );
			oa.miw_fc = omiw;
			omiw->labelsize(10);
			omiw->textsize(9);
			omiw->align(FL_ALIGN_LEFT);
			omiw->tooltip( "notch filter cutoff freq" );
			omiw->b_show_modified = 1;
			omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
			omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
			omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
			omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
			omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
			omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
			omiw->col_update();
			omiw->b_take_focus_on_inside = 0;
			omiw->b_use_limit_min = 1;
			omiw->limit_min = 10;
			omiw->b_use_limit_max = 1;
			omiw->limit_max = 96000;
			omiw->b_invert_wheel = 0;
			omiw->std_wheel_step = 25;
			omiw->ctrl_wheel_step = 5;
			omiw->shift_wheel_step = 100;
			omiw->ctrl_shift_wheel_step = 250;

			omiw->s_printf_format = "%d";
			omiw->force_integer = 1;
			omiw->set_value_from_double( st_filt[filt_idx_init].fc0 );
			omiw->set_callback( (void*)cb_aud_notch_gui, omiw, (void*) ((unsigned int) (i<<16) | 130) );	//upper 16 bits are the gui ctrl index
			omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
			omiw->id2 = 1;
			omiw->allow_right_but_drag = 1;
			omiw->right_drag_x_val_change_factor = 0;
			omiw->right_drag_y_val_change_factor = omiw->limit_max / 100.0f;
			}



//		if( oa.miw_Q == 0  )
			{
			My_Input_Wheel *omiw = new My_Input_Wheel( oa.gp->x() + 20, oa.gp->y() + 60, 45, 15, "Q" );
			oa.miw_Q = omiw;
			omiw->labelsize(10);
			omiw->textsize(9);
			omiw->align(FL_ALIGN_LEFT);
			omiw->tooltip( "notch filter Q, higher narrows notch and attenuates a smaller band of frequencies" );
			omiw->b_show_modified = 1;
			omiw->col_bkg = fl_rgb_color( 220, 255, 220 );
			omiw->col_bkg_hover = fl_rgb_color( 200, 255, 200 );
			omiw->col_bkg_focus = fl_rgb_color( 255, 230, 230 );
			omiw->col_bkg_hover_focus = fl_rgb_color( 255, 220, 220 );
			omiw->col_bkg_modified = fl_rgb_color( 255, 180, 180 );
			omiw->col_bkg_hover_modified = fl_rgb_color( 255, 170, 170 );
			omiw->col_update();
			omiw->b_take_focus_on_inside = 0;
			omiw->b_use_limit_min = 1;
			omiw->limit_min = 1.01;
			omiw->b_use_limit_max = 1;
			omiw->limit_max = 20.0f;
			omiw->b_invert_wheel = 0;
			omiw->std_wheel_step = 0.25;
			omiw->ctrl_wheel_step = 0.0125;
			omiw->shift_wheel_step = 0.5;
			omiw->ctrl_shift_wheel_step = 1;

			omiw->s_printf_format = "%.4f";
			omiw->force_integer = 0;
			omiw->set_value_from_double( st_filt[filt_idx_init].Q );
			omiw->set_callback( (void*)cb_aud_notch_gui, omiw, (void*) ((unsigned int) (i<<16) | 131) );	//upper 16 bits are the gui ctrl index
			omiw->id = 0;														//additional value for callback, useful for matrix arrays of ctrls
			omiw->id2 = 1;
			omiw->allow_right_but_drag = 1;
			omiw->right_drag_x_val_change_factor = 0;
			omiw->right_drag_y_val_change_factor = omiw->limit_max / 100.0f;
			}

		oa.gp->end();
		
		oa.gp->resizable( 0 );
		}


//	remove( oa.gp );
//	add( oa.gp );


	st_aud_notch_gui_ctrls[i] = oa;
	}

//redraw();

}


//st_aud_notch_gui_ctrls_tag oa;
//st_aud_notch_gui_ctrls[i]

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


//favourite_code.cpp

//v1.01		13-mar-2025			//



#include "favourite_code.h"



extern cl_favourite_wnd* wnd_fav;
extern vector<st_favourite_freq_tag>vfav;
extern char sz_demodulator_type[16][32];
extern int demod_type_idx_from_str( string ss );
extern string slast_favourite_fname;
extern bool b_sanitise_favourites;
extern rtl_graph_wnd *wnd_rtl_graph;







/*

//----------------------------------------------------------------------

cl_fi_input_click( int xx, int yy, int wid, int hei, const char *label ) : Fl_Input( xx, yy, wid, hei, label )
{
}







int cl_fi_input_click::handle( int e )
{
string s1;
bool need_redraw = 0;
bool dont_pass_on = 0;


if ( e & FL_MOVE )
	{

//	need_redraw = 1;
//    dont_pass_on = 1;
	}

if ( e == FL_PUSH )
	{
	int ii = Fl::event_clicks();

	if( Fl::event_button() == 1 )											//left click
		{
		left_button = 1;
		wnd_fav->set_sel_row( id0 );
		}
	need_redraw = 1;
	}


if ( e == FL_RELEASE )
	{

	if( Fl::event_button() == 1 )											//left click
		{
		left_button = 0;
		}
	need_redraw = 1;
	}


//if ( ( ( e == FL_KEYDOWN ) || ( e == FL_SHORTCUT ) )  )	//key pressed?
if (  ( e == FL_KEYDOWN ) ) 	//key pressed?
	{
	int key = Fl::event_key();

//	int offx, offy;
//	get_scroll_offsets( offx, offy );

	if( ( key == FL_Control_L ) || (  key == FL_Control_R ) )
		{

		}


	if( key == FL_Enter )
		{
		text_changed = 0;
		}
	else{
		bool key_ignore = 0;
		
		if( key == FL_Tab ) key_ignore = 1;
		
		
		if( !key_ignore ) text_changed = 1;
		}
		
	need_redraw = 1;
	}

if ( need_redraw ) redraw();

if( dont_pass_on ) return 1;

return Fl_Input::handle(e);
}
//----------------------------------------------------------------------

*/









//----------------------------------------------------------------------
cl_fav_text_input::cl_fav_text_input( int xx, int yy, int wid, int hei, const char *label ) : Fl_Input( xx, yy, wid, hei, label )
{
left_button = 0;
text_changed = 0;
sel = 0;
b_type_comment = 0;

//Fl_Input::tab_nav( 0 );

}




cl_fav_text_input::~cl_fav_text_input()
{
}










void cl_fav_text_input::draw()
{
int xx = x();
int yy = y();

int iF = fl_font();
int iS = fl_size();


int px, py;

fl_font( labelfont(), labelsize() );

Fl_Input::color( FL_WHITE );

if( id0 == wnd_fav->row_sel )
	{
	Fl_Input::color( fl_rgb_color( 230, 255, 230 ) );
	}
	
if( text_changed )
	{
	Fl_Color col = fl_rgb_color( 255, 230, 230 );
	
	Fl_Input::color( col );
		
//	fl_rectf( x(), y(), w(), h() );
	}

Fl_Input::draw();


Fl_Input::color( FL_WHITE );


//if( left_button ) fl_color( 100, 100, 100 );
//else fl_color( 0, 0, 0 );

fl_font( iF, iS );
}










int cl_fav_text_input::handle( int e )
{
string s1;
bool need_redraw = 0;
bool dont_pass_on = 0;


if ( e & FL_MOVE )
	{

//	need_redraw = 1;
//    dont_pass_on = 1;
	}



if ( e == FL_PUSH )
	{
	int ii = Fl::event_clicks();

	if( Fl::event_button() == 1 )											//left click
		{

		left_button = 1;
		
		wnd_fav->set_sel_row( id0 );
		
		if( b_type_comment ) do_callback();								//at present, only used by 'en_frci_fi_comment0'				
		}
	need_redraw = 1;
	}


if ( e == FL_RELEASE )
	{

	if( Fl::event_button() == 1 )											//left click
		{
		left_button = 0;
		}
	need_redraw = 1;
	}


//if ( ( ( e == FL_KEYDOWN ) || ( e == FL_SHORTCUT ) ) /*& ( bfl_enter )*/ )	//key pressed?
if (  ( e == FL_KEYDOWN ) ) 	//key pressed?
	{
	int key = Fl::event_key();

//	int offx, offy;
//	get_scroll_offsets( offx, offy );

	if( ( key == FL_Control_L ) || (  key == FL_Control_R ) )
		{

		}


	if( key == FL_Enter )
		{
		text_changed = 0;
		}
	else{
		bool key_ignore = 0;
		
		if( key == FL_Tab ) key_ignore = 1;
		
		
		if( !key_ignore ) text_changed = 1;
		}
		
	need_redraw = 1;
	}

if ( need_redraw ) redraw();

if( dont_pass_on ) return 1;

return Fl_Input::handle(e);
}

//----------------------------------------------------------------------


















//--------

//a-->z
int sort_favourite_by_sname_increasing(const st_favourite_freq_tag &o0, const st_favourite_freq_tag &o1 )
{
if( (o0.sname.length() == 0 ) && ( o1.sname.length() == 0) ) return 0;	//both empty
if( o0.sname.length() == 0 ) return 0;
if( o1.sname.length() == 0 ) return 1;

if( o0.sname.compare( o1.sname ) >= 0 ) return 0; 		//return 0 if o0 '>=' o1	
	
return 1;												//return 1 if o0 '<' o1			!! important to get this right for 'stable_sort()'
}


//z-->a ( o0 and o1 are reversed when comparing)
int sort_favourite_by_sname_decreasing(const st_favourite_freq_tag &o0, const st_favourite_freq_tag &o1 )
{
if( (o1.sname.length() == 0 ) && ( o0.sname.length() == 0) ) return 0;	//both empty
if( o1.sname.length() == 0 ) return 0;
if( o0.sname.length() == 0 ) return 1;

if( o1.sname.compare( o0.sname ) >= 0 ) return 0; 		//return 0 if o1 '>=' o0	
	
return 1;												//return 1 if o1 '<' o0			!! important to get this right for 'stable_sort()'
}
//--------




//--------
//a-->z
int sort_favourite_by_sgroup_increasing(const st_favourite_freq_tag &o0, const st_favourite_freq_tag &o1 )
{
if( (o0.sgroup.length() == 0 ) && ( o1.sgroup.length() == 0) ) return 0;	//both empty
if( o0.sgroup.length() == 0 ) return 0;
if( o1.sgroup.length() == 0 ) return 1;

if( o0.sgroup.compare( o1.sgroup ) >= 0 ) return 0; 	//return 0 if o0 '>=' o1	

return 1;												//return 1 if o0 '<' o1			!! important to get this right for 'stable_sort()'
}


//z-->a ( o0 and o1 are reversed when comparing)
int sort_favourite_by_sgroup_decreasing(const st_favourite_freq_tag &o0, const st_favourite_freq_tag &o1 )
{
if( (o1.sgroup.length() == 0 ) && ( o0.sgroup.length() == 0) ) return 0;	//both empty
if( o1.sgroup.length() == 0 ) return 0;
if( o0.sgroup.length() == 0 ) return 1;

if( o1.sgroup.compare( o0.sgroup ) >= 0 ) return 0; 	//return 0 if o1 '>=' o0	
	
return 1;												//return 1 if o1 '<' o0			!! important to get this right for 'stable_sort()'
}
//--------












//--------
//0-->9
int sort_favourite_by_freq_increasing(const st_favourite_freq_tag &o0, const st_favourite_freq_tag &o1 )
{

if( o0.freq >= o1.freq ) return 0; 						//return 0 if o0 '>=' o1	

return 1;												//return 1 if o0 '<' o1			!! important to get this right for 'stable_sort()'
}


//9-->0 ( o0 and o1 are reversed when comparing)
int sort_favourite_by_freq_decreasing(const st_favourite_freq_tag &o0, const st_favourite_freq_tag &o1 )
{

if( o1.freq >= o0.freq ) return 0; 						//return 0 if o1 '>=' o0	
	
return 1;												//return 1 if o1 '<' o0			!! important to get this right for 'stable_sort()'
}
//--------









//--------
//0-->9
int sort_favourite_by_freq_center_increasing(const st_favourite_freq_tag &o0, const st_favourite_freq_tag &o1 )
{

if( o0.freq_center >= o1.freq_center ) return 0; 		//return 0 if o0 '>=' o1	

return 1;												//return 1 if o0 '<' o1			!! important to get this right for 'stable_sort()'
}


//9-->0 (o0 and o1 are reversed when comparing)
int sort_favourite_by_freq_center_decreasing(const st_favourite_freq_tag &o0, const st_favourite_freq_tag &o1 )
{

if( o1.freq_center >= o0.freq_center ) return 0; 		//return 0 if o1 '>=' o0	
	
return 1;												//return 1 if o1 '<' o0			!! important to get this right for 'stable_sort()'
}
//--------






//--------
//0-->9
int sort_favourite_by_demod_type_increasing(const st_favourite_freq_tag &o0, const st_favourite_freq_tag &o1 )
{

if( o0.demod_type >= o1.demod_type ) return 0; 					//return 0 if o0 '>=' o1	

return 1;														//return 1 if o0 '<' o1			!! important to get this right for 'stable_sort()'
}


//9-->0 ( o0 and o1 are reversed when comparing)
int sort_favourite_by_demod_type_decreasing(const st_favourite_freq_tag &o0, const st_favourite_freq_tag &o1 )
{

if( o1.demod_type >= o0.demod_type ) return 0; 					//return 0 if o1 '>=' o0	
	
return 1;														//return 1 if o1 '<' o0			!! important to get this right for 'stable_sort()'
}
//--------








//--------

//a-->z
int sort_favourite_by_sdev_manufacturer_increasing(const st_favourite_freq_tag &o0, const st_favourite_freq_tag &o1 )
{
if( (o0.sdev_manufacturer.length() == 0 ) && ( o1.sdev_manufacturer.length() == 0) ) return 0;	//both empty
if( o0.sdev_manufacturer.length() == 0 ) return 0;
if( o1.sdev_manufacturer.length() == 0 ) return 1;

if( o0.sdev_manufacturer.compare( o1.sdev_manufacturer ) >= 0 ) return 0; 				//return 0 if o0 '>=' o1	
	
return 1;																//return 1 if o0 '<' o1			!! important to get this right for 'stable_sort()'
}


//z-->a ( o0 and o1 are reversed when comparing)
int sort_favourite_by_sdev_manufacturer_decreasing(const st_favourite_freq_tag &o0, const st_favourite_freq_tag &o1 )
{
if( (o1.sdev_manufacturer.length() == 0 ) && ( o0.sdev_manufacturer.length() == 0) ) return 0;	//both empty
if( o1.sdev_manufacturer.length() == 0 ) return 0;
if( o0.sdev_manufacturer.length() == 0 ) return 1;

if( o1.sdev_manufacturer.compare( o0.sdev_manufacturer ) >= 0 ) return 0; 				//return 0 if o1 '>=' o0	
	
return 1;																//return 1 if o1 '<' o0			!! important to get this right for 'stable_sort()'
}
//--------





//--------

//a-->z
int sort_favourite_by_sdate_increasing(const st_favourite_freq_tag &o0, const st_favourite_freq_tag &o1 )
{
if( (o0.sdate.length() == 0 ) && ( o1.sdate.length() == 0) ) return 0;	//both empty
if( o0.sdate.length() == 0 ) return 0;
if( o1.sdate.length() == 0 ) return 1;

if( o0.sdate.compare( o1.sdate ) >= 0 ) return 0; 						//return 0 if o0 '>=' o1	
	
return 1;																//return 1 if o0 '<' o1			!! important to get this right for 'stable_sort()'
}


//z-->a ( o0 and o1 are reversed when comparing)
int sort_favourite_by_sdate_decreasing(const st_favourite_freq_tag &o0, const st_favourite_freq_tag &o1 )
{
if( (o1.sdate.length() == 0 ) && ( o0.sdate.length() == 0) ) return 0;	//both empty
if( o1.sdate.length() == 0 ) return 0;
if( o0.sdate.length() == 0 ) return 1;

if( o1.sdate.compare( o0.sdate ) >= 0 ) return 0; 						//return 0 if o1 '>=' o0	
	
return 1;																//return 1 if o1 '<' o0			!! important to get this right for 'stable_sort()'
}
//--------





//--------

//a-->z
int sort_favourite_by_stime_increasing(const st_favourite_freq_tag &o0, const st_favourite_freq_tag &o1 )
{
if( (o0.stime.length() == 0 ) && ( o1.stime.length() == 0) ) return 0;	//both empty
if( o0.stime.length() == 0 ) return 0;
if( o1.stime.length() == 0 ) return 1;

if( o0.stime.compare( o1.stime ) >= 0 ) return 0; 						//return 0 if o0 '>=' o1	
	
return 1;																//return 1 if o0 '<' o1			!! important to get this right for 'stable_sort()'
}


//z-->a ( o0 and o1 are reversed when comparing)
int sort_favourite_by_stime_decreasing(const st_favourite_freq_tag &o0, const st_favourite_freq_tag &o1 )
{
if( (o1.stime.length() == 0 ) && ( o0.stime.length() == 0) ) return 0;	//both empty
if( o1.stime.length() == 0 ) return 0;
if( o0.stime.length() == 0 ) return 1;

if( o1.stime.compare( o0.stime ) >= 0 ) return 0; 						//return 0 if o1 '>=' o0	
	
return 1;																//return 1 if o1 '<' o0			!! important to get this right for 'stable_sort()'
}
//--------






int comment_dlg_xx = 100;												//for persistent dlg reshow position
int comment_dlg_yy = 100;
int comment_dlg_cx = 400;
int comment_dlg_cy = 300;



void cb_fav_row_combo( Fl_Widget *w, void* v )
{
string s1;

int id = (intptr_t)v;

int row = id>>16;
int colmn = id & 0xffff;

bool bchanged = 0;


if( colmn == en_frci_fi_comment0 )
	{
	cl_fav_text_input* o = (cl_fav_text_input*)w;

	if( o->left_button )
		{
		printf( "cb_fav_row_combo() - left_button\n" );
		
		int dlg_wnd_fonttype = 4;
		int dlg_wnd_fontsize = 10;
		int bkgd_rr = 255;
		int bkgd_gg = 255;
		int bkgd_bb = 255;
		int txt_rr = 0;
		int txt_gg = 0;
		int txt_bb = 0;
		s1 = o->value();
		
		bool bselect_text = 0;
		bool b_allow_resize = 1;
		if( gc_input_multiline_adj( comment_dlg_xx, comment_dlg_yy, comment_dlg_cx, comment_dlg_cy, dlg_wnd_fonttype, dlg_wnd_fontsize, bkgd_rr, bkgd_gg, bkgd_bb, txt_rr, txt_gg, txt_bb, "Enter Comment", "", &s1, bselect_text, b_allow_resize ) )
			{
			o->value( s1.c_str() );
			}

		}
	}


if( colmn == en_frci_ld_active)
	{
	GCLed* o = (GCLed*)w;
	int ii = (intptr_t)v;

	if( ( o->right_button ) /*&& ( row != wnd_fav->row_sel )*/ )
		{
		s1 = wnd_rtl_graph->fi_name->value();
		if( s1.length() == 0 )
			{
			strpf( s1, "'Name' the tuning before saving." );
			fl_alert( s1.c_str(), 0 );
			return;
			}

		strpf( s1, "Overwrite entry %d with current tuning ?", row );
		bool bwrite = 1;
		int ret = fl_choice( s1.c_str(), "Cancel", "Overwrite", 0 );
		if( ret != 0 )
			{
			wnd_fav->set_leds_column0( -1, 0 );					//set all leds off
			
			wnd_fav->row_sel = row;
			o->ChangeCol( 2 );
//			wnd_fav->set_sel_row( row );
			wnd_rtl_graph->favourite_add( 0 );
//			wnd_fav->redraw();
			}
		return;
		}

	int z0 = o->GetColIndex();
	z0++;
	if( z0 > 2 ) z0 = 2;												//keep active state
	
	if( z0 == 1 ) wnd_fav->set_leds_column0( 2, 0 );			//just setting selection ?, don't change of the led control that is in 'active'
	if( z0 == 2 ) wnd_fav->set_leds_column0(-1, 0 );			//setting active ?, change all to off state
	
	if( z0 == 2 ) wnd_fav->flag_to_tune_using_fav_idx = row;
	
	wnd_fav->set_sel_row( row );

	o->ChangeCol( z0 );
	bchanged = 1;
	}



if( colmn == en_frci_ld_b_use_iffreq)
	{
	GCLed* o = (GCLed*)w;
	int ii = (intptr_t)v;

	int z0 = o->GetColIndex();
	z0++;
	if( z0 > 1 ) z0 = 0;
		
	wnd_fav->set_sel_row( row );
	
	o->ChangeCol( z0 );
	bchanged = 1;
	}




if( colmn == en_frci_fi_freq)
	{
	bchanged = 1;
	}

if( colmn == en_frci_fi_freq_center)
	{
	bchanged = 1;
	}

if( colmn == en_frci_fi_demod_type)
	{
	bchanged = 1;
	}

if( colmn == en_frci_ld_b_use_dwn_aa )
	{
	GCLed* o = (GCLed*)w;
	int ii = (intptr_t)v;

	int z0 = o->GetColIndex();
	z0++;
	if( z0 > 1 ) z0 = 0;
		
	wnd_fav->set_sel_row( row );
	
	o->ChangeCol( z0 );
	bchanged = 1;
	}

if( colmn == en_frci_fi_dwn_srate)
	{
	bchanged = 1;
	}

if( colmn == en_frci_fi_if_freq)
	{
	bchanged = 1;
	}


if( colmn == en_frci_ld_b_bw_bpass )
	{
	GCLed* o = (GCLed*)w;
	int ii = (intptr_t)v;

	int z0 = o->GetColIndex();
	z0++;
	if( z0 > 1 ) z0 = 0;
		
	wnd_fav->set_sel_row( row );
	
	o->ChangeCol( z0 );
	bchanged = 1;
	}


if( colmn == en_frci_fi_iffreq_bw_low)
	{
	bchanged = 1;
	}

if( colmn == en_frci_fi_iffreq_bw_high)
	{
	bchanged = 1;
	}

if( colmn == en_frci_fi_iffreq_bw_taps)
	{
	bchanged = 1;
	}


if( colmn == en_frci_fi_name)
	{
	bchanged = 1;
	}

if( colmn == en_frci_fi_group )
	{
	bchanged = 1;
	}

if( colmn == en_frci_fi_dev_manufacturer)
	{
	bchanged = 1;
	}


if( colmn == en_frci_fi_comment0)
	{
	bchanged = 1;
	}

if( colmn == en_frci_fi_dev_gain)
	{
	bchanged = 1;
	}



if( colmn == en_frci_ld_b_agc )
	{
	GCLed* o = (GCLed*)w;
	int ii = (intptr_t)v;

	int z0 = o->GetColIndex();
	z0++;
	if( z0 > 1 ) z0 = 0;
		
	wnd_fav->set_sel_row( row );
	
	o->ChangeCol( z0 );
	bchanged = 1;
	}


if( colmn == en_frci_fi_audio_gain)
	{
	bchanged = 1;
	}



if( colmn == en_frci_ld_deemph )
	{
	GCLed* o = (GCLed*)w;
	int ii = (intptr_t)v;

	int z0 = o->GetColIndex();
	z0++;
	if( z0 > 2 ) z0 = 0;
		
	o->ChangeCol( z0 );
	bchanged = 1;
	}




if( colmn == en_frci_ld_bias_t)
	{
	GCLed* o = (GCLed*)w;
	int ii = (intptr_t)v;

	int z0 = o->GetColIndex();
	z0++;
	if( z0 > 1 ) z0 = 0;
		
	wnd_fav->set_sel_row( row );
	
	o->ChangeCol( z0 );
	bchanged = 1;
	}
	
	
	

if( colmn == en_frci_ld_direct_sampling)
	{
	GCLed* o = (GCLed*)w;
	int ii = (intptr_t)v;

	int z0 = o->GetColIndex();
	z0++;
	if( z0 > 2 ) z0 = 0;
		
	wnd_fav->set_sel_row( row );
	
	o->ChangeCol( z0 );
	bchanged = 1;
	}


if( colmn == en_frci_fi_iq_gain )
	{
	bchanged = 1;
	}

if( bchanged )
	{
	vector<st_favourite_freq_tag> vv;
	wnd_fav->load_vector_from_gui_ctrls( vv, b_sanitise_favourites );
	vfav = vv;
	wnd_fav->set_via_vector( vfav, sz_demodulator_type, b_sanitise_favourites );
	}



printf( "cb_fav_row_combo() row %d colmn %d  , vfav.size %d\n", row, colmn, vfav.size() );
}





//----------------------------------------------------------------------
cl_favourite_row::cl_favourite_row(int xx, int yy, int ww, int hh, int id_in0, int id_in1, const char *label=0 ) : Fl_Group(xx,yy,ww,hh,label)
{
string s1;

id0 = id_in0;
id1 = id_in1;

int row_id = id0 << 16;


int offsx = 0;
int gapx = 0;
int offsy = yy;

int j = 0;

hdr_dim[j].x = offsx;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 50;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = 0;

j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 150;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = 0;

j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 80;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = 0;

j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 80;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = 0;





j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 80;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = 0;









j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 80;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = 0;

j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 80;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = 0;

j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 35;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = -18;

j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 50;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = -14;

j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 35;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = 0;

j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 50;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = 0;

j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 45;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = -10;

j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 80;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = 0;

j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 80;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = 0;

j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 40;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = -18;

j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 80;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = 0;

j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 35;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = -10;

j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 80;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = 0;


j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 55;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = -10;


j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 80;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = 0;

j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 80;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = 0;

j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 45;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = -10;

j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 45;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = -22;

j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 55;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = 0;

j++;
hdr_dim[j].x = hdr_dim[j-1].x + hdr_dim[j-1].w - 1;
hdr_dim[j].y = offsy;
hdr_dim[j].w = 80;
hdr_dim[j].h = hh;
hdr_dim[j].label_offs_x = 0;



// see 'st_favourite_row_ctrl_tag'
int ii = 0;
ctrl.ld_active = new GCLed( hdr_dim[ii].x + hdr_dim[ii].w/2, hdr_dim[ii].y + 2, 11, 11, "" );
ctrl.ld_active->id = id_in0;
ctrl.ld_active->id2 = ii;
strpf( s1, "%03d", id0 );
ctrl.ld_active->copy_label( s1.c_str() );
ctrl.ld_active->labelsize( 8 );
ctrl.ld_active->tooltip( "yellow: selected\ngreen: activated, receiving on this freq\n\nRight click to store current tuning" );
ctrl.ld_active->align( FL_ALIGN_LEFT );
//ld_carrier_max_tune->led_style = cn_gcled_style_square;
ctrl.ld_active->led_style = cn_gcled_style_round;

ctrl.ld_active->SetColIndex(0, 80, 120, 80);
ctrl.ld_active->SetColIndex(1, 255, 172, 15);
ctrl.ld_active->SetColIndex(2, 80, 255, 80);
//ld_direct_sampling->set_col_from_str( "100 0 0, 255 0 0, 0 0 255", ',' );     //the thrid colour set not used in this app
ctrl.ld_active->callback( cb_fav_row_combo, (void*)(row_id | en_frci_ld_active) );


ii++;
ctrl.fi_name = new cl_fav_text_input( hdr_dim[ii].x, hdr_dim[ii].y, hdr_dim[ii].w, hdr_dim[ii].h, "" );
ctrl.fi_name->id0 = id_in0;
ctrl.fi_name->id1 = ii;
ctrl.fi_name->labelsize( 9 );
ctrl.fi_name->textsize( 9 );
ctrl.fi_name->box( FL_BORDER_BOX );
ctrl.fi_name->value( "name" );
ctrl.fi_name->tooltip( "enter name" );
ctrl.fi_name->callback( cb_fav_row_combo, (void*)(row_id | en_frci_fi_name) );


ii++;
ctrl.fi_comment0 = new cl_fav_text_input( hdr_dim[ii].x, hdr_dim[ii].y, hdr_dim[ii].w, hdr_dim[ii].h, "" );
ctrl.fi_comment0->id0 = id_in0;
ctrl.fi_comment0->id1 = ii;
ctrl.fi_comment0->labelsize( 9 );
ctrl.fi_comment0->textsize( 9 );
ctrl.fi_comment0->box( FL_BORDER_BOX );
ctrl.fi_comment0->value( "comment" );
ctrl.fi_comment0->tooltip( "enter a comment" );
ctrl.fi_comment0->callback( cb_fav_row_combo, (void*)(row_id | en_frci_fi_comment0 ) );

ctrl.fi_comment0->b_type_comment = 1;									//this control will respond to a mouse click and open up a bigger editbox							


ii++;
ctrl.fi_group = new cl_fav_text_input( hdr_dim[ii].x, hdr_dim[ii].y, hdr_dim[ii].w, hdr_dim[ii].h, "" );
ctrl.fi_group->id0 = id_in0;
ctrl.fi_group->id1 = ii;
ctrl.fi_group->labelsize( 9 );
ctrl.fi_group->textsize( 9 );
ctrl.fi_group->box( FL_BORDER_BOX );
ctrl.fi_group->value( "group" );
ctrl.fi_group->tooltip( "enter a grouping name" );
ctrl.fi_group->callback( cb_fav_row_combo, (void*)(row_id | en_frci_fi_group) );


ii++;
ctrl.fi_freq = new cl_fav_text_input( hdr_dim[ii].x, hdr_dim[ii].y, hdr_dim[ii].w, hdr_dim[ii].h, "" );
ctrl.fi_freq->id0 = id_in0;
ctrl.fi_freq->id1 = ii;
ctrl.fi_freq->labelsize( 9 );
ctrl.fi_freq->textsize( 9 );
ctrl.fi_freq->box( FL_BORDER_BOX );
ctrl.fi_freq->value( "freq" );
ctrl.fi_freq->tooltip( "enter freq, this is the effective freq, the station's freq, it amounts to sum: freq cntr + freq sub tune" );
ctrl.fi_freq->callback( cb_fav_row_combo, (void*)(row_id | en_frci_fi_freq) );

ii++;
ctrl.fi_freq_center = new cl_fav_text_input( hdr_dim[ii].x, hdr_dim[ii].y, hdr_dim[ii].w, hdr_dim[ii].h, "" );
ctrl.fi_freq_center->id0 = id_in0;
ctrl.fi_freq_center->id1 = ii;
ctrl.fi_freq_center->labelsize( 9 );
ctrl.fi_freq_center->textsize( 9 );
ctrl.fi_freq_center->box( FL_BORDER_BOX );
ctrl.fi_freq_center->value( "freq cntr" );
ctrl.fi_freq_center->tooltip( "enter center frequency, this is the onboard tuner's freq (with no sub tuning offset)" );
ctrl.fi_freq_center->callback( cb_fav_row_combo, (void*)(row_id | en_frci_fi_freq_center) );

ii++;
ctrl.fi_demod_type = new cl_fav_text_input( hdr_dim[ii].x, hdr_dim[ii].y, hdr_dim[ii].w, hdr_dim[ii].h, "" );
ctrl.fi_demod_type->id0 = id_in0;
ctrl.fi_demod_type->id1 = ii;
ctrl.fi_demod_type->labelsize( 9 );
ctrl.fi_demod_type->textsize( 9 );
ctrl.fi_demod_type->box( FL_BORDER_BOX );
ctrl.fi_demod_type->value( "Demodulator" );
ctrl.fi_demod_type->tooltip( "enter demodulator" );
ctrl.fi_demod_type->callback( cb_fav_row_combo, (void*)(row_id | en_frci_fi_demod_type) );


ii++;
ctrl.ld_b_dwn_aa = new GCLed( hdr_dim[ii].x + hdr_dim[ii].w/2, hdr_dim[ii].y, 11, 11, "" );
ctrl.ld_b_dwn_aa->id = id_in0;
ctrl.ld_b_dwn_aa->id2 = ii;
//strpf( s1, "%03d", id0 );
ctrl.ld_b_dwn_aa->copy_label( "" );
ctrl.ld_b_dwn_aa->labelsize( 8 );
ctrl.ld_b_dwn_aa->tooltip( "use antialias filter before downsampler, essential when there are any channels in the freq range you are tuning" );
ctrl.ld_b_dwn_aa->align( FL_ALIGN_LEFT );
//ld_b_dwn_aa->led_style = cn_gcled_style_square;
ctrl.ld_b_dwn_aa->led_style = cn_gcled_style_round;

ctrl.ld_b_dwn_aa->SetColIndex(0, 80, 120, 80);
ctrl.ld_b_dwn_aa->SetColIndex(1, 80, 255, 80);
//ld_b_dwn_aa->set_col_from_str( "100 0 0, 255 0 0, 0 0 255", ',' );     //the thrid colour set not used in this app
ctrl.ld_b_dwn_aa->callback( cb_fav_row_combo, (void*)(row_id | en_frci_ld_b_use_dwn_aa) );



ii++;
ctrl.fi_dwn_srate = new cl_fav_text_input( hdr_dim[ii].x, hdr_dim[ii].y, hdr_dim[ii].w, hdr_dim[ii].h, "" );
ctrl.fi_dwn_srate->id0 = id_in0;
ctrl.fi_dwn_srate->id1 = ii;
ctrl.fi_dwn_srate->labelsize( 9 );
ctrl.fi_dwn_srate->textsize( 9 );
ctrl.fi_dwn_srate->box( FL_BORDER_BOX );
ctrl.fi_dwn_srate->value( "dwnsrate" );
ctrl.fi_dwn_srate->tooltip( "downsampler srate, try AM: 12000, FM Stereo >= 240000" );
ctrl.fi_dwn_srate->callback( cb_fav_row_combo, (void*)(row_id | en_frci_fi_dwn_srate) );


ii++;
ctrl.ld_b_use_iffreq = new GCLed( hdr_dim[ii].x + hdr_dim[ii].w/2, hdr_dim[ii].y, 11, 11, "" );
ctrl.ld_b_use_iffreq->id = id_in0;
ctrl.ld_b_use_iffreq->id2 = ii;
//strpf( s1, "%03d", id0 );
ctrl.ld_b_use_iffreq->copy_label( "" );
ctrl.ld_b_use_iffreq->labelsize( 8 );
ctrl.ld_b_use_iffreq->tooltip( "use intermediate freq" );
ctrl.ld_b_use_iffreq->align( FL_ALIGN_LEFT );
//ld_b_use_iffreq->led_style = cn_gcled_style_square;
ctrl.ld_b_use_iffreq->led_style = cn_gcled_style_round;

ctrl.ld_b_use_iffreq->SetColIndex(0, 80, 120, 80);
ctrl.ld_b_use_iffreq->SetColIndex(1, 80, 255, 80);
//ld_b_use_iffreq->set_col_from_str( "100 0 0, 255 0 0, 0 0 255", ',' );     //the thrid colour set not used in this app
ctrl.ld_b_use_iffreq->callback( cb_fav_row_combo, (void*)(row_id | en_frci_ld_b_use_iffreq) );


ii++;
ctrl.fi_if_freq = new cl_fav_text_input( hdr_dim[ii].x, hdr_dim[ii].y, hdr_dim[ii].w, hdr_dim[ii].h, "" );
ctrl.fi_if_freq->id0 = id_in0;
ctrl.fi_if_freq->id1 = ii;
ctrl.fi_if_freq->labelsize( 9 );
ctrl.fi_if_freq->textsize( 9 );
ctrl.fi_if_freq->box( FL_BORDER_BOX );
ctrl.fi_if_freq->value( "10000" );
ctrl.fi_if_freq->tooltip( "enter an intermediate freq offset to use" );
ctrl.fi_if_freq->callback( cb_fav_row_combo, (void*)(row_id | en_frci_fi_if_freq) );



ii++;
ctrl.ld_b_bw_bpass = new GCLed( hdr_dim[ii].x + hdr_dim[ii].w/2, hdr_dim[ii].y, 11, 11, "" );
ctrl.ld_b_bw_bpass->id = id_in0;
ctrl.ld_b_bw_bpass->id2 = ii;
//strpf( s1, "%03d", id0 );
ctrl.ld_b_bw_bpass->copy_label( "" );
ctrl.ld_b_bw_bpass->labelsize( 8 );
ctrl.ld_b_bw_bpass->tooltip( "enable bandpass filter" );
ctrl.ld_b_bw_bpass->align( FL_ALIGN_LEFT );
//ld_b_bw_bpass->led_style = cn_gcled_style_square;
ctrl.ld_b_bw_bpass->led_style = cn_gcled_style_round;

ctrl.ld_b_bw_bpass->SetColIndex(0, 80, 120, 80);
ctrl.ld_b_bw_bpass->SetColIndex(1, 80, 255, 80);
//ld_b_bw_bpass->set_col_from_str( "100 0 0, 255 0 0, 0 0 255", ',' );     //the thrid colour set not used in this app
ctrl.ld_b_bw_bpass->callback( cb_fav_row_combo, (void*)(row_id | en_frci_ld_b_bw_bpass) );


ii++;
ctrl.fi_iffreq_bw_low = new cl_fav_text_input( hdr_dim[ii].x, hdr_dim[ii].y, hdr_dim[ii].w, hdr_dim[ii].h, "" );
ctrl.fi_iffreq_bw_low->id0 = id_in0;
ctrl.fi_iffreq_bw_low->id1 = ii;
ctrl.fi_iffreq_bw_low->labelsize( 9 );
ctrl.fi_iffreq_bw_low->textsize( 9 );
ctrl.fi_iffreq_bw_low->box( FL_BORDER_BOX );
ctrl.fi_iffreq_bw_low->value( "80" );
ctrl.fi_iffreq_bw_low->tooltip( "intermediate bandwith low freq" );
ctrl.fi_iffreq_bw_low->callback( cb_fav_row_combo, (void*)(row_id | en_frci_fi_iffreq_bw_low) );


ii++;
ctrl.fi_iffreq_bw_high = new cl_fav_text_input( hdr_dim[ii].x, hdr_dim[ii].y, hdr_dim[ii].w, hdr_dim[ii].h, "" );
ctrl.fi_iffreq_bw_high->id0 = id_in0;
ctrl.fi_iffreq_bw_high->id1 = ii;
ctrl.fi_iffreq_bw_high->labelsize( 9 );
ctrl.fi_iffreq_bw_high->textsize( 9 );
ctrl.fi_iffreq_bw_high->box( FL_BORDER_BOX );
ctrl.fi_iffreq_bw_high->value( "5000" );
ctrl.fi_iffreq_bw_high->tooltip( "intermediate bandwith high freq" );
ctrl.fi_iffreq_bw_high->callback( cb_fav_row_combo, (void*)(row_id | en_frci_fi_iffreq_bw_high) );


ii++;
ctrl.fi_iffreq_bw_taps = new cl_fav_text_input( hdr_dim[ii].x, hdr_dim[ii].y, hdr_dim[ii].w, hdr_dim[ii].h, "" );
ctrl.fi_iffreq_bw_taps->id0 = id_in0;
ctrl.fi_iffreq_bw_taps->id1 = ii;
ctrl.fi_iffreq_bw_taps->labelsize( 9 );
ctrl.fi_iffreq_bw_taps->textsize( 9 );
ctrl.fi_iffreq_bw_taps->box( FL_BORDER_BOX );
ctrl.fi_iffreq_bw_taps->value( "60" );
ctrl.fi_iffreq_bw_taps->tooltip( "enter number of filter coeffs to use, try 60" );
ctrl.fi_iffreq_bw_taps->callback( cb_fav_row_combo, (void*)(row_id | en_frci_fi_iffreq_bw_taps) );


ii++;
ctrl.fi_dev_gain = new cl_fav_text_input( hdr_dim[ii].x, hdr_dim[ii].y, hdr_dim[ii].w, hdr_dim[ii].h, "" );
ctrl.fi_dev_gain->id0 = id_in0;
ctrl.fi_dev_gain->id1 = ii;
ctrl.fi_dev_gain->labelsize( 9 );
ctrl.fi_dev_gain->textsize( 9 );
ctrl.fi_dev_gain->box( FL_BORDER_BOX );
ctrl.fi_dev_gain->value( "30" );
ctrl.fi_dev_gain->tooltip( "sets device gain" );
ctrl.fi_dev_gain->callback( cb_fav_row_combo, (void*)(row_id | en_frci_fi_dev_gain) );


ii++;
ctrl.ld_b_agc = new GCLed( hdr_dim[ii].x + hdr_dim[ii].w/2, hdr_dim[ii].y, 11, 11, "" );
ctrl.ld_b_agc->id = id_in0;
ctrl.ld_b_agc->id2 = ii;
//strpf( s1, "%03d", id0 );
ctrl.ld_b_agc->copy_label( "" );
ctrl.ld_b_agc->labelsize( 8 );
ctrl.ld_b_agc->tooltip( "enable audio agc to keep audio levels consistent" );
ctrl.ld_b_agc->align( FL_ALIGN_LEFT );
//ld_b_agc->led_style = cn_gcled_style_square;
ctrl.ld_b_agc->led_style = cn_gcled_style_round;

ctrl.ld_b_agc->SetColIndex(0, 80, 120, 80);
ctrl.ld_b_agc->SetColIndex(1, 80, 255, 80);
//ld_b_agc->set_col_from_str( "100 0 0, 255 0 0, 0 0 255", ',' );     //the thrid colour set not used in this app
ctrl.ld_b_agc->callback( cb_fav_row_combo, (void*)(row_id | en_frci_ld_b_agc) );


ii++;
ctrl.fi_audio_gain = new cl_fav_text_input( hdr_dim[ii].x, hdr_dim[ii].y, hdr_dim[ii].w, hdr_dim[ii].h, "" );
ctrl.fi_audio_gain->id0 = id_in0;
ctrl.fi_audio_gain->id1 = ii;
ctrl.fi_audio_gain->labelsize( 9 );
ctrl.fi_audio_gain->textsize( 9 );
ctrl.fi_audio_gain->box( FL_BORDER_BOX );
ctrl.fi_audio_gain->value( "1.0" );
ctrl.fi_audio_gain->tooltip( "sets audio gain" );
ctrl.fi_audio_gain->callback( cb_fav_row_combo, (void*)(row_id | en_frci_fi_audio_gain) );


ii++;
ctrl.ld_deemph = new GCLed( hdr_dim[ii].x + hdr_dim[ii].w/2, hdr_dim[ii].y, 11, 11, "" );
ctrl.ld_deemph->id = id_in0;
ctrl.ld_deemph->id2 = ii;
//strpf( s1, "%03d", id0 );
ctrl.ld_deemph->copy_label( "" );
ctrl.ld_deemph->labelsize( 8 );
ctrl.ld_deemph->tooltip( "FM Stereo deemphasis\nred: off\norange: 50uS\ngreen: 75uS (USA)" );
ctrl.ld_deemph->align( FL_ALIGN_LEFT );
//ld_deemph->led_style = cn_gcled_style_square;
ctrl.ld_deemph->led_style = cn_gcled_style_round;

ctrl.ld_deemph->SetColIndex(0, 220, 0, 0);
ctrl.ld_deemph->SetColIndex(1, 255, 200, 60);
ctrl.ld_deemph->SetColIndex(2, 0, 255, 0);

//ld_deemph->set_col_from_str( "100 0 0, 255 0 0, 0 0 255", ',' );     //the thrid colour set not used in this app
ctrl.ld_deemph->callback( cb_fav_row_combo, (void*)(row_id | en_frci_ld_deemph ) );


ii++;
ctrl.fi_date = new cl_fav_text_input( hdr_dim[ii].x, hdr_dim[ii].y, hdr_dim[ii].w, hdr_dim[ii].h, "" );
ctrl.fi_date->id0 = id_in0;
ctrl.fi_date->id1 = ii;
ctrl.fi_date->labelsize( 9 );
ctrl.fi_date->textsize( 9 );
ctrl.fi_date->box( FL_BORDER_BOX );
ctrl.fi_date->value( "2023-12-31" );
ctrl.fi_date->tooltip( "sets date, yyyy-mm-dd, e.g: 2023-12-31" );
ctrl.fi_date->callback( cb_fav_row_combo, (void*)(row_id | en_frci_fi_date) );


ii++;
ctrl.fi_time = new cl_fav_text_input( hdr_dim[ii].x, hdr_dim[ii].y, hdr_dim[ii].w, hdr_dim[ii].h, "" );
ctrl.fi_time->id0 = id_in0;
ctrl.fi_time->id1 = ii;
ctrl.fi_time->labelsize( 9 );
ctrl.fi_time->textsize( 9 );
ctrl.fi_time->box( FL_BORDER_BOX );
ctrl.fi_time->value( "09:00:00" );
ctrl.fi_time->tooltip( "sets time, e.g: 23:59:59" );
ctrl.fi_time->callback( cb_fav_row_combo, (void*)(row_id | en_frci_fi_time) );



ii++;
ctrl.ld_bias_t = new GCLed( hdr_dim[ii].x + hdr_dim[ii].w/2, hdr_dim[ii].y, 11, 11, "" );
ctrl.ld_bias_t->id = id_in0;
ctrl.ld_bias_t->id2 = ii;
//strpf( s1, "%03d", id0 );
ctrl.ld_bias_t->copy_label( "" );
ctrl.ld_bias_t->labelsize( 8 );
ctrl.ld_bias_t->tooltip( "output bias_t voltage" );
ctrl.ld_bias_t->align( FL_ALIGN_LEFT );
//ld_bias_t->led_style = cn_gcled_style_square;
ctrl.ld_bias_t->led_style = cn_gcled_style_round;

ctrl.ld_bias_t->SetColIndex(0, 120, 80, 80);
ctrl.ld_bias_t->SetColIndex(1, 255, 80, 80);
//ld_bias_t->set_col_from_str( "100 0 0, 255 0 0, 0 0 255", ',' );     //the thrid colour set not used in this app
ctrl.ld_bias_t->callback( cb_fav_row_combo, (void*)(row_id | en_frci_ld_bias_t) );


ii++;
ctrl.ld_direct_sampling = new GCLed( hdr_dim[ii].x + hdr_dim[ii].w/2, hdr_dim[ii].y, 11, 11, "" );
ctrl.ld_direct_sampling->id = id_in0;
ctrl.ld_direct_sampling->id2 = ii;
//strpf( s1, "%03d", id0 );
ctrl.ld_direct_sampling->copy_label( "" );
ctrl.ld_direct_sampling->labelsize( 8 );
ctrl.ld_direct_sampling->tooltip( "enable direct sampling (rf tuner bypass), yellow: I-branch,  green: Q-branch" );

ctrl.ld_direct_sampling->align( FL_ALIGN_LEFT );
//ld_direct_sampling->led_style = cn_gcled_style_square;
ctrl.ld_direct_sampling->led_style = cn_gcled_style_round;

ctrl.ld_direct_sampling->SetColIndex(0, 80, 80, 80);
ctrl.ld_direct_sampling->SetColIndex(1, 255, 255, 80);
ctrl.ld_direct_sampling->SetColIndex(2, 0, 255, 80);
ctrl.ld_direct_sampling->callback( cb_fav_row_combo, (void*)(row_id | en_frci_ld_direct_sampling) );


ii++;
ctrl.fi_iq_gain = new cl_fav_text_input( hdr_dim[ii].x, hdr_dim[ii].y, hdr_dim[ii].w, hdr_dim[ii].h, "" );
ctrl.fi_iq_gain->id0 = id_in0;
ctrl.fi_iq_gain->id1 = ii;
ctrl.fi_iq_gain->labelsize( 9 );
ctrl.fi_iq_gain->textsize( 9 );
ctrl.fi_iq_gain->box( FL_BORDER_BOX );
ctrl.fi_iq_gain->value( "0.3" );
ctrl.fi_iq_gain->tooltip( "sets gain for iq signal" );
ctrl.fi_iq_gain->callback( cb_fav_row_combo, (void*)(row_id | en_frci_fi_iq_gain) );


ii++;
ctrl.fi_dev_manufacturer = new cl_fav_text_input( hdr_dim[ii].x, hdr_dim[ii].y, hdr_dim[ii].w, hdr_dim[ii].h, "" );
ctrl.fi_dev_manufacturer->id0 = id_in0;
ctrl.fi_dev_manufacturer->id1 = ii;
ctrl.fi_dev_manufacturer->labelsize( 9 );
ctrl.fi_dev_manufacturer->textsize( 9 );
ctrl.fi_dev_manufacturer->box( FL_BORDER_BOX );
ctrl.fi_dev_manufacturer->value( "mnufctr" );
ctrl.fi_dev_manufacturer->tooltip( "enter device manufacturer" );
ctrl.fi_dev_manufacturer->callback( cb_fav_row_combo, (void*)(row_id | en_frci_fi_dev_manufacturer) );

end();
}




cl_favourite_row::~cl_favourite_row()  
{
}
//----------------------------------------------------------------------



//----------------------------------------------------------------------
cl_box_header::cl_box_header( int xx, int yy, int wid, int hei, const char *label, int label_offs_x_in ) : Fl_Box( xx, yy, wid, hei, label )
{
left_button = 0;
b_sort_order_hidden = 0;
b_sort_order_dimmed = 0;
sort_order = en_hso_increasing;
label_offs_x = label_offs_x_in;
}

cl_box_header::~cl_box_header()
{
}








void cl_box_header::draw()
{
int xx = x();
int yy = y();

int iF = fl_font();
int iS = fl_size();


int px, py;


fl_color( FL_BACKGROUND_COLOR );
fl_rectf( x(), y(), w(), h() );


fl_color( FL_BLACK );
if( box() == FL_BORDER_BOX )
//if(1)
	{
	fl_rect( xx, yy, w(), h() );
	}


	
//fl_Box::draw();

//bsort_order_dimmed = 1;



fl_font( labelfont(), labelsize() );
if( left_button ) fl_color( 200, 80, 80 );
else fl_color( 0, 0, 0 );

int xoffs = 20;
//if( b_sort_order_hidden ) xoffs = 0;

fl_draw( label(), xx + xoffs + label_offs_x, yy + 11 );

if (!b_sort_order_dimmed) fl_color( 0, 0, 0 );
else fl_color( 140, 140, 140 );

//sort_order = en_hso_mixed;

if(!b_sort_order_hidden )
	{
	switch( sort_order )
		{
		case en_hso_increasing:
			px = xx + 7;
			py = yy + 3;
			fl_line( px, py, px, py + 9 );
			fl_line( px, py + 9, px - 2, py + 7 );
			fl_line( px, py + 9, px + 2, py + 7 );

			px = xx + 12;
			py = yy + 4;
			fl_line( px, py, px + 1, py );
			fl_line( px, py + 2, px + 2, py + 2 );
			fl_line( px, py + 4, px + 3, py + 4 );
			fl_line( px, py + 6, px + 4, py + 6 );
		break;

		case en_hso_decreasing:
			px = xx + 7;
			py = yy + 3;
			fl_line( px, py, px, py + 9 );
			fl_line( px, py, px - 2, py + 2 );
			fl_line( px, py, px + 2, py + 2 );

			px = xx + 12;
			py = yy + 4;
			fl_line( px, py + 6, px + 1, py + 6 );
			fl_line( px, py + 4 , px + 2, py + 4 );
			fl_line( px, py + 2, px + 3, py + 2 );
			fl_line( px, py, px + 4, py );
		break;


		case en_hso_mixed:
			px = xx + 7;
			py = yy + 3;
			fl_line( px, py, px, py + 9 );
			fl_line( px, py + 9, px - 2, py + 7 );
			fl_line( px, py + 9, px + 2, py + 7 );

			px = xx + 14;
			py = yy + 3;
			fl_line( px, py, px, py + 9 );
			fl_line( px, py, px - 2, py + 2 );
			fl_line( px, py, px + 2, py + 2 );
		break;

		default:
		break;
		}
	}

//up arrow
//px = xx + 7;
//py = yy + 3;
//fl_line( px, py, px, py + 9 );
//fl_line( px, py, px - 2, py + 2 );
//fl_line( px, py, px + 2, py + 2 );

/*
//dwn/down arrow
//px = xx + 7;
//py = yy + 3;
fl_line( px, py, px, py + 9 );
fl_line( px, py + 9, px - 2, py + 7 );
fl_line( px, py + 9, px + 2, py + 7 );
fl_line( px, py, px - 2, py + 2 );
fl_line( px, py, px + 2, py + 2 );
*/

/*
//increasing 'text line' order symbol (increasing)
px = xx + 12;
py = yy + 4;
fl_line( px, py, px + 1, py );
fl_line( px, py + 2, px + 2, py + 2 );
fl_line( px, py + 4, px + 3, py + 4 );
fl_line( px, py + 6, px + 4, py + 6 );
*/

/*
//decreasing 'text line' order symbol (decreasing)
px = xx + 12;
py = yy + 4;
fl_line( px, py + 6, px + 1, py + 6 );
fl_line( px, py + 4 , px + 2, py + 4 );
fl_line( px, py + 2, px + 3, py + 2 );
fl_line( px, py, px + 4, py );
*/

fl_font( iF, iS );
}










int cl_box_header::handle( int e )
{
string s1;
int len;
char *szTmp;
bool need_redraw = 0;
bool dont_pass_on = 0;
int mousewheel;

if ( e == FL_LEAVE )
	{
	left_button = 0;
	need_redraw = 1;
	}

if ( e & FL_MOVE )
	{

//	need_redraw = 1;
//    dont_pass_on = 1;
	}

if ( e == FL_PUSH )
	{
	int ii = Fl::event_clicks();

	if( Fl::event_button() == 1 )											//left click
		{
		left_button = 1;
		if( !b_sort_order_hidden) do_callback( (void*)this, (void*)id0 );
		}
	need_redraw = 1;
	}


if ( e == FL_RELEASE )
	{

	if( Fl::event_button() == 1 )											//left click
		{
		left_button = 0;
		}
	need_redraw = 1;
	}

if ( need_redraw ) redraw();

if( dont_pass_on ) return 1;

return Fl_Box::handle(e);
}

//----------------------------------------------------------------------





void cb_box_header( Fl_Widget *w, void* v )
{

cl_box_header *o = (cl_box_header*) w;

int id = (intptr_t)v;

printf( "cb_box_header() id %d id0 %d\n", id, o->id0 );

wnd_fav->sort_order_change( id, -1 );
}





extern Fl_Menu_Item menu_sdr_items[];


//----------------------------------------------------------------------
cl_favourite_wnd::cl_favourite_wnd( int xx, int yy, int wid, int hei, const char *label ) : Fl_Double_Window( xx, yy, wid, hei, label )
{
string s1, stip;
wnd_header = 0;
last_wid = 0;
last_hei = 0;

last_header_offsx = -1;
tick_secs_passed = 0;
header_align_time_period = 1e-3;
header_align_time_count = 0;
flag_to_tune_using_fav_idx = -1;
row_sel = -1;
menu_hei = 17;

//Fl_Window* wnd_header0 = new Fl_Window( 0, 0, 3200, 3920, "" );			//PUT header box ctrls in a WINDOW so they are not scaled by FLTK's 'resizable()' feature


//menu bar
menu_fav = new Fl_Menu_Bar(0, 0, this->w(), 25);
menu_fav->textsize(12);
//menu_sdr->copy( menu_sdr_items, this );			//this causes RList selection to not correctly pass the 'v' index to callback: 'void cb_recentlist( Fl_Widget *, void *v )'
menu_fav->menu( menu_sdr_items ) ;					//use this menu assignment for RList to  work
menu_fav->hide();									//hide it initially



//first build row obj, as it loads 'bx_hdr[]' which is used further below
//fl_scrol_grp = new Fl_Group( 2, 25, w()-4, hei-22, "" );
fl_scrol = new Fl_Scroll( 2, cn_header_posy+20, w()-4, hei-(cn_header_posy+20), "" );
for( int i = 0; i < cn_favourite_row_max; i ++ )
	{
	int offsx = 2;
	int gapx = 2;
	int offsy = i*15 + 0;
	int hh = 15;
	grp_row[i] = new cl_favourite_row( offsx + 0, offsy + 2, wid - 4, hh, i, i, "" );
	}
fl_scrol->end();
//fl_scrol_grp->end();

//wnd_header0->end();


wnd_header = new Fl_Window( 0, cn_header_posy, 3200, 18, "" );			//PUT header box ctrls in a WINDOW so they are not scaled by FLTK's 'resizable()' feature

//building heading controls
for( int i = 0; i < cn_favourite_box_header_max; i++ )
	{
	en_header_param_tag param;

	if( i == 0 ) { s1 = ""; param = en_hpm_active; stip = "activate favourite";  }
	if( i == 1 ) { s1 = "name"; param = en_hpm_sname; stip = "favourite's name"; };
	if( i == 2 )  { s1 = "comment"; param = en_hpm_scomment0; stip = "user comments"; };
	if( i == 3 ) {  s1 = "group"; param = en_hpm_sgroup; stip = "group favorite belongs to"; };
	if( i == 4 )  { s1 = "freq"; param = en_hpm_freq; stip = "this is the effective freq, the station's freq, it amounts to sum: freq cntr + freq sub tune"; };
	if( i == 5 )  { s1 = "freq cntr"; param = en_hpm_freq_center; stip = "this is the onboard tuner's freq (with no sub tuning offset)"; };
	if( i == 6 )  { s1 = "demod typ"; param = en_hpm_demod_type; stip = "demodulator, am,fm, widefm, usb, lsb"; };
	if( i == 7 )  { s1 = "dwnAA"; param = en_hpm_b_use_dwn_aa; stip = "use antialias filter before downsampler, essential when there are any channels in the freq range you are tuning"; };
	if( i == 8 )  { s1 = "dwnsrate"; param = en_hpm_dwn_srate; stip = "downsampler srate, try AM: 12000, FM Stereo >= 240000"; };
	if( i == 9 )  { s1 = "if"; param = en_hpm_b_use_iffreq; stip = "use intermediate frequency, helps reduce noise"; };
	if( i == 10 )  { s1 = "if_freq"; param = en_hpm_if_freq; stip = "intermediate frequency's offset"; };
	if( i == 11 )  { s1 = "bpass"; param = en_hpm_b_bw_bpass; stip = "enable bandpass filter"; };
	if( i == 12 )  { s1 = "bpf_low"; param = en_hpm_iffreq_bw_low; stip = "filter low cutoff freq"; };
	if( i == 13 )  { s1 = "bpf_high"; param = en_hpm_iffreq_bw_high; stip = "filter high cutoff freq"; };
	if( i == 14 )  { s1 = "bpf_taps"; param = en_hpm_iffreq_bw_taps; stip = "number of filter ceoffs to use, higher gives a sharper filter transition, but cpu load increases likewise, try 60"; };
	if( i == 15 )  { s1 = "dev_gain"; param = en_hpm_dev_gain; stip = "device's front end gain"; };
	if( i == 16 )  { s1 = "agc"; param = en_hpm_b_agc; stip = "enable agc to keep audio levels consistent"; };
	if( i == 17 )  { s1 = "aud_gain"; param = en_hpm_audio_gain; stip = "audio gain setting"; };
	if( i == 18 )  { s1 = "deemph"; param = en_hpm_deemph; stip = "FM Stereo deemphasis\nred: off\norange: 50uS\ngreen: 75uS (USA)"; };
	if( i == 19 )  { s1 = "date"; param = en_hpm_sdate; stip = "date favourite was created"; };
	if( i == 20 )  { s1 = "time"; param = en_hpm_stime; stip = "time favourite was create"; };
	if( i == 21 )  { s1 = "bias_t"; param = en_hpm_bias_t; stip = "apply bias tee voltage to antenna feed"; };
	if( i == 22 )  { s1 = "dirct_smpl"; param = en_hpm_direct_sampling; stip = "direct sampling, dark: Off (uses tuner - VHF),  yellow: I-branch(no tuner - HF),  green: Q-branch(no tuner - HF)"; };
	if( i == 23 )  { s1 = "iq gain"; param = en_hpm_iq_gain; stip = "gain to use for iq signal"; };
	if( i == 24 )  { s1 = "mnufctr"; param = en_hpm_sdev_manufacturer; stip = "device's reported manufacturer string"; };



///	strpf( s1, "%03d", i );
	bx_hdr[i] = new cl_box_header( grp_row[0]->hdr_dim[i].x, grp_row[0]->hdr_dim[i].y, grp_row[0]->hdr_dim[i].w, grp_row[0]->hdr_dim[i].h, "",grp_row[0]->hdr_dim[i].label_offs_x );
	bx_hdr[i]->id0 = i;

	bx_hdr[i]->param = param;

	if( i > 1 )
		{
		bx_hdr[i]->b_sort_order_dimmed = 1;
		bx_hdr[i]->sort_order = en_hso_mixed;
		}


	if( ( param == en_hpm_active ) || ( param == en_hpm_b_use_iffreq ) || ( param == en_hpm_if_freq ) || ( param == en_hpm_iffreq_bw_low ) || ( param == en_hpm_iffreq_bw_high ) || ( param == en_hpm_dev_gain )  )
		{
		bx_hdr[i]->b_sort_order_hidden = 1;
//		bx_hdr[i]->sort_order = en_hso_none;
		}

	if( ( param == en_hpm_audio_gain ) || ( param == en_hpm_bias_t ) || ( param == en_hpm_direct_sampling ) || ( param == en_hpm_iq_gain ) )
		{
		bx_hdr[i]->b_sort_order_hidden = 1;
		}

	if( ( param == en_hpm_b_use_dwn_aa ) || ( param == en_hpm_iffreq_bw_taps ) || ( param == en_hpm_b_agc ) || ( param == en_hpm_b_bw_bpass ) /*|| (  )*/ )
		{
		bx_hdr[i]->b_sort_order_hidden = 1;
		}


	if( ( param == en_hpm_dwn_srate ) || ( param == en_hpm_deemph ) || ( param == en_hpm_scomment0 ) /*|| ( param == en_hpm_b_agc ) || ( param == en_hpm_b_bw_bpass ) */ )
		{
		bx_hdr[i]->b_sort_order_hidden = 1;
		}



	if( param == en_hpm_if_freq ) 
		{
//		bx_hdr[i]->b_sort_order_dimmed = 1;
		bx_hdr[i]->sort_order = en_hso_mixed;
		}

	bx_hdr[i]->copy_label( s1.c_str() );
	bx_hdr[i]->labelsize( 9 );
//	bx_hdr[i]->box( FL_BORDER_BOX );
	bx_hdr[i]->align( FL_ALIGN_INSIDE|FL_ALIGN_CENTER );
	bx_hdr[i]->callback( cb_box_header, (void*)i );
	
	bx_hdr[i]->stooltip = stip;											//keep a static copy
	bx_hdr[i]->tooltip( bx_hdr[i]->stooltip.c_str() );
	}

wnd_header->end();

/*
Fl_Box* fl_box1 = new Fl_Box( 2 + 100, 2, 100, 15, "freq" );
fl_box1->labelsize( 9 );
fl_box1->box( FL_BORDER_BOX );
fl_box1->align( FL_ALIGN_INSIDE|FL_ALIGN_CENTER );

Fl_Box* fl_box2 = new Fl_Box( 2 + 200, 2, 100, 15, "center freq " );
fl_box2->labelsize( 9 );
fl_box2->align( FL_ALIGN_INSIDE|FL_ALIGN_CENTER );
fl_box2->box( FL_BORDER_BOX );
fl_scrol_headers->end();
*/

end();

header_align( 1 );

}








cl_favourite_wnd::~cl_favourite_wnd()
{


}




//void cl_favourite_wnd::resize(int x, int y, int w, int h )
//{
//Fl_Window::resize(x, y, w, h);
//}




//set 'do_nothing_if_state_is' to -1 to change state of all leds 
void cl_favourite_wnd::set_leds_column0( int do_nothing_if_state_is, bool new_state )
{
for( int i = 0; i < cn_favourite_row_max; i++ )
	{
	if( do_nothing_if_state_is >= 0 )
		{
		if( grp_row[i]->ctrl.ld_active->GetColIndex() == do_nothing_if_state_is ) continue;		//don't change state, if cur state matches 'do_nothing_if_state_is'

		grp_row[i]->ctrl.ld_active->ChangeCol( new_state );
		}
	else{
		grp_row[i]->ctrl.ld_active->ChangeCol( new_state );
		}



	if( i < vfav.size() )
		{
		if( new_state == 0 ) 
			{
			vfav[i].status &= ~en_fvs_sel;								//clear active
			vfav[i].status &= ~en_fvs_active;							//clear sel
			}

		if( new_state == 1 ) 
			{
			vfav[i].status &= ~en_fvs_active;							//clear active
			vfav[i].status |= en_fvs_sel;								//set sel
			}
			
		if( new_state == 2 ) 
			{
			vfav[i].status |= en_fvs_active;							//set active
			vfav[i].status &= ~en_fvs_sel;								//clear sel
			}
		}
	}
}




void cl_favourite_wnd::header_align( bool force )
{
int offsx = fl_scrol->xposition();											//get user's scroll pos, if any

if( !force ) if ( offsx == last_header_offsx ) return;

last_header_offsx = offsx;

printf("cl_favourite_wnd::header_align() offsx %d\n", offsx );


int xx, yy;
xx = grp_row[0]->hdr_dim[0].x - offsx;							
yy = grp_row[0]->hdr_dim[0].y;		
					
if( wnd_header ) wnd_header->position( xx, cn_header_posy );


//cycle one row's controls
//for( int i = 0; i < cn_favourite_box_header_max; i++ )
	{
//	int xx, yy;
//	xx = grp_row[0]->hdr_dim[i].x - offsx;							
//	yy = grp_row[0]->hdr_dim[i].y;							
	
//	bx_hdr[i]->position( xx, yy );
//	bx_hdr[i]->redraw();
	}

redraw();
}













void cl_favourite_wnd::clear()
{
	
for( int i = 0; i < cn_favourite_row_max; i++ )
	{
	// see 'st_favourite_row_ctrl_tag'
	for( int j = 0; j < cn_favourite_box_header_max; j++ )
		{
//		if( j == 0 )  grp_row[i]->ctrl.ld_active->ChangeCol( 0 );
		if( j == 1 )  grp_row[i]->ctrl.fi_name->value("");
		if( j == 2 )  grp_row[i]->ctrl.fi_freq->value("");
		if( j == 3 )  grp_row[i]->ctrl.fi_freq_center->value("");
		if( j == 4 )  grp_row[i]->ctrl.fi_demod_type->value("");
		if( j == 5 )  grp_row[i]->ctrl.fi_dev_manufacturer->value("");
		if( j == 6 )  grp_row[i]->ctrl.fi_date->value("");
		if( j == 7 )  grp_row[i]->ctrl.fi_time->value("");
		}
	}

}












void cl_favourite_wnd::store_to_sel_fav( st_favourite_freq_tag o )
{
if( row_sel >= 0 ) vfav[row_sel] = o;
set_via_vector( vfav, sz_demodulator_type, b_sanitise_favourites );
}





int cl_favourite_wnd::store_to_free_slot( st_favourite_freq_tag o )
{
int idx = find_free_slot( vfav );

if( idx != -1 )
	{
	vfav[idx] = o;
	set_via_vector( vfav, sz_demodulator_type, b_sanitise_favourites );
	printf("cl_favourite_wnd::store_to_free_slot() - saved to free slot %d\n", idx );
	}
else{
	printf("cl_favourite_wnd::store_to_free_slot() - failed to find a free slot to save into\n" );
	}


return idx;
}





/*
void cl_favourite_wnd::sort_move_blanks_to_end_for_param_sname()
{

vector<st_favourite_freq_tag>vtmp;


//remove/collect item if it has a blank param
loop0:
for( int i = 0; i < vfav.size(); i++ )
	{
	st_favourite_freq_tag o = vfav[i];
	if( o.sname.length() == 0 )
		{
		vfav.erase( vfav.begin() + 0 );
		vtmp.push_back( o );
		goto loop0;
		}
	}

//push collected into end of vector
for( int i = 0; i < vtmp.size(); i++ )
	{
	vfav.push_back( vtmp[i] );
	}
}
*/











/*
void cl_favourite_wnd::sort_move_blanks_to_end_for_param_sgroup()
{


vector<st_favourite_freq_tag>vtmp;


//remove/collect item if it has a blank param
loop0:
for( int i = 0; i < vfav.size(); i++ )
	{
	st_favourite_freq_tag o = vfav[i];
	if( o.sgroup.length() == 0 )
		{
		vfav.erase( vfav.begin() + 0 );
		vtmp.push_back( o );
		goto loop0;
		}
	}

//push collected into end of vector
for( int i = 0; i < vtmp.size(); i++ )
	{
	vfav.push_back( vtmp[i] );
	}
}
*/











void cl_favourite_wnd::sort_move_blanks_to_end_for_param( en_header_param_tag pp )
{

vector<st_favourite_freq_tag>vtmp;


//remove/collect item if it has a blank param
loop0:
for( int i = 0; i < vfav.size(); i++ )
	{
	st_favourite_freq_tag o = vfav[i];

	if( pp == en_hpm_sname )
		{
		if( o.sname.length() == 0 )
			{
			vfav.erase( vfav.begin() + 0 );
			vtmp.push_back( o );
			goto loop0;
			}
		}

	if( pp == en_hpm_sgroup )
		{
		if( o.sgroup.length() == 0 )
			{
			vfav.erase( vfav.begin() + 0 );
			vtmp.push_back( o );
			goto loop0;
			}
		}

	if( pp == en_hpm_sdev_manufacturer )
		{
		if( o.sdev_manufacturer.length() == 0 )
			{
			vfav.erase( vfav.begin() + 0 );
			vtmp.push_back( o );
			goto loop0;
			}
		}


	if( pp == en_hpm_scomment0 )
		{
		if( o.scomment0.length() == 0 )
			{
			vfav.erase( vfav.begin() + 0 );
			vtmp.push_back( o );
			goto loop0;
			}
		}

	if( pp == en_hpm_sdate )
		{
		if( o.sdate.length() == 0 )
			{
			vfav.erase( vfav.begin() + 0 );
			vtmp.push_back( o );
			goto loop0;
			}
		}

	if( pp == en_hpm_stime )
		{
		if( o.stime.length() == 0 )
			{
			vfav.erase( vfav.begin() + 0 );
			vtmp.push_back( o );
			goto loop0;
			}
		}
	}

//push collected into end of vector
for( int i = 0; i < vtmp.size(); i++ )
	{
	vfav.push_back( vtmp[i] );
	}
}









//set 'order' to -1, for toggle to next sort order
void cl_favourite_wnd::sort_order_change( int id, int order )
{


//first set all others to an unspecified or mixed order symbol
for( int i = 0; i < cn_favourite_box_header_max; i++ )
	{
	if( i != id )
		{
		bx_hdr[i]->sort_order = en_hso_mixed;
		bx_hdr[i]->b_sort_order_dimmed = 1;
		bx_hdr[i]->redraw();
		}
	}


if( order == -1 ) 	 				//toggle ?
	{
	if( bx_hdr[id]->sort_order == en_hso_increasing ) 
		{
		bx_hdr[id]->sort_order = en_hso_decreasing;
		bx_hdr[id]->b_sort_order_dimmed = 0;
		goto fin;
		}
		
	if( bx_hdr[id]->sort_order == en_hso_decreasing )
		{
		bx_hdr[id]->sort_order = en_hso_increasing;
		bx_hdr[id]->b_sort_order_dimmed = 0;
		goto fin;
		}
		
	if( bx_hdr[id]->sort_order == en_hso_mixed )
		{
		bx_hdr[id]->sort_order = en_hso_increasing;
		bx_hdr[id]->b_sort_order_dimmed = 0;
		goto fin;
		}
	}


if( order >= 0 )		//specific order to go to ?
	{
	bx_hdr[id]->sort_order = (en_header_sort_order_tag)order;
	bx_hdr[id]->b_sort_order_dimmed = 0;
	}

fin:

bool changed = 0;

//		std::stable_sort( vfav.begin(), vfav.end(), sort_favourite_by_name_inceasing );

if( bx_hdr[id]->param == en_hpm_sname )
	{
	if( bx_hdr[id]->sort_order == en_hso_increasing )
		{
		std::stable_sort( vfav.begin(), vfav.end(), sort_favourite_by_sname_increasing );
		changed = 1;
		}

	if( bx_hdr[id]->sort_order == en_hso_decreasing )
		{
		std::stable_sort( vfav.begin(), vfav.end(), sort_favourite_by_sname_decreasing );
//		sort_move_blanks_to_end_for_param_sname();
		
		sort_move_blanks_to_end_for_param( en_hpm_sname );
		changed = 1;
		}
	}


if( bx_hdr[id]->param == en_hpm_sgroup )
	{
	if( bx_hdr[id]->sort_order == en_hso_increasing )
		{
		std::stable_sort( vfav.begin(), vfav.end(), sort_favourite_by_sgroup_increasing );
		changed = 1;
		}

	if( bx_hdr[id]->sort_order == en_hso_decreasing )
		{
		std::stable_sort( vfav.begin(), vfav.end(), sort_favourite_by_sgroup_decreasing );
//		sort_move_blanks_to_end_for_param_sgroup();
		sort_move_blanks_to_end_for_param( en_hpm_sgroup );
		changed = 1;
		}
	}



if( bx_hdr[id]->param == en_hpm_freq )
	{
	if( bx_hdr[id]->sort_order == en_hso_increasing )
		{
		std::stable_sort( vfav.begin(), vfav.end(), sort_favourite_by_freq_increasing );
		changed = 1;
		}

	if( bx_hdr[id]->sort_order == en_hso_decreasing )
		{
		std::stable_sort( vfav.begin(), vfav.end(), sort_favourite_by_freq_decreasing );
		changed = 1;
		}
	}




if( bx_hdr[id]->param == en_hpm_freq_center )
	{
	if( bx_hdr[id]->sort_order == en_hso_increasing )
		{
		std::stable_sort( vfav.begin(), vfav.end(), sort_favourite_by_freq_center_increasing );
		changed = 1;
		}

	if( bx_hdr[id]->sort_order == en_hso_decreasing )
		{
		std::stable_sort( vfav.begin(), vfav.end(), sort_favourite_by_freq_center_decreasing );
		changed = 1;
		}
	}




if( bx_hdr[id]->param == en_hpm_demod_type )
	{
	if( bx_hdr[id]->sort_order == en_hso_increasing )
		{
		std::stable_sort( vfav.begin(), vfav.end(), sort_favourite_by_demod_type_increasing );
		changed = 1;
		}

	if( bx_hdr[id]->sort_order == en_hso_decreasing )
		{
		std::stable_sort( vfav.begin(), vfav.end(), sort_favourite_by_demod_type_decreasing );
		changed = 1;
		}
	}


if( bx_hdr[id]->param == en_hpm_sdev_manufacturer )
	{
	if( bx_hdr[id]->sort_order == en_hso_increasing )
		{
		std::stable_sort( vfav.begin(), vfav.end(), sort_favourite_by_sdev_manufacturer_increasing );
		changed = 1;
		}

	if( bx_hdr[id]->sort_order == en_hso_decreasing )
		{
		std::stable_sort( vfav.begin(), vfav.end(), sort_favourite_by_sdev_manufacturer_decreasing );

		sort_move_blanks_to_end_for_param( en_hpm_sdev_manufacturer );
		changed = 1;
		}
	}


if( bx_hdr[id]->param == en_hpm_sdate )
	{
	if( bx_hdr[id]->sort_order == en_hso_increasing )
		{
		std::stable_sort( vfav.begin(), vfav.end(), sort_favourite_by_sdate_increasing );
		changed = 1;
		}

	if( bx_hdr[id]->sort_order == en_hso_decreasing )
		{
		std::stable_sort( vfav.begin(), vfav.end(), sort_favourite_by_sdate_decreasing );
		sort_move_blanks_to_end_for_param( en_hpm_sdate );
		changed = 1;
		}
	}


if( bx_hdr[id]->param == en_hpm_stime )
	{
	if( bx_hdr[id]->sort_order == en_hso_increasing )
		{
		std::stable_sort( vfav.begin(), vfav.end(), sort_favourite_by_stime_increasing );
		changed = 1;
		}

	if( bx_hdr[id]->sort_order == en_hso_decreasing )
		{
		std::stable_sort( vfav.begin(), vfav.end(), sort_favourite_by_stime_decreasing );
		sort_move_blanks_to_end_for_param( en_hpm_stime );
		changed = 1;
		}
	}

if( changed )
	{
	wnd_fav->set_leds_column0( 1, 0 );			//clear all except if selected
	}

set_via_vector( vfav, sz_demodulator_type, b_sanitise_favourites);
bx_hdr[id]->redraw();
}









void cl_favourite_wnd::sanitise_using_limit( vector<st_favourite_freq_tag> &vv )
{
	
for( int i = 0; i < vv.size(); i++ )
	{
	st_favourite_freq_tag o = vv[i];
	
	if( o.freq < 10 ) o.freq = 10;
	if( o.freq > 100e9 ) o.freq = 100e9;

	if( o.freq_center < 10 ) o.freq_center = 10;
	if( o.freq_center > 100e9 ) o.freq_center = 100e9;

	if( o.dev_gain < 0.0001 ) o.dev_gain = 0.0001;
	if( o.dev_gain > 1000 ) o.dev_gain = 1000;

	if( o.audio_gain < 0.0001 ) o.audio_gain = 0.0001;
	if( o.audio_gain > 1000 ) o.audio_gain = 1000;

	if( o.if_freq < 0 ) o.if_freq = 0;
	if( o.if_freq > 100e9 ) o.if_freq = 100e9;

	if( o.iffreq_bw_low < 0 ) o.iffreq_bw_low = 0;
	if( o.iffreq_bw_low > 500e6 ) o.iffreq_bw_low = 500e6;

	if( o.iffreq_bw_high < o.iffreq_bw_low ) o.iffreq_bw_high = o.iffreq_bw_low;
	if( o.iffreq_bw_high > 1e9 ) o.iffreq_bw_high = 1e9;

	if( o.iq_gain < 0.001 ) o.iq_gain = 0.001;
	if( o.iq_gain > 20.5 ) o.iq_gain = 20.5;
	
	vv[i] = o;
	}
}




int cl_favourite_wnd::find_free_slot( vector<st_favourite_freq_tag>vv )
{
for( int i = 0; i < vv.size(); i++ )
	{
	if( vv[i].sname.length() == 0 ) return i;
	}
	
return -1;
}





void cl_favourite_wnd::set_via_vector( vector<st_favourite_freq_tag>vv, char szdemod[16][32], bool sanitise )
{
string s1;
mystr m1;

clear();


if( sanitise )
	{
	sanitise_using_limit( vv );	
	}


for( int i = 0; i < vv.size(); i++ )
	{
	// see 'st_favourite_row_ctrl_tag'
	for( int j = 0; j < cn_favourite_box_header_max; j++ )
		{
//			if( j == 0 )  grp_row[i]->ctrl.ld_active->ChangeCol( 0 );

		en_header_param_tag param = wnd_fav->bx_hdr[j]->param;

		if( param == en_hpm_active )  
			{
//			grp_row[i]->ctrl.ld_active->ChangeCol( 0 );
//			if( vv[i].status & en_fvs_sel ) grp_row[i]->ctrl.ld_active->ChangeCol( 1 );
			if( vv[i].status & en_fvs_active ) grp_row[i]->ctrl.ld_active->ChangeCol( 2 );
			}

		if( param == en_hpm_sname )  grp_row[i]->ctrl.fi_name->value( vv[i].sname.c_str() );

		if( param == en_hpm_freq )  
			{
			strpf( s1, "%" PRIi64 "", vv[i].freq );
			grp_row[i]->ctrl.fi_freq->value( s1.c_str() );
			}

		if( param == en_hpm_freq_center )
			{
			strpf( s1, "%" PRIi64 "", vv[i].freq_center );
			grp_row[i]->ctrl.fi_freq_center->value( s1.c_str() );
			}

		if( param == en_hpm_demod_type )
			{
			s1 = szdemod[ vv[i].demod_type ];
			grp_row[i]->ctrl.fi_demod_type->value( s1.c_str() );
			}

		if( param == en_hpm_b_use_dwn_aa )
			{
			if( vv[i].b_dwn_aa == 1 ) grp_row[i]->ctrl.ld_b_dwn_aa->ChangeCol( 1 );
			else grp_row[i]->ctrl.ld_b_dwn_aa->ChangeCol( 0 );
			}


		if( param == en_hpm_dwn_srate )
			{
//			strpf( s1, "%d", vv[i].if_freq );
			strpf( s1, "%" PRIi64 "", vv[i].i_dwn_srate );
			grp_row[i]->ctrl.fi_dwn_srate->value( s1.c_str() );
			}


		if( param == en_hpm_b_use_iffreq )
			{
//			strpf( s1, "%d", vv[i].if_freq );
			if( vv[i].b_use_iffreq == 1 ) grp_row[i]->ctrl.ld_b_use_iffreq->ChangeCol( 1 );
			else grp_row[i]->ctrl.ld_b_use_iffreq->ChangeCol( 0 );
			}

		if( param == en_hpm_if_freq )
			{
			strpf( s1, "%" PRIi64 "", vv[i].if_freq );
			grp_row[i]->ctrl.fi_if_freq->value( s1.c_str() );
			}

		if( param == en_hpm_b_bw_bpass )
			{
			if( vv[i].b_bw_bpass == 1 ) grp_row[i]->ctrl.ld_b_bw_bpass->ChangeCol( 1 );
			else grp_row[i]->ctrl.ld_b_bw_bpass->ChangeCol( 0 );
			}

		if( param == en_hpm_iffreq_bw_low )
			{
			strpf( s1, "%" PRIi64 "", vv[i].iffreq_bw_low );
			grp_row[i]->ctrl.fi_iffreq_bw_low->value( s1.c_str() );
			}

		if( param == en_hpm_iffreq_bw_high )
			{
			strpf( s1, "%" PRIi64 "", vv[i].iffreq_bw_high );
			grp_row[i]->ctrl.fi_iffreq_bw_high->value( s1.c_str() );
			}

		if( param == en_hpm_iffreq_bw_taps )
			{
			strpf( s1, "%d", vv[i].iffreq_bw_taps );
			grp_row[i]->ctrl.fi_iffreq_bw_taps->value( s1.c_str() );
			}


		if( param == en_hpm_dev_gain )
			{
			strpf( s1, "%f", vv[i].dev_gain );
			grp_row[i]->ctrl.fi_dev_gain->value( s1.c_str() );
			}

		if( param == en_hpm_b_agc )
			{
			if( vv[i].b_agc == 1 ) grp_row[i]->ctrl.ld_b_agc->ChangeCol( 1 );
			else grp_row[i]->ctrl.ld_b_agc->ChangeCol( 0 );
			}

		if( param == en_hpm_audio_gain )
			{
			strpf( s1, "%f", vv[i].audio_gain );
			grp_row[i]->ctrl.fi_audio_gain->value( s1.c_str() );
			}

		if( param == en_hpm_deemph )
			{
			grp_row[i]->ctrl.ld_deemph->ChangeCol( vv[i].deemph );
			}

		if( param == en_hpm_sgroup )  grp_row[i]->ctrl.fi_group->value( vv[i].sgroup.c_str() );
		
		
		if( param == en_hpm_sdev_manufacturer )  grp_row[i]->ctrl.fi_dev_manufacturer->value( vv[i].sdev_manufacturer.c_str() );
		if( param == en_hpm_scomment0 )  grp_row[i]->ctrl.fi_comment0->value( vv[i].scomment0.c_str() );

		if( param == en_hpm_sdate )  grp_row[i]->ctrl.fi_date->value( vv[i].sdate.c_str() );
		if( param == en_hpm_stime )  grp_row[i]->ctrl.fi_time->value( vv[i].stime.c_str() );

		if( param == en_hpm_bias_t )
			{
//			strpf( s1, "%d", vv[i].b_biat_t );
			if( vv[i].dev_bias_t == 1 ) grp_row[i]->ctrl.ld_bias_t->ChangeCol( 1 );
			else grp_row[i]->ctrl.ld_bias_t->ChangeCol( 0 );
			}

		if( param == en_hpm_direct_sampling )
			{
			if( vv[i].dev_direct_sampling == 0 ) grp_row[i]->ctrl.ld_direct_sampling->ChangeCol( 0 );
			if( vv[i].dev_direct_sampling == 1 ) grp_row[i]->ctrl.ld_direct_sampling->ChangeCol( 1 );
			if( vv[i].dev_direct_sampling == 2 ) grp_row[i]->ctrl.ld_direct_sampling->ChangeCol( 2 );
			}

		if( param == en_hpm_iq_gain )
			{
			strpf( s1, "%f", vv[i].iq_gain );
			grp_row[i]->ctrl.fi_iq_gain->value( s1.c_str() );
			}

		}
	}
redraw();
}





void cl_favourite_wnd::load_vector_from_gui_ctrls( vector<st_favourite_freq_tag>&vv, bool sanitise )
{
string s1;

int iv;
int64_t i64;
float fv;

vv.clear();

for( int i = 0; i < cn_favourite_row_max; i++ )
	{
	st_favourite_freq_tag o;
	
	o.demod_type = en_dmt_am;						//put somthing legal as its used to access: 'sz_demodulator_type'
	
//	o. grp_row[i]->ctrl.led_active->GetColIndex();

	if( grp_row[i]->ctrl.ld_active->GetColIndex() == 2 ) o.status |= en_fvs_active;
	else o.status &= ~en_fvs_active;
	
	
	s1 = grp_row[i]->ctrl.fi_freq->value();		
	sscanf( s1.c_str(), "%" PRIi64 "", &i64 );
	o.freq = i64;


	s1 = grp_row[i]->ctrl.fi_freq_center->value();	
	sscanf( s1.c_str(), "%" PRIi64 "", &i64 );
	o.freq_center = i64;


	s1 = grp_row[i]->ctrl.fi_if_freq->value();	
	sscanf( s1.c_str(), "%" PRIi64 "", &i64 );
	o.if_freq = i64;


	int iv = grp_row[i]->ctrl.ld_b_dwn_aa->GetColIndex();
	o.b_dwn_aa = iv;

	s1 = grp_row[i]->ctrl.fi_dwn_srate->value();	
	sscanf( s1.c_str(), "%" PRIi64 "", &i64 );
	o.i_dwn_srate = i64;


	iv = grp_row[i]->ctrl.ld_b_use_iffreq->GetColIndex();
	o.b_use_iffreq = iv;
	
	
	iv = grp_row[i]->ctrl.ld_b_bw_bpass->GetColIndex();
	o.b_bw_bpass = iv;


	s1 = grp_row[i]->ctrl.fi_iffreq_bw_low->value();	
	sscanf( s1.c_str(), "%" PRIi64 "", &i64 );
	o.iffreq_bw_low = i64;


	s1 = grp_row[i]->ctrl.fi_iffreq_bw_high->value();	
	sscanf( s1.c_str(), "%" PRIi64 "", &i64 );
	o.iffreq_bw_high = i64;


	s1 = grp_row[i]->ctrl.fi_iffreq_bw_taps->value();	
	sscanf( s1.c_str(), "%" PRIi64 "", &i64 );
	o.iffreq_bw_taps = i64;


	
	o.sname = grp_row[i]->ctrl.fi_name->value();

	o.sgroup = grp_row[i]->ctrl.fi_group->value();

	o.sdev_manufacturer = grp_row[i]->ctrl.fi_dev_manufacturer->value();
	o.scomment0 = grp_row[i]->ctrl.fi_comment0->value();


	s1 = grp_row[i]->ctrl.fi_demod_type->value();
	int idx = demod_type_idx_from_str( s1 );
	
	if( idx >= 0 )
		{
		o.demod_type = (en_demodulator_type_tag)idx;
		}



	iv = grp_row[i]->ctrl.ld_b_agc->GetColIndex();
	o.b_agc = iv;


	s1 = grp_row[i]->ctrl.fi_dev_gain->value();	
	sscanf( s1.c_str(), "%f", &fv );
	o.dev_gain = fv;


	s1 = grp_row[i]->ctrl.fi_audio_gain->value();	
	sscanf( s1.c_str(), "%f", &fv );
	o.audio_gain = fv;

	iv = grp_row[i]->ctrl.ld_deemph->GetColIndex();
	o.deemph = iv;

	o.sdate = grp_row[i]->ctrl.fi_date->value();

	o.stime = grp_row[i]->ctrl.fi_time->value();

	iv = grp_row[i]->ctrl.ld_bias_t->GetColIndex();
	o.dev_bias_t = iv;

	iv = grp_row[i]->ctrl.ld_direct_sampling->GetColIndex();
	o.dev_direct_sampling = iv;

	s1 = grp_row[i]->ctrl.fi_iq_gain->value();	
	sscanf( s1.c_str(), "%f", &fv );
	o.iq_gain = fv;

	vv.push_back( o );
	}

if( sanitise ) sanitise_using_limit( vv );
}





void cl_favourite_wnd::set_sel_row( unsigned int row )
{
row_sel = row;

wnd_fav->set_leds_column0( 2, 0 );			//just setting selection ?, don't change of the led control that is in 'active'

if( grp_row[row]->ctrl.ld_active->GetColIndex() != 2 ) grp_row[row]->ctrl.ld_active->ChangeCol( 1 );	//if not active, set as sel

redraw();
}








void cl_favourite_wnd::tick( float dt )
{

if( tick_secs_passed <= 0.00001f ) fl_scrol->scroll_to( 0, 0 );			//once only call

tick_secs_passed += dt;

bool force = 0;

if( w() != last_wid ) force = 1;
if( h() != last_hei ) force = 1;

last_wid = w();
last_hei = h();



header_align_time_count += dt;

if( ( header_align_time_count >= header_align_time_period ) || ( force ) )
	{
	header_align_time_count = 0;
	header_align( force );
	}

}







bool b_menu_show = 0;
//int hscol_x = 0;
//int hscol_y = 0;

int cl_favourite_wnd::handle( int e )
{
string s1;
int len;
char *szTmp;
bool need_redraw = 0;
bool dont_pass_on = 0;
int mousewheel;


if ( (e == FL_MOVE) || (e == FL_DRAG) )
	{
	mousex = Fl::event_x();
	mousey = Fl::event_y();

//	printf( "cl_favourite_wnd::handle() - FL_MOVE %d %d\n", mousex, mousey );


//	wnd_header->size( wnd_header->w(), 15 );
//	wnd_header->position( wnd_header->x(), 15 );
//	fl_scrol->position( wnd_header->x(), 35 );				
	menu_fav->size( menu_fav->w(), 25 );
//	fl_scrol->show();				
	
	if( mousey <= menu_hei ) 
		{
		if( mousex <= 250 ) 
			{
//	hscol_x = fl_scrol->xposition();
//	hscol_y = fl_scrol->yposition();

			b_menu_show = 1;
			wnd_header->hide();
//			wnd_header->size( wnd_header->w(), 15 );

//			fl_scrol->position( wnd_header->x(), 35 );			
//			fl_scrol_grp->position( wnd_header->x(), 35 );			
//			fl_scrol->position( hscol_x, hscol_y );			

//			fl_scrol->
			
			menu_fav->size( menu_fav->w(), 25 );
			menu_fav->show();
			}
		}

	if( mousey > menu_hei )
		{
		if( b_menu_show )		//only do below when required, if its done on every mouse move if stops tooltips from being shown
			{
			b_menu_show = 0;
			menu_fav->hide();
			
			wnd_header->show();
//			wnd_header->size( wnd_header->w(), 15 );
//			wnd_header->position( wnd_header->x(), 15 );
			

//			fl_scrol->hide();				

//			fl_scrol->position( wnd_header->x(), 35 );
//			fl_scrol_grp->position( wnd_header->x(), 35 );
			
						
//			fl_scrol->position( hscol_x, hscol_y );			

	//		menu_fav->size( menu_fav->w(), 25 );
			}
		}

	need_redraw = 1;
    dont_pass_on = 0;
	}


if ( e == FL_PUSH )
	{
	int ii = Fl::event_clicks();

	if( Fl::event_button() == 1 )											//left click
		{
//		do_callback( this, (void*)id0 );
		}
	}

//if ( need_redraw ) redraw();

if( dont_pass_on ) return 1;

return Fl_Double_Window::handle(e);
}

//----------------------------------------------------------------------



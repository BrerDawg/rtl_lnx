/*
Copyright (C) 2025 BrerDawg

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


//input_dropbox.cpp

//v1.01   2021-jul-02		//does not support label, also needs 2 callbacks defined for full functionality, one for 'set_callback()' and one for 'fi_text'
							//see SearchIt for an example of use...

//v1.02   2022-nov-27		//added: 'redraw()' to 'add( string ss )',  added: 'ignore_blank_entries'

//v1.03   2024-mar-13		//added: 'add_at_head()'  'trim_entry_count_to()'

//v1.04   2025-jul-18		//added 'p_callback = 0;' during obj creation, needed this to avoid crash if user did not set it to a callback	
							//added 'ingore_duplicate'   'ignore_case'   'search()'
							//added 'sort_when_adding'   'sort_descending'  for auto sorting    and  'sort()'  for manual sorting
							//added 'mouse_move_scrolling' to 'dropdown' widget
							//added 'bring_last_selection_to_top'  will NOT work as expected if 'sort_when_adding' is set
							
//v1.05   2025-aug-22		//added 'allow_dropdown_to_float_y'
//v1.06   2026-apr-03		//added 'set_with_c_esc_str()' 'get_as_c_esc_str()', useful for saving/recalling to and from a profile ini
//v1.07   2026-sep-11		//moded 'add_at_head()' to make an existing entry appear at head of list


#include "input_dropbox.h"



int sort_input_dropbox_entry_increasing( const string &o0, const string &o1 );
int sort_input_dropbox_entry_decreasing( const string &o0, const string &o1 );



/* 
//-------------------------- EXAMPLE CODE ------------------------------


void cb_idb_search( void* obj, void* args )
{
input_dropbox* o = (input_dropbox*) obj;

string s1 = "??";

if( o->hover_idx < o->vstr.size() )
	{
	s1 = o->vstr[o->hover_idx];
	}
printf("cb_idb_search() - input_dropbox o->hover_idx %d  '%s'\n", o->hover_idx, s1.c_str() );

}






void cb_idb_search_fi_text( void* w, void* v )
{
Fl_Input* o = (Fl_Input*) w;

printf("cb_idb_search_fi_text() - '%s'\n", o->value() );

cb_btSearch( 0, 0 );
}


Fl_Box *bx_label = new Fl_Box( 10, meMain->h()+37, 60, 20, "Search For:" );
bx_label->labelsize( 11 );

idb_search = new input_dropbox( 10, meMain->h()+55, 250, 20, "");
idb_search->expand_max( idb_search->w(), 300 );
//idb_search->fi_text->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);
//idb_search->fi_text->textfont( 4 );
//idb_search->fi_text->textsize( 12 );
idb_search->fi_text->tooltip("Enter Search string here.\nFor file searches (find): ? is any char placeholder, * is any str before or after depending on where its placed.\n-----------------\nFor text searches (grep): use . for any char placeholder - \ne.g: gr.y (for grey or gray),\nuse .* for any str before or after depending on where its placed -\ne.g: synthesi.* (for synthesizer, synthesiser, synthesized)\n-----------------\nuse EGrep for e.g: color|colour\n(see wiki regex).\n\nPress help button for more details.");
idb_search->bt_dropdown->tooltip("Enter Search string here.\nFor file searches (find): ? is any char placeholder, * is any str before or after depending on where its placed.\n-----------------\nFor text searches (grep): use . for any char placeholder - \ne.g: gr.y (for grey or gray),\nuse .* for any str before or after depending on where its placed -\ne.g: synthesi.* (for synthesizer, synthesiser, synthesized)\n-----------------\nuse EGrep for e.g: color|colour\n(see wiki regex).\n\nPress help button for more details.");
idb_search->set_callback( cb_idb_search, (void *)idb_search, (void *)0 );
idb_search->fi_text->callback( (void*)cb_idb_search_fi_text, (void*)0 );
idb_search->fi_text->when( FL_WHEN_ENTER_KEY | FL_WHEN_NOT_CHANGED );
//idb_search->textsize(12);
//idb_search->labelsize(12);
idb_search->box( FL_BORDER_BOX );
//----------------------------------------------------------


*/





//----------------------------------------------------------


void cb_bt_input_dropbox_dropdown( Fl_Widget* w, void*v )
{
input_dropbox* o = (input_dropbox*) v;
o->expand = !o->expand;
o->expand_adj( o->expand );
}



//note this class does not draw label, use an external Fl_Box for labelling
input_dropbox::input_dropbox( int x, int y, int w, int h, const char *label ) : Fl_Window( x, y, w, h, label )
{
bt_dropdown = new Fl_Button( w-20, 0, 20, 16, "@-22>" );
bt_dropdown->callback( cb_bt_input_dropbox_dropdown, (void*)this );

fi_text = new Fl_Input( 1, 0, w-22, 18, label );
fi_text->textsize(12);

//Fl_Box *bx_label = new Fl_Box( 0, 0, 100, 25, "Test" );

p_callback = 0;															//v1.04


posx = x;
posy = y;

normal_hei = h;

expand = 0;
expand_wid = w;
expand_hei = h + 50;

dropbox_posy = 20;
dropbox_line0_posy = 9;
line_hei = 12;

textfont = 4;
textsize = 10;

uchar uir, uig, uib;
 
Fl::get_color( FL_BACKGROUND_COLOR, uir, uig, uib );

col_bkgd_r = uir;
col_bkgd_g = uig;
col_bkgd_b = uib;

col_text_r = 0;
col_text_g = 0;
col_text_b = 0;

col_text_hov_r = 255;
col_text_hov_g = 255;
col_text_hov_b = 255;

col_hov_r = 240;
col_hov_g = 171;
col_hov_b = 128;

col_hov_r = 0;
col_hov_g = 0;
col_hov_b = 180;

scroll_line = 0;

//inside = 0;
hover_idx = -1;

page_up_step = -5;
page_down_step = 5;

can_delete = 1;

end();

dropdown = new input_dropbox_popup( x, y+dropbox_posy, w, 100 );
end();
dropdown->set_owner( this );
dropdown->hide();


ignore_blank_entries = 1;
ingore_duplicate = 1;
ignore_case = 1;
sort_when_adding = 0;
sort_descending = 0;
dropdown->mouse_move_scrolling = 1;
bring_last_selection_to_top = 1;
allow_dropdown_to_float_y = 1;

end();

}







void input_dropbox::expand_max( int maxx, int maxy )
{
expand_wid = maxx;
expand_hei = maxy;

if( dropdown != 0 ) dropdown->resize( dropdown->x(), dropdown->y(), expand_wid, expand_hei);
}





void input_dropbox::expand_adj( bool expand_in )
{
expand = expand_in;

if( expand ) 
	{
	if( allow_dropdown_to_float_y )										
		{
		Fl_Widget *ow = (Fl_Widget*) parent();
		
		if( dropdown->y() + dropdown->h() > ow->h() )
			{
			dropdown->position( dropdown->x(), ow->h() - expand_hei );
			}
		}

	Fl_Group *op = parent();
	dropdown->show();													//make this obj show above other objs in parent
	Fl::grab( dropdown );
	}
else{
	dropdown->hide();
	hover_idx = -1;
	Fl::grab(0);
	}

redraw();
}







int input_dropbox::search( string ss, bool ignorecase )
{
string s1;
mystr m1;


int idx = -1;

if( ignorecase )
	{
	m1 = ss;
	m1.to_upper( s1 );
	ss = s1;
	}

for( int i = 0; i < vstr.size(); i++ )
	{
	s1 = vstr[i];
	m1 = s1;
	if( ignorecase )
		{
		m1.to_upper( s1 );
		}
	
	if( s1.compare( ss ) == 0 ) return i;
	}
return idx;

}










//gets all entries as a C escape str using '\0a' as a delimiter
//see also 'set_with_c_esc_str()'
void input_dropbox::get_as_c_esc_str( string &ss )					//v1.06
{
string s1;
mystr m1;


ss = "";


for( int i = 0; i < vstr.size(); i++ )
	{
	s1 += vstr[i];
	s1 += '\n';
	}

m1 = s1;

m1.StrToEscMostCommon3();
ss = m1.szptr();
}




//takes a C escape str, converts escaping to normal chars, creates new entries using '\n' as a delimiter
//returns number of loaded entries, else 0
//see also 'get_as_c_esc_str()'
int input_dropbox::set_with_c_esc_str( string ss, bool clear_first )		//v1.06
{
string s1;
mystr m1;

m1 = ss;
m1.EscToStr();

if(clear_first) clear();

vector<string>vs;


if( !m1.LoadVectorStrings( vs, '\n' ) ) return 0;

for( int i = 0; i < vs.size(); i++ )
	{
	add( vs[i] );
	}

return vstr.size();
}









void input_dropbox::add( string ss )
{
if( ignore_blank_entries )
	{
	if( ss.length() == 0 ) return;	
	}


if( ingore_duplicate )
	{
	int idx = search( ss, ignore_case );

	if( idx == -1 ) vstr.push_back( ss );
	}
else{
	vstr.push_back( ss );
	}


if( sort_when_adding ) 
	{
	if( sort_descending ) std::stable_sort( vstr.begin(), vstr.end(), sort_input_dropbox_entry_decreasing );
	else std::stable_sort( vstr.begin(), vstr.end(), sort_input_dropbox_entry_increasing );
	}


redraw();							//v1.02
}








void input_dropbox::add_at_head( string ss )							//v1.03
{
if( ignore_blank_entries )
	{
	if( ss.length() == 0 ) return;	
	}


if( ingore_duplicate )
	{
	int idx = search( ss, ignore_case );

	if( idx == -1 ) 
		{
		vstr.insert( vstr.begin() + 0, ss );
		}
	else{																//v1.07, already in list, make entry appear at head
		erase_entry( idx );												
		vstr.insert( vstr.begin() + 0, ss );
		}
	}
else{
	vstr.insert( vstr.begin() + 0, ss );
	}

if( sort_when_adding ) 
	{
	if( sort_descending ) std::stable_sort( vstr.begin(), vstr.end(), sort_input_dropbox_entry_decreasing );
	else std::stable_sort( vstr.begin(), vstr.end(), sort_input_dropbox_entry_increasing );
	}

redraw();
}








void input_dropbox::sort( bool bdesending )								//v1.04
{
if( bdesending ) std::stable_sort( vstr.begin(), vstr.end(), sort_input_dropbox_entry_decreasing );
else std::stable_sort( vstr.begin(), vstr.end(), sort_input_dropbox_entry_increasing );
}






//trims 'vstr' size to get it down to 'cnt' entries if required
//set 'b_from_head' to trim starting from vstr[0], else trim is done starting at vstr[ vstr.size() - 1 ] (the last entry seen in dropdown) and works backwards towards vstr[0]
void input_dropbox::trim_entry_count_to( unsigned int cnt, bool b_from_head )				//v1.03
{

int trim_cnt = vstr.size() - cnt;

if( trim_cnt == 0 ) return;

if( b_from_head )
	{
	for( int i = 0; i < trim_cnt; i++ )
		{
		if( vstr.size() == 0 ) break;
		vstr.erase( vstr.begin() + 0 );
		}
	}
else{
	for( int i = 0; i < trim_cnt; i++ )
		{
		if( vstr.size() == 0 ) break;
		vstr.erase( vstr.end() + 0 );
		}
	}
}






void input_dropbox::clear()
{
//printf( "input_dropbox::clear() !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );

vstr.clear();
redraw();
}


const char* input_dropbox::value()
{
return fi_text->value();

}



void input_dropbox::value( string ss )
{
fi_text->value( ss.c_str() );
}







void input_dropbox::select_entry( unsigned int idx )
{
if( idx < vstr.size() )
	{
	fi_text->value( vstr[idx].c_str() );
	}
}





bool input_dropbox::erase_entry( unsigned int idx )
{
if( idx < vstr.size() )
	{
	vstr.erase( vstr.begin() + idx );
	}
else{
	return 0;
	}
return 1;
}




void input_dropbox::scroll_into_view( unsigned int idx )
{
if( idx >= vstr.size() ) return;

int loop_cnt = 0;
loop:
int posy = dropbox_line0_posy + idx*line_hei - (scroll_line)*line_hei;

loop_cnt++;
if( loop_cnt > 5000 ) return;											//just for safety

if( posy >= dropdown->h() - line_hei*0.3f )
	{
	scroll_line+=1;
	if( scroll_line >= vstr.size() ) scroll_line = vstr.size() - 1;
	goto loop;
	}
	
if( posy < line_hei ) 
	{
	scroll_line-=1;
	if( scroll_line < 0 ) scroll_line = 0;
	goto loop;
	}
//printf( "input_dropbox::scroll_into_view(): idx %d posy %d scroll_line %d\n", idx, posy, scroll_line );	

}





/*
void input_dropbox::set_left_click_cb( void (*p_cb)( My_Input_Wheel*, void* ), void *args )
{
left_click_p_cb = p_cb;
left_click_cb_args = args;
}


void input_dropbox::set_left_double_click_cb( void (*p_cb)( My_Input_Wheel*, void* ), void *args )
{
left_double_click_p_cb = p_cb;
left_double_click_cb_args = args;
}



void input_dropbox::set_right_click_cb( void (*p_cb)( My_Input_Wheel*, void* ), void *args )
{
right_click_p_cb = p_cb;
right_click_cb_args = args;
}


void input_dropbox::set_right_double_click_cb( void (*p_cb)( My_Input_Wheel*, void* ), void *args )
{
right_double_click_p_cb = p_cb;
right_double_click_cb_args = args;
}




void input_dropbox::set_mousewheel_cb( void (*p_cb)( My_Input_Wheel*, void*, int ), void *args )
{
mousewheel_p_cb = p_cb;
mousewheel_cb_args = args;
}



void input_dropbox::set_left_click_release_cb( void (*p_cb)( My_Input_Wheel*, void* ), void *args )
{
left_click_release_p_cb = p_cb;
left_click_release_cb_args = args;
}



void input_dropbox::set_right_click_release_cb( void (*p_cb)( My_Input_Wheel*, void* ), void *args )
{
right_click_release_p_cb = p_cb;
right_click_release_cb_args = args;
}

*/


















void input_dropbox::set_callback( void (*p_cb)( void*, void* ), void *obj_in, void *args_in )
{
p_callback = p_cb;
cb_obj = obj_in;
cb_args = args_in;
}












void input_dropbox::draw()
{
Fl_Group::draw();
return;

}





int input_dropbox::get_hover_idx( int xx, int yy )
{

for( int i = 0; i < vposy.size(); i++ )
	{
	int x0 = vposx[i];
	int y0 = vposy[i];

	int x1 = w();
	int y1 = y0 + line_hei;

	if( ( xx >= x0 ) && ( xx <= x1 )  )
		{
		if( ( yy >= y0 ) && ( yy <= y1 ) )
			{
//printf( "get_hover_idx() i %d\n", i + scroll_line );
			return i + scroll_line;
			}
		}
	}

return -1;
}





int input_dropbox::handle( int e )
{
string s1;
int len;
char *szTmp;
bool need_redraw = 0;
bool dont_pass_on = 0;
//int mousewheel;

//printf( "handle() %d\n", e );	



if ( e == FL_LEAVE )
	{
//	need_redraw = 1;
 	dont_pass_on = 0;
	}


if ( e == FL_ENTER )
	{
//	need_redraw = 1;
    dont_pass_on = 0;
    }


if ( e == FL_FOCUS )
	{
	dont_pass_on = 0;
	}





if (  e & FL_MOVE )	
	{
	
//printf( "mousex: %d %d\n", mousex, mousey );	
	
//	need_redraw = 1;
    dont_pass_on = 0;
	}



if ( e == FL_PUSH )	
	{
//	need_redraw = 1;
//    dont_pass_on = 1;
	}


if ( e == FL_RELEASE )	
	{
//	need_redraw = 1;
//   dont_pass_on = 1;
	}

if ( e == FL_MOUSEWHEEL )
	{
//	need_redraw = 1;
    dont_pass_on = 0;
	}



if ( need_redraw ) redraw();

if( dont_pass_on ) return 1;

return Fl_Window::handle(e);
}

//----------------------------------------------------------













//----------------------------------------------------------
//DON'T USE this class directly, its only for 'class input_dropbox'
input_dropbox_popup::input_dropbox_popup( int x,int y,int w, int h,const char *label ) : Fl_Menu_Window( x, y, w, h, label )
{
wdg_owner = 0;
end();
}


void input_dropbox_popup::set_owner( input_dropbox* ow )
{
wdg_owner = ow;
}






void input_dropbox_popup::draw()
{

Fl_Window::draw();

input_dropbox *o = wdg_owner;

if( o == 0 ) return;


int iF = fl_font();
int iS = fl_size();


string s1;

fl_color( o->col_bkgd_r, o->col_bkgd_g, o->col_bkgd_b );
//fl_line_style ( FL_SOLID, 0 );        	 		//for windows you must set line setting after the colour, see the manual


fl_rectf( 0, 0 , w() , h() );

fl_color( 0, 0, 0 );
fl_rect( 0 , 0 , w() , h() );

fl_color( o->col_text_r, o->col_text_g, o->col_text_b );

int xx = 2;
int yy = o->dropbox_line0_posy;


o->vposx.clear();
o->vposy.clear();

fl_font( o->textfont, o->textsize );
if( o->expand )
	{
	for( int i = o->scroll_line; i < o->vstr.size(); i++ )
		{
//		printf( "%d %s\n", i, vstr[i].c_str() );	
		
		if( o->hover_idx == i ) 
			{
			fl_color( o->col_hov_r, o->col_hov_g, o->col_hov_b );
			fl_rectf( 1, yy-o->line_hei+3, w()-2, o->line_hei );
			fl_color( o->col_text_hov_r, o->col_text_hov_g, o->col_text_hov_b );
			}
		else{
			fl_color( o->col_text_r, o->col_text_g, o->col_text_b );
			}

		s1 = o->vstr[i];
		fl_draw( s1.c_str(), xx, yy );
	
		o->vposx.push_back( xx );
		o->vposy.push_back( yy-o->line_hei+3 );
		yy += o->line_hei;
		}
	}


fl_font( iF, iS );
}









int input_dropbox_popup::handle( int e )
{
string s1;
int len;
char *szTmp;
bool need_redraw = 0;
bool dont_pass_on = 0;

input_dropbox *o = wdg_owner;
if( o == 0 )
	{
	return Fl_Window::handle(e);
	}

//printf( "handle() %d\n", e );	

if ( e == FL_ENTER )
	{
//	inside = 0;
//	printf( "input_dropbox_popup::FL_ENTER\n" );


//	take_focus();
//	Fl::focus( this->parent() );
	need_redraw = 1;
	dont_pass_on = 1;
	}

if ( e == FL_LEAVE )
	{
//	printf( "input_dropbox_popup::FL_LEAVE\n" );
	o->expand_adj( 0 );
	need_redraw = 1;
	dont_pass_on = 1;
	}
if ( e == FL_MOVE )	
	{
	int mousex = Fl::event_x();
	int mousey = Fl::event_y();

	o->hover_idx = o->get_hover_idx( mousex, mousey );

	if( mouse_move_scrolling )											//v1.04
		{
		o->scroll_into_view( o->hover_idx );
		}
	
//printf( "mousex: %d %d\n", mousex, mousey );	
	
	need_redraw = 1;
    dont_pass_on = 1;
	}


if ( e == FL_PUSH )
	{
	need_redraw = 1;
	dont_pass_on = 1;
	}


if ( e == FL_RELEASE )	
	{

	if( o->expand )
		{
		if( ( o->hover_idx >= 0 ) && ( o->hover_idx < o->vstr.size() ) )
			{
			o->fi_text->value( o->vstr[ o->hover_idx].c_str() );
			if( o->p_callback ) o->p_callback( o->cb_obj, o->cb_args);
			
			if(o->bring_last_selection_to_top)
				{
				string s1 = o->vstr[o->hover_idx];
				o->erase_entry( o->hover_idx );
				o->add_at_head( s1 );	
				}
			}
		}

	Fl::grab(0);
	o->expand_adj( 0 );

	Fl::focus( o->fi_text );


//	take_focus();
	need_redraw = 1;
    dont_pass_on = 1;
	}








	
if ( ( e == FL_KEYDOWN ) )
	{
	int key = Fl::event_key();

	if( o->expand )
		{
		if( key == FL_Enter )
			{
			if( ( o->hover_idx >= 0 ) && ( o->hover_idx < o->vstr.size() ) )
				{
				o->fi_text->value( o->vstr[ o->hover_idx].c_str() );
				if( o->p_callback ) o->p_callback( o->cb_obj, o->cb_args);
				}
			Fl::grab(0);
			o->expand_adj( 0 );
			Fl::focus( o->fi_text );
			}

		if( key == FL_Up )
			{
			o->hover_idx--;
			if( o->hover_idx < 0 ) o->hover_idx = 0;
			o->scroll_into_view( o->hover_idx );
			}

		if( key == FL_Down )
			{
			o->hover_idx++;
			if( o->hover_idx >= o->vstr.size() ) 
				{
				o->hover_idx = o->vstr.size() - 1;
				}
			o->scroll_into_view( o->hover_idx );
			}



		if( key == FL_Page_Up )
			{
			o->hover_idx += o->page_up_step;
			if( o->hover_idx < 0 ) o->hover_idx = 0;
			o->scroll_into_view( o->hover_idx );
			}

		if( key == FL_Page_Down )
			{
			o->hover_idx += o->page_down_step;
			if( o->hover_idx >= o->vstr.size() ) o->hover_idx = o->vstr.size() - 1;
			if( o->hover_idx < 0 ) o->hover_idx = 0;
			o->scroll_into_view( o->hover_idx );
			}

		if( key == FL_Home )
			{
			o->hover_idx = 0;
			o->scroll_into_view( o->hover_idx );
			}

		if( key == FL_End )
			{
			o->hover_idx = o->vstr.size() - 1;
			o->scroll_into_view( o->hover_idx );
			}


		if( key == FL_Delete )
			{
			if( o->can_delete )
				{
				o->erase_entry( o->hover_idx );
				}
			}
		}
	need_redraw = 1;
    dont_pass_on = 1;
	}

if ( e == FL_MOUSEWHEEL )
	{
	int mousewheel = Fl::event_dy();
	
	if( mousewheel > 0 ) o->scroll_line++;
	else o->scroll_line--;
	
	if( o->scroll_line < 0 ) o->scroll_line = 0;
	
	if( o->scroll_line >= o->vstr.size() ) o->scroll_line = o->vstr.size() - 1;
	
//printf( "scroll_line: %d\n", scroll_line );	
	need_redraw = 1;
    dont_pass_on = 1;
	}


if ( need_redraw ) redraw();

if( dont_pass_on ) return 1;

return Fl_Menu_Window::handle(e);
}

//----------------------------------------------------------




//a-->z
int sort_input_dropbox_entry_increasing( const string &o0, const string &o1 )
{
if( (o0.length() == 0 ) && ( o1.length() == 0) ) return 0;	//both empty
if( o0.length() == 0 ) return 0;
if( o1.length() == 0 ) return 1;

if( o0.compare( o1 ) >= 0 ) return 0; 					//return 0 if o0 '>=' o1	
	
return 1;												//return 1 if o0 '<' o1			!! important to get this right for 'stable_sort()'
}


//z-->a ( o0 and o1 are reversed when comparing)
int sort_input_dropbox_entry_decreasing( const string &o0, const string &o1 )
{
if( (o1.length() == 0 ) && ( o0.length() == 0) ) return 0;	//both empty
if( o1.length() == 0 ) return 0;
if( o0.length() == 0 ) return 1;

if( o1.compare( o0 ) >= 0 ) return 0; 					//return 0 if o1 '>=' o0	
	
return 1;												//return 1 if o1 '<' o0			!! important to get this right for 'stable_sort()'
}
//--------




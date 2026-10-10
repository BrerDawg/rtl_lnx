//gc_input_multiline.cpp

//v1.01		30-11-11		//
//v1.02		06-01-2021		//added: 'gc_input_multiline_adj()' function to allow adj text details font/size/color etc
							//also added 'delete wnd;'

//v1.03		05-03-2022		//added to 'gc_input_multiline()' and 'gc_input_multiline_adj()', window pos and size (xx,yy,wid,hei) are now C references to allow wnd details to be maintained between calls,
							//also added 'bselect_text' 
//v1.04		27-05-2022		//added ability to specify font details to 'gc_input_multiline()', see 'font_type, font_size'

//v1.05		01-sep-2026		//added to 'gc_input_multiline_adj' and 'gc_input_multiline' and 'gc_input_multiline_wnd' the option 'b_allow_resize'

//v1.06		10-oct-2026		//added 'gc_input_multiline_adj_non_modal()'  
							//added to 'gc_input_multiline::set_text()' for use with non modal version	
							//added 'gc_input_multiline::add_button()'

#include "gc_input_multiline.h"





void cb_gc_input_multiline_ok( Fl_Widget *, void * );
void cb_gc_input_multiline_cancel( Fl_Widget *, void * );



//v1.06
//REFER FOR USAGE: gc_calc's 'cb_wrksheet_octave_log()'  and  'cb_wnd_log_added_button()'

//this obj sets a user supplied wnd pointer, and also zeroes it on wnd closure via buttons, the pointer is
// to a non modal wnd and can be used by user to dynamically change the text, using 'set_text()'
//CHECK pointer has not been ZEROED before use


//user buttons can be ADDED with 'add_button()'


//see also: 'gc_input_multiline(), 'gc_input_multiline_adj()'
//set 'fonttype' < 0 to not change fonttype
//set 'fontsize' < 0 to not change fontsize
//set 'bkgd_r' < 0 to not change color of backgound
//set 'txt_r' < 0 to not change color of text

//see also: 'gc_input_multiline(), 'gc_input_multiline_adj()',  'gc_input_multiline_adj_non_modal()'
bool gc_input_multiline_adj_non_modal( int &xx, int &yy, int &wid, int &hei, int fonttype, int fontsize, int bkgd_r, int bkgd_g, int bkgd_b, int txt_r, int txt_g, int txt_b, const char *title, string label, string *ret, bool bselect_text, bool b_allow_resize, gc_input_multiline_wnd **ptr_wnd )
{
bool ok_pressed = 0;

gc_input_multiline_wnd *wnd = new gc_input_multiline_wnd( xx, yy, wid, hei, title, label, ret, bselect_text, fonttype, fontsize, b_allow_resize );

*ptr_wnd = wnd;			//this can be zeroed by this obj when wnd is closed by user, allows user to detect it closure and avoid trying to make wnd changes  such as calling 'set_text()' 

//if( fonttype >= 3 ) wnd->te->textfont( fonttype );
//if( fontsize >= 3 ) wnd->te->textsize( fontsize );
wnd->te->hide();
wnd->te->show();
wnd->te->resize( wnd->te->x(), wnd->te->y(), wnd->te->w()-5, wnd->te->h()-5 ); 	//this makes the text appear stable, without it, when you click in 'te' 
wnd->te->resize( wnd->te->x(), wnd->te->y(), wnd->te->w()+5, wnd->te->h()+5 ); 	//the text would jump slighty as though vert line spacing was adj (annoying effect)

if( bkgd_r >= 0 ) wnd->te->color( fl_rgb_color( bkgd_r, bkgd_g, bkgd_b )  );
if( txt_r >= 0 ) wnd->te->textcolor( fl_rgb_color( txt_r, txt_g, txt_b )  );

//wnd->set_modal();
wnd->show();

//while( wnd->shown() )
//	{
//	Fl::wait();
//	}

if( wnd->ok_pressed )
	{
	xx = wnd->x();														//v1.03
	yy = wnd->y();
	wid = wnd->w();
	hei = wnd->h();

	*ret = wnd->tb->text();
	delete wnd;															//v1.02
	*ptr_wnd = 0;
	return 1;
	}


if( wnd->cancel_pressed )												//v1.06
	{
//	*ret = wnd->tb->text();

	xx = wnd->x();														//v1.03
	yy = wnd->y();
	wid = wnd->w();
	hei = wnd->h();

	delete wnd;															//v1.02
	*ptr_wnd = 0;
	return 0;
	}
																		//v1.02
return 0;
}











//v1.02
//see also: 'gc_input_multiline(), 'gc_input_multiline_adj_non_modal()'
//set 'fonttype' < 0 to not change fonttype
//set 'fontsize' < 0 to not change fontsize
//set 'bkgd_r' < 0 to not change color of backgound
//set 'txt_r' < 0 to not change color of text

bool gc_input_multiline_adj( int &xx, int &yy, int &wid, int &hei, int fonttype, int fontsize, int bkgd_r, int bkgd_g, int bkgd_b, int txt_r, int txt_g, int txt_b, const char *title, string label, string *ret, bool bselect_text, bool b_allow_resize )
{
bool ok_pressed = 0;

gc_input_multiline_wnd *wnd = new gc_input_multiline_wnd( xx, yy, wid, hei, title, label, ret, bselect_text, fonttype, fontsize, b_allow_resize );

//if( fonttype >= 3 ) wnd->te->textfont( fonttype );
//if( fontsize >= 3 ) wnd->te->textsize( fontsize );
wnd->te->hide();
wnd->te->show();
wnd->te->resize( wnd->te->x(), wnd->te->y(), wnd->te->w()-5, wnd->te->h()-5 ); 	//this makes the text appear stable, without it, when you click in 'te' 
wnd->te->resize( wnd->te->x(), wnd->te->y(), wnd->te->w()+5, wnd->te->h()+5 ); 	//the text would jump slighty as though vert line spacing was adj (annoying effect)

if( bkgd_r >= 0 ) wnd->te->color( fl_rgb_color( bkgd_r, bkgd_g, bkgd_b )  );
if( txt_r >= 0 ) wnd->te->textcolor( fl_rgb_color( txt_r, txt_g, txt_b )  );

wnd->set_modal();
wnd->show();

while( wnd->shown() )
	{
	Fl::wait();
	}

if( wnd->ok_pressed )
	{
	xx = wnd->x();														//v1.03
	yy = wnd->y();
	wid = wnd->w();
	hei = wnd->h();

	*ret = wnd->tb->text();
	delete wnd;															//v1.02
	return 1;
	}

if( wnd->cancel_pressed )												//v1.06
	{
//	*ret = wnd->tb->text();

	xx = wnd->x();														//v1.03
	yy = wnd->y();
	wid = wnd->w();
	hei = wnd->h();

	delete wnd;															//v1.02
	return 0;
	}

																		//v1.02
return 0;
}








//see also: 'gc_input_multiline_adj()',  'gc_input_multiline_adj_non_modal()'
//v1.05
bool gc_input_multiline( int &xx, int &yy, int &wid, int &hei, const char *title, string label, string *ret, bool bselect_text, int font_type, int font_size, bool b_allow_resize )
{

gc_input_multiline_wnd *wnd = new gc_input_multiline_wnd( xx, yy, wid, hei, title, label, ret, bselect_text, font_type, font_size, b_allow_resize );

wnd->set_modal();
wnd->show();

while( wnd->shown() )
	{
	Fl::wait();
	}

if( wnd->ok_pressed )
	{
	xx = wnd->x();														//v1.03
	yy = wnd->y();
	wid = wnd->w();
	hei = wnd->h();
	
	*ret = wnd->tb->text();
	delete wnd;															//v1.02
	return 1;
	}

if( wnd->cancel_pressed )												//v1.06
	{
//	*ret = wnd->tb->text();

	xx = wnd->x();														//v1.03
	yy = wnd->y();
	wid = wnd->w();
	hei = wnd->h();

	delete wnd;															//v1.02
	return 0;
	}


//delete wnd;															//v1.02
return 0;
}














//don't use this directly use: 'gc_input_multiline()'  'gc_input_multiline_adj()' or 'gc_input_multiline_adj_non_modal()'
gc_input_multiline_wnd::gc_input_multiline_wnd( int xx, int yy, int wid, int hei, const char *title, string label, string *ret, bool bselect_text, int font_type, int font_size, bool b_allow_resize ) : Fl_Window( xx, yy, wid, hei, title )
{
ok_pressed = 0;
cancel_pressed = 0;


tb = new Fl_Text_Buffer;
te = new Fl_Text_Editor( 5, 5, wid - 10 , hei - 35 );

if( font_type >= 3 ) te->textfont( font_type );
if( font_size >= 3 ) te->textsize( font_size );

te->buffer( tb );
tb->text( ret->c_str() );
if( bselect_text ) tb->select( 0, tb->length() );						//v1.03

//fi = new Fl_Input( 10, 10, wid - 20 , hei - 50, "" );
//fi->type( FL_MULTILINE_INPUT );
//fi->value( ret->c_str() );

bok = new Fl_Button( wid - 113, hei - 25, 40 , 20, "OK" );
bok->callback( cb_gc_input_multiline_ok, 0 );

bcancel = new Fl_Button( wid - 66, hei - 25, 60 , 20, "Cancel" );
bcancel->callback( cb_gc_input_multiline_cancel, 0 );

if( b_allow_resize ) resizable( this );									//v1.05

end();
}




void cb_gc_input_multiline_ok( Fl_Widget *o, void * )
{
gc_input_multiline_wnd *wnd = (gc_input_multiline_wnd*) o->parent();

wnd->ok_pressed = 1;
wnd->hide();
}



void cb_gc_input_multiline_cancel( Fl_Widget *o, void * )
{
gc_input_multiline_wnd *wnd = (gc_input_multiline_wnd*) o->parent();

wnd->cancel_pressed = 1;												//v1.06
wnd->hide();
}







void gc_input_multiline_wnd::set_text( string &ss )						//v1.06
{
tb->text( ss.c_str() );
//tb->redraw();
}






//v1.06
//add a user defined button
//use 'cl_gc_input_multiline_button_click' flags to work out what event happened in callback

void gc_input_multiline_wnd::add_button(int xx, int yy, int ww, int hh, string slabel, void (*p_cb)( void*, void* ), void *args_in )
{
st_gc_input_multiline_button_tag o;

o.slabel = slabel;
o.btn_type = en_imbt_fl_button;
//o.mouse_click_which_button = en_imba_left_click;

o.cb_p_callback = p_cb;
o.cb_args = args_in;

//en_gc_input_multiline_button_action_tag mouse_click_which_button;

//te->hide();
o.btn = (void*) new cl_gc_input_multiline_button_click( xx, yy, ww, hh, "" );
((Fl_Button*)o.btn)->copy_label( slabel.c_str() );
((Fl_Button*)o.btn)->callback( (void*)p_cb, (void*)args_in );

vbtn.push_back( o );

add( (Fl_Widget*)o.btn );
//redraw();
//bok->callback( cb_gc_input_multiline_ok, 0 );

}

















cl_gc_input_multiline_button_click::cl_gc_input_multiline_button_click(int xx, int yy, int ww, int hh, const char *label) : Fl_Button( xx, yy, ww, hh, label )
{
mousewheel = 0;
b_invert_mousewheel = 0;
b_mousewheel_changed = 0;

left_button = middle_button = right_button = 0;
}





int cl_gc_input_multiline_button_click::handle( int e )
{
string s1;
bool need_redraw = 0;
bool dont_pass_on = 0;


if ( e == FL_ENTER )
	{

	need_redraw = 0;
	dont_pass_on = 0;
	}


//if ( e == FL_PUSH )
//	{
//	b_mousewheel_changed = 0;											//clear this so callback knows what event caused callback

//	need_redraw = 1;
//	dont_pass_on = 0;
//	}


//if ( e == FL_RELEASE )
//	{
//	b_mousewheel_changed = 0;											//clear this so callback knows what event caused callback

//	need_redraw = 1;
//	dont_pass_on = 0;
//	}


if ( e == FL_PUSH )
	{
	b_mousewheel_changed = 0;											//clear this so callback knows what event caused callback

	if( Fl::event_button() == 1 ) left_button = 1;
	if( Fl::event_button() == 2 ) middle_button = 1;
	if( Fl::event_button() == 3 ) right_button = 1;
 
    do_callback();

//    take_focus();
//    has_focus = 1;

//	if( p_mouse_button_changed_cb ) p_mouse_button_changed_cb( cb_mouse_button_changed_obj, cb_mouse_button_changed_args );
//    do_callback();
    
    dont_pass_on = 0;
    need_redraw = 1;
	}


if ( e == FL_RELEASE )
	{
	b_mousewheel_changed = 0;											//clear this so callback knows what event caused callback

	
	if( Fl::event_button() == 1 ) left_button = 0;
	if( Fl::event_button() == 2 ) middle_button = 0;
	if( Fl::event_button() == 3 ) right_button = 0;

//	do_callback();

//	if( p_mouse_button_changed_cb ) p_mouse_button_changed_cb( cb_mouse_button_changed_obj, cb_mouse_button_changed_args );


    dont_pass_on = 0;
    need_redraw = 1;
	}












if ( e == FL_MOUSEWHEEL )
	{
	b_mousewheel_changed = 1;											//set this so callback knows what event caused callback
	
	mousewheel = Fl::event_dy();
	
	if( b_invert_mousewheel ) mousewheel = -mousewheel;
	
//	printf( "cl_button_wheel::handle() - mousewheel: %d\n", mousewheel );

	do_callback();
	
	need_redraw = 1;
    dont_pass_on = 0;
	}


if ( need_redraw ) redraw();

if( dont_pass_on ) return 1;

return Fl_Button::handle(e);

}



//gc_input_multiline.cpp

//v1.01		30-11-11		//
//v1.02		06-01-2021		//added: 'gc_input_multiline_adj()' function to allow adj text details font/size/color etc
							//also added 'delete wnd;'

//v1.03		05-03-2022		//added to 'gc_input_multiline()' and 'gc_input_multiline_adj()', window pos and size (xx,yy,wid,hei) are now C references to allow wnd details to be maintained between calls,
							//also added 'bselect_text' 
//v1.04		27-05-2022		//added ability to specify font details to 'gc_input_multiline()', see 'font_type, font_size'

//v1.05		01-sep-2026		//added to 'gc_input_multiline_adj' and 'gc_input_multiline' and 'gc_input_multiline_wnd' the option 'b_allow_resize'
#include "gc_input_multiline.h"





void cb_gc_input_multiline_ok( Fl_Widget *, void * );
void cb_gc_input_multiline_cancel( Fl_Widget *, void * );



//see also adjustable 'gc_input_multiline_adj()'
//v1.05
bool gc_input_multiline( int &xx, int &yy, int &wid, int &hei, const char *title, string label, string *ret, bool bselect_text, int font_type, int font_size, bool b_allow_resize )
{
bool ok_pressed = 0;

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

xx = wnd->x();															//v1.03
yy = wnd->y();
wid = wnd->w();
hei = wnd->h();

delete wnd;																//v1.02
return 0;
}




//v1.02
//see also simpler 'gc_input_multiline()'
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

xx = wnd->x();															//v1.03
yy = wnd->y();
wid = wnd->w();
hei = wnd->h();

delete wnd;																//v1.02
return 0;
}





gc_input_multiline_wnd::gc_input_multiline_wnd( int xx, int yy, int wid, int hei, const char *title, string label, string *ret, bool bselect_text, int font_type, int font_size, bool b_allow_resize ) : Fl_Window( xx, yy, wid, hei, title )
{
ok_pressed = 0;


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

wnd->hide();
}


//gc_input_multiline.h
//v1.05

#ifndef gc_input_multiline_h
#define gc_input_multiline_h

#include <stdio.h>
#include <string>



#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/fl_ask.H>

using namespace std;

enum en_gc_input_multiline_button_type_tag								//v1.06
{
en_imbt_fl_button,														//Fl_Button type
};

//enum en_gc_input_multiline_button_action_tag
//{
//en_imba_left_click,														//button had a mouse left click
//};





struct st_gc_input_multiline_button_tag								//v1.06
{
int xx, yy, ww, hh;
string slabel;

en_gc_input_multiline_button_type_tag btn_type;
//en_gc_input_multiline_button_action_tag mouse_click_which_button;

void *btn;																//e.g: could be a Fl_Button or Fl_Check_Button

void (*cb_p_callback)( void*, void* );
void *cb_args;

};






class cl_gc_input_multiline_button_click : public Fl_Button
{
private:


public:
int id0, id1;															//useful if there are a matrix of buttons
string sname;															//can be used to hold string that is not displayed

int mousewheel;
bool b_invert_mousewheel;
bool b_mousewheel_changed;												//use this to tell what caused the callback
bool left_button;
bool middle_button;
bool right_button;


private:
int handle(int);

public:
cl_gc_input_multiline_button_click(int x,int y,int w, int h,const char *label=0);

	
};










class gc_input_multiline_wnd;




bool gc_input_multiline( int &xx, int &yy, int &wid, int &hei, const char *title, string label, string *ret, bool bselect_text, int font_type, int font_size, bool b_allow_resize );
bool gc_input_multiline_adj( int &xx, int &yy, int &wid, int &hei, int fonttype, int fontsize, int bkgd_r, int bkgd_g, int bkgd_b, int txt_r, int txt_g, int txt_b, const char *title, string label, string *ret, bool bselect_text, bool b_allow_resize );
bool gc_input_multiline_adj_non_modal( int &xx, int &yy, int &wid, int &hei, int fonttype, int fontsize, int bkgd_r, int bkgd_g, int bkgd_b, int txt_r, int txt_g, int txt_b, const char *title, string label, string *ret, bool bselect_text, bool b_allow_resize, gc_input_multiline_wnd **ptr_wnd );	//v1.06





class gc_input_multiline_wnd : public Fl_Window
{
private:

Fl_Button *bok;
Fl_Button *bcancel;

public:
Fl_Input *fi;
bool ok_pressed;
bool cancel_pressed;
bool b_is_non_modal;

Fl_Text_Buffer *tb;
Fl_Text_Editor *te;

vector<st_gc_input_multiline_button_tag> vbtn;

public:
gc_input_multiline_wnd( int xx, int yy, int wid, int hei, const char *title, string label, string *ret, bool bselect_text, int font_type = -1, int font_size = -1, bool b_allow_resize = 1 );
void set_text( string &ss );
void add_button(int xx, int yy, int ww, int hh, string slabel, void (*p_cb)( void*, void* ), void *args_in );


};


#endif


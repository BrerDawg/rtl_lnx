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

using namespace std;




bool gc_input_multiline( int &xx, int &yy, int &wid, int &hei, const char *title, string label, string *ret, bool bselect_text, int font_type, int font_size, bool b_allow_resize );
bool gc_input_multiline_adj( int &xx, int &yy, int &wid, int &hei, int fonttype, int fontsize, int bkgd_r, int bkgd_g, int bkgd_b, int txt_r, int txt_g, int txt_b, const char *title, string label, string *ret, bool bselect_text, bool b_allow_resize );



class gc_input_multiline_wnd : public Fl_Window
{
private:

Fl_Button *bok;
Fl_Button *bcancel;

public:
Fl_Input *fi;
bool ok_pressed;

Fl_Text_Buffer *tb;
Fl_Text_Editor *te;

public:
gc_input_multiline_wnd( int xx, int yy, int wid, int hei, const char *title, string label, string *ret, bool bselect_text, int font_type = -1, int font_size = -1, bool b_allow_resize = 1 );


};


#endif


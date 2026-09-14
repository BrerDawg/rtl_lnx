//gcrecent.cpp


//v1.01	 	31-01-2012			//
//v1.02		19-10-2013			//removed any CR/LF char in load_settings() 
//v1.03		31-01-2017			//added: make_first_if_a_match(..)
//v1.04		01-mar-2018			//added sample code and warn not to use:  meMain->copy(menuitems, wndMain);
								//  'meMain->copy(menuitems, wndMain);'	--  this causes RList selection to not correctly pass the 'v' index to callback: 'void cb_recentlist( Fl_Widget *, void *v )'
								//  use this menu assignment for RList to work:   'meMain->menu( menuitems );'


#include "gcrecent.h"


// !!! NOTE DONT USE  meMain->copy(menuitems, wndMain);			as mentioned above


// ---- code snipet example for using this obj, not tested ------


/*
#define cn_recent_file_list_count_max 5;

gcrecent rlist( cn_recent_file_list_count_max ); 			//hold 5 recently opened ini setting fnames
string srlist[ cn_recent_file_list_count_max ];				//holds recent file list variable in File menu
string srlist_menu[ cn_recent_file_list_count_max ];		//holds recent file list variable in File menu with RList prefixed


//---------------------------- Main Menu ----------------------------------
Fl_Menu_Item menuitems[] =
{
	{ "&File",              0, 0, 0, FL_SUBMENU },
		{ "&OpenFile...", FL_CTRL + 'o'	, (Fl_Callback *)cb_open_file, 0 },
		{ "&SaveFile...", FL_CTRL + 's'	, (Fl_Callback *)cb_save_file, 0, FL_MENU_DIVIDER },
		{ "RList:", 0	, (Fl_Callback *)cb_recentlist, (void*)0 },         //!!!change number in reload_rlist_menu() if you change file menu above RList: see below, RList menu starts at 3 for this menu example
		{ "RList:", 0	, (Fl_Callback *)cb_recentlist, (void*)1 },			//you need to include the user value, here shown as 0->5
		{ "RList:", 0	, (Fl_Callback *)cb_recentlist, (void*)2 },
		{ "RList:", 0	, (Fl_Callback *)cb_recentlist, (void*)3 },
		{ "RList:", 0	, (Fl_Callback *)cb_recentlist, (void*)4, FL_MENU_DIVIDER },
		{ "E&xit", FL_CTRL + 'q'	, (Fl_Callback *)cb_btQuit, 0 },
		{ 0 },

	{ "&Edit", 0, 0, 0, FL_SUBMENU },
		{ "&Preferences..",  0, (Fl_Callback *)cb_pref},
		{ "&Preferences2..",  0, (Fl_Callback *)cb_pref2},
		{ "&Font Pref..",  0, (Fl_Callback *)cb_font_pref, 0, FL_MENU_DIVIDER },	
		{ "&Clear",  0, (Fl_Callback *)cb_score_clear },	
		{ 0 },

	{ "&DropWnd", 0, (Fl_Callback *)cb_show_mywnd, 0, },
//	{ "&UnicodeWnd", 0, (Fl_Callback *)cb_show_unicode, 0, },

	{ "&Help", 0, 0, 0, FL_SUBMENU },
		{ "&Help", 0				, (Fl_Callback *)cb_bt_help, 0 },
		{ "&About", 0				, (Fl_Callback *)cb_btAbout, 0 },

		{ 0 },

	{ 0 }
};
//------------------------------------------------------------------------




void cb_recentlist( Fl_Widget *, void *v )
{
string s1;
mystr m1;

int which = (int) v;

s1 = srlist[ which ];



m1 = s1;
m1.FindReplace( s1, "\r", "", 0 );	            	//clear possible Windows CR line ending

bool remove_path = 0;
if( remove_path )	                   				//user does not want full path to be used?
	{
	m1.ExtractFilename( '\\', s1 );
	m1 = s1;
	m1.ExtractFilename( '/', s1 );
	}

printf( "cb_recentlist(): '%s'\n", s1.c_str() );


if( a2_wnd->open_file( s1 ) )						//open file
	{
	rlist.add_entry( s1, 1, 1 );					//add to recent file list, dont repeat, make first RList entry
	reload_rlist_menu();							//refresh RList menu items

	last_score_filename = s1;
	GCProfile p( csIniFilename );
	p.WritePrivateProfileStr( "Settings", "LastScoreFile", last_score_filename.c_str() );
	}
}




//this modified 'RList' menu items
void reload_rlist_menu()
{

//load recent file list menu
for( int i = 0; i < rlist.count(); i++ )
	{
	string s1;
	rlist.get_entry( s1, i );
	srlist[ i ] = s1;										//rem for cb_recentlist menu callback to use
	strpf( srlist_menu[ i ], "RList: %s", s1.c_str() );		//make suitable for menu display
	meMain->replace( i + 3, srlist_menu[ i ].c_str() );		//the number sets where in menu RList will appear
	}

}




//load recent file list
void load_recent_list( string cs_ini_fname )
{
//rlist.clear_all();
rlist.load_settings( cs_ini_fname, "Settings", "RList", "", 1, 0, cn_recent_file_list_count_max );
reload_rlist_menu();
}



void LoadSettings(string csIniFilename)
{
load_recent_list( csIniFilename );
}


//save recent file list
void save_recent_list( string cs_ini_fname )
{
rlist.save_settings( cs_ini_fname, "Settings", "RList" );
}



void SaveSettings(string csIniFilename)
{
save_recent_list( csIniFilename );	//do this first so ini is saved before below GCProfile instance is started
}								//this ensures ini is written out to by rlist.save_settings()








void cb_open_file(Fl_Widget *, void *)
{
mystr m1;
string s1;

char *pPathName = fl_file_chooser( "Load Score?", "*", (const char*)last_score_filename.c_str(), 0 );
if (!pPathName) return;


	
if( a2_wnd->open_file( pPathName ) )
	{
	last_score_filename = pPathName;
	GCProfile p( csIniFilename );
	p.WritePrivateProfileStr( "Settings", "LastScoreFile", last_score_filename.c_str() );

	rlist.add_entry( last_score_filename, 1, 1 );							//add to recent file list
	rlist.make_first_if_a_match( last_score_filename );						//if menu entry existed and was not added, it needs to be moved to the first position
	reload_rlist_menu();													//refresh RList menu
	}
else{
	strpf( s1, "Failed to open file: '%s'", pPathName );
	fl_alert( s1.c_str(), 0 );
	}
}









void cb_save_file(Fl_Widget *, void *)
{
mystr m1;
string s1, sf;


char *pPathName = fl_file_chooser( "Save Score?", "*", (const char*)last_score_filename.c_str(), 0 );
if (!pPathName) return;

sf = pPathName;

bool do_write = 0;

unsigned long long int filesz;
if ( m1.filesize( sf, filesz ) )
	{
	strpf( s1, "File already exists, Overwrite it? : '%s'", sf.c_str() );
	int ret = fl_choice( s1.c_str(),"Cancel","Overwrite", 0 );
	if( ret == 1 )
		{
		do_write = 1;
		}
	}
else{
	do_write = 1;
	}

if( do_write )
	{
	a2_wnd->struct_to_str( vse, s1 );

	m1 = s1;
 
	if( m1.writefile( sf ) )
		{
		last_score_filename = sf;
		a2_wnd->copy_label( sf.c_str() );

		GCProfile p( csIniFilename );
		p.WritePrivateProfileStr( "Settings", "LastScoreFile", last_score_filename.c_str() );
		rlist.add_entry( last_score_filename, 1, 1 );						//add to recent file list
		rlist.make_first_if_a_match( last_score_filename );					//if menu entry existed and was not added, it needs to be moved to the first position 
		reload_rlist_menu();												//refresh RList menu
		}
	else{
		strpf( s1, "Failed to save file: '%s'", sf.c_str() );
		fl_alert( s1.c_str(), 0 );
		}
	}

}

*/



gcrecent::gcrecent( int num )
{
max_entries = 1;

if( num > 0 ) max_entries = num;
}



int gcrecent::count(  )
{
return vstr.size();
}



void gcrecent::clear_all(  )
{
vstr.clear();
}





//add an non blank entry pushing other entries out if already at max_entries
//if 'dont_repeat' is set, dont add entry if already existing
//if 'make_first' is set, move entry to 0 index, ripple others down
//returns idx of entry, else -1
int gcrecent::add_entry( string sadd, bool dont_repeat, bool make_first )
{
int idx = -1;
int match_idx;

if( sadd.length() == 0 ) return 0;


if( dont_repeat )
	{
	idx = find_match( sadd );
	}

if( idx == -1 )									//this entry is new?
	{
	if( make_first )
		{
		if( vstr.size() >= max_entries )
			{
			vstr.erase( vstr.begin() + ( vstr.size() - 1 ) );	//remove last entry if at limit
			}
		vstr.insert( vstr.begin(), sadd );		//insert at head
		idx = 0;
		}
	else{
		if( vstr.size() >= max_entries )
			{
			vstr.erase( vstr.begin() );			//first entry if at limit
			}
		vstr.push_back( sadd );					//inset at end
		idx = vstr.size() - 1;
		}
	}
else{
	if( make_first )
		{
		string stmp = vstr[ idx ];
		vstr.erase( vstr.begin() + idx );		//remove the existing entry
		vstr.insert( vstr.begin(), sadd );		//insert at head
		idx = 0;
		}
	}

return idx;
}









//get an entry at idx
bool gcrecent::get_entry ( string &sget, int idx )
{
sget = "";
if( idx >= vstr.size() ) return 0;

sget = vstr[ idx ];

return 1;
}







//search vector vstr for matching entry
//returns idx if found, else -1
int gcrecent::find_match( string &sfind )
{

for( int  i = 0; i < vstr.size(); i++ )
	{
	if ( vstr[ i ].compare( sfind ) == 0 ) return i;
	}

return -1;
}







//search for matching entry
//if found make it first in list
bool gcrecent::make_first_if_a_match( string &sfirst )
{
int idx = find_match( sfirst );

if( idx == -1 ) return 0;


//string s1 = vstr[ idx ];

vstr.erase( vstr.begin() + idx );

add_entry( sfirst, 1, 1 );
return 1;
}





//save to spec ini file, append and inc'd index to 'key' i.e. key000
bool gcrecent::save_settings( string ini_fname, string section, string key )
{
string s1;

if( vstr.size() == 0 ) return 0;

GCProfile p( ini_fname );

for( int i = 0; i < vstr.size(); i ++ )
	{
	strpf( s1, "%s%03d", key.c_str(), i );			//make key with idx
	p.WritePrivateProfileStr( section, s1, vstr[ i ] );
	
//printf("gcrecent::save_settings(): %s\n", s1.c_str() );	
	}

return 1;
}





//load from spec ini file 'max' number of entries, use inc'd index with 'key' i.e. key000
//if 'dont_repeat' is set, dont add entry if already existing
//if 'make_first' is set, move entry to 0 index, ripple others down
//removes any LF or CR also
//returns number of entries added, else -1
int gcrecent::load_settings( string ini_fname, string section, string key, string def, bool dont_repeat, bool make_first, int max )
{
int i;
mystr m1;

GCProfile p( ini_fname );
//printf( "add_entry '%s'\n", ini_fname.c_str() );		//make key with idx

if( max < 0 ) return 0;

for( i = 0; i < max; i ++ )
	{
	string s1, s2;
	strpf( s1, "%s%03d", key.c_str(), i );		//make key with idx
	p.GetPrivateProfileStr( section, s1, def, &s2 );
	m1 = s2;
	m1.FindReplace( s2, "\r", "", 0 );
	m1 = s2;
	m1.FindReplace( s2, "\n", "", 0 );

	add_entry( s2, dont_repeat, make_first );
//printf( "add_entry '%s'\n", s2.c_str() );		//make key with idx
	}
//getchar();
return i;
}

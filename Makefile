#	A Makefile

#define all executables here
all: rtl_lnx

#define compiler options	
CC=g++
#CFLAGS=-g -O0 -fno-inline

OPTMZ_GLOBAL=-O0									# applies to all files if OPTMZ_SELECT0= is blank
OPTMZ_SELECT_O0=-O0									# !!!!!!!!!!! -O0 for fast compile but slower code execution, this overrides OPTMZ_GLOBAL=...
OPTMZ_SELECT_O3=-O3 -march=native -ffast-math		# !!!!!!!!!!! -O3 for slow compile but faster code execution, this overrides OPTMZ_GLOBAL=...
							
# use  -fsanitize=address -fno-omit-frame-pointer   flags to find 'malloc' and 'delete' mismatch, and also find other crashes such as buffer overflow, where using GDB does not show the crash

CFLAGS= -g -Wfatal-errors -Wfatal-errors -fpermissive -Dbuild_date="\"`date +%Y-%b-%d`\"" #-Dbuild_date="\"2016-Mar-23\""			#64 bit

#LIBS=-L/usr/X11/lib -L/usr/local/lib -L/usr/lib /usr/local/lib/libfltk.a ./librtlsdr.a -lusb-1.0 -lfltk_images -lpng -lz -ljpeg -lrt -lm -lXcursor -lXfixes -lXext -lXft -lfontconfig -lXinerama -lXrender -lpthread -ldl -lX11 -lfftw3 
#LIBS=-L/usr/X11/lib -L/usr/local/lib -L/usr/lib /usr/local/lib/libfltk.a -lrtlsdr -lusb-1.0 -lfltk_images -lpng -lz -ljpeg -lrt -lm -lXcursor -lXfixes -lXext -lXft -lfontconfig -lXinerama -lXrender -lpthread -ldl -lX11 -lfftw3 -lasound -ljack -lasound `pkg-config --libs rtaudio` -ljack
#INCLUDE= -I/usr/local/include -Irtl_include -I/usr/include/libusb-1.0/
LIBS=-L/usr/X11/lib -L/usr/local/lib -L/usr/lib -lfltk -lfltk_images -lrtlsdr -lfltk_images -lpng -lz -ljpeg -lrt -lm -lXcursor -lXfixes -lXext -lXft -lfontconfig -lXinerama -lXrender -lpthread -ldl -lX11 -lfftw3 -lasound `pkg-config --libs rtaudio`
INCLUDE= -I/usr/local/include -Irtl_include -I/home/gc/rtl-sdr-blog/include

#define object files for each executable, see dependancy list at bottom
obj1= rtl_scan.o GCProfile.o pref.o GCCol.o GCLed.o gclog.o gcthrd.o gcpipe.o rtl_graph.o rtlobj.o convenience.o gc_rtaudio.o rt_code.o my_input_wheel.o input_dropbox.o mgraph.o favourite_code.o aa_canvas.o filter_code.o audio_formats.o line_clip_code.o bmp_code.o gc_srateconv.o vert_meter.o rotary_knob_code.o demod_code.o button_wheel_code.o fm_demode_code.o aud_spect_code.o gc_input_multiline.o
#obj2= gc_rtl_power.o getopt.o convenience.o
#obj2= backprop.o layer.o



#linker definition
rtl_lnx: $(obj1)
	$(CC) $(CFLAGS) -o $@ $(obj1) $(LIBS)

#gc_rtl: $(obj2)
#	$(CC) $(CFLAGS) -o $@ $(obj2) $(LIBS)



#compile definition for all cpp files to be complied into .o files
%.o: %.c
	$(CC) $(CFLAGS) $(OPTMZ_GLOBAL) $(OPTMZ) $(INCLUDE) -c $<

%.o: %.cpp
	$(CC) $(CFLAGS) $(OPTMZ_GLOBAL) $(OPTMZ) $(INCLUDE) -c $<

%.o: %.cxx
	$(CC) $(CFLAGS) $(OPTMZ_GLOBAL) $(OPTMZ) $(INCLUDE) -c $<



#dependancy list per each .o file
rtl_scan.o: rtl_scan.cpp rtl_scan.h GCProfile.h pref.h GCCol.h GCLed.h gclog.h gcthrd.h gcpipe.h rtl_graph.h globals.h rtlobj.h convenience.h gc_rtaudio.h rt_code.h my_input_wheel.h input_dropbox.h mgraph.h favourite_code.h aa_canvas.h filter_code.h audio_formats.h line_clip_code.h bmp_code.h gc_srateconv.h vert_meter.h rotary_knob_code.h demod_code.h button_wheel_code.h fm_demode_code.h halfband_poly_optimised_float.h aud_spect_code.h gc_input_multiline.h
rtl_scan.o: OPTMZ=$(OPTMZ_SELECT_O3)

GCProfile.o: GCProfile.h
GCProfile.o: OPTMZ=$(OPTMZ_SELECT_O3)

pref.o: pref.h GCCol.h GCLed.h
pref.o: OPTMZ=$(OPTMZ_SELECT_O3)

GCCol.o: GCCol.h
GCCol.o: OPTMZ=$(OPTMZ_SELECT_O3)

GCLed.o: GCLed.h
GCLed.o: OPTMZ=$(OPTMZ_SELECT_O3)

gclog.o: gclog.h GCProfile.h
gclog.o: OPTMZ=$(OPTMZ_SELECT_O3)

gcthrd.o: gcthrd.cpp gcthrd.h GCProfile.h
gcthrd.o: OPTMZ=$(OPTMZ_SELECT_O3)

gcpipe.o: gcpipe.h GCProfile.h
gcpipe.o: OPTMZ=$(OPTMZ_SELECT_O3)

rtl_graph.o: rtl_graph.cpp rtl_graph.h GCProfile.h pref.h GCCol.h GCLed.h gclog.h gcthrd.h gcpipe.h globals.h bmp_code.h rtlobj.h convenience.h gc_rtaudio.h rt_code.h my_input_wheel.h input_dropbox.h mgraph.h favourite_code.h aa_canvas.h filter_code.h audio_formats.h line_clip_code.h bmp_code.h gc_srateconv.h vert_meter.h rotary_knob_code.h demod_code.h button_wheel_code.h fm_demode_code.h halfband_poly_optimised_float.h aud_spect_code.h gc_input_multiline.h
rtl_graph.o: OPTMZ=$(OPTMZ_SELECT_O3)

rtlobj.o: rtlobj.cpp rtlobj.h GCProfile.h globals.h
rtlobj.o: OPTMZ=$(OPTMZ_SELECT_O3)
#pcaudio.o: pcaudio.h gcthrd.h

gc_rtaudio.o: gc_rtaudio.cpp gc_rtaudio.h globals.h
gc_rtaudio.o: OPTMZ=$(OPTMZ_SELECT_O3)

rt_code.o: rt_code.cpp rt_code.h globals.h GCProfile.h
rt_code.o: OPTMZ=$(OPTMZ_SELECT_O3)

my_input_wheel.o: my_input_wheel.h GCProfile.h
my_input_wheel.o: OPTMZ=$(OPTMZ_SELECT_O3)

input_dropbox.o: input_dropbox.h  GCProfile.h
input_dropbox.o: OPTMZ=$(OPTMZ_SELECT_O3)

mgraph.o: mgraph.cpp mgraph.h GCProfile.h
mgraph.o: OPTMZ=$(OPTMZ_SELECT_O3)

favourite_code.o: favourite_code.h globals.h rtl_scan.h rtl_graph.h GCProfile.h pref.h GCCol.h GCLed.h gc_input_multiline.h
favourite_code.o: OPTMZ=$(OPTMZ_SELECT_O3)

audio_formats.o: audio_formats.h GCProfile.h
audio_formats.o: OPTMZ=$(OPTMZ_SELECT_O3) 

filter_code.o: filter_code.cpp filter_code.h globals.h  GCProfile.h audio_formats.h
filter_code.o: OPTMZ=$(OPTMZ_SELECT_O3)

aa_canvas.o: aa_canvas.h globals.h GCProfile.h filter_code.h line_clip_code.h bmp_code.h
aa_canvas.o: OPTMZ=$(OPTMZ_SELECT_O3)
 
line_clip_code.o: line_clip_code.h GCProfile.h
line_clip_code.o: OPTMZ=$(OPTMZ_SELECT_O3)

bmp_code.o: bmp_code.h GCProfile.h
bmp_code.o: OPTMZ=$(OPTMZ_SELECT_O3)

gc_srateconv.o: gc_srateconv.cpp gc_srateconv.h GCProfile.h filter_code.h
gc_srateconv.o: OPTMZ=$(OPTMZ_SELECT_O3)

vert_meter.o: vert_meter.cpp vert_meter.h GCProfile.h
vert_meter.o: OPTMZ=$(OPTMZ_SELECT_O3)

rotary_knob_code.o: rotary_knob_code.cpp rotary_knob_code.h GCProfile.h
rotary_knob_code.o: OPTMZ=$(OPTMZ_SELECT_O3)

demod_code.o: demod_code.cpp demod_code.h GCProfile.h globals.h rtl_scan.h rtl_graph.h iir_sos_code.h halfband_poly_optimised_float.h aud_spect_code.h 
demod_code.o: OPTMZ=$(OPTMZ_SELECT_O3)

#elliptical_iir_code.o: elliptical_iir_code.cpp elliptical_iir_code.h
#elliptical_iir_code.o: OPTMZ=$(OPTMZ_SELECT_O3)

button_wheel_code.o: button_wheel_code.h GCProfile.h rtl_scan.h rtl_graph.h globals.h
button_wheel_code.o: OPTMZ=$(OPTMZ_SELECT_O3)

fm_demode_code.o: fm_demode_code.cpp fm_demode_code.h globals.h GCProfile.h rtl_scan.h rtl_graph.h iir_sos_code.h halfband_poly_optimised_float.h
fm_demode_code.o: OPTMZ=$(OPTMZ_SELECT_O3)

aud_spect_code.o: aud_spect_code.h globals.h rtl_scan.h rtl_graph.h demod_code.h demod_code.h GCProfile.h globals.h rtl_scan.h rtl_graph.h mgraph.h
aud_spect_code.o: OPTMZ=$(OPTMZ_SELECT_O3)

gc_input_multiline.o: gc_input_multiline.h GCProfile.h 
gc_input_multiline.o: OPTMZ=$(OPTMZ_SELECT_O3)

#gc_rtl_power.o: gc_rtl_power.h getopt.h convenience.h
#getopt.o: getopt.h
#convenience.o: convenience.h
#layer.o: layer.h


.PHONY : clean
clean : 
		-rm rtl_lnx $(obj1)
#		-rm gc_rtl $(obj2)
#		-rm backprop $(obj2)



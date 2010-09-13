/* Dynamically reconfigurable coloring for GUI
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "config.h"
#include <stdio.h>
#include "visualizer.h"

/*****************************************************************************
 *  PALETTE                                                                  *
 *****************************************************************************/
static BOOL palette_initialized = FALSE;
static const char* color_names[PALETTE_SIZE];

void set_color_names(); // defined below

Palette::Palette() { 
  if(!palette_initialized) { set_color_names(); palette_initialized=TRUE; }
  for(int i=0;i<PALETTE_SIZE;i++) stack_depth[i]=0;
  default_palette(); 
  overlay_from_file("local.pal",false); // check for default override
}

void Palette::use_color(ColorName c) {
  glColor4f(colors[c][0],colors[c][1],colors[c][2],colors[c][3]);
}
void Palette::scale_color(ColorName c,flo r, flo g, flo b, flo a) {
  glColor4f(colors[c][0]*r,colors[c][1]*g,colors[c][2]*b,colors[c][3]*a);
}
void Palette::blend_color(ColorName c1,ColorName c2, flo frac1) {
  flo frac2 = 1-frac1;
  glColor4f(colors[c1][0]*frac1+colors[c2][0]*frac2,
            colors[c1][1]*frac1+colors[c2][1]*frac2,
            colors[c1][2]*frac1+colors[c2][2]*frac2,
            colors[c1][3]*frac1+colors[c2][3]*frac2);
}

void Palette::set_background(ColorName c) {
  glClearColor(colors[c][0],colors[c][1],colors[c][2],colors[c][3]);
}

void Palette::set_color(ColorName c, flo r, flo g, flo b) {
  set_color(c,r,g,b,1);
}
void Palette::set_color(ColorName c, flo r, flo g, flo b, flo a) {
  colors[c][0]=r; colors[c][1]=g; colors[c][2]=b; colors[c][3]=a;
}

void Palette::push_color(ColorName c, flo r, flo g, flo b) {
  push_color(c,r,g,b,1);
}
void Palette::push_color(ColorName c, flo r, flo g, flo b, flo a) {
  if(stack_depth[c]>=PALETTE_STACK_SIZE)
    uerror("Color stack depth exceeeded at %s",color_names[c]);
  for(int i=0;i<4;i++) color_stack[c][stack_depth[c]][i] = colors[c][i];
  colors[c][0]=r; colors[c][1]=g; colors[c][2]=b; colors[c][3]=a;
  stack_depth[c]++;
}
void Palette::pop_color(ColorName c) {
  stack_depth[c]--;
  if(stack_depth[c]<0) 
    uerror("Empty color stack popped for %s",color_names[c]);
  for(int i=0;i<4;i++) colors[c][i] = color_stack[c][stack_depth[c]][i];
}

void Palette::substitute_color(ColorName c, ColorName overlay) {
  push_color(c, colors[overlay][0], colors[overlay][1], 
	     colors[overlay][2], colors[overlay][3]);
}

void Palette::overlay_from_file(const char* filename, bool warnfail) {
  FILE* file;
  if((file = fopen(filename, "r"))==NULL) {
    if(warnfail)
      debug("WARNING: Couldn't open palette file %s.\n",filename); 
  } else {
    char buf[255]; int line=0;
    while(fgets(buf,255,file)) {
      line++;
      char name[255]; flo r, g, b, a;
      int n = sscanf(buf,"%255s %f %f %f %f",name,&r,&g,&b,&a);
      if(n==EOF || n==0 || name[0]=='#') continue; // whitespace or comment
      if(n>=4) {
	if(n==4) a=1;
	int c;
	for(c=0;c<PALETTE_SIZE;c++) {
	  if(strcmp(name,color_names[c])==0) {
	    set_color((ColorName)c,r,g,b,a); break;
	  }
	}
	if(c==PALETTE_SIZE) 
	  debug("WARNING: no color named %s (%s line %d)\n",
                name,filename,line);
      } else {
	debug("WARNING: bad color at %s line %d; should be NAME R G B [A]\n",
	      filename,line);
      }
    }
    fclose(file);
  }
}

/*****************************************************************************
 *  COLOR NAMES & SPECIFICATIONS                                             *
 *****************************************************************************/
// necessary to match enums and strings
void set_color_names() {
  for(int i=0;i<PALETTE_SIZE;i++) { color_names[i]=""; } // default
  color_names[BACKGROUND] = "BACKGROUND";
  color_names[TIME_DISPLAY] = "TIME_DISPLAY";
  color_names[FPS_DISPLAY] = "FPS_DISPLAY";
  color_names[LAG_WARNING] = "LAG_WARNING";
  color_names[PHOTO_FLASH] = "PHOTO_FLASH";
  color_names[SIMPLE_BODY] = "SIMPLE_BODY";
  color_names[MOTE_BODY] = "MOTE_BODY";
  color_names[DRAG_SELECTION] = "DRAG_SELECTION";
  color_names[DEVICE_SELECTED] = "DEVICE_SELECTED";
  color_names[DEVICE_ID] = "DEVICE_ID";
  color_names[DEVICE_SCRIPT] = "DEVICE_SCRIPT";
  color_names[DEVICE_VALUE] = "DEVICE_VALUE";
  color_names[DEVICE_PROBES] = "DEVICE_PROBES";
  color_names[VECTOR_BODY] = "VECTOR_BODY";
  color_names[VECTOR_TIP] = "VECTOR_TIP";
  color_names[USER_SENSOR_1] = "USER_SENSOR_1";
  color_names[USER_SENSOR_2] = "USER_SENSOR_2";
  color_names[USER_SENSOR_3] = "USER_SENSOR_3";
  color_names[RGB_LED] = "RGB_LED";
  color_names[RED_LED] = "RED_LED";
  color_names[GREEN_LED] = "GREEN_LED";
  color_names[BLUE_LED] = "BLUE_LED";
  color_names[RADIO_RANGE_RING] = "RADIO_RANGE_RING";
  color_names[NET_CONNECTION_FUZZY] = "NET_CONNECTION_FUZZY";
  color_names[NET_CONNECTION_SHARP] = "NET_CONNECTION_SHARP";
  color_names[NET_CONNECTION_LOGICAL] = "NET_CONNECTION_LOGICAL";
  color_names[RADIO_CELL_INFO] = "RADIO_CELL_INFO";
  color_names[RADIO_BACKOFF] = "RADIO_BACKOFF";
  color_names[DEVICE_DEBUG] = "DEVICE_DEBUG";
  color_names[BUTTON_COLOR] = "BUTTON_COLOR";
  color_names[ODE_SELECTED] = "ODE_SELECTED";
  color_names[ODE_DISABLED] = "ODE_DISABLED";
  color_names[ODE_BOT] = "ODE_BOT";
  color_names[ODE_EDGES] = "ODE_EDGES";
  color_names[ODE_WALL] = "ODE_WALL";
  //color_names[] = "";
}

// set the default colors
void Palette::default_palette() {
  set_color(BACKGROUND,0,0,0,0.5); // black background
  set_color(TIME_DISPLAY,1,0,1);
  set_color(FPS_DISPLAY,1,0,1);
  set_color(LAG_WARNING,1,0,0);
  set_color(PHOTO_FLASH, 1, 1, 1, 1);
  set_color(SIMPLE_BODY, 1.0, 0.25, 0.0, 0.8);
  set_color(MOTE_BODY,1,0,1,0.8);
  set_color(DRAG_SELECTION, 1, 1, 0, 0.5);
  set_color(DEVICE_SELECTED, 0.5, 0.5, 0.5, 0.8);
  set_color(DEVICE_ID, 1, 0, 0, 0.8);
  set_color(DEVICE_SCRIPT, 1, 0, 0, 0.8);
  set_color(DEVICE_VALUE, 0.5, 0.5, 1, 0.8);
  set_color(DEVICE_PROBES, 0, 1, 0, 0.8);
  set_color(VECTOR_BODY, 0, 0, 1, 0.8);
  set_color(VECTOR_TIP, 1, 0, 1, 0.8);
  set_color(USER_SENSOR_1, 1.0, 0.5, 0, 0.8);
  set_color(USER_SENSOR_2, 0.5, 0, 1.0, 0.8);
  set_color(USER_SENSOR_3, 1.0, 0, 0.5, 0.8);
  set_color(RGB_LED, 1, 1, 1, 0.8);
  set_color(RED_LED, 1, 0, 0, 0.8);
  set_color(GREEN_LED, 0, 1, 0, 0.8);
  set_color(BLUE_LED, 0, 0, 1, 0.8);
  set_color(RADIO_RANGE_RING, 0.25, 0.25, 0.25, 0.8);
  set_color(NET_CONNECTION_FUZZY, 0, 1, 0, 0.25);
  set_color(NET_CONNECTION_SHARP, 0, 1, 0, 1);
  set_color(NET_CONNECTION_LOGICAL, 0.5, 0.5, 1, 0.8);
  set_color(RADIO_CELL_INFO, 0, 1, 1, 0.8);
  set_color(RADIO_BACKOFF, 1, 0, 0, 0.8);
  set_color(DEVICE_DEBUG, 1, 0.8, 0.8, 0.5);
  set_color(BUTTON_COLOR, 0, 1.0, 0.5, 0.8);
  set_color(ODE_SELECTED, 0, 0.7, 1, 1);
  set_color(ODE_DISABLED, 0.8, 0, 0, 0.7);
  set_color(ODE_BOT, 1, 0, 0, 0.7);
  set_color(ODE_EDGES, 0, 0, 1, 1);
  set_color(ODE_WALL, 1, 0, 0, 0.1);
  //set_color(, 0, 0, 0, 0);
}

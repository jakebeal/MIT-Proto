/* Dynamically reconfigurable coloring for GUI
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __PALETTE__
#define __PALETTE__

/*
  Palette files are formatted:
  NAME R G B [A]
  Whitespace is OK, comments are lines starting with #
*/

enum ColorName {
  BACKGROUND, TIME_DISPLAY, FPS_DISPLAY, LAG_WARNING, PHOTO_FLASH,
  SIMPLE_BODY, MOTE_BODY, DRAG_SELECTION, DEVICE_SELECTED,
  DEVICE_ID, DEVICE_SCRIPT, DEVICE_VALUE, DEVICE_PROBES, 
  VECTOR_BODY, VECTOR_TIP, 
  USER_SENSOR_1, USER_SENSOR_2, USER_SENSOR_3, 
  RGB_LED, RED_LED, GREEN_LED, BLUE_LED,
  RADIO_RANGE_RING, NET_CONNECTION_FUZZY, NET_CONNECTION_SHARP, 
  NET_CONNECTION_LOGICAL, RADIO_CELL_INFO, RADIO_BACKOFF, DEVICE_DEBUG, 
  BUTTON_COLOR,
  ODE_SELECTED, ODE_DISABLED, ODE_BOT, ODE_EDGES, ODE_WALL,
  PALETTE_SIZE
};

// allows color customization & push/pop of colors
#define PALETTE_STACK_SIZE 4
class Palette {
 private:
  flo colors[PALETTE_SIZE][4]; // active colors
  flo color_stack[PALETTE_SIZE][PALETTE_STACK_SIZE][4]; // saved colors
  int stack_depth[PALETTE_SIZE]; // used to prevent overflow
  void set_color(ColorName c, flo r, flo g, flo b);
  void set_color(ColorName c, flo r, flo g, flo b, flo a);
  
 public:
  Palette();                   // create a default palette
  void overlay_from_file(const char* filename); // replace colors with file values
  void default_palette();      // initialize the default palette [user fn]
  void use_color(ColorName c); // make the current color C
  void scale_color(ColorName c,flo r, flo g, flo b, flo a); // multiply
  void blend_color(ColorName c1,ColorName c2, flo frac1); // mix c1 and c2
  void push_color(ColorName c, flo r, flo g, flo b); // temp change of entry
  void push_color(ColorName c, flo r, flo g, flo b, flo a); // w. non-1 alpha
  void substitute_color(ColorName c, ColorName overlay); // push overlay onto c
  void pop_color(ColorName c); // undo a temporary change
  // some color calls need to be fed values directly
  void set_background(ColorName c);
};

#endif // __PALETTE__

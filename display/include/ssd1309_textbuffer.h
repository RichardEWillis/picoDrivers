// Text buffer overlay for the ssd1309_driver
// Create a 20 x 8 Text screen onto a
// private framebuffer (128 x 64 pix)
// left/right edges contain blank columns
// for centering the text on the page.

#ifndef __SSD1309_TEXTBUFFER_H__
#define __SSD1309_TEXTBUFFER_H__

#include <stdio.h>
#include <stdint.h>
#include "pico/types.h"

// 'mode'
// Setup how and when the framebuffer is written to the 
// display
//   ON_TEXT_CHANGE : After every update on the page
//                    eg. after stb_puts() is called. 
//   ON_DEMAND      : Only when stb_refresh() is called.
#define REFRESH_ON_TEXT_CHANGE  0
#define REFRESH_ON_DEMAND       1
// (set in stb_init() -> 'mode')

// 'wrap'
// Setup the display wether to wordwrap or not.
// If disabled, then text going off the screen to the right 
// are lost.
#define SET_TEXTWRAP_OFF        0
#define SET_TEXTWRAP_ON         1
// (set in stb_init() -> 'wrap')

// Initialize the text buffer.
// If the text buffer is the only API using the display
// then set 'initDisp' to 1, else set to 0 and init/
// start the display before calling stb_init().
// Use 'mode' to determine when the display is refreshed
// from the text buffer.
// Returns 0 on success, 1 on some error.
int stb_init(int mode, int wrap, int initDisp, uint gpio_dc, uint gpio_res);

// Clear the text screen
// Returns 0 on success, 1 on some error.
int stb_clear(void);

// Set the cursor to (x,y).
//  x := leftmost side is zero. Range {0 .. 19}
//  y := topmost side is zero (page:0). Range {0 .. 7}
// Returns 0 on success, 1 on some error.
int stb_cursor(uint x, uint y);

// Put characters into the text buffer. '\n' is inerpreted
// as a newline and will reset x back to zero and increment y.
// return # characters (including \n) parsed.
// Overruns (if no wordwrap) will be lost and not counted in 
// the parsed count.
int stb_putc(char c);
int stb_puts(const char * s);
int stb_newline(void); // shortcut for stb_putc('\n');

// Manually refresh the display with the current text buffer 
// contents. This MUST BE called if the text buffer is not
// auto refreshing the display on text buffer updates.
// Returns 0 on success, 1 on some error.
int stb_refresh(void);

#endif /* __SSD1309_TEXTBUFFER_H__ */


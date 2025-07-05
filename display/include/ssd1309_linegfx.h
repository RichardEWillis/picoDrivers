/* Simple Line Graphics API 
 * Draw lines, arcs and circles.
 * Enclosed areas can be filled.
 * For the SSD1309 128x64 OLED Display
 * Note: this extends ssd1309_textbuffer when
 *       USE_LINE_GFX is defined at compile time.
 */

#ifndef __LINE_GRAPHICS_H__
#define __LINE_GRAPHICS_H__

#include <stdio.h>
#include <stdint.h>
#include "pico/types.h"

/* Interoperation with the text renderer: 
 *  - use stb_clear()  to clear the screen
 *    of both text and graphics.
 *  - This will be rendered FIRST, before any 
 *    text or floating text box is rendered.
 *  - If not using the graphics lib, then
 *    ASCII ART is possible using the charset
 *    but it consumes text space.
 *  - use stb_refresh() to redraw the screen.
 *  - The line gfx API uses its own framebuffer
 *    for the initial rendering, then this is 
 *    overlayed onto the main frame buffer when
 *    stb_refresh() is called.
*/

// in inverted mode, colours have the opposite effect,
// note: clearing the screen sets the colour to all white.
#define COLOUR_BLK  1   /* normal: will set piuxel as dark */
#define COLOUR_WHT  2   /* normal: will set piuxel as light */

// Arc Angles are defined in Deg. x 10 so the range is 
//  {0 .. 3599} (0.0 - 359.9)

// Clear the graphics part of the frame buffer.
// Does not affect any other part of the buffer (text, low lvl fb)
//  Returns,
//      0 := OK, 1:= Error
int lgfx_clear(void);

// Draw a line from (x1,y1) to (x2,y2)
//  Inputs: (x1,y1) starting coords
//          (x2,y2) ending coords
//          c       colour (see COLOUR_***)
//  Limits:
//          x   {0 .. 127}
//          y   {0 .. 63}
//          c   COLOUR_BLK | COLOUR_WHT
//  Returns,
//      0 := OK, 1:= Error
int lgfx_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t c);

// Draw a box from (x1,y1) to (x2,y2)
//  Inputs: (x1,y1) starting coords
//          (x2,y2) ending coords
//          c       colour (see COLOUR_***)
//  Limits:
//          x   {0 .. 127}
//          y   {0 .. 63}
//          c   COLOUR_BLK | COLOUR_WHT
//  Returns,
//      0 := OK, 1:= Error
int lgfx_box(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t c);

// Draw an Arc
//  Inputs: (cx,cy)     centre of the arc
//          r           radius (pixels)
//          a1          starting arc angle 
//          a2          ending arc angle
//          c       colour (see COLOUR_***)
//  Limits:
//          cx   {0 .. 127}
//          cy   {0 .. 63}
//          r    {0 .. 63}
//          a1,a2 {0 ..3599}
//          c   COLOUR_BLK | COLOUR_WHT
//  Returns,
//      0 := OK, 1:= Error
int lgfx_arc(uint8_t cx, uint8_t cy, uint8_t r, uint16_t a1, uint16_t a2, uint8_t c);

// Draw a full circle
//  Inputs: (cx,cy)     centre of the arc
//          r           radius (pixels)
//          c       colour (see COLOUR_***)
//  Limits:
//          cx   {0 .. 127}
//          cy   {0 .. 63}
//          r    {0 .. 63}
//          c   COLOUR_BLK | COLOUR_WHT
//  Returns,
//      0 := OK, 1:= Error
int lgfx_circle(uint8_t cx, uint8_t cy, uint8_t r, uint8_t c);

// Implement a Horizontal Bar Graph
//  Inputs: (x,y)   leftmost starting point (top left of graph)
//          h       graph height (pixels)
//          len     total graph length (keep constant)
//          fill    amount of the graph to fill N-1
//          c       colour (see COLOUR_***)
//  Limits:
//          x    {0 .. 127}
//          len  {0 .. 127}
//          fill {0 .. 127}
//          y    {0 .. 63}
//          h    {0 .. 63}
//          c   COLOUR_BLK | COLOUR_WHT
//  Returns,
//      0 := OK, 1:= Error
//  Notes:
//  [1] the fill length is just an x value of the range {x .. x+len}
//      representing 0% to 100% fill percentage. graph fills from
//      left to right.
//  Example: (x,y) = (0,60), len = 100, fill = 50, c = BLACK
//      creates a graph, 4 pixels high at the bottom left of the 
//      screen and setup to directly interpret 'fill' as a percentage
//      from 0 .. 100. Fill is currently set to 50%.
// -----------------------------------------------------------------
int lgfx_bgraph(uint8_t x, uint8_t y, uint8_t h, uint8_t len, uint8_t fill, uint8_t c);

#endif /* __LINE_GRAPHICS_H__ */

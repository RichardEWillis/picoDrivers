/******************************************************************************
 * linegfx
 * 
 * Simple graphics primatives support. Refactored from ssd1309_linegfx.c.
 * 
 * Ver. 2.0
 * 
 * This version uses the gfxDriver Stack and so is more flexible to different
 * graphics display sizes. 
 * 
 * Features:
 *  - no longer tied to one display (SSD1309, 128x64 OLED)
 *  - uses the gfx driver stack for access to the mounted graphics driver
 *  - has its own buffer which renders into the driver's framebuffer.
 * 
 * Limitations:
 *  - Still expects page type of graphics driver where 8 rows are encoded
 *    into one page byte, LSB in the top row.
 *  - only supports line drawing at this point.
 * 
 * InterOperation with Text Graphics
 *  - Ver 2.0
 *    - Text graphics is permanently set to Layer 1, Line Graphics can
 *      only be set to Layer FOREGROUND as this time so it will always
 *      be rendered on top of any text.
 *    - Line graphics API does not have a refresh() call. Instead, use
 *      the textgfx_refresh() function from textgfx.h
 */

#include <string.h>
#include <stdlib.h>
#include <linegfx.h>
#include <cpyutils.h>
#include <gfxDriverLowPriv.h>

// must now be initialized as the underlying gfx driver fb size is unknown 
// until discovered in the new mounted driver.
static uint8_t *   gfx_linebuffer = NULL;
static size_t      gfx_bufferlen = 0;
static int         gfx_fastcopy_enabled = 0;  // set to 'true' to allow fast copies in the framebuffer
static size_t      drvr_screen_width = 0;
static size_t      drvr_screen_height = 0;
static size_t      drvr_screen_pages = 0;

// call after driver is mounted and started to get info
// Returns:
//  0 := ok
//  1 := error (not mounted?)
int lgfx_init(uint8_t layer_prio) {
    int rc = 1;
    if ( bsp_gfxDriverIsReady() ) {
        if (gfx_linebuffer == NULL) {
            drvr_screen_width  = gfx_getDispWidth();
            drvr_screen_height = gfx_getDispHeight();
            drvr_screen_pages  = gfx_getDispPageHeight();
            gfx_bufferlen = (drvr_screen_pages * drvr_screen_width);
            gfx_fastcopy_enabled = ( (gfx_bufferlen % 4) == 0 );
            gfx_linebuffer = (uint8_t *)malloc(gfx_bufferlen);
            if (gfx_linebuffer) {
                gfxutil_fb_clear(gfx_linebuffer, gfx_bufferlen, gfx_fastcopy_enabled);
                rc = gfx_setFrameBufferLayerPrio(gfx_linebuffer,layer_prio, FB_NO_MASK);
            }
        } else {
            rc = 0; // ignore a repeat call, already initialized.
        }
    }
    return rc;
}

static void lgfx_clearbuf(void) {
    if (gfx_linebuffer) {
        uint32_t i = 0;
        if (gfx_fastcopy_enabled) {
            uint32_t * fb32 = (uint32_t *)gfx_linebuffer;
            uint32_t   fb32len = gfx_bufferlen / sizeof(uint32_t);
            for ( i = 0 ; i < fb32len ; i++ )
                fb32[i] = 0;
        } else {
            for ( i = 0 ; i < gfx_bufferlen ; i++ )
                gfx_linebuffer[i] = 0;
        }
    }
}

// // called by the text renderer API to copy in any graphics 
// // FIRST into the main text framebuffer
// void lgfx_copy_buffer(uint8_t * tfb) {
//     uint32_t   i = 0;
// #if (USE_FAST_FB_COPY==1)
//     uint32_t * fb32 = (uint32_t *)gfx_linebuffer;
//     uint32_t * tfb32 = (uint32_t *)tfb;
//     uint32_t   fb32len = FRAMEBUFFER_SIZE / sizeof(uint32_t);
//     for ( i = 0 ; i < fb32len ; i++ )
//         tfb32[i] = fb32[i];
// #else
//     for ( i = 0 ; i < FRAMEBUFFER_SIZE ; i++ )
//         tfb[i] = gfx_linebuffer[i];
// #endif
// }



static int lgfx_plot(uint8_t x, uint8_t y, uint8_t c) {
    // plot pixel into the local frambuffer.
    uint8_t page = y >> 3;
    uint8_t n = y - (page << 3);
    uint8_t m = (1<< n);
    uint32_t idx = (uint32_t)page * drvr_screen_width + x;
    if (c != COLOUR_WHT) {
        gfx_linebuffer[idx] |= m;
    } else {
        gfx_linebuffer[idx] &= (uint8_t)~m;
    }
    return 0;
}

// add a small horizontal line segment
static int lgfx_linex(uint8_t x, uint8_t y, uint8_t len, uint8_t c) {
    int rc = 1;
    while (len) {
        rc = lgfx_plot(x, y, c);
        x++;
        len --;
        if (rc || x >= drvr_screen_width)
            break;
    }
    return rc;
}

// add a small vertical line segment
static int lgfx_liney(uint8_t x, uint8_t y, uint8_t len, uint8_t c) {
    int rc = 1;
    while (len) {
        rc = lgfx_plot(x, y, c);
        y++;
        len --;
        if (rc || y >= drvr_screen_height)
            break;
    }
    return rc;
}

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
// Arc Angles are defined in Deg. x 10 so the range is 
//  {0 .. 3599} (0.0 - 359.9)

// Clear the graphics part of the frame buffer.
// Does not affect any other part of the buffer (text, low lvl fb)
//  Returns,
//      0 := OK, 1:= Error
int lgfx_clear(void) {
    lgfx_clearbuf();
    return 0;
}

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
int lgfx_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t c) {
    int rc = 1;
    if ((x1 < drvr_screen_width) && (x2 < drvr_screen_width) && 
        (y1 < drvr_screen_height) && (y2 < drvr_screen_height) && 
        (c >= COLOUR_BLK) && (c <= COLOUR_WHT)) {
        int dx = (int)x2 - (int)x1;
        int dy = (int)y2 - (int)y1;
        int ax = (dx >= 0) ? dx : (-1)*dx; // abs x dist
        int ay = (dy >= 0) ? dy : (-1)*dy; // abs y dist

        // dx        := line projection on x-axis (signed)
        // ax        := |dx| (absolute len)
        // dy, ay    := same a dx,ax except for y-axis

        if (ay == 0) {
            // horizontal line, use line-segment to draw the entire thing, starting from the lower x point
            rc = lgfx_linex((dx >=0)?x1:x2,y1,ax+1,c); // add +1 to len to cover the end pixel.
        } else if (ax == 0) {
            // vertical line, use line-segment to draw the entire thing
            rc = lgfx_liney(x1,(dy >=0)?y1:y2,ay+1,c);
        } else {
            int D, sx, sy, err, e2;
            // sloped line, use Bresenham Algo (int math)
            ay = (-1)*ay; // this algo wants this slope sign flipped.
            sx = (x1 < x2) ? 1 : -1;
            sy = (y1 < y2) ? 1 : -1;
            err = ax + ay;
            rc = 0;
            while (rc == 0) {
                lgfx_plot(x1, y1, c);
                e2 = err * 2;
                if (e2 >= ay) {
                    if (x1 == x2) {
                        break;
                    }
                    err += ay;
                    x1 += sx;
                }
                if (e2 <= ax) {
                    if (y1 == y2) {
                        break;
                    }
                    err += ax;
                    y1 += sy;
                }
            }
        }
    }
    return rc;
}

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
int lgfx_box(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t c) {
    int rc = lgfx_line(x1,y1,x2,y1,c);
    if (!rc) {
        rc = lgfx_line(x2,y1,x2,y2,c);
        if (!rc) {
            rc = lgfx_line(x2,y2,x1,y2,c);
            if (!rc) {
                rc = lgfx_line(x1,y2,x1,y1,c);
            }
        }
    }
    return rc;
}

int lgfx_filled_box(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t c) {
    int rc = lgfx_box(x1, y1, x2, y2, c);
    if (!rc) {
        size_t i;
        uint8_t ya = y1;
        uint8_t yb = y2;
        if (yb < ya) {
            ya = y2;
            yb = y1;
        }
        for (i = ya+1 ; i < yb ; i++) {
            rc = lgfx_line(x1,i,x2,i,c);
            if (rc) {
                break;
            }
        }
    }
    return rc;
}

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
int lgfx_arc(uint8_t cx, uint8_t cy, uint8_t r, uint16_t a1, uint16_t a2, uint8_t c) {
    return 1;
}

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
int lgfx_circle(uint8_t cx, uint8_t cy, uint8_t r, uint8_t c) {
    return 1;
}

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
int lgfx_bgraph(uint8_t x, uint8_t y, uint8_t h, uint8_t len, uint8_t fill, uint8_t c) {
    int rc = 1;
    volatile int i,j,n;
    uint8_t * glb = &(gfx_linebuffer[x]);
    uint8_t * pi[8]; // the 8 page pointers
    uint8_t   mpage[8];  // bitmask for each page.. where the active area of the graph is. if zero then page not int he graph.
    
    // optimize this graph to make use of the underlying screen memory mapping
    // where pages are stacks of 8-bit columns in a horizontal row 128 elements 
    // wide.
    if ((x+len < drvr_screen_width) && (y+h < drvr_screen_height) && 
        (c >= COLOUR_BLK) && (c <= COLOUR_WHT) && (h > 0) && (len > 0)) {
        for (i=0 ; i<8 ; i++ ) {
            pi[i] = glb;
            glb += drvr_screen_width;
            // work out the bit masks
            mpage[i] = 0;
            n = i << 3; //abs posn of topmost pixel in this page (range:0..63)
            for (j = 0 ; j < 8 ; j++ ) {
                if ((n+j) >= y && (n+j) <= (y+h-1)) {
                    mpage[i] |= 1<<j; // this pixel is in the bar, set its mask
                }
            }
        }
        i = 0;
        while (len) {
            // work through total length of bar graph
            for (j=0 ; j < 8 ; j++ ) {
                if (mpage[j]) {
                    if (i < fill) {
                        // add bar
                        *(pi[j]) |= mpage[j];
                    } else {
                        // remove bar
                        *(pi[j]) &= (uint8_t)~(mpage[j]);
                    }
                }
            }
            len --;
            i ++;
            for (j=0 ; j < 8 ; j++ ) {
                pi[j] ++; // adv. to next pixel column over in each page
            }
        }
        rc = 0;
    }
    return rc;
}

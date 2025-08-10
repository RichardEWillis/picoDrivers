/******************************************************************************
 * textgfx
 * 
 * Simple text graphics support. Refactored from ssd1309_textbuffer.c and
 * ssd1309_ftb.c
 * 
 * Ver. 2.0
 * 
 * This version uses the gfxDriver Stack and so is more flexible to different
 * graphics display sizes. 
 * 
 * Features:
 *  - no longer tied to one display (SSD1309, 128x64 OLED)
 *  - uses the gfx driver stack for access to the mounted graphics driver
 *  - has its own text buffer which renders into the graphics framebuffer.
 *  - now integrates floating text box API (ftbgfx_)
 * 
 * Limitations:
 *  - Still expects page type of graphics driver where 8 rows are encoded
 *    into one page byte, LSB in the top row.
 */

#ifndef __TEXTGFX_H__
#define __TEXTGFX_H__

#include <stdio.h>
#include <stdint.h>
#include "pico/types.h"

// Initialize the text layer. This is required BEFORE 
// initializing either fixed or floating text buffers and 
// prior to their own init functions.
// Inputs:
//  [uint8_t] layer_prio
//                    Set text layer priority. This also affects
//                    floating text buffers. Typical layer 
//                    setting is SET_FB_LAYER_1 but this can
//                    be changed to eg. put text in front of
//                    other layers like linegfx.
//                    range: {0 .. SET_FB_LAYER_FOREGROUND}
//                    where 0 = SET_FB_LAYER_BACKGROUND.
// Note: This call MUST BE CALLED prior to textgfx_init() or ftbgfx_init().
// Returns:
//  0 on success
//  1 on some error.
// ----------------------------------------------------------------------------
int text_init(uint8_t layer_prio);


// ----------------------------------------------------------------------------
// --- Static Text Box API
// ----------------------------------------------------------------------------

// 'mode'
// Setup how and when the framebuffer is written to the 
// display
//   ON_TEXT_CHANGE : After every update on the page
//                    eg. after textgfx_puts() is called. 
//   ON_DEMAND      : Only when textgfx_refresh() is called.
#define REFRESH_ON_TEXT_CHANGE  0
#define REFRESH_ON_DEMAND       1
// (set in textgfx_init() -> 'mode')

// 'wrap'
// Setup the display wether to wordwrap or not.
// If disabled, then text going off the screen to the right 
// are lost.
#define SET_TEXTWRAP_OFF        0
#define SET_TEXTWRAP_ON         1
// (set in textgfx_init() -> 'wrap')

// -----------------------------------------------------------
// (!) Normally, a return of 0 is success and +1 is an error.
//     Exceptions to this are documented in affected function
//     headers below.
// -----------------------------------------------------------

// Initialize the text buffer.
// Mount the display driver before calling this function.
// Inputs:
//  [int] mode        determine when the display is refreshed
//                    from the text buffer.
//                    (REFRESH_ON_*)
//                      0 := update screen after updating text 
//                           buffer.
//                      1 := only update screen on refresh()
//  [int] wrap        auto-wrap text to the next line.
//                    (SET_TEXTWRAP_*)
//                      0 := no wrap
//                      1 := wrap
// Note: This call is NOT required to invoke if only using
//       floating text buffers, API further below in this 
//       header.
// Returns:
//  0 on success
//  1 on some error.
// ----------------------------------------------------------------------------
int textgfx_init(int mode, int wrap);

// Get the size parameters of the fixed text box.
// If returning -1 then text box not initialized.
int textgfx_get_width(void);       /* number of characters wide */
int textgfx_get_height(void);      /* number of characters high */

// Set the cursor to (x,y).
//  x := leftmost side is zero. Range {0 .. 19}
//  y := topmost side is zero (page:0). Range {0 .. 7}
// Returns 0 on success, 1 on some error.
int textgfx_cursor(uint x, uint y);

// Get current cursor position. If -1 then text box not initialized.
int textgfx_get_cursor_posn_x(void);
int textgfx_get_cursor_posn_y(void);

// change the refresh and text wrap modes 
// after the textgfx layer has already been started.
int textgfx_get_refresh_mode(void);     /* returns: REFRESH_ON_*      */
int textgfx_get_text_wrap_mode(void);   /* returns: SET_TEXTWRAP_*    */
int textgfx_set_refresh_mode(int mode);     /* returns: 0 := ok, 1:= not init'd */
int textgfx_set_text_wrap_mode(int wrap);   /* returns: 0 := ok, 1:= not init'd */

// Clear the text screen
// Returns 0 on success, 1 on some error.
int textgfx_clear(void);

// Put characters into the text buffer. '\n' is inerpreted
// as a newline and will reset x back to zero and increment y.
// Overruns (if no wordwrap) will be lost and not counted in 
// the parsed count.
// Returns:
//  -1    := error (not initialized?)
//   0    := should not occur, treat as an error
//   1+   := # characters (including \n) parsed.
// ---
int textgfx_putc(char c);
int textgfx_puts(const char * s);
int textgfx_newline(void); // shortcut for textgfx_putc('\n');

// Manually refresh the display with the current text buffer 
// contents. This MUST BE called if the text buffer is not
// auto refreshing the display on text buffer updates.
// Returns 0 on success, 1 on some error.
int textgfx_refresh(void);

// ----------------------------------------------------------------------------
// --- Floating Text Box API
// --- Ver 2.0 : can use FTB, static text layer or both. FTB is no longer
// ---           dependent on the static text system.
// ----------------------------------------------------------------------------

/* Number of concurrent active floating test boxes supported in the API */
#ifndef FTB_COUNT
  #define FTB_COUNT 4
#endif

/* future - Scaling factor */
#define FTB_SCALE_1     1       /* original scale, 1:1 */
#define FTB_SCALE_1_5   2       /* expanded 1.5x       */
#define FTB_SCALE_2     3       /* expanded 2x         */

#define FTB_BKGRND_OPAQUE 0
#define FTB_BKGRND_TRANSP 1
#define FTB_TEXT_NOWRAP   0
#define FTB_TEXT_WRAP     1

// Setup for floating text boxes prior to their use.
int ftbgfx_init(void);

// Return the drawspace extents of the mounted display driver
// Does not require an open floating text box instance.
int ftbgfx_get_max_pix_width(void);
int ftbgfx_get_max_pix_height(void);
int ftbgfx_get_max_textbox_width(void);
int ftbgfx_get_max_textbox_height(void);

// Create a new Floating Text Box, up to FTB_COUNT in total can be created.
//  xpos,ypos   starting top-left box coordinates (can be changed later)
//  width,hght  width and height of the text box, in characters.
//  bkgnd_trans [0,1] if true then the text background is transparent and data
//              already in the framebuffer will not be deleted in the text 
//              boxes render area.
//              If false, then the FB area will be cleared first.
// wwrap        [0,1]   0 := do not CR/LF at end of line. Stay on the same line.
//                      1 := perform CR/LF at end of line, if not at the bottom line already.
//                           cursor is set back to the left side on the next line down.
// scale        (not yet supported) Font scaling.
//
// Returns:
//  [void *] (handle) handle to the new text box. Required byh all other calls.
// -----------------------------------------------------------------------------------------------
void * ftbgfx_new(uint8_t xpos, uint8_t ypos, uint8_t width, uint8_t hght, 
    uint8_t bkgnd_trans, uint8_t wwrap, uint8_t scale);

// Set box as drawable (not hidden)
//  Returns,
//      0 := OK, 1:= Error
int ftbgfx_enable(void * ftbhnd);

// Set box as hidden, not drawn but still active and usable.
//  Returns,
//      0 := OK, 1:= Error
int ftbgfx_disable(void * ftbhnd);

// disable and "delete" the text box. Cannot be used again until re-started using ftbgfx_new()
//  Returns,
//      0 := OK, 1:= Error
int ftbgfx_delete(void * ftbhnd);

// Clear the text box, resets cursor to 0,0 (top-left)
//  Returns,
//      0 := OK, 1:= Error
int ftbgfx_clear(void * ftbhnd);

// Move the text box in the frame buffer space
//  xpos,ypos -- location in the framebuffer (pixel offset)
//  Returns,
//      0 := OK, 1:= Error
int ftbgfx_move(void * ftbhnd, uint8_t xpos, uint8_t ypos);

// Move the cursor in the text box, within its boundaries!
//  xpos,ypos -- location in the framebuffer (text character location), relative to top-left
//  Returns,
//      0 := OK, 1:= Error
int ftbgfx_cursor(void * ftbhnd, uint8_t xpos, uint8_t ypos);

// shortcut to ftb_cursor(id,0,0);
//  Returns,
//      0 := OK, 1:= Error
int ftbgfx_home(void * ftbhnd);

// put character at cursor and advance the cursor. End of line (text-box)
// behavoir depends upon the word-wrap (wwrap) mode configured for the new FTB.
//  ftbID   handle to the open text box.
//  c       ASCII char. '\n' will cr/nl the cursor (next line down @ col 0) if possible.
//          0x80 'DEL' will back cursor one character, towards left side.
// Returns,
//  [int]  0 nothing done (at right limit of text box with no wrap option?)
//         1 character added. using '\n' or 0x80 will return zero!
//        -1 error.
int ftbgfx_putc(void * ftbhnd, char c);

// put string at cursor, advancing the cursor as the string is added. End of line (text-box)
// behavoir depends upon the word-wrap (wwrap) mode configured for the new FTB.
//  ftbID   handle to the open text box.
//  s       NULL terminated ASCII string (UTF-8). Can include '\n' for rc/nl and 0x80 'DEL' for
//          backspace.
// Returns,
//  [int] 0+ len of string put into the text box.
//        -1 error.
int ftbgfx_puts(void * ftbhnd, const char * s);

// Shortcut to ftb_putc('\n);
//  Returns,
//      0 := OK, 1:= Error
int ftbgfx_newline(void * ftbhnd);

// Shortcut to ftb_putc([DEL]);
//  Returns,
//      0 := OK, 1:= Error
int ftbgfx_bkspace(void * ftbhnd);

// Manually refresh the display with the current floating text buffer contents.
int ftbgfx_refresh(void * ftbhnd);

// Refresh all open floating text boxes. Does not require a handle.
int ftbgfx_refresh_all(void);

#endif /* __TEXTGFX_H__ */


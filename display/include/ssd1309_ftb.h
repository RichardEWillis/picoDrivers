/* Floating Text Buffer API 
 * Impliments n text boxes that can be placed at any location in the 
 * physical frame-buffer. Offset is by the top-left boxes pixel 
 * coordinates (x,y).
 */

#ifndef __FLOATING_TXT_BUF_H__
#define __FLOATING_TXT_BUF_H__

#include <stdio.h>
#include <stdint.h>
#include "pico/types.h"

/* Number of concurrent active floating test boxes supported in the API */
#define FTB_COUNT 4

/* future - Scaling factor */
#define FTB_SCALE_1     1       /* original scale, 1:1 */
#define FTB_SCALE_1_5   2       /* expanded 1.5x       */
#define FTB_SCALE_2     3       /* expanded 2x         */

// Text buffer layer of the driver must call this to initialize all of the floating text boxes
// prior to their use.
void ftb_init(void);

#define FTB_BKGRND_OPAQUE 0
#define FTB_BKGRND_TRANSP 1
#define FTB_TEXT_NOWRAP   0
#define FTB_TEXT_WRAP     1

// Enable a FTB instance, {0 .. FTB_COUNT-1}
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
//  [int] (ftbID) ID for this box. Use in subsequent calls. -1 indicates an error.
// -----------------------------------------------------------------------------------------------
int ftb_new(uint8_t xpos, uint8_t ypos, uint8_t width, uint8_t hght, 
    uint8_t bkgnd_trans, uint8_t wwrap, uint8_t scale);

// Set box as drawable (not hidden)
//  Returns,
//      0 := OK, 1:= Error
int ftb_enable(int ftbID);

// Set box as hidden, not drawn but still active and usable.
//  Returns,
//      0 := OK, 1:= Error
int ftb_disable(int ftbID);

// disable and "delete" the text box. Cannot be used again until re-started using ftb_new()
//  Returns,
//      0 := OK, 1:= Error
int ftb_delete(int ftbID);

// Clear the text box, resets cursor to 0,0 (top-left)
//  Returns,
//      0 := OK, 1:= Error
int ftb_clear(int ftbID);

// Move the text box in the frame buffer space
//  xpos,ypos -- location in the framebuffer (pixel offset)
//  Returns,
//      0 := OK, 1:= Error
int ftb_move(int ftbID, uint8_t xpos, uint8_t ypos);

// Move the cursor in the text box, within its boundaries!
//  xpos,ypos -- location in the framebuffer (text character location), relative to top-left
//  Returns,
//      0 := OK, 1:= Error
int ftb_cursor(int ftbID, uint8_t xpos, uint8_t ypos);

// shortcut to ftb_cursor(id,0,0);
//  Returns,
//      0 := OK, 1:= Error
int ftb_home(int ftbID);

// put character at cursor and advance the cursor. End of line (text-box)
// behavoir depends upon the word-wrap (wwrap) mode configured for the new FTB.
//  ftbID   handle to the open text box.
//  c       ASCII char. '\n' will cr/nl the cursor (next line down @ col 0) if possible.
//          0x80 'DEL' will back cursor one character, towards left side.
// Returns,
//  [int]  0 nothing done (at right limit of text box with no wrap option?)
//         1 character added. using '\n' or 0x80 will return zero!
//        -1 error.
int ftb_putc(int ftbID, char c);

// put string at cursor, advancing the cursor as the string is added. End of line (text-box)
// behavoir depends upon the word-wrap (wwrap) mode configured for the new FTB.
//  ftbID   handle to the open text box.
//  s       NULL terminated ASCII string (UTF-8). Can include '\n' for rc/nl and 0x80 'DEL' for
//          backspace.
// Returns,
//  [int] 0+ len of string put into the text box.
//        -1 error.
int ftb_puts(int ftbID, const char * s);

// Shortcut to ftb_putc('\n);
//  Returns,
//      0 := OK, 1:= Error
int ftb_newline(int ftbID);

// Shortcut to ftb_putc([DEL]);
//  Returns,
//      0 := OK, 1:= Error
int ftb_bkspace(int ftbID);

#endif /* __FLOATING_TXT_BUF_H__ */

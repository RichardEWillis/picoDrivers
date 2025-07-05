
#include "ssd1309_ftb.h"
#include "ssd1309_fb_private.h"
#include "ssd1309_driver.h"

// replace with TB defines
#define TB_WIDTH    TXTBUFFER_WIDTH
#define TB_HEIGHT   TXTBUFFER_HEIGHT
#define TB_MAXLEN   (TB_WIDTH * TB_HEIGHT)
#define FB_WIDTH    SSD1309_DISP_COLS
#define FB_HEIGHT   (SSD1309_DISP_PAGES * SSD1309_DISP_PIX_PER_OCTET)
#define FB_BUF_LEN  (FB_WIDTH * FB_HEIGHT / SSD1309_DISP_PIX_PER_OCTET)
//#define FONT_W      5

//extern uint8_t frame_buffer[FB_BUF_LEN];
//extern char    font_5x7[256];

typedef struct ftbctx_type {
    uint8_t pid;            /* floating text box ID, matches index. If -1 then unused (not open)*/
    uint8_t wwrap;          /* 0:= do nothing at end of line, 1:= wrap cursor to col0, nxt page */
    uint8_t trans;          /* 0:= bkgrnd opaque, tb overwrites all fb pixels, 1:= OR with fb   */
    uint8_t scale;          /* not currently used */
    uint8_t rsvd_1;         /* reserved, not used */
    uint8_t doRender;       /* 0:= do not render this tb, 1:= render into frame buffer          */
    uint8_t x;              /* ftb x pixel-position in the framebuffer (col#)                   */
    uint8_t y;              /* ftb y pixel-position in the framebuffer (0 .. 63) (!) not page#  */
    uint8_t width;          /* ftb width, in characters                                         */
    uint8_t height;         /* ftb height, in characters                                        */
    uint8_t currx;          /* cursor X position in text box                                    */
    uint8_t curry;          /* cursor Y position in text box                                    */
    char    tb[TB_MAXLEN];  /* floating text box, at max possible size                          */
} ftbctx_t;

static ftbctx_t ftb[FTB_COUNT];

static int find_avail_ftb(void) {
    int rc = -1;
    int i;
    for ( i = 0 ; i < TB_MAXLEN ; i++ ) {
        if (ftb[i].pid == 0xff) {
            ftb[i].pid = i;
            rc = i;
            break;
        }
    }
    return rc;
}

static void ftb_tb_clear(ftbctx_t * pftb) {
    int i;
    for ( i = 0 ; i < TB_MAXLEN ; i++ )
        pftb->tb[i] = '\0';
}

static void ftb_render(ftbctx_t * pftb) {
    if (pftb && pftb->doRender) {
        uint8_t  n = pftb->y - (pftb->y & 0xF8);
        uint8_t  fbc = pftb->x;
        uint32_t fbi = ((pftb->y >> 8) * FB_WIDTH) + pftb->x;
        uint32_t fbn = fbi + FB_WIDTH;

        // n indicates the number of bits in the font col shifted down into the next page down in
        // the framebuffer. If 0 then the FTB pages are perfectly aligned vertically with the 
        // framebuffer [FB] pages.

        // fbc indexes in the FB but it is constrained to the floating text buffers pixel column and
        // cannot go outside of it. it is tracked to see if it leaves the FB max col index (127) to
        // stop rendering of the FTB when it goes outside of the text buffer viewplane.
        
        // fbi points (in FB linear array!) to the upper part of the font (or whole font col if 
        // y-pos is mult of 8) while fbn points (in FB linear array) to the next col location below 
        // that may accept some remaining part of the font (or nothing), depending on the value of 
        // y (upper-left pix posn. of the text box pos) fbi,fbn range:{0 .. 1023}

        // upper and lower column bit masks.. where the font col will be added in the FB 
        // upper and lower pages (fbi, fbn)
        uint8_t um = 0xff << n; // mask for upper page. '1' indicates a font bit location
        uint8_t lm = 0xff >> (8-n); // mask for lower page, if needed.
        
        // Iterator parameters txt_x, txt_y, font_col(0..5) (1..5) represent the character, 0 is 
        // always blank
        uint8_t tx   = 0;
        uint8_t ty   = 0;
        uint8_t col  = 0;
        
        // Points to the first CHARACTER in the floating text buffer. 
        // This is simply incremented as code iterates over each row in the floating text box
        char * ptb = &(pftb->tb[0]); // W.R.T. Floating Text Buffer (char index)
        
        // Iteration is counted as per the size of the floating text box, in characters 
        // within 6x8 pixel boxes. 
        //
        // Framebuffer parameters to be updated are: 
        //      fbc (inc, reset)    the X in FB[X][Y] in terms of a col index within limits(0 .. 127)
        //      fbp (inc)           the Y in FB[X][Y] in terms of Page# within limits (0..7)
        //                          the font char being rendered may be wholly within fbp's 8 pixels
        //                          or may be partially (up to 7 pix) shifted into fbp+1 (if within range)
        //      fbi (inc)           column index into framebuffer, the 'raw' index. represents the main render page
        //      fbn (inc)           column directly below fbi.. may exceed the index limit for FB.. check this!

        for (ty = 0 ; ty < pftb->height ; ty++) {
            // for each text line in the text box...
            
            // start over at the left side of the text box again 
            // (FB 'X' coord, not an index into the table)
            fbc = pftb->x; 
            // re-calculate where the 2 column locations are in the linear FB space as the
            // ftb page (text row) is incremented
            fbi = (((pftb->y >> 3) + ty) * FB_WIDTH) + pftb->x;
            fbn = fbi + FB_WIDTH;

            for (tx = 0 ; tx < pftb->width ; tx++) {
                // for each charactor on this row of the text box...
                
                const uint8_t * fcol = &(font_5x7[(*ptb) * FONT_W]); // 1st row in the active font
                
                for (col = 0 ; col < 6 ; col++) {
                    // is x-position physically within the confines of the frame buffer ?
                    if (fbc < FB_WIDTH) {
                        // for now, wipe the framebuffer pixels underneath the text box.. 
                        // ignore 'transparent mode'
                        frame_buffer[fbi] &= (uint8_t)~(um);
                        if (n > 0 && fbn < FB_BUF_LEN) // pages aligned or outside of FB ?
                            frame_buffer[fbn] &= (uint8_t)~(lm);
                        // transfer text font, col-by-col (5-valid cols)
                        if (col > 0) {  // 1..5 are rendered from the font character columns. 0 is a blank row
                            frame_buffer[fbi] |= ((*fcol & 0x7F) << n); // bot pixels blanked, row separator
                            if (n > 0 && fbn < FB_BUF_LEN) // pages aligned or outside of FB ?
                                frame_buffer[fbn] |= ((*fcol & 0x7F) >> (8-n));
                            fcol ++; // next col in the font
                        }
                    }
                    fbc ++; // framebuffer 'col' index position range (0..127)
                    fbi ++; // physical location into framebuffer memory, current page
                    fbn ++; // " " next page down
                }
                
                ptb ++; // next char in the floating text box
            }
        }
    }
}

void render_floating_txt_tables(void) {
    int i;
    for ( i = 0 ; i < FTB_COUNT ; i++ ) {
        if (ftb[i].pid == i) {
            ftb_render(&(ftb[i]));
        }
    }
}

void ftb_init(void) {
    int i;
    for ( i = 0 ; i < FTB_COUNT ; i++ ) {
        ftb[i].pid = 0xff; 
        ftb[i].wwrap = 0; 
        ftb[i].trans = 0; 
        ftb[i].scale = 1;
        ftb[i].rsvd_1 = 0;
        ftb[i].doRender = 0; 
        ftb[i].x = 0; 
        ftb[i].y = 0; 
        ftb[i].width = TB_WIDTH; 
        ftb[i].height = TB_HEIGHT; 
        ftb[i].currx = 0;
        ftb[i].curry = 0;
        ftb_tb_clear(&(ftb[i]));
    }
}

int ftb_new(uint8_t xpos, uint8_t ypos, uint8_t width, uint8_t hght, 
    uint8_t bkgnd_trans, uint8_t wwrap, uint8_t scale) {
    int rc = -1;
    if ( xpos < FB_WIDTH && ypos < FB_HEIGHT && width < TB_WIDTH && hght < TB_HEIGHT) {
        rc = find_avail_ftb();
    }
    if (rc >= 0) {
        ftb[rc].wwrap    = (wwrap)?1:0; 
        ftb[rc].trans    = (bkgnd_trans)?1:0; 
        // scale not yet supported.
        ftb[rc].doRender = 0; 
        ftb[rc].x      = xpos; 
        ftb[rc].y      = ypos; 
        ftb[rc].width  = width; 
        ftb[rc].height = hght; 
        ftb[rc].currx  = 0;
        ftb[rc].curry  = 0;
    }
    return rc;
}

int ftb_enable(int ftbID) {
    int rc = 1;
    if (ftbID < FTB_COUNT && ftb[ftbID].pid == ftbID) {
        ftb[ftbID].doRender = 1;
        rc = 0;
    }
    return rc;
}

int ftb_disable(int ftbID) {
    int rc = 1;
    if (ftbID < FTB_COUNT && ftb[ftbID].pid == ftbID) {
        ftb[ftbID].doRender = 0;
        rc = 0;
    }
    return rc;
}

int ftb_delete(int ftbID) {
    return 1;
}

int ftb_clear(int ftbID) {
    int rc = 1;
    if (ftbID < FTB_COUNT && ftb[ftbID].pid == ftbID) {
        ftb_tb_clear(&(ftb[ftbID]));
        ftb[ftbID].currx = 0;
        ftb[ftbID].curry = 0;
        rc = 0;
    }
    return rc;
}

int ftb_move(int ftbID, uint8_t xpos, uint8_t ypos) {
    int rc = 1;
    if ((ftbID < FTB_COUNT) && (ftb[ftbID].pid == ftbID) && 
        (xpos < FB_WIDTH) && (ypos < FB_HEIGHT)) {
        ftb[ftbID].x = xpos; 
        ftb[ftbID].y = ypos; 
        rc = 0;
    }
    return rc;
}

int ftb_cursor(int ftbID, uint8_t xpos, uint8_t ypos) {
    int rc = 1;
    if ((ftbID < FTB_COUNT) && (ftb[ftbID].pid == ftbID) && 
        (xpos < ftb[ftbID].width) && (ypos < ftb[ftbID].height)) {
        ftb[ftbID].currx = xpos;
        ftb[ftbID].curry = ypos;
        rc = 0;
    }
    return rc;
}

int ftb_home(int ftbID) {
    int rc = 1;
    if (ftbID < FTB_COUNT && ftb[ftbID].pid == ftbID) {
        ftb[ftbID].currx = 0;
        ftb[ftbID].curry = 0;
        rc = 0;
    }
    return rc;
}

int ftb_putc(int ftbID, char c) {
    int rc = -1;
    if (ftbID < FTB_COUNT && ftb[ftbID].pid == ftbID) {
        ftbctx_t * pftb = &(ftb[ftbID]);
        rc = 0;
        if (c == 0x80) {
            // backspace
            if (pftb->currx)
                if (pftb->currx < pftb->width && pftb->curry < pftb->height) {
                    // only clear chars if they  are within the text box
                    pftb->tb[ (pftb->curry * pftb->width) + pftb->currx] = '\0';
                }
                // cursor could be outside of box.. that is ok.
                pftb->currx --;
        } else if (c == '\n' || c == '\r') {
            if (pftb->curry < pftb->height) {
                pftb->currx = 0;
                pftb->curry ++;
            }
        } else if (pftb->curry < pftb->height) {
            // still within the text box
            if (pftb->wwrap) {
                // char-wrap mode enabled
                if (pftb->currx >= pftb->width) {
                    pftb->currx = 0;
                    pftb->curry ++;
                }
            }
            if (pftb->currx < pftb->width && pftb->curry < pftb->height) {
                pftb->tb[ (pftb->curry * pftb->width) + pftb->currx] = c;
                // cursor may go outside of the box at which point only a cr-lf or backspace 
                // or next char w/ wrap (or clear) can get back inside.
                pftb->currx ++;
                rc = 1;
            }
        }
    }
    return rc;
}

int ftb_puts(int ftbID, const char * s) {
    int rc = 0;
    int cc = 0;
    while (*s) {
        rc = ftb_putc(ftbID, *s);
        if (rc < 0) {
            break;
        }
        cc ++;
        s ++;
    }
    return ((rc < 0)?rc:cc);
}

int ftb_newline(int ftbID) {
    return ftb_putc(ftbID, '\n');
}

int ftb_bkspace(int ftbID) {
    return ftb_putc(ftbID, 0x80);
}


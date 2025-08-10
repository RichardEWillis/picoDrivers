/******************************************************************************
 * cpyutils
 * 
 * Common framebuffer copy operations.
 * 
 * Ver. 1.0
 * 
 * Features:
 *  - mis-aligned copy operation : copy one fb to another at any (x,y) pixel coord.
 * 
 * Limitations:
 */

#include <string.h>
#include <stdlib.h>
#include <cpyutils.h>

// Simple framebuffer copy. uses a "fast copy" 
// method when framebuffer sizes are compatible
int gfxutil_fb_merge(uint8_t * from, uint8_t * to, size_t len) {
    int rc = -1;
    if (from && to && len) {
        if (len % 4 == 0) {
            uint32_t * f = (uint32_t *)from;
            uint32_t * t = (uint32_t *)to;
            size_t c = len / 4;
            size_t j;
            for (j = 0 ; j < c ; j++)
                t[j] |= f[j];
        } else {
            size_t j;
            for (j = 0 ; j < len ; j++)
                to[j] |= from[j];
        }
        rc = len;
    }
    return rc;
}

// framebuffer --> framebuffer copy
int gfxutil_fbfb_copy(fbdata_t * from, fbdata_t * to) {
    return -1;
}

// textbuffer --> framebuffer copy
int gfxutil_tbfb_copy(fbdata_t * from, fbdata_t * to) {
    return -1;
}


// static void digit_render(uint8_t xpos, uint8_t ypos, uint8_t dig) {
//     uint8_t  fb_page  = ypos / FRAMEBUF_LINES_PER_PG;
//     uint8_t  n        = ypos % FRAMEBUF_LINES_PER_PG; // line-misalignment in no. of bits. 0 := aligned
//     uint8_t  fbmask   = 0xff >> (8 - n); // for retaining existing framebuffer data (shifted digit location)
//     uint8_t  fbmasknxt= 0xff << n;       // mask for the next page down, if shifted.
//     uint8_t  bmidx    = 0; // index into digit bitmap (0..51)
//     uint8_t  digwidth = 0; // prevent a molulo compute by monitoring the X position rel to the width of the digit to find rollover point into the next FB page.
//     uint32_t fb_start = (fb_page * FRAMEBUF_P_WIDTH) + xpos; // starting index into FB (by page)
//     const uint8_t * bm = (dig == BLANK_DIGIT_IDX) ? bmaps[0] : bmaps[dig+1];
    
//     while (bmidx < DIGIT_BUFLEN) {
//         framebuf[fb_start] = (framebuf[fb_start] & fbmask) | (bm[bmidx] << n);
//         if (n && (fb_start + FRAMEBUF_P_WIDTH) < FRAMEBUF_LEN) {
//             // for shifted copies,
//             // render remaining part of the display digit into the next page below,
//             // as long as it is still within the framebuffer
//             framebuf[fb_start+FRAMEBUF_P_WIDTH] = 
//                 (framebuf[fb_start+FRAMEBUF_P_WIDTH] & fbmasknxt) | (bm[bmidx] >> (8-n));
//         }
//         fb_start ++;
//         bmidx ++;
//         digwidth ++;
//         if (digwidth >= DIGIT_WIDTH) {
//             // re-calulate FB page and index posn for next page
//             //fb_page ++;
//             fb_start += FRAMEBUF_P_WIDTH;
//             digwidth = 0;
//         }
//     }
// }

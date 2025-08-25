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

// simple buffer clear
int gfxutil_fb_clear(uint8_t * fb, size_t len, uint8_t fast_clean) {
    int rc = -1;
    if (fb && len) {
        if (fast_clean) {
            uint32_t * f = (uint32_t *)fb;
            size_t c = len / 4;
            size_t j;
            for (j = 0 ; j < c ; j++)
                f[j] = 0;
        } else {
            memset(fb, 0, len);
        }
        rc = len;
    }
    return rc;
}

// Simple framebuffer copy. uses a "fast copy" 
// method when framebuffer sizes are compatible
int gfxutil_fb_merge(uint8_t * from, uint8_t * mask, uint8_t * to, size_t len) {
    int rc = -1;
    if (from && to && len) {
        if (len % 4 == 0) {
            uint32_t * f = (uint32_t *)from;
            uint32_t * t = (uint32_t *)to;
            uint32_t * m = (uint32_t *)mask;
            size_t c = len / 4;
            size_t j;
            for (j = 0 ; j < c ; j++) {
                if (m) {
                    t[j] &= (uint32_t)~(m[j]);
                }
                t[j] |= f[j];
            }
        } else {
            size_t j;
            for (j = 0 ; j < len ; j++) {
                if (mask) {
                    to[j] &= (uint8_t)~(mask[j]);
                }
                to[j] |= from[j];
            }
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

/******************************************************************************
 * "Blit" one smaller framebuffer window into a larger (or fullsize) 
 * Framebuffer. This is useful for some graphics layers that use ROM based
 * pixel maps for sprites or other bitmap images.
 * 
 * buffer 'from' must include its width (pixels) and height (pages) as well 
 * as whether to OR the bitmap into existing data or delete underlying data
 * in the destination framebuffer and overwrite that area.
 * 'from' must also give the top-left pixel coordinate to start in the destination
 * framebuffer.
 * 
 * buffer 'to' must have its width (pixels) and height (pages) such that the 
 * buffer octet array length is (width * pages) bytes.
 * 
 * (!) blit assumes that bitmap buffers are using a page size of 8 representing
 *     8 pixel rows per octet so the given height is in 8-pixel page groups.
 *     EXAMPLE - 64 x 16 bitmap:
 *     X = 64   64 pixels across representing 64 octets to span width
 *     Y = 2    page height is 2 octets so pixel height is (2 * 8) = 16 pixel 
 *              rows   
 *     in the above example, the buffer array length would be (64 * 2) = 128 bytes.
 *     
 * Returns:
 *      0 := Success (EXIT_SUCCESS)
 *      1 := failure, check params.
 * 
 */
#define FRAMEBUF_LINES_PER_PG 8
int gfxutil_blit(const uint8_t * from, size_t from_width, size_t from_pages, bool overwrite, size_t at_x, size_t at_y, uint8_t * to, size_t to_width, size_t to_pages) {
    int rc = 1;
    if (from && to) {
        uint8_t  to_pg    = at_y / FRAMEBUF_LINES_PER_PG;  // dest page (Y) index -> to[]
        uint8_t  n        = at_y % FRAMEBUF_LINES_PER_PG;  // line-misalignment in no. of bits. 0 := aligned
        uint8_t  fbmask   = 0xff >> (8 - n);               // for retaining existing framebuffer data (shifted digit location)
        uint8_t  fbmasknxt= 0xff << n;                     // mask for the next page down, if shifted.
        size_t   f_idx    = 0;                             // index for the source 'from' buffer
        size_t   f_len    = (from_width * from_pages);     // total length of the source buffer
        size_t   fb_start = (to_pg * to_width) + at_x;     // starting index into FB (by page)
        size_t   t_len    = (to_width * to_pages);         // byte/octet length of the dest buffer (linear array geometry)
        size_t   digwidth = 0;      // a tracking counter that catches rollover to the next page in the source bitmap

        while (f_idx < f_len) {
            // prevent overflow off the right of the page, ignore any blit going there ... er... how?
            
            if (overwrite)
                to[fb_start] &= fbmask;
            to[fb_start] |= (from[f_idx] << n);

            if (n && (fb_start + to_width) < t_len) {
                // render remaining part of the display digit into the next page below,
                // as long as it is still within the framebuffer
                if (overwrite)
                    to[fb_start+to_width] &= fbmasknxt;
                to[fb_start+to_width] |= (from[f_idx] >> (8-n));
            }

            fb_start ++;
            f_idx ++;
            digwidth ++;
            if (digwidth >= from_width) {
                // re-calulate dest buffer index by jumping to the next page and back at the start left side again.
                to_pg ++; // next page down
                fb_start = (to_pg * to_width) + at_x; // re-calc starting point in dest buffer
                digwidth = 0;
            }
        }
        rc = 0;
    }
    return rc;
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

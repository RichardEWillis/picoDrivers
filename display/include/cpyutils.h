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

#ifndef __CPYUTILS_H__
#define __CPYUTILS_H__

#include <stdio.h>
#include <stdint.h>
#include "pico/types.h"

/******************************************************************************
 * Simple (frame) buffer clear.
 * 
 * Clean the given (frame) buffer by zeroing it. This offers a fast option
 * that zeros 4 bytes ata  time, if available. caller MUST KNOW if it is 
 * possible; this funtion does not check.
 * 
 * Inputs:
 *  [uint8_t *] fb                  (frame) buffer
 *  [size_t]    len                 len of 'fb' in bytes
 *  [uint8_t]   fast_clean (flag)   if true, the fast algorithm will be used.
 * 
 * Note:
 *  [1]     generic enough to be used for any buffer...
 * 
 * Returns:
 *     -1           processing error
 *      0           nothing cleared
 *      1+          # bytes cleared (matches len)
 */
int gfxutil_fb_clear(uint8_t * fb, size_t len, uint8_t fast_clean);

/******************************************************************************
 * Simple framebuffer merge.
 * 
 * Both buffers must be 'len' bytes.
 * No Mask (mask == NULL):
 *      Buffer 'to' is logically OR'd with 'from' and saved to 'to' :
 *      to = to | from
 * With Mask (mask != NULL):
 *      Buffer 'to' is masked with 'mask' such that any bit location in the
 *      mask set to '1' will mask out the pixel in 'to'; setting it to zero.
 *      Then 'to' is OR'd with 'from'.
 *      to = to & ~mask
 *      to = to | from
 * 
 * If buffers are a multiple of 4 bytes then a "fast merge" algorithm is used 
 * instead.
 * 
 * Returns:
 *     -1           processing error
 *      0           nothing copied
 *      1+          # bytes merged from one fb to the other.
 */
int gfxutil_fb_merge(uint8_t * from, uint8_t * mask, uint8_t * to, size_t len);

// detailed info on source and destination framebuffers required for the copy.
typedef struct fbdata_type {
    uint8_t tl_posn_x; /* top-left start position for the copy */
    uint8_t tl_posn_y; /* top-left start position for the copy */
    uint8_t fb_width;  /* width, in pixels/text */
    uint8_t fb_height; /* height, in pixels/text */
    uint8_t rows_per_page; /* # row in each framebuffer page (grouping of rows) */
    uint8_t resvd[3];  /* for alignment */
    uint32_t cpylen;   /* #bytes to copy out of the source fb */
    char *  ftb;        /* the framebuffer/textbuffer memory */
} fbdata_t;

/******************************************************************************
 * Copy one pixel framebuffer to another.
 * 
 * Source (from) must configure:
 *      (fb_width, fb_height, rows_per_page)    
 *                  Geometry of the framebuffer
 *      cpylen      Bytelen of the framebuffer to copy (up to entire length 
 *                  of fb)
 *      ftb         framebuffer memory, array of page octets
 * Destination (to) must configure:
 *      (tl_posn_x, tl_posn_y)
 *                  pixel coords to start the copy at in the dest. buffer
 *      (fb_width, fb_height, rows_per_page)    
 *                  Geometry of the framebuffer. if smaller than src the 
 *                  copy will stop at the last byte in the dest. buffer.
 *      ftb         framebuffer memory, array of page octets
 *
 * Notes:
 *  [1]     geometry:rows_per_page MUST match between 'from' and 'to' otherwise
 *          call will return 0 (nothing copied).
 * Returns:
 *     -1           processing error
 *      0           nothing copied
 *      1+          # bytes copied from one fb to the other.
 */
int gfxutil_fbfb_copy(fbdata_t * from, fbdata_t * to);

/******************************************************************************
 * Copy a text buffer into a framebuffer.
 * 
 * textbuffer source (from) must configure:
 *      (fb_width, fb_height)
 *                  Geometry of the text buffer, in characters.
 *      cpylen      Bytelen of the framebuffer to copy (up to entire length 
 *                  of fb)
 *      ftb         textbuffer memory, array of characters
 * framebuffer destination (to) must configure:
 *      (tl_posn_x, tl_posn_y)
 *                  pixel coords to start the copy at in the dest. buffer
 *      (fb_width, fb_height, rows_per_page)    
 *                  Geometry of the framebuffer. if smaller than src the 
 *                  copy will stop at the last byte in the dest. buffer.
 *      ftb         framebuffer memory, array of page octets
 * 
*/
int gfxutil_tbfb_copy(fbdata_t * from, fbdata_t * to);

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
int gfxutil_blit(const uint8_t * from, size_t from_width, size_t from_pages, 
    bool overwrite, size_t at_x, size_t at_y, uint8_t * to, size_t to_width, 
    size_t to_pages);



#endif /* __CPYUTILS_H__ */

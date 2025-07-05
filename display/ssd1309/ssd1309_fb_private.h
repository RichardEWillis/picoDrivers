// used only by textbuffer code, not a public API

#ifndef __SSD1309_FB_PRIVATE_H__
#define __SSD1309_FB_PRIVATE_H__

// Set this to 1 to use 'fast' copies for frame buffer operations.
// This will only work if the size of the framebuffer is a multiple
// of 4 (can be cast to uint32_t)
// ssd1309 display is 1024 bytes (8192 pixels) so this can be enabled.
#define USE_FAST_FB_COPY    1

// Set the extents of the Text buffer (width, height, length)
#define TXTBUFFER_WIDTH     20
#define TXTBUFFER_HEIGHT    8
#define TXTBUFFER_LEN       (TXTBUFFER_WIDTH * TXTBUFFER_HEIGHT)
#define TXTBUFFER_LFT_OSET  4   /* left margin size, in pixels, right not specified.                  */
// upper and lower blank boarders not supported on the ssd1309 for 8-bit high fonts
#define FONT_W              5   /* font size (rendered pixel size)                                    */

// If set to 1, the table will be shortened to remove all
// characters in indexes 0..31, leaving [space](0x20)32d at
// index 0. Also indices 127 .. 255 are removed.
#define NO_WHTSPC_CHARS     0

#if (NO_WHTSPC_CHARS == 1)
static uint ftable_zero_offset = 0x20;
static uint ftable_zero_len    = 0x5F; // 95 chars, not including [127] 'DEL' (not rendered) (affects cursor pos)
#else
static uint ftable_zero_offset = 0;
static uint ftable_zero_len    = 0x100;  /* 256 chars (full table) */
#endif


#endif /* __SSD1309_FB_PRIVATE_H__ */


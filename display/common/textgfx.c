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

#include <string.h>
#include <stdlib.h>
#include <textgfx.h>
#include <gfxDriverLowPriv.h>

// The following chars are not looked up but instead directly
// affect the screen cursor: \n, \r, 'DEL'

// Text map - 6 x 8 characters ------------------------------------------------
#define FONT_5x7_WIDTH  6
#define FONT_5x7_HEIGHT 8
#define FONT_W          5
// 5 x 7 font, vertically sliced. 5 bytes per char.
// eg. 0x20 'spc' @ idx = (0x20 * 5)
static const uint8_t font_5x7[] = {
#if (NO_WHTSPC_CHARS == 0)
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x3E, 0x5B, 0x4F, 0x5B, 0x3E,
	0x3E, 0x6B, 0x4F, 0x6B, 0x3E,
	0x1C, 0x3E, 0x7C, 0x3E, 0x1C,
	0x18, 0x3C, 0x7E, 0x3C, 0x18,
	0x1C, 0x57, 0x7D, 0x57, 0x1C,
	0x1C, 0x5E, 0x7F, 0x5E, 0x1C,
	0x00, 0x18, 0x3C, 0x18, 0x00,
	0xFF, 0xE7, 0xC3, 0xE7, 0xFF,
	0x00, 0x18, 0x24, 0x18, 0x00,
	0xFF, 0xE7, 0xDB, 0xE7, 0xFF,
	0x30, 0x48, 0x3A, 0x06, 0x0E,
	0x26, 0x29, 0x79, 0x29, 0x26,
	0x40, 0x7F, 0x05, 0x05, 0x07,
	0x40, 0x7F, 0x05, 0x25, 0x3F,
	0x5A, 0x3C, 0xE7, 0x3C, 0x5A,
	0x7F, 0x3E, 0x1C, 0x1C, 0x08,
	0x08, 0x1C, 0x1C, 0x3E, 0x7F,
	0x14, 0x22, 0x7F, 0x22, 0x14,
	0x5F, 0x5F, 0x00, 0x5F, 0x5F,
	0x06, 0x09, 0x7F, 0x01, 0x7F,
	0x00, 0x66, 0x89, 0x95, 0x6A,
	0x60, 0x60, 0x60, 0x60, 0x60,
	0x94, 0xA2, 0xFF, 0xA2, 0x94,
	0x08, 0x04, 0x7E, 0x04, 0x08,
	0x10, 0x20, 0x7E, 0x20, 0x10,
	0x08, 0x08, 0x2A, 0x1C, 0x08,
	0x08, 0x1C, 0x2A, 0x08, 0x08,
	0x1E, 0x10, 0x10, 0x10, 0x10,
	0x0C, 0x1E, 0x0C, 0x1E, 0x0C,
	0x30, 0x38, 0x3E, 0x38, 0x30,
	0x06, 0x0E, 0x3E, 0x0E, 0x06,
#endif
	0x00, 0x00, 0x00, 0x00, 0x00,   // Space, index 0 or 20
	0x00, 0x00, 0x5F, 0x00, 0x00,
	0x00, 0x07, 0x00, 0x07, 0x00,
	0x14, 0x7F, 0x14, 0x7F, 0x14,
	0x24, 0x2A, 0x7F, 0x2A, 0x12,
	0x23, 0x13, 0x08, 0x64, 0x62,
	0x36, 0x49, 0x56, 0x20, 0x50,
	0x00, 0x08, 0x07, 0x03, 0x00,
	0x00, 0x1C, 0x22, 0x41, 0x00,
	0x00, 0x41, 0x22, 0x1C, 0x00,
	0x2A, 0x1C, 0x7F, 0x1C, 0x2A,
	0x08, 0x08, 0x3E, 0x08, 0x08,
	0x00, 0x80, 0x70, 0x30, 0x00,
	0x08, 0x08, 0x08, 0x08, 0x08,
	0x00, 0x00, 0x60, 0x60, 0x00,
	0x20, 0x10, 0x08, 0x04, 0x02,
	0x3E, 0x51, 0x49, 0x45, 0x3E,
	0x00, 0x42, 0x7F, 0x40, 0x00,
	0x72, 0x49, 0x49, 0x49, 0x46,
	0x21, 0x41, 0x49, 0x4D, 0x33,
	0x18, 0x14, 0x12, 0x7F, 0x10,
	0x27, 0x45, 0x45, 0x45, 0x39,
	0x3C, 0x4A, 0x49, 0x49, 0x31,
	0x41, 0x21, 0x11, 0x09, 0x07,
	0x36, 0x49, 0x49, 0x49, 0x36,
	0x46, 0x49, 0x49, 0x29, 0x1E,
	0x00, 0x00, 0x14, 0x00, 0x00,
	0x00, 0x40, 0x34, 0x00, 0x00,
	0x00, 0x08, 0x14, 0x22, 0x41,
	0x14, 0x14, 0x14, 0x14, 0x14,
	0x00, 0x41, 0x22, 0x14, 0x08,
	0x02, 0x01, 0x59, 0x09, 0x06,
	0x3E, 0x41, 0x5D, 0x59, 0x4E,
	0x7C, 0x12, 0x11, 0x12, 0x7C,
	0x7F, 0x49, 0x49, 0x49, 0x36,
	0x3E, 0x41, 0x41, 0x41, 0x22,
	0x7F, 0x41, 0x41, 0x41, 0x3E,
	0x7F, 0x49, 0x49, 0x49, 0x41,
	0x7F, 0x09, 0x09, 0x09, 0x01,
	0x3E, 0x41, 0x41, 0x51, 0x73,
	0x7F, 0x08, 0x08, 0x08, 0x7F,
	0x00, 0x41, 0x7F, 0x41, 0x00,
	0x20, 0x40, 0x41, 0x3F, 0x01,
	0x7F, 0x08, 0x14, 0x22, 0x41,
	0x7F, 0x40, 0x40, 0x40, 0x40,
	0x7F, 0x02, 0x1C, 0x02, 0x7F,
	0x7F, 0x04, 0x08, 0x10, 0x7F,
	0x3E, 0x41, 0x41, 0x41, 0x3E,
	0x7F, 0x09, 0x09, 0x09, 0x06,
	0x3E, 0x41, 0x51, 0x21, 0x5E,
	0x7F, 0x09, 0x19, 0x29, 0x46,
	0x26, 0x49, 0x49, 0x49, 0x32,
	0x03, 0x01, 0x7F, 0x01, 0x03,
	0x3F, 0x40, 0x40, 0x40, 0x3F,
	0x1F, 0x20, 0x40, 0x20, 0x1F,
	0x3F, 0x40, 0x38, 0x40, 0x3F,
	0x63, 0x14, 0x08, 0x14, 0x63,
	0x03, 0x04, 0x78, 0x04, 0x03,
	0x61, 0x59, 0x49, 0x4D, 0x43,
	0x00, 0x7F, 0x41, 0x41, 0x41,
	0x02, 0x04, 0x08, 0x10, 0x20,
	0x00, 0x41, 0x41, 0x41, 0x7F,
	0x04, 0x02, 0x01, 0x02, 0x04,
	0x40, 0x40, 0x40, 0x40, 0x40,
	0x00, 0x03, 0x07, 0x08, 0x00,
	0x20, 0x54, 0x54, 0x78, 0x40,
	0x7F, 0x28, 0x44, 0x44, 0x38,
	0x38, 0x44, 0x44, 0x44, 0x28,
	0x38, 0x44, 0x44, 0x28, 0x7F,
	0x38, 0x54, 0x54, 0x54, 0x18,
	0x00, 0x08, 0x7E, 0x09, 0x02,
	0x18, 0xA4, 0xA4, 0x9C, 0x78,
	0x7F, 0x08, 0x04, 0x04, 0x78,
	0x00, 0x44, 0x7D, 0x40, 0x00,
	0x20, 0x40, 0x40, 0x3D, 0x00,
	0x7F, 0x10, 0x28, 0x44, 0x00,
	0x00, 0x41, 0x7F, 0x40, 0x00,
	0x7C, 0x04, 0x78, 0x04, 0x78,
	0x7C, 0x08, 0x04, 0x04, 0x78,
	0x38, 0x44, 0x44, 0x44, 0x38,
	0xFC, 0x18, 0x24, 0x24, 0x18,
	0x18, 0x24, 0x24, 0x18, 0xFC,
	0x7C, 0x08, 0x04, 0x04, 0x08,
	0x48, 0x54, 0x54, 0x54, 0x24,
	0x04, 0x04, 0x3F, 0x44, 0x24,
	0x3C, 0x40, 0x40, 0x20, 0x7C,
	0x1C, 0x20, 0x40, 0x20, 0x1C,
	0x3C, 0x40, 0x30, 0x40, 0x3C,
	0x44, 0x28, 0x10, 0x28, 0x44,
	0x4C, 0x90, 0x90, 0x90, 0x7C,
	0x44, 0x64, 0x54, 0x4C, 0x44,
	0x00, 0x08, 0x36, 0x41, 0x00,
	0x00, 0x00, 0x77, 0x00, 0x00,
	0x00, 0x41, 0x36, 0x08, 0x00,
	0x02, 0x01, 0x02, 0x04, 0x02,
#if (NO_WHTSPC_CHARS == 0)
	0x3C, 0x26, 0x23, 0x26, 0x3C,
	0x1E, 0xA1, 0xA1, 0x61, 0x12,
	0x3A, 0x40, 0x40, 0x20, 0x7A,
	0x38, 0x54, 0x54, 0x55, 0x59,
	0x21, 0x55, 0x55, 0x79, 0x41,
	0x21, 0x54, 0x54, 0x78, 0x41,
	0x21, 0x55, 0x54, 0x78, 0x40,
	0x20, 0x54, 0x55, 0x79, 0x40,
	0x0C, 0x1E, 0x52, 0x72, 0x12,
	0x39, 0x55, 0x55, 0x55, 0x59,
	0x39, 0x54, 0x54, 0x54, 0x59,
	0x39, 0x55, 0x54, 0x54, 0x58,
	0x00, 0x00, 0x45, 0x7C, 0x41,
	0x00, 0x02, 0x45, 0x7D, 0x42,
	0x00, 0x01, 0x45, 0x7C, 0x40,
	0xF0, 0x29, 0x24, 0x29, 0xF0,
	0xF0, 0x28, 0x25, 0x28, 0xF0,
	0x7C, 0x54, 0x55, 0x45, 0x00,
	0x20, 0x54, 0x54, 0x7C, 0x54,
	0x7C, 0x0A, 0x09, 0x7F, 0x49,
	0x32, 0x49, 0x49, 0x49, 0x32,
	0x32, 0x48, 0x48, 0x48, 0x32,
	0x32, 0x4A, 0x48, 0x48, 0x30,
	0x3A, 0x41, 0x41, 0x21, 0x7A,
	0x3A, 0x42, 0x40, 0x20, 0x78,
	0x00, 0x9D, 0xA0, 0xA0, 0x7D,
	0x39, 0x44, 0x44, 0x44, 0x39,
	0x3D, 0x40, 0x40, 0x40, 0x3D,
	0x3C, 0x24, 0xFF, 0x24, 0x24,
	0x48, 0x7E, 0x49, 0x43, 0x66,
	0x2B, 0x2F, 0xFC, 0x2F, 0x2B,
	0xFF, 0x09, 0x29, 0xF6, 0x20,
	0xC0, 0x88, 0x7E, 0x09, 0x03,
	0x20, 0x54, 0x54, 0x79, 0x41,
	0x00, 0x00, 0x44, 0x7D, 0x41,
	0x30, 0x48, 0x48, 0x4A, 0x32,
	0x38, 0x40, 0x40, 0x22, 0x7A,
	0x00, 0x7A, 0x0A, 0x0A, 0x72,
	0x7D, 0x0D, 0x19, 0x31, 0x7D,
	0x26, 0x29, 0x29, 0x2F, 0x28,
	0x26, 0x29, 0x29, 0x29, 0x26,
	0x30, 0x48, 0x4D, 0x40, 0x20,
	0x38, 0x08, 0x08, 0x08, 0x08,
	0x08, 0x08, 0x08, 0x08, 0x38,
	0x2F, 0x10, 0xC8, 0xAC, 0xBA,
	0x2F, 0x10, 0x28, 0x34, 0xFA,
	0x00, 0x00, 0x7B, 0x00, 0x00,
	0x08, 0x14, 0x2A, 0x14, 0x22,
	0x22, 0x14, 0x2A, 0x14, 0x08,
	0xAA, 0x00, 0x55, 0x00, 0xAA,
	0xAA, 0x55, 0xAA, 0x55, 0xAA,
	0x00, 0x00, 0x00, 0xFF, 0x00,
	0x10, 0x10, 0x10, 0xFF, 0x00,
	0x14, 0x14, 0x14, 0xFF, 0x00,
	0x10, 0x10, 0xFF, 0x00, 0xFF,
	0x10, 0x10, 0xF0, 0x10, 0xF0,
	0x14, 0x14, 0x14, 0xFC, 0x00,
	0x14, 0x14, 0xF7, 0x00, 0xFF,
	0x00, 0x00, 0xFF, 0x00, 0xFF,
	0x14, 0x14, 0xF4, 0x04, 0xFC,
	0x14, 0x14, 0x17, 0x10, 0x1F,
	0x10, 0x10, 0x1F, 0x10, 0x1F,
	0x14, 0x14, 0x14, 0x1F, 0x00,
	0x10, 0x10, 0x10, 0xF0, 0x00,
	0x00, 0x00, 0x00, 0x1F, 0x10,
	0x10, 0x10, 0x10, 0x1F, 0x10,
	0x10, 0x10, 0x10, 0xF0, 0x10,
	0x00, 0x00, 0x00, 0xFF, 0x10,
	0x10, 0x10, 0x10, 0x10, 0x10,
	0x10, 0x10, 0x10, 0xFF, 0x10,
	0x00, 0x00, 0x00, 0xFF, 0x14,
	0x00, 0x00, 0xFF, 0x00, 0xFF,
	0x00, 0x00, 0x1F, 0x10, 0x17,
	0x00, 0x00, 0xFC, 0x04, 0xF4,
	0x14, 0x14, 0x17, 0x10, 0x17,
	0x14, 0x14, 0xF4, 0x04, 0xF4,
	0x00, 0x00, 0xFF, 0x00, 0xF7,
	0x14, 0x14, 0x14, 0x14, 0x14,
	0x14, 0x14, 0xF7, 0x00, 0xF7,
	0x14, 0x14, 0x14, 0x17, 0x14,
	0x10, 0x10, 0x1F, 0x10, 0x1F,
	0x14, 0x14, 0x14, 0xF4, 0x14,
	0x10, 0x10, 0xF0, 0x10, 0xF0,
	0x00, 0x00, 0x1F, 0x10, 0x1F,
	0x00, 0x00, 0x00, 0x1F, 0x14,
	0x00, 0x00, 0x00, 0xFC, 0x14,
	0x00, 0x00, 0xF0, 0x10, 0xF0,
	0x10, 0x10, 0xFF, 0x10, 0xFF,
	0x14, 0x14, 0x14, 0xFF, 0x14,
	0x10, 0x10, 0x10, 0x1F, 0x00,
	0x00, 0x00, 0x00, 0xF0, 0x10,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
	0xFF, 0xFF, 0xFF, 0x00, 0x00,
	0x00, 0x00, 0x00, 0xFF, 0xFF,
	0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
	0x38, 0x44, 0x44, 0x38, 0x44,
	0x7C, 0x2A, 0x2A, 0x3E, 0x14,
	0x7E, 0x02, 0x02, 0x06, 0x06,
	0x02, 0x7E, 0x02, 0x7E, 0x02,
	0x63, 0x55, 0x49, 0x41, 0x63,
	0x38, 0x44, 0x44, 0x3C, 0x04,
	0x40, 0x7E, 0x20, 0x1E, 0x20,
	0x06, 0x02, 0x7E, 0x02, 0x02,
	0x99, 0xA5, 0xE7, 0xA5, 0x99,
	0x1C, 0x2A, 0x49, 0x2A, 0x1C,
	0x4C, 0x72, 0x01, 0x72, 0x4C,
	0x30, 0x4A, 0x4D, 0x4D, 0x30,
	0x30, 0x48, 0x78, 0x48, 0x30,
	0xBC, 0x62, 0x5A, 0x46, 0x3D,
	0x3E, 0x49, 0x49, 0x49, 0x00,
	0x7E, 0x01, 0x01, 0x01, 0x7E,
	0x2A, 0x2A, 0x2A, 0x2A, 0x2A,
	0x44, 0x44, 0x5F, 0x44, 0x44,
	0x40, 0x51, 0x4A, 0x44, 0x40,
	0x40, 0x44, 0x4A, 0x51, 0x40,
	0x00, 0x00, 0xFF, 0x01, 0x03,
	0xE0, 0x80, 0xFF, 0x00, 0x00,
	0x08, 0x08, 0x6B, 0x6B, 0x08,
	0x36, 0x12, 0x36, 0x24, 0x36,
	0x06, 0x0F, 0x09, 0x0F, 0x06,
	0x00, 0x00, 0x18, 0x18, 0x00,
	0x00, 0x00, 0x10, 0x10, 0x00,
	0x30, 0x40, 0xFF, 0x01, 0x01,
	0x00, 0x1F, 0x01, 0x01, 0x1E,
	0x00, 0x19, 0x1D, 0x17, 0x12,
	0x00, 0x3C, 0x3C, 0x3C, 0x3C,
	0x00, 0x00, 0x00, 0x00, 0x00
#endif
};

// PUBLIC Display driver stack pointer is 'g_llGfxDrvr'

// x,y CURRENT cursor position (text box)
static uint8_t curx = (uint8_t)-1;
static uint8_t cury = (uint8_t)-1;
// max limits of the text box, obtained from the display geometry
static uint8_t curx_max = 0; // one minux width
static uint8_t cury_max = 0; // one minux width
static uint8_t char_width = (uint8_t)-1;
static uint8_t char_height = (uint8_t)-1;
// Operation Modes
static uint8_t txt_mode = 0;
static uint8_t txt_wrap = 0;
// Text Buffer, created once we know the geometry of the display
static char * text_buffer = NULL; 
static size_t textBufLen = 0;
// Some info on the graphics framebuffer and the alignment
// of the text buffer over it
static uint32_t fb_pix_cols = 0;        /* aka pixel width */
static uint32_t tb_left_offset = 0;     /* left offest of TB in the FB ~ 1/2 of width difference */

#define FTB_CTX_WMARK 0xFEEDFACE
#define FTBIDX(x,y,w) ((y * w) + x)

// Floating Text buffer handle ------------------------------------------------
typedef struct ftbgfx_type {
	uint32_t    wmark;                  /* watermark identifying this as a FTB context, see FTB_CTX_WMARK */
    int32_t     ctx_id;                 /* unique ID for this context, when in-use. If unused then this is 0 */
	uint8_t     tl_xpos;
	uint8_t	    tl_ypos;
	uint8_t	    tb_width;
	uint8_t	    tb_height;
	uint8_t	    bk_trans;
	uint8_t     txt_scale;
	uint8_t     word_wrap;
	uint8_t     currx;
	uint8_t     curry;
	uint8_t     do_render;  /* set true to get the underlying text layer to render this box to the screen. if 0 then it's hidden */
	uint8_t     fb_width;	/* gfx framebuffer, number of horz. rows */
	uint8_t     fb_pages;   /* gfx framebuffer, number of vert. pages */
	/* the text buffer (contains characters) */
	uint32_t    tbuf_len;
	char *      tbuf;
	/* the destination framebuffer to render onto */
	uint32_t    fb_len;
	uint8_t *   frame_buffer; 
} ftbgfx_t;
typedef ftbgfx_t * ftbgfx_p;

// Fixed table of FTB contexts
static ftbgfx_t ftbObjList[FTB_COUNT] = {0};

// ID generator
static int32_t ftbIDGen = 0;  /* pre-increment */

// ----------------------------------------------------------------------------
// --- Static Text Box API
// ----------------------------------------------------------------------------

static int textgfx_render(void);

static int tbuf_fill(char c) {
    int rc = 1;
    if (text_buffer) {
        memset(text_buffer, (int)c, textBufLen);
		if (txt_mode == REFRESH_ON_TEXT_CHANGE) {
			rc = textgfx_render(); // returns 0 on success
		} else {
        	rc = 0;
		}
    }
    return rc;
}

static int tbuf_clear(void) {
    return tbuf_fill('\0');
}

// render current text buffer contents into the 
// graphics drivers framebuffer.
// Returns 1 on problems, 0 on success.
static int textgfx_render(void) {
    int rc = 1;
    if (text_buffer) {
        uint32_t  i,j;
        uint32_t  fptr = tb_left_offset;    // (STARTING) index for gfx framebuffer
        uint32_t  page = (uint32_t)-1;      // gfx framebuffer page#, rolls over to 0 in the first loop iteration
        const uint8_t * ftb = NULL;         // font buffer pointer, for the n cols of a character. points to leftmost col to start
        char * tb = &(text_buffer[0]);      // shorter alias 
        uint8_t * frame_buffer = g_llGfxDrvr->get_drvrFrameBuffer();

        for ( i = 0 ; i < textBufLen ; i++ ) {
            // for each character location in the text buffer ...
            if ( (i % char_width) == 0 ) {
                // at end of current line ...
                // capture a text buffer line wrap, re-calculate index into the gfx frame buffer
                page ++;
                fptr = (page * fb_pix_cols) + tb_left_offset;
            }
            // render character, by colunms
            ftb = &(font_5x7[(*tb) * FONT_W]); // 1st col. of the font char.
            for ( j = 0 ; j < FONT_W ; j++ ) {
                frame_buffer[fptr++] = (*ftb) & 0x7F; // msb (bot of font) is blanked
                ftb++; // next col in the font character
            }
            frame_buffer[fptr++] = 0; // last col of the font is blank (vert. spacing)
            tb ++; // next char in the text buffer
        }

        // update screen from changed framebuffer
		// use the higher level BSP API to ensure all fb layers
		// are properly merged before written to screen.
		rc = gfx_displayRefresh();
        //rc = g_llGfxDrvr->refreshDisplay(frame_buffer);
    }
    return rc;
}

int textgfx_init(int mode, int wrap) {
    int rc = 1;
    if (!text_buffer && g_llGfxDrvr->IsReady()) {
        size_t disp_pix_width = g_llGfxDrvr->get_DispWidth();
        size_t disp_pix_height = g_llGfxDrvr->get_DispHeight();
        size_t disp_page_height = g_llGfxDrvr->get_DispPageHeight();
        // For this version we expect the text to fit within one 
        // page of the display. Display pages are groupings of 
        // usually 8 rows of pixels. As such pages are arranged
        // as vertically stacked row groupings in the screen
        // area.
        if (disp_pix_height >= FONT_5x7_HEIGHT) {
            txt_mode = (uint8_t)mode;
            txt_wrap = (uint8_t)wrap;
            curx = cury = 0;
            char_width = (uint8_t)(disp_pix_width / FONT_5x7_WIDTH);
            char_height = (uint8_t)disp_page_height;
            curx_max = char_width - 1;
            cury_max = char_height - 1;
            fb_pix_cols = (uint32_t)disp_pix_width;
            tb_left_offset = (disp_pix_width - (char_width * FONT_5x7_WIDTH)) / 2;
            textBufLen = (char_width * char_height);
            text_buffer = (char *)malloc(textBufLen);
            if (text_buffer) {
                tbuf_clear();
                rc = 0;
            }
        }
    }
    return rc;
}

int textgfx_get_width(void) {
    return (int)char_width;
}

int textgfx_get_height(void) {
    return (int)char_height;
}

int textgfx_cursor(uint x, uint y) {
    int rc = 1;
    if ((x < char_width) && (y < char_height)) {
        curx = x;
        cury = y;
        rc = 0;
    }
    return rc;
}

int textgfx_get_cursor_posn_x(void) {
    return (int)curx;
}

int textgfx_get_cursor_posn_y(void) {
    return (int)cury;
}

int textgfx_get_refresh_mode(void) {
	return (int)txt_mode;
}

int textgfx_get_text_wrap_mode(void) {
	return (int)txt_wrap;
}

int textgfx_set_refresh_mode(int mode) {
	int rc = 1;
	if (text_buffer) {
		txt_mode = (mode) ? REFRESH_ON_DEMAND : REFRESH_ON_TEXT_CHANGE;
		rc = 0;
	}
	return rc;
}

int textgfx_set_text_wrap_mode(int wrap) {
	int rc = 1;
	if (text_buffer) {
		txt_wrap = (wrap) ? SET_TEXTWRAP_ON : SET_TEXTWRAP_OFF;
		rc = 0;
	}
	return rc;
}

int textgfx_clear(void) {
	curx = cury = 0;
    return tbuf_clear();
}

// returns -1 on error, else +1 for char placed or 0 if could not place.
int textgfx_putc(char c) {
    int rc = -1;
    if (text_buffer) {
        if (c == '\n' || c == '\r') {
            curx = 0;
            if (cury < char_height) {
                cury ++; // if this is eq to 'char_height' now, then cursor is out of the text box area.
            }
            rc = 1;
        } else if (c == 0x127) {
            if (curx) {
                curx --; // backspace (not supporting wrapping back up to prev page (x=0) on wordwrap.. may change this)
            }
            rc = 1;
        } else if ((curx < char_width) && (cury < char_height)) {
            text_buffer[cury * char_width + curx] = c;
            curx ++;
            if (curx >= char_width && txt_wrap) {
                curx = 0;
                if (cury < char_height) {
                    cury ++;
                }
            } // if no text wrap then cursor x position can go out of the text box area here.
            rc = 1;
        } else {
            rc = 0;
        }
		if ((rc > 0) && (txt_mode == REFRESH_ON_TEXT_CHANGE)) {
			if (textgfx_render() < 0) {
				rc = -1; // some error occured during rendering
			}
		}
    }
    return rc;
}

int textgfx_puts(const char * s) {
    int rc = -1;
	uint8_t old_txt_mode = txt_mode;
	txt_mode = REFRESH_ON_DEMAND; // prevents char updates from refreshing screen, wait till all chars added.
    if (text_buffer && s) {
        int _rc = 1;
        rc = 0; // counter
        while ((_rc >0) && *s != '\0') {
            _rc = textgfx_putc(*s++);
            if (_rc > 0) {
                rc ++; // count added chars, if char copy is interrupted then return # that had gotten placed
            }
        }
		if ((rc > 0) && (old_txt_mode == REFRESH_ON_TEXT_CHANGE)) {
			if (textgfx_render() < 0) {
				rc = -1; // some error occured during rendering
			}
		}
	}
	txt_mode = old_txt_mode; // restore mode
    return rc;
}

int textgfx_newline(void) {
    return textgfx_putc('\n');
}

int textgfx_refresh(void) {
    return textgfx_render();
}


// ----------------------------------------------------------------------------
// --- Floating Text Box API
// --- Ver 2.0 : can use FTB, static text layer or both. FTB is no longer
// ---           dependent on the static text system.
// ----------------------------------------------------------------------------
#define DLY_WRITE_FB  0
#define DO_WRITE_FB   1
static void ftb_render(ftbgfx_p pftb, int do_writeFB) {
    if (pftb && pftb->do_render) {
		uint8_t  FB_WIDTH = pftb->fb_width; // pixel framebuffer width, horz pixel count
        uint8_t  n = pftb->tl_ypos - (pftb->tl_ypos & 0xF8);
        uint8_t  fbc = pftb->tl_xpos;
        uint32_t fbi = ((uint32_t)(pftb->tl_ypos >> 8) * FB_WIDTH) + pftb->tl_xpos;
        uint32_t fbn = fbi + FB_WIDTH;
		uint8_t * frame_buffer = pftb->frame_buffer;
		uint32_t FB_BUF_LEN = pftb->fb_len;

        // n indicates the number of bits in the font col shifted down into the next page down in
        // the framebuffer. If 0 then the FTB pages are perfectly aligned vertically with the 
        // framebuffer [FB] pages.

        // fbc indexes in the FB but it is constrained to the floating text buffers pixel column and
        // cannot go outside of it. it is tracked to see if it leaves the FB max col index (FB_WIDTH-1) to
        // stop rendering of the FTB when it goes outside of the text buffer viewplane.
        
        // fbi points (in FB linear array!) to the upper part of the font (or whole font col if 
        // y-pos is mult of 8) while fbn points (in FB linear array) to the next col location below 
        // that may accept some remaining part of the font (or nothing), depending on the value of 
        // y (upper-left pix posn. of the text box pos) fbi,fbn range:{0 .. FB_BUF_LEN-1}

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
        char * ptb = &(pftb->tbuf[0]); // W.R.T. Floating Text Buffer (char index)
        
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

        for (ty = 0 ; ty < pftb->tb_height ; ty++) {
            // for each text line in the text box...
            
            // start over at the left side of the text box again 
            // (FB 'X' coord, not an index into the table)
            fbc = pftb->tl_xpos; 
            // re-calculate where the 2 column locations are in the linear FB space as the
            // ftb page (text row) is incremented
            fbi = (((pftb->tl_ypos >> 3) + ty) * FB_WIDTH) + pftb->tl_xpos;
            fbn = fbi + FB_WIDTH;

            for (tx = 0 ; tx < pftb->tb_width ; tx++) {
                // for each charactor on this row of the text box...
                
                const uint8_t * fcol = &(font_5x7[(*ptb) * FONT_W]); // 1st row in the active font
                
                for (col = 0 ; col < FONT_5x7_WIDTH ; col++) {
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
		if (do_writeFB) {
        	// update screen from changed framebuffer
			// use higher level call to pull in other fb layers
			gfx_displayRefresh();
        	//g_llGfxDrvr->refreshDisplay(frame_buffer);
		}
    }
}

static void render_floating_txt_tables(void) {
    int i;
    for ( i = 0 ; i < FTB_COUNT ; i++ ) {
        if (ftbObjList[i].ctx_id > 0) {
			// note: each text box *could* be on a separate framebuffer, so for now I have to write FB to screen for every table update..
            ftb_render(&(ftbObjList[i]),DO_WRITE_FB);
        }
    }
}

static void ftbgfx_tb_clear(ftbgfx_p pftb) {
    int i;
	if (pftb && pftb->tbuf)
    	for ( i = 0 ; i < pftb->tbuf_len ; i++ )
        	pftb->tbuf[i] = '\0';
}

void ftbgfx_init(void) {
	int i;
	for ( i=0 ; i < FTB_COUNT ; i++ ) {
		ftbObjList[i].ctx_id = 0;
	}
}

int ftbgfx_get_max_pix_width(void) {
    return g_llGfxDrvr->get_DispWidth();
}

int ftbgfx_get_max_pix_height(void) {
    return g_llGfxDrvr->get_DispHeight();
}

int ftbgfx_get_max_textbox_width(void) {
    return ftbgfx_get_max_pix_width() / FONT_5x7_WIDTH;
}

// assuming for now that the font height fits within 
// a screen page height, typ. 8 pixels.
int ftbgfx_get_max_textbox_height(void) {
    return g_llGfxDrvr->get_DispPageHeight();
}

void * ftbgfx_new(uint8_t xpos, uint8_t ypos, uint8_t width, uint8_t hght, uint8_t bkgnd_trans, uint8_t wwrap, uint8_t scale) {
	int i;
	ftbgfx_p phndl = NULL;
	int pix_width = ftbgfx_get_max_pix_width();
	int pix_hght  = ftbgfx_get_max_pix_height();
	
	// check inputs for out of range
	if (width * FONT_5x7_WIDTH + xpos >= pix_width )
		return NULL;
	if (hght * FONT_5x7_HEIGHT + ypos >= pix_hght )
		return NULL;

	for ( i=0 ; i < FTB_COUNT ; i++ ) {
		if (ftbObjList[i].ctx_id == 0) {
			phndl = &(ftbObjList[i]);
			break;
		}
	}
	if (phndl) {
		phndl->tbuf_len = (uint32_t)width * hght;
		phndl->tbuf = (char *)malloc(phndl->tbuf_len);
		if (phndl->tbuf) {
			ftbgfx_tb_clear(phndl);
			phndl->wmark = FTB_CTX_WMARK;
			phndl->ctx_id = ++ftbIDGen;
			phndl->tl_xpos = xpos;
			phndl->tl_ypos = ypos;
			phndl->tb_width = width;
			phndl->tb_height = hght;
			phndl->bk_trans = bkgnd_trans;
			phndl->txt_scale = 1; // scaling disabled for now
			phndl->word_wrap = wwrap;
			phndl->currx = 0;
			phndl->curry = 0;
			phndl->do_render = 1; // assume its visible when created
			phndl->fb_width = (uint8_t)g_llGfxDrvr->get_DispWidth();
			phndl->fb_pages = (uint8_t)g_llGfxDrvr->get_DispPageHeight();
			phndl->fb_len = (uint32_t)g_llGfxDrvr->get_FBSize();
			phndl->frame_buffer = g_llGfxDrvr->get_drvrFrameBuffer();
		} else {
			phndl->tbuf_len = 0;
			phndl = NULL; // could not allocate mem for text buffer, failing.
		}		
	}
    return (void *)phndl;
}

ftbgfx_p validate_vptr(void * vptr) {
	ftbgfx_p phndl = (ftbgfx_p)vptr;
	if (phndl && phndl->wmark == FTB_CTX_WMARK)
		return phndl;
	else
		return NULL;
}

int ftbgfx_enable(void * ftbhnd) {
	ftbgfx_p phndl = validate_vptr(ftbhnd);
	if (phndl) {
		phndl->do_render = 1;
	}
    return (phndl) ? 0 : 1;
}

int ftbgfx_disable(void * ftbhnd) {
	ftbgfx_p phndl = validate_vptr(ftbhnd);
	if (phndl) {
		phndl->do_render = 0;
	}
    return (phndl) ? 0 : 1;
}

int ftbgfx_delete(void * ftbhnd) {
    ftbgfx_p phndl = validate_vptr(ftbhnd);
    if (phndl) {
        phndl->ctx_id = 0;
		phndl->do_render = 0;
		if (phndl->tbuf) {
			free(phndl->tbuf);
			phndl->tbuf = NULL;
		}
		phndl->tbuf_len = 0;
    }
    return (phndl) ? 0 : 1;
}

int ftbgfx_clear(void * ftbhnd) {
    ftbgfx_p phndl = validate_vptr(ftbhnd);
    if (phndl) {
		ftbgfx_tb_clear(phndl);
        phndl->currx = 0;
		phndl->curry = 0;
    }
    return (phndl) ? 0 : 1;
}

int ftbgfx_move(void * ftbhnd, uint8_t xpos, uint8_t ypos) {
    int rc = 1;
	ftbgfx_p phndl = validate_vptr(ftbhnd);
    if (phndl) {
		if (xpos < ftbgfx_get_max_pix_width() && 
		    ypos < ftbgfx_get_max_pix_height()) {
            phndl->tl_xpos = xpos;
		    phndl->tl_ypos = ypos;
			rc = 0;
		}
    }
    return rc;
}

int ftbgfx_cursor(void * ftbhnd, uint8_t xpos, uint8_t ypos) {
    int rc = 1;
	ftbgfx_p phndl = validate_vptr(ftbhnd);
    if (phndl) {
		if (xpos < phndl->tb_width && 
		    ypos < phndl->tb_height) {
            phndl->currx = xpos;
		    phndl->curry = ypos;
			rc = 0;
		}
    }
    return rc;
}

int ftbgfx_home(void * ftbhnd) {
    ftbgfx_p phndl = validate_vptr(ftbhnd);
    if (phndl) {
		phndl->currx = 0;
		phndl->curry = 0;
    }
    return (phndl) ? 0 : 1;
    return 1;
}


int ftbgfx_putc(void * ftbhnd, char c) {
	int rc = -1;
    ftbgfx_p phndl = validate_vptr(ftbhnd);
    if (phndl) {
		rc = 0;
        if (c == 0x80) {
            // backspace
			if (phndl->currx) {
				if (phndl->currx < phndl->tb_width && phndl->curry < phndl->tb_height) {
					// only clear chars within the textbox bounds
					phndl->tbuf[FTBIDX(phndl->currx,phndl->curry,phndl->tb_width)] = '\0';
				}
                // cursor could be outside of box.. that is ok.
				phndl->currx --;
			}
		} else if (c == '\n' || c == '\r') {
			// newline-carriagereturn
            if (phndl->curry < phndl->tb_height) {
                phndl->currx = 0;
                phndl->curry ++;
            }
		} else if (phndl->curry < phndl->tb_height) {
			// valid non-whitespace char still within text box bounds
			if (phndl->word_wrap) {
				// can wrap
				if (phndl->currx >= phndl->tb_width) {
					phndl->currx = 0;
					phndl->curry ++;
				}
				if (phndl->currx < phndl->tb_width && phndl->curry < phndl->tb_height) {
					phndl->tbuf[FTBIDX(phndl->currx,phndl->curry,phndl->tb_width)] = c;
					// cursor may go outside of the box at which point only a cr-lf or backspace 
					// or next char w/ wrap (or clear) can get back inside.
					phndl->currx ++;
					rc = 1;
				}
			}
		}
    }
    return rc;
}

int ftbgfx_puts(void * ftbhnd, const char * s) {
	int rc = 0;
	int cc = 0;
    ftbgfx_p phndl = validate_vptr(ftbhnd);
    if (phndl) {
		while (*s) {
			rc = ftbgfx_putc(ftbhnd, *s);
			if (rc < 0) {
				break;
			}
			cc ++;
			s ++;
		}
    }
	return ( (rc < 0) ? rc : cc );
}

int ftbgfx_newline(void * ftbhnd) {
	return ftbgfx_putc(ftbhnd, '\n');
}

int ftbgfx_bkspace(void * ftbhnd) {
	return ftbgfx_putc(ftbhnd, 0x80);
}

int ftbgfx_refresh(void * ftbhnd) {
    ftbgfx_p phndl = validate_vptr(ftbhnd);
    if (phndl) {
		ftb_render(phndl,DO_WRITE_FB); 
    }
    return (phndl) ? 0 : 1;
}

int ftbgfx_refresh_all(void) {
	render_floating_txt_tables();
    return 0;
}


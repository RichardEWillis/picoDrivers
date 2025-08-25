/******************************************************************************
 * led_overlay
 * 
 * A large numeric LED display for any screen that is Sized 
 * (X, Y) pixels or larger.
 * 
 * Ver. 2.0
 * 
 * This version uses the gfxDriver Stack and so is more flexible to different
 * graphics display sizes. 
 * 
 * Features:
 *  - One to three 7-segment digits to support (0-9).
 *  - API decodes an unsigned integer to generate the n digits, range limited
 *    upon the number of digits, eg. 2 digit setup supports 0 .. 99 range.
 *  - has its own buffer which renders into the driver's framebuffer.
 *  - supports the new graphics layers
 * 
 * Limitations:
 *  - No support for A..F on the digits
 * 
 */

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <led_overlay.h>
#include <cpyutils.h>
#include <gfxDriverLowPriv.h>

// must now be initialized as the underlying gfx driver fb size is unknown 
// until discovered in the new mounted driver.
static uint8_t *   led_linebuffer = NULL;
static size_t      led_bufferlen = 0;
static int         led_fastcopy_enabled = 0;  // set to 'true' to allow fast copies in the framebuffer
static size_t      drvr_screen_width = 0;
static size_t      drvr_screen_height = 0;
static size_t      drvr_screen_pages = 0;
static size_t  FRAMEBUF_LINES_PER_PG = 0; // TBD from driver
#define DIGIT_WIDTH  17
#define DIGIT_HEIGHT 32
#define DIGIT_PGHGT  4   // must be const as the digits are already in a ROM bitmap
#define DIGIT_BUFLEN (DIGIT_WIDTH * DIGIT_PGHGT)
#define DIGIT_SPACE  2
#define MAX_DIGITS   3
#define MIN_POS      0
static size_t      MAX_X_POS = 0; // TBD from driver
static size_t      MAX_Y_POS = 0; // TBD from driver
#define MAXVAL_1DIG  9
#define MAXVAL_2DIG  99
#define MAXVAL_3DIG  999
#define MAXVAL_4DIG  9999
#define BLANK_DIGIT_IDX 0xf     /* set this value in ctx.dval[n] to blank digit n */
#define CTX_WMARK    0x4C45444F
//static size_t        DIGIT_BUFLEN = 0; // TBD from driver

// #if defined(DISP_SSD1309)
//   #include <ssd1309_driver.h>
//   extern uint8_t * framebuf; /* or whatever its called */
// //  #define GBUFFER framebuf
//   #define FRAMEBUF_P_WIDTH  128
//   #define FRAMEBUF_P_HEIGHT 64
//   #define FRAMEBUF_PAGES    8
//   #define FRAMEBUF_LINES_PER_PG (FRAMEBUF_P_HEIGHT / FRAMEBUF_PAGES)
//   #define FRAMEBUF_LEN      (FRAMEBUF_PAGES * FRAMEBUF_P_WIDTH) /* in bytes */
// #endif
// #include <led_overlay.h>
//#define DIGIT_PGHGT  (DIGIT_HEIGHT / FRAMEBUF_LINES_PER_PG)
//#define MAX_X_POS    ((FRAMEBUF_P_WIDTH-1) - (MAX_DIGITS * DIGIT_WIDTH) - ((MAX_DIGITS-2) * DIGIT_SPACE))
//#define MAX_Y_POS    ((FRAMEBUF_P_HEIGHT-1) - DIGIT_HEIGHT)
//#define DIGIT_BUFLEN (DIGIT_PGHGT * DIGIT_WIDTH)

/* digit is blanked */
static const uint8_t bmap_dig_blank[DIGIT_BUFLEN] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; 

static const uint8_t bmap_dig_0[DIGIT_BUFLEN] = {
    0xf8, 0xfc, 0xfa, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0xfa, 0xfc, 0xf8, 
    0x3f, 0x7f, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x7f, 0x3f, 
    0xfe, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xfe, 
    0x1f, 0x3f, 0x5f, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0x5f, 0x3f, 0x1f };
static const uint8_t bmap_dig_1[DIGIT_BUFLEN] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xfc, 0xf8, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x7f, 0x3f, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xfe, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x3f, 0x1f };
static const uint8_t bmap_dig_2[DIGIT_BUFLEN] = {
    0x00, 0x00, 0x02, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0xfa, 0xfc, 0xf8, 
    0x00, 0x00, 0x80, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xbf, 0x7f, 0x3f, 
    0xfe, 0xff, 0xfe, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 
    0x1f, 0x3f, 0x5f, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0x40, 0x00, 0x00 };
static const uint8_t bmap_dig_3[DIGIT_BUFLEN] = {
    0x00, 0x00, 0x02, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0xfa, 0xfc, 0xf8, 
    0x00, 0x00, 0x80, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xbf, 0x7f, 0x3f, 
    0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xfe, 0xff, 0xfe, 
    0x00, 0x00, 0x40, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0x5f, 0x3f, 0x1f };
static const uint8_t bmap_dig_4[DIGIT_BUFLEN] = {
    0xf8, 0xfc, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xfc, 0xf8, 
    0x3f, 0x7f, 0xbf, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xbf, 0x7f, 0x3f, 
    0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xfe, 0xff, 0xfe, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x3f, 0x1f };
static const uint8_t bmap_dig_5[DIGIT_BUFLEN] = {
    0xf8, 0xfc, 0xfa, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x02, 0x00, 0x00, 
    0x3f, 0x7f, 0xbf, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0x80, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xfe, 0xff, 0xfe, 
    0x00, 0x00, 0x40, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0x5f, 0x3f, 0x1f };
static const uint8_t bmap_dig_6[DIGIT_BUFLEN] = {
    0xf8, 0xfc, 0xfa, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x02, 0x00, 0x00, 
    0x3f, 0x7f, 0xbf, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0x80, 0x00, 0x00, 
    0xfe, 0xff, 0xfe, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xfe, 0xff, 0xfe, 
    0x1f, 0x3f, 0x5f, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0x5f, 0x3f, 0x1f };
static const uint8_t bmap_dig_7[DIGIT_BUFLEN] = {
    0x00, 0x00, 0x02, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0xfa, 0xfc, 0xf8, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x7f, 0x3f, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xfe, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x3f, 0x1f };
static const uint8_t bmap_dig_8[DIGIT_BUFLEN] = {
    0xf8, 0xfc, 0xfa, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0xfa, 0xfc, 0xf8, 
    0x3f, 0x7f, 0xbf, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xbf, 0x7f, 0x3f, 
    0xfe, 0xff, 0xfe, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xfe, 0xff, 0xfe, 
    0x1f, 0x3f, 0x5f, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0x5f, 0x3f, 0x1f };
static const uint8_t bmap_dig_9[DIGIT_BUFLEN] = {
    0xf8, 0xfc, 0xfa, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0xfa, 0xfc, 0xf8, 
    0x3f, 0x7f, 0xbf, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xbf, 0x7f, 0x3f, 
    0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xfe, 0xff, 0xfe, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x3f, 0x1f };

static const uint8_t * bmaps[] = {
    bmap_dig_blank, /* zero index = blank digit ! */
    bmap_dig_0,
    bmap_dig_1,
    bmap_dig_2,
    bmap_dig_3,
    bmap_dig_4,
    bmap_dig_5,
    bmap_dig_6,
    bmap_dig_7,
    bmap_dig_8,
    bmap_dig_9
};

static uint32_t g_sessionGenerator = 1;

typedef struct led_ctx_type {
    uint32_t watermark;
    uint32_t sessionid;
    uint8_t  xpos;
    uint8_t  ypos;
    uint8_t  diglen;
    uint8_t  autoupdate;
    int32_t  maxvalue;
    uint8_t  dval[MAX_DIGITS]; /* dval[0] is the "ones" digit, far right */
    uint8_t  filler;
} led_ctx_t;

// Sessions now a static array size. Right now just limit it to 1.
#ifndef MAX_LEDO_SESSIONS
  #define MAX_LEDO_SESSIONS 1
#endif
static led_ctx_t led_session[MAX_LEDO_SESSIONS] = {0};

static led_ctx_t * get_session(void) {
    led_ctx_t * sess = NULL;
    size_t i;

    if (!led_linebuffer) {
        return NULL; // not initialized yet.
    }

    for (i = 0 ; i < MAX_LEDO_SESSIONS ; i++) {
        if (led_session[i].sessionid == 0) {
            led_session[i].watermark = CTX_WMARK;
            led_session[i].sessionid = g_sessionGenerator++;
            sess = &(led_session[i]);
            break;
        }
    }

    return sess;
}

static void return_session(led_ctx_t * s) {
    if (s && s->watermark == CTX_WMARK) {
        s->watermark = 0;
        s->sessionid = 0;
    }
}

static void fill_digits(uint8_t * dval, uint8_t dcount, uint32_t val) {
    /* index [0] is the least significant digit */
    dcount = (dcount > MAX_DIGITS) ? MAX_DIGITS : dcount;
    if (dval) {
        uint32_t div  = (uint32_t)pow(10,dcount); /* 1,10,100,1000 */
        uint8_t  lblank = 1; /* flag for initial left-J blanking */
        memset(dval, BLANK_DIGIT_IDX, MAX_DIGITS); /* blank all digits to start */
        while (val >= div) {
            val -= div; /* subtract over-value until value can actually be rendered on avail digits */
        }
        while (dcount) {
            div = div / 10;
            dcount --;
            dval[dcount] = val / div;
            val = val - dval[dcount] * div;
            if (lblank && (dval[dcount] == 0)) {
                dval[dcount] = BLANK_DIGIT_IDX; /* blank it */
            } else if (lblank) {
                lblank = 0;
            }
        }
    }
}

// First called to setup geometry after the graphics driver is loaded.
// Inputs:
//  [uint8_t]  layer_prio       establish layer priority:
//                              SET_FB_LAYER_BACKGROUND is first/lowest
//                              layer and is composited first.
// Returns:
//  0 := success
//  1 := error (gfx driver is loaded? )
int led0_init(uint8_t layer_prio) {
    int rc = 1;
    if ( bsp_gfxDriverIsReady() ) {
        if ( !led_linebuffer ) {
            drvr_screen_width  = gfx_getDispWidth();
            drvr_screen_height = gfx_getDispHeight();
            drvr_screen_pages  = gfx_getDispPageHeight();
            led_bufferlen = (drvr_screen_pages * drvr_screen_width);
            led_fastcopy_enabled = ( (led_bufferlen % 4) == 0 );
            led_linebuffer = (uint8_t *)malloc(led_bufferlen);
            if (led_linebuffer) {
                gfxutil_fb_clear(led_linebuffer, led_bufferlen, led_fastcopy_enabled);
                rc = gfx_setFrameBufferLayerPrio(led_linebuffer,layer_prio, FB_NO_MASK);
                if (rc == EXIT_SUCCESS) {
                    size_t i;
                    FRAMEBUF_LINES_PER_PG = (drvr_screen_height / drvr_screen_pages);
                    MAX_X_POS = ((drvr_screen_width-1) - (MAX_DIGITS * DIGIT_WIDTH) - ((MAX_DIGITS-2) * DIGIT_SPACE));
                    MAX_Y_POS = ((drvr_screen_height-1) - DIGIT_HEIGHT);
                    //DIGIT_BUFLEN = (DIGIT_PGHGT * DIGIT_WIDTH); - just render into the 1K framebuffer, compatible with new compositor layering
                    for (i = 0 ; i < MAX_LEDO_SESSIONS ; i++) {
                        memset( led_session+i, 0, sizeof(led_ctx_t) );
                    }
                }
            }
        } 
    }
    return rc;
}

void * ledo_open(uint8_t xPos, uint8_t yPos, uint8_t digCount, int initValue, uint8_t onUpdate) {
    led_ctx_t * ctx = 0;
    
    if (!led_linebuffer) {
        return NULL; // not initialized yet.
    }

    /* test ranges */
    if ((xPos < MAX_X_POS) && (yPos < MAX_Y_POS) && 
        digCount && (digCount <= MAX_DIGITS)) {
        int32_t  value = 0;
        switch (digCount) {
        case 1:  value = MAXVAL_1DIG; break;
        case 2:  value = MAXVAL_2DIG; break;
        case 3:  
        default: value = MAXVAL_3DIG; break;
        }
        if (initValue > value) {
            initValue = value;
        }
        ctx = get_session();
        if (ctx) {
            ctx->xpos = xPos;
            ctx->ypos = yPos;
            ctx->diglen = digCount;
            ctx->autoupdate = (onUpdate)?1:0;
            ctx->maxvalue = value;
            if (initValue < 0) {
                /* setup to have initial blank LED digits (invisible) */
                memset(&(ctx->dval[0]), BLANK_DIGIT_IDX, MAX_DIGITS);
            } else {
                fill_digits(&(ctx->dval[0]), ctx->diglen, initValue);
            }
            /* if auto-refresh then render the display right now */
            if (ctx->autoupdate) {
                ledo_refresh(ctx);
            }
        }
    }
    return (void *)ctx;
}

// given a digit index into bmaps[], copy it into the framebuffer GBUFFER[]
// at the indicated pixel location, top-left corner
static int digit_render(uint8_t xpos, uint8_t ypos, uint8_t dig) {
    int rc = 1;
    if (led_linebuffer) {
        const uint8_t * bm = (dig == BLANK_DIGIT_IDX) ? bmaps[0] : bmaps[dig+1];
        rc = gfxutil_blit(bm, DIGIT_WIDTH, DIGIT_PGHGT, true, xpos, ypos, 
            led_linebuffer, drvr_screen_width, drvr_screen_pages);
    }
    return rc;
}

// TO-DO : probably will remove this function and directly call digit_render()
//
// given a digit index (0-9,BLANK_DIGIT_IDX) update it in framebuffer GBUFFER[]
// at the indicated pixel location, top-left corner
// Note: this will erase previous bitmap. Use this method to updated the digit.
static int digit_write(uint8_t xpos, uint8_t ypos, uint8_t dig) {
    // no longer needed? the new blit function is set to OVERWRITE 
    // so any umderlying graphics are deleted which would be the 
    // previously rendered digit...
    //digit_render(xpos,ypos,BLANK_DIGIT_IDX);
    //if (dig < BLANK_DIGIT_IDX)
    //    digit_render(xpos,ypos,dig);
    return digit_render(xpos,ypos,dig);
}


void ledo_close( void ** ctx) {
    if (ctx) {
        return_session(*ctx);
        *ctx = 0; // updates arg0 by voiding it
    }
}

int ledo_update(void * _ctx, uint32_t val) {
    int rc = 1;
    if (_ctx) {
        led_ctx_t * ctx = (led_ctx_t *)_ctx;
        if (ctx->watermark == CTX_WMARK) {
            fill_digits(ctx->dval, ctx->diglen, val);
            if (ctx->autoupdate) {
                ledo_refresh(ctx);
            }
            rc = 0; // OK
        }
    }
    return rc;
}

int ledo_refresh(void * _ctx) {
    int rc = 1;
    if (_ctx) {
        led_ctx_t * ctx = (led_ctx_t *)_ctx;
        if (ctx->watermark == CTX_WMARK) {
            // eg. if 3 digits, render dig[2],[1],[0] from left to right
            uint8_t digidx = ctx->diglen;
            uint8_t xposn = ctx->xpos;
            while (digidx) {
                digidx --; // adjust to be an index and run it down to 0
                // updates local frame buffer for each digit
                digit_write(xposn, ctx->ypos, ctx->dval[digidx]);
                xposn += (DIGIT_WIDTH + DIGIT_SPACE);
            }
            // call the underlying compositor to merge layers and 
            // update display
            rc = gfx_displayRefresh();
        }
    }
    return rc;
}

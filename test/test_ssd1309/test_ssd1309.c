// Develop and Test SSD1309 Driver 
// Ver 2.0 - Using new Graphic Driver stack

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "pico/rand.h"
// Driver stack - we are both the low level BSP initialization 
// as well as the high level user. Normally user only includes
// the gfxDriverLow.h header.
#include <gfxDriverLowPriv.h>
// board.h now contains and defines the hardware resources

static void sleep_s(uint32_t s) {
    sleep_ms(s * 1000ull);
}

// Code fault - exception trap
static void trap_error(void) { 
    while (1) {
        sleep_ms(10);
    }
}

// fast-zero the framebuffer.
static void fb_clear(void) {
    uint32_t   i = 0;
    uint32_t * fb32 = (uint32_t *)gfx_getFrameBuffer();
    uint32_t   fb32len = gfx_getFBSize() / sizeof(uint32_t);
    for ( i = 0 ; i < fb32len ; i++ )
        fb32[i] = 0;
}

// fill the given frame buffer with static noise (test)
// Fast mode, use 32-bit random data to fill 4 octets at a time.
static void fb_noise(uint8_t * fb, uint fblen) {
    uint32_t   i = 0;
    uint32_t * fb32 = (uint32_t *)fb;
    uint32_t   fb32len = fblen / sizeof(uint32_t);
    for ( i = 0 ; i < fb32len ; i++ )
        fb32[i] = get_rand_32();
}

// line test. Draw a line in each bit-lane from col0 to col<n>
// starting at page0. At Page<m>,col<n> set the next bit high
// (d0, d1, d2 ... d7) at which point the entire display has
// enabled pixels. This writes into the framebuffer.
// _init() will zero the framebuffer and reset the pixel
// coords to (0,0) ==> PAGE0,Col0,Bit0.

int8_t      lt_bitpos = 0;
uint8_t     lt_mask = 0x01;
uint32_t    lt_idx = 0;
uint32_t    lt_framebufLen = 0;
uint8_t *   lt_framebuf = NULL;

static void linetest_init(void) {
    gfx_clearDisplay();
    fb_clear();
    lt_mask = 0x01;
    lt_bitpos = 0;
    lt_idx = 0;
    lt_framebuf = gfx_getFrameBuffer();
    lt_framebufLen = gfx_getFBSize();
}

// return 0 until the line has reached the end,
// at which point activate the last pixel and 
// return one. subsequent calls to this fcn will
// do nothing (return 1) until linetest_init()
// is called to reset it.
static int linetest_step(void) {
    if (lt_bitpos < 8) {
        lt_framebuf[lt_idx ++] = lt_mask;
        if (lt_idx >= lt_framebufLen) {
            lt_idx = 0;
            lt_bitpos ++;
            if (lt_bitpos < 8)
                lt_mask |= (1 << lt_bitpos);
        }
    }
    return (lt_bitpos >= 8);
}

#define DO_TEST_1       1  /* static 'snow'                     */
#define DO_TEST_2       0  /* simple|primative line test (long) */
#define DO_TEST_3       1  /* fixed text {0..9}{a..z}{A..Z}     */
#define DO_TEST_4       1  /* fixed text non alphanumeric (127+)*/
#define DO_TEST_5       1  /* floating text boxes - static      */
#define DO_TEST_6       1  /* floating text boxes - moving      */
#define DO_TEST_7       1  /* line graphics - lines             */
#define DO_TEST_8       1  /* line graphics - bar graph         */
#define DO_TEST_9       1  /* floating text box over bkgnd gfx  */

#define TEST_SLEEP_SHRT 1
#define TEST_SLEEP_MED  3
#define TEST_SLEEP_LONG 5

//#define TEST_SLEEP_1  1
#define TEST_MSWAIT_1   20
#define TEST_ITER_1     100
#define TEST_SLEEP_2    1
#define TEST_SLEEP_3    1
#define TEST_SLEEP_4    1
#define TEST_SLEEP_5    1
#define TEST_SLEEP_6    1
#define TEST_MSWAIT_6   20
#define TEST_ITER_6     100
#define TEST_SLEEP_7    1
#define TEST_MSWAIT_8   40
#define TEST_SLEEP_8    5
#define TEST_SLEEP_9    3
#define TEST_END_SLEEP  10

#if defined(DO_TEST_3) || defined(DO_TEST_4) || defined(DO_TEST_5)
  #include <textgfx.h>
  #define INIT_TEXT_LAYER SET_FB_LAYER_FOREGROUND
#endif
#if defined(DO_TEST_7) || defined(DO_TEST_8)
  #include <textgfx.h>
  #include <linegfx.h>
  #define INIT_GFX_LAYER SET_FB_LAYER_BACKGROUND
#endif

// Common Subroutines

void test_moving_textbox(void) {
    uint32_t nc = 0; // counts without bounds.
    uint32_t i = nc % 12; // {0..11} bounded
    uint32_t j = 0;
    uint8_t  xp = 0;
    uint8_t  yp = 0;
    uint8_t  mphase = 0; // 0 := diag x++,y++, 1 := lin x++, 2 := diag x--,y--, 3 := lin x--
    void * ft2 = ftbgfx_new(xp, yp, 6, 2, FTB_BKGRND_OPAQUE, FTB_TEXT_WRAP, FTB_SCALE_1);
    const char tbc[] = "0123456789AB";

    if (!ft2) {
        printf("Error [test_moving_textbox] ftbgfx_new()\n");   
        trap_error();
    }
    ftbgfx_enable(ft2);
    do {
        i = nc % 12; // text start index
        ftbgfx_clear(ft2); // clear out old text
        //ftbgfx_refresh(ft2); // write clear txt buf to framebuffer to prevent "smear effect" when moving the txt box
        ftbgfx_move(ft2,xp,yp);
        for (j = 0 ; j < 12 ; j++) {
            i = (i+j) % 12;
            ftbgfx_putc(ft2, tbc[i]);
        }
        ftbgfx_refresh(ft2); // display new box
        if ((nc % 12) == 11) {
            mphase ++;
            if (mphase >= 4) {
                mphase = 0;
            }
        }
        // move the box around on a diamond path
        switch(mphase) {
        case 0:
            xp ++; yp ++;
            break;
        case 1:
            xp ++;
            break;
        case 2:
            if (xp) xp --;
            if (yp) yp --;
            break;
        case 3:
        default:
            if (xp) xp --;
        }
        nc ++;
        sleep_ms(TEST_MSWAIT_6);
    } while (nc < TEST_ITER_6);
    sleep_s(TEST_SLEEP_6);
    ftbgfx_disable(ft2);
    sleep_s(TEST_SLEEP_6);
    ftbgfx_delete(ft2);
}

// =================== MAIN TEST LOOP =========================================

int main() {
    const char * pstrn = 0;
    uint8_t * gfb = 0; /* low level graphics framebuffer, owned by the driver itself */
    uint gfblen = 0;
    uint i = 0;

    // Enable UART so we can print
    stdio_init_all();
    printf("testing ssd1309 driver code\n");
    bsp_ConfigureGfxDriver(); /* connect the correct driver, defined by GFX_DRIVER_LL_STACK [string] */
    pstrn = g_llGfxDrvrPriv->driverName();
    if (pstrn) {
        printf("(confirm) loaded driver is: %s\n", pstrn);
    }
    gfb = gfx_getFrameBuffer();
    gfblen = gfx_getFBSize();
    if (gfb) {
        printf("driver framebuffer obtained, size: %d bytes\n", (int)gfblen);
    }
    bsp_StartGfxDriver();

    // clear display directly after turning display on
    gfx_displayOn();
    sleep_s(1);
    gfx_clearDisplay();
    sleep_s(1);

// basic frame buffer (static snow) API: ssd1309_driver.h
#if (DO_TEST_1 == 1)    
    // Test # 1 "static snow"
    for ( i = 1 ; i < TEST_ITER_1 ; i++ ) {
        fb_noise(gfb, gfblen);
        gfx_refreshDisplay(gfb); // cannot use: gfx_displayRefresh(); the layer compositor is not setup yet.
        sleep_ms(TEST_MSWAIT_1);
    }
#endif

// basic frame buffer (line-based dead pixel test) API: ssd1309_driver.h
#if (DO_TEST_2 == 1)
    // Test # 2 "line test"
    fb_clear();
    linetest_init();
    while ( linetest_step() == 0 )
    {
        gfx_refreshDisplay(gfb); // cannot use: gfx_displayRefresh(); the layer compositor is not setup yet.
        sleep_ms(1);
    }
    gfx_refreshDisplay(gfb); // cannot use: gfx_displayRefresh(); the layer compositor is not setup yet.
    sleep_s(TEST_SLEEP_2);
#endif

    fb_clear();
    gfx_refreshDisplay(gfb); // cannot use: gfx_displayRefresh(); the layer compositor is not setup yet.

    // Setup layers for all testing
#ifdef INIT_TEXT_LAYER
    text_init(INIT_TEXT_LAYER);
#endif
#ifdef INIT_GFX_LAYER
    lgfx_init(INIT_GFX_LAYER);
#endif

    // 20 x 8 text window
#if (DO_TEST_3 == 1)
    textgfx_init(REFRESH_ON_DEMAND, SET_TEXTWRAP_ON);

    // report TB geometry
    printf("Text buffer opened\n");
    printf("Testing word wrap. Screen updated on command.\n");
    printf("  width:    %d\n", textgfx_get_width());
    printf("  height:   %d\n", textgfx_get_height());
    printf("  txt mode: %s\n", (textgfx_get_refresh_mode()) ? "on demand" : "on txt chg" );
    printf("  txt wrap: %s\n", (textgfx_get_text_wrap_mode()) ? "on" : "off" );
    printf("  cursor: (%d,%d)\n", textgfx_get_cursor_posn_x(), textgfx_get_cursor_posn_y());
    printf("\n");

    // test string, TL corner
    if ( textgfx_puts("Hello World!\n") < 0 ) {
        printf("Error [TEST3] stb_puts(Hello World)\n"); 
        trap_error();
    }
    if ( textgfx_puts("012345678901234567890ABCDEFGHIJKLMNOPQRSTU") < 0 ) {
        printf("Error [TEST3] stb_puts(012345678901234567890ABCDEFGHIJKLMNOPQRSTU)\n"); 
        trap_error();
    }
    if ( textgfx_puts("fourth line") < 0 ) {
        printf("Error [TEST3] stb_puts(fourth line)\n"); 
        trap_error();
    }
    if ( textgfx_refresh() ) {
        printf("Error [TEST3] stb_refresh()\n"); 
        trap_error();
    }
    sleep_s(TEST_SLEEP_3);
    textgfx_set_refresh_mode(REFRESH_ON_TEXT_CHANGE);
    printf("Changing Screen updates to activate on a text update.\n");
    printf("Screen should clear...\n");
    if ( textgfx_clear() ) {
        printf("Error [TEST3] textgfx_clear()\n"); 
        trap_error();
    }
    sleep_s(TEST_SLEEP_MED);
    if ( textgfx_puts("This should appear\n") < 0 ) {
        printf("Error [TEST3] stb_puts(This should appear)\n"); 
        trap_error();
    }
    sleep_s(TEST_SLEEP_MED);
    if ( textgfx_clear() ) {
        printf("Error [TEST3] textgfx_clear()\n"); 
        trap_error();
    }
#endif

#if (DO_TEST_4 == 1)
    // Characters beyond 127 (128 .. 255)
    // 64 chars per page
    {
        char c = 0;
        textgfx_init(REFRESH_ON_DEMAND, SET_TEXTWRAP_ON);
        textgfx_set_refresh_mode(REFRESH_ON_DEMAND); // in case already initialized, prev. call would be ignored.
        textgfx_clear();
        textgfx_cursor(0,0);
        textgfx_puts("0x80 - 0xAF\n");
        for (c = 0x80 ; c < 0xB0 ; c++)
            textgfx_putc(c);
        textgfx_refresh();
        sleep_s(TEST_SLEEP_4);

        textgfx_clear();
        textgfx_cursor(0,0);
        textgfx_puts("0xB0 - 0xFF\n");
        for (c = 0xB0 ; c < 0xFF ; c++)
            textgfx_putc(c);
        textgfx_refresh();
        textgfx_clear(); // next write to display should not have any fixed text rendered.
        sleep_s(TEST_SLEEP_4);
    }
#endif

#if (DO_TEST_5 == 1)
    { // setup for using floating text boxes (static locations)
        int rc;
        void * ftbhndl = NULL;
        
        ftbgfx_init();

        // 6x2 box @ (0,0) in the framebuffer top-left corner
        ftbhndl = ftbgfx_new(0, 0, 6, 2, FTB_BKGRND_OPAQUE, FTB_TEXT_WRAP, FTB_SCALE_1);
        if (!ftbhndl) {
            printf("Error [TEST5] ftbgfx_new()\n");   
            trap_error();
        }
        if (ftbgfx_enable(ftbhndl)) {
            printf("Error [TEST5] ftbgfx_enable()\n");   
            trap_error();
        }
        if (ftbgfx_clear(ftbhndl)) {
            printf("Error [TEST5] ftbgfx_clear()\n");   
            trap_error();
        }
        rc = ftbgfx_puts(ftbhndl, "0123456789AB");
        if (rc != 12) {
            printf("Error [TEST5] ftbgfx_puts(), 12 chars w/ wrap on\n");   
            trap_error();
        }
        if ( ftbgfx_refresh(ftbhndl) ) {
            printf("Error [TEST5] ftbgfx_refresh() 1/2\n"); 
            trap_error();
        }
        sleep_s(TEST_SLEEP_5);
        if (ftbgfx_move(ftbhndl,4,4)) {
            printf("Error [TEST5] ftbgfx_move(4,4)\n"); 
            trap_error();
        }
        if ( ftbgfx_refresh(ftbhndl) ) {
            printf("Error [TEST5] ftbgfx_refresh() 2/2\n"); 
            trap_error();
        }
        ftbgfx_disable(ftbhndl);
        textgfx_clear();
        fb_clear();      // delete all data in the driver's framebuffer
        sleep_s(TEST_SLEEP_5);
    }
#endif

#if (DO_TEST_6 == 1)
    {
        test_moving_textbox();
    }
#endif

// line graphing primatives, API: ssd1309_linegfx.h
#if (DO_TEST_7 == 1)
    {
        uint8_t xp, yp;
        // Draw a box and some lines to points on the line
        #define BOX_X1  2
        #define BOX_Y1  2
        #define BOX_X2  125
        #define BOX_Y2  61
        #define MP_X    40
        #define MP_Y    20
        #define MP_STEP 4

        lgfx_clear();
        textgfx_clear();
        if ( lgfx_box(BOX_X1,BOX_Y1,BOX_X2,BOX_Y2,COLOUR_BLK) ) {
            printf("Error [TEST7] lgfx_box()\n");   
            trap_error();
        }
        textgfx_refresh();
#if 0        
        if ( lgfx_line(BOX_X1,BOX_Y1,BOX_X2,BOX_Y2,COLOUR_BLK)) {
            printf("Error [TEST7] lgfx_box()\n");   
            trap_error();
        }
        if ( lgfx_line(BOX_X2,BOX_Y1,BOX_X1,BOX_Y2,COLOUR_BLK)) {
            printf("Error [TEST7] lgfx_box()\n");   
            trap_error();
        }
#endif
        // draw a line base moire pattern to test lines varying vector angles.
        for (xp = BOX_X1 ; xp < BOX_X2 ; xp += MP_STEP) {
            lgfx_line(MP_X, MP_Y, xp, BOX_Y1, COLOUR_BLK);
            textgfx_refresh();
        }
        for (yp = BOX_Y1 ; yp < BOX_Y2 ; yp += MP_STEP) {
            lgfx_line(MP_X, MP_Y, BOX_X2, yp, COLOUR_BLK);
            textgfx_refresh();
        }
        for (xp = BOX_X2 ; xp > BOX_X1 ; xp -= MP_STEP) {
            lgfx_line(MP_X, MP_Y, xp, BOX_Y2, COLOUR_BLK);
            textgfx_refresh();
        }
        for (yp = BOX_Y2 ; yp > BOX_Y1 ; yp -= MP_STEP) {
            lgfx_line(MP_X, MP_Y, BOX_X1, yp, COLOUR_BLK);
            textgfx_refresh();
        }
        sleep_s(TEST_SLEEP_7);
        // overlay text on graphics - this assumes gpfx in backgound
        // text should be readable (to do, fix with a text mask... txt framebufer x2 in size)
        textgfx_set_refresh_mode(REFRESH_ON_TEXT_CHANGE);
        textgfx_cursor(5,5);
        textgfx_puts("TEXT ON GFX\n");
        sleep_s(5);
        textgfx_clear();
        lgfx_clear(); // clear the graphics buffer
        textgfx_refresh(); // update the screen
    }
#endif

// bar graph, API: ssd1309_linegfx.h
#if (DO_TEST_8 == 1)
    {
        uint8_t i;
        // box around the graph
        #define GBOX_X1     0
        #define GBOX_Y1     0
        #define GBOX_X2     51
        #define GBOX_Y2     6
        // graph is 5 pixels high, inside the above box, graph len = 50
        #define GRPH_X      1
        #define GRPH_Y      1
        #define GRPH_H      5
        #define GRPH_LEN    50

        sleep_s(1);
        textgfx_clear();
        lgfx_clear(); 
        if ( lgfx_box(GBOX_X1,GBOX_Y1,GBOX_X2,GBOX_Y2,COLOUR_BLK) ) {
            printf("Error [TEST8] lgfx_box()\n");   
            trap_error();
        }
        for (i=0 ; i <= 50 ; i++) {
            if ( lgfx_bgraph(GRPH_X,GRPH_Y,GRPH_H,GRPH_LEN,i,COLOUR_BLK) ) {
                printf("Error [TEST8] lgfx_box()\n");   
                trap_error();
            }
            textgfx_refresh();
            sleep_ms(TEST_MSWAIT_8);
        }
        sleep_s(2);
    }
#endif

#ifdef DO_TEST_9
    {
        #define FBOX_X1  0
        #define FBOX_Y1  0
        #define FBOX_X2  32
        #define FBOX_Y2  16

        lgfx_clear();
        textgfx_clear();
        if ( lgfx_filled_box(FBOX_X1,FBOX_Y1,FBOX_X2,FBOX_Y2,COLOUR_BLK) ) {
            printf("Error [TEST9] lgfx_filled_box()\n");   
            trap_error();
        }
        textgfx_refresh();
        sleep_s(1);
        // do the same moving box test - moved to a subroutine.
        test_moving_textbox();
        sleep_s(2);
    }
#endif

#ifdef LED_OVERLAY
    #define LED_XPOS 3
    #define LED_YPOS 3
    #define LED_DIGCOUNT 3
    #define LED_INITVAL 0
    #define LED_REFRESH_ON_UPDATE 1
    {
        fb_clear();
        ssd1309drv_disp_frame(&(framebuf[0]), FRAMEBUFFER_SIZE); // clear display

        void * ledctx = ledo_open(LED_XPOS, LED_YPOS, LED_DIGCOUNT, LED_INITVAL, LED_REFRESH_ON_UPDATE);
        if ( ! ledctx ) {
            printf("Error [LED_OVERLAY] ledo_open()\n");   
            trap_error();
        }
        ssd1309drv_disp_frame(&(framebuf[0]), FRAMEBUFFER_SIZE); // clear display
        sleep_s(1);
        if ( ledo_update(ledctx, 123) ) {
            printf("Error [LED_OVERLAY] ledo_update(123)\n");   
            trap_error();
        }
        ssd1309drv_disp_frame(&(framebuf[0]), FRAMEBUFFER_SIZE); // clear display
        sleep_s(1);
        if ( ledo_update(ledctx, 42) ) {
            printf("Error [LED_OVERLAY] ledo_update(42)\n");   
            trap_error();
        }
        ssd1309drv_disp_frame(&(framebuf[0]), FRAMEBUFFER_SIZE); // clear display
        sleep_s(1);
        if ( ledo_update(ledctx, 9) ) {
            printf("Error [LED_OVERLAY] ledo_update(9)\n");   
            trap_error();
        }
        ssd1309drv_disp_frame(&(framebuf[0]), FRAMEBUFFER_SIZE); // clear display
        sleep_s(1);
        if ( ledo_update(ledctx, 0) ) {
            printf("Error [LED_OVERLAY] ledo_update(0)\n");   
            trap_error();
        }
        ssd1309drv_disp_frame(&(framebuf[0]), FRAMEBUFFER_SIZE); // clear display
        sleep_s(1);
        if ( ledo_refresh(ledctx) ) {
            printf("Error [LED_OVERLAY] ledo_refresh()\n");   
            trap_error();
        }
        ssd1309drv_disp_frame(&(framebuf[0]), FRAMEBUFFER_SIZE); // clear display
        sleep_s(1);
        ledo_close(&ledctx);
        if ( ledctx ) {
            printf("Error [LED_OVERLAY] ledo_close()\n");   
            trap_error();
        }
    }
#endif

//    sleep_s(TEST_END_SLEEP);
//    fb_clear();
//    ssd1309drv_disp_frame(&(framebuf[0]), FRAMEBUFFER_SIZE); // clear display

    while (1) {
        sleep_ms(10);
    }
    // never reaches here
    return 0;
}

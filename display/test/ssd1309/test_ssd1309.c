// Develop and Test SSD1309 Driver 

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "pico/rand.h"
#include "ssd1309_driver.h"
#include "ssd1309_textbuffer.h"
#include "ssd1309_ftb.h"
#include "ssd1309_linegfx.h"
#ifdef LED_OVERLAY
  #include "led_overlay.h"
#endif

//#if !defined(spi_default) || !defined(PICO_DEFAULT_SPI_SCK_PIN) || !defined(PICO_DEFAULT_SPI_TX_PIN) || !defined(PICO_DEFAULT_SPI_RX_PIN) || !defined(PICO_DEFAULT_SPI_CSN_PIN)
//#warning "spi/spi_master example requires a board with SPI pins"
//#endif

#define JBC_DISP_GPIO_DC    20
#define JBC_DISP_GPIO_RES   28

static void sleep_s(uint32_t s) {
    sleep_ms(s * 1000ull);
}

// Local Framebuffer
uint8_t framebuf[FRAMEBUFFER_SIZE] = {0};

// fast-zero the framebuffer.
static void fb_clear(void) {
    uint32_t   i = 0;
    uint32_t * fb32 = (uint32_t *)framebuf;
    uint32_t   fb32len = FRAMEBUFFER_SIZE / sizeof(uint32_t);
    for ( i = 0 ; i < fb32len ; i++ )
        fb32[i] = 0;
}

// fill the fame buffer with static noise (test)
// Fast mode, use 32-bit random data to fill 4 octets at a time.
static void fb_noise(void) {
    uint32_t   i = 0;
    uint32_t * fb32 = (uint32_t *)framebuf;
    uint32_t   fb32len = FRAMEBUFFER_SIZE / sizeof(uint32_t);
    for ( i = 0 ; i < fb32len ; i++ )
        fb32[i] = get_rand_32();
}

// line test. Draw a line in each bit-lane from col0 to col127
// starting at page0. At Page7,col128 set the next bit high
// (d0, d1, d2 ... d7) at which point the entire display has
// enabled pixels. This writes into the framebuffer.
// _init() will zero the framebuffer and reset the pixel
// coords to (0,0) ==> PAGE0,Col0,Bit0.

uint8_t lt_mask = 0x01;
uint8_t lt_bitpos = 0;
uint32_t lt_idx = 0;

static void linetest_init(void) {
    fb_clear();
    lt_mask = 0x01;
    lt_bitpos = 0;
    lt_idx = 0;
}

// return 0 until the line has reached the end,
// at which point activate the last pixel and 
// return one. subsequent calls to this fcn will
// do nothing (return 1) until linetest_init()
// is called to reset it.
static int linetest_step(void) {
    if (lt_bitpos < 8) {
        framebuf[lt_idx ++] = lt_mask;
        if (lt_idx >= FRAMEBUFFER_SIZE) {
            lt_idx = 0;
            lt_bitpos ++;
            if (lt_bitpos < 8)
                lt_mask |= (1 << lt_bitpos);
        }
    }
    return (lt_bitpos >= 8);
}

// Code fault - exception trap
static void trap_error(void) { 
    while (1) {
        sleep_ms(10);
    }
}

#define DO_TEST_1       1
#define DO_TEST_2       1
#define DO_TEST_3       1
#define DO_TEST_4       1
#define DO_TEST_5       1
#define DO_TEST_6       1
#define DO_TEST_7       1
#define DO_TEST_8       1

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
#define TEST_MSWAIT_8   20
#define TEST_SLEEP_8    5
#define TEST_END_SLEEP  10

int main() {
    uint i = 0;
    // Enable UART so we can print
    stdio_init_all();
    printf("testing ssd1309 driver code\n");
    ssd1309drv_init(JBC_DISP_GPIO_DC,JBC_DISP_GPIO_RES);
    ssd1309drv_disp_init();
    ssd1309drv_disp_on();
    sleep_s(1);
    ssd1309drv_disp_blank();
    sleep_s(1);

// basic frame buffer (static snow) API: ssd1309_driver.h
#if (DO_TEST_1 == 1)    
    // Test # 1 "static snow"
    for ( i = 1 ; i < TEST_ITER_1 ; i++ ) {
        fb_noise();
        ssd1309drv_disp_frame(&(framebuf[0]), FRAMEBUFFER_SIZE);
        sleep_ms(TEST_MSWAIT_1);
    }
#endif

// basic frame buffer (line-based dead pixel test) API: ssd1309_driver.h
#if (DO_TEST_2 == 1)
    // Test # 2 "line test"
    fb_clear();
    while ( linetest_step() == 0 )
    {
        ssd1309drv_disp_frame(&(framebuf[0]), FRAMEBUFFER_SIZE);
        sleep_ms(1);
    }
    ssd1309drv_disp_frame(&(framebuf[0]), FRAMEBUFFER_SIZE); // the last pixel
    sleep_s(TEST_SLEEP_2);
#endif

    fb_clear();
    ssd1309drv_disp_frame(&(framebuf[0]), FRAMEBUFFER_SIZE); // clear display

    // Text buffer test
    if ( stb_init(REFRESH_ON_DEMAND, SET_TEXTWRAP_ON, 0, 0, 0) ) {
        printf("Error stb_init()\n"); 
        trap_error();
    }
    
    // 20 x 8 text window
#if (DO_TEST_3 == 1)
    // test string, TL corner
    if ( stb_puts("Hello World!\n") ) {
        printf("Error [TEST3] stb_puts(Hello World)\n"); 
        trap_error();
    }
    if ( stb_puts("01234567890123456789ABCDEFGHIJKLMNOPQRST") ) {
        printf("Error [TEST3] stb_puts(01234567890123456789ABCDEFGHIJKLMNOPQRST)\n"); 
        trap_error();
    }
    if ( stb_puts("third line") ) {
        printf("Error [TEST3] stb_puts(third line)\n"); 
        trap_error();
    }
    if ( stb_refresh() ) {
        printf("Error [TEST3] stb_refresh()\n"); 
        trap_error();
    }
    sleep_s(TEST_SLEEP_3);
#endif

#if (DO_TEST_4 == 1)
    // Characters beyond 127 (128 .. 255)
    // 64 chars per page
    {
        char c = 0;
        stb_clear();
        stb_cursor(0,0);
        stb_puts("0x80 - 0xAF\n");
        for (c = 0x80 ; c < 0xB0 ; c++)
            stb_putc(c);
        stb_refresh();
        sleep_s(TEST_SLEEP_4);

        stb_clear();
        stb_cursor(0,0);
        stb_puts("0xB0 - 0xFF\n");
        for (c = 0xB0 ; c < 0xFF ; c++)
            stb_putc(c);
        stb_refresh();
    }
#endif

#if (DO_TEST_5 == 1)
    { // setup for using floating text boxes (static locations)
        sleep_s(TEST_SLEEP_4);
        int rc;
        // 6x2 box @ (0,0) in the framebuffer top-left corner
        int ft1 = ftb_new(0, 0, 6, 2, FTB_BKGRND_OPAQUE, FTB_TEXT_WRAP, FTB_SCALE_1);
        if (ft1 < 0) {
            printf("Error [TEST5] ftb_new()\n");   
            trap_error();
        }
        if (ftb_enable(ft1)) {
            printf("Error [TEST5] ftb_enable()\n");   
            trap_error();
        }
        if (ftb_clear(ft1)) {
            printf("Error [TEST5] ftb_clear()\n");   
            trap_error();
        }
        rc = ftb_puts(ft1, "0123456789AB");
        if (rc != 12) {
            printf("Error [TEST5] ftb_puts(), 12 chars w/ wrap on\n");   
            trap_error();
        }
        if ( stb_refresh() ) {
            printf("Error [TEST5] stb_refresh() 1/2\n"); 
            trap_error();
        }
        sleep_s(TEST_SLEEP_5);
        stb_clear(); /* delete main text box, for testing */
        if (ftb_move(ft1,4,4)) {
            printf("Error [TEST5] ftb_move(4,4)\n"); 
            trap_error();
        }
        if ( stb_refresh() ) {
            printf("Error [TEST5] stb_refresh() 2/2\n"); 
            trap_error();
        }
        ftb_disable(ft1);
        sleep_s(TEST_SLEEP_5);
    }
#endif

#if (DO_TEST_6 == 1)
    {
        uint32_t nc = 0; // counts without bounds.
        uint32_t i = nc % 12; // {0..11} bounded
        uint32_t j = 0;
        uint8_t  xp = 0;
        uint8_t  yp = 0;
        uint8_t  mphase = 0; // 0 := diag x++,y++, 1 := lin x++, 2 := diag x--,y--, 3 := lin x--
        int ft2 = ftb_new(xp, yp, 6, 2, FTB_BKGRND_OPAQUE, FTB_TEXT_WRAP, FTB_SCALE_1);
        const char tbc[] = "0123456789AB";

        if (ft2 < 0) {
            printf("Error [TEST6] ftb_new()\n");   
            trap_error();
        }
        ftb_enable(ft2);
        do {
            i = nc % 12; // text start index
            ftb_clear(ft2); // clear out old text
            ftb_move(ft2,xp,yp);
            for (j = 0 ; j < 12 ; j++) {
                i = (i+j) % 12;
                ftb_putc(ft2, tbc[i]);
            }
            stb_refresh(); // display new box
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
        ftb_disable(ft2);
        sleep_s(TEST_SLEEP_6);
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
        stb_clear();
        if ( lgfx_box(BOX_X1,BOX_Y1,BOX_X2,BOX_Y2,COLOUR_BLK) ) {
            printf("Error [TEST7] lgfx_box()\n");   
            trap_error();
        }
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
        stb_refresh();
        // draw a line base moire pattern to test lines varying vector angles.
        for (xp = BOX_X1 ; xp < BOX_X2 ; xp += MP_STEP) {
            lgfx_line(MP_X, MP_Y, xp, BOX_Y1, COLOUR_BLK);
            stb_refresh();
        }
        for (yp = BOX_Y1 ; yp < BOX_Y2 ; yp += MP_STEP) {
            lgfx_line(MP_X, MP_Y, BOX_X2, yp, COLOUR_BLK);
            stb_refresh();
        }
        for (xp = BOX_X2 ; xp > BOX_X1 ; xp -= MP_STEP) {
            lgfx_line(MP_X, MP_Y, xp, BOX_Y2, COLOUR_BLK);
            stb_refresh();
        }
        for (yp = BOX_Y2 ; yp > BOX_Y1 ; yp -= MP_STEP) {
            lgfx_line(MP_X, MP_Y, BOX_X1, yp, COLOUR_BLK);
            stb_refresh();
        }
        sleep_s(TEST_SLEEP_7);
        lgfx_clear(); // clear the graphics buffer
        stb_refresh(); // update the screen
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
        if ( lgfx_box(GBOX_X1,GBOX_Y1,GBOX_X2,GBOX_Y2,COLOUR_BLK) ) {
            printf("Error [TEST8] lgfx_box()\n");   
            trap_error();
        }
        for (i=0 ; i <= 50 ; i++) {
            if ( lgfx_bgraph(GRPH_X,GRPH_Y,GRPH_H,GRPH_LEN,i,COLOUR_BLK) ) {
                printf("Error [TEST8] lgfx_box()\n");   
                trap_error();
            }
            stb_refresh();
            sleep_ms(TEST_MSWAIT_8);
        }
    }
#endif

#ifdef LED_OVERLAY
    #define LED_XPOS 3
    #define LED_YPOS 3
    #define LED_DIGCOUNT 3
    #define LED_INITVAL 0
    #define LED_REFRESH_ON_UPDATE 1
    {
        void * ledctx = ledo_open(LED_XPOS, LED_YPOS, LED_DIGCOUNT, LED_INITVAL, LED_REFRESH_ON_UPDATE);
        if ( ! ledctx ) {
            printf("Error [LED_OVERLAY] ledo_open()\n");   
            trap_error();
        }
        sleep_s(1);
        if ( ledo_update(ledctx, 123) ) {
            printf("Error [LED_OVERLAY] ledo_update(123)\n");   
            trap_error();
        }
        sleep_s(1);
        if ( ledo_update(ledctx, 42) ) {
            printf("Error [LED_OVERLAY] ledo_update(42)\n");   
            trap_error();
        }
        sleep_s(1);
        if ( ledo_update(ledctx, 9) ) {
            printf("Error [LED_OVERLAY] ledo_update(9)\n");   
            trap_error();
        }
        sleep_s(1);
        if ( ledo_update(ledctx, 0) ) {
            printf("Error [LED_OVERLAY] ledo_update(0)\n");   
            trap_error();
        }
        sleep_s(1);
        if ( ledo_refresh(ledctx) ) {
            printf("Error [LED_OVERLAY] ledo_refresh()\n");   
            trap_error();
        }
        sleep_s(1);
        ledo_close(&ledctx);
        if ( ledctx ) {
            printf("Error [LED_OVERLAY] ledo_close()\n");   
            trap_error();
        }
    }
#endif

    sleep_s(TEST_END_SLEEP);
    fb_clear();
    ssd1309drv_disp_frame(&(framebuf[0]), FRAMEBUFFER_SIZE); // clear display

    while (1) {
        sleep_ms(10);
    }
    // never reaches here
    return 0;
}

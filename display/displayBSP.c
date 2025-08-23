/************************************************************************************************** 
 * Display Stack - BSP Support
 *
 * Version 1.0  (July 2025)
 *
 * CHANGELOG
 *  1.0     July 28 2025
 *          - inital revision, subject to change
 *          - prototyped on the SSD1309 Display Hardware IC on a 128x64 screen
 *
 * This code is called by the BSP initialization to select and setup a specific display driver and
 * create the driver structures.
 * 
 * probe() call will request resources from the BSP layer using reserved names that can include:
 *  DISP_DRVR_SPI_CHAN
 *  DISP_DRVR_SPI_CLK
 *  DISP_DRVR_SPI_MISO
 *  DISP_DRVR_SPI_MOSI
 *  DISP_DRVR_SPI_CS
 *  DISP_DRVR_SPI_CLK_FREQ_HZ
 *  DISP_DRVR_SPI_GPIO_DC
 *  DISP_DRVR_SPI_GPIO_RST
 * These should be defined in a header file, board.h
 * 
 *************************************************************************************************/

#include "gfxDriverLowPriv.h"
#include <cpyutils.h>
#include <stddef.h>
#include <string.h>
#include "pico/stdlib.h"

// called to trap a runtime exception
void loop_error_trap(void) { 
    while (1) {
        sleep_ms(10);
    }
}

// Statically link a driver by defining this as a string of the driver name
#ifndef GFX_DRIVER_LL_STACK
  #define GFX_DRIVER_LL_STACK "SSD1309"
#endif

#ifndef MAX_DRVR_NAMELEN
  #define MAX_DRVR_NAMELEN 16
#endif

typedef struct driverProbe_type {
    const char * name;
    const drvrProbe  probe;
} driverProbe_t;

// Low level graphics driver (private) method stack. Driver to fill-in during its probe()
gfxDriver_p_t llGfxDriverPriv = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

gfxDriver_p_p g_llGfxDrvrPriv = &llGfxDriverPriv;           // private device struct
gfxDriver_p   g_llGfxDrvr = (gfxDriver_p)&llGfxDriverPriv;   // public device struct

/* Add drivers as they are created. Each driver has to "register" a probe method to get activated. */
    /* --- SSD1309 Driver --- */
const char ssd1309Name[] = "SSD1309";
extern int ssd1309_probe(gfxDriver_p_p drvrStack);
    /* (next driver) */

driverProbe_t gfxDriverList[] = {
    {ssd1309Name, &ssd1309_probe},
    // last entry is blank, for end of list
    {NULL, NULL}
};

void bsp_ConfigureGfxDriver(void) {
    driverProbe_t * drvrList = &(gfxDriverList[0]);
    while (drvrList->name) {
        if (strcmp(GFX_DRIVER_LL_STACK,drvrList->name) == 0) {
            if ( drvrList->probe(g_llGfxDrvrPriv) ) {
                loop_error_trap();
            }
            g_llGfxDrvrPriv->Open();
            break;
        }
        drvrList ++;
    }
}

void bsp_StartGfxDriver(void) {
    g_llGfxDrvrPriv->Init();
}

int bsp_gfxDriverIsReady(void) {
    return (g_llGfxDrvrPriv && g_llGfxDrvrPriv->IsReady());
}

void bsp_StopGfxDriver(void) {
    g_llGfxDrvrPriv->Close();
}

// Public API for graphics driver

// show screen (turn on)
int gfx_displayOn(void) {
    return g_llGfxDrvr->displayOn();
}

// hide screen (turn off)
int gfx_displayOff(void) {
    return g_llGfxDrvr->displayOff();
}

// clear the screen, display framebuffer is not changed
int gfx_clearDisplay(void) {
    return g_llGfxDrvr->clearDisplay();
}

// get memory size (octet count) of display frame buffer
size_t gfx_getFBSize(void) {
    return g_llGfxDrvr->get_FBSize();
}

// get pixel width of screen
size_t gfx_getDispWidth(void) {
    return g_llGfxDrvr->get_DispWidth();
}

// get pixel height of screen
size_t gfx_getDispHeight(void) {
    return g_llGfxDrvr->get_DispHeight();
}

// get # vertical pages comprising the screen (8 pixel lines per page)
size_t gfx_getDispPageHeight(void) {
    return g_llGfxDrvr->get_DispPageHeight();
}

// get display driver name (loaded & mounted driver)
const char * gfx_getDriverName(void) {
    return g_llGfxDrvr->driverName();
}

// return a pointer to the driver's framebuffer. This is written to screen
uint8_t * gfx_getFrameBuffer(void) {
    return g_llGfxDrvr->get_drvrFrameBuffer();
}

// driver readyness, false := not ready, true := ready
int gfx_isReady(void) {
    return g_llGfxDrvr->IsReady();
}

// control screen pixel invert on/off. (doInvert = true) := invert on
int gfx_setInvertDisplay(int doInvert) {
    return g_llGfxDrvr->set_displayInvert(doInvert);
}

// control flipping screen on X axis. (doFlip = true) := flip
int gfx_setDisplayFlipX(int doFlip) {
    return g_llGfxDrvr->pset_displayFlipX(doFlip);
}

// control flipping screen on Y axis. (doFlip = true) := flip
int gfx_setDisplayFlipY(int doFlip) {
    return g_llGfxDrvr->set_displayFlipY(doFlip);
}

// control rotating screen. (doRot = true) := rotate 180 deg.
int gfx_setDisplayRot(int doRot) {
    return g_llGfxDrvr->set_displayRot(doRot);
}

// set screen contrast. 0 = normal, range: {-10,0,+10} +ve is darker
int gfx_setDisplayContrast(int con) {
    return g_llGfxDrvr->set_contrast(con);
}

// set screen brightness. 0 = normal, range: {-10,0,+10} +ve is brighter
int gfx_setDisplayBrightness(int bri) {
    return g_llGfxDrvr->set_brightness(bri);
}

// write driver's framebuffer to screen. see also gfx_refreshDisplay()
// THIS IS THE ONLY CALL THAT MERGES ALL REGISTERED FRAMEBUFFER LAYERS
int gfx_displayRefresh(void) {
    gfx_fb_compositor(); // merge all fb layers onto the gfx driver fb first.
    return g_llGfxDrvr->refreshDisplay( g_llGfxDrvr->get_drvrFrameBuffer() );
}

// write frambuffer 'fb' to screen. 
// pass driver's buffer in as 'fb' to write the internal buffer.
int gfx_refreshDisplay(const uint8_t * fb) {
    return g_llGfxDrvr->refreshDisplay(fb);
}

// Graphic Framebuffer Layer Priority Control
// under construction and subject to change how this works.
static uint8_t * fb_layers[FB_LAYER_COUNT] = {0};   // pointer to frame buffers
static uint8_t * fb_mask[FB_LAYER_COUNT] = {0};     // if a layer has a mask then it will be set here.
static int       gfx_fb_can_optimize = -1; // do a optimization check once and either set(1) or clear (0) this for future checks.

// Call this method anyways even if it does nothing right now.
int gfx_setFrameBufferLayerPrio(uint8_t * fb, uint8_t prio, uint8_t have_mask) {
    int rc = 1;
    // if ( g_llGfxDrvrPriv && !fb_layers[SET_FB_LAYER_1] ) {
    //     // driver ready, if not already done, pre-assign the
    //     // text layer which is using the graphic fb right now.
    //     fb_layers[SET_FB_LAYER_1] = gfx_getFrameBuffer();
    // }
    if (fb && (prio < FB_LAYER_COUNT)) {
        if ( !fb_layers[prio] ) {
            fb_layers[prio] = fb;
            if (have_mask) {
                size_t offset = g_llGfxDrvr->get_FBSize();
                // mask is in the second half of the buffer
                fb_mask[prio] = fb + offset;
            } else {
                fb_mask[prio] = NULL;
            }
            rc = 0;
        }
    }
    return rc;
}

// call this method to combine fb layers into the driver's buffer
// This now supports text fb on its own layer. Compositor MUST BE RUN
// to get anything into the graphics framebuffer.
int gfx_fb_compositor(void) {
    int rc = 1;
    if ( g_llGfxDrvrPriv ) {
        int i;
        size_t    fblen   = g_llGfxDrvr->get_FBSize();
        uint8_t * drvr_fb = g_llGfxDrvr->get_drvrFrameBuffer();
        // check for fast operation capability (should only need to invoke once)
        if (gfx_fb_can_optimize < 0) {
            gfx_fb_can_optimize = (fblen % sizeof(uint32_t)) ? 0 : 1;
        }
        // clear driver's fb first to re-do layer compositing into it.
        gfxutil_fb_clear(drvr_fb, fblen, gfx_fb_can_optimize);
        for (i = SET_FB_LAYER_BACKGROUND ; i < FB_LAYER_COUNT ; i++ ) {
            if (fb_layers[i]) {
                gfxutil_fb_merge(fb_layers[i], fb_mask[i], drvr_fb, fblen);
            }
        }
    }
}
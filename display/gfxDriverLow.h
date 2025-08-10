/************************************************************************************************** 
 * Graphic Driver, Lower Layer API stack (Public API)
 *
 * Version 1.0  (July 2025)
 *
 * CHANGELOG
 *  1.0     July 28 2025
 *          - inital revision, subject to change
 *          - prototyped on the SSD1309 Display Hardware IC on a 128x64 screen
 *
 *************************************************************************************************/
#ifndef GFXDRIVERLOW_H
#define GFXDRIVERLOW_H

#include <stdint.h>
#include <stddef.h>

typedef size_t (*fpget_FBSize)(void);
typedef size_t (*fpget_DispWidth)(void);
typedef size_t (*fpget_DispHeight)(void);
typedef size_t (*fpget_DispPageHeight)(void);
typedef int (*fpset_InvertDisplay)(int);
typedef int (*fpset_displayFlipX)(int);
typedef int (*fpset_displayFlipY)(int);
typedef int (*fpset_displayRot)(int);
typedef int (*fpset_DisplayContrast)(int);
typedef int (*fpset_DisplayBrightness)(int);
typedef int (*fpdisplayOn)(void);
typedef int (*fpdisplayOff)(void);
typedef int (*fprefreshDisplay)(const uint8_t *);
typedef int (*fpclearDisplay)(void);
typedef const char * (*fpget_DriverName)(void);
typedef uint8_t * (*fpget_FB)(void);
typedef int (*fpisReady)(void);

typedef struct gfxDriver_type {
    fpdisplayOn             displayOn;              // display on (show pixels, backlight on)
    fpdisplayOff            displayOff;             // display off (dark, no vis. pixels)
    fpset_InvertDisplay     set_displayInvert;      // display invert on/off
    fpset_displayFlipX      pset_displayFlipX;      // (option) flip disp about X-Axis
    fpset_displayFlipY      set_displayFlipY;       // (option) flip disp about Y-Axis
    fpset_displayRot        set_displayRot;         // (option) rotate disp 180 degrees
    fpset_DisplayContrast   set_contrast;           // (option) change screen contrast
    fpset_DisplayBrightness set_brightness;         // (option) change screen brightness
    fprefreshDisplay        refreshDisplay;         // write FB into display
    fpclearDisplay          clearDisplay;           // directly clear the diplay (framebuffer not changed)
    fpget_DriverName        driverName;             // return string ID of the loaded driver
    fpget_FBSize            get_FBSize;             // return # octets req. for a FrameBuffer for this display
    fpget_DispWidth         get_DispWidth;          // return display PIXEL width
    fpget_DispHeight        get_DispHeight;         // return display PIXEL height
    fpget_DispPageHeight    get_DispPageHeight;     // return display PAGE height
    fpget_FB                get_drvrFrameBuffer;    // return pointer to the drivers internal RAM framebuffer
    fpisReady               IsReady;                // return True if graphics driver is ready to use
} gfxDriver_t;

typedef gfxDriver_t * gfxDriver_p;

/* sole instance of the public driver, setup by BSP initialization */
extern gfxDriver_p g_llGfxDrvr;


/* --------------------------------------------------------
 * Low level API calls to cleanup access to graphics driver
 * --------------------------------------------------------
 */

extern int gfx_displayOn(void);            // show screen (turn on)
extern int gfx_displayOff(void);           // hide screen (turn off)
extern int gfx_clearDisplay(void);         // clear the screen, display framebuffer is not changed
/* getters */
extern size_t gfx_getFBSize(void);         // get memory size (octet count) of display frame buffer
extern size_t gfx_getDispWidth(void);      // get pixel width of screen
extern size_t gfx_getDispHeight(void);     // get pixel height of screen
extern size_t gfx_getDispPageHeight(void); // get # vertical pages comprising the screen (8 pixel lines per page)
extern const char * gfx_getDriverName(void); // get display driver name (loaded & mounted driver)
extern uint8_t * gfx_getFrameBuffer(void); // return a pointer to the driver's framebuffer. This is written to screen
extern int gfx_isReady(void);              // driver readyness, false := not ready, true := ready
/* setters */
extern int gfx_setInvertDisplay(int doInvert);     // control screen pixel invert on/off. (doInvert = true) := invert on
extern int gfx_setDisplayFlipX(int doFlip);        // control flipping screen on X axis. (doFlip = true) := flip
extern int gfx_setDisplayFlipY(int doFlip);        // control flipping screen on Y axis. (doFlip = true) := flip
extern int gfx_setDisplayRot(int doRot);           // control rotating screen. (doRot = true) := rotate 180 deg.
extern int gfx_setDisplayContrast(int con);        // set screen contrast. 0 = normal, range: {-10,0,+10} +ve is darker
extern int gfx_setDisplayBrightness(int bri);      // set screen brightness. 0 = normal, range: {-10,0,+10} +ve is brighter
/* Update Screen Contents */

// THIS IS THE ONLY CALL THAT MERGES ALL REGISTERED FRAMEBUFFER LAYERS
// write driver's framebuffer to screen. see also gfx_refreshDisplay()
extern int gfx_displayRefresh(void);

// This DOES NOT Merge FB layers. Only the given layer is copied to screen.
// write frambuffer 'fb' to screen. 
// This can be used to write the driver fb to screen, but it is better to 
// call gfx_displayRefresh() for this instead as all other buffers will 
// be copied to the driver fb before writing to screen. If this merging
// is NOT wanted, then use this call below.
extern int gfx_refreshDisplay(const uint8_t * fb);

// Supports multi-framebuffer APIs 
// Currently this is textbuffer (uses driver fb) and a separate line-graphics layer.
#define SET_FB_LAYER_BACKGROUND 0   /* NOT YET IMPLEMENTED (FUTURE) */
#define SET_FB_LAYER_1          1   /* (!) RESERVED FOR TEXT */
#define SET_FB_LAYER_2          2
//#define SET_FB_LAYER_3          3
//#define SET_FB_LAYER_4          4
//#define SET_FB_LAYER_5          5
//#define SET_FB_LAYER_6          5
#define SET_FB_LAYER_FOREGROUND 3
/* --- */
#define FB_LAYER_COUNT (SET_FB_LAYER_FOREGROUND+1)

#endif /* GFXDRIVERLOW_H */

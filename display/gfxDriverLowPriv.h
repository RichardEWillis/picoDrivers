/************************************************************************************************** 
 * Graphic Driver, Lower Layer API stack (Private API)
 *
 * Version 1.0  (July 2025)
 *
 * CHANGELOG
 *  1.0     July 28 2025
 *          - inital revision, subject to change
 *          - prototyped on the SSD1309 Display Hardware IC on a 128x64 screen
 *
 *************************************************************************************************/
#ifndef GFXDRIVERLOWPRIV_H
#define GFXDRIVERLOWPRIV_H

#include "gfxDriverLow.h"

typedef int (*fpopen)(void);
typedef int (*fpinit)(void);
typedef int (*fpclose)(void);

// driver opaque structure
typedef struct gfxData * gfxData_priv_p;

typedef struct gfxDriverPrivate_type {
    /* public Methods */
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
    /* PRIVATE Methods */
    fpopen                  Open;
    fpinit                  Init;
    fpclose                 Close;
    /* opaque driver specific data, methods */
    gfxData_priv_p          dinfo;
} gfxDriver_p_t;

typedef gfxDriver_p_t * gfxDriver_p_p;

extern gfxDriver_p_p g_llGfxDrvrPriv;

// Driver probe method. Each graphics driver must have one of these.
typedef int (*drvrProbe)(gfxDriver_p_p);   // empty private driver struct, driver to fill-in.


// *** BSP Operations *****************************************************************************
// (!) Call these from the BSP Setup Code.

// Setup Memory, Connect everything together, add private methods into the API stack.
extern void bsp_ConfigureGfxDriver(void);

// Start the driver operating (already opened)
extern void bsp_StartGfxDriver(void);

// Determine if display is mounted and started (ready to use)
extern int bsp_gfxDriverIsReady(void);

// Stop the driver - will probably never be called by most firmware programs.
extern void bsp_StopGfxDriver(void);


/* --------------------------------------------------------------------------------
 * Control Framebuffer Layering Priority when combining together into screen buffer
 * --------------------------------------------------------------------------------
 * Refer to gfxDriverLow.h for the layer dfinitions.
 * 
 * Layer priority sets which fb ends up being rendered first and which fb gets
 * rendered last. _BACKGROUND is ALWAYS rendered first, followed by _1, _2 ... then
 * _FOREGROUND is rendered last.
 * 
 * The Text Buffer APIs do not support layers and as such will be set to act the 
 * same as for SET_FB_LAYER_1, first to render if nothing at SET_FB_LAYER_BACKGROUND.
 * 
 * Compatible APIs will have an init() call that accepts a layer priority, but do 
 * not use SET_FB_LAYER_1 as it has been reserved for text.
 * 
 */

// Allows Graphics APIs to set a layer priority
// Returns:
//  0 := SUCCESS
//  1 := Failed (leyer already configured?)
int gfx_setFrameBufferLayerPrio(uint8_t * fb, uint8_t prio);

// Call to assemble layers into the graphics driver framebuffer.
// Note: for now, text owns the driver's framebuffer so higher
//       layers will be copied onto the text layer.
// Returns:
//  0 := SUCCESS
//  1 := Failed
int gfx_combineFrameBufferLayers(void);

#endif /* GFXDRIVERLOWPRIV_H */

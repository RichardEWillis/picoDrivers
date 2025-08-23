ABOUT: Protoyping a graphix driver stack. Features:

 - ability to set a physical display in BSP.
 - common low level "primative" stack methods across all physical displays.
 - agnostic middle and upper layers

 Stack Layers

 +-----------------------------------------+
 |                                         |
 | High Level - linedraw, text, float-text |
 |            - LED display                |
 |                                         |
 +------------------+----------------------+
                    |
                    |
 +------------------V----------------------+
 |                                         |
 | Mid Layer  (Compositor)                 |
 |  - main framebuffers, mem protection    |
 |  - fb operations, blit* etc.            |
 |                                         |
 +----------------+-------------^----------+
                  |             | (queue page flip)
                  |             |
 +------------+   |   +---------+----------+
 |    BSP     |   |   |    Tick Service    |
 | Operations |   |   |   (Scrn Refresh)   |
 +-----+------+   |   +----^----------+----+
       |          |        |          |
       |          |        |(reg)     | (write FB)
 +-----V----------V--------+----------V----+
 |                                         |
 | Stack API                               |
 |  - open, init, close, get*, set*, etc.  |
 |                                         |
 +------------------+----------------------+
                    |
                    |
 +------------------V----------------------+
 |                                         |
 | Low level driver + HW IO                |
 |                                         |
 +-----------------------------------------+
 
 Stack API and structures

*** Private methods - hide in the public structure variation.

typedef int (*fpopen)(void)
    Call is PRIVATE, only the BSP can use it.
    Enables the driver to operate. create memory if needed. Setup IO. Register Tick svc.
    Driver may also be statically linked in, so no load operation may occur.
                                    
typedef int (*fpinit)(void)
    Call is PRIVATE, only the BSP can use it.
    Allow the driver to initialize its hardware. Start of IO operations.

typedef int (*fpclose)(void)
    Call is PRIVATE, only the BSP can use it.
    Shutdown driver. de-register tick. shutdown display. de-init IO if req. free memory.

*** Public Methods - Available to higher layers

typedef const char * (*fpget_DriverName)(void)
    Return string identifier of the loaded low level graphics driver.

typedef size_t (*fpget_FBSize)(void)
    Return the required size of a framebuffer for this display, in bytes.
    note: LL driver does not create a FB it must be created by higher layer(s).

typedef size_t (*fpget_DispWidth)(void)
    Return the pixel width of the display and frambuffer

typedef size_t (*fpget_DispHeight)(void)
    Return the pixel height of the display.
    (Note, pixels may or may not be the same as Pages, depending on the display)

typedef size_t (*fpget_DispPageHeight)(void)
    Return number of pages constituting the vertical display height.
    Most displays here will concatenate 8 pixel rows into one 8-bit page octet. If so then
    the framebuffer should be arragned as an array of page octets, 
    length: (DispPageHeight * DsipWidth)

typedef void (*fpset_InvertDisplay)(int)
    Set Display to either invert pixels or not.
    Each display may have its own concept of an inverted display.
    ---
    arg1  [int]  doInvert       - True := invert the display, False := normal mode.

typedef void (*fpset_displayFlipX)(int)
    If available, flip display on its X axis when showing framebuffer on screen.
    ---
    arg1  [int]  doRotate       - True := flip, False := no flip (normal)

typedef void (*fpset_displayFlipY)(void)
    If available, flip display on its Y axis when showing framebuffer on screen.
    ---
    arg1  [int]  doRotate       - True := flip, False := no flip (normal)

typedef void (*fpset_displayRot)(void)
    If available, rotate display by 180 degrees when showing framebuffer on screen.
    ---
    arg1  [int]  doRotate       - True := rotate 180, False := normal rotation

typedef void (*fpset_DisplayContrast)(int)
    If available, adjust the screen contrast.
    ---
    arg1  [int]  contrast       - 0 := default level. c>0 := higher contrast, c<0 := lower contrast
                                  min. max. := {-10 .. +10}

typedef void (*fpsetDisplayBrightness)(int)
    If available, adjust the screen brightness.

typedef void (*fpdisplayOn)(void)
    Allow display hardware to show pixels on screen. Turn on any backlight.

typedef void (*fpdisplayOff)(void)
    Tell display hardware to hide pixels on screen or turn screen off. Turn off any backlight.

typedef int (*fprefreshDisplay)(const uint8_t *)
    Write this memory buffer (arg1) into the Display hardware.
    Typically called from the tick service.
    Framebuffer will be one from a higher layer, eg. mid. layer compositor
    (!) Also used to clear the display.


typedef struct gfxDriverPrivate_type {
    /* public Methods */
    fpdisplayOn             displayOn;              // display on (show pixels, backlight on)
    fpdisplayOff            displayOff;             // display off (dark, no vis. pixels)
    fpset_InvertDisplay     set_displayInvert;      // display invert on/off
    fpset_displayFlipX      pset_displayFlipX;      // (option) flip disp about X-Axis
    fpset_displayFlipY      set_displayFlipY;       // (option) flip disp about Y-Axis
    fpset_displayRot        set_displayRot;         // (option) rotate disp 180 degrees
    fprefreshDisplay        refreshDisplay;         // write FB into display
    fpget_DriverName        driverName;             // return string ID of the loaded driver
    fpget_FBSize            get_FBSize;             // return # octets req. for a FrameBuffer for this display
    fpget_DispWidth         get_DispWidth;          // return display PIXEL width
    fpget_DispHeight        get_DispHeight          // return display PIXEL height
    fpget_DispPageHeight    get_DispPageHeight;     // return display PAGE height
    /* PRIVATE Methods */
    fpopen                  Open;
    fpinit                  Init;
    fpclose                 Close;
    TMRSVCTYPE              timerService;   // todo - registered timer?
} gfxDriver_p_t;
typedef gfxDriver_p_t * gfxDriver_p_p;

typedef struct gfxDriver_type {
    fpdisplayOn             displayOn;              // display on (show pixels, backlight on)
    fpdisplayOff            displayOff;             // display off (dark, no vis. pixels)
    fpset_InvertDisplay     set_displayInvert;      // display invert on/off
    fpset_displayFlipX      pset_displayFlipX;      // (option) flip disp about X-Axis
    fpset_displayFlipY      set_displayFlipY;       // (option) flip disp about Y-Axis
    fpset_displayRot        set_displayRot;         // (option) rotate disp 180 degrees
    fprefreshDisplay        refreshDisplay;         // write FB into display
    fpget_DriverName        driverName;             // return string ID of the loaded driver
    fpget_FBSize            get_FBSize;             // return # octets req. for a FrameBuffer for this display
    fpget_DispWidth         get_DispWidth;          // return display PIXEL width
    fpget_DispHeight        get_DispHeight          // return display PIXEL height
    fpget_DispPageHeight    get_DispPageHeight;     // return display PAGE height
} gfxDriver_t;
typedef gfxDriver_t * gfxDriver_p;

Cast gfxDriver_p over memory of type gfxDriver_p_p to get a public API structure for higher layers.





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

#ifndef LED_OVERLAY_H
#define LED_OVERLAY_H

#include <stddef.h>
#include "pico/types.h"

/******************************************************************************
 *   ledo_init
 *   Initialize the LED display layer, MUST BE CALLED FIRST.
 *   The graphics driver must be loaded preior to this call.
 * 
 *   INPUTS
 *      layer_prio      see gfxDriverLow.h for more info on compositor layers.
 *
 *   RETURNS
 *      0 := SUCCESS (EXIT_SUCCESS)
 *      1 := Error occured (driver loaded?)
 */
int led0_init(uint8_t layer_prio);

/******************************************************************************
 *   ledo_open
 *   Open a new LED instance. All open instances now render onto a common
 *   full-sized graphics framebuffer for which the layer prio is set during
 *   the call to led0_init().
 * 
 *   At this time, only one LED session can be open at time. There is not 
 *   much of a reason to have more, but the number of sessions can be controlled
 *   at compile time using MAX_LEDO_SESSIONS.
 * 
 *   INPUTS
 *        xPos          # pixels in from left side, for top-left corner
 *                        range: {0..127}
 *        yPos          # pixels down from the top, for top-left corner
 *                        range: {0..63}
 *        digCount      number of LED digits and will affect the max value
 *                        range: {1..3}
 *        initValue     initial displayed value. if negative then will show blank.
 * [flag] onUpdate      if TRUE, the display will update when the value is
 *                        changed with ledo_update(). IF FALSE then 
 *                        ledo_refresh() must be called.
 *   RETURNS
 *      (void *)0       failed to create new display
 *      (non-null)      new context. pass to other methods.
 */
void * ledo_open(uint8_t xPos, uint8_t yPos, uint8_t digCount, 
                 int initValue, uint8_t onUpdate);

/******************************************************************************
 *   ledo_close
 *   Close a LED instance.
 *   INPUTS
 *      (&)(void *)ctx  Display context. pointer will be nulled before returning.
 *   RETURNS
 *                      none
 */
void ledo_close( void ** ctx);

/******************************************************************************
 *   ledo_update
 *   Update value for displaying on the LED digits.
 *   INPUTS
 *      (void *)ctx     open display context
 *      val             new value
 *   RETURNS
 *      0               success
 *      1               failure
 */
int ledo_update(void * ctx, uint32_t val);

/******************************************************************************
 *   ledo_refresh
 *   Refresh the display for the current setting of the LED digits.
 *   INPUTS
 *      (void *)ctx     open display context
 *   RETURNS
 *      0               success
 *      1               failure
 *   NOTES
 *   [1]    This is only required if the 'onUpdate' parameter given to 
 *          ledo_open() is FALSE.
 */
int ledo_refresh(void * ctx);

#endif /* LED_OVERLAY_H */

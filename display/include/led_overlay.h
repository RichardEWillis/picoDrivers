#ifndef LED_OVERLAY_H
#define LED_OVERLAY_H

#include <stddef.h>

/******************************************************************************
 *   ledo_open
 *   Open a new LED instance. This will be overlaid onto the existing graphics
 *   driver framebuffer, so there is no option to select one.
 *   The returns a new context which must be passed to all API commands and
 *   finally destroyed when the display is closed (if ever).
 *   The context is opaque and not accessible to the caller.
 *   INPUTS
 *      xPos            # pixels in from left side, for top-left corner
 *                        range: {0..127}
 *      yPos            # pixels down from the top, for top-left corner
 *                        range: {0..63}
 *      digCount        number of LED digits and will affect the max value
 *                        range: {1..3}
 *      initValue       initial displayed value. if negative then will show blank.
 *      [flag] onUpdate if TRUE, the display will update when the value is
 *                      changed with ledo_update(). IF FALSE then 
 *                      ledo_refresh() must be called.
 *   RETURNS
 *      (void *)0       failed to create new display
 *      (non-null)      new context. pass to other methods.
 */
void * ledo_open(uint8_t xPos, uint8_t yPos, uint8_t digCount, int initValue, uint8_t onUpdate);

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

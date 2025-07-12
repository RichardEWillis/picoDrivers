#ifndef KEYBOARD_GPIO_H
#define KEYBOARD_GPIO_H

/*****************************************************************************
 * [LIB] keyboard-gpio
 *       Small API routine to define a matrix keypad and perform scans
 *       for keypresses. Key mappings are user-defined. The keyboard
 *       scanning can be connected to an external timed callback or 
 *       the user can setup the library to be polled and any keypresses
 *       then returned from the poll() call.
 * 
 * Version 1.0 (Jan 2025)
 * Author: R. Willis
 * Hardware Supported:
 *  - (Raspberry) Pico RP-2040
 * 
 *****************************************************************************/

typedef enum KYBD_SCAN_MODE_TYPE {
    keyscan_polling = 0,    /* user polled */
    keyscan_timed,          /* externally timer driven */
    keyscan_unk             /* enum limit / unknown type - DO NOT USE */
} kybd_scan_mode_t;

#define MIN_KEYBUF_LEN 1
#define MAX_KEYBUF_LEN 8
#define MAX_KEY_ROWS   8    /* maximum number of registered keyboard rows */
#define MAX_KEY_COLS   8    /* maximum number of registered keyboard cols */

/* ------------------------------------------------------------------------- */
/* === Methods common to both type of keyboards. =========================== */
/* ------------------------------------------------------------------------- */

/* Create a new keymap.
 * The first function to call when creating a new keyboard. It defines the  
 * keyboard size (row, col) and returns a new context for the keyboard.
 * INPUTS
 *   row     number of rows, 1+    [3]
 *   col     number of colums, 1+  [3]
 *   dbtime  debounce timer period multiplier (see Note[1])
 *   kbuf    key buffer length, minimum is 1. Max is MAX_KEYBUF_LEN
 * RETURNS
 *   (ctx)  generated context, pass to other keyboard methods as an identifier.
 * NOTES:
 * [1]  kybd_scan_mode - by default this is set to POLLING when a new keyboard
 *               is created. Overide this by assigning a timer callback to the
 *               keyboard object.
 * [2]  dbtime - this is a counter for denoting a debounce period in which
 *               key detection is ignored. a keypress is only called once this
 *               debounce time expires. the initial detected key-press will
 *               start the timer. There is an independant timer for each key.
 *               Example: keys are scanned every 10 ms and dbtime is set to 4
 *               so the key is officially declared 40 ms after it is initially 
 *               pressed.
 *               State Machine, for each poll/scan step:
 *                 START    key-pressed?
 *                             no: ->START
 *                            yes: ->DBSTART,count:=0
 *                 DBSTART  count ++
 *                          count >= dbtime?
 *                             no: ->DBSTART
 *                            yes: ->KEYCHK
 *                 KEYCHK   key-still pressed?
 *                             no: ->START
 *                            yes: buffer key-press and notify user, ->START
 * [3]  row,col scanning. Internal code will scan the keyboard using the cols
 *      (GPIO pins as OUTPUTS) and rows (GPIO pins are set to inputs w/ internal
 *      pulldown). col scanning is active high so a 'high' on a row indicates
 *      a cross bar connect (pressed button).
 */
void * keyboard_map_create(int rows, int cols, int dbtime, int kbuf);

/* Assign a key definition for a new keyboard map. Call this for every key in 
 * the keyboard to assign a character to the matrix location.
 * INPUTS
 *   ctx  generated context, returned by keyboard_map_create()
 *   row  row index { 0 .. row_count-1 }
 *   col  column index { 0 .. col_count-1 }
 *   key  assigned key, ASCII or pure number. Does not have to be unique in the map!
 * RETURNS
 *   0  := ok
 *   1  := error (out-of-bounds?)
 */
int keyboard_key_assign(void * ctx, int row, int col, char key);

/* Assign GPIOs for each row and column in the keyboard. ALL ROWS,COLUMNS!
 * INPUTS
 *   ctx  generated context, returned by keyboard_map_create()
 *   row  row index { 0 .. row_count-1 }
 *   col  column index { 0 .. col_count-1 }
 *   gpio assigned gpio number (MCU specific)
 * RETURNS
 *   0  := ok
 *   1  := error (out-of-bounds?)
 * Note: For Pico based MCUs these will be the standard GPIO numbering as 
 *       supported in the SDK.
 * Note: define each row,col after creating the keyboard. the order in which
 *       keyboard_key_assign(), keyboard_assign_row_gpio() and 
 *       keyboard_assign_col_gpio() are called does not matter.
 */
int keyboard_assign_row_gpio(void * ctx, int row, int gpio);
int keyboard_assign_col_gpio(void * ctx, int col, int gpio);

/* Return the Keyboard ID for a defined keyboard.
 * INPUT
 *   ctx  known keyboard context.
 * RETURNS
 *   0  := no keyboard found. treat as an error.
 *   1+ := the keyboard ID.
 */
int keyboard_id(void * ctx);

/* return a keyboard context, from a known keyboard ID.
 * INPUT
 *  keyID  the known ID of a generated keyboard.
 * RETURNS
 *  (ctx)  found context
 *  NULL   no context found for ID.
 */
void * keyboard_search(int keyID);

/* Delete a keyboard objects context (not expected to ever be called really)
 */
void keyboard_delete(void * ctx);

/* ------------------------------------------------------------------------- */
/* === Methods used when using user-poll-based keyboard scanning =========== */
/* ------------------------------------------------------------------------- */

/* Poll (step) a created keyboard
 * INPUT
 *  ctx  keyboard context identifier
 * RETURNS
 *  (int)  key status:
 *         0  := no key action
 *         1+ := number of buffered key presses
 *        -1  := error
*/
int keyboard_poll(void * ctx);

/* Obtain the next buffered key.
 * Provide a key buffer. It will be filled with the next key, if any.
 * INPUT
 *  ctx     keyboard context identifier
 *  keybuf  buffer for the returning keypress. ignore if method returns -1.
 * RETURNS
 *  (int)  key status:
 *         0  := no remaining keys
 *         1+ := number of buffered key presses remaining
 *        -1  := error (buffer was empty?)
 */
int keyboard_getkey(void * ctx, char * keybuf);


/* ------------------------------------------------------------------------- */
/* === Methods used when using timer-based keyboard scanning =============== */
/* ------------------------------------------------------------------------- */

/* function prototype of the timer callback function, given to the timer entity.
 * INPUTS
 *  (void*) given context for the keyboard entity, created by keybd_init() and given to the timer during registration.
 * RETURNS
 *  (int)   status of the scan: 
 *            0 := no keypress, 
 *            1 := keypress, 
 *           -1 := SIGNAL: keyboard entity destroyed (timer should unregister/invalidate this callback)
 * Notes:
 * [1]  More than one keyboard entity can be created. Each entity may choose to register with the same 
 *      timer as long as the timer supports multiple contexts.
 * [2]  Timer may or may not react to the keypress (1) but it must unregister/invalidate the callback
 *      if the return code is less than 0 (-1) and not call it again.
 */
typedef int (*kybd_scan_step) (void*);

/* function prototype of the user method called when a key hit is registered.
 * This prototype is to be defined in the user program. It will be called by the keyboard entity
 * when it detects a keypress. This is all driver by the timer. It is not used for poll-based operations.
 * INPUTS
 *  (int)   Keyboard ID, 1+
 *  (void*) Keyboard context
 *  (char)  Key
 */

/* Obtain information from a created keyboard to setup the timer system.
 * The user will be responsible for registering this context and function (kybd_scan_step) with 
 * the external timer block. This lib currently has no knowledge of adjacent timer systems.
 * 
 * Note: keyboard must be initially created and defined. If the context is not known, then
 *       request it using keyboard_search, by KeyID (keyboard ID).
 */

typedef struct KEYBOARD_SCAN_INFO_TYPE {
    int            kybdID;      /* unique ID for the created keyboard */
    void *         context;     /* new keyboard context. given to timer and also to be used by the user program.*/
    kybd_scan_step callback;
} kybd_scan_info_t;

kybd_scan_info_t * keyboard_timer_info_from_id(int keyID);
kybd_scan_info_t * keyboard_timer_info_from_ctx(void * ctx);

#endif

/*
 * Internal keyboard scanning driver, requires an externally configured timed callback 
 * for "ticking" the scan, or can be manually stepped by the user application.
 * 
 *  Version 1.0 (Jan 2025)
 *  HARDWARE SUPPORT:
 *  - Pico RP2040
 * 
 */

#include "keygpio.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int key_id_generator = 1;

void * keyboard_map_create(int rows, int cols, int dbtime, int kbuf) {

    if (rows > MAX_KEY_ROWS || cols > MAX_KEY_COLS || kbuf > MAX_KEYBUF_LEN || kbuf < MIN_KEYBUF_LEN)
        return 0; // ERANGE

    keybrd_ctx_t * ctx = (keybrd_ctx_t *)malloc(sizeof(keybrd_ctx_t));
    if (ctx) {
        int i;
        memset(ctx,0,sizeof(keybrd_ctx_t));
        /* context watermark */
        strcpy(ctx->CTX_WMARK,CWMARK);
        ctx->ctx_id = key_id_generator ++;
        /* watermarking gpio row,col arrays. -1 is an invalid GPIO */
        for (i = 0 ; i < MAX_KEY_ROWS ; i++)
            ctx->gprow[i] = -1;
        for (i = 0 ; i < MAX_KEY_COLS ; i++)
            ctx->gpcol[i] = -1;
        ctx->colcount = cols;
        ctx->rowcount = rows;
        ctx->dbmax = dbtime;
        ctx->keymax = kbuf;
    }
    return CTX2VOID(ctx);
}

int keyboard_key_assign(void * _ctx, int row, int col, char key) {
    int rc = 1;
    if (_ctx) {
        keybrd_ctx_t * ctx = VOID2CTX(_ctx);
        if (strcmp(ctx->CTX_WMARK,CWMARK) == 0) {
            if (row >= 0 && row < MAX_KEY_ROWS) {
                if (col >= 0 && col < MAX_KEY_COLS) {
                    ctx->keymap[row][col] = key;
                    rc = 0;
                }
            }
        }
    }
    return rc;
}

#include "hardware/gpio.h"

#define ROW_ACTIVE 1
#define COL_OFF    0
#define COL_ACTIVE 1

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
int keyboard_assign_row_gpio(void * _ctx, int row, int gpio) {
    int rc = 1;
    if (_ctx) {
        keybrd_ctx_t * ctx = VOID2CTX(_ctx);
        if (strcmp(ctx->CTX_WMARK,CWMARK) == 0) {
            if (row >= 0 && row < MAX_KEY_ROWS) {
                ctx->gprow[row] = gpio;
                gpio_init(gpio);
                gpio_set_dir(gpio, GPIO_IN);
                gpio_pull_down(gpio);
                rc = 0;
            }
        }
    }
    return rc;
}

int keyboard_assign_col_gpio(void * _ctx, int col, int gpio) {
    int rc = 1;
    if (_ctx) {
        keybrd_ctx_t * ctx = VOID2CTX(_ctx);
        if (strcmp(ctx->CTX_WMARK,CWMARK) == 0) {
            if (col >= 0 && col < MAX_KEY_COLS) {
                ctx->gpcol[col] = gpio;
                gpio_init(gpio);
                gpio_put(gpio,COL_OFF);
                gpio_set_dir(gpio, GPIO_OUT);
                rc = 0;
            }
        }
    }
    return rc;
}

/* Return the Keyboard ID for a defined keyboard.
 * INPUT
 *   ctx  known keyboard context.
 * RETURNS
 *   0  := no keyboard found. treat as an error.
 *   1+ := the keyboard ID.
 */
int keyboard_id(void * _ctx) {
    int id = 0;
    if (_ctx) {
        keybrd_ctx_t * ctx = VOID2CTX(_ctx);
        if (strcmp(ctx->CTX_WMARK,CWMARK) == 0) {
            id = ctx->ctx_id;
        }
    }
    return id;
}

/* return a keyboard context, from a known keyboard ID.
 * INPUT
 *  keyID  the known ID of a generated keyboard.
 * RETURNS
 *  (ctx)  found context
 *  NULL   no context found for ID.
 */
void * keyboard_search(int keyID) {
    return NULL;
}

void keyboard_delete(void * ctx) {

}

/* ------------------------------------------------------------------------- */
/* === Methods used when using user-poll-based keyboard scanning =========== */
/* ------------------------------------------------------------------------- */

/*
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
 * 
 *   typedef struct keybrd_ctx_type {
 *       char CTX_WMARK[8]; -- identify this dereferenced void * as a valid keyboard context --
 *       int ctx_id;
 *       -- row and column GPIOS --
 *       int gprow[MAX_KEY_ROWS]; -- GPIO INPUT, PULL-DOWN --
 *       int gpcol[MAX_KEY_COLS]; -- GPIO OUTPUT --
 *       int rowcount; -- set row count --
 *       int colcount; -- set col count --
 *       -- key buffer --
 *       int keybuf[MAX_KEYBUF_LEN];
 *       int keycount; -- # keys in buffer --
 *       int keymax;   -- max allowable keys in buffer --
 *       -- debouncer --
 *       int dbmax;
 *       int dbcount;
 *       -- keymap - at max possible size --
 *       char keymap[MAX_KEY_ROWS][MAX_KEY_COLS];
 *   } keybrd_ctx_t;
 *
 */

/* Poll (step) a created keyboard
 * INPUT
 *  ctx  keyboard context identifier
 * RETURNS
 *  (int)  key status:
 *         0  := no key action
 *         1+ := number of buffered key presses
 *        -1  := error
*/
int keyboard_poll(void * _ctx) {
    volatile int r,c;
    volatile int keypressed = 0;
    int rc = -1;
    if (_ctx) {
        keybrd_ctx_t * ctx = VOID2CTX(_ctx);
        if (ctx->keycount < ctx->keymax) {
            /* can do a scan */
            /* col scanning */
            for ( c=0 ; c < ctx->colcount ; c++ ) {
                gpio_put(ctx->gpcol[c], COL_ACTIVE);
                /* sleep few usec - then scan rows */
                for ( r = 0 ; r < ctx->rowcount ; r++ ) {
                    if ( gpio_get(ctx->gprow[r]) ) {
                        keypressed = 1; /* prevent resetting debounce */
                        /* key deteced, check debounce */
                        if ( ++(ctx->dbcount) >= ctx->dbmax ) {
                            /* debounced, get the key */
                            ctx->keybuf[ ctx->keycount ] = ctx->keymap[r][c];
                            ctx->keycount ++;
                        }
                    }
                    if (keypressed) {
                        break; /* out of inner loop */
                    }
                }
                gpio_put(ctx->gpcol[c], COL_OFF); /* unstrobe column */
                if (keypressed) {
                    break; /* out of outer loop */
                }
                /* sleep few usec */
            }
            if ( !keypressed ) {
                ctx->dbcount = 0; // no detected key (or a bounce) reset the bounce counter
            }
        }
        rc = ctx->keycount; /* # buffered keys */
    }
    return rc;
}

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
int keyboard_getkey(void * _ctx, char * keybuf) {
    int rc = -1;
    if (_ctx && keybuf) {
        keybrd_ctx_t * ctx = VOID2CTX(_ctx);
        if ( ctx->keycount > 0 ) {
            int i;
            *keybuf = ctx->keybuf[0];
            for ( i = 1 ; i < ctx->keycount ; i++ ) {
                ctx->keybuf[i-1] = ctx->keybuf[i]; /* copy down, FIFO style */
            }
            ctx->keybuf[ --(ctx->keycount) ] = 0; /* not required but keeps the buffer clean for debugging */
            rc = ctx->keycount; /* remaining count */
        }
    }
    return rc;
}

//kybd_scan_info_t * keyboard_timer_info_from_id(int keyID);
//kybd_scan_info_t * keyboard_timer_info_from_ctx(void * ctx);


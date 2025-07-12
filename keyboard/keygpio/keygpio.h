#ifndef KEYGPIO_H
#define KEYGPIO_H

#include "keyboard-gpio.h"

//#define  8
//#define    8    /* maximum number of registered keyboard rows */
//#define    8    /* maximum number of registered keyboard cols */


// context structure
typedef struct keybrd_ctx_type {
    char CTX_WMARK[8]; /* identify this dereferenced void * as a valid keyboard context */
    int ctx_id;
    /* row and column GPIOS */
    int gprow[MAX_KEY_ROWS]; /* GPIO INPUT, PULL-DOWN */
    int gpcol[MAX_KEY_COLS]; /* GPIO OUTPUT */
    int rowcount; /* set row count */
    int colcount; /* set col count */
    /* key buffer */
    int keybuf[MAX_KEYBUF_LEN];
    int keycount; /* # keys in buffer */
    int keymax;   /* max allowable keys in buffer */
    /* debouncer */
    int dbmax;
    int dbcount;
    /* keymap - at max possible size */
    char keymap[MAX_KEY_ROWS][MAX_KEY_COLS];
} keybrd_ctx_t;

#define CWMARK "KEY_CTX"
#define CTX2VOID(x) (void*)x
#define VOID2CTX(x) (keybrd_ctx_t *)x

#endif /* KEYGPIO_H */



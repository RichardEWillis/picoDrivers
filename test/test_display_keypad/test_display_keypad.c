#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/uart.h"
#include "pico/critical_section.h"
#include "pico/sem.h"
#include <jbc_kybd_config.h>
#include <keyboard-gpio.h>
#include <gfxDriverLowPriv.h>
#include <textgfx.h>

#define KEYWAIT_TMOUT_MS 1000   /* length of time to wait for keys */
#define INIT_TEXT_LAYER SET_FB_LAYER_FOREGROUND


static void * kybd_hndl = NULL; /* keyboard object handle */
static char keybuf[KEYBUFFER_LEN+1];
static int  keycount = 0;
static int repeat_timer = 0;
static char lastkey = 0;
critical_section_t keybrd_queue;
// for turning the keypad buffer into a queue
semaphore_t * keybuf_sem = NULL;


// ***************************************************************************
// bottom-end functions for supporting keypad scanning and key buffering
// including a higher level key repeat limiter.
// ***************************************************************************

// call from a timer routine to increment timer if running.
static void keybrd_queue_tick(void) {
    critical_section_enter_blocking(&keybrd_queue);
    if (repeat_timer) {
        repeat_timer += KEYBUFFER_TICK_PD_MS;
    }
    critical_section_exit(&keybrd_queue);
}

static void keybrd_queue_push(char c) {
    critical_section_enter_blocking(&keybrd_queue);
    if (c == lastkey) {
        // manage key-repeat timing
        if (repeat_timer) {
            if (repeat_timer >= KEYBUFFER_DLY_TMOUT_MS) {
                repeat_timer = 0;
            }
        } else {
            repeat_timer = KEYBUFFER_TICK_PD_MS;
        }
    } else {
        repeat_timer = 0; // different key, kill the repeat delay if running
    }

    if (repeat_timer == 0) {
        // timeout or unique keypress, can add the key
        if (keycount < KEYBUFFER_LEN) {
            keybuf[keycount] = c;
            keycount ++;
            if (keybuf_sem) {
                sem_release(keybuf_sem);
            }
            keybuf[keycount] = '\0';
            //putchar((int)c);
        }
    }

    lastkey = c;
    critical_section_exit(&keybrd_queue);
}

// ** TASK **
static bool chk_keyboard(repeating_timer_t * rptdata) {
    // Put your timeout handler code in here
    keybrd_queue_tick();
    if (kybd_hndl) {
        int rc = keyboard_poll(kybd_hndl);
        if (rc > 0) {
            char ck;
            while (rc) {
                rc = keyboard_getkey(kybd_hndl, &ck);
                keybrd_queue_push(ck);
            }
        }
    }
    return 1; // set to 0/false to stop the r-timer
}

// ***************************************************************************
// top-end functions for supporting keypad scanning and key buffering
// this block waits on an simple character queue for one or more chars
// until a timeout occurs.
// ***************************************************************************

// pull single character from keybuf queue
// blocks, waiting for at least one character in the queue!
// returns w/ no character (0) upon timeout
static int keybrd_queue_pop_c_blocking(char * c, uint32_t tmout_ms) {
    int rc = 0;
    if (c && keybuf_sem) {
        bool success = sem_acquire_timeout_ms(keybuf_sem, tmout_ms); // block util a character can be pulled or TIMEOUT
        if (success == false) {
            return 0;
        }
    }
    critical_section_enter_blocking(&keybrd_queue);
    if (c) {
        int i;
        *c = keybuf[0];
        for ( i = 1 ; i < keycount ; i++ ) {
            keybuf[i-1] = keybuf[i]; /* copy down, FIFO style */
        }
        keybuf[--keycount] = 0; /* not required but keeps the buffer clean for debugging */
        rc = 1;
    }
    critical_section_exit(&keybrd_queue);
    return rc;
}

// pull multiple characters (blocking)
// wait for chars until 'max' received or 'tmout_ms' expires while waiting on a character
// return the number of chars actually received.
static int keybrd_queue_pop_s_blocking(char * sbuf, size_t max, uint32_t tmout_ms) {
    int ccount = 0;
    int rc = 0;
    while (ccount < max) {
        rc = keybrd_queue_pop_c_blocking(sbuf, tmout_ms);
        if (rc) {
            ccount ++;
            sbuf++;
        } else {
            break;
        }
    }
    return ccount;
}

// ***************************************************************************
// Test support functions
// ***************************************************************************

static void init_keybuf_queue(void) {
    if (!keybuf_sem) {
        keybuf_sem = (semaphore_t *)malloc(sizeof(semaphore_t));
        if (keybuf_sem) {
            // semaphore can count up to the max buffer capacity only
            // and starts empty as does the buffer.
            sem_init(keybuf_sem, 0, KEYBUFFER_LEN);
        }
    }
}

static void * init_keypad(void) {
    void * _hndl = NULL;
    if (!kybd_hndl) {
        kybd_hndl = keyboard_map_create(KYBD_ROW_COUNT, KYBD_COL_COUNT, KYBD_DBTIME, KYBD_BUFLEN);
        if (kybd_hndl) {
            keyboard_assign_row_gpio(kybd_hndl, 0, KYBD_ROW0);
            keyboard_assign_row_gpio(kybd_hndl, 1, KYBD_ROW1);
            keyboard_assign_row_gpio(kybd_hndl, 2, KYBD_ROW2);
            keyboard_assign_row_gpio(kybd_hndl, 3, KYBD_ROW3);
            keyboard_assign_col_gpio(kybd_hndl, 0, KYBD_COL0);
            keyboard_assign_col_gpio(kybd_hndl, 1, KYBD_COL1);
            keyboard_assign_col_gpio(kybd_hndl, 2, KYBD_COL2);
            keyboard_assign_col_gpio(kybd_hndl, 3, KYBD_COL3);
            keyboard_key_assign(kybd_hndl, 0, 0, '1');
            keyboard_key_assign(kybd_hndl, 0, 1, '2');
            keyboard_key_assign(kybd_hndl, 0, 2, '3');
            keyboard_key_assign(kybd_hndl, 0, 3, 'A');
            keyboard_key_assign(kybd_hndl, 1, 0, '4');
            keyboard_key_assign(kybd_hndl, 1, 1, '5');
            keyboard_key_assign(kybd_hndl, 1, 2, '6');
            keyboard_key_assign(kybd_hndl, 1, 3, 'B');
            keyboard_key_assign(kybd_hndl, 2, 0, '7');
            keyboard_key_assign(kybd_hndl, 2, 1, '8');
            keyboard_key_assign(kybd_hndl, 2, 2, '9');
            keyboard_key_assign(kybd_hndl, 2, 3, 'C');
            keyboard_key_assign(kybd_hndl, 3, 0, '*');
            keyboard_key_assign(kybd_hndl, 3, 1, '0');
            keyboard_key_assign(kybd_hndl, 3, 2, '#');
            keyboard_key_assign(kybd_hndl, 3, 3, 'D');
            _hndl = kybd_hndl;
        }
    }
    return _hndl;
}

static void sleep_s(uint32_t s) {
    sleep_ms(s * 1000ull);
}

// Code fault - exception trap
static void trap_error(void) { 
    while (1) {
        sleep_ms(10);
    }
}



int main()
{
    repeating_timer_t kbtmr;
    uint32_t lc = 0;
    char keys[KEYBUFFER_LEN+1];
    int keycount = 0;
    int scrn_height, line_num;

    stdio_init_all();
    printf("Testing Keyboard scan routines\n");

    // graphics driver setup
    bsp_ConfigureGfxDriver(); /* driver defined by GFX_DRIVER_LL_STACK [string] */

    if (g_llGfxDrvrPriv) {
        const char * pstrn = g_llGfxDrvrPriv->driverName();
        if (pstrn) {
            printf("(confirm) loaded driver is: %s\n", pstrn);
        } else {
            printf("Fail, could not resolve mounted gfx driver name.\n");
            trap_error();
        }
    } else {
        printf("failed to load gfx driver\n");
        trap_error();
    }

    // start graphics driver operations w/ hardware
    bsp_StartGfxDriver();
    gfx_displayOn();
    gfx_clearDisplay();

    // turn the keypad buffer into a queue for 
    // allowing top end calls to block
    init_keybuf_queue();

    if (!init_keypad()) {
        printf("fail, init_keypad()\n");
        trap_error();
    }
    
    // Setup the display
    text_init(INIT_TEXT_LAYER);
    textgfx_init(REFRESH_ON_DEMAND, SET_TEXTWRAP_ON);
    // report TB geometry
    printf("Text buffer opened\n");
    printf("Testing word wrap. Screen updated on command.\n");
    printf("  width:    %d\n", textgfx_get_width());
    printf("  height:   %d\n", textgfx_get_height());
    printf("  txt mode: %s\n", (textgfx_get_refresh_mode()) ? "on demand" : "on txt chg" );
    printf("  txt wrap: %s\n", (textgfx_get_text_wrap_mode()) ? "on" : "off" );
    printf("  cursor: (%d,%d)\n", textgfx_get_cursor_posn_x(), textgfx_get_cursor_posn_y());
    printf("\n");
    scrn_height = textgfx_get_height();
    line_num = 0; // track the current line and clear/wrap when trying to go more than height.

    // Enable the keypad bottom-end polling operations
    add_repeating_timer_ms(KYBD_SCAN_PD_MS, chk_keyboard, NULL, &kbtmr); 

    while (true) {
        keycount = keybrd_queue_pop_s_blocking(keys, KEYBUFFER_LEN, KEYWAIT_TMOUT_MS);
        keys[keycount] = '\0';
        printf("<%u> %s\n", lc++, keys);
        // echo to display
        if (keycount) {
            if (line_num >= scrn_height) {
                textgfx_clear(); // home and clear screen
                line_num = 0;
            }
            keys[keycount++] = '\n';
            keys[keycount] = '\0';
            textgfx_puts(keys);
            textgfx_refresh();
            line_num ++;
        }
        // sleep_ms(3000); - no longer needed
    }
}

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/uart.h"
#include "pico/critical_section.h"
#include <keyboard-gpio.h>

/* JBC_PCB_BOARD_VER_3.1
 * Board configuration to support the JBC Iron Controller on a RP Pico 
 * mezzanine board.
 * 
 * Notes:
 * [1]      Makes use of the lower level Board Support Package (bsp.h)
 * 
 * PINOUT
 * 1  CONSOLE_TX (default UART)     40 VBUS
 * 2  CONSOLE_RX (default UART)     39 VSYS
 * 3  GND                           38 GND
 * 4  Keyboard R0                   37 3V3_EN
 * 5  Keyboard R1                   36 3V3_OUT
 * 6  Keyboard R2                   35 ADC_VREF
 * 7  Keyboard R3                   34 DISP_RESET
 * 8  GND                           33 GND
 * 9  Keyboard C0                   32 
 * 10 Keyboard C1                   31 ADC_TEMP
 * 11 Keyboard C2                   30 *RUN
 * 12 Keyboard C3                   29 IRON_ON_HOOK_DET
 * 13 GND                           28 GND
 * 14 AC_ZC_INPUT                   27 PSU_N16_CHRG_STATE
 * 15 HTR_CTRL_ON_L                 26 DISP_DC
 * 16 HTR_CTRL_OFF_L                25 DISP_SPI_MOSI
 * 17 PSU_P16_ON_L                  24 DISP_SPI_SCK
 * 18 GND                           23 GND
 * 19 PSU_N16_ON_L                  22 DISP_SPI_CS
 * 20 PSU_P16_CHRG_STATE            21 DISP_SPI_MISO_NC
 */
/* GP Pin Naming (not in the RPi Pico SDK ?) */
#define GP0     0
#define GP1     1
#define GP2     2
#define GP3     3
#define GP4     4
#define GP5     5
#define GP6     6
#define GP7     7
#define GP8     8
#define GP9     9
#define GP10    10
#define GP11    11
#define GP12    12
#define GP13    13
#define GP14    14
#define GP15    15
#define GP16    16
#define GP17    17
#define GP18    18
#define GP19    19
#define GP20    20
#define GP21    21
#define GP22    22
#define GP26    26
#define GP27    27
#define GP28    28

/* ** [GPIO] Keyboard ---------------------- */
#define KYBD_ROW0   GP2
#define KYBD_ROW1   GP3
#define KYBD_ROW2   GP4
#define KYBD_ROW3   GP5
#define KYBD_COL0   GP6
#define KYBD_COL1   GP7
#define KYBD_COL2   GP8
#define KYBD_COL3   GP9
//
#define KYBD_ROW_COUNT 4
#define KYBD_COL_COUNT 4
#define KYBD_DBTIME    4 /* Tscan ~ 10 msec*/
#define KYBD_BUFLEN    2
#define KYBD_SCAN_PD_MS 20

// (!) Use stdio_uart (GP0,1)
// UART defines
// By default the stdout UART is `uart0`, so we will use the second one
//#define UART_ID uart1
//#define BAUD_RATE 115200
// Use pins 4 and 5 for UART1
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
//#define UART_TX_PIN 4
//#define UART_RX_PIN 5


static void * kybd_hndl = NULL; /* keyboard object handle */

// external keyboard FIFO. Add key entries 
// using queue_push, remove keys using queue_pop
// The queue has an internal key-repeat check feature
// to prevent fast repeated key entries from occuring
#define KEYBUFFER_LEN           16
#define KEYBUFFER_TICK_PD_MS    KYBD_SCAN_PD_MS
#define KEYBUFFER_DLY_TMOUT_MS  200  /* key-repeat-delay [msec] */
// --
static char keybuf[KEYBUFFER_LEN+1];
static int  keycount = 0;
static int repeat_timer = 0;
static char lastkey = 0;
critical_section_t keybrd_queue;
// --
// call from a timer routine to increment timer if running.
static void keybrd_queue_tick(void) {
    critical_section_enter_blocking(&keybrd_queue);
    if (repeat_timer) {
        repeat_timer += KEYBUFFER_TICK_PD_MS;
    }
    critical_section_exit(&keybrd_queue);
}
// --
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
            keybuf[keycount] = '\0';
            putchar((int)c);
        }
    }

    lastkey = c;
    critical_section_exit(&keybrd_queue);
}
// --
static int keybrd_queue_pop_s(char * sbuf) {
    int rc = 0;
    critical_section_enter_blocking(&keybrd_queue);
    if (sbuf && keycount) {
        memcpy(sbuf, keybuf, keycount);
        rc = keycount;
        keycount = 0;
    }
    critical_section_exit(&keybrd_queue);
    return rc;
}
// --
static int keybrd_queue_pop_c(char * c) {
    int rc = 0;
    critical_section_enter_blocking(&keybrd_queue);
    if (c && keycount) {
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
// --


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

int main()
{
    repeating_timer_t kbtmr;
    uint32_t lc = 0;
    char keys[KEYBUFFER_LEN+1];
    int keycount = 0;

    stdio_init_all();

    /* ** [GPIO] Keyboard ---------------------- */
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
    }

    //// Set up our UART
    //uart_init(UART_ID, BAUD_RATE);
    //// Set the TX and RX pins by using the function select on the GPIO
    //// Set datasheet for more information on function select
    //gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    //gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    // Use some the various UART functions to send out data
    // In a default system, printf will also output via the default UART
    
    //// Send out a string, with CR/LF conversions
    //uart_puts(UART_ID, " Hello, UART!\n");
    printf("Testing Keyboard scan routines\n");

    // Timer example code - This example fires off the callback after 2000ms
    //add_alarm_in_ms(20, chk_keyboard, NULL, false);
    // For more examples of timer use see https://github.com/raspberrypi/pico-examples/tree/master/timer
    add_repeating_timer_ms(KYBD_SCAN_PD_MS, chk_keyboard, NULL, &kbtmr); 
    // For more examples of UART use see https://github.com/raspberrypi/pico-examples/tree/master/uart

    while (true) {
        keycount = keybrd_queue_pop_s(keys);
        keys[keycount] = '\0';
        printf("<%u> %s\n", lc++, keys);
        sleep_ms(3000);
    }
}

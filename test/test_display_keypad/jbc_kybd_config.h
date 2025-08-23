#ifndef JBC_KYBD_CONFIG_H
#define JBC_KYBD_CONFIG_H

/* JBC_PCB_BOARD_VER_3.1
 * Board configuration to support the JBC Iron Controller on a RP Pico 
 * mezzanine board.
 * 
 * Notes:
 * [1]      Pulled from the main code to provide a test framework for keyboard.
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

// external keyboard FIFO. Add key entries 
// using queue_push, remove keys using queue_pop
// The queue has an internal key-repeat check feature
// to prevent fast repeated key entries from occuring
#define KEYBUFFER_LEN           16
#define KEYBUFFER_TICK_PD_MS    KYBD_SCAN_PD_MS
#define KEYBUFFER_DLY_TMOUT_MS  200  /* key-repeat-delay [msec] */

#endif /* JBC_KYBD_CONFIG_H */

/* Define the board resources for enabled drivers and services
 *
 * Board used for testing is the 200W-JBC Ver 3.1 (2025) with
 * a RP2040 PICO plugged in.
 * 
 * Display: SSD1309 connected to J3.
 * 
 */

#ifndef BOARD_H
#define BOARD_H

/* SSD1309 Graphics Driver , 128 x 64 */
#define DISP_DRVR_SPI_CHAN          spi_default
#define DISP_DRVR_SPI_CLK           PICO_DEFAULT_SPI_SCK_PIN
#define DISP_DRVR_SPI_MISO          PICO_DEFAULT_SPI_RX_PIN
#define DISP_DRVR_SPI_MOSI          PICO_DEFAULT_SPI_TX_PIN
#define DISP_DRVR_SPI_CS            PICO_DEFAULT_SPI_CSN_PIN
#define DISP_DRVR_SPI_CLK_FREQ_HZ   4000000UL   /* 4 MHz */
#define DISP_DRVR_SPI_GPIO_DC       20
#define DISP_DRVR_SPI_GPIO_RST      28


#endif /* BOARD_H */

#include <stdio.h>
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "pico/time.h"
#include "ssd1309_driver.h"

#define SPI_FCLK_HZ     4000000UL   /* 4 MHz */
#define DISP_DC_CMD     0
#define DISP_DC_DATA    1
#define DISP_RST_ON     0
#define DISP_RST_OFF    1

// Enable to Rotate the display 180 deg.
#define ROTATE_DISPLAY  1

static int ssdd_spi_init(uint32_t Fclk) {
#if !defined(spi_default) || !defined(PICO_DEFAULT_SPI_SCK_PIN) || !defined(PICO_DEFAULT_SPI_TX_PIN) || !defined(PICO_DEFAULT_SPI_RX_PIN) || !defined(PICO_DEFAULT_SPI_CSN_PIN)
#warning spi/spi_master example requires a board with SPI pins
    puts("Default SPI pins were not defined");
#else
    // Enable SPI 0 at 1 MHz and connect to GPIOs
    spi_init(spi_default, (uint)Fclk); // mode: 0
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI);
    // Make the SPI pins available to picotool
    bi_decl(bi_4pins_with_func(PICO_DEFAULT_SPI_RX_PIN, PICO_DEFAULT_SPI_TX_PIN, PICO_DEFAULT_SPI_SCK_PIN, PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI));
#endif
    return 0;
}

ca_u CMD_SET_CONTRAST =         {.s.cmd = C_CONTRAST};
c_u  CMD_DISP_RESUME_RAM =      {.s.cmd = C_RES_TO_RAM};
c_u  CMD_DISP_ENT_ON =          {.s.cmd = C_ENT_DISP_ON};
c_u  CMD_DISP_NORMAL =          {.s.cmd = C_DISP_NORM};
c_u  CMD_DISP_INVERTED =        {.s.cmd = C_DISP_INV};
c_u  CMD_DISP_OFF =             {.s.cmd = C_DISP_OFF};
c_u  CMD_DISP_ON =              {.s.cmd = C_DISP_ON};
c_u  CMD_NOP =                  {.s.cmd = C_NOP};
ca_u CMD_LOCK =                 {.s.cmd = C_LOCK, .s.aa = AA_LOCK_SET};
ca_u CMD_UNLOCK =               {.s.cmd = C_LOCK, .s.aa = AA_LOCK_BASE};

cabcdefg_u CMD_HORZ_SCR_RIGHT = {.s.cmd = C_SCR_HRZ_RGT, .s.aa = AA_SCR_HRZ, .s.ee = EE_SCR_HRZ};
cabcdefg_u CMD_HORZ_SCR_LEFT =  {.s.cmd = C_SCR_HRZ_LFT, .s.aa = AA_SCR_HRZ, .s.ee = EE_SCR_HRZ};
cabcdefg_u CMD_V_RIGHT_H_SCROLL = {};
cabcdefg_u CMD_V_LEFT_H_SCROLL = {};

c_u  CMD_SCROLL_DEACTIVATE =    {.s.cmd = C_SCROLL_OFF};
c_u  CMD_SCROLL_ACTIVATE =      {.s.cmd = C_SCROLL_ON};
ca_u CMD_RAM_MODE_HORZ_ADDR =   {.s.cmd = C_SET_MA_MODE, .s.aa = AA_MA_MODE_H};
ca_u CMD_RAM_MODE_VERT_ADDR =   {.s.cmd = C_SET_MA_MODE, .s.aa = AA_MA_MODE_V};
ca_u CMD_RAM_MODE_PAGE_ADDR =   {.s.cmd = C_SET_MA_MODE, .s.aa = AA_MA_MODE_P};
c_u  CMD_REMAP_SEG0_IS_0 =      {.s.cmd = C_PRL_SEGRM_0};
c_u  CMD_REMAP_SEG0_IS_127 =    {.s.cmd = C_PRL_SEGRM_127};
c_u  CMD_REMAP_COM_IS_INCR =    {.s.cmd = C_COSDIR_NORM};
c_u  CMD_REMAP_COM_IS_DECR =    {.s.cmd = C_COSDIR_REV};

static uint8_t init_frame[] = {
    C_DISP_OFF,                         /* Display off */
    C_CDR_OF, 0xA0,                     /* CLK-Div-Ratio: (0)(div-by-1) , Fosc: (0xA)*/
    C_SET_MA_MODE, AA_MA_MODE_H,        /* Set Memory Address mode to "horizontal"  */
#if (ROTATE_DISPLAY==1)
    C_PRL_SEGRM_127,                    /* Seg remap: SEG0 @ addr:127               */
    C_COSDIR_REV,                     /* Set COM output scan direction to 'reverse' */
#endif
    C_CONTRAST, 0x6F,                   /* Adjust Contract                          */
    C_PCHG_PD, AA_PCHG(0xd,0x3),        /* Phase 1,2 Precharge Periods              */
    C_SET_VCOMH_DL, AA_VCOMH(8),        /* Set Vcomh Level (contract threshold)     */
    C_SCROLL_OFF,                       /* Disable scrolling                        */
    C_RES_TO_RAM,                       /* Set to refresh Display from RAM          */
    C_DISP_NORM,                        /* Normal pixels, black on white            */
    C_SET_COLADDR, 0x00, 0x7F,          /* Set col Addr range 0 .. 127 (reset ptr)  */
    C_SET_PAADDR, 0, 7                  /* Set Page Addr Range 0 .. 7 (reset ptr)   */
};
#define INIT_FRAME_LEN  sizeof(init_frame)

static uint gpio_dc = 0;
static uint gpio_res = 0;

#define SET_DISP_STATE_CMD  DISP_DC_CMD
#define SET_DISP_STATE_DATA DISP_DC_DATA
static void set_disp_dc(bool state) {
    gpio_put(gpio_dc, state);
}

static void pulse_disp_reset(void) {
    gpio_put(gpio_res, DISP_RST_ON);
    sleep_us(5);
    gpio_put(gpio_res, DISP_RST_OFF);
}

// Sets up Display GPIOS: Command Mode, Reset off
int ssd1309drv_init(uint8_t _gpio_dc, uint8_t _gpio_res) {
    gpio_dc = _gpio_dc;
    gpio_res = _gpio_res;
    gpio_init(gpio_dc);
    gpio_init(gpio_res);
    gpio_put(gpio_dc, DISP_DC_CMD);
    gpio_put(gpio_res, DISP_RST_OFF);
    gpio_set_dir(gpio_dc, 1);
    gpio_set_dir(gpio_res, 1);
    return ssdd_spi_init(SPI_FCLK_HZ);
}

int ssd1309drv_disp_init(void) {
    int wcnt = spi_write_blocking(spi_default, &(init_frame[0]), INIT_FRAME_LEN);
    return !(wcnt == INIT_FRAME_LEN);
}

int ssd1309drv_cmd(uint8_t * cmd, size_t cmdlen) {
}

int ssd1309drv_disp_off(void) {
    uint8_t * d = &(CMD_DISP_OFF.d);
    size_t len = sizeof(CMD_DISP_OFF.s);
    int wcnt;
    set_disp_dc(SET_DISP_STATE_CMD);
    wcnt = spi_write_blocking(spi_default, d, len);
    return !(wcnt == len);
}

int ssd1309drv_disp_on(void) {
    uint8_t * d = &(CMD_DISP_ON.d);
    size_t len = sizeof(CMD_DISP_ON.s);
    int wcnt;
    set_disp_dc(SET_DISP_STATE_CMD);
    wcnt = spi_write_blocking(spi_default, d, len);
    return !(wcnt == len);
}

static uint8_t zero_page[SSD1309_DISP_COLS] = {0};

int ssd1309drv_disp_blank(void) {
    int i, wcnt;
    int rc = 0;
    set_disp_dc(SET_DISP_STATE_DATA);
    for (i = 0 ; i < SSD1309_DISP_PAGES ; i++ ) {
        wcnt = spi_write_blocking(spi_default, &(zero_page[0]), SSD1309_DISP_COLS);
        if (wcnt != SSD1309_DISP_COLS) {
            rc = 1;
            break; // problem sending a page out
        }
    }
    return rc;
}

int ssd1309drv_disp_frame(uint8_t * octets, size_t len) {
    int wcnt;
    int rc = 1;
    if (octets && len == SSD1309_OCTET_COUNT) {
        set_disp_dc(SET_DISP_STATE_DATA);
        wcnt = spi_write_blocking(spi_default, octets, len);
        if (wcnt == SSD1309_OCTET_COUNT) {
            rc = 0;
        }
    }
    return rc;
}


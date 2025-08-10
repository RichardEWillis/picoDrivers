#include <stdio.h>
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "pico/time.h"
#include "ssd1309_driver.h" /* now a private local header */

//#define SPI_FCLK_HZ     4000000UL   /* 4 MHz */
#define DISP_DC_CMD     0
#define DISP_DC_DATA    1
#define DISP_RST_ON     0
#define DISP_RST_OFF    1

// Enable to Rotate the display 180 deg.
#define ROTATE_DISPLAY  1

static uint8_t isInitialized = 0;
static uint8_t isOpen = 0;

// Drivers private info struct
struct gfxData {
    spi_inst_t * spichan;   /* SPI channel to write onto */
    uint gpio_dc;           /* configured Display Data/Command pin */
    uint gpio_res;          /* configured Display Reset pin */
};

// global to this page, for allowing driver access to configured params.
struct gfxData g_gfxdata = {0};

// Drivers internal framebuffer. It can be used for data
// or higher layer may make its own. Pass this pointer into
// ssd1309drv_disp_frame() to write it into the display hardware.
uint8_t gfxFrameBuffer[FRAMEBUFFER_SIZE] = {0};

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

#define SET_DISP_STATE_CMD  DISP_DC_CMD
#define SET_DISP_STATE_DATA DISP_DC_DATA

static void set_disp_dc(bool state) {
    gpio_put(g_gfxdata.gpio_dc, state);
}

static void pulse_disp_reset(void) {
    gpio_put(g_gfxdata.gpio_res, DISP_RST_ON);
    sleep_us(5);
    gpio_put(g_gfxdata.gpio_res, DISP_RST_OFF);
}

int ssd1309drv_disp_open(void) {
    isOpen = 1;
    return 0; // not req.
}

int ssd1309drv_disp_init(void) {
    int wcnt = spi_write_blocking(g_gfxdata.spichan, &(init_frame[0]), INIT_FRAME_LEN);
    isInitialized = (wcnt == INIT_FRAME_LEN);
    return !(wcnt == INIT_FRAME_LEN);
}

int ssd1309drv_disp_is_ready(void) {
    return (isInitialized && isOpen);
}

int ssd1309drv_disp_close(void) {
    isOpen = 0;
    return 0; // no actions.
}

int ssd1309drv_cmd(uint8_t * cmd, size_t cmdlen) {
    return 1;
}

int ssd1309drv_disp_off(void) {
    uint8_t * d = &(CMD_DISP_OFF.d);
    size_t len = sizeof(CMD_DISP_OFF.s);
    int wcnt;
    set_disp_dc(SET_DISP_STATE_CMD);
    wcnt = spi_write_blocking(g_gfxdata.spichan, d, len);
    return !(wcnt == len);
}

int ssd1309drv_disp_on(void) {
    uint8_t * d = &(CMD_DISP_ON.d);
    size_t len = sizeof(CMD_DISP_ON.s);
    int wcnt;
    set_disp_dc(SET_DISP_STATE_CMD);
    wcnt = spi_write_blocking(g_gfxdata.spichan, d, len);
    return !(wcnt == len);
}

static uint8_t zero_page[SSD1309_DISP_COLS] = {0};

int ssd1309drv_disp_blank(void) {
    int i, wcnt;
    int rc = 0;
    set_disp_dc(SET_DISP_STATE_DATA);
    for (i = 0 ; i < SSD1309_DISP_PAGES ; i++ ) {
        wcnt = spi_write_blocking(g_gfxdata.spichan, &(zero_page[0]), SSD1309_DISP_COLS);
        if (wcnt != SSD1309_DISP_COLS) {
            rc = 1;
            break; // problem sending a page out
        }
    }
    return rc;
}

int ssd1309_disp_invert(int do_invert) {
    return 1; // not supported yet
}

int ssd1309_disp_flip_x(int do_flip) {
    return 1; // not supported yet
}

int ssd1309_disp_flip_y(int do_flip) {
    return 1; // not supported yet
}

int ssd1309_disp_rot180(int do_rot) {
    return 1; // not supported yet
}

int ssd1309_disp_contrast(int con) {
    return 1; // not supported yet
}

int ssd1309_disp_brightness(int bri) {
    return 1; // not supported yet
}

const char * ssd1309_disp_get_DriverName(void) {
    return "SSD1309";
}

size_t ssd1309_disp_get_FBSize(void) {
    return FRAMEBUFFER_SIZE;
}

size_t ssd1309_disp_get_DispWidth(void) {
    return SSD1309_PIX_WIDTH;
}

size_t ssd1309_disp_get_DispHeight(void) {
    return SSD1309_PIX_HEIGHT;
}

size_t ssd1309_disp_get_DispPageHeight(void) {
    return SSD1309_DISP_PAGES;
}

int ssd1309drv_disp_frame(const uint8_t * octets) {
    int wcnt;
    int rc = 1;
    if (octets) {
        set_disp_dc(SET_DISP_STATE_DATA);
        wcnt = spi_write_blocking(g_gfxdata.spichan, octets, SSD1309_OCTET_COUNT);
        if (wcnt == SSD1309_OCTET_COUNT) {
            rc = 0;
        }
    }
    return rc;
}

uint8_t * ssd1309drv_disp_get_local_framebuffer(void) {
    return (uint8_t *)&(gfxFrameBuffer[0]);
}

#include "../gfxDriverLowPriv.h"
#include <board.h>

int ssd1309_probe(gfxDriver_p_p drvrStack) {
    // Setup HW SPI and GPIOs
    spi_init(DISP_DRVR_SPI_CHAN, (uint)DISP_DRVR_SPI_CLK_FREQ_HZ); // mode: 0
    gpio_set_function(DISP_DRVR_SPI_MISO, GPIO_FUNC_SPI);
    gpio_set_function(DISP_DRVR_SPI_CLK, GPIO_FUNC_SPI);
    gpio_set_function(DISP_DRVR_SPI_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(DISP_DRVR_SPI_CS, GPIO_FUNC_SPI);
    // Make the SPI pins available to picotool
    bi_decl(bi_4pins_with_func(DISP_DRVR_SPI_MISO, DISP_DRVR_SPI_MOSI, DISP_DRVR_SPI_CLK, 
        DISP_DRVR_SPI_CS, GPIO_FUNC_SPI));
    // Setup reset, data/command GPIO pins
    // Sets up Display GPIOS: Command Mode, Reset off
    gpio_init(DISP_DRVR_SPI_GPIO_DC);
    gpio_init(DISP_DRVR_SPI_GPIO_RST);
    gpio_put(DISP_DRVR_SPI_GPIO_DC, DISP_DC_CMD);
    gpio_put(DISP_DRVR_SPI_GPIO_RST, DISP_RST_OFF);
    gpio_set_dir(DISP_DRVR_SPI_GPIO_DC, 1);
    gpio_set_dir(DISP_DRVR_SPI_GPIO_RST, 1);

    g_gfxdata.spichan = DISP_DRVR_SPI_CHAN;
    g_gfxdata.gpio_dc = DISP_DRVR_SPI_GPIO_DC;
    g_gfxdata.gpio_res = DISP_DRVR_SPI_GPIO_RST;
    /* ... */
    // Setup LL struct
    drvrStack->displayOn = &ssd1309drv_disp_on;
    drvrStack->displayOff = &ssd1309drv_disp_off;
    drvrStack->set_displayInvert = &ssd1309_disp_invert;
    drvrStack->pset_displayFlipX = &ssd1309_disp_flip_x;
    drvrStack->set_displayFlipY = &ssd1309_disp_flip_y;
    drvrStack->set_displayRot = &ssd1309_disp_rot180;
    drvrStack->set_contrast = &ssd1309_disp_contrast;
    drvrStack->set_brightness = &ssd1309_disp_brightness;
    drvrStack->refreshDisplay = &ssd1309drv_disp_frame;
    drvrStack->clearDisplay = &ssd1309drv_disp_blank;
    drvrStack->driverName = &ssd1309_disp_get_DriverName;
    drvrStack->get_FBSize = &ssd1309_disp_get_FBSize;
    drvrStack->get_DispWidth = &ssd1309_disp_get_DispWidth;
    drvrStack->get_DispHeight = &ssd1309_disp_get_DispHeight;
    drvrStack->get_DispPageHeight = &ssd1309_disp_get_DispPageHeight;
    drvrStack->get_drvrFrameBuffer = &ssd1309drv_disp_get_local_framebuffer;
    drvrStack->IsReady = &ssd1309drv_disp_is_ready;
    // private control methods, BSP only
    drvrStack->Open = &ssd1309drv_disp_open;
    drvrStack->Init = &ssd1309drv_disp_init;
    drvrStack->Close = &ssd1309drv_disp_close;
    // Driver specific data, driver only
    drvrStack->dinfo = &g_gfxdata;
    return 0;
}

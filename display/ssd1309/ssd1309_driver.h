#ifndef __SSD1309_DRIVER_H__
#define __SSD1309_DRIVER_H__

#include <stdint.h>

/* --- Display Parameters ------------------------------------------------- */

#define SSD1309_DISP_COLS           128
#define SSD1309_DISP_PAGES          8
#define SSD1309_DISP_PIX_PER_OCTET  8
#define SSD1309_OCTET_COUNT         (SSD1309_DISP_COLS * SSD1309_DISP_PAGES)
#define SSD1309_PIXCOUNT            (SSD1309_OCTET_COUNT * SSD1309_DISP_PIX_PER_OCTET)
#define SSD1309_PIX_WIDTH           SSD1309_DISP_COLS
#define SSD1309_PIX_HEIGHT          (SSD1309_DISP_PAGES * SSD1309_DISP_PIX_PER_OCTET)
#define FRAMEBUFFER_SIZE            SSD1309_OCTET_COUNT

/* --- Structures and Types ----------------------------------------------- */

// command byte
typedef struct c_type {
    uint8_t cmd;
} c_t;
#define C_T_LEN sizeof(c_t)

// command byte + 1 arg
typedef struct ca_type {
    uint8_t cmd;
    uint8_t aa;
} ca_t;
#define CA_T_LEN sizeof(ca_t)

// command byte + 2 args
typedef struct cab_type {
    uint8_t cmd;
    uint8_t aa;
    uint8_t bb;
} cab_t;
#define CAB_T_LEN sizeof(cab_t)

// command byte + 7 args
typedef struct cabcdefg_type {
    uint8_t cmd;
    uint8_t aa;
    uint8_t bb;
    uint8_t cc;
    uint8_t dd;
    uint8_t ee;
    uint8_t ff;
    uint8_t gg;
} cabcdefg_t;
#define CABCDEFG_T_LEN sizeof(cabcdefg_t)

typedef union c_u {
    c_t     s;
    uint8_t d;
} c_u;

typedef union ca_u {
    ca_t    s;
    uint8_t d[CA_T_LEN];
} ca_u;

typedef union cab_u {
    cab_t   s;
    uint8_t d[CAB_T_LEN];
} cab_u;

typedef union cabcdefg_u {
    cabcdefg_t s;
    uint8_t    d[CABCDEFG_T_LEN];
} cabcdefg_u;

/* --- Commands ----------------------------------------------------------- */

/* Fundamental Commands */
#define C_CONTRAST      0x81                    /* Set display contrast (aa)    */
#define C_RES_TO_RAM    0xA4                    /* restore to RAM               */
#define C_ENT_DISP_ON   0xA5                    /* entire display on            */
#define C_DISP_NORM     0xA6                    /* normal display (black pix)   */
#define C_DISP_INV      0xA7                    /* inverted display (wht pix)   */
#define C_DISP_OFF      0xAE                    /* display off                  */
#define C_DISP_ON       0xAF                    /* display on                   */
#define C_NOP           0xE3                    /* No-Operation                 */
#define C_LOCK          0xFD                    /* Set/Clear command lockout    */
#define AA_LOCK_BASE    0x12                    /*   Clear lock                 */
#define AA_LOCK_SET     (AA_LOCK_BASE | (1 << 2)) /* Set lock                   */
/* Scrolling Commands */
#define C_SCR_HRZ_RGT   0x26                    /* horizaontal scroll to right  */
#define C_SCR_HRZ_LFT   0x27                    /* horizaontal scroll to left   */
#define AA_SCR_HRZ      0x00                    /* horz cmd only                */
#define BB_SCR_MASK     0x07                    /* both horz and vert commands  */
#define BB_SCR(x)       (x & BB_SCR_MASK)       /* ""                           */
#define CC_SCR_MASK     0x07                    /* ""                           */
#define CC_SCR(x)       (x & CC_SCR_MASK)       /* ""                           */
#define DD_SCR_MASK     0x07                    /* ""                           */
#define DD_SCR(x)       (x & DD_SCR_MASK)       /* ""                           */
#define EE_SCR_HRZ      0x00                    /* horz cmd only                */
#define FF_SCR_MASK     0x07                    /* both horz and vert commands  */
#define FF_SCR(x)       (x & FF_SCR_MASK)       /* ""                           */
#define GG_SCR_MASK     0x07                    /* ""                           */
#define GG_SCR(x)       (x & GG_SCR_MASK)       /* ""                           */
#define C_SCR_HV_VR     0x29                    /* scroll vert. AND right       */
#define C_SCR_HV_VL     0x2A                    /* scroll vert. AND left        */
#define AA_SCR_HV_H_ON  0x01                    /* in HV scroll, stop H portion */    
#define AA_SCR_HV_H_OFF 0x00                    /* in HV scroll, run H portion  */    
#define EE_SCR_HV_RCMSK 0x3F                    /* mask: row count per step (V) */
#define EE_SCR_HV(x)    (x & EE_SCR_HV_RCMSK)   /* row count per step (V)       */
#define C_SCROLL_OFF    0x2E                    /* stop scrolling operations    */
#define C_SCROLL_ON     0x2F                    /* start scrolling operations   */
#define C_SCR_VCA       0xA3                    /* set vert. scroll area        */
#define AA_SCRVCA_MASK  0x3F
#define AA_SCRVCA(x)    (x & AA_SCRVCA_MASK)    /* # fxed top rows (no scroll)  */
#define BB_SCRVCA_MASK  0x7F
#define CC_SCRVCA(x)    (x & BB_SCRVCA_MASK)    /* # scrolling rows below fix'd */
#define C_SCRWIN_RGH    0x2C                    /* setup right scrolling window */
#define C_SCRWIN_LFT    0x2D                    /* setup left scrolling window  */
#define AA_SCRWIN       0x00                    /* leave at 0                   */
#define BB_SCRWIN_MASK  0x07
#define BB_SCRWIN(x)    (x & BB_SCRWIN_MASK)    /* start PAGE number (idx)      */
#define CC_SCRWIN       0x01                    /* leave at 1                   */
#define DD_SCRWIN_MASK  0x07
#define DD_SCRWIN(x)    (x & DD_SCRWIN_MASK)    /* end PAGE number              */
#define EE_SCRWIN       0x00                    /* leave at 0                   */
#define FF_SCRWIN_MASK  0x7F
#define FF_SCRWIN(x)    (x & FF_SCRWIN_MASK)    /* start (left) column #        */
#define GG_SCRWIN_MASK  0x7F
#define GG_SCRWIN(x)    (x & GG_SCRWIN_MASK)    /* end (right) column #         */
/* For Page Addressing Mode Only */
#define C_PA_LCOL_SA(x) (x & 0x0F)              /* PAM : Low nibble col addr.   */
#define C_PA_HCOL_SA(x) (0x10 | (x & 0x0F))     /* PAM : High nibble col addr.  */
#define C_PA_SADDR(x)   (0xB0 | (x & 0x07))     /* PAM : Page start addr.       */
/* All Addressing Modes */
#define C_SET_MA_MODE   0x20                    /* Set Memory Addressing Mode:  */
#define AA_MA_MODE_H    0x00                    /*    Horizontal mode           */
#define AA_MA_MODE_V    0x01                    /*    Vertical mode             */
#define AA_MA_MODE_P    0x02                    /*    Page mode (default)       */
#define C_SET_COLADDR   0x21                    /* Set column Address           */
#define AA_COLADDR_MASK 0x7F
#define AA_COLADDR(x)   (x & AA_COLADDR_MASK)   /*    start address             */
#define BB_COLADDR_MASK 0x7F
#define BB_COLADDR(x)   (x & BB_COLADDR_MASK)   /*    end addr (above start)    */
#define C_SET_PAADDR    0x22                    /* Set Page Address (index)     */
#define AA_PAADDR_MASK  0x07                   
#define AA_PAADDR(x)    (x & AA_PAADDR_MASK)    /*    start page #              */
#define BB_PAADDR_MASK  0x07
#define BB_PAADDR(x)    (x & BB_PAADDR_MASK)    /*    end page #                */
/* Panel Resolution and Layout Configuration */
#define C_PRL_DSLINE(x) (0x40 | (x & 0x3F))     /* Display start line (0..63)   */
#define C_PRL_SEGRM_0   0xA0                    /* Seg remap: SEG0 @ addr:0     */
#define C_PRL_SEGRM_127 0xA1                    /* Seg remap: SEG0 @ addr:127   */
#define C_PRL_MUXRAT    0xA8                    /* Set multiplex ratio to aa+1  */
#define AA_PRL_MRAT(x)  (0x0F | (x & 0x3F))     /* aa range: 15 .. 63           */
#define C_COSDIR_NORM   0xC0                    /* Set COM output scan direction to 'normal' */
#define C_COSDIR_REV    0xC8                    /* Set COM output scan direction to 'reverse' */
#define C_DOFF_VERT     0xD3                    /* Set Disp Offset (Vert)       */
#define AA_DOFF_V_MASK  0x3F
#define AA_DOFF_V(x)    (x & AA_DOFF_V_MASK)    /*   range (0..63)              */
#define C_CP_H_CFG      0xDA                    /* Set COM pins H/W Config.     */
                        /* pe := disable/enable COM left/right remap (0 := dis.)*/
                        /* pc := seq./alt. COM pin configuration (0 := seq.)    */
#define AA_CP_HC(pe,pc) (0x02 | ((pe & 0x01) << 5) | ((pc & 0x01) << 4))
#define C_SET_GPIO      0xDC                    /* configure single GPIO pin    */
#define AA_GPIO_OFF     0x00                    /* GPIO disabled, Hi-Z          */
/* GPIO Pin */
#define AA_GPIO_IN      0x01                    /* GPIO as input (N/A for SPI)  */
#define AA_GPIO_OUT_HI  0x03                    /* GPIO out-high                */
#define AA_GPIO_OUT_LOW 0x02                    /* GPIO out-low                 */
#define AA_GPIO(x)      (x & 0x03)              /* set to one of the 4 above    */
/* Timing and Driving Configuration */
#define C_CDR_OF        0xD5                    /* Display (clk-div-rat)(Fosc)  */
                        /* r := clk div. ratio, n+1 (0 .. 15), 0 := div-by-1    */
                        /* f := clk freq. (0..15) (default:7)                   */
#define AA_CDR_OF(r,f)  (((f & 0x0f) << 4) | (r & 0x0f))
#define C_PCHG_PD       0xD9                    /* Set precharge period         */
                        /* p1 := phase-1 period (0 invalid) (default:2)         */
                        /* p2 := phase-2 period (0 invalid) (default:2)         */
#define AA_PCHG(p1,p2)  (((p2 & 0x0f) << 4) | (p1 & 0x0f))
#define C_SET_VCOMH_DL  0xDB                    /* Set Vcomh deselect level (contrast) */
#define AA_VCOMH(x)     ((x & 0x0f) << 2)       /* (range 0..15) default: 13    */

extern ca_u         CMD_SET_CONTRAST;           // change contrast value in aa
extern c_u          CMD_DISP_RESUME_RAM;        // Disp opt. resume to RAM (A4h)
extern c_u          CMD_DISP_ENT_ON;            // Disp opt. Entire Display On (A5h)
extern c_u          CMD_DISP_NORMAL;            // pixel-on = BLACK
extern c_u          CMD_DISP_INVERTED;          // pixel-on = WHITE
extern c_u          CMD_DISP_OFF;               // turn off display
extern c_u          CMD_DISP_ON;                // turn on display
extern c_u          CMD_NOP;                    // special NOP command, does nothing
extern ca_u         CMD_LOCK;                   // lock command bus (req. unlock)
extern ca_u         CMD_UNLOCK;                 // unlock command bus
extern cabcdefg_u   CMD_HORZ_SCR_RIGHT;         // Horizontal scroll to the right (non windowed)
extern cabcdefg_u   CMD_HORZ_SCR_LEFT;          // Horizontal scroll to the left (non windowed)
extern cabcdefg_u   CMD_V_RIGHT_H_SCROLL;       // Vert. and Horizontal to Right Scrolling
extern cabcdefg_u   CMD_V_LEFT_H_SCROLL;        // Vert. and Horizontal to Left Scrolling
extern c_u          CMD_SCROLL_DEACTIVATE;      // turn off any active screen scrolling
extern c_u          CMD_SCROLL_ACTIVATE;        // turn on screen scrolling as per config.
extern ca_u         CMD_RAM_MODE_HORZ_ADDR;     // memory programming mode: "horizontal addressing"
extern ca_u         CMD_RAM_MODE_VERT_ADDR;     // memory programming mode: "vertical addressing"
extern ca_u         CMD_RAM_MODE_PAGE_ADDR;     // memory programming mode: "page addressing" (default)
extern c_u          CMD_REMAP_SEG0_IS_0;        // segment remap: SEG0 mapped to addr:0
extern c_u          CMD_REMAP_SEG0_IS_127;      // segment remap: SEG0 mapped to addr:127
extern c_u          CMD_REMAP_COM_IS_INCR;      // COM scanned COM0 .. COM(n-1)
extern c_u          CMD_REMAP_COM_IS_DECR;      // COM scanned COM(n-1) .. COM0

  // command constructors (variable data)

#define CMD_SET_CONTRAST(x) ca_u cmd={s.aa = x}

#endif /* __SSD1309_DRIVER_H__ */

#include <linux/tty.h>
#include <linux/console.h>
#include <linux/string.h>

#include "t6963.c"

u16 framebuffer[LCD_SIZE];


static int lcd_first_vc = 1;
static int lcd_last_vc  = 16;
static struct vc_data *lcd_display_fg = NULL;

static enum { TYPE_LCD } lcd_type;
static char *lcd_type_name;

module_param(lcd_first_vc, int, 0);
module_param(lcd_last_vc, int, 0);

static const char __init *lcdcon_startup(void) {
    lcd_num_col = LCD_COLS;
    lcd_num_row = LCD_ROWS;

    lcd_graphics_base = 0x00;
    
    /* XXX: set cursor?? */

    lcd_type_name = "generic Toshiba T6963C";
    lcd_type = TYPE_LCD;

    printk("t6963: display startup.\n");

    return "T6963";
}

static void lcd_clear_text_area(void) {
    u16 i;

    // clear the framebuffer
    for(i=0; i<LCD_SIZE;i++) {
        framebuffer[i]=0;
    }

    // clear the lcd screen
    lcd_pos_cursor(0,0);

    lcd_cmd_long(lcd_graphics_base,CMD_ADDR_PTR); 
}

static void lcdcon_init(struct vc_data *c, int init) {
    c->vc_complement_mask = 0x0800; /* reverse video */
    c->vc_display_fg = &lcd_display_fg;

    if(init) {
        c->vc_cols=lcd_num_col;
        c->vc_rows=lcd_num_row;
    } else {
        vc_resize(c->vc_num, lcd_num_col, lcd_num_row);
    }
    
    /* make the first LCD console visible */
    if(lcd_display_fg == NULL)
        lcd_display_fg = c;

    MOD_INC_USE_COUNT;
}

static void lcdcon_deinit(struct vc_data *c) {
    if(mda_display_fg == c)
        mda_display_fg = NULL;

    MOD_DEC_USE_COUNT;
}

static inline u16 lcd_convert_attr(u16 ch) {

static inline u16 lcd_convert_attr(u16 ch)
{
    /* only reverse, blinking and reverse+blinking supported */
    /* underline simulated by reverse */
    u16 attr=0;

#ifdef LCD_DEBUG
        printk(KERN_DEBUG "mdacon.c for lcd: mdacon_convert_attr %i\n",ch);
#endif

    if (ch & 0x8000) {
        // blink
        attr |= 0x0800;
    }
    if (ch & 0x1000) {
        // reverse
        attr |= 0x0500;
    }
    if (ch & 0x0800) {
        // underline, simulate by reverse
        attr |= 0x0500;
    }

    return (ch & 0x00ff) | attr;
}

static u8 lcdcon_build_attr(struct vc_data *c, u8 color, u8 intensity,
        u8 blink, u8 underline, u8 reverse) {
    /* The attribute is just a bit vector:
     *  
     *  Bit 0..1 : intensity (0..2)
     *  Bit 2    : underline
     *  Bit 3    : reverse
     *  Bit 7    : blink
     */

    return (intensity & 3) |
        ((underline & 1) << 2) |
        ((reverse   & 1) << 3) |
        ((blink     & 1) << 7);
}

static void lcdcon_invert_region(struct vc_data *c, u16 *p, int count) {
    /*
    for(; count > 0; count--) {
        scr_writew(scr_readw(p) ^ 0x0800, p);
        p++;
    }
    */
}

static void lcdcon_putc(struct vc_data *c, int ch, int y, int x) {
    u16 pos;

    pos=LCD_POS(x,y);

    /* XXX: STOPPED HERE 4am */
    lcd_cmd_long(pos, CMD_ADDR_PTR); /* set the address */
    lcd_write(ch);
}

const struct consw lcd_con = {
    .con_startup =          lcdcon_startup,
    .con_init =             lcdcon_init,
    .con_deinit =           lcdcon_deinit,
    .con_clear =            lcdcon_clear,
    .con_putc =             lcdcon_putc,
    .con_putcs =            lcdcon_putcs,
    .con_cursor =           lcdcon_cursor,
    .con_scroll =           lcdcon_scroll,
    .con_bmove =            lcdcon_bmove,
    .con_switch =           lcdcon_switch,
    .con_blank =            lcdcon_blank,
    .con_font_op =          lcdcon_font_op,
    .con_set_palette =      lcdcon_set_palette,
    .con_scrolldelta =      lcdcon_scrolldelta,
    .con_build_attr =       lcdcon_build_attr,
    .con_invert_region =    lcdcon_invert_region,
};

int __init lcd_console_init(void) {
    if(lcd_first_vc > lcd_last_vc)
        return 1;

    take_over_console(&lcd_con, lcd_first_vc-1, lcd_last_vc-1, 0);
    return 0
}

void __exit lcd_console_exit(void) {
    give_up_console(&lcd_con);
}

module_init(lcd_console_init);
module_exit(lcd_console_exit);

/*******************************************************************************
 * T6963C Device Driver
 *
 * Jacob Farkas
 * March 4, 2005
 *
 * Adapted from Alxeander Frink's 2.2 T6963C driver and Linux mdacon.c
 *
 *      Pinout:
 *   PC         LCD
 * ------  -------------
 *   1      READ (10)
 *   2-9    DATA0-7 (1-8)
 *   14     ENABLE (12)
 *   16     C/D (9)
 *   17     WRITE (11)
 *
 ******************************************************************************/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <asm/io.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <asm/segment.h>

#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/config.h>
#include <linux/version.h>
#include <linux/spinlock.h>

#include "t6963.h"
#include "t6963_commands.h"

MODULE_LICENSE("GPL");

static spinlock_t lcd_lock = SPIN_LOCK_UNLOCKED;

/* display size */
#define LCD_ROWS        5
#define LCD_COLS        30
#define LCD_SIZE        (LCD_ROWS*LCD_COLS)

/* change your parallel port here */
#define PORT_BASE       0x378
#define DATA            (PORT_BASE)
#define STATUS          (PORT_BASE+1)
#define CTRL            (PORT_BASE+2)

/* 
 *      Pinout:
 *   PC         LCD
 * ------  -------------
 *   1      READ (10)
 *   2-9    DATA0-7 (1-8)
 *   14     ENABLE (12)
 *   16     C/D (9)
 *   17     WRITE (11)
 *
 * If your LCD pinout is different, change it here:
 */

#define CTRL_READ       0x01 // 1: read
#define CTRL_CE         0x02 // 1: enable
#define CTRL_CMD        0x04 // 1: command, 0: data
#define CTRL_WRITE      0x08 // 1: write

#define CTRL_RESET           /* there are not enough output pins on the
                              * parallel port to include the reset pin.
                              * the Toshiba data sheet says that it must be 
                              * pulled low when the device is first powered on, 
                              * but the display works fine without it
                              */

#define LCD_RETRY_LIM   255  // number of times to check the status before 
                             // giving up

#define LCD_DEBUG       0   // printk() debug messages

/* I'm using non-delayed output for these macros. Put the bits on the wire, then
 * delay the necessary amount of time with a DELAY, rather than delaying for
 * each pin.
 */

#define LCD_DELAY       outb_p(0x00, 0x80)

#define READ_H          outb(inb(CTRL) | CTRL_READ, CTRL)
#define READ_L          outb(inb(CTRL) & ~CTRL_READ, CTRL)
#define CE_H            outb(inb(CTRL) | CTRL_CE, CTRL) 
#define CE_L            outb(inb(CTRL) & ~CTRL_CE, CTRL)
#define LCD_COMMAND     outb(inb(CTRL) | CTRL_CMD, CTRL)
#define LCD_DATA        outb(inb(CTRL) & ~CTRL_CMD, CTRL)
#define WRITE_H         outb(inb(CTRL) | CTRL_WRITE, CTRL)
#define WRITE_L         outb(inb(CTRL) & ~CTRL_WRITE, CTRL)
#define DATA_IN         outb(0x20, CTRL)
#define DATA_OUT        outb(0xd0, CTRL)


/* state variables */
static struct t6963_status lcd_stat;
//static unsigned int lcd_num_col, lcd_num_row;
//static unsigned int lcd_graphics_base=0;
//static unsigned int lcd_graphics_size=0;
//static u8 lcd_row_width;

//int lcd_addr_ptr;
//int lcd_cursor_pos;
//char lcd_cursor_size;

//int lcd_text_base=0;
//int lcd_text_size=0;

//u8 lcd_display_mode=0;

#define LCD_POS(x,y)    ((y)*(lcd_stat.lcd_num_col) + (x))

static u8 lcd_status(void) {
    u8 stat;
    unsigned long flags;

    spin_lock_irqsave(&lcd_lock, flags);

    DATA_IN;
    LCD_COMMAND;
    READ_H;
    CE_H;
    LCD_DELAY;

    stat=inb_p(DATA);

    DATA_OUT;
    LCD_DELAY;

    if(LCD_DEBUG>3)
        printk("t6963: status %02x\n", stat);

    spin_unlock_irqrestore(&lcd_lock, flags);

    lcd_stat.status=stat;
    
    return stat;
}

static char lcd_status_poll(void) {
    u8 i, stat;

    for(i=0;i<LCD_RETRY_LIM;i++) {
        stat=lcd_status();
        if((stat & STATUS_CMD) && (stat & STATUS_RW))
            return 0;
    }

    if(i==LCD_RETRY_LIM) {
        if(LCD_DEBUG)
            printk("t6963: status polling failed!\n");
        return -1;
    }

    return 0;
}

static char lcd_aw_status_poll(void) {
    u8 i;

    for(i=0;i<LCD_RETRY_LIM;i++) {
        if(lcd_status() & (STATUS_AUTO_WR))
            return 0;
    }

    if(i==LCD_RETRY_LIM) {
        if(LCD_DEBUG)
            printk("t6963: auto write status polling failed!\n");
        return -1;
    }

    return 0;
}

static char lcd_ar_status_poll(void) {
    u8 i;

    for(i=0;i<LCD_RETRY_LIM;i++) {
        if(lcd_status() & (STATUS_AUTO_RD))
            return 0;
    }

    if(i==LCD_RETRY_LIM) {
        if(LCD_DEBUG)
            printk("t6963: auto read status polling failed!\n");
        return -1;
    }

    return 0;
}

static char lcd_cmd(u8 cmd) {
    unsigned long flags;

    if(lcd_status_poll())
        return -1;
    
    spin_lock_irqsave(&lcd_lock, flags);
    DATA_OUT;

    outb_p(cmd, DATA);
  
    LCD_COMMAND;
    WRITE_H;
    CE_H;
    LCD_DELAY;
  
    CE_L;
    WRITE_L;
    LCD_COMMAND;
    LCD_DELAY;

    spin_unlock_irqrestore(&lcd_lock, flags);

    return 0;
}

static void _lcd_write(u8 data) {
    unsigned long flags;

    spin_lock_irqsave(&lcd_lock, flags);
    
    outb_p(data, DATA);

    DATA_OUT;
    LCD_DATA;
    WRITE_H;
    CE_H;
    LCD_DELAY;

    DATA_OUT;
    LCD_DELAY;

    spin_unlock_irqrestore(&lcd_lock, flags);
}

static char lcd_write(u8 data) {
    if(lcd_status_poll())
        return -1;
    _lcd_write(data);
    return 0;
}

static char lcd_auto_write(u8 data) {
    if(lcd_aw_status_poll()) {
        if(LCD_DEBUG)
            printk("---auto write status polling failed!\n");
        return -1;
    }
    _lcd_write(data);
    return 0;
}

static u8 _lcd_read(void) {
    u8 data;
    unsigned long flags;

    spin_lock_irqsave(&lcd_lock, flags);

    DATA_IN;
    LCD_DELAY;

    LCD_DATA;
    READ_H;
    CE_H;
    LCD_DELAY;

    data=inb(DATA);

    CE_L;
    READ_L;
    LCD_COMMAND;

    spin_unlock_irqrestore(&lcd_lock, flags);
    return data;
}

static u8 lcd_read(u8 *data) {
    if(lcd_status_poll())
        return 1;
    *data=_lcd_read();
    return 0;
}

static u8 lcd_auto_read(u8 *data) {
    if(lcd_ar_status_poll())
        return 1;
    *data=_lcd_read();
    return 0;
}

static char lcd_cmd_d1(u8 data, u8 cmd) {
    if(lcd_write(0)<0)
        return -1;
    if(lcd_write(data)<0)
        return -1;
    if(lcd_cmd(cmd)<0)
        return -1;

    return 0;
}

static char lcd_cmd_d2(u8 data1, u8 data2, u8 cmd) {
    if(lcd_write(data1)<0)
        return -1;
    if(lcd_write(data2)<0)
        return -1;
    if(lcd_cmd(cmd)<0)
        return -1;
    
    return 0;
}

static char lcd_cmd_long(u16 data, u8 cmd) {
    if(lcd_write(data&0xff)<0)
        return -1;
    if(lcd_write(data>>8)<0)
        return -1;
    if(lcd_cmd(cmd)<0)
        return -1;

    return 0;
}

static void lcd_pos_cursor(int x, int y) {
    lcd_cmd_d2(x,y,CMD_CURSOR_POS);
}

static char lcd_write_text(const u8 *data, int count) {
    int i;

    if(count<=0)
        return 0;

    if(LCD_DEBUG>2)
        printk("t6963: auto text write \"");
    lcd_cmd(CMD_AUTO_WRITE);
    for(i=0;i<count;i++) {
        if(LCD_DEBUG>2)
            printk("%02x", *(data+i));
        if(lcd_auto_write(*(data+i)-0x20)) 
            return -1;
    }
    lcd_cmd(CMD_AUTO_RESET);
    if(LCD_DEBUG>2)
        printk("\"\n");
    return i;
}

static char lcd_write_bytes(const u8 *data, int count) {
    int i;

    if(count<=0)
        return 0;

    if(LCD_DEBUG>2)
        printk("t6963: auto write \"");
    lcd_cmd(CMD_AUTO_WRITE);
    for(i=0;i<count;i++) {
        if(LCD_DEBUG>2)
            printk("%02x", *(data+i));
        if(lcd_auto_write(*(data+i))) 
            return -1;
    }
    lcd_cmd(CMD_AUTO_RESET);
    if(LCD_DEBUG>2)
        printk("\"\n");
    return i;
}

static char lcd_read_bytes(u8 *data, int count) {
    int i;

    if(count<=0)
        return 0;
    
    lcd_cmd(CMD_AUTO_READ);
    for(i=0;i<count;i++) {
        if(lcd_auto_read(data+i))
            return -1;
    }
    lcd_cmd(CMD_AUTO_RESET);
    return i;
}

static char lcd_text_clear(void) {
    int i;
    // reset addr pointer
    if(lcd_cmd_long(lcd_stat.text_base,CMD_ADDR_PTR)<0) 
        return -1;

    // clear screen
    if(lcd_cmd(CMD_AUTO_WRITE)<0)
        return -1;
    for(i=lcd_stat.text_base;i<lcd_stat.graphics_base;i++) {
        if(lcd_auto_write(0x00)<0)
            return -1;
    }

    printk("t6963: text memory from 0x%04x to 0x%04x cleared.\n", 
            lcd_stat.text_base, lcd_stat.text_base+i);

    if(lcd_cmd(CMD_AUTO_RESET)<0)
        return -1;
    return 0;
}

static char lcd_graphics_clear(void) {
    int i;
    // reset addr pointer
    if(lcd_cmd_long(lcd_stat.graphics_base,CMD_ADDR_PTR)<0) 
        return -1;

    // clear screen
    if(lcd_cmd(CMD_AUTO_WRITE)<0)
        return -1;
    for(i=lcd_stat.graphics_base;
        i<lcd_stat.graphics_base+(2*(8*lcd_stat.row_width*lcd_stat.rows))
        ;i++) {
        if(lcd_auto_write(0x00)<0)
            return -1;
    }

    printk("t6963: graphics memory from 0x%04x to 0x%04x cleared.\n", lcd_stat.graphics_base, i);

    if(lcd_cmd(CMD_AUTO_RESET)<0)
        return -1;
    return 0;
}

static void lcd_enable_cursor(void) {
    lcd_stat.display_mode |= DISPLAYMODE_CUR;
    lcd_cmd(CMD_DISPLAYMODE | lcd_stat.display_mode);
}

static void lcd_disable_cursor(void) {
    lcd_stat.display_mode &= ~DISPLAYMODE_CUR;
    lcd_cmd(CMD_DISPLAYMODE | lcd_stat.display_mode);
}

static void lcd_enable_graphics(void) {
    lcd_stat.display_mode |= DISPLAYMODE_GRPH;
    lcd_cmd(CMD_DISPLAYMODE | lcd_stat.display_mode);
}

static void lcd_disable_graphics(void) {
    lcd_stat.display_mode &= ~DISPLAYMODE_GRPH;
    lcd_cmd(CMD_DISPLAYMODE | lcd_stat.display_mode);
}

static void lcd_enable_text(void) {
    lcd_stat.display_mode |= DISPLAYMODE_TEXT;
    lcd_cmd(CMD_DISPLAYMODE | lcd_stat.display_mode);
}

static void lcd_disable_text(void) {
    lcd_stat.display_mode &= ~DISPLAYMODE_TEXT;
    lcd_cmd(CMD_DISPLAYMODE | lcd_stat.display_mode);
}

static void lcd_enable_blink(void) {
    lcd_stat.display_mode |= DISPLAYMODE_BLK;
    lcd_cmd(CMD_DISPLAYMODE | lcd_stat.display_mode);
}

static void lcd_disable_blink(void) {
    lcd_stat.display_mode &= ~DISPLAYMODE_BLK;
    lcd_cmd(CMD_DISPLAYMODE | lcd_stat.display_mode);
}

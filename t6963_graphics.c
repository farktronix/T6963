#include "t6963.c"

static int lcd_major;

static char lcd_reset(unsigned char rows, unsigned char cols) {
    lcd_stat.cols=cols;
    lcd_stat.rows=rows;
    lcd_stat.text_base=0x02;
    lcd_stat.graphics_base=lcd_stat.text_base+rows*cols+(2*cols);
    lcd_stat.row_width=cols%8?cols+(8-(cols%8)):cols;

    if(LCD_DEBUG) {
        printk("t6963: graphics base: 0x%04x row width: 0x%02x\n", 
                lcd_stat.graphics_base, lcd_stat.row_width);
        printk("t6963: text base: 0x%04x row width: 0x%02x\n", lcd_stat.text_base, 
                lcd_stat.row_width);
    }

    if(lcd_cmd(CMD_AUTO_RESET)<0)
        return -1;

    lcd_disable_graphics();
    lcd_disable_text();
    lcd_disable_blink();
    lcd_disable_cursor();

    if(lcd_cmd_long(lcd_stat.graphics_base-2, CMD_GRAPHIC_HOME_ADDR)<0)
        return -1;
    if(lcd_cmd_d2(lcd_stat.row_width, 0, CMD_GRAPHIC_AREA_SET)<0)
        return -1;

    if(lcd_cmd_long(lcd_stat.text_base-2, CMD_TEXT_HOME_ADDR)<0)
        return -1;
    if(lcd_cmd_d2(cols, 0x00, CMD_TEXT_AREA_SET)<0)
        return -1;

    if(lcd_cmd(CMD_MODESET | MODESET_XOR)<0)
        return -1;
    
    if(lcd_graphics_clear()<0)
        return -1;
    if(lcd_text_clear()<0)
        return -1;

    if(lcd_cmd_long(lcd_stat.text_base,CMD_ADDR_PTR)<0) 
        return -1;

    if(lcd_cmd_long(lcd_stat.graphics_base,CMD_ADDR_PTR)<0) 
        return -1;

    lcd_enable_cursor();
    lcd_disable_blink();
    lcd_disable_text();
    lcd_enable_graphics();

    printk("t6963: reset complete\n");
    return 0;
}

ssize_t t6963_write(struct file *file, const char __user *buf, size_t count, loff_t *offset) {
    if(lcd_stat.entry_mode) {
       lcd_write_text(buf, count);
    } else {
        lcd_write_bytes(buf, count);
    }
    return count;
}

ssize_t t6963_read(struct file *file, char *buf, size_t count, loff_t *offset) {
    return lcd_read_bytes(buf, count);
}

int t6963_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
                unsigned long arg) {
    unsigned int addr;

    switch(cmd) {
        case T6963_RESET:
            lcd_reset(LCD_ROWS, LCD_COLS);
            break;
        case T6963_TEXT_ON:
            if(*((unsigned char*)arg)) {
                lcd_enable_text();
            } else {
                lcd_disable_text();
            }
            break;
        case T6963_GRAPHICS_ON:
            if(*((unsigned char*)arg)) {
                lcd_enable_graphics();
            } else {
                lcd_disable_graphics();
            }
            break;
        case T6963_TEXT_MODE:
            lcd_stat.entry_mode=1;
            break;
        case T6963_GRAPHICS_MODE:
            lcd_stat.entry_mode=0;
            break;
        case T6963_ADDR:
            copy_from_user(&addr, (unsigned int*)arg, 2);
            if(LCD_DEBUG>2)
                printk("address: 0x%04x\n", addr);
            lcd_cmd_long(addr, CMD_ADDR_PTR);
            break;
        case T6963_CLEAR_GRAPHICS:
            lcd_graphics_clear();
            break;
        case T6963_CLEAR_TEXT:
            lcd_text_clear();
            break;
        case T6963_GET_STATUS:
            copy_to_user((struct t6963_status*)arg, &lcd_stat, 
                    sizeof(struct t6963_status));    
            break;
        case T6963_SET_GRAPHICS_BASE:
            copy_from_user(&(lcd_stat.graphics_base), (unsigned int*)arg, 2);
            lcd_cmd_long(lcd_stat.graphics_base-2, CMD_GRAPHIC_HOME_ADDR); 
            break;
        case T6963_SET_TEXT_BASE:
            copy_from_user(&(lcd_stat.text_base), (unsigned int*)arg, 2);
            lcd_cmd_long(lcd_stat.text_base-2, CMD_TEXT_HOME_ADDR); 
            break;
    }
    return 0;
}

int t6963_close(struct inode *indoe, struct file *file) {
    return 0;
}

int t6963_open(struct inode *inode, struct file *file) {
    if(lcd_reset(LCD_ROWS, LCD_COLS)<0)
        printk("t6963: reset failed!\n");
    return 0;
}

static struct file_operations t6963_fops = {
    open: t6963_open,
    release: t6963_close,
    write: t6963_write,
    read: t6963_read,
    ioctl: t6963_ioctl,
};

int t6963_init(void) {
    if((lcd_major=register_chrdev(0, "t6963", &t6963_fops)) == -EBUSY) {
        printk("Can't register t6963 driver\n");
        return -EIO;
    }
    printk("t6963: init successful, major number %d\n", lcd_major);

    if(lcd_reset(LCD_ROWS, LCD_COLS)<0)
        printk("t6963: reset failed!\n");

    return 0;
}

void t6963_exit(void) {
    unregister_chrdev(lcd_major, "t6963");
    printk("t6963: driver unloaded\n");
}

module_init(t6963_init);
module_exit(t6963_exit);

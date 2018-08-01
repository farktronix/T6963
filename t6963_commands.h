#ifndef _T6963_COMMANDS
#define _T6963_COMMANDS

/* status bits returned from the T6963C */
#define STATUS_CMD      0x01
#define STATUS_RW       0x02
#define STATUS_AUTO_RD  0x04
#define STATUS_AUTO_WR  0x08
#define STATUS_RDY      0x20
#define STATUS_PEEK     0x40
#define STATUS_BLINK    0x80

/* commands */
#define CMD_TEXT_HOME_ADDR      0x40 // start of text memory area
#define CMD_TEXT_AREA_SET       0x41 // size of one text row in Bytes
#define CMD_GRAPHIC_HOME_ADDR   0x42 // start of graphics memory area
#define CMD_GRAPHIC_AREA_SET    0x43 // size of graphics row in Bytes

#define CMD_MODESET             0x80 // must be ORed with one of the following:
    #define MODESET_OR          0x00 
    #define MODESET_XOR         0x01
    #define MODESET_AND         0x02
    #define MODESET_TEXT        0x04
    // character generation, or'ed with the others 
    #define MODESET_CG          0x08 // 1 - RAM CG, 0 - ROM CG

#define CMD_DISPLAYMODE         0x90 //must be ORed with one or more of the following:
    #define DISPLAYMODE_GRPH    0x08 // graphics display on
    #define DISPLAYMODE_TEXT    0x04 // text display on
    #define DISPLAYMODE_CUR     0x02 // cursor displayed
    #define DISPLAYMODE_BLK     0x01 // cursor blink

#define CMD_ADDR_PTR            0x24 // RAM address pointer
#define CMD_WRITE_INCR          0xC0 // increase addr pointer after write
#define CMD_WRITE_NC            0xC4 // no change to addr pointer after write
#define CMD_WRITE_DEC           0xC2 // decrease addr pointer after write

#define CMD_READ_INCR           0xC1 // increase addr pointer after read
#define CMD_READ_NC             0xC5 // no change to addr pointer after read
#define CMD_READ_DEC            0xC3 // decrease adr pointer after read

#define CMD_AUTO_WRITE          0xB0 // auto data write
#define CMD_AUTO_READ           0xB1 // auto data read
#define CMD_AUTO_RESET          0xB2 // return from auto write/read

#define CMD_BIT_SET             0xF0 // set/reset a bit, ORed with the following and a
                                     // bit location given by the 3 LSb's
    #define BIT_SET             0x08 // set
    #define BIT_RESET           0x00 // reset

#define CMD_CURSOR              0xA0 // sets 1-8 line cursor, ORed with the number of
                                     // lines desired

#define CMD_CURSOR_POS          0x21 // sets the position of the cursor

// ioctl commands
enum {
    T6963_RESET,
    T6963_TEXT_ON,
    T6963_GRAPHICS_ON,
    T6963_TEXT_MODE,
    T6963_GRAPHICS_MODE,
    T6963_ADDR,
    T6963_SET_GRAPHICS_BASE,
    T6963_SET_TEXT_BASE,
    T6963_CLEAR_GRAPHICS,
    T6963_CLEAR_TEXT,
    T6963_GET_STATUS,
};

struct t6963_status {
    unsigned int rows; // height of display
    unsigned int cols; // width of display
    unsigned char row_width; // width of row in memory
    unsigned int graphics_base; // address of graphics section
    unsigned int text_base; // address of text section
    unsigned char display_mode; // CMD_DISPLAYMODE bitmask
    unsigned char status; // most recent LCD status
    unsigned char entry_mode; // 1- text entry, 0- graphics entry

};

#endif

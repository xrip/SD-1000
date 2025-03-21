#include <stdio.h>
#include "SMSlib.h"


#define ROM_NUMBER 5
#define ROM_NAME_LENGTH 31
// ROM-stored Menu data:
// 0xfff -- selected game index, write here to launch
// 0x1000 -- total menu items
// 0x1001 -- 31byte for each menu item with \0
volatile const __at(0x1000) unsigned char menu_item_num = ROM_NUMBER;
volatile const __at(0x1001) unsigned char menu_items[ROM_NAME_LENGTH * ROM_NUMBER] = {
    "Double Dragon                \0" 
    "Sonic                        \0" 
    "Terminator 2                 \0" 
    "Alien 3                      \0" 
    "Bram Stroker's Dracula       \0" 
    "Hang On SG-1000              \0" 
    "123456789012345678901234567 7\0" 
    "123456789012345678901234567 8\0" 
    "123456789012345678901234567 9\0" 
    "123456789012345678901234567 0\0"
};

volatile unsigned char __at(0xfff) selected;  // Write ROM index here to launch it from RP2040

// Current selected menu item
unsigned char selected_menu_item = 0;

void draw_menu(void) {
    

    for(unsigned char i = 0; i < menu_item_num; i++) {
        SMS_setNextTileatXY(0, 4 + i);  // Column 10, start at row 5
        if(i == selected_menu_item)
            SMS_print(">"); // Display selector
        else
            SMS_print(" "); // No selector

        SMS_printatXY(2, 4 + i, &menu_items[i*(ROM_NAME_LENGTH - 1)]);
    }                 
}

// VDP Interrupt handler for line interrupt
void irq_handler(void) __critical __interrupt {
    SMS_setBGPaletteColor(0, RGB(0, 0, 0));        // Red title
    SMS_setBGPaletteColor(1, RGB(3, 3, 3));        // White text
}

void main(void) {
    unsigned int keys, previous_keys = 0;

    /* Clear VRAM */
  SMS_VRAMmemsetW(0, 0x0000, 16384);

  SMS_autoSetUpTextRenderer();

    SMS_printatXY(4,0, "SMS Multicart by xrip");

	
    draw_menu();

    SMS_setLineCounter(8);
    SMS_setLineInterruptHandler (irq_handler);
    SMS_enableLineInterrupt();

    while(1) {
        SMS_setBGPaletteColor(0, RGB(1, 1, 1));        // Red title
	SMS_setBGPaletteColor(1, RGB(3, 3, 0));        // Title color


        keys = SMS_getKeysStatus();

        // Up button pressed
        if ((keys & PORT_A_KEY_UP) && !(previous_keys & PORT_A_KEY_UP)) {
            if(selected_menu_item == 0)
                selected_menu_item = menu_item_num - 1;
            else
                selected_menu_item--;
            draw_menu();
        }

        // Down button pressed
        if ((keys & PORT_A_KEY_DOWN) && !(previous_keys & PORT_A_KEY_DOWN)) {
            selected_menu_item = (selected_menu_item + 1) % menu_item_num;
            draw_menu();
        }

        if ((keys & PORT_A_KEY_START) && !(previous_keys & PORT_A_KEY_START)) {
		selected = selected_menu_item;
	   
	}
        previous_keys = keys;


        SMS_waitForVBlank();
    }
}

SMS_EMBED_SEGA_ROM_HEADER(9999,0);
SMS_EMBED_SDSC_HEADER_AUTO_DATE(1,0,"xrip","Multicartridge","SMS Multicartridge by xrip");

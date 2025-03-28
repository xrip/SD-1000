#include "SGlib.h"
#include "font.h"
#include "sega_logo.h"

/* macros for SEGA and SDSC headers */
#define SMS_BYTE_TO_BCD(n) (((n)/10)*16+((n)%10))

#define SMS_EMBED_SEGA_ROM_HEADER_16KB_REGION_CODE  0x4B
#define SMS_EMBED_SEGA_ROM_HEADER_16KB(productCode,revision)                                   \
 const __at (0x3ff0) unsigned char __SMS__SEGA_signature[16]={'T','M','R',' ','S','E','G','A', \
                                                                          0xFF,0xFF,0xFF,0xFF, \
                  SMS_BYTE_TO_BCD((productCode)%100),SMS_BYTE_TO_BCD(((productCode)/100)%100), \
      (((productCode)/10000)<<4)|((revision)&0x0f),SMS_EMBED_SEGA_ROM_HEADER_16KB_REGION_CODE}


volatile unsigned char __at(0xfff) selected;  // Write ROM index here to launch it from RP2040

#define ROM_NUMBER 18
#define ROM_NAME_LENGTH 31
// ROM-stored Menu data:
// 0xfff -- selected game index, write here to launch
// 0x1000 -- total menu items
// 0x1001 -- 31byte for each menu item with \0
const __at(0x4000) unsigned char menu_item_num = ROM_NUMBER;
const __at(0x4001) unsigned char menu_items[16384] = { 0 };
/*const char *menu_items[ROM_NUMBER] = {
	"Double Dragon",
	"Sonic",
	"Terminator 2",
	"Alien 3",
	"Bram Stroker's Dracula",
	"Hang On SG-1000",
	"123456789012345678901234567 7",
	"123456789012345678901234567 8",
	"123456789012345678901234567 9",
	"123456789012345678901234567 0",
	"12345678901234567890123456 11",
	"12345678901234567890123456 12",
	"12345678901234567890123456 13",
	"12345678901234567890123456 14",
	"12345678901234567890123456 15",
	"12345678901234567890123456 16",
	"12345678901234567890123456 17",
	"12345678901234567890123456 18"
};*/

const unsigned char sprite_pointer[16] = {
	0b00001000, //     *
	0b00001100, //     **
	0b00001110, //     ***
	0b11111111, //   ******
	0b00001110, //     ***
	0b00001100, //     **
	0b00001000, //     *
	0b00000000  //
};

void SG_print (const unsigned char *str) {
	while (*str) {
		SG_setTile(*str++);
	}
}

#define SG_printatXY(x,y, str) do{SG_setNextTileatXY(x,y); SG_print(str);}while(0)

static void draw_pointer(unsigned char y) {
	SG_initSprites();
	SG_addSprite(4,(8*4)+y*8,0, SG_COLOR_CYAN);
	SG_finalizeSprites();
	SG_copySpritestoSAT();
}


static void setup() {
	SG_VRAMmemsetW(0, 0x0000, 8192);
	SG_setBackdropColor(SG_COLOR_BLACK);

	SG_loadTilePatterns(sms_sg1000_cart_logo, 1, 192);

	SG_loadTilePatterns(devkitSMS_font__tiles__1bpp, 32, sizeof(devkitSMS_font__tiles__1bpp));
	SG_loadTilePatterns(devkitSMS_font__tiles__1bpp, 32+0x100, sizeof(devkitSMS_font__tiles__1bpp));
	SG_loadTilePatterns(devkitSMS_font__tiles__1bpp, 32+0x200, sizeof(devkitSMS_font__tiles__1bpp));

// SG_loadTileColours but not uses rom
	SG_VRAMmemset(0x2000, SG_COLOR_LIGHT_BLUE << 4, 224);
	SG_VRAMmemset(0x2000+224, 0xF1, (1920*3) - 224);

	SG_loadSpritePatterns(sprite_pointer, 0	, 8);


	for (int i =0; i < menu_item_num; i++) {
		SG_printatXY(2, 4 + i, &menu_items[i*31]);
	}

	draw_pointer(0);

	// Draw SEGA logo
	for (int y = 0; y < 3; y++) {
		for (int x = 0; x < 8; x++) {
			SG_setTileatXY(x + 1, y, y * 8 + (x + 1));
		}
	}

	SG_printatXY(10, 0, "MARK III/SMS/SG-1000");
	SG_printatXY(15, 1,     "multicart");
	SG_printatXY(16, 2,      "by xrip");

	SG_printatXY(3, 23, "<< prev page $ next page >> ");

	SG_displayOn() ;
}


void main(void) {
	setup();

	unsigned int keys, previous_keys = 0;
	unsigned int selected_menu_item = 0;



while (1) {
	keys = SG_getKeysStatus();

	// Up button pressed
	if ((keys & PORT_A_KEY_UP) && !(previous_keys & PORT_A_KEY_UP)) {
		if(selected_menu_item == 0)
			selected_menu_item = menu_item_num - 1;
		else
			selected_menu_item--;

		draw_pointer(selected_menu_item);
	}

	// Down button pressed
	if ((keys & PORT_A_KEY_DOWN) && !(previous_keys & PORT_A_KEY_DOWN)) {
		selected_menu_item = (selected_menu_item + 1) % menu_item_num;
		draw_pointer(selected_menu_item);
	}

	if ((keys & PORT_A_KEY_START) && !(previous_keys & PORT_A_KEY_START)) {
		selected = selected_menu_item;

	}
	previous_keys = keys;



	SG_waitForVBlank ();
}
	    
}

SMS_EMBED_SEGA_ROM_HEADER_16KB(9999,0);

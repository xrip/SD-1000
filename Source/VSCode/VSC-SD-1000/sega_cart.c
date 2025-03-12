/*
SD-1000 MultiCART by Andrea Ottaviani 2024
SEGA SC-3000 - SG-1000  multicart based on Raspberry Pico board
*/


#include <stdlib.h>
#include <pico/multicore.h>
#include <hardware/gpio.h>
#include <hardware/clocks.h>

#include "tusb.h"
#include "ff.h"
#include "fatfs_disk.h"

#include "sg1000_menu_rom.h"

// Pico pin usage definitions

#define A0_PIN    0
#define A1_PIN    1
#define A2_PIN    2
#define A3_PIN    3
#define A4_PIN    4
#define A5_PIN    5
#define A6_PIN    6
#define A7_PIN    7
#define A8_PIN    8
#define A9_PIN    9
#define A10_PIN  10
#define A11_PIN  11
#define A12_PIN  12
#define A13_PIN  13
#define A14_PIN  14
#define A15_PIN  15
#define D0_PIN   16
#define D1_PIN   17
#define D2_PIN   18
#define D3_PIN   19
#define D4_PIN   20
#define D5_PIN   21
#define D6_PIN   22
#define D7_PIN   23
#define MEMR_PIN  24
#define MEMW_PIN  25
#define MREQ_PIN  26
#define CEROM2_PIN  27
#define DSRAM_PIN  28
#define IOR_PIN  29

// Pico pin usage masks

#define A0_PIN_MASK     0x00000001L
#define A1_PIN_MASK     0x00000002L
#define A2_PIN_MASK     0x00000004L
#define A3_PIN_MASK     0x00000008L
#define A4_PIN_MASK     0x00000010L
#define A5_PIN_MASK     0x00000020L
#define A6_PIN_MASK     0x00000040L
#define A7_PIN_MASK     0x00000080L
#define A8_PIN_MASK     0x00000100L
#define A9_PIN_MASK     0x00000200L
#define A10_PIN_MASK    0x00000400L
#define A11_PIN_MASK    0x00000800L
#define A12_PIN_MASK    0x00001000L
#define A13_PIN_MASK    0x00002000L
#define A14_PIN_MASK    0x00004000L
#define A15_PIN_MASK    0x00008000L
#define D0_PIN_MASK     0x00010000L
#define D1_PIN_MASK     0x00020000L
#define D2_PIN_MASK     0x00040000L
#define D3_PIN_MASK     0x00080000L
#define D4_PIN_MASK     0x00100000L
#define D5_PIN_MASK     0x00200000L  // gpio 21
#define D6_PIN_MASK     0x00400000L
#define D7_PIN_MASK     0x00800000L

#define MEMR_PIN_MASK   0x01000000L //gpio 24
#define MEMW_PIN_MASK   0x02000000L
#define MREQ_PIN_MASK   0x04000000L  //gpio 26
#define CEROM2_PIN_MASK 0x08000000L
#define DSRAM_PIN_MASK  0x10000000L
#define IOR_PIN_MASK    0x20000000L

// Aggregate Pico pin usage masks
#define ALL_GPIO_MASK  	0x3FFFFFFFL
#define BUS_PIN_MASK    0x0000FFFFL
#define DATA_PIN_MASK   0x00FF0000L
#define FLAG_MASK       0x2F000000L
#define ROM_MASK ( MREQ_PIN_MASK  )
#define ALWAYS_IN_MASK  (BUS_PIN_MASK | FLAG_MASK)
#define ALWAYS_OUT_MASK (DATA_PIN_MASK | DOUTE_PIN_MASK)

#define SET_DATA_MODE_OUT   gpio_set_dir_out_masked(DATA_PIN_MASK)
#define SET_DATA_MODE_IN    gpio_set_dir_in_masked(DATA_PIN_MASK)
// We're going to erase and reprogram a region 256k from the start of flash.
// Once done, we can access this at XIP_BASE + 256k.


#define ROM_SIZE  65536L
unsigned char ROM[ROM_SIZE];
unsigned char files[256 * 256] = {0};
unsigned char file_names[32 * 25] = {0};
char current_path[1024] = "\0";
char path[1024];
unsigned int file_start_idx = 0, file_end_idx = 0;


/*
 Theory of Operation
 -------------------
 sega sends command to mcu on cart by writing to 50000 (CMD), 50001 (parameter) and menu (50002-50641)
 sega must be running from RAM when it sends a command, since the mcu on the cart will
 go away at that point. Sega polls 50001 until it reads $1.
*/
// Memory locations for communication
#define CMD_ADDR        30000
#define PARAM_ADDR      30001
#define FILELIST_ADDR   30002
#define FILETYPE_ADDR   31000
#define FILEPAGE_START  31030
#define ACK_ADDR        29999

void __not_in_flash_func(core1_main()) {
    multicore_lockout_victim_init();

    gpio_set_dir_in_masked(ALWAYS_IN_MASK);
    // Initial conditions
    SET_DATA_MODE_IN;

    while (1) {
        while (gpio_get_all() & MREQ_PIN_MASK); //memr = b5 mreq=b10
        const uint32_t pins = gpio_get_all(); // re-read for SG-1000;
        const uint32_t address = pins & BUS_PIN_MASK;
        if (!(pins & MEMR_PIN_MASK)) {
            SET_DATA_MODE_OUT;
            gpio_put_masked(DATA_PIN_MASK, ROM[address] << 16);
            //   while (!(gpio_get_all() & MEMR_PIN_MASK));
            SET_DATA_MODE_IN;
        } else if (!(pins & MEMW_PIN_MASK)) {
            ROM[address] = (gpio_get_all() & DATA_PIN_MASK) >> 16;
            // while (!(gpio_get_all() & MEMW_PIN_MASK));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////
//                     MENU Reset
////////////////////////////////////////////////////////////////////////////////////

void reset_sg1000() {
    multicore_lockout_start_blocking();

    while (!(gpio_get_all() & MEMR_PIN_MASK));
    SET_DATA_MODE_OUT;
    gpio_put_masked(DATA_PIN_MASK, 0xc7 << 16);
    while (!(gpio_get_all() & MEMR_PIN_MASK));
    SET_DATA_MODE_IN;

    while (!(gpio_get_all() & MEMR_PIN_MASK));
    SET_DATA_MODE_OUT;
    gpio_put_masked(DATA_PIN_MASK, 0xc7 << 16);
    while (!(gpio_get_all() & MEMR_PIN_MASK));
    SET_DATA_MODE_IN;
    multicore_lockout_end_blocking();


    while (!(gpio_get_all() & MEMR_PIN_MASK));
    SET_DATA_MODE_OUT;
    gpio_put_masked(DATA_PIN_MASK, 0xc7 << 16);
    while (!(gpio_get_all() & MEMR_PIN_MASK));
    SET_DATA_MODE_IN;
}


////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    char isDir;
    char filename[13];
    char long_filename[32];
    char full_path[210];
} DIR_ENTRY; // 256 bytes = 256 entries in 64k

unsigned int num_dir_entries = 0; // how many entries in the current directory

/**
 * Compare function for directory entries sorting
 */
static int entry_compare(const void *p1, const void *p2) {
    const DIR_ENTRY *e1 = (DIR_ENTRY *) p1;
    const DIR_ENTRY *e2 = (DIR_ENTRY *) p2;

    // Directories come first
    if (e1->isDir && !e2->isDir) return -1;
    if (!e1->isDir && e2->isDir) return 1;

    // Then sort alphabetically
    return strcasecmp(e1->long_filename, e2->long_filename);
}

static char *get_filename_ext(char *filename) {
    char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}


/**
 * Check if file is a valid ROM
 */
static int is_valid_file(const char *filename) {
    const char *ext = get_filename_ext((char *) filename);
    return strcasecmp(ext, "BIN") == 0 ||
           strcasecmp(ext, "ROM") == 0 ||
           strcasecmp(ext, "SMS") == 0 ||
           strcasecmp(ext, "SG") == 0 ||
           strcasecmp(ext, "SC") == 0;
}

/**
 * Read directory contents
 */
static unsigned int read_directory(const char *path) {
    FILINFO fno;
    num_dir_entries = 0;
    DIR_ENTRY *dst = (DIR_ENTRY *) &files[0];

    if (!fatfs_is_mounted()) {
        mount_fatfs_disk();
    }

    FATFS FatFs;
    if (f_mount(&FatFs, "", 1) != FR_OK) {
        return 0;
    }

    DIR dir;
    if (f_opendir(&dir, path) != FR_OK) {
        f_mount(0, "", 1);
        return 0;
    }

    // Read directory entries
    while (num_dir_entries < 255) {
        if (f_readdir(&dir, &fno) != FR_OK || fno.fname[0] == 0) {
            break;
        }

        // Skip hidden and system files
        if (fno.fattrib & (AM_HID | AM_SYS)) {
            continue;
        }

        // Add directories and valid files
        dst->isDir = (fno.fattrib & AM_DIR) ? 1 : 0;
        if (!dst->isDir && !is_valid_file(fno.fname)) {
            continue;
        }

        // Copy filename information
        strncpy(dst->long_filename, fno.fname, sizeof(dst->long_filename) - 1);
        dst->long_filename[sizeof(dst->long_filename) - 1] = 0;

        // Copy 8.3 filename
        if (fno.altname[0]) {
            strncpy(dst->filename, fno.altname, sizeof(dst->filename) - 1);
        } else {
            strncpy(dst->filename, fno.fname, sizeof(dst->filename) - 1);
        }
        dst->filename[sizeof(dst->filename) - 1] = 0;

        dst->full_path[0] = 0;
        dst++;
        num_dir_entries++;
    }

    f_closedir(&dir);
    f_mount(0, "", 1);

    // Sort entries
    qsort((DIR_ENTRY *) &files[0], num_dir_entries, sizeof(DIR_ENTRY), entry_compare);

    return 1;
}

/**
 * Load ROM file into memory
 */
static uint32_t load_file(const char *filename) {
    FATFS FatFs;
    FIL fil;
    UINT bytes_read = 0;

    if (f_mount(&FatFs, "", 1) != FR_OK) {
        return 0;
    }

    if (f_open(&fil, filename, FA_READ) != FR_OK) {
        f_mount(0, "", 1);
        return 0;
    }

    // Read file into buffer
    uint8_t *dst = &files[0];
    const uint32_t bytes_to_read = ROM_SIZE;

    if (f_read(&fil, dst, bytes_to_read, &bytes_read) != FR_OK) {
        bytes_read = 0;
    }

    f_close(&fil);
    f_mount(0, "", 1);

    return bytes_read;
}

////////////////////////////////////////////////////////////////////////////////////
//                     filelist
////////////////////////////////////////////////////////////////////////////////////

void update_file_list() {
    const DIR_ENTRY *entries = (DIR_ENTRY *) &files[0];
    memset(&ROM[FILELIST_ADDR], 0, 32 * 20);

    for (int n = 0; n < file_end_idx - file_start_idx; n++) {
        const DIR_ENTRY *entry = &entries[n + file_start_idx];
        char display_name[32] = {0};

        if (entry->isDir) {
            strcpy(display_name, "DIR> ");
            strcat(display_name, entry->long_filename);
            ROM[FILETYPE_ADDR + n] = 1;
        } else {
            strcpy(display_name, entry->long_filename);
            ROM[FILETYPE_ADDR + n] = 0;
        }
        for (int i = 0; i < 31; i++) {
            const char c = display_name[i];
            ROM[FILELIST_ADDR + i + n * 32] = c <= 20 ? 32 : c;
        }
        // Also store in file_names buffer
        memcpy(&file_names[n * 32], display_name, 32);
    }

    ROM[FILEPAGE_START] = file_start_idx;
    ROM[FILEPAGE_START + 1] = file_end_idx;
    ROM[FILEPAGE_START + 2] = num_dir_entries;
}

enum commands {
    READ_DIRECTORY = 1,
    LOAD_GAME,
    NEXT_PAGE,
    PREV_PAGE,
    PARENT_DIRECTORY,
};

////////////////////////////////////////////////////////////////////////////////////
//                     Directory Up
////////////////////////////////////////////////////////////////////////////////////
static inline void change_directory_up() {
    uint16_t length = strlen(current_path);
    if (length > 0) {
        while (length && current_path[--length] != '/');
        current_path[length] = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////
//                     LOAD Game
////////////////////////////////////////////////////////////////////////////////////
static inline void load_rom() {
    const unsigned int current = ROM[PARAM_ADDR] + file_start_idx - 1;

    const DIR_ENTRY *entries = (DIR_ENTRY *) &files[0];

    if (entries[current].isDir) {
        // directory
        strcat(current_path, "/");
        strcat(current_path, entries[current].filename);
        ROM[CMD_ADDR] = READ_DIRECTORY; // re-read dir, path is changed
    } else {
        memset(path, 0, sizeof(path));
        strcat(path, current_path);
        strcat(path, "/");
        strcat(path, entries[current].filename);

        // Set path in ROM
        strncpy((char *) &ROM[FILELIST_ADDR], path, sizeof(path));

        ROM[CMD_ADDR] = PARENT_DIRECTORY;
        sleep_ms(540);
        reset_sg1000();
        load_file(path); // load rom in files[]
        reset_sg1000();
        memcpy(ROM, files, sizeof(ROM));
        reset_sg1000();
        while (1);
    }
}

////////////////////////////////////////////////////////////////////////////////////
//                     Sega Cart Main
////////////////////////////////////////////////////////////////////////////////////

static void sega_menu(const enum commands command) {
    ROM[CMD_ADDR] = 0;
    switch (command) {
        case PARENT_DIRECTORY:
            change_directory_up();
        case READ_DIRECTORY:
            read_directory(current_path);

            file_start_idx = 0;
            file_end_idx = num_dir_entries < 20 ? num_dir_entries : 20;

            update_file_list();
            break;
        case NEXT_PAGE:
            if (file_end_idx < num_dir_entries) {
                unsigned int maxfiles = 20;
                if (file_end_idx + maxfiles > num_dir_entries) maxfiles = num_dir_entries - file_end_idx;
                file_start_idx = file_end_idx;
                file_end_idx = file_start_idx + maxfiles;
                update_file_list();
            }
            break;
        case PREV_PAGE:
            if (file_start_idx >= 20) {
                file_start_idx -= 20;
                file_end_idx = file_start_idx + 20;

                update_file_list();
            }
            break;
        case LOAD_GAME:
            load_rom();
            break;
    }
    ROM[ACK_ADDR] = 1;
}

void sega_cart_main() {
    set_sys_clock_khz(250000, true);


    gpio_init_mask(ALL_GPIO_MASK);

    gpio_init(DSRAM_PIN);
    gpio_set_dir(DSRAM_PIN, GPIO_OUT);
    gpio_put(DSRAM_PIN, true);

    memcpy(ROM, SG1000_MENU_ROM, sizeof(SG1000_MENU_ROM));

    multicore_launch_core1(core1_main);

    // Initial conditions
    while (1) {
        sega_menu(ROM[CMD_ADDR]);
    }
    __unreachable();
}

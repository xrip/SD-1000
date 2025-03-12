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
unsigned char nomefiles[32 * 25] = {0};
char current_path[1024] = "\0";
char path[1024];
unsigned int fileda = 0, filea = 0;


/*
 Theory of Operation
 -------------------
 sega sends command to mcu on cart by writing to 50000 (CMD), 50001 (parameter) and menu (50002-50641)
 sega must be running from RAM when it sends a command, since the mcu on the cart will
 go away at that point. Sega polls 50001 until it reads $1.
*/

void __not_in_flash_func(core1_main()) {
    char dataWrite = 0;

    multicore_lockout_victim_init();


    gpio_set_dir_in_masked(ALWAYS_IN_MASK);
    // Initial conditions
    SET_DATA_MODE_IN;

    while (1) {
        while (gpio_get_all() & MREQ_PIN_MASK); //memr = b5 mreq=b10
        const uint32_t pins = gpio_get_all(); // re-read for SG-1000;
        const uint32_t addr = pins & BUS_PIN_MASK;
        if (!(pins & MEMR_PIN_MASK)) {
            SET_DATA_MODE_OUT;
            gpio_put_masked(DATA_PIN_MASK, ROM[addr] << 16);
            //   while (!(gpio_get_all() & MEMR_PIN_MASK));
            SET_DATA_MODE_IN;
        } else if (!(pins & (MEMW_PIN_MASK))) {
            dataWrite = ((gpio_get_all() & DATA_PIN_MASK) >> 16);
            ROM[addr] = dataWrite;
            // while (!(gpio_get_all() & MEMW_PIN_MASK));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////
//                     MENU Reset
////////////////////////////////////////////////////////////////////////////////////

void reset() {
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

int num_dir_entries = 0; // how many entries in the current directory

static int entry_compare(const void *p1, const void *p2) {
    const DIR_ENTRY *e1 = (DIR_ENTRY *) p1;
    const DIR_ENTRY *e2 = (DIR_ENTRY *) p2;
    if (e1->isDir && !e2->isDir) return -1;
    if (!e1->isDir && e2->isDir) return 1;
    return strcasecmp(e1->long_filename, e2->long_filename);
}

static char *get_filename_ext(char *filename) {
    char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

static int is_valid_file(char *filename) {
    const char *ext = get_filename_ext(filename);
    if (strcasecmp(ext, "BIN") == 0 || strcasecmp(ext, "ROM") == 0
        || strcasecmp(ext, "SMS") == 0
        || strcasecmp(ext, "SG") == 0 || strcasecmp(ext, "SC") == 0)
        return 1;
    return 0;
}

static int read_directory(const char *path) {
    FILINFO fno;
    int ret = 0;
    num_dir_entries = 0;
    DIR_ENTRY *dst = (DIR_ENTRY *) &files[0];

    if (!fatfs_is_mounted())
        mount_fatfs_disk();

    FATFS FatFs;
    if (f_mount(&FatFs, "", 1) == FR_OK) {
        DIR dir;
        if (f_opendir(&dir, path) == FR_OK) {
            while (num_dir_entries < 255) {
                if (f_readdir(&dir, &fno) != FR_OK || fno.fname[0] == 0)
                    break;
                if (fno.fattrib & (AM_HID | AM_SYS))
                    continue;
                dst->isDir = fno.fattrib & AM_DIR ? 1 : 0;
                if (!dst->isDir)
                    if (!is_valid_file(fno.fname)) continue;
                // copy file record to first ram block
                // long file name
                strncpy(dst->long_filename, fno.fname, 31);
                dst->long_filename[31] = 0;
                // 8.3 name
                if (fno.altname[0])
                    strcpy(dst->filename, fno.altname);
                else {
                    // no altname when lfn is 8.3
                    strncpy(dst->filename, fno.fname, 12);
                    dst->filename[12] = 0;
                }
                dst->full_path[0] = 0; // path only for search results
                dst++;
                num_dir_entries++;
            }
            f_closedir(&dir);
        } // else strcpy(errorBuf, "Can't read directory");
        f_mount(0, "", 1);
        qsort((DIR_ENTRY *) &files[0], num_dir_entries, sizeof(DIR_ENTRY), entry_compare);
        ret = 1;
    } // else strcpy(errorBuf, "Can't read flash memory");
    return ret;
}


/* load file in  ROM */

static unsigned int load_file(char *filename) {
    FATFS FatFs;
    UINT br, size = 0;


    if (f_mount(&FatFs, "", 1) != FR_OK) {
        // strcpy(errorBuf, "Can't read flash memory");
        return 0;
    }
    FIL fil;
    if (f_open(&fil, filename, FA_READ) != FR_OK) {
        // strcpy(errorBuf, "Can't open file");
        goto cleanup;
    }


    // set a default error
    // strcpy(errorBuf, "Can't read file");

    unsigned char *dst = &files[0];
    const int bytes_to_read = 64 * 1024;
    // read the file to SRAM
    if (f_read(&fil, dst, bytes_to_read, &br) != FR_OK) {
        goto closefile;
    }
    size += br;


closefile:
    f_close(&fil);
cleanup:
    f_mount(0, "", 1);

    return br;
}

////////////////////////////////////////////////////////////////////////////////////
//                     filelist
////////////////////////////////////////////////////////////////////////////////////

void filelist(const DIR_ENTRY *en, const unsigned int da, const unsigned int a) {
    char longfilename[32];

    for (int i = 0; i < 32 * 20; i++) ROM[50002 + i] = 0;
    for (int n = 0; n < (a - da); n++) {
        memset(longfilename, 0, 32);

        if (en[n + da].isDir) {
            strcpy(longfilename, "DIR->");
            ROM[51000 + n] = 1;
            strcat(longfilename, en[n + da].long_filename);
        } else {
            ROM[51000 + n] = 0;
            strcpy(longfilename, en[n + da].long_filename);
        }
        for (int i = 0; i < 31; i++) {
            ROM[50002 + i + (n * 32)] = longfilename[i];
            if ((ROM[50002 + i + (n * 32)]) <= 20) ROM[50002 + i + (n * 32)] = 32;
        }
        strcpy((char *) &nomefiles[32 * n], longfilename);
    }
    ROM[51030] = da;
    ROM[51031] = a;
    ROM[51032] = num_dir_entries;
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
static inline void DirUp() {
    unsigned int length = strlen(current_path);
    if (length > 0) {
        while (length && current_path[--length] != '/');
        current_path[length] = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////
//                     LOAD Game
////////////////////////////////////////////////////////////////////////////////////
static inline void LoadGame() {
    char longfilename[31];
    const unsigned int numfile = ROM[50001] + fileda - 1;

    const DIR_ENTRY *entry = (DIR_ENTRY *) &files[0];

    strcpy(longfilename, entry[numfile].filename);
    if (entry[numfile].isDir) {
        // directory
        strcat(current_path, "/");
        strcat(current_path, entry[numfile].filename);
        ROM[50000] = READ_DIRECTORY; // re-read dir, path is changed
    } else {
        memset(path, 0, sizeof(path));
        strcat(path, current_path);
        strcat(path, "/");
        strcat(path, longfilename);
        for (int i = 0; i < sizeof(path); i++) ROM[50002 + i] = path[i];
        ROM[50000] = PARENT_DIRECTORY;
        sleep_ms(540);
        reset();
        load_file(path); // load rom in files[]
        reset();
        memcpy(ROM, files, sizeof(ROM));
        reset();
        while (1);
    }
}

////////////////////////////////////////////////////////////////////////////////////
//                     Sega Cart Main
////////////////////////////////////////////////////////////////////////////////////

static void SEGAMenu(const enum commands command) {
    unsigned int maxfiles = 20;

    ROM[50000] = 0;
    switch (command) {
        case PARENT_DIRECTORY: DirUp();
        case READ_DIRECTORY:
            read_directory(current_path);
            fileda = 0;
            if (maxfiles > num_dir_entries) maxfiles = num_dir_entries;
            filea = fileda + maxfiles;
            filelist((DIR_ENTRY *) &files[0], fileda, filea);
            break;
        case NEXT_PAGE:
            if (filea < num_dir_entries) {
                if (filea + maxfiles > num_dir_entries) maxfiles = num_dir_entries - filea;
                fileda = filea;
                filea = fileda + maxfiles;
                filelist((DIR_ENTRY *) &files[0], fileda, filea);
            }
            break;
        case PREV_PAGE:
            if (fileda >= 20) {
                fileda = fileda - 20;
                filea = fileda + 20;
                filelist((DIR_ENTRY *) &files[0], fileda, filea);
            }
            break;
        case LOAD_GAME:
            LoadGame();
            break;
    }
    ROM[49999] = 1;
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
        SEGAMenu(ROM[50000]);
    }
    __unreachable();
}

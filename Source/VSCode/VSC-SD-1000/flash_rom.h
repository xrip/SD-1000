#pragma once
#include "flash_rom1.h"
#include "flash_rom2.h"
#include "flash_rom3.h"
#include "flash_rom4.h"
#include "flash_rom5.h"
#include "flash_rom6.h"

// Define an array of pointers to the ROM arrays
const uint8_t * const flash_roms[] = {
    flash_rom1,
    flash_rom2,
    flash_rom3,
    flash_rom4,
    flash_rom5,
    flash_rom6,
};
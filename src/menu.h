#ifndef _MENU_H
#define _MENU_H

#include <cx16.h>
#include <cbm.h>
#include <ascii_charmap.h>
#include <stdint.h>
#include "constants.h"

void menu_settext(uint8_t x, uint8_t y, const char* str);
void menu_centertext(uint8_t y, const char* str);
uint8_t menu_select(uint8_t initial_mode);

#endif // _MENU_H

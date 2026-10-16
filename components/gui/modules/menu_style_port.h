#pragma once

#include <stdint.h>

/* Numeric values intentionally match MomentumSettings.MenuStyle.  This small
   adapter avoids including two different MenuStyle typedefs in one unit. */
void menu_set_momentum_style(uint32_t style);
uint32_t menu_get_momentum_style(void);

/**
 * @file menu.h
 * GUI: Menu view module API
 */

#pragma once

#include "lock_screen_style.h"

#include "../view.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Menu anonymous structure */
typedef struct Menu Menu;

/** Menu Item Callback */
typedef void (*MenuItemCallback)(void* context, uint32_t index);

/** Menu allocation and initialization
 *
 * @return     Menu instance
 */
Menu* menu_alloc(void);

/** Free menu
 *
 * @param      menu  Menu instance
 */
void menu_free(Menu* menu);

/** Get Menu view
 *
 * @param      menu  Menu instance
 *
 * @return     View instance
 */
View* menu_get_view(Menu* menu);

/** Add item to menu
 *
 * @param      menu      Menu instance
 * @param      label     menu item string label
 * @param      icon      IconAnimation instance
 * @param      index     menu item index
 * @param      callback  MenuItemCallback instance
 * @param      context   pointer to context
 */
void menu_add_item(
    Menu* menu,
    const char* label,
    const Icon* icon,
    uint32_t index,
    MenuItemCallback callback,
    void* context);

/** Clean menu
 * @note       this function does not free menu instance
 *
 * @param      menu  Menu instance
 */
void menu_reset(Menu* menu);

/** Set current menu item
 *
 * @param      menu   Menu instance
 * @param      index  The index
 */
void menu_set_selected_item(Menu* menu, uint32_t index);

/** Menu render styles. Values must stay identical to Momentum-Firmware. */
#ifndef MOMENTUM_MENU_STYLE_DEFINED
#define MOMENTUM_MENU_STYLE_DEFINED
typedef enum {
    MenuStyleList,
    MenuStyleWii,
    MenuStyleDsi,
    MenuStylePs4,
    MenuStyleVertical,
    MenuStyleC64,
    MenuStyleCompact,
    MenuStyleMNTM,
    MenuStyleCoverFlow,
    MenuStyleCount,
} MenuStyle;
#endif

/** Set current menu render style (persisted).
 *
 * @param      style  MenuStyle to apply
 */
void menu_set_style(MenuStyle style);

/** Get current menu render style.
 *
 * @return     active MenuStyle
 */
MenuStyle menu_get_style(void);

#ifdef __cplusplus
}
#endif

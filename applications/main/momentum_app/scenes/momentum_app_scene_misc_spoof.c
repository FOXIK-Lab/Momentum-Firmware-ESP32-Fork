#include "../momentum_app.h"

enum VarItemListIndex {
    VarItemListIndexFlipperName, // TODO: Split into name, mac, serial
    VarItemListIndexShellColor,
};

const char* const shell_color_names[FuriHalVersionColorCount] = {
    "Real",
    "Black",
    "White",
    "Transparent",
};
static void momentum_app_scene_misc_spoof_shell_color_changed(VariableItem* item) {
    MomentumApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, shell_color_names[index]);
    momentum_settings.spoof_color = index;
    furi_hal_version_set_hw_color((FuriHalVersionColor)index);
    app->save_color = true;
    app->save_settings = true;
}

void momentum_app_scene_misc_spoof_var_item_list_callback(void* context, uint32_t index) {
    MomentumApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void momentum_app_scene_misc_spoof_on_enter(void* context) {
    MomentumApp* app = context;
    VariableItemList* var_item_list = app->var_item_list;
    VariableItem* item;

    item = variable_item_list_add(var_item_list, "Flipper Name", 0, NULL, app);
    variable_item_set_current_value_text(item, app->device_name);

    item = variable_item_list_add(
        var_item_list,
        "Shell Color",
        FuriHalVersionColorCount,
        momentum_app_scene_misc_spoof_shell_color_changed,
        app);
    uint8_t current_color = furi_hal_version_get_hw_color();
    if(current_color >= FuriHalVersionColorCount) current_color = FuriHalVersionColorUnknown;
    momentum_settings.spoof_color = current_color;
    variable_item_set_current_value_index(item, current_color);
    variable_item_set_current_value_text(item, shell_color_names[current_color]);

    variable_item_list_set_enter_callback(
        var_item_list, momentum_app_scene_misc_spoof_var_item_list_callback, app);

    variable_item_list_set_selected_item(
        var_item_list,
        scene_manager_get_scene_state(app->scene_manager, MomentumAppSceneMiscSpoof));

    view_dispatcher_switch_to_view(app->view_dispatcher, MomentumAppViewVarItemList);
}

bool momentum_app_scene_misc_spoof_on_event(void* context, SceneManagerEvent event) {
    MomentumApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        scene_manager_set_scene_state(app->scene_manager, MomentumAppSceneMiscSpoof, event.event);
        consumed = true;
        switch(event.event) {
        case VarItemListIndexFlipperName:
            scene_manager_next_scene(app->scene_manager, MomentumAppSceneMiscSpoofName);
            break;
        default:
            break;
        }
    }

    return consumed;
}

void momentum_app_scene_misc_spoof_on_exit(void* context) {
    MomentumApp* app = context;
    variable_item_list_reset(app->var_item_list);
}

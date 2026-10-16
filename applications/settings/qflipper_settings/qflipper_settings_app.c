#include <furi.h>
#include <desktop/desktop.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/variable_item_list.h>

typedef struct {
    Gui* gui;
    Desktop* desktop;
    ViewDispatcher* view_dispatcher;
    VariableItemList* list;
    bool enabled;
    bool enabled_on_boot;
} QflipperSettingsApp;

static const char* const on_off_text[] = {"OFF", "ON"};

static uint32_t qflipper_settings_exit(void* context) {
    UNUSED(context);
    return VIEW_NONE;
}

static void qflipper_enabled_changed(VariableItem* item) {
    QflipperSettingsApp* app = variable_item_get_context(item);
    app->enabled = variable_item_get_current_value_index(item) != 0;
    variable_item_set_current_value_text(item, on_off_text[app->enabled]);
    desktop_api_set_qflipper_enabled(app->desktop, app->enabled);
}

static void qflipper_boot_changed(VariableItem* item) {
    QflipperSettingsApp* app = variable_item_get_context(item);
    app->enabled_on_boot = variable_item_get_current_value_index(item) != 0;
    variable_item_set_current_value_text(item, on_off_text[app->enabled_on_boot]);
    desktop_api_set_qflipper_on_boot(app->enabled_on_boot);
}

int32_t qflipper_settings_app(void* p) {
    UNUSED(p);

    QflipperSettingsApp* app = calloc(1, sizeof(QflipperSettingsApp));
    app->gui = furi_record_open(RECORD_GUI);
    app->desktop = furi_record_open(RECORD_DESKTOP);
    app->enabled = desktop_api_qflipper_is_enabled();
    app->enabled_on_boot = desktop_api_qflipper_on_boot();
    app->view_dispatcher = view_dispatcher_alloc();
    app->list = variable_item_list_alloc();

    variable_item_list_set_header(app->list, "qFlipper Settings:");
    VariableItem* item = variable_item_list_add(
        app->list, "qFlipper", 2, qflipper_enabled_changed, app);
    variable_item_set_current_value_index(item, app->enabled);
    variable_item_set_current_value_text(item, on_off_text[app->enabled]);

    item = variable_item_list_add(
        app->list, "Enable on boot", 2, qflipper_boot_changed, app);
    variable_item_set_current_value_index(item, app->enabled_on_boot);
    variable_item_set_current_value_text(item, on_off_text[app->enabled_on_boot]);

    View* view = variable_item_list_get_view(app->list);
    view_set_previous_callback(view, qflipper_settings_exit);
    view_dispatcher_add_view(app->view_dispatcher, 0, view);
    view_dispatcher_attach_to_gui(
        app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_switch_to_view(app->view_dispatcher, 0);
    view_dispatcher_run(app->view_dispatcher);

    view_dispatcher_remove_view(app->view_dispatcher, 0);
    variable_item_list_free(app->list);
    view_dispatcher_free(app->view_dispatcher);
    furi_record_close(RECORD_DESKTOP);
    furi_record_close(RECORD_GUI);
    free(app);
    return 0;
}

#include <furi.h>
#include <desktop/desktop.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>

typedef struct {
    Gui* gui;
    Desktop* desktop;
    ViewDispatcher* view_dispatcher;
    Submenu* submenu;
    bool open_usb_storage;
} OthersSettingsApp;

static uint32_t others_settings_exit(void* context) {
    UNUSED(context);
    return VIEW_NONE;
}

static void others_settings_enter(void* context, uint32_t index) {
    OthersSettingsApp* app = context;
    if(index == 0) {
        app->open_usb_storage = true;
        view_dispatcher_stop(app->view_dispatcher);
    }
}

int32_t others_settings_app(void* p) {
    UNUSED(p);

    OthersSettingsApp* app = calloc(1, sizeof(OthersSettingsApp));
    app->gui = furi_record_open(RECORD_GUI);
    app->desktop = furi_record_open(RECORD_DESKTOP);
    app->view_dispatcher = view_dispatcher_alloc();
    app->submenu = submenu_alloc();

    submenu_set_header(app->submenu, "Others:");
    submenu_add_item(app->submenu, "USB Storage", 0, others_settings_enter, app);
    view_set_previous_callback(submenu_get_view(app->submenu), others_settings_exit);

    view_dispatcher_add_view(app->view_dispatcher, 0, submenu_get_view(app->submenu));
    view_dispatcher_attach_to_gui(
        app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_switch_to_view(app->view_dispatcher, 0);
    view_dispatcher_run(app->view_dispatcher);

    view_dispatcher_remove_view(app->view_dispatcher, 0);
    submenu_free(app->submenu);
    view_dispatcher_free(app->view_dispatcher);
    if(app->open_usb_storage) desktop_api_open_usb_storage(app->desktop);
    furi_record_close(RECORD_DESKTOP);
    furi_record_close(RECORD_GUI);
    free(app);
    return 0;
}

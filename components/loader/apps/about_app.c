/* T-Embed About pages. Navigation adapted from Momentum-Firmware (GPL-3.0).
 * Attribution license: licenses/Momentum-Firmware-GPL-3.0.txt.
 * Only this ESP32 port is modified. */
#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/elements.h>
#include <furi_hal_version.h>
#include <u8g2.h>

typedef struct {
    uint8_t page;
    bool about_battery;
} AboutModel;

typedef struct {
    ViewDispatcher* dispatcher;
    View* view;
    int32_t result;
} AboutApp;

static void about_draw(Canvas* canvas, void* context) {
    AboutModel* model = context;
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontSecondary);
    if(model->page == 0) {
        canvas_draw_str(canvas, 0, 9, "Product: T-Embed CC1101");
        char model_text[32];
        snprintf(model_text, sizeof(model_text), "Model: %s", furi_hal_version_get_model_code());
        canvas_draw_str(canvas, 0, 19, model_text);
        canvas_draw_str(canvas, 0, 29, "Flipper Zero Esp32");
        canvas_draw_str(canvas, 0, 39, "Momentum Port");
        canvas_draw_str(canvas, 0, 49, "Github: github.com/Filip1x3");
        if(model->about_battery) elements_button_left(canvas, "Prev.");
        elements_button_right(canvas, "Next");
    } else {
        canvas_draw_str(canvas, 0, 8, "Forked from:");
        /* Original URL: https://github.com/Sor3nt/Flipper-Zero-ESP32-Port */
        canvas_set_custom_u8g2_font(canvas, u8g2_font_4x6_tr);
        canvas_draw_str(canvas, 0, 17, "https://github.com/Sor3nt/");
        canvas_draw_str(canvas, 0, 24, "Flipper-Zero-ESP32-Port");
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 0, 32, "Assets and other code:");
        /* Original URL: https://github.com/Next-Flip/Momentum-Firmware */
        canvas_set_custom_u8g2_font(canvas, u8g2_font_4x6_tr);
        canvas_draw_str(canvas, 0, 41, "https://github.com/Next-Flip/");
        canvas_draw_str(canvas, 0, 48, "Momentum-Firmware");
        canvas_set_font(canvas, FontSecondary);
        elements_button_left(canvas, "Prev.");
    }
}

static bool about_input(InputEvent* event, void* context) {
    AboutApp* app = context;
    if(event->type != InputTypeShort) return false;
    bool consumed = false;
    bool exit_to_battery = false;
    with_view_model(app->view, AboutModel * model, {
        if(event->key == InputKeyRight && model->page == 0) {
            model->page = 1;
            consumed = true;
        } else if(event->key == InputKeyLeft) {
            if(model->page == 1) {
                model->page = 0;
                consumed = true;
            } else if(model->about_battery) {
                exit_to_battery = true;
                consumed = true;
            }
        }
    }, consumed);
    if(exit_to_battery) {
        app->result = 1;
        view_dispatcher_stop(app->dispatcher);
    }
    return consumed;
}

static uint32_t about_previous(void* context) {
    UNUSED(context);
    return VIEW_NONE;
}

int32_t about_app(void* p) {
    AboutApp app = {0};
    Gui* gui = furi_record_open(RECORD_GUI);
    app.dispatcher = view_dispatcher_alloc();
    app.view = view_alloc();
    view_allocate_model(app.view, ViewModelTypeLocking, sizeof(AboutModel));
    with_view_model(app.view, AboutModel * model, {
        model->page = 0;
        model->about_battery = p && !strncmp(p, "about_battery", strlen("about_battery"));
    }, false);
    view_set_context(app.view, &app);
    view_set_draw_callback(app.view, about_draw);
    view_set_input_callback(app.view, about_input);
    view_set_previous_callback(app.view, about_previous);
    view_set_input_mode(app.view, ViewInputModeLeftRight);
    view_dispatcher_add_view(app.dispatcher, 0, app.view);
    view_dispatcher_attach_to_gui(app.dispatcher, gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_switch_to_view(app.dispatcher, 0);
    view_dispatcher_run(app.dispatcher);
    view_dispatcher_remove_view(app.dispatcher, 0);
    view_free(app.view);
    view_dispatcher_free(app.dispatcher);
    furi_record_close(RECORD_GUI);
    return app.result;
}

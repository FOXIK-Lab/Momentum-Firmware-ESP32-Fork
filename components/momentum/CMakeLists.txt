idf_component_register(
    SRCS
        "../../lib/momentum/settings.c"
        "../../lib/momentum/namespoof.c"
        "asset_packs_port.c"
        "rgb_backlight_port.c"
    INCLUDE_DIRS "." "../../lib" "../../lib/momentum"
    REQUIRES furi furi_hal storage flipper_format gui toolbox assets
)

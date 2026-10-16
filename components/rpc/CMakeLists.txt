idf_component_register(
    SRCS
        "rpc.c"
        "rpc_system.c"
        "rpc_storage.c"
        "rpc_app.c"
        "rpc_gui.c"
        "rpc_property.c"
        "../../applications/services/rpc/rpc_desktop.c"
    INCLUDE_DIRS "."
    PRIV_INCLUDE_DIRS "../../applications/services"
    REQUIRES furi furi_hal toolbox storage nanopb flipper_protobuf notification btshim datetime gui loader input assets fw_ota momentum
)

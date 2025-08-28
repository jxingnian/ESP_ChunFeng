idf_component_register(
    SRCS
        screens/ui_ScreenStart.c
        screens/ui_ScreenMain.c
        ui.c
        components/ui_comp_hook.c
        ui_helpers.c
        ui_events.c
        fonts/ui_font_MiSansBold16.c
        gif/loading-4802_256.c
    PRIV_REQUIRES
        lvgl__lvgl
        main
    INCLUDE_DIRS
        "." "gif"
)
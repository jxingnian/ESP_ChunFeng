idf_component_register(
       SRCS
            screens/ui_Screen1.c
            ui.c
            components/ui_comp_hook.c
            ui_helpers.c
       PRIV_REQUIRES
            lvgl
       INCLUDE_DIRS
            "."
)


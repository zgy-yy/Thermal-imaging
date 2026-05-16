set(src_dirs
    main.c
    App/ui/ui.c
)
set(include_dirs . App/ui)

idf_component_register(SRCS ${src_dirs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES middlewares lvgl__lvgl esp_timer)

target_compile_definitions(${COMPONENT_LIB} PUBLIC LV_CONF_INCLUDE_SIMPLE)

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_definitions(${lvgl_lib} PRIVATE LV_CONF_INCLUDE_SIMPLE)
target_include_directories(${lvgl_lib} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

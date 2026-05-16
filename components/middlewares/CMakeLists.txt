set(src_dirs Camera/uvc_camera.c Lcd/st7735s.c)
set(include_dirs Camera Lcd)

set(requireds esp_psram espressif__usb_stream bsp esp_driver_gpio)

idf_component_register(SRCS ${src_dirs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES ${requireds})

component_compile_options(-ffast-math -O3 -Wno-error=format -Wno-format)
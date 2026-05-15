set(src_dirs Uvc/uvc_host_reader.c)
set(include_dirs Uvc)

set(requireds esp_psram espressif__usb espressif__usb_host_uvc)

idf_component_register(SRCS ${src_dirs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES ${requireds})

component_compile_options(-ffast-math -O3 -Wno-error=format -Wno-format)

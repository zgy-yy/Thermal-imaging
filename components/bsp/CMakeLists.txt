set(src_dirs Usart/uart_comm.c)
set(include_dirs Usart)

set(requireds esp_driver_uart)

idf_component_register(SRCS ${src_dirs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES ${requireds})

component_compile_options(-ffast-math -O3 -Wno-error=format -Wno-format)
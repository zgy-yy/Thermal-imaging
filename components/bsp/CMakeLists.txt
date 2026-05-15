set(src_dirs Usart/uart_comm.c Spi/spi_comm.c)
set(include_dirs Usart Spi)

set(requireds esp_driver_uart esp_driver_spi)

idf_component_register(SRCS ${src_dirs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES ${requireds})

component_compile_options(-ffast-math -O3 -Wno-error=format -Wno-format)
set(COMPONENT_SRCS main.c axp192.c st7735s.c fontx.c button_int.c mcpwm_control.c)
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()
spiffs_create_partition_image(storage ../font FLASH_IN_PROJECT)

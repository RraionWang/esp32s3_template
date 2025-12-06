idf_component_register(SRCS "my_ws.c" "main.c" "audio_driver.c" "my_fs.c" "midi.c" "my_ws.c"
                    INCLUDE_DIRS ".")


littlefs_create_partition_image(storage ../flash_data FLASH_IN_PROJECT)
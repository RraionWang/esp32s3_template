#include <stdio.h>
#include "audio_driver.h"
#include "my_fs.h"
#include "midi.h"
#include "freertos/FreeRTOS.h"
#include "my_ws.h"

void app_main(void)
{
    //  init_littlefs();
    // init_sdcard() ; 
    // i2s_driver_init();
    // es8311_codec_init();


    init_midi();

    // 创建 FreeRTOS MIDI 任务
midi_task(NULL) ;


// init_wifi_station() ; 




}
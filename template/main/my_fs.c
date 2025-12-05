#include "my_fs.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_littlefs.h"
#include "sdmmc_cmd.h"
#include "driver/gpio.h"
#include "esp_vfs_common.h"
#include "driver/spi_common.h"
#include "esp_vfs_fat.h"


static const char *TAG = "esp_littlefs";

#define LITTLEFS_MOUNT_POINT  "/littlefs"
#define SD_MOUNT_POINT "/sd"


#define PIN_NUM_MISO  GPIO_NUM_3
#define PIN_NUM_MOSI  GPIO_NUM_1
#define PIN_NUM_CLK   GPIO_NUM_2
#define PIN_NUM_CS    GPIO_NUM_42
#define MY_SD_HOST_ID  SPI2_HOST
#define MY_SDMMC_FREQ SDMMC_FREQ_DEFAULT



void init_littlefs(void)
{
    ESP_LOGI(TAG, "Initializing LittleFS");

    esp_vfs_littlefs_conf_t conf = {
        .base_path = LITTLEFS_MOUNT_POINT,
        .partition_label = "storage",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    // Use settings defined above to initialize and mount LittleFS filesystem.
    // Note: esp_vfs_littlefs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return;
    }
}

void  init_sdcard(){
    esp_err_t ret;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    const char mount_point[] = SD_MOUNT_POINT;
    ESP_LOGI(TAG, "开始初始化SD卡");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = MY_SD_HOST_ID ; 
    host.max_freq_khz = MY_SDMMC_FREQ ; 
    
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD卡初始化SPI总线失败");
        return;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ESP_LOGI(TAG, "挂载文件系统中");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "挂载文件系统失败，请格式化sd卡 ");
        } else {
            ESP_LOGE(TAG, "初始化卡 (%s)失败. 请检查上拉电阻或者卡引脚 ", esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(TAG, "已经成功挂载sd卡操作系统");
}
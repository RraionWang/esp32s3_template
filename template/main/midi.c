#include "midi.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define UART_NUM UART_NUM_1
#define TX_PIN 15      // 自己换你喜欢的 GPIO

void init_midi(){
        uart_config_t conf = {
        .baud_rate = 31250,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM, &conf);
    uart_set_pin(UART_NUM, TX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, 256, 0, 0, NULL, 0);
}


void midi_send(uint8_t *data, int len)
{
    uart_write_bytes(UART_NUM, (const char *)data, len);
}

void midi_note_on(uint8_t note, uint8_t vel)
{
    uint8_t msg[3] = {0x90, note, vel};
    midi_send(msg, 3);
}

void midi_note_off(uint8_t note)
{
    uint8_t msg[3] = {0x80, note, 0};
    midi_send(msg, 3);
}

void midi_program_change(uint8_t program)
{
    uint8_t msg[2] = {0xC0, program};
    midi_send(msg, 2);
}

void midi_task(void *arg)
{
    // 设置音色为八音盒（Music Box = Program 10）
    midi_program_change(10);

    // 设置音量为 10%（MIDI CC7 = Volume）
    uint8_t volume_msg[3] = {0xB0, 0x07, 12}; // 通道0，音量=12
    midi_send(volume_msg, 3);

    int notes[] = {72, 74, 76, 77, 79, 81, 83}; // C5 D5 E5 F5 G5 A5 B5
    int count = sizeof(notes) / sizeof(notes[0]);

    while (1)
    {
        for (int i = 0; i < count; i++)
        {
            midi_note_on(notes[i], 100);       // Note On
            vTaskDelay(pdMS_TO_TICKS(400));    // 发声时间

            midi_note_off(notes[i]);           // Note Off
            vTaskDelay(pdMS_TO_TICKS(80));     // 间隔
        }

        vTaskDelay(pdMS_TO_TICKS(1000));       // 播完等待一下
    }
}

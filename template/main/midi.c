#include "midi.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_random.h"

#define UART_NUM UART_NUM_1
#define TX_PIN 1 // 自己换你喜欢的 GPIO

// 全局音量控制 (0-127)
#define MASTER_VOLUME 10  // 在这里调整整体音量！

const char *note_table[128] = {
    "C-1","C#-1","D-1","D#-1","E-1","F-1","F#-1","G-1","G#-1","A-1","A#-1","B-1",
    "C0","C#0","D0","D#0","E0","F0","F#0","G0","G#0","A0","A#0","B0",
    "C1","C#1","D1","D#1","E1","F1","F#1","G1","G#1","A1","A#1","B1",
    "C2","C#2","D2","D#2","E2","F2","F#2","G2","G#2","A2","A#2","B2",
    "C3","C#3","D3","D#3","E3","F3","F#3","G3","G#3","A3","A#3","B3",
    "C4","C#4","D4","D#4","E4","F4","F#4","G4","G#4","A4","A#4","B4",
    "C5","C#5","D5","D#5","E5","F5","F#5","G5","G#5","A5","A#5","B5",
    "C6","C#6","D6","D#6","E6","F6","F#6","G6","G#6","A6","A#6","B6",
    "C7","C#7","D7","D#7","E7","F7","F#7","G7","G#7","A7","A#7","B7",
    "C8","C#8","D8","D#8","E8","F8","F#8","G8","G#8","A8","A#8","B8",
    "C9","C#9","D9","D#9","E9","F9","F#9","G9"
};


void init_midi()
{
    uart_config_t conf = {
        .baud_rate = 31250,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM, &conf);
    uart_set_pin(UART_NUM, TX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, 256, 0, 0, NULL, 0);

    calcBeat();

    // 先设置 SAM2695 硬件主音量 (全局控制)
    sam2695_set_master_volume(MASTER_VOLUME);
    
    vTaskDelay(pdMS_TO_TICKS(50)); // 等待芯片初始化

    // 使用钢弦吉他音色 (Acoustic Guitar Steel)
    midi_program_change(25);

    // 设置 MIDI 通道音量 (软件控制)
    midi_set_volume(MASTER_VOLUME);
    
    // 添加混响效果 (MIDI CC91 = Reverb)
    uint8_t reverb_msg[3] = {0xB0, 0x5B, 50};
    midi_send(reverb_msg, 3);
    
    // 设置表情控制器增加动态感 (MIDI CC11 = Expression)
    uint8_t expression_msg[3] = {0xB0, 0x0B, 100};
    midi_send(expression_msg, 3);
}

// 音量控制函数
void midi_set_volume(uint8_t volume)
{
    if (volume > 127) volume = 127;
    
    // CC7 = Channel Volume (影响所有音符)
    uint8_t msg[3] = {0xB0, 0x07, volume};
    midi_send(msg, 3);
    
    ESP_LOGI("MIDI", "音量设置为: %d", volume);
}

// SAM2695 硬件主音量控制 (通过 GM System On + Master Volume)
void sam2695_set_master_volume(uint8_t volume)
{
    if (volume > 127) volume = 127;
    
    // 使用标准 GM Master Volume SysEx
    uint8_t sysex_msg[] = {
        0xF0,           // SysEx 开始
        0x7F,           // 通用实时消息
        0x7F,           // 所有设备
        0x04,           // 设备控制
        0x01,           // Master Volume
        volume & 0x7F,  // 音量低字节
        volume >> 7,    // 音量高字节 (通常是0)
        0xF7            // SysEx 结束
    };
    
    midi_send(sysex_msg, sizeof(sysex_msg));
    
    vTaskDelay(pdMS_TO_TICKS(10)); // 给芯片时间处理
    
    ESP_LOGI("SAM2695", "硬件主音量: %d", volume);
}

void midi_send(uint8_t *data, int len)
{
    uart_write_bytes(UART_NUM, (const char *)data, len);
}

void midi_note_on(uint8_t note, uint8_t vel)
{
    uint8_t msg[3] = {0x90, note, vel};
    midi_send(msg, 3);

    ESP_LOGI("ON", "NOTE:%s VEL:%d", note_table[note], vel);
}

void midi_note_off(uint8_t note)
{
    uint8_t msg[3] = {0x80, note, 0};
    midi_send(msg, 3);
}

void midi_program_change(uint8_t program)
{
    // Bank Select MSB = 0
    uint8_t bank_msb[3] = {0xB0, 0x00, 0x00};
    midi_send(bank_msb, 3);

    // Program Change
    uint8_t msg[2] = {0xC0, program};
    midi_send(msg, 2);
}

int beat_ms;
int note_1;
int note_1_2;
int note_1_4;
int note_1_8;
int note_1_16;
int note_1_32;

int bpm = 85; // 降低BPM，更适合弹唱节奏

void calcBeat()
{
    beat_ms = 60000 / bpm;
    note_1 = beat_ms * 4;
    note_1_2 = beat_ms * 2;
    note_1_4 = beat_ms;
    note_1_8 = beat_ms / 2;
    note_1_16 = beat_ms / 4;
    note_1_32 = beat_ms / 8;
}

// 吉他六弦空弦的 MIDI 编号
int guitar_open_string_midi[6] = {
    /*1弦*/ 64, // E4
    /*2弦*/ 59, // B3
    /*3弦*/ 55, // G3
    /*4弦*/ 50, // D3
    /*5弦*/ 45, // A2
    /*6弦*/ 40  // E2
};

void playGuitarNote(int stringNum, int fretNum, int velocity)
{
    int baseNote = guitar_open_string_midi[stringNum];
    int midiNote = baseNote + fretNum;

    midi_note_on(midiNote, velocity);
}

void stopGuitarNote(int stringNum, int fretNum)
{
    int baseNote = guitar_open_string_midi[stringNum];
    int midiNote = baseNote + fretNum;

    midi_note_off(midiNote);
}

int harmony[HARMONY_NUM_MAX][6] = {
    {0, 0, 0, 2, 2, 0},  // HARMONY_NUM_Em
    {3, 0, 0, 0, 2, 3},  // HARMONY_NUM_G
    {2, 3, 2, 0, -1, 2}, // HARMONY_NUM_DFSharp

    {0, 1, 0, 2, 3, -1}, // HARMONY_NUM_C
    {2, 3, 2, 0, 0, 2},  // HARMONY_NUM_D
    {0, 2, 2, 2, 0, 0},  // HARMONY_NUM_A
    {0, 1, 2, 2, 0, 0},  // HARMONY_NUM_Am
    {1, 3, 2, 0, 0, 0},  // HARMONY_NUM_E

    {1, 3, 2, 2, 1, 1},  // HARMONY_NUM_F
    {1, 3, 2, 0, -1, 1}, // HARMONY_NUM_Fmaj7
    {1, 1, 2, 2, 1, 1},  // HARMONY_NUM_Fm

    {3, 1, 0, 0, 2, 3},  // HARMONY_NUM_G7
    {0, 1, 0, 0, 2, 0},  // HARMONY_NUM_Cmaj7
    {3, 1, 0, 2, 3, -1}, // HARMONY_NUM_Cadd9
    {3, 3, 0, 0, 2, 3},  // HARMONY_NUM_Gadd9
};

// 改进的下扫弦 - 带力度变化和随机性
void play_down(int keep, int fret[6], int accent)
{
    ESP_LOGI("down", "下扫开始 (accent=%d)", accent);

    for (int i = 5; i >= 0; i--)
    {
        if (fret[i] == -1)
        {
            continue;
        }

        // 低音弦力度更大，高音弦力度递减
        // accent: 0=普通, 1=强拍
        int base_velocity = accent ? 65 : 45;
        int velocity = base_velocity + (5 - i) * 6;  // 从高到低递减
        
        // 添加轻微随机波动 (±5)
        velocity += (esp_random() % 11) - 5;
        
        // 限制范围
        if (velocity > 110) velocity = 110;
        if (velocity < 30) velocity = 30;

        playGuitarNote(i, fret[i], velocity);
        
        // 扫弦速度随机化 (10-18ms)
        int delay = 10 + (esp_random() % 9);
        vTaskDelay(pdMS_TO_TICKS(delay));
    }

    ESP_LOGI("down", "下扫结束");
    vTaskDelay(pdMS_TO_TICKS(keep));

    // 分散关闭音符，模拟自然衰减
    for (int i = 5; i >= 0; i--)
    {
        if (fret[i] == -1)
        {
            continue;
        }
        
        stopGuitarNote(i, fret[i]);
        vTaskDelay(pdMS_TO_TICKS(3 + (esp_random() % 5)));
    }
}

// 改进的上扫弦
void play_up(int keep, int fret[6], int accent)
{
    ESP_LOGI("up", "上扫开始 (accent=%d)", accent);

    for (int i = 0; i < 6; i++)
    {
        if (fret[i] == -1)
        {
            continue;
        }

        // 上扫时高音弦力度稍大
        int base_velocity = accent ? 60 : 40;
        int velocity = base_velocity + i * 5;
        
        // 添加随机波动
        velocity += (esp_random() % 11) - 5;
        
        if (velocity > 105) velocity = 105;
        if (velocity < 25) velocity = 25;

        playGuitarNote(i, fret[i], velocity);
        
        int delay = 10 + (esp_random() % 9);
        vTaskDelay(pdMS_TO_TICKS(delay));
    }

    ESP_LOGI("up", "上扫结束");
    vTaskDelay(pdMS_TO_TICKS(keep));

    for (int i = 0; i < 6; i++)
    {
        if (fret[i] == -1)
        {
            continue;
        }
        
        stopGuitarNote(i, fret[i]);
        vTaskDelay(pdMS_TO_TICKS(3 + (esp_random() % 5)));
    }
}

// 闷音扫弦效果
void play_mute(int fret[6])
{
    ESP_LOGI("mute", "闷音扫弦");
    
    for (int i = 5; i >= 0; i--)
    {
        if (fret[i] == -1) continue;
        
        playGuitarNote(i, fret[i], 25);  // 很低的力度
        vTaskDelay(pdMS_TO_TICKS(8));
    }
    
    vTaskDelay(pdMS_TO_TICKS(40));  // 很短的持续
    
    for (int i = 5; i >= 0; i--)
    {
        if (fret[i] == -1) continue;
        stopGuitarNote(i, fret[i]);
    }
}

// 分解和弦 - 模拟指弹
void play_arpeggio(int fret[6], int pattern)
{
    ESP_LOGI("arp", "分解和弦 pattern=%d", pattern);
    
    int sequences[][6] = {
        {5, 3, 2, 1, 2, 3},  // 常见分解模式1
        {5, 2, 3, 1, 3, 2},  // 常见分解模式2
        {5, 4, 3, 2, 1, 0},  // 从低到高
    };
    
    int *seq = sequences[pattern % 3];
    
    for (int i = 0; i < 6; i++)
    {
        int string = seq[i];
        if (fret[string] == -1) continue;
        
        int velocity = 50 + (esp_random() % 20);
        playGuitarNote(string, fret[string], velocity);
        vTaskDelay(pdMS_TO_TICKS(note_1_16));
        stopGuitarNote(string, fret[string]);
    }
}

// 经典扫弦节奏型1: 下 下上 上下上
void sweep1(int fret[6])
{
    play_down(note_1_8, fret, 1);      // 强拍下扫
    play_down(note_1_16, fret, 0);     // 弱
    play_up(note_1_16, fret, 0);       // 弱上扫
    play_up(note_1_16, fret, 0);       // 弱上扫
    play_down(note_1_16, fret, 0);     // 弱
    play_up(note_1_16, fret, 0);       // 弱上扫
}

// 扫弦节奏型2: 下 下 上上下上
void sweep2(int fret[6])
{
    play_down(note_1_8, fret, 1);      // 强拍
    play_down(note_1_8, fret, 0);      
    play_up(note_1_16, fret, 0);       
    play_up(note_1_16, fret, 0);       
    play_down(note_1_16, fret, 0);     
    play_up(note_1_16, fret, 0);       
}

// 灵魂乐节奏: 下 闷 下上 闷 下
void sweep_soul(int fret[6])
{
    play_down(note_1_16, fret, 1);     
    play_mute(fret);                    
    vTaskDelay(pdMS_TO_TICKS(note_1_16));
    play_down(note_1_16, fret, 0);     
    play_up(note_1_16, fret, 0);       
    play_mute(fret);                    
    vTaskDelay(pdMS_TO_TICKS(note_1_16));
    play_down(note_1_16, fret, 0);     
}

void midi_task(void *arg)
{
    while (1)
    {
        // 使用不同的扫弦模式演奏和弦进行
        ESP_LOGI("MIDI", "======= 开始新一轮 =======");
        
        // C - G - Am - Em 经典流行进行
        sweep1(harmony[HARMONY_NUM_C]);
        sweep1(harmony[HARMONY_NUM_G]);
        sweep2(harmony[HARMONY_NUM_Am]);
        sweep2(harmony[HARMONY_NUM_Em]);
        
        // F - Em 带分解和弦
        play_arpeggio(harmony[HARMONY_NUM_F], 0);
        sweep1(harmony[HARMONY_NUM_Em]);
        
        // 加入灵魂乐节奏变化
        sweep_soul(harmony[HARMONY_NUM_C]);
        sweep_soul(harmony[HARMONY_NUM_G]);
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // 段落间停顿
    }
}
#ifndef MIDI_H
#define MIDI_H

#include <stdint.h>
#include "driver/uart.h"

// 和弦枚举定义
typedef enum
{
    HARMONY_NUM_Em = 0,
    HARMONY_NUM_G,
    HARMONY_NUM_DFSharp,
    HARMONY_NUM_C,
    HARMONY_NUM_D,
    HARMONY_NUM_A,
    HARMONY_NUM_Am,
    HARMONY_NUM_E,
    HARMONY_NUM_F,
    HARMONY_NUM_Fmaj7,
    HARMONY_NUM_Fm,
    HARMONY_NUM_G7,
    HARMONY_NUM_Cmaj7,
    HARMONY_NUM_Cadd9,
    HARMONY_NUM_Gadd9,
    HARMONY_NUM_MAX
} harmony_num_t;

// 音符名称表(如果需要的话)
extern const char *note_table[];

// 全局变量声明
extern int harmony[HARMONY_NUM_MAX][6];
extern int guitar_open_string_midi[6];

// 节拍相关变量声明
extern int beat_ms;
extern int note_1;
extern int note_1_2;
extern int note_1_4;
extern int note_1_8;
extern int note_1_16;
extern int note_1_32;
extern int bpm;

// 函数声明
void init_midi(void);
void midi_send(uint8_t *data, int len);
void midi_note_on(uint8_t note, uint8_t vel);
void midi_note_off(uint8_t note);
void midi_program_change(uint8_t program);
void midi_set_volume(uint8_t volume);
void sam2695_set_master_volume(uint8_t volume);

void calcBeat(void);

void playGuitarNote(int stringNum, int fretNum, int velocity);
void stopGuitarNote(int stringNum, int fretNum);

void play_down(int keep, int fret[6], int accent);
void play_up(int keep, int fret[6], int accent);
void play_mute(int fret[6]);
void play_arpeggio(int fret[6], int pattern);

void sweep1(int fret[6]);
void sweep2(int fret[6]);
void sweep_soul(int fret[6]);

void midi_task(void *arg);

#endif // MIDI_H
/*********************************************************************************************

     SAATunes v 1.04
	 
	 A library to play MIDI tunes on the SAA1099 using an arduino. 
	 For documentation, see SAATunes.cpp
	 
   Copyright (c) 2011, 2016, Len Shustek
   SAA1099 version Copyright (c) 2018, Jacob Field
   
**************************************************************************************/
/*
  Change log
   25 September 2017, Jacob Field
     - Original (release) library version, v1.03
	 16 August 2018, Jacob Field
	- Fixed reported bug: first letter of <arduino.h> in the #include line not being capitalized was breaking library on linux
	- Released as v1.04
  -----------------------------------------------------------------------------------------*/

#ifndef SAATunes_h
#define SAATunes_h

#include <stdint.h>
#include <stdbool.h>

//Volume (Velocity) variables
#define DO_VOLUME 1         // generate volume-modulating code? Needs -v on Miditones.
#define ASSUME_VOLUME  0    // if the file has no header, should we assume there is volume info?
#define DO_PERCUSSION 1     // generate percussion sounds? Needs DO_VOLUME, and -pt on Miditones
#define DO_DECAY 1     		// Do decay on sustained notes? We'll assume yes. Later we might make it toggleable in program.

#define SAATUNES_CHANNELS_MAX 6

#define SAA1099_PORT_VAL  255
#define SAA1099_PORT_REG  511

//Percussion variables
#if DO_PERCUSSION
#define NO_DRUM 0xff
#endif


struct file_hdr_t {  // the optional bytestream file header
    char id1;     // 'P'
    char id2;     // 't'
    unsigned char hdr_length; // length of whole file header
    unsigned char f1;         // flag byte 1
    unsigned char f2;         // flag byte 2
    unsigned char num_tgens;  // how many tone generators are used by this score
};
#define HDR_F1_VOLUME_PRESENT 0x80
#define HDR_F1_INSTRUMENTS_PRESENT 0x40
#define HDR_F1_PERCUSSION_PRESENT 0x20

struct SAATunesContext
{
    struct file_hdr_t file_header;
    volatile bool tune_playing;			// Is the score still playing?
    bool volume_present;

    const uint8_t *score_start;
    const uint8_t *score_cursor;

    bool channel_active[SAATUNES_CHANNELS_MAX];        		    // An array which stores which channels are active and which are not
    uint8_t prevOctaves[SAATUNES_CHANNELS_MAX]; //Array for storing the last octave value for each channel

    volatile unsigned long wait_toggle_count;      /* countdown score waits */

#if DO_DECAY
    unsigned int decay_rate;				// 1/16 of the rate in Milliseconds in which to decay notes
    uint8_t decay_timer[SAATUNES_CHANNELS_MAX];
    uint8_t decay_volume[SAATUNES_CHANNELS_MAX];
    bool doing_decay[SAATUNES_CHANNELS_MAX];
#endif

#if DO_PERCUSSION
    uint8_t drum_chan_one; // channel playing drum now
    uint8_t drum_tick_count; // count ticks before bit change based on random bit
    uint8_t drum_tick_limit; // count to this
    int drum_duration;   // limit on drum duration
#endif

};


/**
 * Initialises SAATunes context
 *
 * @param context
 */
void SAATunesInit(struct SAATunesContext *context);


/**
 * Start playing a score
 *
 * @param context
 * @param score
 */
void SAATunesPlayScore(struct SAATunesContext *context, const uint8_t *score);


/**
 * Stop playing the score
 *
 * @param context
 */
void SAATunesStopScore(struct SAATunesContext *context);


/**
 *
 * @param context
 */
void SAATunesStrum(struct SAATunesContext *context);


/**
 *
 * @param context
 * @param chan
 * @param note
 * @param volume
 */
void SAATunesPlayNote(struct SAATunesContext *context, uint8_t chan, uint8_t note, uint8_t volume);


/**
 *
 * @param context
 * @param chan
 */
void SAATunesStopNote (struct SAATunesContext *context, uint8_t chan);


/**
 * Tick to be called every 1ms
 *
 * @param context
 */
void SAATunesTick(struct SAATunesContext *context);

#endif
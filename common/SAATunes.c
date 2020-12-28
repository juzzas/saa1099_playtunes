/*********************************************************************************************

     SAATunes v 1.04
	 
	 A library to play MIDI tunes on the SAA1099 sound generator chip using an arduino
	 
	 Based on Len Shustek's PlayTune library, which plays a tune on a normal arduino
	 using its built in hardware timers.
	 You can find his library at: https://github.com/LenShustek/arduino-playtune
	 
	 Mine doesn't use timers, just a once-per-millisecond intterupt to turn notes
	 on/off and other things. All the code for addressing the SAA1099 and converting the 
	 bytestream is my own, and I've added a few tidbits that weren't in Len's original
	 library.
	 
	 Special features and MIDI functions this library supports
	 
	 - Velocity Data (Which is interpreted as the initial note volume)
	 - Which channels are off/on, accessed through a boolean array
	 - Note volume decay (Decays the volume of a note that is held on over a specified rate)
	 
	 What I would like to add in the future
	 
	 - Using the SAA1099's built in noise channels for percussion (Which Len's original library supports)
	 - Using the SAA1099's built in envelope generators for different insturments (Maybe also some software
		generated envelopes, or wiggling the note's frequency back/forth slightly to produce a different sound?)
	 - Support for sustain pedal being used in a MIDI recording (Not sure if Len's original library supports 
		this or not)
	 

  ------------------------------------------------------------------------------------
   The MIT License (MIT)
   Original code Copyright (c) 2011, 2016, Len Shustek
   SAA1099 modified version Copyright (c) 2018, Jacob Field

  Permission is hereby granted, free of charge, to any person obtaining a copy of
  this software and associated documentation files (the "Software"), to deal in
  the Software without restriction, including without limitation the rights to use,
  copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
  Software, and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
**************************************************************************************/
/*
  Change log
   25 September 2017, Jacob Field
     - Original (release) library version, v1.03
	16 August 2018, Jacob Field
	- Fixed reported bug: first letter of <arduino.h> in the #include line not being capitalized was breaking library on linux.
	- Released as v1.04
  -----------------------------------------------------------------------------------------*/
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <z80.h>

#include "SAATunes.h"


__sfr __banked __at SAA1099_PORT_VAL  saa_port_val;
__sfr __banked __at SAA1099_PORT_REG  saa_port_reg;

static uint8_t noteAdr[] = {5, 32, 60, 85, 110, 132, 153, 173, 192, 210, 227, 243}; // The 12 note-within-an-octave values for the SAA1099, starting at B
static uint8_t octaveAdr[] = {0x10, 0x11, 0x12}; //The 3 octave addresses (was 10, 11, 12)
static uint8_t channelAdr[] = {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D}; //Addresses for the channel frequencies

static void SAATunesStepScore (struct SAATunesContext *context);

// ******** Code Blocks ******** \\

void SAATunesInit(struct SAATunesContext *context)
{
    memset(context, 0, sizeof(*context));

    context->volume_present = ASSUME_VOLUME;

#if DO_DECAY
    //Modifiable delay rate variable (In MS, and each time the specified number of MS has passed, decays volume by 1/16).
    //Originally 125, which is what I determined is /roughly/ correct for how long piano notes sustain while held down
    context->decay_rate = 125;
#endif

#if DO_PERCUSSION
    context->drum_chan_one = NO_DRUM;
#endif

    // Reset/Enable all the sound channels
    saa_port_reg = 0x1c;
    saa_port_val = 0x02;
    saa_port_val = 0x00;

    saa_port_reg = 0x1c;
    saa_port_val = 0x01;

    // Enable the frequency channels
    saa_port_reg = 0x14;
    saa_port_val = 0x3f;

    // Disable the noise channels
    saa_port_reg = 0x15;
    saa_port_val = 0x00;

    // Disable envelopes on Channels 2 and 5
    saa_port_reg = 0x18;
    saa_port_val = 0x00;

    saa_port_reg = 0x19;
    saa_port_val = 0x00;

    SAATunesStopNote(context, 0);
    SAATunesStopNote(context, 1);
    SAATunesStopNote(context, 2);
    SAATunesStopNote(context, 3);
    SAATunesStopNote(context, 4);
    SAATunesStopNote(context, 5);
}

void SAATunesStrum(struct SAATunesContext *context)
{
    SAATunesPlayNote(context, 3, 24, 64);
    z80_delay_ms(100);
    SAATunesPlayNote(context, 0, 48, 64);
    z80_delay_ms(100);
    SAATunesPlayNote(context, 1, 52, 64);
    z80_delay_ms(100);
    SAATunesPlayNote(context, 2, 55, 64);
    z80_delay_ms(100);
    SAATunesPlayNote(context, 3, 60, 64);
    z80_delay_ms(100);
    SAATunesPlayNote(context, 4, 64, 64);
    z80_delay_ms(1024);
    SAATunesStopNote(context, 0);
    SAATunesStopNote(context, 1);
    SAATunesStopNote(context, 2);
    SAATunesStopNote(context, 3);
    SAATunesStopNote(context, 4);
    SAATunesStopNote(context, 5);
}



// Start playing a note on a particular channel

void SAATunesPlayNote(struct SAATunesContext *context, uint8_t chan, uint8_t note, uint8_t volume)
{
  context->channel_active[chan] = false;
	
  //Percussion code, in this version we're ignoring percussion. 
  if (note > 127) { // Notes above 127 are percussion sounds.
	note = 60; //Set note to some random place
	volume = 0; //Then set it to 0 volume
  }
  
  //Shift the note down by 1, since MIDI octaves start at C, but octaves on the SAA1099 start at B
  note += 1; 

  uint8_t octave = (note / 12) - 1; //Some fancy math to get the correct octave
  uint8_t noteVal = note - ((octave + 1) * 12); //More fancy math to get the correct note

  context->prevOctaves[chan] = octave; //Set this variable so we can remember /next/ time what octave was /last/ played on this channel

  //Octave addressing and setting code:
  saa_port_reg = octaveAdr[chan / 2];

  if (chan == 0 || chan == 2 || chan == 4) {
      saa_port_val = octave | (context->prevOctaves[chan + 1] << 4); //Do fancy math so that we don't overwrite what's already on the register, except in the area we want to.
  }
  
  if (chan == 1 || chan == 3 || chan == 5) {
      saa_port_val = (octave << 4) | context->prevOctaves[chan - 1]; //Do fancy math so that we don't overwrite what's already on the register, except in the area we want to.
  }
  
  //Note addressing and playing code
  //Set address to the channel's address
    saa_port_reg = channelAdr[chan];

  //EXPERIEMNTAL WARBLE CODE
  //noteAdr[noteVal] += random(-2, 2); //a plus/minus value of 15 gives a really out of tune version


  //Write actual note data
    saa_port_val = noteAdr[noteVal];

  //Volume updating
  //Set the Address to the volume (amplitude) channel
    saa_port_reg = chan;

  #if DO_DECAY
	//Decay channel updating
	context->doing_decay[chan] = true;
	context->decay_timer[chan] = 0;
  #endif

  #if DO_VOLUME
    {
        //Velocity is a value from 0-127, the SAA1099 only has 16 levels, so divide by 8.
        uint8_t vol = volume / 8;

        saa_port_val = (vol << 4) | vol;

#if DO_DECAY
        //Update the beginning volume for the decay controller
        context->decay_volume[chan] = vol;
#endif

    };
  #else
		
	//If we're not doing velocity, then just set it to max.
	saa_port_val = 0xff;

	#if DO_DECAY
		//Update the beginning volume for the decay controller
		context->decayVolume[chan] = 16;
	#endif
      
  #endif

    context->channel_active[chan] = true;

}


//-----------------------------------------------
// Stop playing a note on a particular channel
//-----------------------------------------------

void SAATunesStopNote (struct SAATunesContext *context, uint8_t chan) {
	
	context->channel_active[chan] = false;
	
	/* CURRENTLY MODIFIED FOR HACKED SUSTAIN
	
	if (drum_chan_one == chan){ //If drum is active on this channel, then run special code to disable
		drum_chan_one = NO_DRUM;
		
		//Set noise generator mode
		digitalWrite(AO, HIGH);
		PORTD = 0x16;
		writeAddress();

		if (chan < 3){
			digitalWrite(AO, LOW);
			PORTD = PORTD | (0 >> 4);
			writeAddress();
		} else {
			digitalWrite(AO, LOW);
			PORTD = PORTD | (0 << 4);
			writeAddress();
		}
		
		//Enable tone generator
		digitalWrite(AO, HIGH);
		PORTD = 0x14;
		writeAddress();

		digitalWrite(AO, LOW);
		bitSet(PORTD, chan);
		writeAddress();
	
		//Disable noise generator
		digitalWrite(AO, HIGH);
		PORTD = 0x15;
		writeAddress();

		digitalWrite(AO, LOW);
		bitClear(PORTD, chan);
		writeAddress();
	
	} 
    
	uint8_t volAddress[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};

	digitalWrite(AO, HIGH);
	PORTD = volAddress[byte(chan)];
	writeAddress();

	digitalWrite(AO, LOW);
	PORTD = 0x00;
	writeAddress();

	
	#if DO_DECAY
		doingDecay[chan] = false;
		decayVolume[chan] = 16;
		decayTimer[chan] = 0;
	#endif
	
	*/
	
}


//-----------------------------------------------
// Start playing a score
//-----------------------------------------------
void SAATunesPlayScore (struct SAATunesContext *context, const uint8_t *score) {

    context->volume_present = ASSUME_VOLUME;

    context->score_start = score;

	// look for the optional file header
	memcpy(&context->file_header, score, sizeof(struct file_hdr_t)); // copy possible header from PROGMEM to RAM
	if (context->file_header.id1 == 'P' && context->file_header.id2 == 't') { // validate it
        context->volume_present = context->file_header.f1 & HDR_F1_VOLUME_PRESENT;
        context->score_start += context->file_header.hdr_length; // skip the whole header
	}

    context->score_cursor = context->score_start;
    SAATunesStepScore(context);  /* execute initial commands */
    context->tune_playing = true; //Release the intterupt routine
}


/* Do score commands until a "wait" is found, or the score is stopped.
This is called initially from tune_playscore, but then is called
from the interrupt routine when waits expire.
*/

#define CMD_PLAYNOTE	0x90	/* play a note: low nibble is generator #, note is next byte */
#define CMD_INSTRUMENT  0xc0 /* change instrument; low nibble is generator #, instrument is next byte */
#define CMD_STOPNOTE	0x80	/* stop a note: low nibble is generator # */
#define CMD_RESTART	0xe0	/* restart the score from the beginning */
#define CMD_STOP	0xf0	/* stop playing */
/* if CMD < 0x80, then the other 7 bits and the next byte are a 15-bit big-endian number of msec to wait */

static void SAATunesStepScore (struct SAATunesContext *context)
{
    uint8_t cmd, opcode, chan, note, vol;
    unsigned duration;

    while (1) {
        cmd = *context->score_cursor;
        context->score_cursor++;

        if (cmd < 0x80)
        { /* wait count in msec. */
            duration = ((unsigned)cmd << 8) | *context->score_cursor;
            context->score_cursor++;
            context->wait_toggle_count = duration; //((unsigned long) wait_timer_frequency2 * duration + 500) / 1000

            if (context->wait_toggle_count == 0)
                context->wait_toggle_count = 1;
            break;
        }
		
        opcode = cmd & 0xf0;
        chan = cmd & 0x0f; //Should erase the low nibble?

        if (opcode == CMD_STOPNOTE) { /* stop note */
            SAATunesStopNote (context, chan);
        }
        else if (opcode == CMD_PLAYNOTE)
        { /* play note */
            note = *context->score_cursor;
            context->score_cursor++;

#if DO_VOLUME
            if (context->volume_present) {
                vol = *context->score_cursor;
                context->score_cursor++;
            }
            else {
                vol = 127;
            }
            SAATunesPlayNote(context, chan, note, vol);
#else
            if (context->volume_present)
                ++context->score_cursor; // skip volume byte

            SAATunesPlayNote (context, chan, note, 127);
#endif
        }
        else if (opcode == CMD_RESTART) { /* restart score */
            context->score_cursor = context->score_start;
        }
        else if (opcode == CMD_STOP) { /* stop score */
            context->tune_playing = false;
            break;
        }
    }
}


//-----------------------------------------------
// Stop playing a score
//-----------------------------------------------

void SAATunesStopScore (struct SAATunesContext *context) {
    int i;

    for (i=0; i<6; ++i)
        SAATunesStopNote(context, i);

    context->tune_playing = false;
}


//Timer stuff
// Interrupt is called once a millisecond
void SAATunesTick(struct SAATunesContext *context)
{
	//Begin new note code
	if (context->tune_playing && context->wait_toggle_count && --context->wait_toggle_count == 0) {
        // end of a score wait, so execute more score commands
        SAATunesStepScore(context);
    }

	#if DO_DECAY
		//Note that this (the for loop) isn't ideal. Wish I didn't have to check every one, somehow.
		for(uint8_t x = 0; x <= 5; x++){
		
			if(context->doing_decay[x] == true){
			
				//Add time to the timer
                context->decay_timer[x] += 1;
			
				if(context->decay_timer[x] >= context->decay_rate){ //Check to see if we've met or exceeded the decay rate
					//Set the Address to the volume channel
					saa_port_reg = x;

					//Read the current volume, subtract one
					uint8_t volume = context->decay_volume[x];
                    context->decay_volume[x] -= 1;
				
					//Then, write modified volume
					saa_port_val = (volume << 4) | volume;

                    context->decay_timer[x] = 0;
				
					if (volume == 0)
                    {
                        context->doing_decay[x] = false;
					}
				}
			
			}
		}
	#endif
}





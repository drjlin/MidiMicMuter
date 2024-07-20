// MIDI_decoder for ATtiny85
//
// This code is a modification of Nick Gammons Midi decoder for ATtiny85, as described and released here:
// https://arduino.stackexchange.com/questions/14054/interfacing-an-attiny85-with-midi-via-software-serial?answertab=active#tab-top
//
// Original Version fpr Arduino Uno (ny Nick Gammon): http://www.gammon.com.au/forum/?id=12746
//
// Descrption:
// The decoder listens to control change MUTE_CC on Midi channel MUTE_CHANNEL and switches an LED and an analog switch connected to pins 7 and 6, respectively.
// Received values between 0 and 63 put both pins LOW and values between 64 and 127 put the two pins HIGH.
//  
// ATMEL ATTINY 25/45/85 / ARDUINO
// Pin 1 is /RESET
//
//                  +-\/-+
// Ain0 (D 5) PB5  1|    |8  Vcc
// Ain3 (D 3) PB3  2|    |7  PB2 (D 2) Ain1 
// Ain2 (D 4) PB4  3|    |6  PB1 (D 1) pwm1
//            GND  4|    |5  PB0 (D 0) pwm0
//                  +----+


#include <SoftwareSerial.h>

const byte LED = 2;   // pin 7 on ATtiny85
const byte SWITCH = 1;  // pin 6

// Plug MIDI into pin D2 (MIDI in serial) (pin 7 on ATtiny85)

SoftwareSerial midi (0, 4);  // Rx, Tx

// Channel and CC to listen to
const byte MUTE_CHANNEL = 7;
const byte MUTE_CC = 63;

const int noRunningStatus = -1;

int runningStatus;
unsigned long lastRead;
byte lastCommand;

void setup() {
  //  Set MIDI baud rate:
  midi.begin(31250);
  runningStatus = noRunningStatus;
  pinMode (LED, OUTPUT);
} // end of setup

void RealTimeMessage (const byte msg)
  {
    // ignore realtime messages
  } // end of RealTimeMessage

// get next byte from serial (blocks)
int getNext ()
  {

  if (runningStatus != noRunningStatus)
    {
    int c = runningStatus;
    // finished with look ahead
    runningStatus = noRunningStatus;
    return c;
    }

  while (true)
    {
    while (midi.available () == 0)
      {}
    byte c = midi.read ();
    if (c >= 0xF8)  // RealTime messages
      RealTimeMessage (c);
    else
      return c;
    }
  } // end of getNext

const char * notes [] = { "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B " };

byte note;
byte octave;
byte velocity;
byte cc_message;
byte cc_param;

// interpret a note in terms of note name and octave
void getNote ()
  {
  note = getNext ();
  octave = note / 12;
  note %= 12;
  }  // end of getNote

void getVelocity ()
  {
  velocity = getNext ();
  }

// show a control message 
void showControlMessage ()
  {
  cc_message =  getNext () & 0x7F;
  cc_param = getNext ();
  }  // end of showControlMessage

// read a system exclusive message 
void showSystemExclusive ()
  {
  int count = 0;
  while (true)
    {
    while (midi.available () == 0)
      {}
    byte c = midi.read ();
    if (c >= 0x80)
      {
      runningStatus = c;
      return;  
      }

    } // end of reading until all system exclusive done
  }  // end of showSystemExclusive

void loop() 
{
  byte c = getNext ();
  unsigned int parameter;

  if (((c & 0x80) == 0) && (lastCommand & 0x80))
    {
    runningStatus = c;
    c = lastCommand; 
    }

  // channel is in low order bits
  int channel = (c & 0x0F) + 1;

  // messages start with high-order bit set
  if (c & 0x80)
    {
    lastCommand = c;
    switch ((c >> 4) & 0x07)
      {
      case 0:   // Note off
        getNote ();
        getVelocity ();
        // only for testing with piano - comment in production
        digitalWrite (LED, LOW);
        digitalWrite (SWITCH, LOW);
        // end
        break;

      case 1:   // Note on
        getNote ();
        getVelocity ();
        // only for testing with piano - comment in production
        if (velocity == 0) {
          digitalWrite (LED, LOW);
          digitalWrite (SWITCH, LOW);
        }
        else
          if ((channel == 1) && (note == 0) && (octave == 3)) {
            digitalWrite (LED, HIGH);
            digitalWrite (SWITCH, HIGH);
          }
        // end
        break;

      case 2:  // Polyphonic pressure
        getNote ();
        parameter = getNext ();  // pressure
        break;

      case 3: // Control change
        showControlMessage ();
        if ((channel == MUTE_CHANNEL) && (cc_message == MUTE_CC) && (cc_param < 64)) {
          digitalWrite (LED, LOW);
          digitalWrite (SWITCH, LOW);
        }
        else if ((channel == MUTE_CHANNEL) && (cc_message == MUTE_CC) && (cc_param >= 64)) {
          digitalWrite (LED, HIGH);
          digitalWrite (SWITCH, HIGH);
        }
        break;

      case 4:  // Program change
        parameter = getNext ();  // program
        break;

      case 5: // After-touch pressure
        parameter = getNext (); // value
        break;

      case 6: // Pitch wheel change 
        parameter = getNext () |  getNext () << 7; 
        break;

      case 7:  // system message
        {
        lastCommand = 0;           // these won't repeat I don't think
        switch (c & 0x0F)
          {
          case 0: // Exclusive
            parameter = getNext (); // vendor ID
            showSystemExclusive ();
            break;

          case 1: // Time code
            parameter = getNext () ;
            break;

          case 2:  // Song position 
            parameter = getNext () |  getNext () << 7; 
            break;

          case 3: // Song select
            parameter = getNext () ;  // song
            break;

          case 4:    // reserved
          case 5:    // reserved
          case 6:    // tune request
          case 7:    // end of exclusive
          case 8:    // timing clock
          case 9:    // reserved
          case 10:   // start
          case 11:   // continue
          case 12:   // stop
          case 13:   // reserved
          case 14:   // active sensing
          case 15:   // reset
             break;

          }  // end of switch on system message  

        }  // end system message
        break;
      }  // end of switch
    }  // end of if
  else
    {
    // unexpected, ignore 
    }

}  // end of loop
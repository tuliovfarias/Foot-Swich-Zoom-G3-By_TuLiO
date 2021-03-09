#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

void setup() {
  pinMode(8, OUTPUT);
  MIDI.begin(MIDI_CHANNEL_OMNI);
}

void loop() {
  digitalWrite(8, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);                       // wait for a second
  MIDI.sendProgramChange(2,1);
  digitalWrite(8, LOW);    // turn the LED off by making the voltage LOW
  delay(500);  
}

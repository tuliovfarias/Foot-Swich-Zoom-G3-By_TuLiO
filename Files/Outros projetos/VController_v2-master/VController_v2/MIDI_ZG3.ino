// Please read VController_v2.ino for information about the license and authors

// ******************************** MIDI messages and functions for the Zoom G3 ********************************
uint8_t ZG3_MIDI_port = 0;
#define ZG3_NUMBER_OF_FX_SLOTS 6 // The Zoom G3 has 6 FX slots
uint16_t ZG3_FX[ZG3_NUMBER_OF_FX_SLOTS]; //Memory location for the 6 FX

// ********************************* Section 1: Zoom G3 SYSEX messages ********************************************
// Documentation of Zoom sysex has been moved to http://www.vguitarforums.com/smf/index.php?topic=4329.msg131444#msg131444 (The ZOOM G3 v2 MIDI specification)
// The relevant messages are repeated here
// 1) The Zoom responds to an MIDI identity request message with F0 7E 00 (Device ID) 06 02 52 (Manufacturing ID for Zoom) 5A (model number G3) 00  00 00 32 2E 31 30 F7
#define ZG3_MODEL_NUMBER 0x5A
// 2) The editor keeps sending F0 52 00 5A 50 F7. The G3 does not seem to respond to it. But it may signal editor mode on.
#define ZG3_EDITOR_MODE_ON 0x50
// 3) If the editor sends F0 52 00 5A 33 F7, the G3 responds with the current Bank number (CC00 and CC32) and the current Program number (PC)
#define ZG3_REQUEST_PATCH_NUMBER 0x33
// 4) If the editor sends F0 52 00 5A 29 F7, the G3 responds with the current patch in 110 bytes with comaand number 28. Byte 0x61 - 0x6B contain the patch name. with a strange 0 at position 0x65
#define ZG3_REQUEST_CURRENT_PATCH 0x29
// 5) The editor then reads all individual patches by sending F0 52 00 5A 09 00 00 {00-63} (patch number) F7.
//    The G3 responds with 120 bytes with command number 08. Byte 0x66 - 0x70 contain the patch name. with an "overflow byte" at position 0x6A.
// 6) At the end the editor sends F0 52 00 5A 51 F7 and communication seems to stop.
#define ZG3_EDITOR_MODE_OFF 0x51
// 7) Switch effect on/off:
//    Switch on effect 1: F0 52 00 5A 31 00 00 01 (on) 00 F7, switch off: F0 52 00 5A 31 00 00 00 (off) 00 F7
//    Switch on effect 2: F0 52 00 5A 31 01 00 01 (on) 00 F7 switch off: F0 52 00 5A 31 01 00 00 (off) 00 F7. etc. sixth byte changes consistent for the effect
//    Same message is sent for changing a knob on the G3, but byte 7 is not 0x00 
// 8) Tempo. set bpm=40: F0 52 00 5A 31 06 08 28 00 F7 => 0x28 = 40, bpm=240: F0 52 00 5A 31 06 08 7A 01 F7 => 0x7A = 122, 122+128 = 240, so the last two bytes are the tempo.

void check_SYSEX_in_ZG3(const unsigned char* sxdata, short unsigned int sxlength) { // Check incoming sysex messages from ZG3. Called from MIDI:OnSysEx/OnSerialSysEx

  // Check if it is a message from a ZOOM G3
  if ((sxdata[2] == ZG3_device_id) && (sxdata[3] == ZG3_MODEL_NUMBER)) {
    // Check if it is the patch data
    if ((sxdata[4] == 0x08) && (sxlength == 120)) {
      // Read the patch name - needs to be read in two parts, because the fifth byte in the patchname string is a 0.
      for (uint8_t count = 0; count < 4; count++) { // Read the first four characters of the name
        SP[Current_switch].Label[count] = static_cast<char>(sxdata[count + 0x66]); //Add ascii character to the SP.Label String
      }
      for (uint8_t count = 4; count < 10; count++) { // Read the last six characters of the name
        SP[Current_switch].Label[count] = static_cast<char>(sxdata[count + 0x67]); //Add ascii character to the SP.Label String
      }
      // Remember the 6 FX slot types:
      // The 8th bit of the FX type is stored in an "overflow byte". Here we extract the relevant bit from the overflow byte, move it in position and add the byte that contains bit 1-7.
      // Check out my ZOOM G3 v2 MIDI specification online for more information
      SP[Current_switch].Target_byte1 = ((sxdata[10] & B01000000) << 1) + sxdata[11];
      SP[Current_switch].Target_byte2 = ((sxdata[18] & B00000010) << 6) + sxdata[24];
      SP[Current_switch].Target_byte3 = ((sxdata[34] & B00001000) << 4) + sxdata[38];
      SP[Current_switch].Target_byte4 = ((sxdata[50] & B00100000) << 2) + sxdata[52];
      SP[Current_switch].Target_byte5 = ((sxdata[58] & B00000001) << 7) + sxdata[65];
      SP[Current_switch].Target_byte6 = ((sxdata[74] & B00000100) << 5) + sxdata[79];

      if (SP[Current_switch].PP_number == ZG3_patch_number) {
        ZG3_patch_name = SP[Current_switch].Label; // Load patchname when it is read
        update_main_lcd = true; // And show it on the main LCD
      }
      DEBUGMSG(SP[Current_switch].Label);
      PAGE_request_next_switch();
    }
    if ((sxdata[4] == 0x28) && (sxlength == 110)) {
      // These codes are the same as above. Only the sxdata postitions are all minus 5.
      ZG3_FX[0] = ((sxdata[5] & B01000000) << 1) + sxdata[6];
      ZG3_FX[1] = ((sxdata[13] & B00000010) << 6) + sxdata[19];
      ZG3_FX[2] = ((sxdata[29] & B00001000) << 4) + sxdata[33];
      ZG3_FX[3] = ((sxdata[45] & B00100000) << 2) + sxdata[47];
      ZG3_FX[4] = ((sxdata[53] & B00000001) << 7) + sxdata[60];
      ZG3_FX[5] = ((sxdata[69] & B00000100) << 5) + sxdata[74];
      
      // Read the patch name - needs to be read in two parts, because the fifth byte in the patchname string is a 0.
      for (uint8_t count = 0; count < 4; count++) { // Read the first four characters of the name
        ZG3_patch_name[count] = static_cast<char>(sxdata[count + 0x61]); //Add ascii character to the SP.Label String
      }
      for (uint8_t count = 4; count < 10; count++) { // Read the last six characters of the name
        ZG3_patch_name[count] = static_cast<char>(sxdata[count + 0x62]); //Add ascii character to the SP.Label String
      }
      
      update_page = FULL; // Need to update everything, otherwise displays go blank on detection of the G3.
    }
    if ((sxdata[4] == 0x31) && (sxdata[6] == 0x00) && (sxlength == 10)) { // Check for effect switched off on the ZOOM G5.
      uint8_t index = sxdata[5];
      if (index < 6) ZG3_FX[index] =  ZG3_FX[index] & B11111110; //Make a zero of bit 1 - this will switch off the effect
      PAGE_load_current(false);
    }
  }
}

void check_PC_in_ZG3(byte channel, byte program) {  // Check incoming PC messages from Zoom G3. Called from MIDI:OnProgramChange

  // Check the source by checking the channel
  if ((Current_MIDI_port == ZG3_MIDI_port) && (channel == ZG3_MIDI_channel)) { // Zoom G3 outputs a program change
    if (ZG3_patch_number != program) {
      ZG3_patch_number = program;
      ZG3_page_check();
      ZG3_do_after_patch_selection();
    }
  }
}

void check_CC_in_ZG3(byte channel, byte control, byte value) {  // Check incoming CC messages from Zoom G3
  // Check the source by checking the channel
  if (channel == ZG3_MIDI_channel) { // GP10 outputs a control change message
  }
}

// Detection of Zoom G3

void ZG3_check_detect() { // Started from MIDI/MIDI_check_for_devices()
  if (ZG3_connected) {
    if (ZG3_not_detected >= ZG3_MAX_NOT_DETECTED) ZG3_disconnect();
    ZG3_not_detected++;
  }
}

void ZG3_identity_check(const unsigned char* sxdata, short unsigned int sxlength) {
  // Check if it is a Zoom G3
  if ((sxdata[6] == ZG3_MODEL_NUMBER) && (sxdata[7] == 0x00)) {
    ZG3_not_detected = 0;
    if (ZG3_connected == false) ZG3_connect(sxdata[2]); //Byte 2 contains the correct device ID
    }
}

void ZG3_connect(uint8_t device_id) {
    ZG3_connected = true;
    ZG3_on = true;
    LCD_show_status_message("Zoom G3 connected");
    ZG3_device_id = device_id; //Byte 2 contains the correct device ID
    ZG3_MIDI_port = Current_MIDI_port; // Set the correct MIDI port for this device
    DEBUGMSG("Zoom G3 connected on MIDI port " + String(Current_MIDI_port));
    write_ZG3(ZG3_EDITOR_MODE_ON); // Put the Zoom G3 in EDITOR mode
    write_ZG3(ZG3_REQUEST_PATCH_NUMBER);
    write_ZG3(ZG3_REQUEST_CURRENT_PATCH); //This will update the FX buttons
    //update_page = FULL;
}

void ZG3_disconnect() {
  ZG3_connected = false;
  ZG3_on = false;
  LCD_show_status_message("Zoom G3 offline   ");
  DEBUGMSG("Zoom G3 offline");
  update_page = FULL;
  update_main_lcd = true;
}

void write_ZG3(uint8_t message) {
  uint8_t sysexmessage[6] = {0xF0, 0x52, ZG3_device_id, ZG3_MODEL_NUMBER, message, 0xF7};
  //ZG3_check_sysex_delay();
  if (ZG3_MIDI_port == USBMIDI_PORT) usbMIDI.sendSysEx(6, sysexmessage);
  if (ZG3_MIDI_port == MIDI1_PORT) MIDI1.sendSysEx(5, sysexmessage);
  if (ZG3_MIDI_port == MIDI2_PORT) MIDI2.sendSysEx(5, sysexmessage);
  if (ZG3_MIDI_port == MIDI3_PORT) MIDI3.sendSysEx(5, sysexmessage);
  MIDI_debug_sysex(sysexmessage, 6, "out(Zoom G3)");
}

void ZG3_request_patch(uint8_t number) { //Will request the complete patch information from the Zoom G3 (will receive 120 bytes as an answer)
  uint8_t sysexmessage[9] = {0xF0, 0x52, ZG3_device_id, ZG3_MODEL_NUMBER, 0x09, 0x00, 0x00, number, 0xF7};
  //ZG3_check_sysex_delay();
  if (ZG3_MIDI_port == USBMIDI_PORT) usbMIDI.sendSysEx(9, sysexmessage);
  if (ZG3_MIDI_port == MIDI1_PORT) MIDI1.sendSysEx(8, sysexmessage);
  if (ZG3_MIDI_port == MIDI2_PORT) MIDI2.sendSysEx(8, sysexmessage);
  if (ZG3_MIDI_port == MIDI3_PORT) MIDI3.sendSysEx(8, sysexmessage);
  MIDI_debug_sysex(sysexmessage, 9, "out(Zoom G3)");
}

void ZG3_set_FX_state(uint8_t number, uint8_t state) { //Will set an effect on or off
  uint8_t sysexmessage[10] = {0xF0, 0x52, ZG3_device_id, ZG3_MODEL_NUMBER, 0x31, number, 0x00, state, 0x00, 0xF7}; // F0 52 00 5A 31 00 (FX) 00 01 (on) 00 F7
  //ZG3_check_sysex_delay();
  if (ZG3_MIDI_port == USBMIDI_PORT) usbMIDI.sendSysEx(10, sysexmessage);
  if (ZG3_MIDI_port == MIDI1_PORT) MIDI1.sendSysEx(9, sysexmessage);
  if (ZG3_MIDI_port == MIDI2_PORT) MIDI2.sendSysEx(9, sysexmessage);
  if (ZG3_MIDI_port == MIDI3_PORT) MIDI3.sendSysEx(9, sysexmessage);
  MIDI_debug_sysex(sysexmessage, 10, "out(Zoom G3)");
}

void ZG3_send_bpm() { //Will change the bpm to the specified value
  uint8_t sysexmessage[10] = {0xF0, 0x52, ZG3_device_id, ZG3_MODEL_NUMBER, 0x31, 0x06, 0x08, (uint8_t)(bpm % 128), (uint8_t)(bpm / 128), 0xF7}; // F0 52 00 5A 31 06 08 7A 01 F7
  //ZG3_check_sysex_delay();
  if (ZG3_MIDI_port == USBMIDI_PORT) usbMIDI.sendSysEx(10, sysexmessage);
  if (ZG3_MIDI_port == MIDI1_PORT) MIDI1.sendSysEx(9, sysexmessage);
  if (ZG3_MIDI_port == MIDI2_PORT) MIDI2.sendSysEx(9, sysexmessage);
  if (ZG3_MIDI_port == MIDI3_PORT) MIDI3.sendSysEx(9, sysexmessage);
  MIDI_debug_sysex(sysexmessage, 10, "out(Zoom G3)");
}

void ZG3_SendProgramChange(uint8_t new_patch) {

  //if (new_patch == ZG3_patch_number) ZG3_unmute();
  ZG3_patch_number = new_patch;
  MIDI_send_PC(new_patch, ZG3_MIDI_channel, ZG3_MIDI_port);
  DEBUGMSG("out(ZG3) PC" + String(new_patch)); //Debug
  //GR55_mute();
  //VG99_mute();
  //ZG3_assign_read = false; // Assigns should be read again
  ZG3_do_after_patch_selection();
}

void ZG3_do_after_patch_selection() {
  //ZG3_request_onoff = false;
  ZG3_on = true;
  if (SEND_GLOBAL_TEMPO_AFTER_PATCH_CHANGE == true) {
    //delay(5); // ZG3 misses send bpm command...
    //ZG3_send_bpm();
  }
  Current_patch_number = ZG3_patch_number; // the patch name
  Current_device = ZG3;
  update_LEDS = true;
  update_main_lcd = true;
  update_page = PAR_ONLY;
  //ZG3_request_guitar_switch_states();
  //EEPROM.write(EEPROM_ZG3_PATCH_NUMBER, ZG3_patch_number);

}

void ZG3_relsel_load(uint8_t sw, uint8_t bank_position, uint8_t bank_size) {
  if (ZG3_bank_selection_active == false) ZG3_bank_number = (ZG3_patch_number / bank_size);
  uint8_t new_patch = (ZG3_bank_number * bank_size) + bank_position;
  if (new_patch > ZG3_PATCH_MAX) new_patch = new_patch - ZG3_PATCH_MAX - 1 + ZG3_PATCH_MIN;
  ZG3_patch_load(sw, new_patch); // Calculate the patch number for this switch
}

void ZG3_patch_load(uint8_t sw, uint8_t number) {
  SP[sw].Colour = ZG3_PATCH_COLOUR;
  SP[sw].PP_number = number;
}

void ZG3_patch_select(uint16_t new_patch) {

  //if (new_patch == ZG3_patch_number) ZG3_select_switch();
  ZG3_SendProgramChange(new_patch);
  ZG3_bank_selection_active = false;

}

void ZG3_bank_updown(bool updown, uint8_t bank_size) {
  uint8_t bank_number = (ZG3_patch_number / bank_size);

  if (ZG3_bank_selection_active == false) {
    ZG3_bank_selection_active = true;
    ZG3_bank_number = bank_number; //Reset the bank to current patch
    ZG3_bank_size = bank_size;
  }
  // Perform bank up:
  if (updown == UP) {
    if (ZG3_bank_number >= (ZG3_PATCH_MAX / bank_size)) ZG3_bank_number = (ZG3_PATCH_MIN / bank_size); // Check if we've reached the top
    else ZG3_bank_number++; //Otherwise move bank up
  }
  // Perform bank down:
  if (updown == DOWN) {
    if ((ZG3_bank_number * bank_size) <= ZG3_PATCH_MIN) ZG3_bank_number = ZG3_PATCH_MAX / bank_size; // Check if we've reached the bottom
    else ZG3_bank_number--; //Otherwise move bank down
  }

  if (ZG3_bank_number == bank_number) ZG3_bank_selection_active = false; //Check whether were back to the original bank

  update_page = FULL; //Re-read the patchnames for this bank
}

void ZG3_page_check() { // Checks if the current patch is on the page and will reload the page if not
  bool onpage = false;
  for (uint8_t s = 0; s < NUMBER_OF_SWITCHES; s++) {
    if ((SP[s].Type == ZG3_RELSEL) && (SP[s].PP_number == ZG3_patch_number)) {
      onpage = true;
      ZG3_patch_name = SP[s].Label; // Set patchname correctly
      ZG3_Remember_FXs(s); // Copy the FX states
    }
  }
  if (!onpage) update_page = FULL;
}

void ZG3_display_patch_number_string() {
  if (ZG3_bank_selection_active == false) {
    ZG3_number_format(ZG3_patch_number, Current_patch_number_string);
  }
  else {
    String start_number1, end_number1;
    ZG3_number_format(ZG3_bank_number * ZG3_bank_size, start_number1);
    ZG3_number_format((ZG3_bank_number + 1) * ZG3_bank_size - 1, end_number1);
    Current_patch_number_string = Current_patch_number_string + start_number1 + "-" + end_number1;
  }
}

void ZG3_number_format(uint8_t number, String &Output) {
  char BankChar = 65 + (number / 10);
  Output = Output + BankChar + String(number % 10);
}

void ZG3_Remember_FXs(uint8_t Sw) {
  // Remember the 6 FX slot types in the ZG3_FX array:
  ZG3_FX[0] = SP[Sw].Target_byte1;
  ZG3_FX[1] = SP[Sw].Target_byte2;
  ZG3_FX[2] = SP[Sw].Target_byte3;
  ZG3_FX[3] = SP[Sw].Target_byte4;
  ZG3_FX[4] = SP[Sw].Target_byte5;
  ZG3_FX[5] = SP[Sw].Target_byte6;
}

// Toggle Zoom G3 stompbox parameter
void ZG3_FX_press(uint8_t Sw, uint8_t Cmd, uint8_t number) {

  // Send sysex MIDI command to Zoom G3
  ZG3_set_FX_state(SP[Sw].PP_number - 1, SP[Sw].State - 1);

  // Show message
  LCD_show_status_message(SP[Sw].Label);

  //PAGE_load_current(false); // To update the other switch states, we re-load the current page
}

void ZG3_FX_set_type_and_state(uint8_t Sw) {

  //Effect type and state are stored in the ZG3_FX array
  uint8_t index = SP[Sw].PP_number - 1; //The index for the ZG3_FX_array

  //Effect can have three states: 0 = no effect, 1 = off, 2 = on
  if (ZG3_FX[index] & 0x01) SP[Sw].State = 2; //Effect on
  else SP[Sw].State = 1; // Effect off

  // Define array for G3 effeect names and colours
  struct ZG3_FX_type_struct { // Combines all the data we need for controlling a parameter in a device
    char Name[17]; // The name for the label
    uint8_t Colour; // The colour for this effect.
  };

#define ZG3_NUMBER_OF_FX 120

  const PROGMEM ZG3_FX_type_struct ZG3_FX_types[ZG3_NUMBER_OF_FX] = { // Table with the name and colour for every effect of the Zoom G3
    {"M-FILTER", FX_FILTER_COLOUR}, // 01
    {"THE VIBE", FX_MODULATE_COLOUR}, // 02
    {"Z-ORGAN", FX_MODULATE_COLOUR}, // 03
    {"SLICER", FX_FILTER_COLOUR}, // 04
    {"PHASE DELAY", FX_DELAY_COLOUR}, // 05
    {"FILTER DELAY", FX_DELAY_COLOUR}, // 06
    {"PITCH DELAY", FX_DELAY_COLOUR}, // 07
    {"STEREO DELAY", FX_DELAY_COLOUR}, // 08
    {"BIT CRUSH", FX_FILTER_COLOUR}, // 09
    {"BOMBER", FX_FILTER_COLOUR}, // 10
    {"DUO-PHASE", FX_MODULATE_COLOUR}, // 11
    {"MONOSYNTH", FX_MODULATE_COLOUR}, // 12
    {"SEQ FILTER", FX_MODULATE_COLOUR}, // 13
    {"RANDOM FLTR", FX_MODULATE_COLOUR}, // 14
    {"WARP PHASER", FX_MODULATE_COLOUR}, // 15
    {"TRIGGER HOLD DLY", FX_DELAY_COLOUR}, // 16
    {"CHOR+DLY", FX_DELAY_COLOUR}, // 17
    {"CHOR+REV", FX_REVERB_COLOUR}, // 18
    {"DLY+REV", FX_REVERB_COLOUR}, // 19
    {"COMP PHASER", FX_MODULATE_COLOUR}, // 20
    {"COMP AWAH", FX_MODULATE_COLOUR}, // 21
    {"FLANGER V-CHO", FX_MODULATE_COLOUR}, // 22
    {"COMP OD", FX_FILTER_COLOUR}, // 23
    {"COMP", FX_FILTER_COLOUR}, // 24 -Start
    {"Rock COMP", FX_FILTER_COLOUR}, // 25
    {"M COMP", FX_FILTER_COLOUR}, // 26
    {"SLOW ATTACK", FX_FILTER_COLOUR}, // 27
    {"ZNR", FX_FILTER_COLOUR}, // 28
    {"NOISE GATE", FX_FILTER_COLOUR}, // 29
    {"DIRTY GATE", FX_AMP_COLOUR}, // 30
    {"GRAPHIC EQ", FX_FILTER_COLOUR}, // 31
    {"PARA EQ", FX_FILTER_COLOUR}, // 32
    {"COMB FILTER", FX_FILTER_COLOUR}, // 33
    {"AUTO WAH", FX_FILTER_COLOUR}, // 34
    {"RESONANCE", FX_FILTER_COLOUR}, // 35
    {"STEP", FX_FILTER_COLOUR}, // 36
    {"CRY", FX_FILTER_COLOUR}, // 37
    {"OCTAVE", FX_PITCH_COLOUR}, // 38
    {"TREMOLO", FX_MODULATE_COLOUR}, // 39
    {"PHASER", FX_MODULATE_COLOUR}, // 40
    {"RING MOD", FX_MODULATE_COLOUR}, // 41
    {"CHORUS", FX_MODULATE_COLOUR}, // 42
    {"DETUNE", FX_MODULATE_COLOUR}, // 43
    {"VINTAGE CE", FX_MODULATE_COLOUR}, // 44
    {"STEREO CHORUS", FX_MODULATE_COLOUR}, // 45
    {"ENSEMBLE", FX_MODULATE_COLOUR}, // 46
    {"VINTAGE FLANGER", FX_MODULATE_COLOUR}, // 47
    {"DYNO FLANGER", FX_MODULATE_COLOUR}, // 48
    {"VIBRATO", FX_MODULATE_COLOUR}, // 49
    {"PITH SHIFT", FX_MODULATE_COLOUR}, // 50
    {"BEND CHORUS", FX_MODULATE_COLOUR}, // 51
    {"MONO PITCH", FX_PITCH_COLOUR}, // 52
    {"HPS", FX_PITCH_COLOUR}, // 53
    {"DELAY", FX_DELAY_COLOUR}, // 54
    {"TAPE ECHO", FX_DELAY_COLOUR}, // 55
    {"MOD DELAY", FX_DELAY_COLOUR}, // 56
    {"ANALOG DELAY", FX_DELAY_COLOUR}, // 57
    {"REVERSE DELAY", FX_DELAY_COLOUR}, // 58
    {"MULTI TAP DELAY", FX_DELAY_COLOUR}, // 59
    {"DYNA DELAY", FX_DELAY_COLOUR}, // 60
    {"HALL", FX_REVERB_COLOUR}, // 61
    {"ROOM", FX_REVERB_COLOUR}, // 62
    {"TILED ROOM", FX_REVERB_COLOUR}, // 63
    {"SPRING", FX_REVERB_COLOUR}, // 64
    {"ARENA REVERB", FX_REVERB_COLOUR}, // 65
    {"EARLY REFLECTION", FX_REVERB_COLOUR}, // 66
    {"AIR", FX_REVERB_COLOUR}, // 67
    {"PEDAL VX", FX_FILTER_COLOUR}, // 68
    {"PEDAL CRY", FX_FILTER_COLOUR}, // 69
    {"PEDAL PITCH", FX_PITCH_COLOUR}, // 70
    {"PEDAL MN PIT", FX_PITCH_COLOUR}, // 71
    {"BOOSTER", FX_DIST_COLOUR}, // 72
    {"OVERDRIVE", FX_DIST_COLOUR}, // 73
    {"TUBE SCREAM", FX_DIST_COLOUR}, // 74
    {"GOVERNOR", FX_DIST_COLOUR}, // 75
    {"DIST+", FX_DIST_COLOUR}, // 76
    {"DIST 1", FX_DIST_COLOUR}, // 77
    {"SQUEAK", FX_DIST_COLOUR}, // 78
    {"FUZZ SMILE", FX_DIST_COLOUR}, // 79
    {"GREAT MUFF", FX_DIST_COLOUR}, // 80
    {"METAL WORLD", FX_DIST_COLOUR}, // 81
    {"HOTBOX", FX_DIST_COLOUR}, // 82
    {"Z WILD", FX_DIST_COLOUR}, // 83
    {"LEAD ZOOM9002", FX_DIST_COLOUR}, // 84
    {"EXTREME DIST", FX_DIST_COLOUR}, // 85
    {"ACOUSTIC", FX_AMP_COLOUR}, // 86
    {"Z CLEAN", FX_AMP_COLOUR}, // 87
    {"Z MP1", FX_AMP_COLOUR}, // 88
    {"BOTTOM", FX_AMP_COLOUR}, // 89
    {"Z DREAM", FX_AMP_COLOUR}, // 90
    {"Z SCREAM", FX_AMP_COLOUR}, // 91
    {"Z NEOS", FX_AMP_COLOUR}, // 92
    {"FD COMBO", FX_AMP_COLOUR}, // 93
    {"VX COMBO", FX_AMP_COLOUR}, // 94
    {"US BLUES", FX_AMP_COLOUR}, // 95
    {"BG CRUNCH", FX_AMP_COLOUR}, // 96
    {"HW STACK", FX_AMP_COLOUR}, // 97
    {"TANGERINE", FX_AMP_COLOUR}, // 98
    {"MS CRUNCH", FX_AMP_COLOUR}, // 99
    {"MS DRIVE", FX_AMP_COLOUR}, // 100
    {"BG DRIVE", FX_AMP_COLOUR}, // 101
    {"DZ DRIVE", FX_AMP_COLOUR}, // 102
    {"TW ROCK", FX_AMP_COLOUR}, // 103
    {"MATCH30", FX_AMP_COLOUR}, // 104
    {"FD VIBRO", FX_AMP_COLOUR}, // 105
    {"HD REVERB", FX_AMP_COLOUR}, // 106
    {"FLANGER", FX_MODULATE_COLOUR}, // 107
    {"---", 0}, // 108
    {"TONE CITY", FX_AMP_COLOUR}, // 109
    {"B-BREAKER", FX_AMP_COLOUR}, // 110
    {"BGN DRIVE", FX_AMP_COLOUR}, // 111
    {"DELUXE R", FX_AMP_COLOUR}, // 112
    {"ALIEN", FX_AMP_COLOUR}, // 113
    {"REVO 1", FX_AMP_COLOUR}, // 114
    {"CAR DRIVE", FX_AMP_COLOUR}, // 115
    {"MS 1959", FX_AMP_COLOUR}, // 116
    {"VX JIMI", FX_AMP_COLOUR}, // 117
    {"118", 0}, // 118
    {"119", 0}, // 119
    {"120", 0}, // 120
  };

  uint8_t FX_type = ZG3_FX[index] >> 1; //The FX type is stored in bit 1-7.
  String msg = ZG3_FX_types[FX_type].Name;  //Find the patch name in the ZG3_FX_types array
  LCD_set_label(Sw, msg);
  SP[Sw].Colour = ZG3_FX_types[FX_type].Colour; //Find the LED colour in the ZG3_FX_types array
}

// Please read VController_v2.ino for information about the license and authors

// ******************************** MIDI messages and functions for the Boss GP-10 ********************************

// ********************************* Section 1: GP10 SYSEX messages ********************************************

//Messages are abbreviated to just the address and the data bytes. Checksum is calculated automatically
//Example: {0xF0, 0x41, 0x10, 0x00, 0x00, 0x00, 0x05, 0x12, 0x7F, 0x00, 0x00, 0x01, 0x01, 0x7F, 0xF7} is reduced to 0x7F000001, 0x01

#define GP10_EDITOR_MODE_ON 0x7F000001, 0x01 //Gets the GP-10 spitting out lots of sysex data. Should be switched on, otherwise the tuner does not work
#define GP10_EDITOR_MODE_OFF 0x7F000001, 0x00
#define GP10_REQUEST_CURRENT_PATCH_NAME 0x20000000, 12 //Request 12 bytes for current patch name
#define GP10_REQUEST_PATCH_NUMBER 0x00000000, 1 //Request current patch number

#define GP10_TUNER_ON 0x7F000002, 0x02 // Changes the running mode of the GP-10 to Tuner - Got these terms from the VG-99 sysex manual.
#define GP10_TUNER_OFF 0x7F000002, 0x00 //Changes the running mode of the GP10 to play.
#define GP10_SOLO_ON 0x2000500B, 0x01
#define GP10_SOLO_OFF 0x2000500B, 0x00

#define GP10_TEMPO 0x20000801  // Accepts values from 40 bpm - 250 bpm
#define GP10_HARMONIST_KEY 0x20014001 // Sets the key for the harmonist, which is the only effect that is key related.

#define GP10_FOOT_VOL 0x20020803 // The address of the footvolume - values between 0 and 100
#define GP10_COSM_GUITAR_SW 0x20001000 // The address of the COSM guitar switch
#define GP10_NORMAL_PU_SW 0x20000804 // The address of the COSM guitar switch
bool GP10_request_onoff = false;

uint8_t GP10_MIDI_port = 0; // What port is the GP10 connected to (0 - 3)

#define GP10_SYSEX_DELAY_LENGTH 10 // time between sysex messages (in msec). 
unsigned long GP10sysexDelay = 0;

uint8_t GP10_FX_toggle_LED = 0; // The LED shown for the FX type button

// ********************************* Section 2: GP10 common MIDI in functions ********************************************

void check_SYSEX_in_GP10(const unsigned char* sxdata, short unsigned int sxlength) { // Check incoming sysex messages from GP10. Called from MIDI:OnSysEx/OnSerialSysEx

  // Check if it is a message from a GP-10
  if ((sxdata[2] == GP10_device_id) && (sxdata[3] == 0x00) && (sxdata[4] == 0x00) && (sxdata[5] == 0x00) && (sxdata[6] == 0x05) && (sxdata[7] == 0x12)) {
    uint32_t address = (sxdata[8] << 24) + (sxdata[9] << 16) + (sxdata[10] << 8) + sxdata[11]; // Make the address 32 bit

    // Check if it is the patch number
    if (address == 0x00000000) {
      if (GP10_patch_number != sxdata[12]) { //Right after a patch change the patch number is sent again. So here we catch that message.
        GP10_patch_number = sxdata[12];
        GP10_assign_read = false; // Assigns should be read again
        GP10_page_check();
        GP10_do_after_patch_selection();
        update_page = FULL;
      }
    }

    // Check if it is the current parameter
    if (address == SP[Current_switch].Address) {
      switch (SP[Current_switch].Type) {
        case GP10_PATCH:
        case GP10_RELSEL:
          for (uint8_t count = 0; count < 12; count++) {
            SP[Current_switch].Label[count] = static_cast<char>(sxdata[count + 12]); //Add ascii character to the SP.Label String
          }
          if (SP[Current_switch].PP_number == GP10_patch_number) {
            GP10_patch_name = SP[Current_switch].Label; // Load patchname when it is read
            update_main_lcd = true; // And show it on the main LCD
          }
          DEBUGMSG(SP[Current_switch].Label);
          PAGE_request_next_switch();
          break;

        case GP10_PARAMETER:
        case GP10_ASSIGN:
          //case GP10_ASSIGN:
          GP10_read_parameter(sxdata[12], sxdata[13]);
          PAGE_request_next_switch();
          break;
      }
    }

    // Check if it is the patch name (address: 0x20, 0x00, 0x00, 0x00)
    if (address == 0x20000000) {
      GP10_patch_name = "";
      for (uint8_t count = 12; count < 24; count++) {
        GP10_patch_name = GP10_patch_name + static_cast<char>(sxdata[count]); //Add ascii character to Patch Name String
      }
      update_main_lcd = true;
    }

    // Read GP10 assign area
    GP10_read_complete_assign_area(address, sxdata, sxlength);

    // Check if it is the guitar on/off states
    GP10_check_guitar_switch_states(sxdata, sxlength);

    // Check if it is some other stompbox function and copy the status to the right LED
    //GP10_check_stompbox_states(sxdata, sxlength);
  }
}

void check_PC_in_GP10(byte channel, byte program) {  // Check incoming PC messages from GP10. Called from MIDI:OnProgramChange

  // Check the source by checking the channel
  if ((Current_MIDI_port == GP10_MIDI_port) && (channel == GP10_MIDI_channel)) { // GP10 outputs a program change
    if (GP10_patch_number != program) {
      GP10_patch_number = program;
      GP10_assign_read = false; // Assigns should be read again
      request_GP10(GP10_REQUEST_CURRENT_PATCH_NAME); // So the main display always show the correct patch
      GP10_page_check();
      GP10_do_after_patch_selection();
    }
  }
}

void check_CC_in_GP10(byte channel, byte control, byte value) {  // Check incoming CC messages from GP10.
  // Check the source by checking the channel
  if (channel == GP10_MIDI_channel) { // GP10 outputs a control change message
  }
}

// Detection of GP-10

void GP10_check_detect() { // Started from MIDI/MIDI_check_for_devices()
  if (GP10_connected) {
    //DEBUGMSG("GP-10 not detected times " + String(GP10_not_detected));
    if (GP10_not_detected >= GP10_MAX_NOT_DETECTED) GP10_disconnect();
    GP10_not_detected++;
  }
}

void GP10_identity_check(const unsigned char* sxdata, short unsigned int sxlength) {
  // Check if it is a GP-10
  if ((sxdata[6] == 0x05) && (sxdata[7] == 0x03)) {
    GP10_not_detected = 0;
    if (GP10_connected == false) GP10_connect(sxdata[2]); //Byte 2 contains the correct device ID
  }
}

void GP10_connect(uint8_t device_id) {
  GP10_connected = true;
  LCD_show_status_message("GP-10 connected ");
  GP10_device_id = device_id;
  GP10_MIDI_port = Current_MIDI_port; // Set the correct MIDI port for this device
  DEBUGMSG("GP-10 connected on MIDI port " + String(Current_MIDI_port));
  request_GP10(GP10_REQUEST_PATCH_NUMBER);
  request_GP10(GP10_REQUEST_CURRENT_PATCH_NAME); // So the main display always show the correct patch
  //write_GP10(GP10_EDITOR_MODE_ON); // Put the GP10 in EDITOR mode - otherwise tuner will not work
  GP10_do_after_patch_selection();
  GP10_assign_read = false; // Assigns should be read again
  update_page = FULL;
}

void GP10_disconnect() {
  GP10_connected = false;
  GP10_on = false;
  LCD_show_status_message("GP-10 offline   ");
  DEBUGMSG("GP-10 offline");
  update_page = FULL;
  update_main_lcd = true;
}


// ********************************* Section 3: GP10 common MIDI out functions ********************************************

void GP10_check_sysex_delay() { // Will delay if last message was within GP10_SYSEX_DELAY_LENGTH (10 ms)
  while (millis() - GP10sysexDelay <= GP10_SYSEX_DELAY_LENGTH) {}
  GP10sysexDelay = millis();
}

void write_GP10(uint32_t address, uint8_t value) { // For sending one data byte

  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into four bytes: ad[3], ad[2], ad[1] and ad[0]
  uint8_t checksum = MIDI_calc_Roland_checksum(ad[3] + ad[2] + ad[1] + ad[0] + value); // Calculate the Roland checksum
  uint8_t sysexmessage[15] = {0xF0, 0x41, GP10_device_id, 0x00, 0x00, 0x00, 0x05, 0x12, ad[3], ad[2], ad[1], ad[0], value, checksum, 0xF7};
  GP10_check_sysex_delay();
  if (GP10_MIDI_port == USBMIDI_PORT) usbMIDI.sendSysEx(15, sysexmessage);
  if (GP10_MIDI_port == MIDI1_PORT) MIDI1.sendSysEx(14, sysexmessage);
  if (GP10_MIDI_port == MIDI2_PORT) MIDI2.sendSysEx(14, sysexmessage);
  if (GP10_MIDI_port == MIDI3_PORT) MIDI3.sendSysEx(14, sysexmessage);
  MIDI_debug_sysex(sysexmessage, 15, "out(GP10)");
}

void write_GP10(uint32_t address, uint8_t value1, uint8_t value2) { // For sending two data bytes

  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into four bytes: ad[3], ad[2], ad[1] and ad[0]
  uint8_t checksum = MIDI_calc_Roland_checksum(ad[3] + ad[2] + ad[1] + ad[0] + value1 + value2); // Calculate the Roland checksum
  uint8_t sysexmessage[16] = {0xF0, 0x41, GP10_device_id, 0x00, 0x00, 0x00, 0x05, 0x12, ad[3], ad[2], ad[1], ad[0], value1, value2, checksum, 0xF7};
  GP10_check_sysex_delay();
  if (GP10_MIDI_port == USBMIDI_PORT) usbMIDI.sendSysEx(16, sysexmessage);
  if (GP10_MIDI_port == MIDI1_PORT) MIDI1.sendSysEx(15, sysexmessage);
  if (GP10_MIDI_port == MIDI2_PORT) MIDI2.sendSysEx(15, sysexmessage);
  if (GP10_MIDI_port == MIDI3_PORT) MIDI3.sendSysEx(15, sysexmessage);
  MIDI_debug_sysex(sysexmessage, 16, "out(GP10)");
}

void request_GP10(uint32_t address, uint8_t no_of_bytes) {
  uint8_t *ad = (uint8_t*)&address; //Split the 32-bit address into four bytes: ad[3], ad[2], ad[1] and ad[0]
  uint8_t no1 = no_of_bytes / 128;
  uint8_t no2 = no_of_bytes % 128;
  uint8_t checksum = MIDI_calc_Roland_checksum(ad[3] + ad[2] + ad[1] + ad[0] +  no1 + no2); // Calculate the Roland checksum
  uint8_t sysexmessage[18] = {0xF0, 0x41, GP10_device_id, 0x00, 0x00, 0x00, 0x05, 0x11, ad[3], ad[2], ad[1], ad[0], 0x00, 0x00, no1, no2, checksum, 0xF7};
  GP10_check_sysex_delay();
  if (GP10_MIDI_port == USBMIDI_PORT) usbMIDI.sendSysEx(18, sysexmessage);
  if (GP10_MIDI_port == MIDI1_PORT) MIDI1.sendSysEx(17, sysexmessage);
  if (GP10_MIDI_port == MIDI2_PORT) MIDI2.sendSysEx(17, sysexmessage);
  if (GP10_MIDI_port == MIDI3_PORT) MIDI3.sendSysEx(17, sysexmessage);
  MIDI_debug_sysex(sysexmessage, 18, "out(GP10)");
}

void GP10_request_patch_number() {
  request_GP10(GP10_REQUEST_PATCH_NUMBER);
}

void GP10_send_bpm() {
  write_GP10(GP10_TEMPO, bpm / 16, bpm % 16); // Tempo is modulus 16. It's all so very logical. NOT.
}

// ********************************* Section 4: GP10 program change ********************************************

uint8_t GP10_previous_patch_number = 0;
uint8_t GP10_patch_memory = 0;

void GP10_SendProgramChange(uint8_t new_patch) {

  if (new_patch == GP10_patch_number) GP10_unmute();
  GP10_patch_number = new_patch;
  MIDI_send_PC(new_patch, GP10_MIDI_channel, GP10_MIDI_port);
  DEBUGMSG("out(GP10) PC" + String(new_patch)); //Debug
  //GR55_mute();
  //VG99_mute();
  GP10_assign_read = false; // Assigns should be read again
  GP10_do_after_patch_selection();
}

void GP10_do_after_patch_selection() {
  GP10_request_onoff = false;
  GP10_on = true;
  if (SEND_GLOBAL_TEMPO_AFTER_PATCH_CHANGE == true) {
    delay(5); // GP10 misses send bpm command...
    GP10_send_bpm();
  }
  Current_patch_number = GP10_patch_number; // the patch name
  Current_device = GP10;
  update_LEDS = true;
  update_main_lcd = true;
  update_page = PAR_ONLY;
  GP10_request_guitar_switch_states();
  //EEPROM.write(EEPROM_GP10_PATCH_NUMBER, GP10_patch_number);

}

void GP10_relsel_load(uint8_t sw, uint8_t bank_position, uint8_t bank_size) {
  if (GP10_bank_selection_active == false) GP10_bank_number = (GP10_patch_number / bank_size);
  uint8_t new_patch = (GP10_bank_number * bank_size) + bank_position;
  if (new_patch > GP10_PATCH_MAX) new_patch = new_patch - GP10_PATCH_MAX - 1 + GP10_PATCH_MIN;
  GP10_patch_load(sw, new_patch); // Calculate the patch number for this switch
}

void GP10_patch_load(uint8_t sw, uint8_t number) {
  SP[sw].Colour = GP10_PATCH_COLOUR; // Set the colour
  SP[sw].PP_number = number;
  number++;
  SP[sw].Address = 0x20000000 + ((number / 0x20) * 0x1000000) + ((number % 0x20) * 0x40000); //Calculate the address where the patchname is stored on the GP-10
}

void GP10_patch_select(uint16_t new_patch) {
  // Check whether the current patch needs to be switched on or whether a new patch is chosen
  if (new_patch == GP10_patch_number) GP10_select_switch(); // Not a new patch - do US20 emulation
  else GP10_SendProgramChange(new_patch); //New patch - send program change
  GP10_bank_selection_active = false;

}

void GP10_bank_updown(bool updown, uint8_t bank_size) {
  uint8_t bank_number = (GP10_patch_number / bank_size);

  if (GP10_bank_selection_active == false) {
    GP10_bank_selection_active = true;
    GP10_bank_number = bank_number; //Reset the bank to current patch
    GP10_bank_size = bank_size;
  }
  // Perform bank up:
  if (updown == UP) {
    if (GP10_bank_number >= ((GP10_PATCH_MAX / bank_size) - 1)) GP10_bank_number = (GP10_PATCH_MIN / bank_size); // Check if we've reached the top
    else GP10_bank_number++; //Otherwise move bank up
  }
  // Perform bank down:
  if (updown == DOWN) {
    if ((GP10_bank_number * bank_size) <= GP10_PATCH_MIN) GP10_bank_number = GP10_PATCH_MAX / bank_size; // Check if we've reached the bottom
    else GP10_bank_number--; //Otherwise move bank down
  }

  if (GP10_bank_number == bank_number) GP10_bank_selection_active = false; //Check whether were back to the original bank

  update_page = FULL; //Re-read the patchnames for this bank
}

void GP10_page_check() { // Checks if the current patch is on the page and will reload the page if not
  bool onpage = false;
  for (uint8_t s = 0; s < NUMBER_OF_SWITCHES; s++) {
    if ((SP[s].Type == GP10_RELSEL) && (SP[s].PP_number == GP10_patch_number)) {
      onpage = true;
      GP10_patch_name = SP[s].Label; // Set patchname correctly
    }
  }
  if (!onpage) {
    update_page = FULL;
  }
}

void GP10_display_patch_number_string() {
  // Uses GP10_patch_number as input and returns Current_patch_number_string as output in format "P01"
  if (GP10_bank_selection_active == false) {
    GP10_number_format(GP10_patch_number, Current_patch_number_string);
  }
  else {
    //Current_patch_number_string = "P" + String(GP10_bank_number) + "-";
    String start_number, end_number;
    GP10_number_format(GP10_bank_number * GP10_bank_size, start_number);
    GP10_number_format((GP10_bank_number + 1) * GP10_bank_size - 1, end_number);
    Current_patch_number_string = Current_patch_number_string + start_number + "-" + end_number;
  }
}

void GP10_number_format(uint8_t number, String &Output) {
  Output = Output + "P" + String((number + 1) / 10) + String((number + 1) % 10);
}

// ** US-20 simulation
// Selecting and muting the GP10 is done by storing the settings of COSM guitar switch and Normal PU switch
// and switching both off when guitar is muted and back to original state when the GP10 is selected

void GP10_request_guitar_switch_states() {
  //GP10_select_LED = GP10_PATCH_COLOUR; //Switch the LED on
  request_GP10(GP10_COSM_GUITAR_SW, 1);
  request_GP10(GP10_NORMAL_PU_SW, 1);
  GP10_request_onoff = true;
  no_device_check = true;
}

void GP10_check_guitar_switch_states(const unsigned char* sxdata, short unsigned int sxlength) {
  if (GP10_request_onoff == true) {
    uint32_t address = (sxdata[8] << 24) + (sxdata[9] << 16) + (sxdata[10] << 8) + sxdata[11]; // Make the address 32 bit
    if (address == GP10_COSM_GUITAR_SW) {
      GP10_COSM_onoff = sxdata[12];  // Store the value
    }

    if (address == GP10_NORMAL_PU_SW) {
      GP10_nrml_pu_onoff = sxdata[12];  // Store the value
      GP10_request_onoff = false;
      no_device_check = false;
    }
  }
}

void GP10_select_switch() {
  Current_device = GP10;
  if (GP10_on) {
    GP10_always_on_toggle();
  }
  else {
    GP10_unmute();
    //GR55_mute();
    //VG99_mute();
    //if (mode != MODE_GP10_GR55_COMBI) LCD_show_status_message(GP10_patch_name); // Show the correct patch name
  }
}

void GP10_always_on_toggle() {
  if (US20_emulation_active) {
    GP10_always_on = !GP10_always_on; // Toggle GP10_always_on
    if (GP10_always_on) {
      GP10_unmute();
      LCD_show_status_message("GP10 always ON");
    }
    else {
      //GP10_mute();
      LCD_show_status_message("GP10 can be muted");
    }
  }
}


void GP10_unmute() {
  GP10_on = true;
  //GP10_select_LED = GP10_PATCH_COLOUR; //Switch the LED on
  //write_GP10(GP10_FOOT_VOL, 100); // Switching guitars does not work - the wrong values are read from the GP-10. ?????
  write_GP10(GP10_COSM_GUITAR_SW, GP10_COSM_onoff); // Switch COSM guitar on
  write_GP10(GP10_NORMAL_PU_SW, GP10_nrml_pu_onoff); // Switch normal pu on
}

void GP10_mute() {
  if ((US20_emulation_active) && (!GP10_always_on)) {
    GP10_on = false;
    //    GP10_select_LED = GP10_OFF_COLOUR; //Switch the LED off
    //write_GP10(GP10_FOOT_VOL, 0);
    write_GP10(GP10_COSM_GUITAR_SW, 0x00); // Switch COSM guitar off
    write_GP10(GP10_NORMAL_PU_SW, 0x00); // Switch normal pu off
  }
}

// ********************************* Section 5: GP10 parameter and assign control ********************************************

// Procedures for the GP10_PARAMETER:
// 1. Load in SP array L load_page()
// 2. Request parameter state - in Request_current_switch()
// 3. Read parameter state - GP10_read_parameter() below
// 4. Press switch - GP10_parameter_press() below - also calls GP10_check_update_label()
// 5. Release switch - GP10_parameter_release() below - also calls GP10_check_update_label()

struct GP10_parameter_struct { // Combines all the data we need for controlling a parameter in a device
  uint16_t Target; // Target of the assign as given in the assignments of the GP10 / GR55
  uint32_t Address; // The address of the parameter
  char Name[17]; // The name for the label
  uint8_t Sublist; // Which sublist to read for the FX or amp type - 0 if second byte does not contain the type or if there is no sublist +100 Show value from sublist.
  uint8_t Colour; // The colour for this effect.
};

//typedef struct stomper Stomper;
#define GP10_FX_COLOUR 255 // Just a colour number to pick the colour from the GP10_FX_colours table
#define GP10_FX_TYPE_COLOUR 254 //Another number for the FX type

// Make sure you edit the GP10_NUMBER_OF_STOMPS when adding new parameters
#define GP10_NUMBER_OF_PARAMETERS 19

const PROGMEM GP10_parameter_struct GP10_parameters[GP10_NUMBER_OF_PARAMETERS] = {
  {0x0F1, 0x20005800, "FX", 1, GP10_FX_COLOUR},
  {0x0F2, 0x20005801, "FX Type", 101, GP10_FX_TYPE_COLOUR},
  {0x166, 0x20016000, "WAH SW", 0, FX_FILTER_COLOUR},
  {0x16D, 0x20016800, "Chorus", 17, FX_MODULATE_COLOUR},
  {0x176, 0x20017000, "DLY", 20, FX_DELAY_COLOUR},
  {0x188, 0x20017800, "RVB", 30, FX_REVERB_COLOUR},
  {0x192, 0x20020000, "EQ SW", 0, FX_FILTER_COLOUR},
  {0x000, 0x20001000, "COSM GUITAR", 0, FX_GTR_COLOUR},
  {0x001, 0x20001001, "COSM Type", 0, FX_GTR_COLOUR},
  {0x0A5, 0x20000804, "NORMAL PU", 0, GP10_STOMP_COLOUR},
  {0x002, 0x20001800, "E-GTR TYPE", 0, FX_GTR_COLOUR},
  {0x0CF, 0x20001002, "COSM NS SW", 0, FX_GTR_COLOUR},
  {0x09F, 0x20004000, "ALT TUNING", 0, FX_PITCH_COLOUR},
  {0x0AD, 0x2000400E, "12 STRING SW", 0, FX_PITCH_COLOUR},
  {0x0DE, 0x20005000, "Amp SW", 37, FX_AMP_COLOUR},
  {0x0DF, 0x20005001, "Amp Type", 137, FX_AMP_COLOUR},
  {0x0E8, 0x2000500B, "Amp solo", 0, FX_AMP_COLOUR},
  {0x0EC, 0x2000500A, "Amp Gain SW", 0, FX_AMP_COLOUR},
  {0xFFF, 0x10001000, "Guitar2MIDI", 0, GP10_STOMP_COLOUR}, //Can not be controlled from assignment, but can be from GP10_PARAMETER!!!
};

#define GP10_SIZE_OF_SUBLIST 66
const PROGMEM char GP10_sublists[GP10_SIZE_OF_SUBLIST][8] = {
  // Sublist 1 - 16: FX types
  "OD/DS", "COMPRSR", "LIMITER", "EQ", "T.WAH", "P.SHIFT", "HARMO", "P. BEND", "PHASER", "FLANGER", "TREMOLO", "PAN", "ROTARY", "UNI-V", "CHORUS", "DELAY",

  // Sublist 17 - 19: Chorus types
  "MONO", "STEREO1", "STEREO2",

  // Sublist 20 - 29: Delay types
  "SINGLE", "PAN", "STEREO", "DUAL-S", "DUAL-P", "DU L/R", "REVRSE", "ANALOG", "TAPE", "MODLTE",

  // Sublist 30 - 36: Reverb types
  "AMBNCE", "ROOM", "HALL1", "HALL2", "PLATE", "SPRING", "MODLTE",

  // Sublist 37 - 66 : Amp types
  "NAT CLN", "FULL RG", "COMBO C", "STACK C", "HiGAIN", "POWER D", "EXTREME", "CORE MT", "JC-120", "CL TWIN",
  "PRO CR", "TWEED", "DELUXE", "VO DRVE", "VO LEAD", "MTCH DR", "BG LEAD", "BG DRVE", "MS1959I", "M1959II",
  "RFIER V", "RFIER M", "T-AMP L", "SLDN", "5150 DR", "BGNR UB", "ORNG RR", "BASS CL", "BASS CR", "BASS Hi"
};

const uint8_t GP10_FX_colours[17] = { // Table with the LED colours for the different FX states
  FX_DIST_COLOUR, // Colour for "OD/DS"
  FX_FILTER_COLOUR, // Colour for "COMPRSR"
  FX_FILTER_COLOUR, // Colour for "LIMITER"
  FX_FILTER_COLOUR, // Colour for "EQ"
  FX_FILTER_COLOUR, // Colour for "T.WAH"
  FX_PITCH_COLOUR, // Colour for "P.SHIFT"
  FX_PITCH_COLOUR, // Colour for  "HARMO"
  FX_PITCH_COLOUR, // Colour for "P. BEND"
  FX_MODULATE_COLOUR, // Colour for "PHASER"
  FX_MODULATE_COLOUR, // Colour for "FLANGER"
  FX_MODULATE_COLOUR, // Colour for "TREMOLO"
  FX_MODULATE_COLOUR, // Colour for "PAN"
  FX_MODULATE_COLOUR, // Colour for  "ROTARY"
  FX_MODULATE_COLOUR, // Colour for "UNI-V"
  FX_MODULATE_COLOUR, // Colour for "CHORUS"
  FX_DELAY_COLOUR, // Colour for "DELAY"
};

// Toggle GP10 stompbox parameter
void GP10_parameter_press(uint8_t Sw, uint8_t Cmd, uint8_t number) {

  // Send sysex MIDI command to GP-10
  uint8_t value = 0;
  if (SP[Sw].State == 1) value = Page[Current_page].Switch[Sw].Cmd[Cmd].Value1;
  if (SP[Sw].State == 2) value = Page[Current_page].Switch[Sw].Cmd[Cmd].Value2;
  if (SP[Sw].State == 3) value = Page[Current_page].Switch[Sw].Cmd[Cmd].Value3;
  if (SP[Sw].State == 4) value = Page[Current_page].Switch[Sw].Cmd[Cmd].Value4;
  if (SP[Sw].State == 5) value = Page[Current_page].Switch[Sw].Cmd[Cmd].Value5;
  if (SP[Sw].Latch == RANGE) value = Expr_value;
  write_GP10(GP10_parameters[number].Address, value);

  // Show message
  GP10_check_update_label(Sw, value);
  LCD_show_status_message(SP[Sw].Label);

  PAGE_load_current(false); // To update the other parameter states, we re-load the current page
}

void GP10_parameter_release(uint8_t Sw, uint8_t Cmd, uint8_t number) {
  // Work out state of pedal
  if (SP[Sw].Latch == MOMENTARY) {
    SP[Sw].State = 2; // Switch state off
    write_GP10(GP10_parameters[number].Address, Page[Current_page].Switch[Sw].Cmd[Cmd].Value1);

    PAGE_load_current(false); // To update the other switch states, we re-load the current page
  }
}

void GP10_read_parameter(uint8_t byte1, uint8_t byte2) { //Read the current GP10 parameter
  SP[Current_switch].Target_byte1 = byte1;
  SP[Current_switch].Target_byte2 = byte2;

  // Set the status
  SP[Current_switch].State = 0;
  if (SP[Current_switch].Type == GP10_PARAMETER) {
    if (byte1 == Page[Current_page].Switch[Current_switch].Cmd[0].Value5) SP[Current_switch].State = 5;
    if (byte1 == Page[Current_page].Switch[Current_switch].Cmd[0].Value4) SP[Current_switch].State = 4;
    if (byte1 == Page[Current_page].Switch[Current_switch].Cmd[0].Value3) SP[Current_switch].State = 3;
    if (byte1 == Page[Current_page].Switch[Current_switch].Cmd[0].Value2) SP[Current_switch].State = 2;
    if (byte1 == Page[Current_page].Switch[Current_switch].Cmd[0].Value1) SP[Current_switch].State = 1;
  }
  else { // It must be a GP10_ASSIGN
    if (byte1 == SP[Current_switch].Assign_min) SP[Current_switch].State = 2;
    if (byte1 == SP[Current_switch].Assign_max) SP[Current_switch].State = 1;
  }

  // Set the colour
  uint8_t index = SP[Current_switch].PP_number; // Read the parameter number (index to GP10-parameter array)
  uint8_t my_colour = GP10_parameters[index].Colour;

  //Check for special colours:
  if (my_colour == GP10_FX_COLOUR) SP[Current_switch].Colour = GP10_FX_colours[byte2]; //FX type read in byte2
  else if (my_colour == GP10_FX_TYPE_COLOUR) SP[Current_switch].Colour = GP10_FX_colours[byte1]; //FX type read in byte1
  else SP[Current_switch].Colour =  my_colour;

  // Set the display message
  String msg = GP10_parameters[index].Name;
  if ((GP10_parameters[index].Sublist > 0) && (GP10_parameters[index].Sublist < 100)) { // Check if a sublist exists
    String type_name = GP10_sublists[GP10_parameters[index].Sublist + byte2 - 1];
    msg = msg + " (" + type_name + ")";
  }
  if (GP10_parameters[index].Sublist > 100) { // Check if state needs to be read
    String type_name = GP10_sublists[GP10_parameters[index].Sublist + byte1 - 101];
    msg = msg + ": " + type_name;
  }
  //Copy it to the display name:
  LCD_set_label(Current_switch, msg);
}

void GP10_check_update_label(uint8_t Sw, uint8_t value) { // Updates the label for extended sublists
  uint8_t index = SP[Sw].PP_number; // Read the parameter number (index to GP10-parameter array)
  if (index != NOT_FOUND) {
    if (GP10_parameters[index].Sublist > 100) { // Check if state needs to be read
      LCD_clear_label(Sw);
      // Set the display message
      String msg = GP10_parameters[index].Name;
      String type_name = GP10_sublists[GP10_parameters[index].Sublist + value - 101];
      msg = msg + ": " + type_name;

      //Copy it to the display name:
      LCD_set_label(Sw, msg);
    }
  }
}
// **** GP10 assigns
// GP10 assigns are organised different from other Roland devices. The settings are spread out, which is a pain:
// 0x20022000 - 0x20022007: Assign switch 1 - 8
// 0x20022008 - 0x2002201F: Assign target 1 - 8
// 0x20022020 - 0x2002203F: Assign target min 1 - 8
// 0x20022040 - 0x2002205F: Assign target max 1 - 8
// 0x20022060 - 0x20022067: Assign source 1 - 8
// 0x20022068 - 0x2002206F: Assign latch (momentary/toggle) 1 - 8
// The rest we do not need, so we won't read it

// Whenever there is a GP10 assign on the page, the entire 112 bytes of the  GP10 assigns are read from the device
// Because the MIDI buffer of the VController is limited to 60 bytes including header and footer bytes, we read them in three turns

// Procedures for GP10_ASSIGN:
// 1. Load in SP array - GP10_assign_load() below
// 2. Request - GP10_request_complete_assign_area() (first time only)
// 3. Read assign area - GP10_read_complete_assign_area() (first time only)
// 4. Request parameter state - GP10_assign_request() below, also uses GP10_target_lookup()
// 5. Read parameter state - GP10_read_parameter() above
// 6. Press switch - GP10_assign_press() below
// 7. Release switch - GP10_assign_release() below

#define GP10_assign_address_set1 0x20022000
#define GP10_assign_address_set2 0x20022028
#define GP10_assign_address_set3 0x20022050

void GP10_assign_press(uint8_t Sw) { // Switch set to GP10_ASSIGN is pressed
  // Send cc MIDI command to GP-10
  uint8_t cc_number = SP[Sw].Trigger;
  MIDI_send_CC(cc_number, 127, GP10_MIDI_channel, GP10_MIDI_port);

  // Display the patch function
  if (SP[Sw].Assign_on) {
    uint8_t value = 0;
    if (SP[Sw].State == 1) value = SP[Sw].Assign_max;
    else value = SP[Sw].Assign_min;
    GP10_check_update_label(Sw, value);
  }
  LCD_show_status_message(SP[Sw].Label);

  if (SP[Sw].Assign_on) PAGE_load_current(false); // To update the other switch states, we re-load the current page
}

void GP10_assign_release(uint8_t Sw) { // Switch set to GP10_ASSIGN is released
  // Send cc MIDI command to GP-10
  uint8_t cc_number = SP[Sw].Trigger;
  MIDI_send_CC(cc_number, 0, GP10_MIDI_channel, GP10_MIDI_port);

  // Update status
  if (SP[Sw].Latch == MOMENTARY) {
    if (SP[Sw].Assign_on) SP[Sw].State = 2; // Switch state off
    else SP[Sw].State = 0; // Assign off, so LED should be off as well

    if (SP[Sw].Assign_on) PAGE_load_current(false); // To update the other switch states, we re-load the current page
  }
}

void GP10_assign_load(uint8_t sw, uint8_t assign_number, uint8_t cc_number) { // Switch set to GP10_ASSIGN is loaded in SP array
  SP[sw].Trigger = cc_number; //Save the cc_number in the Trigger variable
  SP[sw].Assign_number = assign_number;
}

void GP10_request_complete_assign_area() {
  request_GP10(GP10_assign_address_set1, 40);  //Request the first 40 bytes of the GP10 assign area
  request_GP10(GP10_assign_address_set2, 40);  //Request the second 40 bytes of the GP10 assign area
  request_GP10(GP10_assign_address_set3, 32);  //Request the last 32 bytes of the GP10 assign area
}

void GP10_read_complete_assign_area(uint32_t address, const unsigned char* sxdata, short unsigned int sxlength) {
  if (address == GP10_assign_address_set1) { // Read assign byte 0 - 39 from GP10 memory
    for (uint8_t count = 0; count < 40; count++) {
      GP10_assign_mem[count] = sxdata[count + 12]; //Add byte to the assign memory
    }
  }

  if (address == GP10_assign_address_set2) { // Read assign byte 40 - 79 from GP10 memory
    for (uint8_t count = 0; count < 40; count++) {
      GP10_assign_mem[count + 40] = sxdata[count + 12]; //Add byte to the assign memory
    }
  }

  if (address == GP10_assign_address_set3) { // Read assign byte 80 - 111 from GP10 memory
    for (uint8_t count = 0; count < 32; count++) {
      GP10_assign_mem[count + 80] = sxdata[count + 12]; //Add byte to the assign memory
    }
    GP10_assign_read = true;
    GP10_assign_request();
  }
}

void GP10_assign_request() { //Request the current assign
  bool found, assign_on;
  String msg;
  uint8_t assign_switch, assign_source, assign_latch;
  uint16_t assign_target, assign_target_min, assign_target_max;

  // First we read out the GP10_assign_mem. We calculate the bytes we need to read from an index.
  uint8_t index = SP[Current_switch].Assign_number - 1; //index is between 0 (assign 1) and 7 (assign 8)
  if (index < 8) {
    assign_switch = GP10_assign_mem[index];
    assign_target = (GP10_assign_mem[8 + (index * 3)] << 8) + (GP10_assign_mem[9 + (index * 3)] << 4) + GP10_assign_mem[10 + (index * 3)];
    assign_target_min = (GP10_assign_mem[0x21 + (index * 4)] << 8) + (GP10_assign_mem[0x22 + (index * 4)] << 4) + GP10_assign_mem[0x23 + (index * 4)];
    assign_target_max = (GP10_assign_mem[0x41 + (index * 4)] << 8) + (GP10_assign_mem[0x42 + (index * 4)] << 4) + GP10_assign_mem[0x43 + (index * 4)];
    assign_source = GP10_assign_mem[0x60 + index];
    assign_latch = GP10_assign_mem[0x68 + index];

    uint8_t my_trigger = SP[Current_switch].Trigger;
    if ((my_trigger >= 1) && (my_trigger <= 31)) my_trigger = my_trigger + 12; // Trigger is cc01 - cc31
    if ((my_trigger >= 64) && (my_trigger <= 95)) my_trigger = my_trigger - 20; // Trigger is cc64 - cc95
    assign_on = ((assign_switch == 0x01) && (my_trigger == assign_source)); // Check if assign is on by checking assign switch and source

    DEBUGMSG("GP-10 Assign_switch: 0x" + String(assign_switch, HEX));
    DEBUGMSG("GP-10 Assign_target 0x:" + String(assign_target, HEX));
    DEBUGMSG("GP-10 Assign_min: 0x" + String(assign_target_min, HEX));
    DEBUGMSG("GP-10 Assign_max: 0x" + String(assign_target_max, HEX));
    DEBUGMSG("GP-10 Assign_source: 0x" + String(assign_source, HEX));
    DEBUGMSG("GP-10 Assign_latch: 0x" + String(assign_latch, HEX));
    DEBUGMSG("GP-10 Assign_trigger-check:" + String(my_trigger) + "==" + String(assign_source));

  }
  else {
    assign_on = false; // Wrong assign number - switch off the assign
    assign_source = 0;
    assign_target = 0;
    assign_latch = MOMENTARY;
  }

  if (assign_on) {
    SP[Current_switch].Assign_on = true; // Switch the pedal on
    SP[Current_switch].Latch = assign_latch;
    SP[Current_switch].Assign_max = assign_target_max;
    SP[Current_switch].Assign_min = assign_target_min;

    // Request the target
    DEBUGMSG("Request target of assign " + String(index + 1) + ": " + String(SP[Current_switch].Address, HEX));
    found = GP10_target_lookup(assign_target); // Lookup the address of the target in the GR55_Parameters array
    if (found) request_GP10((SP[Current_switch].Address), 2);
    else {  // Pedal set to a parameter which we do not have in the list.
      SP[Current_switch].PP_number = NOT_FOUND;
      SP[Current_switch].Colour = GP10_STOMP_COLOUR;
      SP[Current_switch].Latch = MOMENTARY; // Because we cannot read the state, it is best to make the pedal momentary
      // Set the Label
      msg = "ASGN" + String(SP[Current_switch].Assign_number) + ": Unknown";
      LCD_set_label(Current_switch, msg);
      PAGE_request_next_switch();
    }
  }
  else { // Assign is off
    SP[Current_switch].Assign_on = false; // Switch the pedal off
    SP[Current_switch].State = 0; // Switch the stompbox off
    SP[Current_switch].Latch = MOMENTARY; // Make it momentary
    SP[Current_switch].Colour = GP10_STOMP_COLOUR; // Set the on colour to default
    // Set the Label
    msg = "CC#" + String(SP[Current_switch].Trigger);
    LCD_set_label(Current_switch, msg);
    PAGE_request_next_switch();
  }
}

bool GP10_target_lookup(uint16_t target) { // Finds the target and its address in the GP10_parameters table

  // Lookup in GP10_parameter array
  bool found = false;
  for (uint8_t i = 0; i < GP10_NUMBER_OF_PARAMETERS; i++) {
    if (target == GP10_parameters[i].Target) { //Check is we've found the right target
      SP[Current_switch].PP_number = i; // Save the index number
      SP[Current_switch].Address = GP10_parameters[i].Address;
      found = true;
    }
  }
  return found;
}


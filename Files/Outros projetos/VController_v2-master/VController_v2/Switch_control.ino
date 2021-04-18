// Please read VController_v2.ino for information about the license and authors

// Here an action is taken after a switch is pressed. This action is read from the config file

void setup_switch_control()
{
  //SCO_reset_all_switch_states();
  //PAGE_load_current(true);
}

// Do something with buttons being pressed
void main_switch_control()  // Checks if a button has been pressed and check out which functions have to be executed
{
  if (switch_pressed > 0) {
    if (switch_pressed > 16) LCD_show_status_message("Switch " + String(switch_pressed));
    else SCO_switch_pressed_commands(Current_page, switch_pressed - 1);
  }

  if (switch_released > 0) {
    SCO_switch_released_commands(Current_page, switch_released - 1);
  }

  if (switch_long_pressed > 0) {
    if (switch_long_pressed == 16) SCO_switch_VController_standbye(); //Switch VController on/off on long press button 16
    SCO_switch_long_pressed_commands(Current_page, switch_long_pressed - 1);
  }

  if (expr_ped_changed > 0) {
    //if (expr_ped_changed == 1) SCO_switch_pressed_commands(Current_page, 16);
    uint8_t index = (expr_ped_changed - 1);
    MIDI_send_CC(Exp_CC_number[index], Expr_value, Exp_CC_channel[index], Exp_CC_port[index]); // Just send a CC message - can be set on the Config page
    LCD_show_status_message("CC#" + String(Exp_CC_number[index]) + ": " + String(Expr_value));
  }

  SCO_update_tap_tempo_LED();
}

void SCO_switch_pressed_commands(uint8_t Pg, uint8_t Sw) {
  SP[Sw].Pressed = true;

  // Run through the commands:
  for (uint8_t c = 0; c < NUMBER_OF_COMMANDS; c++) {
    //execute command
    uint8_t Type, Data1, Data2;
    //if (Sw < NUMBER_OF_SWITCHES) { // It is a regular switch
    Type = Page[Pg].Switch[Sw].Cmd[c].Type;
    Data1 = Page[Pg].Switch[Sw].Cmd[c].Data1;
    Data2 = Page[Pg].Switch[Sw].Cmd[c].Data2;
    //}
    /*else { // It is an external switch
      Type = Ext_switch[Sw].Cmd[c].Type;
      Data1 = Ext_switch[Sw].Cmd[c].Data1;
      Data2 = Ext_switch[Sw].Cmd[c].Data2;
      LCD_show_status_message("Type: " + String(Type));
    }*/

    switch (Type) {
      case TAP_TEMPO:
        SCO_global_tap_tempo_press();
        break;
      case GP10_PATCH:
        GP10_patch_select(Data1 - 1);
        GP10_patch_name = SP[Sw].Label; // Store current patch name
        break;
      case GP10_RELSEL:
        GP10_patch_select(SP[Sw].PP_number);
        GP10_patch_name = SP[Sw].Label; // Store current patch name
        break;
      case GP10_BANK_UP:
        GP10_bank_updown(UP, Data1);
        break;
      case GP10_BANK_DOWN:
        GP10_bank_updown(DOWN, Data1);
        break;
      case GP10_PARAMETER:
        if (c == 0) SCO_update_parameter_state(Sw);
        GP10_parameter_press(Sw, c, Data1);
        break;
      case GP10_ASSIGN:
        if (c == 0) SCO_update_parameter_state(Sw);
        GP10_assign_press(Sw);
        break;
      case GP10_MUTE:
        GP10_mute();
        break;
      case GR55_PATCH:
        if ((Data1 >= 1) && (Data2 >= 1) && (Data2 <= 3)) GR55_patch_select(((Data1 - 1) * 3) + (Data2 - 1));
        GR55_patch_name = SP[Sw].Label; // Store current patch name
        break;
      case GR55_RELSEL:
        GR55_patch_select(SP[Sw].PP_number);
        GR55_patch_name = SP[Sw].Label; // Store current patch name
        break;
      case GR55_BANK_UP:
        GR55_bank_updown(UP, Data1);
        break;
      case GR55_BANK_DOWN:
        GR55_bank_updown(DOWN, Data1);
        break;
      case GR55_PARAMETER:
        if (c == 0) SCO_update_parameter_state(Sw);
        GR55_parameter_press(Sw, c, Data1);
        break;
      case GR55_ASSIGN:
        if (c == 0) SCO_update_parameter_state(Sw);
        GR55_assign_press(Sw);
        break;
      case GR55_MUTE:
        GR55_mute();
        break;
      case VG99_PATCH:
        VG99_patch_select((Data2 * 100) + Data1 - 1);
        VG99_patch_name = SP[Sw].Label; // Store current patch name
        break;
      case VG99_RELSEL:
        VG99_patch_select(SP[Sw].PP_number);
        VG99_patch_name = SP[Sw].Label; // Store current patch name
        break;
      case VG99_BANK_UP:
        VG99_bank_updown(UP, Data1);
        break;
      case VG99_BANK_DOWN:
        VG99_bank_updown(DOWN, Data1);
        break;
      case VG99_PARAMETER:
        if (c == 0) SCO_update_parameter_state(Sw);
        VG99_parameter_press(Sw, c, Data1);
        break;
      case VG99_ASSIGN:
        if (c == 0) SCO_update_parameter_state(Sw);
        VG99_assign_press(Sw);
        break;
      case VG99_MUTE:
        VG99_mute();
        break;
      case ZG3_PATCH:
        ZG3_patch_select(Data1 - 1);
        ZG3_Remember_FXs(Sw);
        ZG3_patch_name = SP[Sw].Label; // Store current patch name
        break;
      case ZG3_RELSEL:
        ZG3_patch_select(SP[Sw].PP_number);
        ZG3_Remember_FXs(Sw);
        ZG3_patch_name = SP[Sw].Label; // Store current patch name
        break;
      case ZG3_BANK_UP:
        ZG3_bank_updown(UP, Data1);
        break;
      case ZG3_BANK_DOWN:
        ZG3_bank_updown(DOWN, Data1);
        break;
      case ZG3_FX_TOGGLE:
        if (c == 0) SCO_update_parameter_state(Sw);
        ZG3_FX_press(Sw, c, Data1);
        break;
      case SELECT_PAGE:
        SCO_select_page(Data1);
        break;
      case SELECT_NEXT_DEVICE:
        SCO_select_next_device();
        break;
      case COMBI_BANK_UP:
        SCO_combi_bank_updown(UP, Data1);
        break;
      case COMBI_BANK_DOWN:
        SCO_combi_bank_updown(DOWN, Data1);
        break;
      case STANDBYE:
        SCO_switch_VController_on();
        break;
      case MIDI_PC:
        MIDI_send_PC(Data1, Data2, Page[Pg].Switch[Sw].Cmd[c].Value1);
        break;
      case MIDI_NOTE:
        MIDI_send_note_on(Data1, Data2, Page[Pg].Switch[Sw].Cmd[c].Value1, Page[Pg].Switch[Sw].Cmd[c].Value2);
        break;
    }
  }
  update_LEDS = true;
  update_main_lcd = true;
}

void SCO_switch_released_commands(uint8_t Pg, uint8_t Sw) {
  //LCD_show_status_message("Command: " + String(Command));
  SP[Sw].Pressed = false;

  // Run through the commands:
  for (uint8_t c = 0; c < NUMBER_OF_COMMANDS; c++) {
    //execute command
    uint8_t Data1 = Page[Pg].Switch[Sw].Cmd[c].Data1;
    //uint8_t Data2 = Page[Pg].Switch[Sw].Cmd[c].Data2;

    switch (Page[Pg].Switch[Sw].Cmd[c].Type) {
      case GP10_PARAMETER:
        GP10_parameter_release(Sw, c, Data1);
        break;
      case GP10_ASSIGN:
        GP10_assign_release(Sw);
        break;
      case GR55_PARAMETER:
        GR55_parameter_release(Sw, c, Data1);
        break;
      case GR55_ASSIGN:
        GR55_assign_release(Sw);
        break;
      case VG99_PARAMETER:
        VG99_parameter_release(Sw, c, Data1);
        break;
      case VG99_ASSIGN:
        VG99_assign_release(Sw);
        break;
      case MIDI_NOTE:
        MIDI_send_note_off(Data1, Page[Pg].Switch[Sw].Cmd[c].Data2, Page[Pg].Switch[Sw].Cmd[c].Value1, Page[Pg].Switch[Sw].Cmd[c].Value2);
        break;
    }
  }
  update_LEDS = true;
  update_main_lcd = true;
}

void SCO_switch_long_pressed_commands(uint8_t Pg, uint8_t Sw) {
  //LCD_show_status_message("Command: " + String(Page[Pg].Switch[Sw].Cmd[0].Type));
  switch (Page[Pg].Switch[Sw].Cmd[0].Type) {
    case SELECT_PAGE:
    case SELECT_NEXT_DEVICE:
      Current_page = Previous_page; // "Undo" for pressing SELECT_PAGE or SELECT_NEXT_DEVICE
      SCO_select_page(PAGE_MODE_SELECT);
      break;
  }
}
void SCO_update_parameter_state(uint8_t Sw) {
  switch (SP[Sw].Latch) {
    case MOMENTARY:
      SP[Sw].State = 1; // Switch state on
      break;
    case TOGGLE:  // Toggle state
      SP[Sw].State++;
      if (SP[Sw].State > 2) SP[Sw].State = 1;
      break;
    case TRISTATE:  // Select next state
      SP[Sw].State++;
      if (SP[Sw].State > 3) SP[Sw].State = 1;
      break;
    case FOURSTATE:  // Select next state
      SP[Sw].State++;
      if (SP[Sw].State > 4) SP[Sw].State = 1;
      break;
    case FIVESTATE: // Select next state
      SP[Sw].State++;
      if (SP[Sw].State > 5) SP[Sw].State = 1;
      break;
  }
}

void SCO_reset_all_switch_states() {
  for (uint8_t p = 0; p < NUMBER_OF_PAGES; p++) {
    for (uint8_t s = 0; s < NUMBER_OF_SWITCHES; s++) {
      switch_state[p][s] = false;
    }
  }
}

// Common functions
void SCO_select_page(uint8_t new_page) {
  Previous_page = Current_page; // Store the mode we come from...
  Current_page = new_page;
  //EEPROM.write(EEPROM_mode, Current_page);
  update_page = FULL;
}

void SCO_toggle_page(uint8_t page1, uint8_t page2) {
  Previous_page = Current_page; // Store the mode we come from...
  if (Current_page == page1) SCO_select_page(page2);
  else SCO_select_page(page1);
}

void SCO_combi_bank_updown(bool updown, uint8_t bank_size) {
  if (Current_device == GP10) GP10_bank_updown(updown, bank_size);
  if (Current_device == GR55) GR55_bank_updown(updown, bank_size);
  if (Current_device == VG99) VG99_bank_updown(updown, bank_size);
  if (Current_device == ZG3) ZG3_bank_updown(updown, bank_size);
}

uint8_t current_selected_device = GP10;

void SCO_select_next_device() { // Will select the next device that is connected
  // Put device connection states in array
  bool DCS[NUMBER_OF_DEVICES] = {GP10_connected, GR55_connected, VG99_connected, ZG3_connected};
  uint8_t Tries = NUMBER_OF_DEVICES; // Limited the number of tries for the loop, in case no device is
  while (Tries > 0) {
    Tries--;
    current_selected_device++;
    DEBUGMSG("Poging:" + String(Tries) + " Device:" + String(current_selected_device));
    if (current_selected_device >= NUMBER_OF_DEVICES) current_selected_device = 0;
    if (DCS[current_selected_device] == true) { // device is selected
      SCO_select_device_page(current_selected_device);
      Tries = 0; //And we are done
    }
  }
}

void SCO_select_device_page(uint8_t device) {
  uint8_t page;
  if (device == GP10) page = PAGE_GP10_RELSEL;
  if (device == GR55) page = PAGE_GR55_RELSEL;
  if (device == VG99) page = PAGE_VG99_RELSEL;
  if (device == ZG3) page = PAGE_ZG3_RELSEL;
  DEBUGMSG("Select page: " + String(page));
  SCO_select_page(page);
}

// ************************ Start Global tap tempo ************************
// Call global_tap_tempo()
// We only support bpms from 40 to 250:
#define MIN_TIME 240000 // (60.000.000 / 250 bpm)
#define MAX_TIME 1500000 // (60.000.000 / 40 bpm)

#define NUMBER_OF_TAPMEMS 5 // Tap tempo will do the average of five
uint32_t tap_time[NUMBER_OF_TAPMEMS];
uint8_t tap_time_index = 0;
uint32_t new_time, time_diff, avg_time;
uint32_t prev_time = 0;

void SCO_global_tap_tempo_press() {

  new_time = micros(); //Store the current time
  time_diff = new_time - prev_time;
  prev_time = new_time;
  DEBUGMSG("Tap no:" + String(tap_time_index) + " " + String(time_diff));

  // If time difference between two taps is too short or too long, we will start new tapping sequence
  if ((time_diff < MIN_TIME) || (time_diff > MAX_TIME)) {
    tap_time_index = 0;
  }
  else {

    // Shift tapmems to the left if neccesary
    if (tap_time_index >= NUMBER_OF_TAPMEMS) {
      for (uint8_t i = 1; i < NUMBER_OF_TAPMEMS; i++) {
        tap_time[i - 1] = tap_time[i];
      }
      tap_time_index--;  // A little wild, but now it works!
    }

    // Store time difference in memory
    tap_time[tap_time_index] = time_diff;

    //Calculate the average time
    //First add all the valid times up
    uint32_t total_time = 0;
    for (uint8_t j = 0; j <= tap_time_index; j++) {
      total_time = total_time + tap_time[j];
      Serial.print(String(tap_time[j]) + " ");
    }
    //Then calculate the average time
    avg_time = total_time / (tap_time_index + 1);
    bpm = ((60000000 + (avg_time / 2)) / avg_time); // Calculate the bpm
    //EEPROM.write(EEPROM_bpm, bpm);  // Store it in EEPROM
    DEBUGMSG(" tot:" + String(total_time) + " avg:" + String(avg_time));

    // Send it to the devices
    GP10_send_bpm();
    GR55_send_bpm();
    VG99_send_bpm();
    ZG3_send_bpm();

    // Move to the next memory slot
    tap_time_index++;
  }
  LCD_show_status_message("Tempo " + String(bpm) + " bpm");
  LCD_update(switch_pressed - 1);
  SCO_reset_tap_tempo_LED();
}

#define BPM_LED_ON_TIME 100 // The time the bpm LED is on in msec. 50 for real LED, 100 for virtual LED
#define BPM_LED_ADJUST 1   // LED is running a little to slow. This is an adjustment of a few msecs
uint32_t bpm_LED_timer = 0;
uint32_t bpm_LED_timer_length = BPM_LED_ON_TIME;

void SCO_update_tap_tempo_LED() {

  // Check if timer needs to be set
  if (bpm_LED_timer == 0) {
    bpm_LED_timer = millis();
  }

  // Check if timer runs out
  if (millis() - bpm_LED_timer > bpm_LED_timer_length) {
    bpm_LED_timer = millis(); // Reset the timer

    // If LED is currently on
    if (global_tap_tempo_LED == BPM_COLOUR_ON) {
      global_tap_tempo_LED = BPM_COLOUR_OFF;  // Turn the LED off
      VG99_TAP_TEMPO_LED_OFF();
      bpm_LED_timer_length = (60000 / bpm) - BPM_LED_ON_TIME - BPM_LED_ADJUST; // Set the time for the timer
    }
    else {
      global_tap_tempo_LED = BPM_COLOUR_ON;   // Turn the LED on
      VG99_TAP_TEMPO_LED_ON();
      bpm_LED_timer_length = BPM_LED_ON_TIME; // Set the time for the timer
    }
    update_LEDS = true;
  }
}

void SCO_reset_tap_tempo_LED() {
  bpm_LED_timer = millis();
  global_tap_tempo_LED = BPM_COLOUR_ON;    // Turn the LED on
  //VG99_TAP_TEMPO_LED_ON();
  bpm_LED_timer_length = BPM_LED_ON_TIME;  // Set the time for the timer
}

// Bass mode: sends a CC message with the number of the lowest string that is being played.
// By making smart assigns on a device, you can hear just the bass note played
#define GUITAR_TO_MIDI_CHANNEL 1 //The MIDI channel of the first string
#define BASS_CC_NUMBER 15 //The CC number for the bass control
#define BASS_CC_CHANNEL GR55_MIDI_channel //The MIDI channel on which this cc must be sent
#define BASS_CC_PORT GR55_MIDI_port //The MIDI port on which this cc must be sent
#define BASS_MIN_VEL 100 // Minimum velocity - messages below it ae ignored
uint8_t bass_string = 0; //remembers the current midi channel

void SCO_bass_mode_note_on(byte channel, byte note, byte velocity) {
  if ((channel >= GUITAR_TO_MIDI_CHANNEL) && (channel <= GUITAR_TO_MIDI_CHANNEL + 6)) {
    uint8_t string_played = channel - GUITAR_TO_MIDI_CHANNEL + 1;
    if ((string_played > bass_string) && (velocity >= BASS_MIN_VEL)) {
      bass_string = string_played; //Set the bass play channel to the current channel
      MIDI_send_CC(BASS_CC_NUMBER , bass_string, BASS_CC_CHANNEL, BASS_CC_PORT);
    }
  }
}

void SCO_bass_mode_note_off(byte channel, byte note, byte velocity) {
  if ((channel >= GUITAR_TO_MIDI_CHANNEL) && (channel <= GUITAR_TO_MIDI_CHANNEL + 6)) {
    uint8_t string_played = channel - GUITAR_TO_MIDI_CHANNEL + 1;
    if (string_played == bass_string) bass_string = 0; //Reset the bass play channel
  }
}

void SCO_switch_VController_toggle() {
  if (VController_on) SCO_switch_VController_standbye();
  else SCO_switch_VController_on();
}

void SCO_switch_VController_standbye() {
  // Store current values in memory
  VController_on = false;
  Current_page = Previous_page; // Undo the page change of button 16
  EEP_write_eeprom_common_data();

  // Select page 0 - it has all LEDs and switches turned off
  SCO_select_page(0);

  // Turn off LCD backlights
  LCD_backlight_off();
}

void SCO_switch_VController_on() {
  // Store current values in memory
  VController_on = true;
  EEP_read_eeprom_common_data();

  // Turn off LCD backlights
  LCD_backlight_on();

  // Select page
  if (Current_page == 0) Current_page = 1; // Check if it is not zero - otherwise you cannot switch the VController on
  SCO_select_page(Current_page);

  // Show startup message
  LCD_show_startup_message();
}



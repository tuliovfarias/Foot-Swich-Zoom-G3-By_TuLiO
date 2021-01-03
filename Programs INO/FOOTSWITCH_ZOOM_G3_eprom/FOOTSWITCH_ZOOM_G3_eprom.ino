#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>
#include <Usb.h>
#include <usbhub.h>
#include <usbh_midi.h>
#include <TM1637Display.h>

//Endereço EPROM dos patches
#define A_EPROM  0
#define B_EPROM  1
#define C_EPROM  2
#define D_EPROM  3

//BOTÕES
#define BT_A    2
//#define BT_B
//#define BT_C
//#define BT_D
//#define BT_UP  
#define BT_DOWN 3
//
////LEDS
//#define LED_A
//#define LED_B
//#define LED_C
//#define LED_D

//Display 7 seg
#define CLK 8
#define DIO 9

USB  Usb;
USBH_MIDI  Midi(&Usb);
TM1637Display display(CLK, DIO);

//Funções
void pin_config(void);
void display_config(void);
void display_set(void);
void load_patches(void);
void bt_read(void);
void writePatch(byte patch);
void readPatch();
void bt_check(void);

//Variáveis globais
uint8_t display_data[] = { 0xff, 0xff, 0xff, 0xff };

byte patch = 0;
byte patchOld = -1;
byte bank;
bool program_mode=0; //0 - modo de programação / 1 - modo de seleção

bool btA;
bool btB;
bool btC;
bool btD;
bool btUP;
bool btDOWN;

byte A_patch;
byte B_patch;
byte C_patch;
byte D_patch;


uint8_t message1[6] = {0xF0, 0x52, 0x00, 0x5A, 0x50, 0xF7}; //editor mode on
uint8_t message3[6] = {0xF0, 0x52, 0x00, 0x5A, 0x51, 0xF7}; //editor mode off
uint8_t message2[6] = {0xF0, 0x52, 0x00, 0x5A, 0x33, 0xF7}; //request patch number
uint8_t  rcvd[20]; 
char buf[20];
uint8_t bufMidi[64];
uint8_t y=0;
char i;

/////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(9600);
  //while (!Serial);
  if (Usb.Init() == -1) {
    Serial.println("USB NAO INICIOU");
    while (1);
  }
  Serial.println("USB INICIOU");
  //attachInterrupt (bt_A, bt_interrupt, CHANGE);

  for (int i = 0; i < 4; i++) {
    writePatch(i);  //os 4 primeiros comandos são ignorados
    delay(200);
  }

  pin_config();
  load_patches(); //carrega patches salvos
  display_config();
  display_set(); //exibe config atual no display
  bt_read(); //lê estado inicial dos botões
  Usb.Init();
  Usb.Task();
  //Midi.SendSysEx(message1,6); //ativa modo editor
}

///////////////////////////////////////////////////////////////////////////////////////
uint8_t outBuf[20];uint8_t sysexBuf[20];uint8_t x;uint8_t size1;

void loop() {
  bt_check();
  if(Serial.available()){
    Serial.read();
    program_mode=!program_mode;
    Serial.println("program mode = "+(String)program_mode);
  }
  //delay(1000);
  //Midi.SendSysEx(message2,6); //requisita patch
}


///////////////////////////////////////////////////////////////////////////////////////
void writePatch(byte patch) { //send program change over USB
  Usb.Task();
  //Serial.println("TASK");
  if ( Usb.getUsbTaskState() == USB_STATE_RUNNING ) //if USB is running, continue
  {
    //Serial.println("RUNNING");
    byte Message[2];                 // Construct the midi message (2 bytes)
    Message[0] = 0xC0;               // 0xC0 is for Program Change (Change to MIDI channel 0)
    Message[1] = patch;             // patch [0-99]
    Midi.SendData(Message);          // Send the message
    //Serial.println("SEND");
    delay(10);
  }
}

void loadPatch(){
  Midi.SendSysEx(message1,6); //ativa modo editor
  Midi.SendSysEx(message2,6); //requisita patch
  while (y!=2){
    //y = Midi.RecvData(rcvd);
    if ( (y = Midi.RecvRawData(outBuf)) > 0 ) {
      //Serial.println(rcvd); 
      Midi.extractSysExData(outBuf, sysexBuf);
      /*for (i = 0; i <= y; i++) {
        sprintf(buf, " %02u", outBuf[i]);
        Serial.print(buf);
      }
      Serial.println("");*/
    }
  }
  //Serial.println(outBuf[y]);
  patch=outBuf[y];
  y=0;
  Midi.SendSysEx(message3,6); //desativa modo editor
}
void load_patches() {
  A_patch = EEPROM.read(A_EPROM);
  B_patch = EEPROM.read(B_EPROM);
  C_patch = EEPROM.read(C_EPROM);
  D_patch = EEPROM.read(D_EPROM);
}
/*
  void bt_interrupt(){
  Serial.println("interrupt");
  if(digitalRead(bt_A)){SendMIDI(A_patch);Serial.println("patch A"+(String)(A_patch));}
  else {SendMIDI(7);Serial.println("patch A8");}
  }*/

void bt_check(void) {
  if (digitalRead(BT_A) != btA) {
    btA = digitalRead(BT_A);
    if(program_mode==1){
      loadPatch();
      A_patch=patch;
      EEPROM.write(A_EPROM, A_patch);
      delay(100);
      Serial.println("patch A = " + (String)(A_patch));
    }
    else{
      writePatch(A_patch);
      Serial.println("patch pedaleira = " + (String)(EEPROM.read(A_EPROM)));
    }
  }
  /*
  if (digitalRead(BT_B) != btB) {
    btA = digitalRead(BT_B);
    SendMIDI(B_patch);
    Serial.println("patch B" + (String)(B_patch));
  }
  if (digitalRead(BT_C) != btB) {
    btA = digitalRead(BT_C);
    SendMIDI(C_patch);
    Serial.println("patch C" + (String)(C_patch));
  }
  if (digitalRead(BT_D) != btD) {
    btA = digitalRead(bt_D);
    SendMIDI(D_patch);
    Serial.println("patch D" + (String)(D_patch));
  }
  if (digitalRead(BT_UP) != btUP) {
    Serial.println("UP" + (String)(bank));
  }
  
  if (digitalRead(BT_DOWN) != btDOWN) {
    Serial.println("DOWN" + (String)(bank));
  }
  */
}

void pin_config() {
  pinMode(BT_A, INPUT_PULLUP );
//  pinMode(BT_B, INPUT_PULLUP );
//  pinMode(BT_C, INPUT_PULLUP );
//  pinMode(BT_D, INPUT_PULLUP );
//  pinMode(BT_UP, INPUT_PULLUP);
//  pinMode(BT_DOWN, INPUT_PULLUP);
//
//  pinMode(LED_A, OUTPUT);
//  pinMode(LED_B, OUTPUT);
//  pinMode(LED_C, OUTPUT);
//  pinMode(LED_D, OUTPUT);

}

void bt_read(){
  btA = digitalRead(BT_A);
//  btB = digitalRead(bt_B);
//  btC = digitalRead(bt_C);
//  btD = digitalRead(bt_D);
}

void display_config(){
  display.setBrightness(0x0f);
}

void display_set(){
  //display_data[0] = display.encodeDigit(0);
  display.setSegments(display_data);
}




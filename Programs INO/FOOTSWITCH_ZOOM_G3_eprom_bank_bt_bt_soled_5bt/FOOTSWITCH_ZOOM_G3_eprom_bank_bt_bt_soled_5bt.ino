#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>
#include <Usb.h>
#include <usbhub.h>
#include <usbh_midi.h>
#include <TM1637Display.h>
#include <FastLED.h>

#define NUM_BANKS    4  //número de banks
#define NUM_PATCHES  5  //número de patches por bank

//BOTÕES
#define BT_A    5
#define BT_B    4
#define BT_C    7
#define BT_D    6
#define BT_UP   3 
#define BT_DOWN 2
//
////LEDS
#define LED_PIN     8
#define NUM_LEDS    4  //1 pra cada patch
#define NUM_SEG     7  //numero de leds nos 7 segmentos
#define BRIGHTNESS  50 //brilho dos leds (0-255)
/*
//Display 7 seg
#define CLK 8
#define DIO 2
*/
USB  Usb;
USBH_MIDI  Midi(&Usb);
CRGB leds[NUM_LEDS];
CRGBPalette16 RGB_colors(CRGB::Red,CRGB::Blue,CRGB::Green,CRGB::Yellow,CRGB::MediumVioletRed,CRGB::Aqua,CRGB::White,CRGB::Orange,CRGB::Red,CRGB::Blue,CRGB::Green,CRGB::Yellow,CRGB::MediumVioletRed,CRGB::Aqua,CRGB::White,CRGB::Orange);
//TM1637Display display(CLK, DIO);

//Funções
void pin_config(void);
void led_config(void);
void led_show(void);
//void display_config(void);
//void display_set(void);
void load_patches(void);
void loadPatch(void);
void bt_read(void);
void writePatch(byte patch);
void readPatch();
void bt_check(void);

//Variáveis globais
uint8_t display_data[] = { 0xff, 0xff, 0xff, 0xff }; //8888

byte bank_patch[NUM_BANKS][NUM_PATCHES]; //matriz que armazena todos os patches de cada bank
byte patch = 0;
byte foot_patch=0;
byte bank=0;
bool program_mode=0; //0 - modo de programação / 1 - modo de seleção
bool bt5_mode=0;     //0 - modo de 4 botões de patch / 1 - modo de 5 botões de patch

bool bt_patch=1; //indica se um botão é pressionado (coeça em 1 para carregar no início)
bool bt_updown=0; //indica se o botão UP ou DOWN foi pressionado
bool btA;
bool btB;
bool btC;
bool btD;
bool btUP;
bool btDOWN;

//Mensagens SysEx para a pedaleira
uint8_t message1[6] = {0xF0, 0x52, 0x00, 0x5A, 0x50, 0xF7}; //editor mode on
uint8_t message2[6] = {0xF0, 0x52, 0x00, 0x5A, 0x33, 0xF7}; //request patch number
uint8_t message3[6] = {0xF0, 0x52, 0x00, 0x5A, 0x51, 0xF7}; //editor mode off

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

  while (Usb.getUsbTaskState()!=USB_STATE_RUNNING){Usb.Task();} //if USB is running, continue
  Serial.println("USB CONECTOU");
  
  for (int i = 0; i < 4; i++) {
    writePatch(i);  //os 4 primeiros comandos são ignorados
    delay(200);
  }

  pin_config();
  load_patches(); //carrega patches salvos
  led_config();
  led_show();
  //display_config();
  //display_set(); //exibe config atual no display
  bt_read(); //lê estado inicial dos botões
  for(bank=0;bank<NUM_PATCHES;bank++){led_show();foot_patch++;delay(500);} //Liga leds em sequencia de cores
  bank=0;foot_patch=0;led_show();
  //Midi.SendSysEx(message1,6); //ativa modo editor
}

///////////////////////////////////////////////////////////////////////////////////////
unsigned int i=0;
bool x = false;
void loop() {
  bt_check();
  //piscar led no modo de programação
  if(program_mode==1){
    i++;
    if(i==200){x=!x;i=0;}
    if(x){FastLED.setBrightness(0);FastLED.show();}
    else{FastLED.setBrightness(BRIGHTNESS);FastLED.show();}
  }
  else if(x){FastLED.setBrightness(BRIGHTNESS);FastLED.show();x=0;}
  
  if(Serial.available()){
    char c = Serial.read();
    if(c=='1'){program_mode=!program_mode;Serial.println("Program mode = "+(String)program_mode);}
    if(c=='+'){bank++;Serial.println("UP bank:" + (String)(bank));}
    if(c=='-'){bank--;Serial.println("DOWN bank:" + (String)(bank));}
    if(c=='*'){
      Serial.println("\nMEMÓRIA FLASH");
      for(int i=0;i<NUM_BANKS;i++){
        for(int j=0;j<NUM_PATCHES;j++){
          Serial.print(bank_patch[i][j]);
          Serial.print(" ");
        }
        Serial.println("");
      }
    }
    if(c=='/'){
      Serial.println("\nMEMÓRIA EPROM");
      for(int i=0;i<NUM_BANKS;i++){
        for(int j=0;j<NUM_PATCHES;j++){
          Serial.print(EEPROM.read(i*(NUM_PATCHES)+j));
          Serial.print(" ");
        }
        Serial.println("");
      }
    }
    //elseif (c=='p')bank++;  
    
    
  }
}
///////////////////////////////////////////////////////////////////////////////////////
void writePatch(byte patch) {
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
  uint8_t outBuf[20];uint8_t sysexBuf[20];uint8_t size_buf=0;
  //char buf[20];
  Midi.SendSysEx(message1,6); //ativa modo editor
  Midi.SendSysEx(message2,6); //requisita patch
  while (size_buf!=2){
    //size_buf = Midi.RecvData(rcvd);
    if ( (size_buf = Midi.RecvRawData(outBuf)) > 0 ) {
      Midi.extractSysExData(outBuf, sysexBuf);
      /*for (i = 0; i <= size_buf; i++) {
        sprintf(buf, " %02u", outBuf[i]);
        Serial.print(buf);
      }
      Serial.println("");*/
    }
  }
  //Serial.println(outBuf[y]);
  patch=outBuf[size_buf];
  size_buf=0;
  Midi.SendSysEx(message3,6); //desativa modo editor
  //return patch;
}
void load_patches() {
  byte k=0;
  for (int i=0;i<NUM_BANKS;i++){
    for (int j=0;j<NUM_PATCHES;j++){
      //EEPROM.update(k++,0); //reseta EPROM
      bank_patch[i][j]=EEPROM.read(k++); //carrega patchs da EPROM
    }
  }
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
    foot_patch=0; //posição 0 = patch A no footswitch
    bt_patch=1; //indica que um botão foi pressionado
    Serial.print("A ");
  }
  if (digitalRead(BT_B) != btB) {
    btB = digitalRead(BT_B);
    foot_patch=1; //posição 1 = patch B no footswitch
    bt_patch=1; //indica que um botão foi pressionado
    Serial.print("B ");
  }
  if (digitalRead(BT_C) != btC) {
    btC = digitalRead(BT_C);
    foot_patch=2; //posição 2 = patch C no footswitch
    bt_patch=1; //indica que um botão foi pressionado
    Serial.print("C ");
  }
  if (digitalRead(BT_D) != btD) {
    btD = digitalRead(BT_D);
    foot_patch=3; //posição 3 = patch D no footswitch
    bt_patch=1; //indica que um botão foi pressionado
    Serial.print("D ");
    
  }
  if (digitalRead(BT_UP) != btUP) {
    if(bt5_mode==1){
      btUP = digitalRead(BT_UP);
      foot_patch=4; //posição 4 = patch E no footswitch
      bt_patch=1; //indica que um botão foi pressionado
      Serial.print("E ");        
    }
    else{
      btUP = digitalRead(BT_UP);
      if(bank==NUM_BANKS-1){
        bank=0;
      }
      else{
        bank++;
      }
      Serial.println("UP bank:" + (String)(bank));
      //delay(700); //evitar repique da chave
      foot_patch=0; //volta para posição A
      bt_patch=1; //indica que um botão foi pressionado
      bt_updown=1; //indica que o botão up ou down foi pressionado
    }
  }
  
  if (digitalRead(BT_DOWN) != btDOWN) {
    btDOWN = digitalRead(BT_DOWN);
    program_mode=!program_mode;
    Serial.println("Program mode = "+(String)program_mode);    
    /*
    if(bt5_mode==1){ //NESSE MODO, ELE FUNCIONA COMO UP
      if(bank==NUM_BANKS-1){
        bank=0;
      }
      else{
        bank++;
      }
      Serial.println("UP bank:" + (String)(bank));
      //delay(700); //evitar repique da chave
      foot_patch=0; //volta para posição A
      bt_patch=1; //indica que um botão foi pressionado
      bt_updown=1; //indica que o botão up ou down foi pressionado    
    }
    else{
      if(bank==0){
        bank=NUM_BANKS-1;
      }
      else{
        bank--;
      }
      Serial.println("DOWN bank:" + (String)(bank));
      //delay(700); //evitar repique da chave
      foot_patch=0; //volta para posição A
      bt_patch=1; //indica que um botão foi pressionado
      bt_updown=1; //indica que o botão up ou down foi pressionado
    }
    */
  }
  if (bt_patch==1){
    led_show();
//    display_set();
    if(program_mode==1 & bt_updown==0){
      loadPatch();
      byte pos_eprom= (bank*NUM_PATCHES)+foot_patch;//converte para posição na EPROM
      bank_patch[bank][foot_patch] = patch; //carrega patch da pedaleira na matriz de paches
      EEPROM.write(pos_eprom,patch);
      for(int i=0;i<5;i++){FastLED.setBrightness(0);FastLED.show();delay(70);FastLED.setBrightness(BRIGHTNESS);FastLED.show();delay(100);} //piscar led rápido indicando que programou
      Serial.println("Programou patch: "+(String)(bank_patch[bank][foot_patch]));
    }
    else{
      writePatch(bank_patch[bank][foot_patch]);
      Serial.println("Carregou patch: "+(String)(bank_patch[bank][foot_patch]));
      bt_updown=0;
    }
    bt_patch=0;
  }
}

void pin_config() {
  pinMode(BT_A, INPUT_PULLUP );
  pinMode(BT_B, INPUT_PULLUP );
  pinMode(BT_C, INPUT_PULLUP );
  pinMode(BT_D, INPUT_PULLUP );
  pinMode(BT_UP, INPUT_PULLUP);
  pinMode(BT_DOWN, INPUT_PULLUP);
}

void bt_read(){
  btA = digitalRead(BT_A);
  btB = digitalRead(BT_B);
  btC = digitalRead(BT_C);
  btD = digitalRead(BT_D);
  btUP = digitalRead(BT_UP);
  btDOWN = digitalRead(BT_DOWN);
}

/*
void display_config(){
  display.clear();
  display.setBrightness(0x0f);
}

void display_set(){
  display.clear();
  uint8_t display_data[4];
  //display.showNumberDec(foot_patch+1, false);
  //display.showNumberDec(bank+1, false,1,1);
  display_data[1]=display.encodeDigit(bank+1);
  display_data[2]=SEG_G;
  display_data[3]=display.encodeDigit(foot_patch+10);
  display.setSegments(display_data);
  //display_data[0] = display.encodeDigit(0);
  //display_data[1] = display.encodeDigit(1);
  //display.setSegments(display_data);
  //display.setBrightness(7, true); // Turn on
}
*/
void led_config(){
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.setBrightness(BRIGHTNESS);
}

void led_show(){
  FastLED.clear();
  leds[(NUM_LEDS-1)-foot_patch]= ColorFromPalette(RGB_colors, bank*16); //cor muda de acordo com bank 
  FastLED.show();
}


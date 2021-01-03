/* 
 * Projeto: Footswitch Programável para pedaleira ZOOM G3 - FSP01-ZG3
 * Autor: Túlio Vieira @tuliovieiraqdm
 * 
 * Modos de funcionamento e funções de cada botão do FS: 
 * 
 *            BOTÃO 1 | BOTÃO 2 | BOTÃO 3 | BOTÃO 4 | BOTÃO 5 | BOTÃO 6 
 *    MODO 4: Patch A | Patch B | Patch C | Patch D |   DOWN  |   UP
 *    MODO 5: Patch A | Patch B | Patch C | Patch D | Patch E |   UP
 *    MODO 6: Patch A | Patch B | Patch C | Patch D | Patch E | Patch F
 * 
 *    MODO PROG: Modo de programação
 *    MODO PLAY: Modo normal
 *    
 *    obs: UP e DOWN são para selecionar o próximo bank e o anterior, respectivamente.
 *         Cada bank mostra uma cor nos leds
 *    
 *    Para trocar entre os modos, pressionar BOTÃO 6 e outro botão simultaneamente.
 *    Cada botão seleciona um modo:
 *       BOTÃO 1 | BOTÃO 2 | BOTÃO 3 | BOTÃO 4 | BOTÃO 5 | BOTÃO 6
 *       MODO 4  | MODO 5  | MODO 6  | PROG    | PLAY    | (pressionado)
 *    
 * Para programar um patch numa posição do FOOT SWICH: 
 *  1º- Selecione o modo de programação (led irá piscar indicando que está no modo PROG)
 *  2º- Selecione o patch na pedaleira da ZOOM
 *  3º- Aperte o botão do Foot switch que deseja associar (led irá piscar mais rápido indicando que progamou)
 */
     
#include <Arduino.h> 
#include <EEPROM.h>
#include <SPI.h>
#include "Usb.h"
#include "usbhub.h"
#include "usbh_midi.h"
#include "FastLED.h"

#define NUM_BANKS    4  //número de banks
#define NUM_PATCHES  6  //número de patches por bank

//BOTÕES
#define BT_A    5
#define BT_B    4
#define BT_C    7
#define BT_D    6
#define BT_UP   2 
#define BT_DOWN 3
//
////LEDS
#define LED_PIN     8
#define NUM_LEDS    6  //1 pra cada patch
#define BRIGHTNESS  50 //brilho dos leds (0-255)

USB  Usb;
USBH_MIDI  Midi(&Usb);
CRGB leds[NUM_LEDS];
CRGBPalette16 RGB_colors(CRGB::Red,CRGB::Blue,CRGB::Green,CRGB::Yellow,CRGB::MediumVioletRed,CRGB::Aqua,CRGB::White,CRGB::Orange,CRGB::Red,CRGB::Blue,CRGB::Green,CRGB::Yellow,CRGB::MediumVioletRed,CRGB::Aqua,CRGB::White,CRGB::Orange);

//Funções
void pin_config(void);
void led_config(void);
void led_show(void);
void load_patches(void);
void loadPatch(void);
void bt_read(void);
void writePatch(byte patch);
void readPatch();
void bt_check(void);
String bank_to_letter();

//Variáveis globais
byte bank_patch[NUM_BANKS][NUM_PATCHES]; //matriz que armazena todos os patches de cada bank
byte patch = 0;
byte foot_patch=0;
byte bank=0;
String bank_letter="A";
bool program_mode=0; //0 - modo de programação / 1 - modo de seleção
byte bt_mode=4;     //4 - modo de 4 botões de patch / 5 - modo de 5 botões de patch / 6 - modo de 6 botões de patch 
unsigned int cont=0; //variável auxiliar para contagem de tempo

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
  Serial.begin(115200);
  //while (!Serial);
  Serial.println("INICIANDO...");
  if (Usb.Init() == -1) { // Teste se o USB foi conectado (aguarda conectar)
    Serial.println("USB NAO INICIOU");
    while (1);
  }
  Serial.println("USB INICIOU");
  //attachInterrupt (bt_A, bt_interrupt, CHANGE);
  
  while (Usb.getUsbTaskState()!=USB_STATE_RUNNING){Usb.Task();} //if USB is running, continue
  Serial.println("USB CONECTOU");
  
  for (int i = 0; i < 4; i++) {
    writePatch(i);  //os 4 primeiros comandos enviados pra pedaleira são ignorados (não sei por quê)
    delay(200);
  }

  pin_config();
  load_patches(); //carrega patches salvos
  led_config(); //configura leds e ajusta brilho
  //led_show();
  bt_read(); //lê os estados iniciais dos botões
  for(bank=0;bank<NUM_PATCHES;bank++){led_show();foot_patch++;delay(500);} //Liga leds em sequencia de cores
  bank=0;foot_patch=0; //Posição inicial default
  led_show();
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
    if(c=='+'){bank++;Serial.print("\nUP bank:" + (String)(bank));}
    if(c=='-'){bank--;Serial.print("\nDOWN bank:" + (String)(bank));}
    if(c=='*'){
      Serial.print("\nMEMÓRIA FLASH\n");
      for(int i=0;i<NUM_BANKS;i++){
        for(int j=0;j<NUM_PATCHES;j++){
          Serial.print(bank_patch[i][j]);
          Serial.print(" ");
        }
        Serial.print("\n");
      }
    }
    if(c=='/'){
      Serial.print("\nMEMÓRIA EPROM\n");
      for(int i=0;i<NUM_BANKS;i++){
        for(int j=0;j<NUM_PATCHES;j++){
          Serial.print(EEPROM.read(i*(NUM_PATCHES)+j));
          Serial.print(" ");
        }
        Serial.print("\n");
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

void bt_check(void) {
  bool shift=0; //Indica se o modo de funcionamento foi alterado
  //UP////////////////////////////////////////////////////////////////////(Momentary button)
  if (!digitalRead(BT_UP)){
    while (!digitalRead(BT_UP)){
      //Serial.print(".");
      if (digitalRead(BT_A) != btA){btA = digitalRead(BT_A);bt_mode=4;shift=1;} //ativa modo 4 botões
      if (digitalRead(BT_B) != btB){btB = digitalRead(BT_B);bt_mode=5;shift=1;} //ativa modo 5 botões
      if (digitalRead(BT_C) != btC){btC = digitalRead(BT_C);bt_mode=6;shift=1;} //ativa modo 6 botões
      if (digitalRead(BT_D) != btD){btD = digitalRead(BT_D);program_mode=1;shift=1;} //ativa modo de programação
      if (digitalRead(BT_DOWN) != btDOWN){btDOWN = digitalRead(BT_DOWN);program_mode=0;shift=1;} //desativa modo de programação
    }
    if(shift==0){
      if(bt_mode==4||bt_mode==5){   //Funciona como UP
        if(bank==NUM_BANKS-1) bank=0;
        else bank++;
        Serial.println("UP para bank: " + (String)bank);
        //delay(700); //evitar repique da chave
        bt_updown=1; //indica que o botão up ou down foi pressionado   
      }
      else{ //Funciona como PATCH F (bt_mode=6)
        foot_patch=5; //posição 5 = patch F no footswitch
        bt_patch=1; //indica que um botão de patch foi pressionado
      }
    }
    else{
      Serial.println("Mode: "+ (String)bt_mode);
    }
  }
  //DOWN//////////////////////////////////////////////////////////////////(Non-momentary button)
  if (digitalRead(BT_DOWN) != btDOWN) {
    if(bt_mode==5||bt_mode==6){ //Funciona como PATCH E
      btDOWN = digitalRead(BT_DOWN);
      foot_patch=4; //posição 4 = patch E no footswitch
      bt_patch=1; //indica que um botão de patch foi pressionado
    }
    else{ //Funciona como DOWN (bt_mode=4)
      btDOWN = digitalRead(BT_DOWN);
      if(bank==0) bank=NUM_BANKS-1;
      else bank--;
      Serial.println("DOWN para bank: " + (String)bank);
      //delay(700); //evitar repique da chave
      bt_updown=1; //indica que o botão up ou down foi pressionado 
    }
  }
  //PATCH A////////////////////////////////////////////////////////////////(Non-momentary button)
  if (digitalRead(BT_A) != btA) {
    btA = digitalRead(BT_A);
    foot_patch=0; //posição 0 = patch A no footswitch
    bt_patch=1; //indica que um botão de patch foi pressionado
  }
  //PATCH B////////////////////////////////////////////////////////////////(Non-momentary button)
  if (digitalRead(BT_B) != btB) {
    btB = digitalRead(BT_B);
    foot_patch=1; //posição 1 = patch B no footswitch
    bt_patch=1; //indica que um botão de patch foi pressionado
  }
  //PATCH C////////////////////////////////////////////////////////////////(Non-momentary button)
  if (digitalRead(BT_C) != btC) {
    btC = digitalRead(BT_C);
    foot_patch=2; //posição 2 = patch C no footswitch
    bt_patch=1; //indica que um botão de patch foi pressionado
  }
  //PATCH D////////////////////////////////////////////////////////////////(Non-momentary button)
  if (digitalRead(BT_D) != btD) {
    btD = digitalRead(BT_D);
    foot_patch=3; //posição 3 = patch D no footswitch
    bt_patch=1; //indica que um botão de patch foi pressionado
  }
  
  if (bt_patch==1){
    led_show();
    if(program_mode==1){
      loadPatch();
      byte pos_eprom= (bank*NUM_PATCHES)+foot_patch;//converte para posição na EPROM
      bank_patch[bank][foot_patch] = patch; //carrega patch da pedaleira na matriz de paches
      EEPROM.write(pos_eprom,patch); //carrega patch da pedaleira na memória EPROM
      for(int i=0;i<5;i++){FastLED.setBrightness(0);FastLED.show();delay(70);FastLED.setBrightness(BRIGHTNESS);FastLED.show();delay(100);} //piscar led rápido indicando que programou
      Serial.println(bank_to_letter()+(String)(bank)+" programou patch: "+(String)(bank_patch[bank][foot_patch]));
    }
    else{
      writePatch(bank_patch[bank][foot_patch]); //carrega patch na pedaleira
      Serial.println(bank_to_letter()+(String)(bank)+" carregou patch: "+(String)(bank_patch[bank][foot_patch]));
    }
    bt_patch=0;
  }
  else if (bt_updown==1){
    foot_patch=0; //volta para posição A
    led_show();
    writePatch(bank_patch[bank][foot_patch]); //carrega patch na pedaleira
    Serial.println(bank_to_letter()+(String)(bank)+" carregou patch: "+(String)(bank_patch[bank][foot_patch]));
    bt_updown=0;
  }
}

void pin_config() {
  pinMode(BT_A, INPUT_PULLUP );
  pinMode(BT_B, INPUT_PULLUP );
  pinMode(BT_C, INPUT_PULLUP );
  pinMode(BT_D, INPUT_PULLUP );
  pinMode(BT_DOWN, INPUT_PULLUP);
  pinMode(BT_UP, INPUT_PULLUP);
}

void bt_read(){
  btA = digitalRead(BT_A);
  btB = digitalRead(BT_B);
  btC = digitalRead(BT_C);
  btD = digitalRead(BT_D);
  btDOWN = digitalRead(BT_DOWN);
  //btUP = digitalRead(BT_UP);
}

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

String bank_to_letter(){
  switch (foot_patch)
  {
  case 0:
    bank_letter="A";
    break;
  case 1:
    bank_letter="B";
    break;
  case 2:
    bank_letter="C";
    break;
  case 3:
    bank_letter="D";
    break;
  case 4:
    bank_letter="E";
    break;  
  case 5:
    bank_letter="F";
    break;  
  default:
    break;
  }
  return bank_letter;
}
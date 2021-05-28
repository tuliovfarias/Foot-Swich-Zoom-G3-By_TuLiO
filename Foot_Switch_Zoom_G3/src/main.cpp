/* 
 * Projeto: Footswitch Programável para pedaleira ZOOM G3/G3X
 * Versão: 2.0.0
 * Autor: Túlio Vieira Farias @tuliovieiraqdm
 * Github: https://github.com/tuliovfarias/Foot-Swich-Zoom-G3-By_TuLiO
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
 *         Cada bank tem uma cor nos leds
 *    
 *    Para trocar entre os modos, pressionar BOTÃO 6 por 3 segundos e depois, o modo desejado.
 *    Cada botão seleciona um modo:
 *       BOTÃO 1 | BOTÃO 2 | BOTÃO 3 | BOTÃO 4 | BOTÃO 5 | BOTÃO 6
 *       MODO 4  | MODO 5  | MODO 6  | PROG    | PLAY    | (pressionado)
 *    
 * Para programar um patch numa posição do FOOTSWICH: 
 *  1º- Selecione o modo de programação (leds irão piscar lentamente indicando que está no modo PROG)
 *  2º- Selecione o patch na pedaleira da ZOOM
 *  3º- Aperte o botão do Foot switch que deseja carregar o path (led irá piscar mais rápido indicando que progamou)
 */
     
#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>
#include "Usb.h"
#include "usbhub.h"
#include "usbh_midi.h"
#include "FastLED.h"
//#include "TimerOne.h"

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
CRGB leds[NUM_LEDS+1];
CRGBPalette16 RGB_colors(CRGB::Red,CRGB::Blue,CRGB::Green,CRGB::Yellow,CRGB::MediumVioletRed,CRGB::Aqua,CRGB::White,CRGB::Orange,CRGB::Red,CRGB::Blue,CRGB::Green,CRGB::Yellow,CRGB::MediumVioletRed,CRGB::Aqua,CRGB::White,CRGB::Orange);

//Funções
void pin_config(void);
void led_config(void);
void led_show(void);
void load_from_EPROM(void);
void loadPatch(void);
void bt_read(void);
void writePatch(byte g3_patch);
void readPatch();
void bt_check(void);
String bank_to_letter();
void hold_function();

//Variáveis globais
byte bank_patch[NUM_BANKS][NUM_PATCHES]; //matriz que armazena todos os patches de cada bank
byte g3_patch = 0; //patch da pedaleira [0-99] (A0 a J9) 
byte fs_patch=0; //Número que representa banco 0-5
String fs_patch_letter="A"; //Letra que representa banco A-F
byte fs_bank=0; //Banco do footswitch (0 a NUM_BANKS)

bool program_mode=0; //0 - modo de programação / 1 - modo de seleção
byte bt_mode=4;     //4 - modo de 4 botões de patch / 5 - modo de 5 botões de patch / 6 - modo de 6 botões de patch 
unsigned int cont=0; //variável auxiliar para contagem de tempo
bool hold=0; //Indica se entrou no modo de configuração (HOLD pressionado por 3 segundos)
bool hold_flag=0; //Indica se entrou no modo de configuração (HOLD pressionado por 3 segundos)

bool bt_patch=1; //indica se um botão é pressionado (coeça em 1 para carregar no início)
bool bt_updown=0; //indica se o botão UP ou DOWN foi pressionado
bool btA; //Armazenha estado do botão (non momentary) pois cada vez que aperta, inverte o estado.
bool btB;
bool btC;
bool btD;
bool btUP;
bool btDOWN;
int j=0;

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
  load_from_EPROM(); //carrega patches salvos na EPROM
  led_config(); //configura leds e ajusta brilho
  //led_show();
  bt_read(); //lê os estados iniciais dos botões
  for(fs_bank=0;fs_bank<NUM_PATCHES;fs_bank++){led_show();fs_patch++;delay(400);} //Liga leds em sequencia de cores
  fs_bank=0;fs_patch=0; //Posição inicial default: A0
  led_show();
  //Midi.SendSysEx(message1,6); //ativa modo editor

  // Configuração do timer1 
  TCCR1A = 0;  //configura timer para operação normal pinos OC1A e OC1B desconectados
}

///////////////////////////////////////////////////////////////////////////////////////
unsigned int i=0;
bool x = false;
void loop() {
  bt_check();
  //piscar led no modo de programação
  if (hold==0){
    if((program_mode==1)){
      i++;
      if(i==200){x=!x;i=0;}
      if(x){FastLED.setBrightness(0);FastLED.show();}
      else{for (int j=0;j<bt_mode;j++)leds[j+NUM_LEDS-bt_mode]= ColorFromPalette(RGB_colors, fs_bank*16);FastLED.setBrightness(BRIGHTNESS);FastLED.show();}
    }
    else if(x){FastLED.setBrightness(BRIGHTNESS);led_show();x=0;}
  }  
  
  //Gravar patches manualmente:
  /*if(Serial.available()){
    char c = Serial.read();
    if(c=='1'){program_mode=!program_mode;Serial.println("Program mode = "+(String)program_mode);}
    if(c=='+'){fs_bank++;Serial.print("\nUP fs_bank:" + (String)(fs_bank));}
    if(c=='-'){fs_bank--;Serial.print("\nDOWN fs_bank:" + (String)(fs_bank));}
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
    //elseif (c=='p')fs_bank++;  
  }*/
}
///////////////////////////////////////////////////////////////////////////////////////
void writePatch(byte g3_patch) {
  Usb.Task();
  //Serial.println("TASK");
  if ( Usb.getUsbTaskState() == USB_STATE_RUNNING ) //if USB is running, continue
  {
    //Serial.println("RUNNING");
    byte Message[2];                 // Construct the midi message (2 bytes)
    Message[0] = 0xC0;               // 0xC0 is for Program Change (Change to MIDI channel 0)
    Message[1] = g3_patch;             // g3_patch [0-99]
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
  g3_patch=outBuf[size_buf];
  size_buf=0;
  Midi.SendSysEx(message3,6); //desativa modo editor
  //return g3_patch;
}
void load_from_EPROM() {
  byte k=0;
  for (int i=0;i<NUM_BANKS;i++){
    for (int j=0;j<NUM_PATCHES;j++){
      //EEPROM.update(k++,0); //reseta EPROM
      bank_patch[i][j]=EEPROM.read(k++); //carrega patchs da EPROM
    }
  }
}

void bt_check(void) {
  //UP////////////////////////////////////////////////////////////////////(Momentary button)
  if (!digitalRead(BT_UP)){
    Serial.println("HOLD PRESSED...");
    TCCR1B = 0;TCCR1B |= (1<<CS10)|(1 << CS12);  // configura prescaler para 1024: CS12 = 1 e CS10 = 1
    TCNT1 = 0xC900; // valor para overflow ocorrer em ~ 3 segundos (2^16-(3s*8MHz/1024))
    TIFR1  = (1 << TOV0); // limpar flag de interrupção
    TIMSK1 |= (1 << TOIE1); // habilita a interrupção do TIMER1
    while (!digitalRead(BT_UP)){

    }
    Serial.println("HOLD UNPRESSED");
    TCCR1B &= ~(1<< CS12);TCCR1B &= ~(1<< CS11);TCCR1B &= ~(1<< CS10); //Parar timer
    if(hold==0){ // Se o botão HOLD não foi pressionado por 3 segundos, funciona na função normal
      if(bt_mode==4||bt_mode==5){   //Funciona como UP
        if(fs_bank==NUM_BANKS-1) fs_bank=0;
        else fs_bank++;
        Serial.println("UP para fs_bank: " + (String)fs_bank);
        //delay(700); //evitar repique da chave
        bt_updown=1; //indica que o botão up ou down foi pressionado   
      }
      else{ //Funciona como PATCH F (bt_mode=6)
        fs_patch=5; //posição 5 = patch F no footswitch
        bt_patch=1; //indica que um botão de patch foi pressionado
      }
    }
  }
  //DOWN//////////////////////////////////////////////////////////////////(Non-momentary button)
  if (digitalRead(BT_DOWN) != btDOWN) {
    if(hold==0){
      if(bt_mode==5||bt_mode==6){ //Funciona como PATCH E
        btDOWN = digitalRead(BT_DOWN);
        fs_patch=4; //posição 4 = patch E no footswitch
        bt_patch=1; //indica que um botão de patch foi pressionado
      }
      else{ //Funciona como DOWN (bt_mode=4)
        btDOWN = digitalRead(BT_DOWN);
        if(fs_bank==0) fs_bank=NUM_BANKS-1;
        else fs_bank--;
        Serial.println("DOWN para fs_bank: " + (String)fs_bank);
        //delay(700); //evitar repique da chave
        bt_updown=1; //indica que o botão up ou down foi pressionado 
      }
    }
    else{
      btDOWN = digitalRead(BT_DOWN);
      program_mode=0; //desativa modo de programação
      hold=0;
      FastLED.clear();leds[1]= ColorFromPalette(RGB_colors, fs_bank*16);
      for(int i=0;i<3;i++){FastLED.setBrightness(0);FastLED.show();delay(70);FastLED.setBrightness(BRIGHTNESS);FastLED.show();delay(100);} //piscar led rápido indicando que configurou modo
      led_show();
      Serial.println("Mode: "+ (String)bt_mode);
    }
  }
  //PATCH A////////////////////////////////////////////////////////////////(Non-momentary button)
  if (digitalRead(BT_A) != btA) {
    if(hold==0){
      btA = digitalRead(BT_A);
      fs_patch=0; //posição 0 = patch A no footswitch
      bt_patch=1; //indica que um botão de patch foi pressionado
    }
    else{
      btA = digitalRead(BT_A);
      bt_mode=4;
      hold=0;
      FastLED.clear();leds[5]= ColorFromPalette(RGB_colors, 0*16);
      for(int i=0;i<3;i++){FastLED.setBrightness(0);FastLED.show();delay(70);FastLED.setBrightness(BRIGHTNESS);FastLED.show();delay(100);} //piscar led rápido indicando que configurou modo
      led_show();
      Serial.println("Mode: "+ (String)bt_mode);
      
    }
  }
  //PATCH B////////////////////////////////////////////////////////////////(Non-momentary button)
  if (digitalRead(BT_B) != btB) {
    if(hold==0){
      btB = digitalRead(BT_B);
      fs_patch=1; //posição 1 = patch B no footswitch
      bt_patch=1; //indica que um botão de patch foi pressionado
    }
    else{
      btB = digitalRead(BT_B);
      bt_mode=5;
      hold=0;
      FastLED.clear();leds[4]= ColorFromPalette(RGB_colors, 1*16);
      for(int i=0;i<3;i++){FastLED.setBrightness(0);FastLED.show();delay(70);FastLED.setBrightness(BRIGHTNESS);FastLED.show();delay(100);} //piscar led rápido indicando que configurou modo
      led_show();
      Serial.println("Mode: "+ (String)bt_mode);
    }
  }
  //PATCH C////////////////////////////////////////////////////////////////(Non-momentary button)
  if (digitalRead(BT_C) != btC) {
    if(hold==0){
      btC = digitalRead(BT_C);
      fs_patch=2; //posição 2 = patch C no footswitch
      bt_patch=1; //indica que um botão de patch foi pressionado
    }
    else{
      btC = digitalRead(BT_C);
      bt_mode=6;
      hold=0;
      FastLED.clear();leds[3]= ColorFromPalette(RGB_colors, 2*16);
      for(int i=0;i<3;i++){FastLED.setBrightness(0);FastLED.show();delay(70);FastLED.setBrightness(BRIGHTNESS);FastLED.show();delay(100);} //piscar led rápido indicando que configurou modo
      led_show();
      Serial.println("Mode: "+ (String)bt_mode);    }
  }
  //PATCH D////////////////////////////////////////////////////////////////(Non-momentary button)
  if (digitalRead(BT_D) != btD) {
    if(hold==0){
      btD = digitalRead(BT_D);
      fs_patch=3; //posição 3 = patch D no footswitch
      bt_patch=1; //indica que um botão de patch foi pressionado
    }
    else{
      btD = digitalRead(BT_D);
      program_mode=1; //ativa o modo de programação
      hold=0;
      FastLED.clear();
      delay(400); //para evitar duplo clique
      //led_show();
    }
  }
  //Carregar patches:////////////////////////////////////////////////////////////////////////////
  if (bt_patch==1){
    led_show();
    if(program_mode==1){
      loadPatch();
      byte pos_eprom= (fs_bank*NUM_PATCHES)+fs_patch;//converte para posição na EPROM
      bank_patch[fs_bank][fs_patch] = g3_patch; //carrega patch da pedaleira na matriz de paches
      EEPROM.write(pos_eprom,g3_patch); //carrega patch da pedaleira na memória EPROM
      for(int i=0;i<5;i++){FastLED.setBrightness(0);FastLED.show();delay(70);FastLED.setBrightness(BRIGHTNESS);FastLED.show();delay(100);} //piscar led rápido indicando que programou
      Serial.println(bank_to_letter()+(String)fs_bank+" programou patch: "+(String)(bank_patch[fs_bank][fs_patch]));
    }
    else{
      writePatch(bank_patch[fs_bank][fs_patch]); //carrega patch na pedaleira
      Serial.println(bank_to_letter()+(String)fs_bank+" carregou patch: "+(String)(bank_patch[fs_bank][fs_patch]));
    }
    bt_patch=0;
  }
  else if (bt_updown==1){
    fs_patch=0; //volta para posição A
    led_show();
    writePatch(bank_patch[fs_bank][fs_patch]); //carrega patch na pedaleira
    Serial.println(bank_to_letter()+(String)fs_bank+" carregou patch: "+(String)(bank_patch[fs_bank][fs_patch]));
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
  leds[(NUM_LEDS-1)-fs_patch]= ColorFromPalette(RGB_colors, fs_bank*16); //cor muda de acordo com fs_bank 
  FastLED.show();
}

String bank_to_letter(){
  switch (fs_patch)
  {
  case 0:
    fs_patch_letter="A";
    break;
  case 1:
    fs_patch_letter="B";
    break;
  case 2:
    fs_patch_letter="C";
    break;
  case 3:
    fs_patch_letter="D";
    break;
  case 4:
    fs_patch_letter="E";
    break;  
  case 5:
    fs_patch_letter="F";
    break;  
  default:
    break;
  }
  return fs_patch_letter;
}

ISR(TIMER1_OVF_vect){ //interrupção do TIMER1 
  hold=1; Serial.println("hold = 1");
  program_mode=0;
  for (j=0;j<=NUM_LEDS;j++){ // Liga todos os leds coloridos
    leds[NUM_LEDS-j]= ColorFromPalette(RGB_colors, ((j-1)*16));
  }
  FastLED.setBrightness(BRIGHTNESS/2);FastLED.show();
}
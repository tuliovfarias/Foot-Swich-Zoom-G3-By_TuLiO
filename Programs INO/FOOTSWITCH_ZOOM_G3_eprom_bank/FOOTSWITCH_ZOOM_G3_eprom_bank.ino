#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>
#include <Usb.h>
#include <usbhub.h>
#include <usbh_midi.h>
#include <TM1637Display.h>

#define NUM_BANKS   10  //número de banks
#define NUM_PATCHES  4  //número de patches por bank

//BOTÕES
#define BT_A    5
#define BT_B    4
#define BT_C    7
#define BT_D    6
//#define BT_UP  
//#define BT_DOWN 3
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
void loadPatch(void);
void bt_read(void);
void writePatch(byte patch);
void readPatch();
void bt_check(void);

//Variáveis globais
uint8_t display_data[] = { 0xff, 0xff, 0xff, 0xff };

byte bank_patch[NUM_BANKS][NUM_PATCHES]; //matriz que armazena todos os patches de cada bank
byte patch = 0;
byte foot_patch=0;
byte bank=0;
bool program_mode=0; //0 - modo de programação / 1 - modo de seleção

bool bt_on; //indica se um botão é pressionado
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
  //Midi.SendSysEx(message1,6); //ativa modo editor
}

///////////////////////////////////////////////////////////////////////////////////////
void loop() {
  bt_check();
  if(Serial.available()){
    Serial.read();
    program_mode=!program_mode;
    Serial.println("program mode = "+(String)program_mode);
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
    for (int j=0;j<NUM_BANKS;j++){
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
    bt_on=1; //indica que um botão foi pressionado
    Serial.println("A");
  }
  if (digitalRead(BT_B) != btB) {
    btB = digitalRead(BT_B);
    foot_patch=1; //posição 1 = patch B no footswitch
    bt_on=1; //indica que um botão foi pressionado
    Serial.println("B");
  }
  if (digitalRead(BT_C) != btC) {
    btC = digitalRead(BT_C);
    foot_patch=2; //posição 2 = patch C no footswitch
    bt_on=1; //indica que um botão foi pressionado
    Serial.println("C");
  }
  if (digitalRead(BT_D) != btD) {
    btD = digitalRead(BT_D);
    foot_patch=3; //posição 3 = patch D no footswitch
    bt_on=1; //indica que um botão foi pressionado
    Serial.println("D");
    
  }/*
  if (digitalRead(BT_UP) != btUP) {
    Serial.println("UP" + (String)(bank));
  }
  
  if (digitalRead(BT_DOWN) != btDOWN) {
    Serial.println("DOWN" + (String)(bank));
  }
  */
  if (bt_on==1){
    byte end_eprom= (bank*NUM_PATCHES)+foot_patch;//converte para posição na EPROM
    if(program_mode==1){
      loadPatch();
      bank_patch[bank][foot_patch] = patch; //carrega patch da pedaleira na matriz de paches
      EEPROM.write(end_eprom,patch);
      Serial.println("progamou!");
      //delay(100);
      //Serial.println("Programou patch: "+(String)(patch));
      //Serial.println("em patch: "+(String)(foot_patch)+" bank: "+(String)(bank));
    }
    else{
      writePatch(bank_patch[bank][foot_patch]);
      Serial.println("carregou!");
      //Serial.println("Carregou patch: "+(String)(patch));
      //Serial.println("em patch: "+(String)(foot_patch)+" bank: "+(String)(bank));
    }
    bt_on=0;
    //delay(100); //evitar repique
  }
}

void pin_config() {
  pinMode(BT_A, INPUT_PULLUP );
  pinMode(BT_B, INPUT_PULLUP );
  pinMode(BT_C, INPUT_PULLUP );
  pinMode(BT_D, INPUT_PULLUP );
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
  btB = digitalRead(BT_B);
  btC = digitalRead(BT_C);
  btD = digitalRead(BT_D);
}

void display_config(){
  display.setBrightness(0x0f);
}

void display_set(){
  //display_data[0] = display.encodeDigit(0);
  display.setSegments(display_data);
}




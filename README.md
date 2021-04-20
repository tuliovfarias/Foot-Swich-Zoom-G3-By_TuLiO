# Foot-Swich-Zoom-G3 - By TuLiO
 Foot switch programável para pedaleira Zoom G3/G3X 
 Video de demostração do funcionamento: https://youtu.be/4s4xHeftQxM
 
 ![Foto](https://user-images.githubusercontent.com/39657511/115471423-5852df00-a20e-11eb-9cc1-b21aef2c4f86.png)
 
 # Materiais
 - Arduino Pro Mini 3.3V 8Mhz
 - FTDI programmer - Ft232rl
 - Mini Host Shield
 - 6 leds RGB programáveis
 - Fios de rede
 - Caixa para pedal 6 furos
 - 6 chaves para pedal (botões)
 - Conector fêmea de fonte
 - Folha adesiva
 
 # Instruções
- No HostShield, cortar trilha que liga VBUS ao VCC 3.3V  (traço vermelho da imagem abaixo) e ligar VBUS ao pino RAW do Arduino (para alimentar a pedaleira com 5V e não 3.3V)

![usb-host-shield-mini-vbus-mod](https://user-images.githubusercontent.com/39657511/115169232-d8086e80-a093-11eb-8dc7-eab545d5dd18.jpg)
- Encaixar o Arduino sobre o HostShield e soldar os pinos 9 a 13, RST, VCC e GND.

![Host_Arduino jpg](https://user-images.githubusercontent.com/39657511/115168876-cc687800-a092-11eb-8ee1-01b8fb302477.png)
- Ligar os pinos 2 a 7 do Arduino no comum das chaves usando os fios e nos outros dois pinos da chave ligar VCC e GND (do conector de fonte).
- Ligar pino 8 do Arduino no DI do primeiro led e VCC e GND (do conector de fonte).
- Ligar o FTDI no Arduino

![FTDI_Arduino_Pro_Mini](https://user-images.githubusercontent.com/39657511/115168754-57953e00-a092-11eb-9f70-8a057418d2eb.png)
- Instalar driver do FTDI: HARDWARES\FTDI programmer\CDM21228_Setup.zip
- Na IDE, selecionar "Arduino Pro or Pro Mini"

# Vesões
 # v1.0

 Botão HOLD precisa ficar segurado para ativar as funções secundárias (MODE 4,5,6 e PROGRAM)

 # v2.0 - Em andamento...

 Botão HOLD precisa ficar pressionado por 3 segundos até que LEDs coloridos se acendam.
 Depois, só selecionar a função secundária.
 - Uma das chaves precisa ser momentary

# Referências
 - https://www.talkbass.com/threads/diy-zoom-b3-g3-foot-controller-android-app.1160429/
 - https://alselectro.wordpress.com/2017/05/14/arduino-pro-mini-how-to-upload-code/
 - https://geekhack.org/index.php?topic=80421.0
 - https://diyelectromusic.wordpress.com/2020/08/01/mini-usb-midi-to-midi/
 - https://github.com/vegos/ZoomG3_ArduinoMIDI
 - https://github.com/sixeight7/VController_v3

# Bibliotecas:
- USB_Host_Shield_2.0 - https://github.com/felis/USB_Host_Shield_2.0
- TimerOne - https://github.com/PaulStoffregen/TimerOne
- FastLED - https://github.com/FastLED/FastLED

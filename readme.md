ArduRadio: Playing Internet Radio with Arduino
===============================================
This project allows to play Internet Radio using Arduino. The project uses NFC tags (Type 2 -Mifare Ultralight) to switch stations.

A more detailed description of the project can be found in http://www.slideshare.net/JavierMontaner/arduradio You can also check the images folder in this github project for some pictures

For further info you can contact the author Javier Montaner (twitter: https://twitter.com/tumaku_)

Required HW:

- Arduino Mega
- Official Arduino Ethernet shield
- MP3 shield from SparkFun Electronics
- NFC shield from SeeedStudio

The libraries of the MP3 shield (by Sparkfun Electronics and modified by Bill Porter) and for the NFC Shield (by SeeedStudio based on the work of Adafruit/Ladyada) have been updated to add new functionality.

Instructions
------------------------
### Library instructions

Copy the two folders in the libraries folder of this project (PN532Seeed, SFEMP3Shield) inside the libraries folder of your Arduino installation. The location of the libraries folder depends on your OS and installation but it is usually located just under your Arduino home directory.

### Stacking instructions

- Top Level:    NFC shield
- 2nd Level:    MP3 shield
- 1st Level:    Ethernet shield
- Bottom Level: Arduino Mega

You need to bend some PINs in the shields to avoid connecting certain hardcoded PIN values in the Arduino libraries. To avoid damaging the shields I have used stackable headers between Ethernet and MP3 shields and between MP3 and NFC shields. Bending a stackable PIN will not damage your shield and if it gets broken it is easier and cheaper to replace.

### Wiring instructions

#### Ethernet shield
Just plug on top  of Arduino Mega. No special wiring required

#### MP3 shield
Plug on top of Ethernet shield using stackable headers for all the PINs (2 x 6-PIN headers plus 2 x 8-PIN headers).
Bend digital PIN 7 of MP3 shield to avoid connection to homonym PIN in Ethernet shield. This PIN is hardwired in the MP3 shield as the SPI chip select (SS) PIN to transfer data (audio) to the shield.
Connect a jumper wire:
- PIN 7 (SS) in MP3 shield -> PIN 45 in Arduino Mega 

The MP3 shield library has been updated to consider this change:

    #define MP3_XDCS 45 //Data Chip Select / BSYNC Pin  //JMG To reuse with MEGA and WiFi shield

This change has been implemented to avoid incompatibilities in the future with the official Arduino Wifi shield that uses PIN 7 for other purposes.

Connect jumper wires between the SPI PINs hardwired in the MP3 shield and those in the Arduino Mega:
- PIN 11 (MOSI) in MP3 shield -> PIN 51 in Arduino Mega
- PIN 12 (MISO) in MP3 shield -> PIN 50 in Arduino Mega
- PIN 13 (SCK)  in MP3 shield -> PIN 52 in Arduino Mega

#### NFC shield
Plug on top of MP3 shield using stackable headers:
- Use 1 x 6-PIN header for the power PINS (RST, 3.3V, 5V, GND, GND, Vin)
- Use 1 x 6-PIN header for digital PINs 0 to 5 
Most of these PIN connections are not required other than to hold the NFC shield attached to the other shields.

The NFC shield uses SPI but the PINs are hardwired not to the digital PINS (11, 12, 13) as in the MP3 shield but to the equivalent ICSP PINS.
Connect jumper wires between the SPI PINs hardwired in the ICSP plug of the NFC shield and those in the Arduino Mega:
- ICSP PIN MOSI in NFC shield -> PIN 46 in Arduino Mega
- ICSP PIN MISO in NFC shield -> PIN 47 in Arduino Mega
- ICSP PIN SCK  in NFC shield -> PIN 48 in Arduino Mega

Additionally the NFC shield requires to receive power (5V) through the ICSP plug. Connect an extra jumper wire:
- ICSP PIN Vcc in NFC shield -> Power PIN 5V in NFC shield

One last jumper wire needs to be connected for the SPI chip select (SS) PIN of the NFC chip. Connect a jumper wire:
- PIN 10 (SS) in NFC shield -> PIN 49 in Arduino Mega 

The NFC shield library allows the personalisation of this PIN values by SW at initialisation. The Arduino sketch (.ino) is the place where these PIN values are defined (be careful not to define PINs already used for a different functionality)

    #define NFC_SS 49
    #define NFC_MISO 47
    #define NFC_MOSI 46
    #define NFC_SCK 48
    PN532 nfc(NFC_SCK, NFC_MISO, NFC_MOSI, NFC_SS);

Personalising NFC Tags with an Android Terminal
------------------------------------------------
You can use NXP application TagWriter. Select "Create and write" and then  "New","URL" and "Create new bookmark". Enter the URL of the audio stream/station and then press "Next" and bring the tag close to the handset. 

Note that although the TagWriter application can write NFC tags of any type, the current version of ArduRadio can only understand Type 2 tags (Mifare Ultralight)

Known Issues
-------------
- This set-up only works for audio streams up to 32kbps. Higher qualities cannot be processed fast enough by teh processor and result in bumpy audio (or no audio at all). Higher bitrates could theoretically be achievable with a faster and more powerful processor (e.g. the one implemented in teh new official Wifi shield)
- The NFClibrary can only read NFC tags Type 2 (Mifare Ultralight) and even then in a very harcoded way. A proper NFC/NDEF library must be written for a real and universal product. In the current version when an unknown tag is read or the NDEF cannot be processed, the code just looks for the "Next Station" no matter what the format of the tag is.

Esternal Links
----------------
- Jordi Parra's SpotifyRadio: Radio that plays Spotify music http://postscapes.com/spotify-box
- Bill Porter´s blog entry on MP3 shield: http://www.billporter.info/sparkfun-mp3-shield-arduino-library/
- (Rui) Techman's blog entry on VS1053 and MP3 shield: http://supertechman.blogspot.com.es/2010/11/playing-mp3-with-vs1053-arduino-shield.html

License
-------------
Copyright 2012 Javier Montaner

The *SFEMP3Shield library* is based on the work of Bill Porter (www.billporter.info) ans is opened under the following licensing terms:

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or(at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details. <http://www.gnu.org/licenses/>

This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License. To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.


The *PN532 library* is based on the library produced by SeeedStudio which originates from a previous work of adafruit/ladyada and is licensed under the MIT license.

The *rest of the files and work in this project* are licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License. To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.

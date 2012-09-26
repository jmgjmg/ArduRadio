ArduRadio: Playing Internet radioo with Arduino
===============================================

Arduino Library for Seeedstudio NFC Shield based on NXP PN532 chip

This project allows to play Internet Radio using Arduino. The project uses NFC tags (Type 2 -Mifare Ultralight) to switch stations.

Required HW:

- Arduino Mega
- official Arduino Ethernet shield
- MP3 shield from SparkFun Electronics
- NFC shield from SeeedStudio

The libraries of the MP3 shield (Sparkfun Electronics and modified by Bill Porter) and for the NFC Shield (SeeedStudio based on the work of Adafruit/Ladyada) have been updated to add new functionality.

Connection instructions:
------------------------
To be done

Personlaising NFC Tags with an Android terminal
------------------------------------------------
To be done


KNOWN ISSUES
-------------
To be done


Example sketch
--------------
This sample code initializes NFC and Ethernet shields and then enters an infinite loop waiting for 
the detection of NFC/RFID tags.
When a tag is detected, its ID is read and then uploaded to Evrythng server in internet using its 
http REST API.
Note that in order to use this service, you must obtain your own credentials from Evrythng 
(check instructions at evrythng.com). 
Alternatively, you can use other similar services available in Internet.

You have to retrieve your authentication token from https:evrythng.net/settings/tokens and update 
its value in your Arduino code: 

    client.println("X-Evrythng-Token: ...yourAPITokenHere...")
    
You must also crate a new thng in evrytng with a property called ReadTag. Once created, you have 
to update its thngId value in your Arduino code: 

    client.println("PUT http:evrythng.net/thngs/...yourThingIdHere.../properties/ReadTag HTTP/1.1");


License
-------------
Copyright 2012 Javier Montaner

The SFEMP3Shield library is based on the work of Bill Porter (www.billporter.info) ans is opened under the following licensing terms:

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or(at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details. <http://www.gnu.org/licenses/>

This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License. To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.


The PN532 library is based on the library produced by SeeedStudio which originates from a previous work of adafruit/ladyada and is licensed under the MIT license.

The rest of the files and work in this project are licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License. To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.

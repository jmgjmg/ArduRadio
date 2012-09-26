// PN532 library by adafruit/ladyada
// MIT license

// authenticateBlock, readMemoryBlock, writeMemoryBlock contributed
// by Seeed Technology Inc (www.seeedstudio.com)

#include <Arduino.h>
#include "PN532Seeed.h"
#include "SPI.h"

//#define PN532DEBUG 1

byte pn532ack[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
byte pn532response_firmwarevers[] = {0x00, 0xFF, 0x06, 0xFA, 0xD5, 0x03};
byte mifare_ultralight_nfc_header[] = {0xE1,0x10,0x06};
byte nfc_http_www[] = {0x68,0x74,0x74,0x70,0x3A,0x2F,0x2F,0x77,0x77,0x77,0x2E,0x00};
byte nfc_http[] = {0x68,0x74,0x74,0x70,0x3A,0x2F,0x2F,0x00};


#define PN532_PACKBUFFSIZ 64
byte pn532_packetbuffer[PN532_PACKBUFFSIZ];
#define PN532_CS 10
PN532::PN532(uint8_t clk, uint8_t miso, uint8_t mosi, uint8_t ss) {
    _clk = clk;
    _miso = miso;
    _mosi = mosi;
    #if defined(__AVR_ATmega1280__)|| defined(__AVR_ATmega2560__)
    //_ss = PN532_CS;  //JMG removed because of physical repinning through wired connection (ICSP PINs bypassed/not used)
	_ss= ss; 
    pinMode(ss, OUTPUT);
    #else
    _ss = ss;
    #endif
    pinMode(_ss, OUTPUT);
    pinMode(_clk, OUTPUT);
    pinMode(_mosi, OUTPUT);
    pinMode(_miso, INPUT);
	urlBuffer[0]=0x00;
}

void PN532::begin() {
   digitalWrite(_ss, LOW);

    delay(1000);

    // not exactly sure why but we have to send a dummy command to get synced up
    pn532_packetbuffer[0] = PN532_FIRMWAREVERSION;
    sendCommandCheckAck(pn532_packetbuffer, 1);

    // ignore response!
}

void PN532::RFConfiguration(uint8_t mxRtyPassiveActivation) {
    pn532_packetbuffer[0] = PN532_RFCONFIGURATION;
    pn532_packetbuffer[1] = PN532_MAX_RETRIES; 
    pn532_packetbuffer[2] = 0xFF; // default MxRtyATR
    pn532_packetbuffer[3] = 0x01; // default MxRtyPSL
	pn532_packetbuffer[4] = mxRtyPassiveActivation;

	sendCommandCheckAck(pn532_packetbuffer, 5);
    // ignore response!
}
uint32_t PN532::getFirmwareVersion(void) {
    uint32_t response;

    pn532_packetbuffer[0] = PN532_FIRMWAREVERSION;

    if (! sendCommandCheckAck(pn532_packetbuffer, 1))
        return 0;

    // read data packet
    readspidata(pn532_packetbuffer, 12);
    // check some basic stuff
    if (0 != strncmp((char *)pn532_packetbuffer, (char *)pn532response_firmwarevers, 6)) {
        return 0;
    }

    response = pn532_packetbuffer[6];
    response <<= 8;
    response |= pn532_packetbuffer[7];
    response <<= 8;
    response |= pn532_packetbuffer[8];
    response <<= 8;
    response |= pn532_packetbuffer[9];

    return response;
}


// default timeout of one second
boolean PN532::sendCommandCheckAck(uint8_t *cmd, uint8_t cmdlen, uint16_t timeout) {
    uint16_t timer = 0;
    // write the command
    spiwritecommand(cmd, cmdlen);


    // Wait for chip to say its ready!
    while (readspistatus() != PN532_SPI_READY) {
        if (timeout != 0) {
            timer+=10;
            if (timer > timeout) {
                return false;
			}
        }
        delay(10);
    }

    // read acknowledgement
    if (!spi_readack()) {
        return false;
    }

    timer = 0;
    // Wait for chip to say its ready!
    while (readspistatus() != PN532_SPI_READY) {
        if (timeout != 0) {
            timer+=10;
            if (timer > timeout)
                return false;
        }
        delay(10);
    }

    return true; // ack'd command
}

boolean PN532::SAMConfig(void) {
    pn532_packetbuffer[0] = PN532_SAMCONFIGURATION;
    pn532_packetbuffer[1] = 0x01; // normal mode;
    pn532_packetbuffer[2] = 0x14; // timeout 50ms * 20 = 1 second
    pn532_packetbuffer[3] = 0x01; // use IRQ pin!

    if (! sendCommandCheckAck(pn532_packetbuffer, 4))
        return false;

    // read data packet
    readspidata(pn532_packetbuffer, 8);

    return  (pn532_packetbuffer[5] == 0x15);
}

uint32_t PN532::authenticateBlock(uint8_t cardnumber /*1 or 2*/,uint32_t cid /*Card NUID*/, uint8_t blockaddress /*0 to 63*/,uint8_t authtype/*Either KEY_A or KEY_B */, uint8_t * keys) {
    pn532_packetbuffer[0] = PN532_INDATAEXCHANGE;
    pn532_packetbuffer[1] = cardnumber;  // either card 1 or 2 (tested for card 1)
    if(authtype == KEY_A)
    {
        pn532_packetbuffer[2] = PN532_AUTH_WITH_KEYA;
    }
    else
    {
        pn532_packetbuffer[2] = PN532_AUTH_WITH_KEYB;
    }
    pn532_packetbuffer[3] = blockaddress; //This address can be 0-63 for MIFARE 1K card

    pn532_packetbuffer[4] = keys[0];
    pn532_packetbuffer[5] = keys[1];
    pn532_packetbuffer[6] = keys[2];
    pn532_packetbuffer[7] = keys[3];
    pn532_packetbuffer[8] = keys[4];
    pn532_packetbuffer[9] = keys[5];

    pn532_packetbuffer[10] = ((cid >> 24) & 0xFF);
    pn532_packetbuffer[11] = ((cid >> 16) & 0xFF);
    pn532_packetbuffer[12] = ((cid >> 8) & 0xFF);
    pn532_packetbuffer[13] = ((cid >> 0) & 0xFF);

    if (! sendCommandCheckAck(pn532_packetbuffer, 14))
        return false;

    // read data packet
    readspidata(pn532_packetbuffer, 2+6);

#ifdef PN532DEBUG
    for(int iter=0;iter<14;iter++)
    {
        Serial.print(pn532_packetbuffer[iter], HEX);
        Serial.print(" ");
    }
    Serial.println();
    // check some basic stuff

    Serial.println("AUTH");
    for(uint8_t i=0;i<2+6;i++)
    {
        Serial.print(pn532_packetbuffer[i], HEX); Serial.println(" ");
    }
#endif

    if((pn532_packetbuffer[6] == 0x41) && (pn532_packetbuffer[7] == 0x00))
    {
  	return true;
    }
    else
    {
  	return false;
    }

}

uint32_t PN532::readMemoryBlock(uint8_t cardnumber /*1 or 2*/,uint8_t blockaddress /*0 to 63*/, uint8_t * block) {
    pn532_packetbuffer[0] = PN532_INDATAEXCHANGE;
    pn532_packetbuffer[1] = cardnumber;  // either card 1 or 2 (tested for card 1)
    pn532_packetbuffer[2] = PN532_MIFARE_READ;
    pn532_packetbuffer[3] = blockaddress; //This address can be 0-63 for MIFARE 1K card

    if (! sendCommandCheckAck(pn532_packetbuffer, 4))
        return false;

    // read data packet
    readspidata(pn532_packetbuffer, 18+6);
    // check some basic stuff
#ifdef PN532DEBUG
    Serial.println("READ");
#endif
    for(uint8_t i=8;i<18+6;i++)
    {
        block[i-8] = pn532_packetbuffer[i];
#ifdef PN532DEBUG
        Serial.print(pn532_packetbuffer[i], HEX); Serial.print(" ");
#endif
    }
    if((pn532_packetbuffer[6] == 0x41) && (pn532_packetbuffer[7] == 0x00))
    {
  	return true; //read successful
    }
    else
    {
  	return false;
    }

}

//Do not write to Sector Trailer Block unless you know what you are doing.
uint32_t PN532::writeMemoryBlock(uint8_t cardnumber /*1 or 2*/,uint8_t blockaddress /*0 to 63*/, uint8_t * block) {
    pn532_packetbuffer[0] = PN532_INDATAEXCHANGE;
    pn532_packetbuffer[1] = cardnumber;  // either card 1 or 2 (tested for card 1)
    pn532_packetbuffer[2] = PN532_MIFARE_WRITE;
    pn532_packetbuffer[3] = blockaddress;

    for(uint8_t byte=0; byte <16; byte++)
    {
        pn532_packetbuffer[4+byte] = block[byte];
    }

    if (! sendCommandCheckAck(pn532_packetbuffer, 20))
        return false;
    // read data packet
    readspidata(pn532_packetbuffer, 2+6);

#ifdef PN532DEBUG
    // check some basic stuff
    Serial.println("WRITE");
    for(uint8_t i=0;i<2+6;i++)
    {
        Serial.print(pn532_packetbuffer[i], HEX); Serial.println(" ");
    }
#endif

    if((pn532_packetbuffer[6] == 0x41) && (pn532_packetbuffer[7] == 0x00))
    {
  	return true; //write successful
    }
    else
    {
  	return false;
    }
}

uint32_t PN532::readPassiveTargetID(uint8_t cardbaudrate) {
    uint32_t cid;

    pn532_packetbuffer[0] = PN532_INLISTPASSIVETARGET;
    pn532_packetbuffer[1] = 1;  // max 1 cards at once (we can set this to 2 later)
    pn532_packetbuffer[2] = cardbaudrate;
    if (! sendCommandCheckAck(pn532_packetbuffer, 3))
        return 0x0;  // no cards read

    // read data packet
    readspidata(pn532_packetbuffer, 20);
    // check some basic stuff

    Serial.print("Found "); Serial.print(pn532_packetbuffer[7], DEC); Serial.println(" tags");
    if (pn532_packetbuffer[7] != 1)
        return 0;
    
    uint16_t sens_res = pn532_packetbuffer[9];
    sens_res <<= 8;
    sens_res |= pn532_packetbuffer[10];
    uint8_t sel_res = pn532_packetbuffer[11];
    Serial.print("Sens Response: 0x");  Serial.println(sens_res, HEX);
    Serial.print("Sel Response: 0x");  Serial.println(sel_res, HEX);

    cid = 0;
    for (uint8_t i=0; i< pn532_packetbuffer[12]; i++) {
        cid <<= 8;
        cid |= pn532_packetbuffer[13+i];
        // Serial.print(" 0x"); Serial.print(pn532_packetbuffer[13+i], HEX);
    }

#ifdef PN532DEBUG
    Serial.println("TargetID");
    for(uint8_t i=0;i<20;i++)
    {
        Serial.print(pn532_packetbuffer[i], HEX); Serial.println(" ");
    }
#endif  
	
	urlBuffer[0]=0x00;  //reset URL

	if ((sens_res==0x0004) && (sel_res==0x08)) {Serial.println("Mifare Classic");}
	else if ((sens_res==0x0044) && (sel_res==0x00)){
	   Serial.println("Mifare Ultralight");
		for (uint8_t i=0;i<4;i++) {
		   readMemoryBlock(1, i*4, ultralightData+i*16);
#ifdef PN532DEBUG		   
		   Serial.println();
		   for (uint8_t j=0;j<16;j++) {
			 Serial.print(ultralightData[i*16+j], HEX); Serial.print(" ");
		   }
#endif  		   
		}	
		if (0 == strncmp((char *)ultralightData +12, (char *)mifare_ultralight_nfc_header, 3)) {
		   Serial.println("NFC/NDEF formatted tag");
		}
		findNdefUrlRecord((uint8_t*)ultralightData, 16, 64);
	}
    return cid;
}

/* Remark: TLV length value is not checked for out of bound attacks/errors or for long length value format 0xFF 0x.. ox..)
    NDEF Record parsing is "wrongly" hardcoded looking for URL as first record. Other formats will fail :-(
 */
 
bool PN532::findNdefUrlRecord(uint8_t* buff, int startPos, int maxPos) {
	int currentPos = startPos;
	bool urlFound = false;
	urlBuffer[0]=0x00;

	while (currentPos < maxPos) {
		byte currentTag=buff[currentPos];
		if (currentTag==0x00) {currentPos++;} //Null TLV found, skip to next one (no LV bytes)
		else if (currentTag==0xFE) {currentPos=maxPos;} //End TLV found; exit loop
		else if (currentTag!=0x03) {currentPos+=buff[currentPos+1]+2;} //Non NFC tags; discard and skip to the next tag
		else { //NFC NDEF tag found: process it
			currentPos+= 2; //move to first record in NDEF message
			if (buff[currentPos]!=0xD1){ 
				//First tag not an NFC Forum Well Known Type -> stop parsing and leave . This harcoding is not always correct
				currentPos=maxPos;
			} else if (buff[currentPos+3]!=0x55) { 
				   //not an NDEF URL record -> stop parsing and leave . This harcoding is not always correct
				   currentPos=maxPos; 
		    } else {//NFC URL record tag found: process it				 
			    int urlPayloadLength = buff[currentPos+2] -1;
				byte idCode= buff[currentPos+4];
				int urlPos =0;
				if (idCode==0x01) {
				    strcpy((char *)urlBuffer, (char *)nfc_http_www);
				} else if (idCode==0x03) {
					strcpy((char *)urlBuffer, (char *)nfc_http);
				}
				strncat((char *)urlBuffer, (char *)buff+currentPos+5,urlPayloadLength);
				urlFound = true;
				currentPos=maxPos; //stop parsing
			}					
		}
	}	
#ifdef PN532DEBUG	
	Serial.println("URL String: ");
	Serial.println((char*)urlBuffer);
	Serial.println("Return bool");
	Serial.println(urlFound);
	Serial.println("===========");
#endif  
	return urlFound;
}


char* PN532::getNdefUrl() {
	return (char *)urlBuffer;
}

//returns portValue; it returns 0 if the Url is not properly formatted
int PN532::parseUrl(char * server, char * path) {
	int currentPos=0;
	int portValue=0;
	char * portStart = NULL;
	char * pathStart = NULL;
	
	if(urlBuffer[0]==0x00) return 0;

	if (sscanf((char *)urlBuffer,"%*15[^:]://%s",server)==0) //remove prefix protocol
		sscanf((char *)urlBuffer,"%s",server);

		
	portStart=strchr(server,':'); 
	if (portStart==NULL) { //NO port included in URL
		portValue=80;
	    pathStart=strchr(server,'/');
		path[0]='/';
		if (pathStart==NULL) { //No path included in URL
			path[1]=0x00;
		} else {
			sscanf(server,"%80[^/]/%s",server,path+1);			
		}
	} else {	
		int result= sscanf(server,"%80[^:]:%u%s", server, &portValue, path); 
		if (result ==1) { //Port value is not numeric; error
			return 0;
		}
		if (result==2) { //Empty path
			path[0]='/';
			path[1]=0x00;
		}
		if ((result==3) && (path[0]!='/')) { //Wrong port or path format; error
			return 0;
		}
	}
	return portValue;
	
}


/************** high level SPI */


boolean PN532::spi_readack() {
    uint8_t ackbuff[6];

    readspidata(ackbuff, 6);

    return (0 == strncmp((char *)ackbuff, (char *)pn532ack, 6));
}

/************** mid level SPI */

uint8_t PN532::readspistatus(void) {
    digitalWrite(_ss, LOW);
    delay(2);
	
    spiwrite(PN532_SPI_STATREAD);

    // read byte
    uint8_t x = spiread();
	

    digitalWrite(_ss, HIGH);
    return x;
}

void PN532::readspidata(uint8_t* buff, uint8_t n) {
    digitalWrite(_ss, LOW);
    delay(2);
    spiwrite(PN532_SPI_DATAREAD);

#ifdef PN532DEBUG
    Serial.print("Reading: ");
#endif
    for (uint8_t i=0; i<n; i++) {
        delay(1);
        buff[i] = spiread();
#ifdef PN532DEBUG
        Serial.print(" 0x");
        Serial.print(buff[i], HEX);
#endif
    }

#ifdef PN532DEBUG
    Serial.println();
#endif

    digitalWrite(_ss, HIGH);
}

void PN532::spiwritecommand(uint8_t* cmd, uint8_t cmdlen) {
    uint8_t checksum;

    cmdlen++;

#ifdef PN532DEBUG
    Serial.print("\nSending: ");
#endif

    digitalWrite(_ss, LOW);
    delay(2);     // or whatever the delay is for waking up the board
    spiwrite(PN532_SPI_DATAWRITE);

    checksum = PN532_PREAMBLE + PN532_PREAMBLE + PN532_STARTCODE2;
    spiwrite(PN532_PREAMBLE);
    spiwrite(PN532_PREAMBLE);
    spiwrite(PN532_STARTCODE2);

    spiwrite(cmdlen);
    uint8_t cmdlen_1=~cmdlen + 1;
    spiwrite(cmdlen_1);

    spiwrite(PN532_HOSTTOPN532);
    checksum += PN532_HOSTTOPN532;

#ifdef PN532DEBUG
    Serial.print(" 0x"); Serial.print(PN532_PREAMBLE, HEX);
    Serial.print(" 0x"); Serial.print(PN532_PREAMBLE, HEX);
    Serial.print(" 0x"); Serial.print(PN532_STARTCODE2, HEX);
    Serial.print(" 0x"); Serial.print(cmdlen, HEX);
    Serial.print(" 0x"); Serial.print(cmdlen_1, HEX);
    Serial.print(" 0x"); Serial.print(PN532_HOSTTOPN532, HEX);
#endif

    for (uint8_t i=0; i<cmdlen-1; i++) {
        spiwrite(cmd[i]);
        checksum += cmd[i];
#ifdef PN532DEBUG
        Serial.print(" 0x"); Serial.print(cmd[i], HEX);
#endif
    }
    uint8_t checksum_1=~checksum;
    spiwrite(checksum_1);
    spiwrite(PN532_POSTAMBLE);
    digitalWrite(_ss, HIGH);

#ifdef PN532DEBUG
    Serial.print(" 0x"); Serial.print(checksum_1, HEX);
    Serial.print(" 0x"); Serial.print(PN532_POSTAMBLE, HEX);
    Serial.println();
#endif
} 
/************** low level SPI */

void PN532::spiwrite(uint8_t c) {
    int8_t i;
    digitalWrite(_clk, HIGH);

    for (i=0; i<8; i++) {
        digitalWrite(_clk, LOW);
        if (c & _BV(i)) {
            digitalWrite(_mosi, HIGH);
        } else {
            digitalWrite(_mosi, LOW);
        }
        digitalWrite(_clk, HIGH);
    }
}

uint8_t PN532::spiread(void) {
    int8_t i, x;
    x = 0;
    digitalWrite(_clk, HIGH);

    for (i=0; i<8; i++) {
        if (digitalRead(_miso)) {
            x |= _BV(i);
        }
        digitalWrite(_clk, LOW);
        digitalWrite(_clk, HIGH);
    }
    return x;
}

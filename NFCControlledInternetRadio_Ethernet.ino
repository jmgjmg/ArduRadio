/**************************************
*
*  Example for Sparkfun MP3 Shield Library
*      By: Bill Porter
*      www.billporter.info
*
*   Function:
*      This sketch listens for commands from a serial terminal (like the Serial Monitor in 
*      the Arduino IDE). If it sees 1-9 it will try to play an MP3 file named track00x.mp3
*      where x is a number from 1 to 9. For eaxmple, pressing 2 will play 'track002.mp3'.
*      A lowe case 's' will stop playing the mp3.
*      'f' will play an MP3 by calling it by it's filename as opposed to a track number. 
*
*      Sketch assumes you have MP3 files with filenames like 
*      "track001.mp3", "track002.mp3", etc on an SD card loaded into the shield. 
*
***************************************/

#include <SPI.h>

//Add the SdFat Libraries
#include <SD.h>

//and the MP3 Shield Library
#include <SFEMP3Shield.h>
#include <PN532Seeed.h>
#include <Ethernet.h>


//create and name the library object
SFEMP3Shield MP3player;

 
#define NFC_SS 49
#define NFC_MISO 47
#define NFC_MOSI 46
#define NFC_SCK 48
PN532 nfc(NFC_SCK, NFC_MISO, NFC_MOSI, NFC_SS);


byte temp;
byte result;

int totalReadData =0;
char title[30];
char artist[30];
char album[30];
byte mac[] = { 
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x01 };
IPAddress ip(192,168,1,20);
EthernetClient client;
int initialLoop =0;
int reconnectCounter=0;
unsigned long absoluteTime = 0;
#define ETHER_CS 53
uint16_t totalBytes=0;
int noReceptionCounter =0;
int speakerVolume =0;
char streamData[128];
unsigned long nfcTime =0;
uint32_t tagId=0;
int currentStation=0;
char serverString [][50]={"stream1.radiomonitor.com",
                        "stream1.radiomonitor.com",
                        "95.81.146.2",
                        "www.sermadridsur.com",                        
                        "78.129.159.205"};
         
char urlString [][70]={"/MountainFM",
                        "/NECR",
                        "/franceinter/all/franceinter-32k.mp3",
                        "/",
                        "/"};
int portValue[]={80,80,80,8002,8005};

char ndefServer[80]="";
char ndefPath[80]="";
int ndefPortNumber=0;

void setup() {
  
  pinMode (53, OUTPUT);
  Serial.begin(115200);
  
  
  //boot up the MP3 Player Shield
  int timer= millis();
  Serial.println("pre init MP3");  
  result = MP3player.begin();
  //check result, see readme for error codes.
  if(result != 0) {
    Serial.print("Error code: ");
    Serial.print(result);
    Serial.println(" when trying to start MP3 player");
    while (true);
  } else {
    Serial.println("MP3 init ok");
    while (millis()-timer <500) ;
  }
  
  timer= millis();
  Serial.println("pre init NFC");  
  nfc.begin();
  nfc.RFConfiguration(0x01); // default is 0xFF (try forever; ultimately it does time out but after a long while
                             // modifies NFC library to set up a timeout while searching for RFID tags
                             // retry 0x01 times
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    // stop
    while (true);
  }    
  // ok, print received data!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  Serial.print("Supports "); Serial.println(versiondata & 0xFF, HEX);
  // configure board to read RFID tags and cards
  nfc.SAMConfig();
  Serial.println("post init NFC");  
  while (millis()-timer <500) ;
  

 timer= millis();
  Serial.println("pre begin mac");
  if (!Ethernet.begin(mac)) {
    Serial.println("Error initialising Ethernet link");    
    while (true);    
  }
  Serial.println("post begin mac");
  // EtherDeselect();
  while (millis()-timer <500) ;
    
  currentStation=0;
  restartConnection(currentStation);
  MP3player.SetVolume(25,25);
  Serial.println("And now streaming begins");  
}

void loop() {
  if (initialLoop==1) { 
    noReceptionCounter=0;
    nfcTime = millis();
    if (client.connect((char *) ndefServer, ndefPortNumber)) {
        client.print("GET "); client.print(ndefPath); client.println(" HTTP/1.1");
        client.println("Connection: Keep-Alive");
        client.print("HOST: "); client.println(ndefServer);
        client.println();  
        Serial.println(" - Connected");
        initialLoop=2;    
    } else {
        Serial.println("Error: not connected");
        reconnectCounter++;
        if (reconnectCounter>=3) {
          Serial.println("Selecting Next Station");
          nextStation();
          restartConnection(currentStation);
          return;
        }
        delay(5000);
        return;
    }

  }
  

  //MP3player.pauseDataStream();
  if ((millis() - nfcTime) >1000){
    nfcTime=millis();
    digitalWrite(NFC_SS, LOW);//SPI select RFID reader
    tagId = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A);
    digitalWrite(NFC_SS, HIGH);//SPI deselect RFID reader
    if (tagId != 0) 
    {         
      Serial.print("NDEF URL: ");
      Serial.println(nfc.getNdefUrl());
      int result= nfc.parseUrl((char *)ndefServer, (char *)ndefPath);
      if (result!=0) {
          ndefPortNumber = result;
          Serial.print("Port number: ");
          Serial.println(ndefPortNumber);
          Serial.print("Server: ");
          Serial.println(ndefServer);
          Serial.print("Path: ");
          Serial.println(ndefPath);
          restartConnection();
          return;
      }
      
      Serial.print("Read card #"); Serial.println(tagId);
      if (tagId==61987245) currentStation=3;
      else {
        if (tagId==208592448) currentStation=4;  //2085924408
        else { 
            nextStation();
        }
      }
      restartConnection(currentStation);
      return;
    }
  }
  
 unsigned long startTime = millis();
  
  totalBytes=0;
  while  (((millis() - startTime ) < 100)) {
    if (client.available() ) {
      totalBytes=client.read((uint8_t *)streamData, 32);
      break;
    }
  }
  if (totalBytes==0) {
    Serial.print(".");  
    noReceptionCounter++;
    if (noReceptionCounter == 100) {
      restartConnection();
    }
    return;
  } else {
    noReceptionCounter=0;
    if (totalBytes!=32) Serial.print("+"); 
    totalReadData+=totalBytes;
    if (totalReadData>10000) {
      Serial.print("*"); 
      totalReadData=0;
    }
  }    
  MP3player.transferStream(totalBytes,streamData);
}




void restartConnection(){
    initialLoop=1;
    reconnectCounter=0;
    totalReadData=0;
    client.flush();
    client.stop(); 
    Serial.print("New Station: "); Serial.println(ndefServer);
}

void restartConnection(int newStation){
    ndefPortNumber = portValue[newStation];
    sscanf(serverString[newStation],"%s",ndefServer);
    sscanf(urlString[newStation],"%s",ndefPath);
    restartConnection();
}

void nextStation() {
    currentStation++;
     if (((currentStation)*sizeof(int))>=sizeof(portValue)) currentStation=0; 
}



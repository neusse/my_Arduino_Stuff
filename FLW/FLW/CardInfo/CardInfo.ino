/*
  SD card test

  This example shows how use the utility libraries on which the'
  SD library is based in order to get info about your SD card.
  Very useful for testing a card when you're not sure whether its working or not.

  The circuit:
    SD card attached to SPI bus as follows:
 ** MOSI - pin 11 on Arduino Uno/Duemilanove/Diecimila
 ** MISO - pin 12 on Arduino Uno/Duemilanove/Diecimila
 ** CLK - pin 13 on Arduino Uno/Duemilanove/Diecimila
 ** CS - depends on your SD card shield or module.
 		Pin 4 used here for consistency with other Arduino examples


  created  28 Mar 2011
  by Limor Fried
  modified 9 Apr 2012
  by Tom Igoe
*/
// include the SD library:
#include <SPI.h>
#include <SD.h>

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;

// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
// MKRZero SD: SDCARD_SS_PIN

const int chipSelect = 10;
const int myMISO = 50;
const int myMOSI = 51;
const int mySCK = 52;
const int mySS = 53;  // slave select  SS

//SPI: Pins 50(MISO), 51(MOSI), 52(SCK), and 53(SS).
// To communicate with the periphery through the SPI interface. 
// use the SPI library.
// TWI/I²C: Pins 20(SDA) and 21(SCL)
// UART: pins 0(RX) and 1(TX), 19(RX1) and 18(TX1), 17(RX2) and 16(TX2), 15(RX3) and 14(TX3).
// PWM: Pins 2-13 and 44-46
// ADC: Pins A0-A16
// Digital I/O: Pins 0-53


// An SS pin does not need programming as such apart from making it an 
// output and setting it high. When you want to talk to a particular SPI device
// you set it's SS pin low, do the SPI transfers and then set the SS pin high
// again when you finished talking to the SPI device.

//#SPI: Pins 50(MISO), 51(MOSI), 52(SCK), and 53(SS).

//pinMode(chipSelect,  OUTPUT);

//pinMode(myMISO,      INPUT_PULLUP);
//pinMode(myMOSI,      OUTPUT); //pullup the MOSI pin on the SD card
//pinMode(mySCK,       OUTPUT);
//pinMode(MySS,        OUTPUT);
 
//digitalWrite(chipSelect, HIGH);
//digitalWrite(mySS,       HIGH);
//digitalWrite(myMOSI,     HIGH); //pullup the MOSI pin on the SD card

//digitalWrite(mySCK,      LOW); //pull 



void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }


  pinMode(chipSelect,  OUTPUT);
//pinMode(myMISO,      INPUT_PULLUP);
//pinMode(myMOSI,      OUTPUT); //pullup the MOSI pin on the SD card
//pinMode(mySCK,       OUTPUT);
//pinMode(MySS,        OUTPUT);


 
digitalWrite(chipSelect, LOW);
//digitalWrite(mySS,       HIGH);
//digitalWrite(myMOSI,     HIGH); //pullup the MOSI pin on the SD card

//digitalWrite(mySCK,      LOW); //pull 


  if( !SD.begin(chipSelect)) {
    Serial.println("Card Faild!=======");
    //return;  
  }

  Serial.println("My Card talked!!!!!");
  

//  pinMode(chipSelect,OUTPUT);
//  digitalWrite(chipSelect, HIGH);
//pinMode(11, OUTPUT);
//digitalWrite(11, HIGH); //pullup the MOSI pin on the SD card
//pinMode(12,INPUT_PULLUP);//pullup the MISO pin on the SD card
//pinMode(13, OUTPUT);
//digitalWrite(13, LOW); //pull 
//  Serial.print("\nInitializing SD card...");

  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!card.init(SPI_HALF_SPEED, chipSelect)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card inserted?");
    Serial.println("* is your wiring correct?");
    Serial.println("* did you change the chipSelect pin to match your shield or module?");
    while (1);
  } else {
    Serial.println("Wiring is correct and a card is present.");
  }

  // print the type of card
  Serial.println();
  Serial.print("Card type:         ");
  switch (card.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }

  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    while (1);
  }

  Serial.print("Clusters:          ");
  Serial.println(volume.clusterCount());
  Serial.print("Blocks x Cluster:  ");
  Serial.println(volume.blocksPerCluster());

  Serial.print("Total Blocks:      ");
  Serial.println(volume.blocksPerCluster() * volume.clusterCount());
  Serial.println();

  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  Serial.print("Volume type is:    FAT");
  Serial.println(volume.fatType(), DEC);

  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  volumesize /= 2;                           // SD card blocks are always 512 bytes (2 blocks are 1KB)
  Serial.print("Volume size (Kb):  ");
  Serial.println(volumesize);
  Serial.print("Volume size (Mb):  ");
  volumesize /= 1024;
  Serial.println(volumesize);
  Serial.print("Volume size (Gb):  ");
  Serial.println((float)volumesize / 1024.0);

  Serial.println("\nFiles found on the card (name, date and size in bytes): ");
  root.openRoot(volume);

  // list all files in the card with date and size
  root.ls(LS_R | LS_DATE | LS_SIZE);
}

void loop(void) {
}

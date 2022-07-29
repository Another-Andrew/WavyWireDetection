/*

  This is a WIP data-logging program for an Arduino Mega 2560, connected to a generic microSD card reader/writer and an ADS1220.
  It is meant to be used for collecting vibrational data on draw benches.

  //  ADS1220 Breakout Board pin-out
  //
  //  |ADS1220 pin label| Pin Function         |Arduino Connection|
  //  |-----------------|:--------------------:|-----------------:|
  //  | DRDY            | Data ready Output pin|  D2              |
  //  | MISO            | Slave Out            |  D50             |
  //  | MOSI            | Slave In             |  D51             |
  //  | SCLK            | Serial Clock         |  D52             |
  //  | CS              | Chip Select          |  D49             |
  //  | DVDD            | Digital VDD          |  +5V             |
  //  | DGND            | Digital Gnd          |  Gnd             |
  //  | AN0-AN3         | Analog Input         |  -               |
  //  | AVDD            | Analog VDD           |  -               |
  //  | AGND            | Analog Gnd           |  -               |

  //  |SD R/W  pin label| Pin Function         |Arduino Connection|
  //  |-----------------|:--------------------:|-----------------:|
  //  | MISO            | Slave Out            |  D50             |
  //  | MOSI            | Slave In             |  D51             |
  //  | SCLK            | Serial Clock         |  D52             |
  //  | CS              | Chip Select          |  D48             |
  //  | VCC             | Power Rail           |  +5V             |
  //  | GND             | Power Gnd            |  Gnd             |

*/

#include "Protocentral_ADS1220.h"
#include <SPI.h>
#include <SD.h>

// SD card objects
Sd2Card card;
SdVolume volume;
SdFile root;
File dataLog;

// adc alias 
#define pc_ads1220 adc

// Chip select variables for switching SPI devices
#define SD_CS_PIN    48  // Any digital out pin
#define ADS1220_CS_PIN    49  // Any digital out pin
#define ADS1220_DRDY_PIN  2   // Must be an interupt pin

// ADC parameters
#define PGA 1                 // Programmable Gain = 1
#define VREF 2.048            // Internal reference of 2.048V
#define VFSR VREF/PGA
#define FSR (((long int)1<<23)-1)

#define sampleSize 100
#define fileName "datalog.csv"

File logFile;                      // Generic file object for writing to microSD card
// char fileName[] = "datalog.csv";  // Filename for csv file

// ADC objects
Protocentral_ADS1220 pc_ads1220;
int32_t adc_data;
volatile bool drdyIntrFlag = false;

// Helper functions for ADC
void drdyInterruptHndlr(){
    drdyIntrFlag = true;
}
void enableInterruptPin() {
    attachInterrupt(digitalPinToInterrupt(ADS1220_DRDY_PIN), drdyInterruptHndlr, FALLING);
}

// Sets the chip select pin to high. This disables the modules
void disableModules() {
    // digitalWrite(ADS1220_CS_PIN, HIGH);
    digitalWrite(SD_CS_PIN, HIGH); // Disable SD card r/w SPI bus
}

// Setup function that runs once when the microcontroller is first powered on.
void setup() {
    disableModules();  
    // Open serial interface with a BAUD rate of 57600
    Serial.begin(57600);
    writeSD(recordData());
}

void loop() {}

float *recordData() {
    // Array to store all the samples. Has to be static because a pointer will be passed between functions to access this array. 
    static float samples[sampleSize];

    // Initialize connection with adc
    pc_ads1220.begin(ADS1220_CS_PIN, ADS1220_DRDY_PIN);  // Begin SPI communication with adc.
    pc_ads1220.set_data_rate(DR_330SPS);  // Sets sampling rate. Valid options are listed at the bottom. 
    pc_ads1220.set_pga_gain(PGA_GAIN_32); // Sets the gain of the built-in lna.
    pc_ads1220.select_mux_channels(MUX_AIN0_AIN1);  // Configure for differential measurement between AIN0 and AIN1
    pc_ads1220.Start_Conv();  // Start continuous conversion mode
    enableInterruptPin();

    // For sampleSize. Loop, but don't increment i unless a successful sample has been read. 
    for (int i = 0; i < sampleSize;) {
        // Once Data-Ready flag is true
        if (drdyIntrFlag) {
            drdyIntrFlag = false;
            adc_data = pc_ads1220.Read_Data_Samples();
            samples[i] = (float) ((adc_data*VFSR*1000)/FSR);     //In  mV
            i++;
        }
    }
  
    // Returns a pointer to the samples array. 
    return samples;
}

// Writes data to the SD card. Takes pointer to an array of samples. 
int writeSD(float *samples) {
  
    // Shutdown the ADC to address the SD card reader through SPI interface.
    pc_ads1220.SPI_Command(2);
    digitalWrite(ADS1220_CS_PIN, HIGH);
    digitalWrite(SD_CS_PIN, LOW);

    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("initialization failed!");
        return 1;
    }

    // Constructs single string, separated by commas, out of array of floats.
    String dataString = "";
    for (int i = 0; i < sampleSize; i++) {
        dataString += String(samples[i]);
        if (i != (sampleSize-1)) {
            dataString += ",";
        }
    }

    // If file doesn't exist, create it. Then print data to text file. 
    Serial.println("initialization done.");
    if (SD.exists(fileName)) {
        Serial.println("File already exists, overwritting data.");

    } else {

    Serial.println("File does not exist, making it.");
        logFile = SD.open(fileName, FILE_WRITE);
        logFile.close();
    }

    logFile = SD.open(fileName, FILE_WRITE);
    delay(200);
    if (logFile.println(dataString) >= 1) {
        Serial.println("Successfully wrote to SD card. ");
    } else {
        Serial.println("No data was written.");
    }

    delay(100);
    logFile.close();
    // Serial.println(dataString);
    Serial.println("All done!");
    while(1);
  
}

int largestValue(float *data) {
    for (int i=1; i<(sizeof(data)/sizeof(float)); ) {
        if (data[i] > data[i-1]) {
            
        }

    }
}

  // Enable ADC
  // Serial.println("Initializing ADC...");
  // Check if you can read a configuration register. If the register exists, then the program assumes the module is functional.
  // if (!pc_ads1220.get_config_reg()) {
  //   Serial.println("ADC initialization failed.");
  //   while (1);
  // } else {
  //   Serial.println("ADC initialized and ready");
  // }

  // Shutdown adc
  // pc_ads1220.SPI_Command(2);
  // Initializes SD card
  // Serial.println("\nInitializing SD card...");
//  if (!SD.begin(SD_CS_PIN)) {
//    Serial.println("SD initialization failed.");
//    while (1);
//  } else {
//    Serial.println("SD card reader/writer wiring is correct and a card is present.");
//  }
  // digitalWrite(SD_CS_PIN, HIGH);

/*

================================================Other Notes================================================

Library Config Registers:
    m_config_reg0 = 0x00;   //Default settings: AINP=AIN0, AINN=AIN1, Gain 1, PGA enabled
    m_config_reg1 = 0x04;   //Default settings: DR=20 SPS, Mode=Normal, Conv mode=continuous, Temp Sensor disabled, Current Source off
    m_config_reg2 = 0x10;   //Default settings: Vref internal, 50/60Hz rejection, power open, IDAC off
    m_config_reg3 = 0x00;   //Default settings: IDAC1 disabled, IDAC2 disabled, DRDY pin only

Sampling Mode Registers:
  #define DR_20SPS    0x00
  #define DR_45SPS    0x20
  #define DR_90SPS    0x40
  #define DR_175SPS   0x60
  #define DR_330SPS   0x80
  #define DR_600SPS   0xA0
  #define DR_1000SPS  0xC0

Gain Mode Registers: 
  #define PGA_GAIN_1   0x00
  #define PGA_GAIN_2   0x02
  #define PGA_GAIN_4   0x04
  #define PGA_GAIN_8   0x06
  #define PGA_GAIN_16  0x08
  #define PGA_GAIN_32  0x0A
  #define PGA_GAIN_64  0x0C
  #define PGA_GAIN_128 0x0E

================================================Control Logic Loop================================================

0. Start loop
1. Pull ADC-CS pin down to enable the module.
2. Read 500 samples into an array.
3. Pull ADC-CS pin up to disable the module. 
4. Pull SD-CS pin down to enable the module.
5. Open new file log.csv
6. Write data in array to csv and clear array
7. Close file and pull SD-CS pin up to disable the module. 
8. Check if desired samples has been reached (Whole logic loop in while loop?)

*/

/*
Include path
    C:\Program Files (x86)\Arduino\hardware\arduino\avr\cores\arduino
    C:\Program Files (x86)\Arduino\hardware\arduino\avr\variants\mega
    C:\Users\Andrew\Documents\Arduino\libraries\ProtoCentral_ADS1220_24-bit_ADC_Library\src
    C:\Program Files (x86)\Arduino\hardware\arduino\avr\libraries\SPI\src
    C:\Program Files (x86)\Arduino\libraries\SD\src
    c:\program files (x86)\arduino\hardware\tools\avr\lib\gcc\avr\7.3.0\include
    c:\program files (x86)\arduino\hardware\tools\avr\lib\gcc\avr\7.3.0\include-fixed
    c:\program files (x86)\arduino\hardware\tools\avr\avr\include
*/


/***************************************************************************
  This is a library for the BME280 humidity, temperature & pressure sensor

  Designed specifically to work with the Adafruit BME280 Breakout
  ----> http://www.adafruit.com/products/2650

  These sensors use I2C or SPI to communicate, 2 or 4 pins are required
  to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/
#include <LowPower.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Time.h>  

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 9

#define SEALEVELPRESSURE_HPA (1013.25)

//Adafruit_BME280 bme; // I2C
Adafruit_BME280 bme(BME_CS); // hardware SPI
//Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO,  BME_SCK);

//
// Time stuff
//

// single character message tags
#define TIME_HEADER   'T'   // Header tag for serial time sync message
#define FORMAT_HEADER 'F'   // Header tag indicating a date format message
#define FORMAT_SHORT  's'   // short month and day strings
#define FORMAT_LONG   'l'   // (lower case l) long month and day strings

#define TIME_REQUEST  7     // ASCII bell character requests a time sync message 

static boolean isLongFormat = true;


// SD card SPI selector
const int chipSelect = 4;

void setup() {
  Serial.begin(9600);
  Serial.println(F("Datalogging: BME280"));

  if (!bme.begin()) {
    Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
    while (1);
  }

  
  Serial.print(F("Initializing SD card..."));

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println(F("Card failed, or not present"));
    // don't do anything more:
    return;
  }
  Serial.println(F("card initialized."));

  // On Linux, you can use "date +T%s\n > /dev/ttyACM0" (UTC time zone)
  setSyncProvider( requestSync);  //set function to call when sync required
  Serial.println(F("Waiting for sync message, do date +T%s\n"));
}

void loop() {
  while (timeStatus() == timeNotSet){
      delay(1000);
      if (Serial.available() > 1) { // wait for at least two characters
        char c = Serial.read();
        //Serial.print(c);
        if( c == TIME_HEADER) {
          processSyncMessage();
        }
        else if( c== FORMAT_HEADER) {
          processFormatMessage();
        }
      }    
    // Winter time, 1 hour ahead.
    adjustTime(60*60);
    }
    delay(1000);
    // make a string for assembling the data to log:
    String dataString = "";
    
    dataString += getDigits(hour()); dataString += ":";
    dataString += getDigits(minute());dataString += ":";
    dataString += getDigits(second());dataString += " ";

    dataString += dayShortStr(weekday()); dataString += " ";
    dataString += day(); dataString += " ";
    dataString += monthShortStr(month()); dataString += " ";
    dataString += year(); dataString += " ";
    
    dataString += F("Temp=");
    dataString += bme.readTemperature();
    dataString +=" *C";

    dataString += F(" ApproxAlt=");
    dataString += bme.readAltitude(SEALEVELPRESSURE_HPA);
    dataString += F(" m");

    dataString += F(" Hum=");
    dataString += bme.readHumidity();
    dataString += F(" %");

    dataString += F(" Pres=");
    dataString += (bme.readPressure() / 100.0F);
    dataString += " hPa";

    File dataFile = SD.open("datalog.txt", FILE_WRITE);
    
    // if the file is available, write to it:
    if (dataFile) {
      dataFile.println(dataString);
      dataFile.close();
      // print to the serial port too:
      Serial.println(dataString);
    }
    // if the file isn't open, pop up an error:
    else {
      Serial.println(F("error opening datalog.txt"));
    }
    delay(1000);
    int m = 15;
    for(int i=0; i<m; i++){
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  
    }
    adjustTime((8*m));
}

String getDigits(int digits){
  if(digits < 10){
      return "0" + (String)digits;
  }else{
    return (String)digits;
    }
  
  }
void digitalClockDisplay() {
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  if(isLongFormat)
    Serial.print(dayStr(weekday()));
  else  
   Serial.print(dayShortStr(weekday()));
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  if(isLongFormat)
     Serial.print(monthStr(month()));
  else
     Serial.print(monthShortStr(month()));
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
}

void printDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void  processFormatMessage() {
   char c = Serial.read();
   if( c == FORMAT_LONG){
      isLongFormat = true;
      Serial.println(F("Setting long format"));
   }
   else if( c == FORMAT_SHORT) {
      isLongFormat = false;   
      Serial.println(F("Setting short format"));
   }
}

void processSyncMessage() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013 - paul, perhaps we define in time.h?

   pctime = Serial.parseInt();   
   if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
     setTime(pctime); // Sync Arduino clock to the time received on the serial port
   }
}

time_t requestSync() {
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}

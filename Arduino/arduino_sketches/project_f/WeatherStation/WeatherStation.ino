
/*************************************************** 
Watchdog timer
 ****************************************************/

#include <avr/wdt.h>

/*************************************************** 
Adafruit Sensor 
 ****************************************************/
 

#include <Wire.h>
// #include <SPI.h> // to show the need for this by the sensor
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11 
#define BME_CS 9

//Adafruit_BME280 bme; // I2C
Adafruit_BME280 bme(BME_CS); // hardware SPI
//Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO,  BME_SCK);


/*************************************************** 
Adafruit Wifi 3000
 ****************************************************/
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
//#include "utility/debug.h"

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed

#define WLAN_SSID       "Lura_1"           // cannot be longer than 32 characters!
#define WLAN_PASS       "12345678"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define IDLE_TIMEOUT_MS  3000      // Amount of time to wait (in milliseconds) with no data 
                                   // received before closing the connection.  If you know the server
                                   // you're accessing is quick to respond, you can reduce this value.

// What page to grab!
#define WEBSITE      "einbox.net"


String g_webpage = "/tools/iot.pl?key=heisann&input1=";

/*************************************************** 

  Start of sketch

 ****************************************************/

uint32_t g_ip;

void setup(void)
{
  wdt_disable();
  
  Serial.begin(115200);
  Serial.println(F("Init"));

  if (!bme.begin()) {  
    Serial.println(F("!bme"));
    while (1);
  }

  Serial.println(F("cc3k!"));   
  /* Initialise the module */
  
  if (!cc3000.begin())
  {
    Serial.println(F("!cc3k"));
    while(1);
  }
  //Serial.println("cc3k: reboot");
  
  delay(1000);
  connectToWifi();

  getDHCP();

  g_ip = getIpOfWebsite();
  
  wdt_enable(WDTO_8S);
  
}


void loop(void)
{
    if(cc3000.checkConnected()){
      
      
      
      String sensors;
      sensors += g_webpage;
      sensors += F("Temp_c=");
      sensors += (String)bme.readTemperature();    
      
      sensors += F("%20Psur_Pa=");
      sensors += bme.readPressure();    
      
      sensors += F("%20Hmdty_%=");
      sensors += bme.readHumidity();
  
      //Serial.print("F: "); Serial.println(getFreeRam(), DEC);
      connectToWebsite(&g_ip, &sensors);
      wdt_reset();
      
      delay(4000);
      wdt_reset();
      delay(4000);
      wdt_reset();
      delay(4000);
      wdt_reset();
    }


    


 

}



void connectToWifi(){
  // Optional SSID scan
  // listSSIDResults();
  
  Serial.print(F("?: ")); Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F(":("));
    while(1);
  }
   
  Serial.println(F(":D"));
}

void getDHCP(){
    /* Wait for DHCP to complete */
  Serial.println(F("ReqDHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(1000); // ToDo: Insert a DHCP timeout!
    Serial.print(F("."));
  }    
}

uint32_t getIpOfWebsite(){
  uint32_t ip = 0;
  // Try looking up the website's IP address
  Serial.print(WEBSITE); Serial.print(F(" -> "));
  while (ip == 0) {
    if (! cc3000.getHostByName(WEBSITE, &ip)) {
      Serial.println(F("NoRslv!"));
    }
    delay(1000);
  }

  cc3000.printIPdotsRev(ip);
  return ip;  
}

/*
int doPingOfIp(uint32_t ip){
    
  // Optional: Do a ping test on the website
  
  Serial.print(F("\n\rP ")); cc3000.printIPdotsRev(ip); Serial.print("...");  
  int replies = cc3000.ping(ip, 5);
  Serial.print(replies); Serial.println(F(" rly"));
  return replies;
}
*/

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

bool connectToWebsite(uint32_t *ip, String *request){
    // Transform to char
  char requestBuf[request->length()+1];
  request->toCharArray(requestBuf,request->length()); 
  
  /* Try connecting to the website.
     Note: HTTP/1.1 protocol is used to keep the server from closing the connection before all data is read.
  */
  boolean isConn;
  Adafruit_CC3000_Client www = cc3000.connectTCP(*ip, 80);
  if (www.connected()) {
    Serial.println(requestBuf);
    www.fastrprint(F("GET "));
    www.fastrprint(requestBuf);
    www.fastrprint(F(" HTTP/1.1\r\n"));
    www.fastrprint(F("Host: ")); www.fastrprint(WEBSITE); www.fastrprint(F("\r\n"));
    www.fastrprint(F("\r\n"));
    www.println();
    isConn = true;
  } else {
    Serial.println(F("!con:("));    
    return false;
  }

  Serial.println(F("-------------------------------------"));
  
  /* Read data until either the connection is closed, or the idle timeout is reached. */ 
  unsigned long lastRead = millis();
  while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (www.available()) {
      char c = www.read();
      Serial.print(c);
      lastRead = millis();
    }
  }
  www.close();
  Serial.println(F("-------------------------------------"));

  Serial.print(F("RF:"));
  Serial.println(freeRam());
  return isConn;
}


void disconnectWifi(){
    
  /* You need to make sure to clean up after yourself or the CC3000 can freak out */
  /* the next time your try to connect ... */
  Serial.println(F("Disc"));
  cc3000.disconnect();
  
}



/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details


bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("No ip!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP : ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nnm: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\ngw: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\n: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\n: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}
*/
/**************************************************************************/

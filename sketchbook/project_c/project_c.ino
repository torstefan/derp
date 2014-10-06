#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>
#include "pitches.h"


#define buttonPin      2
                    // 3 is reserved for bubble teller, pin 2 and 3 can support interupt.
#define buttonLeft     4 
#define buttonRight    5
#define melodyPin      6
#define resetDispPin   7
const int softwareTx = 8;
#define ONE_WIRE_BUS   9
const int softwareRx = 10; // Not used, needed for library
const int led        = 12;


#define selectorPin A0

// Data wire is plugged into port 9 on the Arduino

// Set the temperature probe to 12-bit resolution.
#define TEMPERATURE_PRECISION 12

volatile int buttonPressed = HIGH; // Button state
int buttonLastState = HIGH;

boolean subMenuSelected = false;


// These are the Arduino pins required to create a software seiral
//  instance. We'll actually only use the TX pin.
// Serial 7-Segment dispaly pin



SoftwareSerial s7s(softwareRx, softwareTx);

int selector = 0; // Analog value from 0-1024
int mod_0 = 0; // Modulus for the selector, to get temprange between 0 - 100
int mod_1 = 0; // Modulus for the selector, to get temprange between 0 - 100
volatile int t_0 = 0;
volatile int t_1 = 0;

int subMenuExitTimer = 0; // Used in finding delta time of leftbutton press idicating submenu exit.


unsigned int counter = 0;  // This variable will count up to 65k
char tempString[10];  // Will be used with sprintf to create strings

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// void buzz(int targetPin, long frequency, long length) {
void setup(void)
{
  pinMode(buttonPin, INPUT_PULLUP); // Enable internal pull-up resistor on pin 2
  pinMode(buttonLeft, INPUT_PULLUP); // Enable internal pull-up resistor on pin 3
  pinMode(buttonRight, INPUT_PULLUP); // Enable internal pull-up resistor on pin 4
  pinMode(resetDispPin, OUTPUT);

  
  // Add interupt handler for button.
  attachInterrupt(0, changeState, FALLING);
  
  pinMode(melodyPin, OUTPUT);      //buzzer
  buzz(melodyPin, NOTE_B5, 1000/10 );
  delay(50);
  buzz(melodyPin, NOTE_C6, 1000/10 );

  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);     
  
  // Set the selector
  
  selector = analogRead(selectorPin);
  mod_0 = selector % 100;
  mod_1 = selector % 100;
  
  // start serial port
  Serial.begin(9600);
  Serial.println();
  Serial.println("DERP Engineering - Project C - Brewing Aid");
  Serial.println();
  
  // Start up the library
  sensors.begin();
  sensors.setResolution(TEMPERATURE_PRECISION);  

  // Reset the Display
  resetDisplay();
  
  // Must begin s7s software serial at the correct baud rate.
  //  The default of the s7s is 9600.
  s7s.begin(9600);

//  showWelcome();
  // Clear the display before jumping into loop
  clearDisplay();  
  buzz(melodyPin, NOTE_C6, 1000/10 );
}

void loop(void)
{ 
   
  if(mainMenuSelected()){
    menu();    
  }else{
    getTemp();
  
  }
  delay(100); // Also has 100 ms in BlinkLed , 200 ms total
}

boolean mainMenuSelected(){
  
    return buttonLastState != buttonPressed;

}



void menu(){
  Serial.println("Menu mode");
  selector = analogRead(selectorPin);
  String text;

  int range = map(selector, 0, 1023, 3,0);
  switch (range){
  
     case 0:
       programBrewCycles(range);
       break;
       
     case 1:
       text = "B   ";
       writeText(text);
       break;     
       
     case 2:
       text = "C   ";
       writeText(text);
       break;
       
     case 3:
       text = "D   ";
       writeText(text);
       resetDisplay();
       break;

  
  }

}

void programBrewCycles(int menuSelected){
  
  if(menuSelected == 0){
    String text;
    text = "A   ";
    writeText(text);
    
    if(mainMenuSelected() & digitalRead(buttonLeft) == LOW){      
      if(exitSubMenu()){
         subMenuSelected = false;      
      }else{      
         subMenuSelected = true;
      }
    }
    
    if(subMenuSelected){
        text = "A  1";
        writeText(text);
        sing(2);      
      }
      

  }
}

boolean exitSubMenu(){
  
  if(digitalRead(buttonLeft) == LOW){
    if(millis() - subMenuExitTimer > 1000){
      return true;
    }
  }else{
    subMenuExitTimer = millis();
    return false;
  }
}

void writeText(String text){
  Serial.println(text);
  char t[5]; 
  text.toCharArray(t,5);
  sprintf(tempString, "%4s", t);
  s7s.print(tempString);
  setDecimals(0b00000000);
  delay(100);

}

void resetDisplay(){
  
    digitalWrite(resetDispPin, LOW);
    delay(300);
    digitalWrite(resetDispPin, HIGH);
    
  
}

void setTempAlarm(){

   selector = analogRead(selectorPin);
   if(selector > 1000){
     selector = 999;
   }
   
   mod_1 = (selector/10)% 100;
   if((mod_1-1) > mod_0 || (mod_1+1) < mod_0 ){
     sprintf(tempString, "%4d", mod_1);
     s7s.print(tempString);
     setDecimals(0b00000000);  // Sets digit 3 decimal off
     mod_0 = mod_1;
     delay(400);
   }
}

void changeState(){ 
  t_1 = millis();
  if((t_1 - t_0) > 100){
    t_0 = t_1;
    buttonPressed = !buttonPressed; 
    buzz(melodyPin, NOTE_C6, 1000/10 );
  }
}

void showWelcome(){
  // Clear the display, and then turn on all segments and decimals
  clearDisplay();  // Clears display, resets cursor  
  s7s.print("-HI-");  // Displays -HI- on all digits
  setDecimals(0b111111);  // Turn on all decimals, colon, apos

  // Flash brightness values at the beginning
  setBrightness(0);  // Lowest brightness
  delay(1500);
  setBrightness(127);  // Medium brightness
  delay(1500);
  setBrightness(255);  // High brightness
  delay(1500);

}

void getTemp(){ 
  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  
  Serial.print("Temperature for the device 1 (index 0) is: ");
  sensors.setResolution(TEMPERATURE_PRECISION); 
  float t = sensors.getTempCByIndex(0);
  int temp = t * 10;
  Serial.println(t);  
  blinkLed(led);
  //buzz(melodyPin, NOTE_E2, 1000/6 );


  setTempAlarm();


  if(t > -50){

     // Magical sprintf creates a string for us to send to the s7s.
    //  The %4d option creates a 4-digit integer.
    sprintf(tempString, "%4d", temp);
  
    // This will output the tempString to the S7S
    s7s.print(tempString);
    setDecimals(0b00000100);  // Sets digit 3 decimal on

  }else{
    s7s.print("-NA-"); 
    delay(1000);
    clearDisplay(); 
  }
}



void blinkLed(int led){
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(100);               // wait for a second
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW

}
// Send the clear display command (0x76)
//  This will clear the display and reset the cursor
void clearDisplay()
{
  s7s.write(0x76);  // Clear display command
}

// Set the displays brightness. Should receive byte with the value
//  to set the brightness to
//  dimmest------------->brightest
//     0--------127--------255
void setBrightness(byte value)
{
  s7s.write(0x7A);  // Set brightness command byte
  s7s.write(value);  // brightness data byte
}

// Turn on any, none, or all of the decimals.
//  The six lowest bits in the decimals parameter sets a decimal 
//  (or colon, or apostrophe) on or off. A 1 indicates on, 0 off.
//  [MSB] (X)(X)(Apos)(Colon)(Digit 4)(Digit 3)(Digit2)(Digit1)
void setDecimals(byte decimals)
{
  s7s.write(0x77);
  s7s.write(decimals);
}


//Mario main theme melody
int melody[] = {
  NOTE_E7, NOTE_E7, 0, NOTE_E7, 
  0, NOTE_C7, NOTE_E7, 0,
  NOTE_G7, 0, 0,  0,
  NOTE_G6, 0, 0, 0, 

  NOTE_C7, 0, 0, NOTE_G6, 
  0, 0, NOTE_E6, 0, 
  0, NOTE_A6, 0, NOTE_B6, 
  0, NOTE_AS6, NOTE_A6, 0, 

  NOTE_G6, NOTE_E7, NOTE_G7, 
  NOTE_A7, 0, NOTE_F7, NOTE_G7, 
  0, NOTE_E7, 0,NOTE_C7, 
  NOTE_D7, NOTE_B6, 0, 0,

  NOTE_C7, 0, 0, NOTE_G6, 
  0, 0, NOTE_E6, 0, 
  0, NOTE_A6, 0, NOTE_B6, 
  0, NOTE_AS6, NOTE_A6, 0, 

  NOTE_G6, NOTE_E7, NOTE_G7, 
  NOTE_A7, 0, NOTE_F7, NOTE_G7, 
  0, NOTE_E7, 0,NOTE_C7, 
  NOTE_D7, NOTE_B6, 0, 0
};
//Mario main them tempo
int tempo[] = {
  12, 12, 12, 12, 
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12, 

  12, 12, 12, 12,
  12, 12, 12, 12, 
  12, 12, 12, 12, 
  12, 12, 12, 12, 

  9, 9, 9,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,

  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,

  9, 9, 9,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
};

//

//Underworld melody
int underworld_melody[] = {
  NOTE_C4, NOTE_C5, NOTE_A3, NOTE_A4, 
  NOTE_AS3, NOTE_AS4, 0,
  0,
  NOTE_C4, NOTE_C5, NOTE_A3, NOTE_A4, 
  NOTE_AS3, NOTE_AS4, 0,
  0,
  NOTE_F3, NOTE_F4, NOTE_D3, NOTE_D4,
  NOTE_DS3, NOTE_DS4, 0,
  0,
  NOTE_F3, NOTE_F4, NOTE_D3, NOTE_D4,
  NOTE_DS3, NOTE_DS4, 0,
  0, NOTE_DS4, NOTE_CS4, NOTE_D4,
  NOTE_CS4, NOTE_DS4, 
  NOTE_DS4, NOTE_GS3,
  NOTE_G3, NOTE_CS4,
  NOTE_C4, NOTE_FS4,NOTE_F4, NOTE_E3, NOTE_AS4, NOTE_A4,
  NOTE_GS4, NOTE_DS4, NOTE_B3,
  NOTE_AS3, NOTE_A3, NOTE_GS3,
  0, 0, 0
};
//Underwolrd tempo
int underworld_tempo[] = {
  12, 12, 12, 12, 
  12, 12, 6,
  3,
  12, 12, 12, 12, 
  12, 12, 6,
  3,
  12, 12, 12, 12, 
  12, 12, 6,
  3,
  12, 12, 12, 12, 
  12, 12, 6,
  6, 18, 18, 18,
  6, 6,
  6, 6,
  6, 6,
  18, 18, 18,18, 18, 18,
  10, 10, 10,
  10, 10, 10,
  3, 3, 3
};

int song = 0;

void sing(int s){      
   // iterate over the notes of the melody:
   song = s;
   if(song==2){
     Serial.println(" 'Underworld Theme'");
     int size = sizeof(underworld_melody) / sizeof(int);
     for (int thisNote = 0; thisNote < size; thisNote++) {

       // to calculate the note duration, take one second
       // divided by the note type.
       //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
       int noteDuration = 1000/underworld_tempo[thisNote];

       buzz(melodyPin, underworld_melody[thisNote],noteDuration);

       // to distinguish the notes, set a minimum time between them.
       // the note's duration + 30% seems to work well:
       int pauseBetweenNotes = noteDuration * 1.30;
       delay(pauseBetweenNotes);

       // stop the tone playing:
       buzz(melodyPin, 0,noteDuration);

    }

   }else{

     Serial.println(" 'Mario Theme'");
     int size = sizeof(melody) / sizeof(int);
     for (int thisNote = 0; thisNote < size; thisNote++) {

       // to calculate the note duration, take one second
       // divided by the note type.
       //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
       int noteDuration = 1000/tempo[thisNote];

       buzz(melodyPin, melody[thisNote],noteDuration);

       // to distinguish the notes, set a minimum time between them.
       // the note's duration + 30% seems to work well:
       int pauseBetweenNotes = noteDuration * 1.30;
       delay(pauseBetweenNotes);

       // stop the tone playing:
       buzz(melodyPin, 0,noteDuration);

    }
  }
}

void buzz(int targetPin, long frequency, long length) {
  digitalWrite(13,HIGH);
  long delayValue = 1000000/frequency/2; // calculate the delay value between transitions
  //// 1 second's worth of microseconds, divided by the frequency, then split in half since
  //// there are two phases to each cycle
  long numCycles = frequency * length/ 1000; // calculate the number of cycles for proper timing
  //// multiply frequency, which is really cycles per second, by the number of seconds to 
  //// get the total number of cycles to produce
  for (long i=0; i < numCycles; i++){ // for the calculated length of time...
    digitalWrite(targetPin,HIGH); // write the buzzer pin high to push out the diaphram
    delayMicroseconds(delayValue); // wait for the calculated delay value
    digitalWrite(targetPin,LOW); // write the buzzer pin low to pull back the diaphram
    delayMicroseconds(delayValue); // wait again or the calculated delay value
  }
  digitalWrite(13,LOW);

}



#include <Button.h>
#include <NewTone.h>
#include <Potentiometer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>

#include <pitches.h>
#define POTENTIOMETER_PIN A0
#define TONE_PIN 6 // Pin you have speaker/piezo connected to (be sure to include a 100 ohm resistor).
#define BUTTON_PIN 2 
#define L_BUTTON_PIN 4
#define R_BUTTON_PIN 5
#define DISPLAY_TX_PIN 8
#define DISPLAY_RX_PIN 10
#define DISPLAY_RESET_PIN 7 

#define TEMPERATURE_PROBE_PIN 9
#define TEMPERATURE_PRECISION 12

#define LONGPRESS_LEN    10  // Min nr of loops for a long press
#define DELAY            0  // Delay per loop in ms
#define MENU_ITEMS  4


enum { EV_NONE=0, EV_SHORTPRESS, EV_LONGPRESS };
enum { NEXT_ITEM=2 };
enum { COMMA=5, COLON, NONE };

const boolean DEBUG = false;

const int buttonLeft  = 4;
const int buttonRight = 5;
const int led         = 12;

Button button = Button(BUTTON_PIN,BUTTON_PULLUP);
Button l_button = Button(L_BUTTON_PIN,BUTTON_PULLUP);
Button r_button = Button(R_BUTTON_PIN,BUTTON_PULLUP);

boolean button_was_pressed; // previous state
int button_pressed_counter; // press running duration
int sub_menu_selected = 0;

String display_txt;

boolean main_menu_active = false;

SoftwareSerial s7s(DISPLAY_RX_PIN, DISPLAY_TX_PIN);
Potentiometer pot = Potentiometer(POTENTIOMETER_PIN);

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(TEMPERATURE_PROBE_PIN);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature temp_probe(&oneWire);


void setup(){
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Enable internal pull-up resistor on pin 4
  pinMode(L_BUTTON_PIN, INPUT_PULLUP); // Enable internal pull-up resistor on pin 4
  pinMode(R_BUTTON_PIN, INPUT_PULLUP); // Enable internal pull-up resistor on pin 4
  pinMode(DISPLAY_RESET_PIN, OUTPUT);
  pinMode(led,OUTPUT);
  
  pot.setSectors(MENU_ITEMS);
  
  reset_display(DISPLAY_RESET_PIN);
  s7s.begin(9600);
  
  Serial.begin(9600);
  
  button_was_pressed = false;
  button_pressed_counter = 0;
  
    // Start up the Dallas Temperature library
  temp_probe.begin();
  temp_probe.setResolution(TEMPERATURE_PRECISION);

}


void loop(){
  int menu_status = get_menu_status();
  int punct_mark;
  
  if(main_menu_active)
  {
     // Menu mode
     switch(pot.getSector())
     {
             
       case 3:
         display_txt = "A";
       break;
       
       case 2:
         display_txt = "B";
       break;
       
       case 1:
         display_txt = "C";
       break;     
     
       case 0:
         display_txt = "D";
         play(2,30);
       break;
     
     }
     
     
     
     if(menu_status == NEXT_ITEM)
     {
       Serial.print("N.I. ");     
       display_txt = (String)sub_menu_selected;
       ++sub_menu_selected;
       
     }
     
     if(l_button.uniquePress())
     {
         play(3,1);
         Serial.print("L ");
        display_txt = "L";     
     }
      
     if(r_button.uniquePress())
     {
         play(4,1);
         Serial.print("R ");     
         display_txt = "R";
     }
     
     
  }else
  {
    // Temp mode
    
    #display_txt = (String)get_temp();

    if(display_txt.equals("-127"))    
      punct_mark = NONE;
    else
      punct_mark = COMMA;
    
    
  }
  
  write_text(display_txt, punct_mark);
  
  static int counter = 0;
  if ((++counter & 0x1f) == 0)
    Serial.println();
  
  delay(DELAY);
  
}



int get_menu_status()
{
  boolean event = handle_button();

  // do other things
  switch (event) {
    case EV_NONE:
      return 0;
      break;
    case EV_SHORTPRESS:
      if(main_menu_active){
        return NEXT_ITEM;;
      }else{
        play(1,3);
        main_menu_active=true;
        return 0;
      }
      break;
    case EV_LONGPRESS:
      play(0,3);
      main_menu_active=false;
      return 0;
  }
  
  
}

int handle_button()
{
  int event;
  boolean button_now_pressed = button.isPressed();

  if (!button_now_pressed && button_was_pressed) {
    Serial.println(button_pressed_counter);
    if (button_pressed_counter < LONGPRESS_LEN)
      event = EV_SHORTPRESS;
    else
      event = EV_LONGPRESS;
  }
  else
    event = EV_NONE;

  if (button_now_pressed)
  {
    ++button_pressed_counter;
    digitalWrite(led, HIGH);
  }
  else
  {
    button_pressed_counter = 0;
    digitalWrite(led, LOW);
  }
  
  button_was_pressed = button_now_pressed;
  return event;
}
  
int melody[5][30] = {
  {NOTE_E5, NOTE_E5, NOTE_C5,},
  {NOTE_C5, NOTE_C5, NOTE_E5,},
  {
  NOTE_B4, NOTE_B4, NOTE_C5, NOTE_D5, 
  NOTE_D5, NOTE_C5, NOTE_B4, NOTE_A4, 
  NOTE_G4, NOTE_G4, NOTE_A4, NOTE_B4,
  NOTE_B4,          NOTE_A4, NOTE_A4, 
 
  NOTE_B4, NOTE_B4, NOTE_C5, NOTE_D5, 
  NOTE_D5, NOTE_C5, NOTE_B4, NOTE_A4,
  NOTE_G4, NOTE_G4, NOTE_A4, NOTE_B4, 
  NOTE_A4,          NOTE_G4, NOTE_G4, 
              
  },
  {NOTE_C5},
  {NOTE_E5}
};
                  
                  
                  
int noteDurations[5][30] = { 
  {8,8,4},
  {8,8,4},
  {
  4, 4, 4, 4, 
  4, 4, 4, 4, 
  4, 4, 4, 4, 
  3,    8, 2,
 
  4, 4, 4, 4, 
  4, 4, 4, 4, 
  4, 4, 4, 4, 
  3,    8, 2,
  },
  {8},
  {8}
};

void play(int song, int length){

  noNewTone(TONE_PIN); // Turn off the tone.

  delay(100);

  for (int thisNote = 0; thisNote < length; thisNote++) { // Loop through the notes in the array.
    int noteDuration = 1000/noteDurations[song][thisNote];
    NewTone(TONE_PIN, melody[song][thisNote], noteDuration); // Play thisNote for noteDuration.
    delay(noteDuration * 4 / 3); // Wait while the tone plays in the background, plus another 33% delay between notes.
  }
}

void reset_display(int resetDispPin){

    digitalWrite(resetDispPin, LOW);
    delay(300);
    digitalWrite(resetDispPin, HIGH);


}

void write_text(String text, int punct_mark ){
  char tempString[10];  // Will be used with sprintf to create strings

  //Serial.println(text);
  char t[5];
  text.toCharArray(t,5);
  sprintf(tempString, "%4s", t);
  s7s.print(tempString);

  switch(punct_mark)
  {
    case COLON:
      set_decimals(0b00010000);  // Sets digit 3 decimal on     
     
      break;
    case COMMA:
      set_decimals(0b00000100);  // Sets digit 3 decimal on
      break;
      
    default:
      set_decimals(0b00000000);    
  }

  delay(100);

}

void set_decimals(byte decimals)
{
  s7s.write(0x77);
  s7s.write(decimals);
}


int get_temp(){
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  if(DEBUG)
    Serial.print("Requesting temperatures...");
  
  temp_probe.requestTemperatures(); // Send the command to get temperatures
  
  if(DEBUG)
    Serial.println("DONE");
 
  Serial.print("Temp: ");
  temp_probe.setResolution(TEMPERATURE_PRECISION);
  float t = temp_probe.getTempCByIndex(0);
  int temp = t * 10;
  Serial.println(t);
  blink_led(led);

  if(t > -50){ 
     return temp;
  }else{
    return -127;
  }
}

void blink_led(int led){
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(100);               // wait for a second
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW

}


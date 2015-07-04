
#include <Button.h>
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Potentiometer.h>
#include <MemoryFree.h>

#define POTENTIOMETER_PIN A0

#define DISPLAY_TX_PIN 8
#define DISPLAY_RX_PIN 10
#define DISPLAY_RESET_PIN 7

#define BIG_BUTTON_PIN 2
#define L_BUTTON_PIN 4
#define R_BUTTON_PIN 5

#define REMOTE_PWR_ON 10
#define REMOTE_PWR_OFF 11

#define TEMPERATURE_PROBE_PIN 9
#define TEMPERATURE_PRECISION 11

#define PRETTY_PRINT_MULTIPLIER 10

#define MIN_PWR_SWITCH_PAUSE 10


enum { COMMA=5, COMMA_10=6, COLON, NONE };
enum { ON=10, OFF=11 };

const int led = 12;

boolean isLedOn = false;
long timeSincePwrSwitch;

unsigned long lastPollPeriod = 0;
const int pollPeriod = 1000;

unsigned long lastTimeChangePeriod = 0;
int lastTempChangePeriod = 0;


int lastPowerState;

int holdTemp = 0;
float acceptedChange = 0.0;
int pwr_switch_pause_addition = 0;

int menuItemSelected;

Potentiometer potentiometer = Potentiometer(POTENTIOMETER_PIN);

Button big_button = Button(BIG_BUTTON_PIN,BUTTON_PULLUP);
Button l_button = Button(L_BUTTON_PIN,BUTTON_PULLUP);
Button r_button = Button(R_BUTTON_PIN,BUTTON_PULLUP);


SoftwareSerial s7s(DISPLAY_RX_PIN, DISPLAY_TX_PIN);

OneWire oneWire(TEMPERATURE_PROBE_PIN);
DallasTemperature temp_probe(&oneWire);

String out;

void setup() {
  Serial.begin(9600);
  Serial.println(F("Project C - SousVide Edition"));
  Serial.println(F("Setup staring."));

  pinMode(led,OUTPUT);
  pinMode(DISPLAY_RESET_PIN, OUTPUT);

  pinMode(BIG_BUTTON_PIN, INPUT_PULLUP); // Enable internal pull-up resistor on pin  
  pinMode(L_BUTTON_PIN, INPUT_PULLUP); // Enable internal pull-up resistor on pin
  pinMode(R_BUTTON_PIN, INPUT_PULLUP); // Enable internal pull-up resistor on pin
  
  pinMode(REMOTE_PWR_ON, OUTPUT);
  pinMode(REMOTE_PWR_OFF, OUTPUT);
  
  reset_display(DISPLAY_RESET_PIN);
  s7s.begin(9600);
  
  temp_probe.begin();
  temp_probe.setResolution(TEMPERATURE_PRECISION);
  
  potentiometer.setSectors(2); // Two choises for the menu. Temp control ON, or OFF

  Serial.println(F("Turning off remote power "));
  remote_power_on();
  delay(10000);
  remote_power_off();
  print_power_state();  
  reset_display(DISPLAY_RESET_PIN);
  
}

void loop() {

  menuItemSelected = potentiometer.getSector();
  

  if((millis() - lastPollPeriod) > pollPeriod) {
    lastPollPeriod = millis();
  
    isLedOn = !isLedOn;
    digitalWrite(led, isLedOn ? HIGH : LOW );
  
    out = "";
    get_new_variables_from_serial();
    
    float temp = get_temp();
    int temp_do_display = temp * PRETTY_PRINT_MULTIPLIER;
    
    out += "Menu_selected="; 
    out += menuItemSelected;
    
    // Converts the float into a serial 
    out += " Temp_hr_0=";
    out += to_string_from_float(temp);
    out += " ";
    
    if(menuItemSelected == 1){

      if(big_button.isPressed()){      
        holdTemp = get_new_hold_temp();
      }
      
      // Main magic - Does temp control on heater, and writes the result to dispaly
      
      if(do_temp_control(temp, holdTemp)){
        write_text("H" + (String)(temp_do_display),COMMA);    
      }else{
        write_text("C" + (String)(temp_do_display),COMMA);    
      }
      
    }else{
          write_text((String)(temp_do_display) ,COMMA);
    }    

    
    print_power_state();

   Serial.println(out);
  }
  
  switch_remote_pwr(l_button.isPressed() ? ON : NONE);
  switch_remote_pwr(r_button.isPressed() ? OFF : NONE);

}

String to_string_from_float(float input){
    char outstr[15];
    dtostrf(input,sizeof(input), 2, outstr); // input float, output size, trailing digits after . , putput buffer.
    return (String)outstr;
}

float get_temp_change(unsigned int per_n_second, float temp_now){

  
    if((millis() - lastTimeChangePeriod) > (per_n_second * 1000)){
      
      float temp_change  = temp_now  - lastTempChangePeriod;

      lastTimeChangePeriod = millis();
      lastTempChangePeriod = temp_now;
     
      return temp_change;      
      
    }else{
      return 0;
    }  
}

void get_new_variables_from_serial(){
  int new_holdTemp; // Tells the arduno which temperature to reach.
  int new_pwr_switch_pause_addition; // The pause time between switching the remote power on off.
  float new_acceptedChange;
  
  if(Serial.available() >0){

    new_holdTemp = Serial.parseInt();
    new_pwr_switch_pause_addition = Serial.parseInt();
    new_acceptedChange = Serial.parseFloat();
  
    if(Serial.read() == '\n'){
      if(new_holdTemp >= 0){
        holdTemp = new_holdTemp;

        out = out + "New_hold_temp=" + (String)holdTemp +" ";
      }
      
      if(new_pwr_switch_pause_addition >= 0){
      
        pwr_switch_pause_addition = new_pwr_switch_pause_addition;

        out = out + "Recived new_pwr_switch_pause_time=" + new_pwr_switch_pause_addition + " ";
      }    

      if(new_acceptedChange > -50.0){      
        acceptedChange = new_acceptedChange;      
      }
    
    } 
    
  }

  out = out + "PSP=" + (String)(MIN_PWR_SWITCH_PAUSE + pwr_switch_pause_addition) + " ";
}

int get_new_hold_temp(){
  
  int temp = (int)get_temp();
  clear_display();
  delay(200);
  write_text((String)(temp * PRETTY_PRINT_MULTIPLIER),COMMA);
  delay(1000);
  clear_display();
  delay(200);
  return temp;
  

}
//
// Magic happens here!
//
boolean do_temp_control(float temp, int holdTemp){
    boolean on = false;
    
    // Logic
    // Heater is on when temp is under holdTemp and change is under accepted change
    // Check for change in temp each N second;

    // Temp also has to be above 0 degrees, since removal of temp probe gives -127.
    // system is designed not to work below zero degrees
    
    float tempChange = get_temp_change(10, temp);
    
    if(temp > 0 & temp < (holdTemp) & tempChange <= acceptedChange){
      switch_remote_pwr(ON);
      on = true;
    }else{
      switch_remote_pwr(OFF);
    }
   
    out += "Accepted_change=" + to_string_from_float(acceptedChange) + " ";
    if(tempChange != 0){
      out += "Temp_change=";
      out += to_string_from_float(tempChange) + " ";
    }
    
    out = out + "Temp_control=" + to_string_from_float(temp) + " Hold_temp=" + holdTemp + " ";
    
    return on;
}

void print_power_state(){
 
  out = out + "Last_Power_State=";
  lastPowerState == 11 ? out = out + "OFF " : out = out + "ON ";


}

void switch_remote_pwr(int power_status){
  if (power_status == NONE){
    return;
  }
  
  int power_switch_pause_seconds =  MIN_PWR_SWITCH_PAUSE + pwr_switch_pause_addition;


  long deltaTime = millis() - timeSincePwrSwitch;


  out = out + "delta_lock=";
  if(deltaTime > (power_switch_pause_seconds * 1000)){
    out = out + "OFF ";
  }else{
    out = out + "ON ";
  }
  
  if(power_status != lastPowerState && deltaTime > (power_switch_pause_seconds * 1000)){  
    switch(power_status)
    {
      case ON:
        remote_power_on();
        break;
      
      case OFF:
        remote_power_off();
        break;
  
      default:
        break;
    }
  }else if (l_button.isPressed() || r_button.isPressed()) {
    
    if(power_status == lastPowerState && deltaTime < (power_switch_pause_seconds * 1000)){
      power_status == ON ? write_text("ON", NONE) : write_text("OFF", NONE);   
    }else{
      
      write_text((String)(deltaTime / 1000), NONE);
      delay(500);
      write_text("D"+(String)power_switch_pause_seconds, NONE);
      delay(500);  
    }
  }
}

void remote_power_off(){
    digitalWrite(REMOTE_PWR_OFF, HIGH);
    delay(100);
    digitalWrite(REMOTE_PWR_OFF, LOW);
    lastPowerState = OFF;
    timeSincePwrSwitch = millis();
    write_text((String)"OFF", NONE);
    delay(500);
}


void remote_power_on(){
    digitalWrite(REMOTE_PWR_ON, HIGH);
    delay(100);
    digitalWrite(REMOTE_PWR_ON, LOW);
    lastPowerState = ON;
    timeSincePwrSwitch = millis();
    write_text((String)"ON", NONE);
    delay(500);
}

float get_temp(){
  temp_probe.requestTemperatures(); // Send the command to get temperatures
  

  //temp_probe.setResolution(TEMPERATURE_PRECISION);
  float t = temp_probe.getTempCByIndex(0);
  return t;

}
void blink_led(int led){
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(100);               // wait for a second
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW

}
void reset_display(int resetDispPin){

    digitalWrite(resetDispPin, LOW);
    delay(300);
    digitalWrite(resetDispPin, HIGH);


}

void write_text(String text, int punct_mark ){
  char tempString[10];  // Will be used with sprintf to create strings

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
    case COMMA_10:
      set_decimals(0b00000010);  // Sets digit 3 decimal on
      break;     
      
    default:
      set_decimals(0b00000000);    
  }

  delay(100);

}

void clear_display(){
  s7s.write(0x76);
}

void set_decimals(byte decimals)
{
  s7s.write(0x77);
  s7s.write(decimals);
}


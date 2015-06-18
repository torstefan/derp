
#include <Button.h>
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Potentiometer.h>

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

#define MIN_PWR_SWITCH_PAUSE 20


enum { COMMA=5, COMMA_10=6, COLON, NONE };
enum { ON=10, OFF=11 };

const int led = 12;
int c = 0;
boolean isLedOn = false;
long timeSincePwrSwitch;

long lastPollPeriod = 0;
int pollPeriod = 1000;

int lastPowerState;

int holdTemp = 0;
int pwr_switch_pause_addition = 0;


int menuItemSelected;

Potentiometer potentiometer = Potentiometer(POTENTIOMETER_PIN);

Button big_button = Button(BIG_BUTTON_PIN,BUTTON_PULLUP);
Button l_button = Button(L_BUTTON_PIN,BUTTON_PULLUP);
Button r_button = Button(R_BUTTON_PIN,BUTTON_PULLUP);


SoftwareSerial s7s(DISPLAY_RX_PIN, DISPLAY_TX_PIN);

OneWire oneWire(TEMPERATURE_PROBE_PIN);
DallasTemperature temp_probe(&oneWire);

void setup() {
  Serial.begin(9600);
  Serial.println("Project C v0.2.");
  Serial.println("Setup staring.");
  // put your setup code here, to run once:
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

}

void loop() {
  // put your main code here, to run repeatedly: 

  if(c<1){
    Serial.println("Turning off remote power ");
    remote_power_on();
    delay(10000);
    remote_power_off();
    print_power_state();  
    Serial.println();
    Serial.println("Setup complete..");
    Serial.println();

    reset_display(DISPLAY_RESET_PIN);
  }

  menuItemSelected = potentiometer.getSector();
  

  if((millis() - lastPollPeriod) > pollPeriod) {
    get_new_variables_from_serial();
  
    lastPollPeriod = millis();
    isLedOn = !isLedOn;
    digitalWrite(led, isLedOn ? HIGH : LOW );
  
    float temp = get_temp();
    long int temp_do_display = temp * PRETTY_PRINT_MULTIPLIER;
    
    Serial.print("Time=" + String(lastPollPeriod/1000) + " ");
    Serial.print("Menu_selected=" + (String)menuItemSelected + " ");  
    Serial.print("Temp_hr_0="); Serial.print(temp); Serial.print(" ");

    if(menuItemSelected == 1){

      if(big_button.isPressed()){      
        holdTemp = get_new_hold_temp();
      }
      
      if(do_temp_control((int)temp, holdTemp)){
        write_text("H" + (String)(temp_do_display),COMMA);    
      }else{
        write_text("C" + (String)(temp_do_display),COMMA);    
      }
      
    }else{
          write_text((String)(temp_do_display) ,COMMA);
    }    

    
    print_power_state();
    

    c++;
  

   Serial.println();
  }
  
  switch_remote_pwr(l_button.isPressed() ? ON : NONE);
  switch_remote_pwr(r_button.isPressed() ? OFF : NONE);

}

void get_new_variables_from_serial(){

  // Looks for bytes stored in a 64 byte serial input buffer
  if(Serial.available() >0){
    int new_holdTemp = Serial.parseInt();
    int new_pwr_switch_pause_addition = Serial.parseInt();
    
    if(new_holdTemp > 0){
        holdTemp = new_holdTemp;
        Serial.print("New_holdTemp=" + (String)holdTemp +" ");    
    }
    
    if(new_pwr_switch_pause_addition > 0){
      pwr_switch_pause_addition = new_pwr_switch_pause_addition;
      Serial.print("New_pwr_pause_addition=" + (String)pwr_switch_pause_addition +" ");
    }

  }

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

boolean do_temp_control(int temp, int holdTemp){
    boolean on = false;
    
    if(temp > 0 & temp < (holdTemp)){
      switch_remote_pwr(ON);
      on = true;
    }else{
      switch_remote_pwr(OFF);
    }
    Serial.print("Temp_control="), Serial.print(temp), Serial.print(" Hold_temp="), Serial.print(holdTemp), Serial.print(" ");
    return on;
}

void print_power_state(){
  Serial.print("Last_Power_State=");
  lastPowerState == 11 ? Serial.print("OFF") : Serial.print("ON");

}

void switch_remote_pwr(int power_status){
  if (power_status == NONE){
    return;
  }
  
  int power_switch_pause_seconds =  MIN_PWR_SWITCH_PAUSE + pwr_switch_pause_addition;


  long deltaTime = millis() - timeSincePwrSwitch;

  Serial.print("delta_lock=");  
  if(deltaTime > (power_switch_pause_seconds * 1000)){
    Serial.print("OFF ");
  }else{
    Serial.print("ON ");  
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


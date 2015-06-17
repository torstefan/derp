
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
#define TEMPERATURE_PRECISION 9

#define PRETTY_PRINT_MULTIPLIER 10

#define PWR_SWITCH_PAUSE 20


enum { COMMA=5, COMMA_10=6, COLON, NONE };
enum { ON=10, OFF=11 };

const int led = 12;
int c = 0;
boolean isLedOn = false;
long timeSincePwrSwitch;

long lastPollPeriod = 0;
int pollPeriod = 1000;

int lastPowerState;

int holdTemp;

int menuItemSelected;

Potentiometer potentiometer = Potentiometer(POTENTIOMETER_PIN);

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
  
    lastPollPeriod = millis();
    isLedOn = !isLedOn;
    digitalWrite(led, isLedOn ? HIGH : LOW );
  
    int temp = get_temp();
    
    holdTemp = 30 * PRETTY_PRINT_MULTIPLIER;
    
    Serial.print("Time=");Serial.print(lastPollPeriod/1000);Serial.print(" ");
    Serial.print("Menu_selected="); Serial.print(menuItemSelected);Serial.print(" ");  


    if(menuItemSelected == 1){
          do_temp_control(temp, holdTemp);
          write_text("H" + (String)temp,COMMA);    
    }else{
          write_text((String)temp,COMMA);
    }    

    
    print_power_state();
    

    c++;
  

   Serial.println();
  }
  
  switch_remote_pwr(l_button.isPressed() ? ON : NONE);
  switch_remote_pwr(r_button.isPressed() ? OFF : NONE);

}

void do_temp_control(int temp, int holdTemp){
  
    if(temp > 0 & temp < (holdTemp)){
      switch_remote_pwr(ON);   
    }else{
      switch_remote_pwr(OFF);
    }
    Serial.print("Temp_0="), Serial.print(temp/PRETTY_PRINT_MULTIPLIER), Serial.print(" Hold_temp="), Serial.print(holdTemp/PRETTY_PRINT_MULTIPLIER), Serial.print(" ");
}

void print_power_state(){
  Serial.print("Last_Power_State=");
  lastPowerState == 11 ? Serial.print("OFF") : Serial.print("ON");

}

void switch_remote_pwr(int power_status){
  if (power_status == NONE){
    return;
  }

  long deltaTime = millis() - timeSincePwrSwitch;

  Serial.print("delta_lock=");  
  if(deltaTime > (PWR_SWITCH_PAUSE * 1000)){
    Serial.print("OFF ");
  }else{
    Serial.print("ON ");  
  }
  
  if(power_status != lastPowerState && deltaTime > (PWR_SWITCH_PAUSE * 1000)){  
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
    write_text((String)(deltaTime / 1000), NONE);
    delay(500);
  
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

long int get_temp(){
  temp_probe.requestTemperatures(); // Send the command to get temperatures
  

  //temp_probe.setResolution(TEMPERATURE_PRECISION);
  float t = temp_probe.getTempCByIndex(0);
  return (long int) (t * PRETTY_PRINT_MULTIPLIER);

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

void set_decimals(byte decimals)
{
  s7s.write(0x77);
  s7s.write(decimals);
}


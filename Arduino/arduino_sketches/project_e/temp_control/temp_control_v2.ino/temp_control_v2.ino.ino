//-------------------------------------------------------------------
//
// Sous Vide Controller
// Tor Stefan Lura - for Derp Engineering R&D Dep.
//
// Based on sketch of Bill Earl - for Adafruit Industries
// And the Arduino PID and PID AutoTune Libraries 
// by Brett Beauregard
//------------------------------------------------------------------

// Library for the potentiometer 
#include <Potentiometer.h>

// Libraries for the physical button
#include <Button.h>

// PID Library
#include <PID_v1.h>
#include <PID_AutoTune_v0.h>

// Libraries for the screen
#include <SoftwareSerial.h>

// Libraries for the DS18B20 Temperature Sensor
#include <OneWire.h>
#include <DallasTemperature.h>

// So we can save and retrieve settings
#include <EEPROM.h>


// ************************************************
// Pin definitions
// ************************************************

// Display
#define DISPLAY_TX_PIN 8
#define DISPLAY_RX_PIN 10
#define DISPLAY_RESET_PIN 7

// Temperature probe
#define TEMPERATURE_PROBE_PIN 9
#define TEMPERATURE_PRECISION 12

// Remote relay
#define REMOTE_PWR_ON 10
#define REMOTE_PWR_OFF 11

// Status led
#define STATUS_LED 12

// Potentiometer
#define POTENTIOMETER_PIN A0

// Physical buttons
#define BIG_BUTTON_PIN 2
#define L_BUTTON_PIN 4
#define R_BUTTON_PIN 5


// ************************************************
// PID Variables and constants
// ************************************************

//Define Variables we'll be connecting to
double Setpoint;
double Input;
double Output;

volatile long onTime = 0;

// pid tuning parameters
double Kp;
double Ki;
double Kd;

// EEPROM addresses for persisted data
const int SpAddress = 0;
const int KpAddress = 8;
const int KiAddress = 16;
const int KdAddress = 24;

//Specify the links and initial tuning parameters
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

// 10 second Time Proportional Output window
int WindowSize = 10000; 
unsigned long windowStartTime;

// ************************************************
// Auto Tune Variables and constants
// ************************************************
byte ATuneModeRemember=2;

double aTuneStep=500;
double aTuneNoise=1;
unsigned int aTuneLookBack=20;

boolean tuning = false;

PID_ATune aTune(&Input, &Output);

// ************************************************
// States for state machine
// ************************************************
enum operatingState { OFF = 0, SETP, RUN, TUNE_P, TUNE_I, TUNE_D, AUTO};
operatingState opState = OFF;


// ************************************************
// Display Variables and constants
// ************************************************

SoftwareSerial s7s(DISPLAY_RX_PIN, DISPLAY_TX_PIN);
enum { COMMA=5, COMMA_10=6, COLON, NONE };
const int logInterval = 10000; // log every 10 seconds
long lastLogTime = 0;

// ************************************************
// Sensor Variables and constants
// ************************************************

OneWire oneWire(TEMPERATURE_PROBE_PIN);

DallasTemperature t_sensors (&oneWire);

// arrays to hold device address
DeviceAddress tempSensor;

// ************************************************
// Relay Variables
// ************************************************

long timeSincePwrSwitch;
enum powerState { R_ON=10, R_OFF=11 };
powerState lastPowerState;
powerState powerStateNow;


// ************************************************
// Physical input
// ************************************************

Potentiometer potentiometer = Potentiometer(POTENTIOMETER_PIN);

Button big_button = Button(BIG_BUTTON_PIN,BUTTON_PULLUP);
Button l_button = Button(L_BUTTON_PIN,BUTTON_PULLUP);
Button r_button = Button(R_BUTTON_PIN,BUTTON_PULLUP);





// ************************************************
// Setup
// ************************************************

void setup() {
  Serial.begin(115200);
  Serial.println(F("Project C - SousVide Edition v2"));
  Serial.println(F("Setup staring."));

  
  // Initialize Display
  
  pinMode(DISPLAY_RESET_PIN, OUTPUT);
  reset_display(DISPLAY_RESET_PIN);
  s7s.begin(9600);
  hello_display(); hello_display(); 

  // Initialize Relay Control
  
  pinMode(REMOTE_PWR_ON, OUTPUT);
  pinMode(REMOTE_PWR_OFF, OUTPUT);
  remote_power_on();
  delay(5000);
  remote_power_off();    
  powerStateNow = R_OFF;

  // Initialize LED Status
  
  pinMode(STATUS_LED,OUTPUT);

  // Initialize physical input
  
  // 6 choises for the statemachine. 
  // OFF, RUN, Tune Setpoint, Tune Kp, Tune Ki, Tune Kd  
  potentiometer.setSectors(6); 
  pinMode(BIG_BUTTON_PIN, INPUT_PULLUP); // Enable internal pull-up resistor on pin  
  pinMode(L_BUTTON_PIN, INPUT_PULLUP); // Enable internal pull-up resistor on pin
  pinMode(R_BUTTON_PIN, INPUT_PULLUP); // Enable internal pull-up resistor on pin
  

  // Initialize sensors
  
  t_sensors.begin();
  if(!t_sensors.getAddress(tempSensor, 0)){
    write_display("ErrS", NONE);    
    }
   t_sensors.setResolution(tempSensor, 12);
   t_sensors.setWaitForConversion(false);

  
   // Initialize the PID and related variables
   
  LoadParameters();
  myPID.SetTunings(Kp,Ki,Kd);

  myPID.SetSampleTime(1000);
  myPID.SetOutputLimits(0, WindowSize);


  // Run timer2 interrupt every 15 ms 
  
  TCCR2A = 0;
  TCCR2B = 1<<CS22 | 1<<CS21 | 1<<CS20;

  //Timer2 Overflow Interrupt Enable
  TIMSK2 |= 1<<TOIE2;
  
  Serial.println("Setup done");
  Serial.println("");
}

// ************************************************
// Timer Interrupt Handler
// ************************************************
SIGNAL(TIMER2_OVF_vect) 
{
  if (opState == OFF)
  {
    powerStateNow = R_OFF;
   if(lastPowerState != powerStateNow){
    remote_power_off();
    lastPowerState == R_OFF;
    }
   
   
  }
  else
  {
    DriveOutput();
  }
}

// ************************************************
// Main loop
// ************************************************

void loop() {

  opState = (operatingState)potentiometer.getSector();
  
  Serial.print("OpState=" + opStateToText());  
  
  Serial.print(" SP=" + (String)Setpoint +" Kp=" + (String)Kp + " ");
  Serial.println(" Ki=" + (String)Ki +" Kd=" + (String)Kd + " ");
  
  write_display(opStateToText(), NONE);
  delay(500);

   switch (opState)
   {
   case OFF:
      Off();
      break;
   case SETP:
      Tune_Sp();
      break;
    case RUN:
      Run();
      break;
   case TUNE_P:
      TuneP();
      break;
   case TUNE_I:
      TuneI();
      break;
   case TUNE_D:
      TuneD();
      break;
   }

  Serial.println();
  
}
// ************************************************
// Off State
// ************************************************

void Off()
{
  
   myPID.SetMode(MANUAL);
   
   remote_power_off();  // make sure it is off
   set_dimlevel(0x00);
   uint8_t buttons = 0;
   
   while(OFF == (operatingState)potentiometer.getSector())
   {
    }

   
   // Prepare to transition to a possible RUN state
   t_sensors.requestTemperatures(); // Start an asynchronous temperature reading

   
}

// ************************************************
// Run State
// ************************************************

void Run()
{
   //turn the PID on
   myPID.SetMode(AUTOMATIC);
   windowStartTime = millis();
   
   
   // write setpoint to serial display
   clear_display();
   set_dimlevel((byte)255);
   write_display("S" + (String)Setpoint, NONE);
   delay(2000);
   

   SaveParameters();
   myPID.SetTunings(Kp,Ki,Kd);

   uint8_t buttons = 0;
   while(RUN == (operatingState)potentiometer.getSector())
   {
            
      if ((big_button.isPressed()) 
         && (r_button.isPressed()) 
         && (abs(Input - Setpoint) < 0.5))  // Should be at steady-state
      {
         StartAutoTune();
      }
      
      if ((big_button.isPressed()) 
         && (l_button.isPressed()) 
         )  // Should be at steady-state
      {
         CancelAutoTune();
      }     
      
      DoControl();

      write_display(String(Input), COMMA);
      
      float pct = map(Output, 0, WindowSize, 0, 1000);

     if (l_button.isPressed()){
        write_display(to_string_from_float(pct/10),COMMA);
        set_decimals(0b00100010);
        delay(1000);
      }
      
      if (r_button.isPressed()){
        write_display((String)Setpoint,COMMA);        
        delay(1000);
      }
                
      if (tuning)
      {
        set_decimals(0b00100010); // Sets the apostrophe on to indicate tuning, and the regular comma
      }
      
      
      // periodically log to serial port in csv format
      if (millis() - lastLogTime > logInterval)  
      {
        Serial.print(" PowerState=" + powerStateToText());
        Serial.print(" Setpoint=" + (String)(Setpoint));
        Serial.print(" pct="+ to_string_from_float(pct));
        Serial.print(" Input=" + (String)Input);        
        Serial.println(" Output="+ (String)Output);
      }

      delay(100);
   }
}
// ************************************************
// Setpoint Entry State
// LEFT/RIGHT to change setpoint
// SHIFT for 10x tuning
// KNOB to change state
// ************************************************
void Tune_Sp()
{
 
   while(SETP == (operatingState)potentiometer.getSector())
   {
 

      float increment = 0.1;
      if (big_button.isPressed())
      {
        increment *= 10;
      }
     
      if (r_button.isPressed())
      {
         Setpoint += increment;
         delay(200);
      }
      if (l_button.isPressed())
      {
         Setpoint -= increment;
         delay(200);
      }
      
      write_display((String)Setpoint, NONE);
      
      DoControl();
   }
}
// ************************************************
// Proportional Tuning State
// LEFT/RIGHT to change Kp
// SHIFT for 10x tuning
// KNOB to change state
// ************************************************
void TuneP()
{

   while(TUNE_P == (operatingState)potentiometer.getSector())
   {
      float increment = 1.0;
      if (big_button.isPressed())
      {
        increment *= 10;
      }

      if (r_button.isPressed())
      {
         Kp += increment;
         delay(200);
      }
      if (l_button.isPressed())
      {
         Kp -= increment;
         delay(200);
      }
      
      write_display((String)Kp, NONE);
      
      DoControl();
   }
}

// ************************************************
// Integral Tuning State
// LEFT/RIGHT to change Kp
// SHIFT for 10x tuning
// KNOB to change state
// ************************************************
void TuneI()
{

   while(TUNE_I == (operatingState)potentiometer.getSector())
   {
      

      float increment = 0.01;
      if (big_button.isPressed())
      {
        increment *= 10;
      }      
      if (r_button.isPressed())
      {
         Ki += increment;
         delay(200);
      }
      if (l_button.isPressed())
      {
         Ki -= increment;
         delay(200);
      }
      
      write_display((String)Ki, NONE);
      DoControl();
   }
}

// ************************************************
// Derivative Tuning State
// LEFT/RIGHT to change Kp
// SHIFT for 10x tuning
// KNOB to change state
// ************************************************

void TuneD()
{

   while(TUNE_D == (operatingState)potentiometer.getSector())
   {
      float increment = 0.01;
      if (big_button.isPressed())
      {
        increment *= 10;
      }
      if (r_button.isPressed())
      {
         Kd += increment;
         delay(200);
      }
      if (l_button.isPressed())
      {
         Kd -= increment;
         delay(200);
      }
      
      write_display((String)Kd, NONE);
      
      DoControl();
   }
}

// ************************************************
// ************************************************
// ************************************************
//
// Control logic
//
// ************************************************
// ************************************************
// ************************************************
// 
// Execute the control loop
// 
void DoControl()
{
  // Read the input:
  if (t_sensors.isConversionAvailable(0))
  {
    Input = t_sensors.getTempC(tempSensor);
    t_sensors.requestTemperatures(); // prime the pump for the next one - but don't wait
  }
  
  if (tuning) // run the auto-tuner
  {
     
     if (aTune.Runtime()) // returns 'true' when done
     {
        FinishAutoTune();
     }
  }
  else // Execute control algorithm
  {
     myPID.Compute();
  }
  
  // Time Proportional relay state is updated regularly via timer interrupt.
  onTime = Output; 
}

// 
// Called by ISR every 15ms to drive the output
// 
void DriveOutput()
{  
  long now = millis();
  // Set the output
  // "on time" is proportional to the PID output
  if(now - windowStartTime>WindowSize)
  { //time to shift the Relay Window
     windowStartTime += WindowSize;
  }
  if((onTime > 2000) && (onTime > (now - windowStartTime)))
  {
    
    powerStateNow = R_ON;
    
    if(lastPowerState != powerStateNow)
    {
      
      remote_power_on();
      lastPowerState = R_ON;
    }
     
  }
  else
  {
    powerStateNow = R_OFF;
    if(lastPowerState != powerStateNow){      
      remote_power_off();
      lastPowerState = R_OFF;
    }

  }
}


// ************************************************
// Auto-Tuninging functions
// ************************************************

// 
// Start the Auto-Tuning cycle
// 

void StartAutoTune()
{
   // REmember the mode we were in
   ATuneModeRemember = myPID.GetMode();

   // set up the auto-tune parameters
   aTune.SetNoiseBand(aTuneNoise);
   aTune.SetOutputStep(aTuneStep);
   aTune.SetLookbackSec((int)aTuneLookBack);
   tuning = true;
}

// 
// Return to normal control
// 
void FinishAutoTune()
{
   tuning = false;

   // Extract the auto-tune calculated parameters
   Kp = aTune.GetKp();
   Ki = aTune.GetKi();
   Kd = aTune.GetKd();

   // Re-tune the PID and revert to normal control mode
   myPID.SetTunings(Kp,Ki,Kd);
   myPID.SetMode(ATuneModeRemember);
   
   // Persist any changed parameters to EEPROM
   SaveParameters();
}

//
// Cancel Auto-Tune
//

void CancelAutoTune()
{
  tuning = false;
  
  // Cancel Auto-tuning;
  aTune.Cancel();
  
  // Revert to normal control mode
  myPID.SetMode(ATuneModeRemember);
  
  }

// ************************************************
// Relay functions
// ************************************************

void remote_power_off()
{
    digitalWrite(REMOTE_PWR_OFF, HIGH);
    delay(100);
    digitalWrite(REMOTE_PWR_OFF, LOW);
    lastPowerState = R_OFF;
    digitalWrite(STATUS_LED, LOW);
    timeSincePwrSwitch = millis();
    write_display((String)"OFF-", NONE);
    delay(800);
}


void remote_power_on()
{
    digitalWrite(REMOTE_PWR_ON, HIGH);
    delay(100);
    digitalWrite(REMOTE_PWR_ON, LOW);
    lastPowerState = R_ON;
    digitalWrite(STATUS_LED, HIGH);
    
    timeSincePwrSwitch = millis();
    write_display((String)"ON--", NONE);
    delay(800);
}


// ************************************************
// Display functions
// ************************************************

//
// Hello msg
//
void hello_display()
{
    write_display("  -)", COLON);
    delay(500);
    write_display("(-  ", COLON);
    delay(500);
}
//
// Reset the 4 byte serial display
//
void reset_display(int resetDispPin)
{
    digitalWrite(resetDispPin, LOW);
    delay(300);
    digitalWrite(resetDispPin, HIGH);


}
//
//  Write 4 byte string to the display
//
void write_display(String text, int punct_mark )
{
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

//
// Clear the 4 byte serial display
//
void clear_display()
{
  s7s.write(0x76);
}

//
// Activate decimal points on the 4 byte serial display
//
void set_decimals(byte decimals)
{
  s7s.write(0x77);
  s7s.write(decimals);
}

void set_dimlevel(byte level)
{
  Serial.write(0x7A);  // Brightness control command
  Serial.write(level);  // 0 - 255
}

String opStateToText(){
     
     // Uses global variable
     switch (opState)
   {
   case OFF:
      return F("OFF");
      break;
   case SETP:
      return F("-Sp-");
      break;
    case RUN:
      return F("-Run");
      break;
   case TUNE_P:
      return F("---p");
      break;
   case TUNE_I:
      return F("---I");
      break;
   case TUNE_D:
      return F("---D");
      break;
   default:
      return F("----");
   }
  

}

String powerStateToText(){
     // Uses global variable
     switch (powerStateNow)
   {
   case R_OFF:
      return F("rOFF");
      break;
   case R_ON:
      return F("r ON");
      break;
   }
  
  }
String to_string_from_float(float input){
    char outstr[15];
    dtostrf(input,sizeof(input), 2, outstr); // input float, output size, trailing digits after . , putput buffer.
    return (String)outstr;
}
// ************************************************
// EEPROM functions
// ************************************************


// 
// Save any parameter changes to EEPROM
// 
void SaveParameters()
{
   if (Setpoint != EEPROM_readDouble(SpAddress))
   {
      EEPROM_writeDouble(SpAddress, Setpoint);
   }
   if (Kp != EEPROM_readDouble(KpAddress))
   {
      EEPROM_writeDouble(KpAddress, Kp);
   }
   if (Ki != EEPROM_readDouble(KiAddress))
   {
      EEPROM_writeDouble(KiAddress, Ki);
   }
   if (Kd != EEPROM_readDouble(KdAddress))
   {
      EEPROM_writeDouble(KdAddress, Kd);
   }
}

// 
// Load parameters from EEPROM
// 
void LoadParameters()
{
  // Load from EEPROM
   Setpoint = EEPROM_readDouble(SpAddress);
   Kp = EEPROM_readDouble(KpAddress);
   Ki = EEPROM_readDouble(KiAddress);
   Kd = EEPROM_readDouble(KdAddress);
   
   // Use defaults if EEPROM values are invalid
   if (isnan(Setpoint))
   {
     Setpoint = 28;
   }
   if (isnan(Kp))
   {
     Kp = 850;
   }
   if (isnan(Ki))
   {
     Ki = 0.5;
   }
   if (isnan(Kd))
   {
     Kd = 0.1;
   }  
}


// 
// Write floating point values to EEPROM
// 
void EEPROM_writeDouble(int address, double value)
{
   byte* p = (byte*)(void*)&value;
   for (int i = 0; i < sizeof(value); i++)
   {
      EEPROM.write(address++, *p++);
   }
}

// 
// Read floating point values from EEPROM
// 
double EEPROM_readDouble(int address)
{
   double value = 0.0;
   byte* p = (byte*)(void*)&value;
   for (int i = 0; i < sizeof(value); i++)
   {
      *p++ = EEPROM.read(address++);
   }
   return value;
}





















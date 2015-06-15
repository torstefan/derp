#include <stage.h>
#include <Button.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define BUTTON_PIN 2 

#define TEMPERATURE_PROBE_PIN 9
#define TEMPERATURE_PRECISION 12


const int led         = 12;
Stage st;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(TEMPERATURE_PROBE_PIN);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature temp_probe(&oneWire);
Button button = Button(BUTTON_PIN,BUTTON_PULLUP);


void setup()
{
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Enable internal pull-up resistor on pin 4
  pinMode(led,OUTPUT);
  temp_probe.begin();
  temp_probe.setResolution(TEMPERATURE_PRECISION);

  Serial.begin(9600);
  st = Stage(22,27,5);
  st.pause(false);
}

void loop()
{
	int temp = get_temp();
	if(st.completed())
	{
		Serial.println("Stage completed");	
	}else
	{
		if(button.uniquePress())
		{
			st.isPaused() ? 
			    Serial.print("Brewing timer paused: ") :
			    Serial.print("Brewing timer active: ");

			Serial.println(st.pause(!st.isPaused()));	
		
		}
		if(st.isPaused())
		{
		
  			digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
		}else{
				
  			digitalWrite(led, LOW);   // turn the LED on (HIGH is the voltage level)
		}
		Serial.print("TUSC: ");	
		Serial.println(st.timeLeft());	
		Serial.print("Temp: ");	
		Serial.println(temp);
		Serial.print("Seconds Active: ");
		Serial.println(st.secondsActive());
	
		switch (st.checkTempAlarm(temp))
		{
			case TEMP_HIGH:
				Serial.print("Maxtemp: "); 
				Serial.println(st.getMaxTemp());
				Serial.println("Temp to high! ");
				break;
			case TEMP_LOW:
				Serial.println("Temp to low! ");
				break;
			case TEMP_OK:
				Serial.println("Temp OK ");
				break;
		}


	}
	Serial.println();
    delay(1000);
    
}

float get_temp()
{

  temp_probe.requestTemperatures(); // Send the command to get temperatures
  temp_probe.setResolution(TEMPERATURE_PRECISION);
  float t = temp_probe.getTempCByIndex(0);
  if(t > -50){ 
     return t;
  }else{
    return -127;
  }
}

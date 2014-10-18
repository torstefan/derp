#include <stage.h>
Stage st;

void setup()
{
  Serial.begin(9600);
  st = Stage();
  st.setMaxTemp(24);
  st.setMinTemp(22);
  st.setLength(5);
  st.pause(false);
}

void loop()
{
	int temp = 23;
	if(st.completed())
	{
		Serial.println("Stage completed");	
	}else
	{
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

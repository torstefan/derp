#include "stage.h"
#include "Arduino.h"

Stage::Stage()
{
}

Stage::Stage(int min_temp, int max_temp, int minutes)
{

  _min_temp = min_temp;
  _max_temp = max_temp;
  _length_minutes = minutes;

}
void Stage::setMaxTemp(int max_temp)
{
  _max_temp = max_temp;
}

void Stage::setMinTemp(int min_temp)
{
  _min_temp = min_temp;
}

void Stage::setLength(int minutes)
{
  _length_minutes = minutes;
}

int Stage::getMaxTemp()
{
  return _max_temp;
}

int Stage::getMinTemp()
{
  return _min_temp;
}

int Stage::getLength()
{
  return _length_minutes;
}


int Stage::checkTempAlarm(int temp)
{
    if( temp > _max_temp)
    {
	return TEMP_HIGH;
    }
    if( temp < _min_temp)
    {
	return TEMP_LOW;
    }

    return TEMP_OK;
}

int Stage::secondsActive()
{
  return int(_time_active / 1000);
}

int Stage::minutesActive()
{
  return int((_time_active / 1000) / 60);
}

int Stage::timeLeft()
{
    return _length_minutes - minutesActive();
    
}

boolean Stage::completed()
{
    if(!_paused)
    {
	_updateTimeActive();

	if(timeLeft() <= 0)
	{
	    return true;	
	}

    }
  return false;
}

boolean Stage::isPaused()
{
    return _paused;
}


unsigned long Stage::pause(boolean paused)
{
    if(_time_started == 0)
    {
	_time_started = millis(); 
    }

    if(!_paused & paused)
    {
	_time_paused = millis();
	return _time_paused; 
    }
    
    if(_paused & !paused){
	unsigned long time_now = millis();
	_time_active -= (time_now - _time_paused);
	_updateTimeActive();
	return _time_active;
    }

    _paused = paused;

}
void Stage::_updateTimeActive(){
	unsigned long time_now = millis();
	_time_active += (time_now - _time_active); 
}


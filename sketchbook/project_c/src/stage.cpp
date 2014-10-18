#include "stage.h"
#include "Arduino.h"



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

int Stage::checkTempAlarm(int temp)
{
  if((temp >_max_temp) || (temp < _min_temp))
  {
    return 1;  
  }
  else
  {
    return -1;
  }
}

int Stage::completed()
{
  _m_seconds_active = millis();
  return 0;
}

int Stage::getMinutesActive()
{
  return 0;
}

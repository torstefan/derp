#ifndef STAGE_H
#define STAGE_H

#include "Arduino.h"

class Stage
{
  
  public:
    void setMaxTemp(int max_temp);
    void setMinTemp(int min_temp);
    void setLength(int minutes);
    int checkTempAlarm(int temp);
    int completed();
    int getMinutesActive();

  private:
    int _max_temp;
    int _min_temp;
    int _length_minutes;
    int _m_seconds_active;



};

#endif

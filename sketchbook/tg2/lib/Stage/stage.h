#ifndef STAGE_H
#define STAGE_H

#include "Arduino.h"

enum {
    TEMP_HIGH = 10,
    TEMP_LOW = 11,
    TEMP_OK = 12
};

class Stage
{
  
  public:
    Stage();
    Stage(int min_temp, int max_temp, int minutes);
    void setMaxTemp(int max_temp);
    void setMinTemp(int min_temp);
    void setLength(int minutes);
    int getMaxTemp();
    int getMinTemp();
    int getLength();

    int checkTempAlarm(int temp);
    int secondsActive();
    int minutesActive();
    int timeLeft();
    boolean completed();
    boolean isPaused();
    void pause(boolean paused);

  private:
    void _updateTimeActive();
    int _max_temp=0;
    int _min_temp=0;
    int _length_minutes=0;
    unsigned long _time_started=0;
    unsigned long _time_paused=0;
    unsigned long _time_active=0;

    boolean _paused=true;




};

#endif

class Stage
{
  private:
    int _max_temp;
    int _min_temp;
    int _length_minutes;
    int _minutes_active;

  public:
    void setMaxTemp(int max_temp);
    void setMinTemp(int min_temp);
    void setLength(int minutes);
    int checkAlarm(int temp);
    int getMinutesActive();


};

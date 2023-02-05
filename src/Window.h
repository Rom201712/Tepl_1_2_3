#pragma once

#define ON HIGH
#define OFF LOW

class Window
{
private:

    // int changelevel_;         // количество шагов закрытия/открытия окна (>0 - открытие, <0 - закрытие)
    unsigned long timeopen_;  // время выключения открытия окон
    unsigned long timeclose_; // время выключения закрытия окон
    int level_ = 0;           // уровень открытия
    int opentimewindow_;      // полное время  работы механизма, сек
    int relayUp_, relayDown_;
    MB11016P_ESP *relay__;

public:
    Window(MB11016P_ESP *mb11016p, int relayUp, int relayDown, int opentimewindow) : opentimewindow_(opentimewindow),  relayUp_(relayUp), relayDown_(relayDown)
    {
        relay__ = mb11016p;
    }

    int getlevel() const
    {
        return level_;
    }
    //изменение времени работы механизма
    void setopentimewindow(int opentimewindow)
    {
        if (0 == level_ && !getWindowDown())
            opentimewindow_ = opentimewindow;
            // Serial.println(opentimewindow);
    }

    int getWindowUp() const
    {
        return relay__->getRelay(relayUp_);
    }

    int getWindowDown() const
    {
        return relay__->getRelay(relayDown_);
    }

    int getOpenTimeWindow() const
    {
        return opentimewindow_;
    }

    //включение механизма открытия окна
    void openWindow(int changelevel)
    {
        if (changelevel + level_ > 100)
            changelevel = 100 - level_;
        if (relay__->getRelay(relayUp_) == OFF && relay__->getRelay(relayDown_) == OFF)
        {
            relay__->setOn(relayUp_);
            timeopen_ = millis() + 10 * (static_cast<unsigned long>(changelevel * opentimewindow_));
            level_ = constrain(level_ + changelevel, 0, 100);
        }
    }
    //включение механизма закрытия окна
    void closeWindow(int changelevel)
    {
        if (relay__->getRelay(relayUp_) == OFF && relay__->getRelay(relayDown_) == OFF)
        {
            relay__->setOn(relayDown_);
            timeclose_ = millis() + 10 * (static_cast<unsigned long>(changelevel * opentimewindow_));
            level_ = constrain(level_ - changelevel, 0, 100);
        }
    }

    //выключение механизма открытия и закрытия
    void off() const
    {
        if (timeopen_ < millis())
            relay__->setOff(relayUp_);
        if (timeclose_ < millis())
            relay__->setOff(relayDown_);
    }
    unsigned long getOpenTime() const
    {
        return timeopen_;
    }
    unsigned long getCloseTime() const
    {
        return timeclose_;
    }
};
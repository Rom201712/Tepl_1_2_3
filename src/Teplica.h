#pragma once

#include "Window.h"
#include "Heat.h"

class Teplica
{
private:
    int _mode, _id;
    int _oldtemperatute, _setpump, _setheat, _setwindow, _relayPump, _relayHeat, _relayUp, _relayDown, _channel_t;
    int _hysteresis = 20, _counter = 0;
    int32_t _correctiontemp, _temperature, _temperatureIntegral, _temperatureIntegralUp, _temperatureIntegralDown;
    bool _there_are_windows = true;
    unsigned long _time_air, _time_decrease_in_humidity, _time_close_window;
    const uint32_t _WAITING_ON_PUMP = 300000;
    const uint AIRLEVEL = 25;
    MB11016P_ESP *__relay = nullptr;
    // TypeMB16 *__relay = nullptr;
    MB1108A_ESP *__sensor = nullptr;
    Heat *__heat = nullptr;
    // Sensor *__sensor;
    Window *__window = nullptr;

public:
    // Ошибки датчиков температуры
    static const uint NO_ERROR = 1;
    static const uint NO_POWER = 0xffff;
    // конструктор для MB110_8A (блок датчиков) и MB110_16P (блок реле)
    Teplica(int id, int channel_t, int relayPump, int relayHeat, int relayUp, int relayDown,
            int setpump, int setheat, int setwindow, int opentimewindows, MB1108A_ESP *mb1108a, MB11016P_ESP *mb11016p, Heat *heat, bool there_are_windows = true)
        : _id(id), _setpump(setpump), _setheat(setheat), _channel_t(channel_t),
          _setwindow(setwindow), _relayPump(relayPump),
          _relayHeat(relayHeat), _relayUp(relayUp), _relayDown(relayDown), _there_are_windows(there_are_windows)
    {
        __window = new Window(mb11016p, relayUp, relayDown, opentimewindows);
        __relay = mb11016p;
        __sensor = mb1108a;
        __heat = heat;
        _oldtemperatute = setpump;
        _mode = AUTO;
    }
    // конструктор для MB110_8A (блок датчиков) и MB110_16P (блок реле)
    // номер температурного сенсора, номер релейного выхода, уставка, температурный модуль, релейный модульgas.getValve1(),
    // Teplica(int id, int channel_t, int relayPump, int relayHeat, int setpump, int setheat, MB1108A_ESP *mb1108a, MB11016P_ESP *mb11016p, Gas *gas) : _id(id), _channel_t(channel_t), _relayPump(relayPump), _relayHeat(relayHeat), _setpump(setpump), _setheat(setheat), _hysteresis(20)
    // {
    //     __sensor = mb1108a;
    //     __relay = mb11016p;
    //     __heat. = gas;
    // }
    enum mode
    {
        AUTO,                // автоматический режим
        MANUAL,              // ручной режим
        AIR,                 // проветривание
        DECREASE_IN_HUMIDITY // осушение
    };

    void regulationPump(int temperature) const
    {
        if (getMode() == AUTO)
        {
            // если теплица без окон
            if (!_there_are_windows)
            {
                // управление основным нагревателем
                if (_setpump - temperature > _hysteresis >> 1)
                    if (millis() > _time_close_window)
                    {
                        __relay->setOn(_relayPump);
                    }
                // управление дополнительным нагревателем
                if (_setheat - temperature > _hysteresis >> 1)
                {
                    __heat->setRelay(_relayHeat, ON);
                }
                // управление основным нагревателем
                if (temperature > _setpump)
                {
                    __relay->setOff(_relayPump);
                }
                // управление дополнительным нагревателем
                if (temperature > _setheat)
                {
                    __heat->setRelay(_relayHeat, OFF);
                }
            }
            // если теплица с окнами
            else if (getLevel() <= 10)
            {
                // управление включением нагревателей
                if (_setpump - temperature > _hysteresis >> 1)
                    if (millis() > _time_close_window)
                    {
                        __relay->setOn(_relayPump);
                    }
                // управление дополнительным нагревателем
                if (_setheat - temperature > _hysteresis >> 1)
                {
                    __heat->setRelay(_relayHeat, ON);
                }
            }
            // управление выключением нагревателей
            if (temperature > _setpump)
            {
                __relay->setOff(_relayPump);
            }
            // управление дополнительным нагревателем
            if (temperature > _setheat)
            {
                __heat->setRelay(_relayHeat, OFF);
            }
        }
    }
    // управление окнами
    void regulationWindow(int temperature, int outdoortemperature = 0)
    {
        if (_temperatureIntegral < 0 && temperature > _setwindow)
        {
            _temperatureIntegral = 0;
        }
        if (_temperatureIntegral > 0 && temperature < _setwindow)
        {
            _temperatureIntegral = 0;
        }
        if (getMode() == AUTO)
        {
            int Tout = 0;
            if (outdoortemperature)
                Tout = constrain(map(outdoortemperature - _setpump, -1000, 1000, -5, 10), -5, 10);
            _temperatureIntegral += temperature - _setwindow;
            _counter++;
            // условие открытия окон
            if (temperature > _setwindow + 500) // если очень жарко
            {
                __window->openWindow(30);
                _temperatureIntegral = 0;
                _counter = 0;
            }
            else if (_oldtemperatute <= temperature) // если температура не падает
            {
                if (temperature > _setwindow + 250 && __window->getlevel() < 20)
                {
                    __window->openWindow(20 + Tout);
                    _temperatureIntegral = 0;
                }
                else if (temperature > _setwindow + 50)
                    if (_temperatureIntegral > 100) //
                    {
                        __window->openWindow(constrain(map(_counter, 1, 16, 12, 5), 5, 15) + Tout);
                        _temperatureIntegral = 0;
                        _counter = 0;
                    }
            }
            // условие закрытия окон
            if (temperature <= _setpump + _hysteresis)
            {
                if (_oldtemperatute >= temperature) //
                {
                    if (__window->getlevel() > 30) // прикрытие  окон при достижении температуры уставки
                    {
                        __window->closeWindow(getLevel() / 2); //
                    }
                    else // закрытие окон при достижении температуры уставки
                    {
                        if (__window->getlevel() > 10)
                            _time_close_window = millis() + _WAITING_ON_PUMP; // задержка последующего пуска нагревателя
                        __window->closeWindow(getLevel() + 5);
                    }
                }
            }
            else if (getLevel())
            {
                if (temperature < _setwindow && _oldtemperatute - temperature > 150)
                {
                    __window->closeWindow(constrain(map(_counter, 1, 16, 15, 5), 5, 15) - Tout);
                    _temperatureIntegral = 0;
                    _counter = 0;
                }
                else if (temperature < _setwindow && _oldtemperatute >= temperature)
                    if (_temperatureIntegral < -100) //
                    {
                        __window->closeWindow(constrain(map(_counter, 1, 16, 15, 5), 5, 15) - Tout);
                        _temperatureIntegral = 0;
                        _counter = 0;
                    }
            }
        }
        // Serial.printf("Tepl %d, temper: %d\n", getAdressT(), temperature);
        // Serial.printf("Tepl %d, oldtemper: %d\n", getAdressT(), _oldtemperatute);
        // Serial.printf("Tepl %d, temperSet: %d\n", getAdressT(), _setwindow);
        // Serial.printf("Tepl %d, integral: %d\n", getAdressT(), _temperatureIntegral);
        // Serial.printf("Tepl %d, coun: %d\n", getAdressT(), _counter);
        // Serial.printf("Tepl %d, level: %d\n\n", getAdressT(), getLevel());
        _oldtemperatute = temperature;
    }

    // режим проветривания
    void air(unsigned long airtime = 900000, int outdoortemperature = 0)
    {
        if (!_there_are_windows)
        {
            return;
        }
        else if (__relay->getRelay(_relayUp) || __relay->getRelay(_relayDown))
            return;
        else if (getLevel() < AIRLEVEL)
        {
            _time_air = millis() + airtime;
            __window->openWindow(AIRLEVEL - getLevel());
        }
    }

    // режим осушения
    void decrease_in_humidity(unsigned long heattime = 900000, int outdoortemperature = 0)
    {
        if (!_there_are_windows)
        {
            return;
        }

        else if (__relay->getRelay(_relayUp) || __relay->getRelay(_relayDown))
            return;
        else if (getLevel() < AIRLEVEL)
        {
            _time_decrease_in_humidity = millis() + heattime;
            __relay->setOn(_relayPump);
            __window->openWindow(AIRLEVEL - getLevel());
        }
    }
    void updateWorkWindows()
    {
        __window->off();
        if (_mode == AIR && _time_air < millis())
        {
            _mode = AUTO;
        }
        if (_mode == DECREASE_IN_HUMIDITY && _time_decrease_in_humidity < millis())
        {
            _mode = AUTO;
        }
        if (_mode == AIR || _mode == DECREASE_IN_HUMIDITY)
            if (getLevel() < AIRLEVEL)
                __window->openWindow(AIRLEVEL - getLevel());
        alarm();
    }
    void alarm()
    {
        if (!_there_are_windows)
        {
            if (NO_ERROR == getSensorStatus())
            {
                if (getTemperature() <= _setheat)
                    _mode = AUTO;
                if (getTemperature() - _setwindow > 300)
                    _mode = AUTO;
            }
            return;
        }
        if (NO_ERROR == getSensorStatus())
            if (getTemperature() <= _setheat)
            {
                if (_mode != AUTO)
                {
                    _mode = AUTO;
                    __window->closeWindow(getLevel() + 5);
                }
            }
    }

    // установка уставки насос
    void setSetPump(int setpoint)
    {
        _setpump = setpoint;
    }
    // установка уставки доп.нагреватель
    void setSetHeat(int setpoint)
    {
        _setheat = setpoint;
    }

    int getSetPump()
    {
        return _setpump;
    }
    int getSetHeat()
    {
        return _setheat;
    }

    // установка гистерезиста насосов
    void setHysteresis(int hysteresis)
    {
        _hysteresis = hysteresis;
    }
    int getHysteresis()
    {
        return _hysteresis;
    }

    void setMode(int mode) 
    {
        _mode = mode;
        if (_mode == AIR)
            air();
        else if (_mode == DECREASE_IN_HUMIDITY)
            decrease_in_humidity();
    }

    int getMode() const
    {
        return _mode;
    }
    int getTemperature()
    {
        return _temperature;
    }
    void setTemperature()
    {
        _temperature = __sensor->getData(1 + 6 * _channel_t) + _correctiontemp;
    }

    int getSensorStatus()
    {
        int answer = __sensor->getData(2 + 6 * _channel_t);
        if (0 == answer)
            answer = 1;
        return answer;
    }

    int getPump()
    {
        return __relay->getRelay(_relayPump);
    }
    void setPump(int vol)
    {
        vol ? __relay->setOn(_relayPump) : __relay->setOff(_relayPump);
    }
    int getHeat()
    {
        return __heat->getStatusRelay(_relayHeat);
    }

    void setHeat(int vol)
    {
        __heat->setRelay(_relayHeat, vol);
    }

    void setAdress(int channel_t)
    {
        _channel_t = channel_t;
    }
    void setCorrectionTemp(int correctiontemp)
    {
        _correctiontemp = correctiontemp;
    }
    int getId()
    {
        return _id;
    }

    int getLevel() const
    {
        if (!_there_are_windows)
            return 0;
        return __window->getlevel();
    }

    // установка уставки окна
    void setSetWindow(int setpoint)
    {
        _temperatureIntegral = 0;
        _setwindow = setpoint;
        _oldtemperatute = getTemperature();
    }

    void setOpenTimeWindow(int time)
    {
        if (!_there_are_windows)
            return;
        __window->setopentimewindow(time);
    }

    bool getThereAreWindows()
    {
        return _there_are_windows;
    }
    // установка уровня окон
    void setWindowlevel(int changelevel) const
    {
        if (!_there_are_windows)
        {
            return;
        }
        changelevel -= __window->getlevel();
        if (changelevel > 0)
            __window->openWindow(changelevel);
        if (changelevel < 0)
            __window->closeWindow(-changelevel);
    }

    int getSetWindow()
    {
        return _setwindow;
    }

    int getOpenTimeWindow() const
    {
        if (!_there_are_windows)
            return 0;
        return __window->getOpenTimeWindow();
    }
};
// релейный модуль
// #include <Arduino.h>
#pragma once

#include <ModbusRTU.h>

bool cbWrite_MB16(Modbus::ResultCode event, uint16_t transactionId, void *data)
{
    // Serial.printf("MB110-16P result:\t0x%02X, Mem: %d\n", event, ESP.getFreeHeap());
    // &_mb11016[16] = event;
    return true;
}

class MB11016P_ESP
{
private:
    uint8_t _netadress; // адрес устройства в сети Modbus)
    ModbusRTU *__mb11016p;
    bool _mb11016[17] = {};

public:
    MB11016P_ESP(ModbusRTU *master, uint8_t netadress, bool *mb11016) : _netadress(netadress), __mb11016p(master)
    {
        *_mb11016 = mb11016;
        for (int i = 0; i < 17; i++)
            setOff(i);
    }

    // запись в блок МВ110-16Р (включение реле управления)
    void write()
    {
        
        __mb11016p->writeCoil(_netadress, 0, _mb11016, 16, cbWrite_MB16);
    }

    void setOn(int relay)
    {
        _mb11016[relay] = 1;
    }

    void setOff(int relay)
    {
        _mb11016[relay] = 0;
    }

    bool getRelay(int relay)
    {
        return _mb11016[relay];
    }

    bool getError()
    {
        return _mb11016[16];
    }

    void setAdress(int adr)
    {
        _netadress = adr;
    }
};

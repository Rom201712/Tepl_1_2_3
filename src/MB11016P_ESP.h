// релейный модуль
#include <ModbusRTU.h>

namespace MB11016P
{
    int _errmodbus{}; 
    bool cbWrite(Modbus::ResultCode event, uint16_t transactionId, void *data)
    {
        // Serial.printf("MB110-16P result:\t0x%02X, Mem: %d\n", event, ESP.getFreeHeap());
        _errmodbus = event;
        return true;
    }
};



class MB11016P_ESP
{
private:
    uint8_t _netadress{}; // адрес устройства в сети Modbus)
    int _quantity{};
    static const int LENG_MB = 17;
    bool _mb11016[LENG_MB] = {};
    ModbusRTU *__mb11016p;

public:
    MB11016P_ESP(ModbusRTU *master, uint8_t netadress, int quantity) : _netadress(netadress), _quantity(quantity), __mb11016p(master)
    {
    }

    // запись в блок МВ110-16Р (включение реле управления)
    void write()
    {
        __mb11016p->writeCoil(_netadress, 0, _mb11016, LENG_MB - 1, MB11016P::cbWrite);
        while (__mb11016p->slave())
        {
            __mb11016p->task();
            delay(10);
        }
        _mb11016[LENG_MB - 1] = MB11016P::_errmodbus;
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
        return _mb11016[LENG_MB - 1];
    }
    void setAdress(int adr)
    {
        _netadress = adr;
    }
};
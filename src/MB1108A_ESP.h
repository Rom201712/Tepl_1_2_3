#pragma once

#include <ModbusRTU.h>

uint16_t _mb1108[49] = {};
bool cbRead(Modbus::ResultCode event, uint16_t transactionId, void *data)
{
    // Serial.printf("MB110-8A result:\t0x%02X, Mem: %d\n", event, ESP.getFreeHeap());
    // for (auto d : _mb1108)
    //     Serial.printf(" %d", d);
    // Serial.println();
    _mb1108[48] = event;
    return true;
}

class MB1108A_ESP
{
    uint8_t _netadress; // адрес устройства в сети Modbus)
    int _quantity;
    ModbusRTU *__mb11008a;

public:
    MB1108A_ESP(ModbusRTU *master, int netadress, int quantity) : _netadress(netadress), _quantity(quantity), __mb11008a(master)
    {
    }

    void read()
    {
        __mb11008a->readIreg(_netadress, 0, _mb1108, _quantity * 6, cbRead);
    }

    int getErrorMB1108A()
    {
        return _mb1108[48];
    }

    uint getData(int index)
    {
        return _mb1108[index];
    }
    void setAdress(int adr)
    {
        _netadress = adr;
    }
    ~MB1108A_ESP(){};
};
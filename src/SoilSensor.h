
#pragma once
#include <Arduino.h>
#include <ModbusRTU.h>

// #include <vector>
namespace Soil
{
    const int LENG_MB = 6;
    uint16_t _mb[LENG_MB]{};
    enum Sensor_Data
    {
        humidity,     //  влажность почвы
        temperature,  //  температура почвы
        conductivity, //  EC почвы
        salinity,     //  соленость почвы
        TDS,
        status
    };
    bool cbRead(Modbus::ResultCode event, uint16_t transactionId, void *data)
    {
        // Serial.printf("SoilSensor result:\t0x%02X, Mem: %d\n", event, ESP.getFreeHeap());
        // for (auto d : Soil::_mb)
        //     Serial.printf(" %d", d);
        // Serial.println();
        Soil::_mb[LENG_MB - 1] = event;
        return true;
    }
};

class SoilSensor
{
    int _netadress; // id устройства в массиве Modbus
    int _datasensor[Soil::LENG_MB-1] = {0xffff, 0xffff, 0xffff, 0xffff, 0xffff};
    std::vector<int> _tempvec;
    ModbusRTU *_mb_sensor;

    uint32_t _baudRate = 9600;
    // std::vector<int> _humvec;

public:
    static const uint NO_ERROR = 1;
    static const uint NO_POWER = 0xffff;
    SoilSensor() = default;
    SoilSensor(ModbusRTU *mb_sensor, int adress, uint32_t baudRate = 9600) : _netadress(adress), _mb_sensor(mb_sensor), _baudRate(baudRate) {
    }
    
    int getStatus()
    {
        return Soil::_mb[Soil::Sensor_Data::status];
    }
    int getTemperature()
    {
        return _datasensor[Soil::temperature];
    }
    int getHumidity()
    {
        return _datasensor[Soil::humidity];
    }
    int getConductivity()
    {
        return _datasensor[Soil::conductivity];
    }
    int getSalinity()
    {
        return _datasensor[Soil::salinity];
    }
    int getTDS()
    {
        return _datasensor[Soil::TDS];
    }
    void setAdress(int adr)
    {
        _netadress = adr;
    }

    int getAdress()
    {
        return _netadress;
    }
    int getAverageTemperature()
    {
        int32_t T = 0;
        for (int i = 0; i < _tempvec.size(); i++)
        {
            T += _tempvec[i];
        }
        if (!_tempvec.empty())
            T /= _tempvec.size();
        _tempvec.clear();
        return T;
    }

    void read()
    {
        _mb_sensor->readHreg(_netadress, 0, Soil::_mb, 6, Soil::cbRead);

        while (_mb_sensor->slave())
        {
            _mb_sensor->task();
            delay(10);
        }

        if (Soil::_mb[Soil::Sensor_Data::status] == Modbus::EX_SUCCESS)
        {
            _datasensor[Soil::temperature] = Soil::_mb[Soil::temperature];
            _datasensor[Soil::humidity] = Soil::_mb[Soil::humidity];
            _datasensor[Soil::salinity] = Soil::_mb[Soil::salinity];
            _datasensor[Soil::conductivity] = Soil::_mb[Soil::conductivity];
            // Serial.printf("Температура %d - %d\n", getAdress(), getTemperature());
            // Serial.printf("Сатус %d - %d\n", getAdress(), getStatus());
        }
    }
};
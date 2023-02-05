#pragma once

#include <Arduino.h>
#include <string>

#include "SoftwareSerial.h"

class Nextion
{
private:
    SoftwareSerial *serialNextion_;

public:
    Nextion(SoftwareSerial &port) : serialNextion_(&port) {}

    void operator()(String data) const
    {
        send_(data);
    }

    void operator()(String dev, double data) const
    {
        send_(dev, data);
    }

    void operator()(String dev, String data) const
    {
        send_(dev, data);
    }

private:
    // отправка на Nextion
    void send_(String dev) const
    {
        serialNextion_->print(dev); // Отправляем данные dev(номер экрана, название переменной) на Nextion
        send_End();
    }
    void send_(String dev, double data) const
    {
        serialNextion_->print(dev + "=");
        serialNextion_->print(data, 0);
        send_End();
    }
    void send_(String dev, String data) const
    {
        serialNextion_->print(dev + "=\"");
        serialNextion_->print(data + "\"");
        send_End();
    }
    void send_End() const
    {
        serialNextion_->write(0xff);
        serialNextion_->write(0xff);
        serialNextion_->write(0xff);
    }
};
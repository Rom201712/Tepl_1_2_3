#pragma once

#include <Arduino.h>
#include <string>

#include "SoftwareSerial.h"
#include "Teplica.h"
#include "MB1108A_ESP.h"
#include "Heat.h"

#define CMD_READ_TIMEOUT 50
#define READ_TIMEOUT 50

#define MIN_ASCII 32
#define MAX_ASCII 255

class Nextion
{
private:
    SoftwareSerial *_nextionSerial;
    bool _echo; // Response Command Show
    // Callback Function
    typedef void (*hmiListner)(String messege, String date, String response);
    hmiListner listnerCallback;
    SemaphoreHandle_t nexton_mutex;

    // цвет на экране
    const uint LIGHT = 57048;
    const uint RED = 55688;
    const uint GREEN = 2016;
    const uint BLUE = 1566;

public:
    Nextion(SoftwareSerial &port) : _nextionSerial(&port)
    {
        // nexton_mutex = xSemaphoreCreateMutex();
    }

    void echoEnabled(bool echoEnabled)
    {
        _echo = echoEnabled;
    }

    // SET CallBack Event
    void hmiCallBack(hmiListner callBack)
    {
        listnerCallback = callBack;
    }

    void operator()(const String &data) const
    {
        send(data);
    }

    void operator()(const String &dev, const double data) const
    {
        send(dev, data);
    }

    void operator()(const String &dev, const String &data) const
    {
        send(dev, data);
    }

    // Listen For incoming callback event from HMI
    void listen()
    {
        // xSemaphoreTake(nexton_mutex, portMAX_DELAY);
        handle();
        // xSemaphoreGive(nexton_mutex);
    }

    // вывод данных теплицы 1
    void inditepl1(Teplica &tepl, Heat &heat, MB1108A_ESP &mb1108a)
    {
        if (!mb1108a.getErrorMB1108A())
        { // ввывод температуры и ошибок датчика температуры
            if (1 == tepl.getSensorStatus())
            {
                send("p0.t0.font", 5);
                send("p0.t0.txt", String(tepl.getTemperature() / 100.0, 1));
                if (tepl.getTemperature() < tepl.getSetHeat())
                    send("p0.t0.pco", BLUE);
                else if (tepl.getTemperature() > tepl.getSetPump() + 300)
                    send("p0.t0.pco", RED);
                else
                    send("p0.t0.pco", GREEN);
            }
            else
            {
                send("p0.t0.pco", LIGHT);
                send("p0.t0.font", 1);
                send("p0.t0.txt", "Er " + String(tepl.getSensorStatus(), HEX));
            }
        }
        else
        {
            send("p0.t0.pco", RED);
            send("p0.t0.font", 1);
            send("p0.t0.txt", "Er 108");
        }
        // вывод уставки насос
        send("p0.x1.val", tepl.getSetPump() / 10);
        // вывод уставки дополнительный обогреватель
        send("p0.x2.val", tepl.getSetHeat() / 10);
        // вывод уставки окно
        send("p0.x3.val", tepl.getSetWindow() / 10);
        // вывод режима работы
        if (tepl.getMode() == Teplica::MANUAL)
            send("p0.t3.txt", "M");
        if (tepl.getMode() == Teplica::AUTO)
            send("p0.t3.txt", "A");
        if (tepl.getMode() == Teplica::AIR)
            send("p0.t3.txt", "W");
        if (tepl.getMode() == Teplica::DECREASE_IN_HUMIDITY)
            send("p0.t3.txt", "H");
        // идикация состояния насоса
        send("p0.p1.pic", tepl.getPump() ? 12 : 11);
        // идикация состояния дополнительного обогревателя
        send("p0.p2.pic", tepl.getHeat() && heat.getSatusHeat() ? 12 : 11);
        // вывод уровня открытия окон
        send("p0.h0.val", tepl.getLevel());
    }

    // вывод данных теплицы 2
    void inditepl2(Teplica &tepl, Heat &heat, MB1108A_ESP &mb1108a)
    {
        if (!mb1108a.getErrorMB1108A())
        {
            // ввывод температуры и ошибок датчика температуры
            if (1 == tepl.getSensorStatus())
            {
                send("p0.t7.font", 5);
                send("p0.t7.txt", String(tepl.getTemperature() / 100.0, 1));
                if (tepl.getTemperature() < tepl.getSetHeat())
                    send("p0.t7.pco", BLUE);
                else if (tepl.getTemperature() > tepl.getSetPump() + 300)
                    send("p0.t7.pco", RED);
                else
                    send("p0.t7.pco", GREEN);
            }
            else
            {
                send("p0.t7.pco", LIGHT);
                send("p0.t7.font", 1);
                send("p0.t7.txt", "Er " + String(tepl.getSensorStatus(), HEX));
            }
        }
        else
        {
            send("p0.t7.pco", RED);
            send("p0.t7.font", 1);
            send("p0.t7.txt", "Er 108");
        }
        // ввывод уставки насос
        send("p0.x4.val", tepl.getSetPump() / 10);
        // ввывод уставки дополнительный обогреватель
        send("p0.x6.val", tepl.getSetHeat() / 10);
        // вывод уставки окно
        send("p0.x7.val", tepl.getSetWindow() / 10);
        // ввывод режима работы
        if (tepl.getMode() == Teplica::MANUAL)
            send("p0.t4.txt", "M");
        if (tepl.getMode() == Teplica::AUTO)
            send("p0.t4.txt", "A");
        if (tepl.getMode() == Teplica::AIR)
            send("p0.t4.txt", "W");
        if (tepl.getMode() == Teplica::DECREASE_IN_HUMIDITY)
            send("p0.t4.txt", "H");
        // идикация состояния насоса
        send("p0.p3.pic", tepl.getPump() ? 12 : 11);
        // идикация состояния дополнительного обогревателя (задвижка)
        send("p0.p4.pic", tepl.getHeat() && heat.getSatusHeat() ? 12 : 11);
        // вывод уровня открытия окон
        send("p0.h1.val", tepl.getLevel());
    }

    // вывод данных теплицы 3
    void inditepl3(Teplica &tepl, Heat &heat, MB1108A_ESP &mb1108a)
    {
        if (!mb1108a.getErrorMB1108A())
        {
            // ввывод температуры и ошибок датчика температуры
            if (1 == tepl.getSensorStatus())
            {
                send("p0.t8.font", 5);
                send("p0.t8.txt", String(tepl.getTemperature() / 100.0, 1));
                if (tepl.getTemperature() < tepl.getSetHeat())
                    send("p0.t8.pco", BLUE);
                else if (tepl.getTemperature() > tepl.getSetPump() + 300)
                    send("p0.t8.pco", RED);
                else
                    send("p0.t8.pco", GREEN);
            }
            else
            {
                send("p0.t8.pco", LIGHT);
                send("p0.t8.font", 1);
                send("p0.t8.txt", "Er " + String(tepl.getSensorStatus(), HEX));
            }
        }
        else
        {
            send("p0.t8.pco", RED);
            send("p0.t8.font", 1);
            send("p0.t8.txt", "Er 108");
        }
        // ввывод уставки насос
        send("p0.x8.val", tepl.getSetPump() / 10);
        // ввывод уставки дополнительный обогреватель
        send("p0.x10.val", tepl.getSetHeat() / 10);
        // вывод уставки окно
        send("p0.x11.val", tepl.getSetWindow() / 10);
        // ввывод режима работы
        if (tepl.getMode() == Teplica::MANUAL)
            send("p0.t5.txt", "M");
        if (tepl.getMode() == Teplica::AUTO)
            send("p0.t5.txt", "A");
        if (tepl.getMode() == Teplica::AIR)
            send("p0.t5.txt", "W");
        if (tepl.getMode() == Teplica::DECREASE_IN_HUMIDITY)
            send("p0.t5.txt", "H");
        // индикация состояния насоса
        send("p0.p5.pic", tepl.getPump() ? 12 : 11);
        // индикация состояния дополнительного обогревателя (задвижка)
        send("p0.p6.pic", tepl.getHeat() && heat.getSatusHeat() ? 12 : 11);
        // вывод уровня открытия окон
        send("p0.h2.val", tepl.getLevel());
    }

    void hmi_p1(Teplica &tepl, String &err, int count)
    {
        if (count < 5)
        {
            send("p1.x0.val", tepl.getSetPump() / 10);
            send("p1.x4.val", tepl.getSetHeat() / 10);
            send("p1.x1.val", tepl.getHysteresis() / 10);
            send("p1.x2.val", tepl.getOpenTimeWindow());
            send("p1.x3.val", tepl.getSetWindow() / 10);
            send("p1.h0.val", tepl.getLevel());
            send("p1.h1.val", tepl.getHysteresis());
            send("p1.h2.val", tepl.getOpenTimeWindow());
            send("p1.n0.val", tepl.getLevel());
            send("g0.txt", err);
        }

        send("b3.picc", tepl.getPump() ? 2 : 1);

        switch (tepl.getMode())
        {
        case Teplica::MANUAL:
            send("b0.picc", 2);
            send("b1.picc", 1);
            send("b2.picc", 1);
            break;
        case Teplica::AUTO:
            send("b0.picc", 1);
            send("b1.picc", 1);
            send("b2.picc", 1);
            break;
        case Teplica::AIR:
            send("b0.picc", 1);
            send("b1.picc", 2);
            send("b2.picc", 1);
            break;
        case Teplica::DECREASE_IN_HUMIDITY:
            send("b0.picc", 1);
            send("b1.picc", 1);
            send("b2.picc", 2);
            break;
        default:
            break;
        }
    }

    void page2(int sspeed, int mspeed)
    {
    }

    void page3(MB1108A_ESP &water_temper, bool flag)
    {
    }

private:
    // отправка на Nextion
    void send(const String &dev) const
    {
        _nextionSerial->print(dev); // Отправляем данные dev(номер экрана, название переменной) на Nextion
        sendEnd();
    }
    void send(const String &dev, const double data) const
    {
        _nextionSerial->print(dev + "=");
        _nextionSerial->print(data, 0);
        sendEnd();
    }
    void send(const String &dev, const String &data) const
    {
        _nextionSerial->print(dev + "=\"");
        _nextionSerial->print(data + "\"");
        sendEnd();
    }
    void sendEnd() const
    {
        _nextionSerial->write(0xff);
        _nextionSerial->write(0xff);
        _nextionSerial->write(0xff);
    }

    String checkHex(byte currentNo)
    {
        if (currentNo < 10)
        {
            return "0x" + String(currentNo, HEX);
        }
        return String(currentNo, HEX);
    }
    void handle()
    {
        String response;
        String messege;
        String date;
        int count_ff = 0;
        bool charEquals = false;
        unsigned long startTime = millis();
        ((SoftwareSerial *)_nextionSerial)->listen(); // Start software serial listen

        while (_nextionSerial->available())
        {
            int inc = _nextionSerial->read();
            response.concat(checkHex(inc) + " ");
            if (inc == 0x23)
            {
                messege.clear();
                date.clear();
                response.clear();
                charEquals = false;
            }
            else if (inc == 0xff)
            {
                // response.clear();
                count_ff++;
            }
            else if (inc == 0x66)
            {
                delay(3);
                inc = _nextionSerial->read();
                response.concat(checkHex(inc) + " ");
                messege += String(inc, DEC);
            }
            else
            {
                if (inc <= MAX_ASCII && inc >= MIN_ASCII)
                {
                    if (!charEquals)
                    {
                        if (char(inc) == '=')
                        {
                            charEquals = true;
                            date.clear();
                        }
                        else
                        {
                            messege += char(inc);
                        }
                    }
                    else
                    {
                        date += char(inc);
                    }
                }
            }
            if (count_ff == 3 || inc == 0x0A)
            {
                if (_echo)
                {
                    Serial.println("\nOnEvent : [ M : " + messege + " | D : " + date + " | R : " + response + " ]");
                }
                listnerCallback(messege, date, response);
                messege.clear();
                response.clear();
                date.clear();
                count_ff = 0;
                charEquals = false;
            }
            delay(3);

            // if (millis() - startTime > CMD_READ_TIMEOUT)
            //     return;
        }
    }

    /*
    String handle()
    {
        String response;
        String messege;
        String date;
        bool charEquals = false;
        unsigned long startTime = millis();
        // ((SoftwareSerial *)_nextionSerial)->listen(); // Start software serial listen

        while ((millis() - startTime < CMD_READ_TIMEOUT))
        {
            while (_nextionSerial->available())
            {
                int inc = _nextionSerial->read();
                response.concat(checkHex(inc) + " ");
                if (inc == 0x23)
                {
                    messege.clear();
                    date.clear();
                    // response.clear();
                    charEquals = false;
                }
                else if (inc == 0x0A)
                {
                    if (_echo)
                    {
                        Serial.println("OnEvent : [ M : " + messege + " | D : " + date + " | R : " + response + " ]");
                    }

                    listnerCallback(messege, date, response);
                    messege.clear();
                    date.clear();
                    charEquals = false;
                }
                else
                {
                    if (inc < MAX_ASCII && inc > MIN_ASCII)
                    {
                        if (!charEquals)
                        {
                            if (char(inc) == '=')
                            {
                                charEquals = true;
                                date.clear();
                            }
                            else
                            {
                                messege += char(inc);
                            }
                        }
                        else
                        {
                            date += char(inc);
                        }
                    }
                    else
                    {
                        messege.clear();
                        date.clear();
                        charEquals = false;
                    }
                }
                delay(3);
            }
            return response;
        }
        return {};
    }

    byte readCMDLastByte()
    {
        //* This has to only be enabled for Software serial
        ((SoftwareSerial *)_nextionSerial)->listen(); // Start software serial listen

        byte lastByte = -1;
        unsigned long startTime = millis(); // Start time for Timeout
        while ((millis() - startTime < CMD_READ_TIMEOUT))
        {
            while (_nextionSerial->available() > 0)
            {
                lastByte = _nextionSerial->read();
                delay(10);
            }
        }
        return lastByte;
    }

    void flushSerial()
    {
        Serial.flush();
        _nextionSerial->flush();
    }
    */
};
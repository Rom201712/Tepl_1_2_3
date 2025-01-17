#pragma once

#define ON HIGH
#define OFF LOW

#include <WiFi.h>
#include <SoftwareSerial.h>
#include <Bounce2.h>
#include <ModbusRTU.h>
#include <ModbusIP_ESP8266.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include "Preferences.h"
#include <AsyncElegantOTA.h>
#include <freertos/task.h>
#include <FreeRTOSConfig.h>

#include "MB11016P_ESP.h"
#include "MB1108A_ESP.h"
#include "heat.h"
#include "Teplica.h"
#include "NEXTION.h"
#include "SoilSensor.h"

Preferences flash;
AsyncWebServer server(80);

#define RXDNEX 23 // плата в теплице
#define TXDNEX 22 // плата в теплице
// static const int RXDNEX = 19; // тестовая плата
// static const int TXDNEX = 23; // тестовая плата

#define RXDMASTER 25
#define TXDMASTER 33
#define MbMasterSerial Serial1
#define MbSlaveSerial Serial2

// цвет на экране
const double LIGHT = 57048;
const double RED = 55688;
const double GREEN = 2016;
const double BLUE = 1566;

#define DEBUG_WIFI

const String VER = "Ver - 3.0. Date - " + String(__DATE__) + "\r";
int IDSLAVE = 13; // адрес в сети Modbus

String ssid = "yastrebovka";
String password = "zerNo32_";
// String ssid = "Home-RP";
// String password = "12rp1974";

const uint32_t AIRTIME = 1200000; // длительность проветривания и осушения

const uint TIME_UPDATE_GREENOOUSE = 4;     // период регулировки окон теплиц, мин
const uint TIME_UPDATE_MODBUS_MB110 = 5;   // период обновления данных для блока реле, сек
const uint TIME_UPDATE_MODBUS_SENSOR = 20; // период обновления данных от датчиков, сек
const uint TIME_UPDATE_WATER_SENSOR = 300;   // период обновления данных от датчиков температуры теплоносителя, сек
const uint TIME_UPDATE_HMI = 200;         // период обновления данных на дисплее, мсек

SoftwareSerial SerialNextion;

enum
{
  rs485mode1 = 0,          //  1 - 1 - режим работы теплица1, 3 - насос1, 1 - доп. нагреватель1, 9-16 - уставка теплица1
  rs485mode11,             //  2 - 1-8 - уровень открытия окна теплица1, 9-16 сдвиг уставки для окон
  rs485mode2,              //  3 - 1 - режим работы теплица2, 3 - насос2, 1 - доп. нагреватель2, 9-16 - уставка теплица2
  rs485mode21,             //  1 - 1-8 - уровень открытия окна теплица2,  9-16 сдвиг уставки для окон
  rs485mode3,              //  2 - 1 - режим работы теплица3, 3 - насос3, 1 - доп. нагреватель3, 9-16 - уставка теплица3
  rs485mode31,             //  3 - 1-8 - уровень открытия окна теплица3,  9-16 сдвиг уставки для окон
  rs485mode7,              //  _ - 1 - режим работы теплица_, 3 - насос_, 1 - доп. нагреватель_, 9-16 - уставка теплица_
  rs485mode71,             //  8 - 1-8 - уровень открытия окна теплица_,  9-16 сдвиг уставки для окон
  rs485settings,           //  9 - 1 - наличие датчика дождя, 2 - наличие датчика температуры наружного воздуха, 3-13 - время открытия окна теплица4 (сек), 14-16 - величина гистерезиса включения насосов (гр.С)
  rs485temperature1,       //  10 - температура в теплиц1
  rs485temperature2,       //  11 - температура в теплиц2
  rs485temperature3,       //  12 - температура в теплиц3
  rs485temperature7,       //  13 - температура в теплиц_
  rs485temperatureoutdoor, //  14 - температура на улице
  rs485humidity45,         //  15 - 1-8 влажность теплица1, 9-16 влажность теплица2
  rs485humidity67,         //  16 - 1-8 влажность теплица3, 9-16 влажность на улице
  rs485humidityoutdoor,    //  17 - 1-8 влажность теплица_
  rs485error45,            //  18 - 1-8 ошибки теплица1 , 9-16 ошибки теплица2
  rs485error67,            //  29 - 1-8 ошибки теплица3 , 9-16 ошибки теплица_
  rs485error8,             //  20 - ошибки температура на улице
  rs485rain,               //  21 - 1-8 - показания датчика дождя, 9-16 ошибки датчик дождя
  test,
  rs485_HOLDING_REGS_SIZE //  leave this one
};

enum
{
  WiFimode1 = 100,     //  режим работы теплица 1
  WiFipump1,           //  насос 1
  WiFiheat1,           //  доп. нагреватель 1
  WiFisetpump1,        //  уставка теплица 1
  WiFisetheat1,        //  уставка доп.нагрветель 1
  WiFisetwindow1,      //  уставка окна теплица 1
  WiFitemperature1,    //  температура в теплиц 1
  WiFihumidity1,       //  влажность теплица 1
  WiFierror1,          //  ошибки теплица 1
  WiFiLevel1,          //  уровень открытия окна теплица 1
  WiFiHysteresis1,     // гистерезис насосов теплица 1
  WiFiOpenTimeWindow1, // время открытия окон теплица 1
  WiFiSoilSensorT1,    //  температура почвы 1
  WiFiSoilSensorH1,    //  влажность почвы 1
  WiFiSoilSensorC1,    //  Conductivity(EC) проводимость почвы 1
  WiFiSoilSensorS1,   //  salinity соленость почвы 1
  WiFiSoilSensorTDS1,    //  TDS почвы 1


  WiFimode2,          //  режим работы теплица 2
  WiFipump2,          //  насос 2
  WiFiheat2,          //  доп. нагреватель 2
  WiFisetpump2,       //  уставка теплица 2
  WiFisetheat2,       //  уставка доп.нагрветель 2
  WiFisetwindow2,     //  уставка окна теплица 2
  WiFitemperature2,   //  температура в теплиц 2
  WiFihumidity2,      //  влажность теплица 2
  WiFierror2,         //  ошибки теплица 2
  WiFiLevel2,         //  уровень открытия окна теплица 2
  WiFiHysteresis2,    // гистерезис насосов теплица 2
  WiFiOpenTimeWindow, // время открытия окон теплица 2
  WiFiSoilSensorT2,    //  температура почвы 2
  WiFiSoilSensorH2,    //  влажность почвы 2
  WiFiSoilSensorC2,    //  Conductivity(EC) проводимость почвы 2
  WiFiSoilSensorS2,   //  salinity соленость почвы 2
  WiFiSoilSensorTDS2,    //  TDS почвы 2

  WiFimode3,           //  режим работы теплица 3
  WiFipump3,           //  насос 3
  WiFiheat3,           //  доп. нагреватель 3
  WiFisetpump3,        //  уставка теплица 3
  WiFisetheat3,        //  уставка доп.нагрветель 3
  WiFisetwindow3,      //  уставка окна теплица 3
  WiFitemperature3,    //  температура в теплиц 3
  WiFihumidity3,       //  влажность теплица 3
  WiFierror3,          //  ошибки теплица 3
  WiFiLevel3,          //  уровень открытия окна теплица 3
  WiFiHysteresis3,     // гистерезис насосов теплица 3
  WiFiOpenTimeWindow3, // время открытия окон теплица 3
  WiFiSoilSensorT3,    //  температура почвы 2
  WiFiSoilSensorH3,    //  влажность почвы 2
  WiFiSoilSensorC3,    //  Conductivity(EC) проводимость почвы 2
  WiFiSoilSensorS3,   //  salinity соленость почвы 2
  WiFiSoilSensorTDS3,    //  TDS почвы 2

  WiFi_HOLDING_REGS_SIZE //  leave this one
};

enum
{
  wifi_flag_edit_1 = 300,
  wifi_UstavkaPump_1,
  wifi_UstavkaHeat_1,
  wifi_UstavkaWin_1,
  wifi_setWindow_1,
  wifi_mode_1,
  wifi_pump_1,
  wifi_heat_1,
  wifi_hysteresis_1,
  wifi_time_open_windows_1,
  res48,
  wifi_flag_edit_2,
  wifi_UstavkaPump_2,
  wifi_UstavkaHeat_2,
  wifi_UstavkaWin_2,
  wifi_setWindow_2,
  wifi_mode_2,
  wifi_pump_2,
  wifi_heat_2,
  wifi_hysteresis_2,
  wifi_time_open_windows_2,
  res58,
  wifi_flag_edit_3,
  wifi_UstavkaPump_3,
  wifi_UstavkaHeat_3,
  wifi_UstavkaWin_3,
  wifi_setWindow_3,
  wifi_mode_3,
  wifi_pump_3,
  wifi_heat_3,
  wifi_hysteresis_3,
  wifi_time_open_windows_3,
  res68,
  wifi_flag_edit__,
  wifi_UstavkaPump__,
  wifi_UstavkaHeat__,
  wifi_UstavkaWin__,
  wifi_setWindow__,
  wifi_mode__,
  wifi_pump__,
  wifi_heat__,
  wifi_hysteresis__,
  wifi_time_open_windows__,
  res78,
  wifi_HOLDING_REGS_SIZE //  leave this one
};

int modbusdateWiFi[WiFi_HOLDING_REGS_SIZE];
unsigned long timesendnextion;
const unsigned long TIME_UPDATE_MODBUS = 1000;
long updateNextion;
String pageNextion = "p0";
int counterMBRead = 0;
int coun1 = 0;
int arr_adr[12];
int arr_set[8];

TaskHandle_t Task_updateGreenHouse;
TaskHandle_t Task_updateDateSensor;
TaskHandle_t Task_webSerialSend;

ModbusRTU slave;
ModbusIP slaveWiFi;
ModbusRTU mb_master;

MB11016P_ESP mb11016p = MB11016P_ESP(&mb_master, 100, 0);
MB11016P_ESP mbsl8di8ro = MB11016P_ESP(&mb_master, 102, 0); // китайский блок реле (для управления пушкой и тепл.3)

MB1108A_ESP mb1108a = MB1108A_ESP(&mb_master, 101, 3);
Heat heat = Heat(0, 1, 2, 3, &mbsl8di8ro);

SoilSensor soil1(&mb_master, 1, 9600);

Teplica Tepl1 = Teplica(1, 0, 0, heat.getValve1(), 1, 2, 900, 700, 11000, 60000, &mb1108a, &mb11016p, &heat);
Teplica Tepl2 = Teplica(2, 1, 4, heat.getValve2(), 5, 6, 900, 700, 11000, 60000, &mb1108a, &mb11016p, &heat);
Teplica Tepl3 = Teplica(3, 2, 4, heat.getValve3(), 5, 6, 900, 700, 11000, 60000, &mb1108a, &mbsl8di8ro, &heat);

Teplica *arr_Tepl[3];

Nextion hmi(SerialNextion);

// void pageNextion_p0();
void pageNextion_p1(int i);
void pageNextion_p2();
void pageNextion_p3();
void indiTepl1();
void indiTepl2();
void indiTepl3();
void indiGas();
// void indiOutDoor();
// int indiRain();
void pars_str_adr(String &str);
void pars_str_set(String &str);
void saveOutModBusArr();
void controlScada();
String calculateTimeWork();

void updateGreenHouse(void *pvParameters);
void updateMB(void *pvParameters);
void update_WiFiConnect(void *pvParameters);
void sendNextion(void *pvParameters);
void readNextion(void *pvParameters);
void onHMIEvent(String messege, String data, String response);


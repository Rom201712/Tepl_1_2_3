
#include "main.h"
#include <TimeUtil.h>

void setup()
{
  pinMode(GPIO_NUM_2, OUTPUT);

  Serial.begin(115200);
  Serial.print(__DATE__);

  displNext("rest");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  WiFi.setHostname("Tepl_1_2_3_windows");
  Serial.printf("\nConnecting to WiFi..\n");
  int counter_WiFi = 0;

  while (WiFi.status() != WL_CONNECTED && counter_WiFi < 10)
  {
    delay(1000);
    counter_WiFi++;
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    String ssid = "yastrebovka";
    String password = "zerNo32_";
    WiFi.begin(ssid.c_str(), password.c_str());
    counter_WiFi = 0;
    while (WiFi.status() != WL_CONNECTED && counter_WiFi < 10)
    {
      delay(1000);
      counter_WiFi++;
    }
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Greenhaus #1, #2, #3 - windows.\n" + VER + calculateTimeWork() + "\nRSSI: " + String(WiFi.RSSI())); });

  AsyncElegantOTA.begin(&server); // Start ElegantOTA
  server.begin();
#ifdef USE_WEB_SERIAL
  WebSerial.begin(&server);
  WebSerial.msgCallback(recvMsg);
#endif

#ifdef DEBUG_WIFI
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.printf("Connect to:\t");
    Serial.println(ssid);
    Serial.printf("IP address:\t");
    Serial.println(WiFi.localIP());
    Serial.printf("Hostname:\t");
    Serial.println(WiFi.getHostname());
    Serial.printf("Mac Address:\t");
    Serial.println(WiFi.macAddress());
    Serial.printf("Subnet Mask:\t");
    Serial.println(WiFi.subnetMask());
    Serial.printf("Gateway IP:\t");
    Serial.println(WiFi.gatewayIP());
    Serial.printf("DNS:t\t\t");
    Serial.println(WiFi.dnsIP());
    Serial.println("HTTP server started");
  }
  else
    Serial.printf("No connect WiFi\n");
#endif
#ifdef SCAN_WIFI
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  for (;;)
  {
    Serial.println("scan start");
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0)
    {
      Serial.println("no networks found");
    }
    else
    {
      Serial.print(n);
      Serial.println(" networks found");
      for (int i = 0; i < n; ++i)
      {
        // Print SSID and RSSI for each network found
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.print(")");
        Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
        delay(10);
      }
    }
    Serial.println("");
    // Wait a bit before scanning again
    delay(5000);
  }
#endif

  flash.begin("eerom", false);
  Serial1.begin(flash.getInt("mspeed", 19200), SERIAL_8N1, RXDMASTER, TXDMASTER, false); // Modbus Master
  Serial2.begin(flash.getInt("sspeed", 19200));                                          // Modbus Slave
  SerialNextion.begin(19200, SWSERIAL_8N1, RXDNEX, TXDNEX, false);

  mbsl8di8ro.setAdress(flash.getInt("heat_adr", 102));

  String incStr;
  slave.begin(&Serial2);
  slave.slave(IDSLAVE);

  slaveWiFi.slave(IDSLAVE);
  slaveWiFi.begin();

  mb_master.begin(&Serial1);
  mb_master.master();

  for (int i = rs485mode1; i < rs485_HOLDING_REGS_SIZE; i++)
    slave.addHreg(i);
  for (int i = WiFimode1; i < WiFi_HOLDING_REGS_SIZE; i++)
    slave.addIreg(i);
  for (int i = wifi_flag_edit_1; i < wifi_HOLDING_REGS_SIZE; i++)
    slave.addHreg(i);

  arr_Tepl[0] = &Tepl1;
  arr_Tepl[1] = &Tepl2;
  arr_Tepl[2] = &Tepl3;

  // считывание параметров установок теплиц из памяти
  for (int i = 0; i < 3; i++)
  {
    uint t = flash.getUInt(String("SetPump" + String(arr_Tepl[i]->getId())).c_str(), 500);
    arr_Tepl[i]->setSetPump(t);
    t = flash.getUInt(String("SetHeat" + String(arr_Tepl[i]->getId())).c_str(), 300);
    arr_Tepl[i]->setSetHeat(t);
    t = flash.getUInt(String("SetSetWindow" + String(arr_Tepl[i]->getId())).c_str(), 600);
    arr_Tepl[i]->setSetWindow(t);
    t = flash.getUInt(String("Hyster" + String(arr_Tepl[i]->getId())).c_str(), 20);
    arr_Tepl[i]->setHysteresis(t);
    t = flash.getUInt(String("Opentwin" + String(arr_Tepl[i]->getId())).c_str(), 60);
    arr_Tepl[i]->setOpenTimeWindow(t);
  }

  incStr = flash.getString("adr", "");
  pars_str_adr(incStr);

  // первоначальное закрытие окон
  Tepl1.setWindowlevel(-100);
  Tepl2.setWindowlevel(-100);
  Tepl3.setWindowlevel(-100);

  tickerWiFiConnect.attach_ms(600000, update_WiFiConnect); // таймер проверки соединения WiFi (раз в 10 минут)

  xTaskCreatePinnedToCore(
      updateDateSensor,        /* Запрос по Modbus по  сети RS485 */
      "Task_updateDateSensor", /* Название задачи */
      10000,                   /* Размер стека задачи */
      NULL,                    /* Параметр задачи */
      10,                      /* Приоритет задачи */
      &Task_updateDateSensor,  /* Идентификатор задачи, чтобы ее можно было отслеживать */
      0);                      /* Ядро для выполнения задачи (0) */

  updateNextion = millis();

  xTaskCreatePinnedToCore(
      updateGreenHouse,        /* Регулировка окон*/
      "Task_updateGreenHouse", /* Название задачи */
      10000,                   /* Размер стека задачи */
      NULL,                    /* Параметр задачи */
      2,                       /* Приоритет задачи */
      &Task_updateGreenHouse,  /* Идентификатор задачи, чтобы ее можно было отслеживать */
      0);

#ifdef USE_WEB_SERIAL
  xTaskCreatePinnedToCore(
      webSerialSend,        /* */
      "Task_wedSerialSend", /* Название задачи */
      10000,                /* Размер стека задачи */
      NULL,                 /* Параметр задачи */
      4,                    /* Приоритет задачи */
      &Task_webSerialSend,  /* Идентификатор задачи, чтобы ее можно было отслеживать */
      1);
#endif
}
void loop()
{
  saveOutModBusArr();
  slave.task();
  slaveWiFi.task();
  heat.update();
  controlScada();
  if (millis() > 20000)
    if (!mb1108a.getErrorMB1108A())
    {
      for (Teplica *t : arr_Tepl)
      {
        if (pageNextion != "p3")
          if (1 == t->getSensorStatus())
            t->regulationPump(t->getTemperature());
      }
      for (Teplica *t : arr_Tepl)
      {
        t->updateWorkWindows();
      }
    }

  if (SerialNextion.available())
    readNextion();

  if (millis() > updateNextion)
  {
    long t = millis();
    if (pageNextion == "p0")
    {
      coun1 = 0;

      // TimingUtil test("indi_p0"); //тестирование времени вывода

      // вывод данных теплицы 1
      indiTepl1();
      // вывод данных теплицы 2
      indiTepl2();
      // вывод данных теплицы 3
      indiTepl3();
      // вывод данных о работе дизельного обогревателя
      indiGas();
    }
    else if (pageNextion == "p1_0")
      pageNextion_p1(0);
    else if (pageNextion == "p1_1")
      pageNextion_p1(1);
    else if (pageNextion == "p1_2")
      pageNextion_p1(2);
    else if (pageNextion == "p2")
      pageNextion_p2();
    else if (pageNextion == "p3")
      pageNextion_p3();
    updateNextion = millis() + 1000;
    digitalWrite(GPIO_NUM_2, !digitalRead(GPIO_NUM_2));
  }
}

// данные с Nextion
void readNextion()
{
  char inc;
  String incStr = "";
  while (SerialNextion.available())
  {
    inc = SerialNextion.read();
    // Serial.print(inc);
    incStr += inc;
    if (inc == 0x23)
      incStr = "";
    if (inc == '\n')
    {
      if (incStr != "" || incStr.length() > 2)
      {
        analyseString(incStr);
      }
      return;
    }
    delay(10);
  }
}

// получение данных от датчиков температуры
void updateDateSensor(void *pvParameters)
{
  for (;;)
  {
    mb1108a.read();
    update_mbmaster();

    mb11016p.write();
    update_mbmaster();

    mbsl8di8ro.write();
    update_mbmaster();

    if (!mb1108a.getErrorMB1108A())
    {
      if (1 == Tepl1.getSensorStatus())
      {
        Tepl1.setTemperature();
      }
      if (1 == Tepl2.getSensorStatus())
      {
        Tepl2.setTemperature();
      }
      if (1 == Tepl3.getSensorStatus())
      {
        Tepl3.setTemperature();
      }
    }
    vTaskDelay(TIME_UPDATE_MODBUS / portTICK_PERIOD_MS);
  }
}

// парсинг полученых данных от дисплея Nextion
void analyseString(String incStr)
{
  // Serial.printf("\nNextion send - %s\n", incStr);
  for (int i = 0; i < incStr.length(); i++)
  {
    if (incStr.substring(i).startsWith("page0")) //
      pageNextion = "p0";
    if (incStr.substring(i).startsWith("pageSet")) //
    {
      uint16_t temp = uint16_t(incStr.substring(i + 7, i + 8).toInt());
      if (temp == Tepl1.getId())
        pageNextion = "p1_0";
      if (temp == Tepl2.getId())
        pageNextion = "p1_1";
      if (temp == Tepl3.getId())
        pageNextion = "p1_2";
    }
    if (incStr.substring(i).startsWith("page2")) //
      pageNextion = "p2";
    if (incStr.substring(i).startsWith("page3")) //
    {
      pageNextion = "p3";
    }

    if (incStr.substring(i).startsWith("m1")) //  переключение режим теплица 1 автомат - ручной
    {
      Tepl1.setMode(Tepl1.getMode() == Teplica::AUTO ? Teplica::MANUAL : Teplica::AUTO);
      return;
    }
    if (incStr.substring(i).startsWith("m2")) //
    {
      Tepl2.setMode(Tepl2.getMode() == Teplica::AUTO ? Teplica::MANUAL : Teplica::AUTO);
      return;
    }
    if (incStr.substring(i).startsWith("m3")) //
    {
      Tepl3.setMode(Tepl3.getMode() == Teplica::AUTO ? Teplica::MANUAL : Teplica::AUTO);
      return;
    }
    if (incStr.substring(i).startsWith("w1")) //  переключение режим теплица 1 проветривание
    {
      Tepl1.setMode(Teplica::AIR);
      return;
    }
    if (incStr.substring(i).startsWith("w2")) //  переключение режим теплица 2 проветривание
    {
      Tepl2.setMode(Teplica::AIR);
      return;
    }
    if (incStr.substring(i).startsWith("w3")) //  переключение режим теплица 3 проветривание
    {
      Tepl3.setMode(Teplica::AIR);
      return;
    }
    if (incStr.substring(i).startsWith("dh1")) //  переключение режим теплица 1 осушение
    {
      Tepl1.setMode(Teplica::DECREASE_IN_HUMIDITY);
      return;
    }
    if (incStr.substring(i).startsWith("dh2")) //  переключение режим теплица 2 осушение
    {
      Tepl2.setMode(Teplica::DECREASE_IN_HUMIDITY);
      return;
    }
    if (incStr.substring(i).startsWith("dh3")) //  переключение режим теплица 3 осушение
    {
      Tepl3.setMode(Teplica::DECREASE_IN_HUMIDITY);
      return;
    }

    if (incStr.substring(i).startsWith("pump1")) //
    {
      Tepl1.setPump(Tepl1.getPump() ? OFF : ON);
      return;
    }
    if (incStr.substring(i).startsWith("pump2")) //
    {
      Tepl2.setPump(Tepl2.getPump() ? OFF : ON);
      return;
    }
    if (incStr.substring(i).startsWith("pump3")) //
    {
      Tepl3.setPump(Tepl3.getPump() ? OFF : ON);
      return;
    }

    if (incStr.substring(i).startsWith("val1")) //
    {
      heat.setRelay(heat.getValve1(), heat.getStatusRelay(heat.getValve1()) ? OFF : ON);
      return;
    }
    if (incStr.substring(i).startsWith("val2")) //
    {
      heat.setRelay(heat.getValve2(), heat.getStatusRelay(heat.getValve2()) ? OFF : ON);
      return;
    }
    if (incStr.substring(i).startsWith("val3")) //
    {
      heat.setRelay(heat.getValve3(), heat.getStatusRelay(heat.getValve3()) ? OFF : ON);
      return;
    }

    // тестирование работы задвижек
    if (incStr.substring(i).startsWith("tval1")) //
    {
      heat.setTestRelay(heat.getValve1(), heat.getStatusRelay(heat.getValve1()) ? OFF : ON);
      return;
    }
    if (incStr.substring(i).startsWith("tval2")) //
    {
      heat.setTestRelay(heat.getValve2(), heat.getStatusRelay(heat.getValve2()) ? OFF : ON);
      return;
    }
    if (incStr.substring(i).startsWith("tval3")) //
    {
      heat.setTestRelay(heat.getValve3(), heat.getStatusRelay(heat.getValve3()) ? OFF : ON);
      return;
    }

    if (incStr.substring(i).startsWith("set")) //
    {
      pars_str_set(incStr);
      return;
    }
    if (incStr.substring(i).startsWith("adr")) //
    {
      pars_str_adr(incStr);
      flash.putString("adr", incStr);
    }
    if (incStr.substring(i).startsWith("ss")) // изменение скорости шины связи с терминалом
    {
      uint16_t temp = uint16_t(incStr.substring(i + 2, i + 7).toInt());
      flash.putInt("sspeed", temp);
      Serial2.end();
      Serial2.begin(temp); // Modbus Slave
    }
    if (incStr.substring(i).startsWith("ms")) // изменение скорости шины связи с контроллером реле и контроллером датчиков
    {
      uint16_t temp = uint16_t(incStr.substring(i + 2, i + 7).toInt());
      flash.putInt("mspeed", temp);
      Serial1.end();
      Serial1.begin(flash.getInt("mspeed", 2400), SERIAL_8N1, RXDMASTER, TXDMASTER, false); // Modbus Master
    }
    if (incStr.substring(i).startsWith("heat_adr")) // изменение скорости шины связи с контроллером реле и контроллером датчиков
    {
      uint16_t temp = uint16_t(incStr.substring(i + 8).toInt());
      flash.putInt("heat_adr", temp);
      mbsl8di8ro.setAdress(temp);
    }
  }
}

void pars_str_set(String &str)
{
  int j = 0;
  String st = "";
  for (int i = 3; i < str.length(); i++)
  {
    if (str.charAt(i) == '.' || str.charAt(i) == '\n')
    {
      arr_set[j] = st.toInt();
      // Serial.println(arr_set[j]);
      j++;
      st = "";
    }
    else
      st += str.charAt(i);
  }
  arr_Tepl[arr_set[0] - 1]->setSetPump(arr_set[1] * 10);
  flash.putUInt(String("SetPump" + String(arr_set[0])).c_str(), arr_set[1] * 10);
  arr_Tepl[arr_set[0] - 1]->setSetHeat(arr_set[2] * 10);
  flash.putUInt(String("SetHeat" + String(arr_set[0])).c_str(), arr_set[2] * 10);

  arr_Tepl[arr_set[0] - 1]->setSetWindow(arr_set[3] * 10);
  flash.putUInt(String("SetSetWindow" + String(arr_set[0])).c_str(), arr_set[3] * 10);

  arr_Tepl[arr_set[0] - 1]->setWindowlevel(arr_set[4]);
  arr_Tepl[arr_set[0] - 1]->setHysteresis(arr_set[5]);
  flash.putUInt(String("Hyster" + String(arr_set[0])).c_str(), arr_set[5]);

  arr_Tepl[arr_set[0] - 1]->setOpenTimeWindow(arr_set[6]);
  flash.putUInt(String("Opentwin" + String(arr_set[0])).c_str(), arr_set[6]);
}

void pars_str_adr(String &str)
{
  int j = 0;
  String st = "";
  for (int i = 3; i < str.length(); i++)
  {
    if (str.charAt(i) == '.' || str.charAt(i) == '\n')
    {
      arr_adr[j] = st.toInt();
      // Serial.println(arr_adr[j]);
      j++;
      st = "";
    }
    else
      st += str.charAt(i);
  }
  Tepl1.setAdress(arr_adr[0]);
  displNext("p2.n0.val", arr_adr[0]);
  Tepl1.setCorrectionTemp(10 * arr_adr[1]);
  displNext("p2.t0.txt", String(arr_adr[1]));
  Tepl2.setAdress(arr_adr[2]);
  displNext("p2.n1.val", arr_adr[2]);
  Tepl2.setCorrectionTemp(10 * arr_adr[3]);
  displNext("p2.t1.txt", String(arr_adr[3]));
  Tepl3.setAdress(arr_adr[4]);
  displNext("p2.n2.val", arr_adr[4]);
  Tepl3.setCorrectionTemp(10 * arr_adr[5]);
  displNext("p2.t2.txt", String(arr_adr[5]));
  mb1108a.setAdress(arr_adr[10]);
  displNext("p2.n4.val", arr_adr[10]);
  mb11016p.setAdress(arr_adr[11]);
  displNext("p2.n5.val", arr_adr[11]);
}

// заполнение таблицы для передачи (протокол Modbus)
void saveOutModBusArr()
{
  // для терминала
  slave.Hreg(rs485mode1, Tepl1.getMode() | Tepl1.getPump() << 2 | Tepl1.getHeat() << 3 | Tepl1.getSetPump() / 10 << 8);
  slave.Hreg(rs485mode1, Tepl1.getLevel() | (Tepl1.getSetWindow() - Tepl1.getSetPump()) / 100 << 8);
  slave.Hreg(rs485mode2, Tepl2.getMode() | Tepl2.getPump() << 2 | Tepl2.getHeat() << 3 | Tepl2.getSetPump() / 10 << 8);
  // slave.Hreg(rs485mode21, Tepl2.getSetPump() << 8);
  slave.Hreg(rs485mode3, Tepl3.getMode() | Tepl3.getPump() << 2 | Tepl3.getHeat() << 3 | Tepl3.getSetPump() / 10 << 8);
  // slave.Hreg(rs485mode31, Tepl3.getSetPump() << 8);
  //  slave.Hreg(rs485mode7, Tepl_.getMode() | Tepl_.getPump() << 2 | Tepl_.getHeat() << 3 | Tepl_.getSetPump() / 10 << 8);
  //  slave.Hreg(rs485mode71, Tepl_.getLevel() | (Tepl_.getSetWindow() - Tepl_.getSetPump()) / 100 << 8);
  slave.Hreg(rs485temperature1, Tepl1.getTemperature() / 10);
  slave.Hreg(rs485temperature2, Tepl2.getTemperature() / 10);
  slave.Hreg(rs485temperature3, Tepl3.getTemperature() / 10);

  slave.Hreg(rs485error45, Tepl1.getSensorStatus() | Tepl2.getSensorStatus() << 8);
  slave.Hreg(rs485error67, Tepl3.getSensorStatus());

  // для SCADA
  int i = 0;
  for (Teplica *t : arr_Tepl)
  {
    slaveWiFi.Ireg(WiFimode1 + i, t->getMode());
    slaveWiFi.Ireg(WiFipump1 + i, t->getPump());
    slaveWiFi.Ireg(WiFiheat1 + i, t->getHeat());
    slaveWiFi.Ireg(WiFisetpump1 + i, t->getSetPump());
    slaveWiFi.Ireg(WiFitemperature1 + i, t->getTemperature());
    slaveWiFi.Ireg(WiFierror1 + i, t->getSensorStatus());
    slaveWiFi.Ireg(WiFiLevel1 + i, t->getLevel());
    slaveWiFi.Ireg(WiFisetwindow1 + i, t->getSetWindow());
    slaveWiFi.Ireg(WiFisetheat1 + i, t->getSetHeat());
    slaveWiFi.Ireg(WiFiHysteresis1 + i, t->getHysteresis());
    slaveWiFi.Ireg(WiFiOpenTimeWindow1 + i, t->getOpenTimeWindow());
    i += 12;
  }
}

void update_mbmaster()
{
  unsigned long t = millis() + 420;
  while (t > millis())
  {
    mb_master.task();
    yield();
    // delay(5);
  }
}
void update_WiFiConnect()
{
  /*----настройка Wi-Fi---------*/
  int counter_WiFi = 0;

  while (WiFi.status() != WL_CONNECTED && counter_WiFi < 10)
  {
    WiFi.disconnect();
    WiFi.reconnect();
    // Serial.println("Reconecting to WiFi..");
    counter_WiFi++;
    delay(1000);
  }
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("Connect to:\t%s\n", ssid);
  else
    Serial.printf("Dont connect to: %s\n", ssid);
}

/*--------------------------------------- изменения параметров с компьютера ------------------------------------*/
void controlScada()
{
  int number = 0xfff;

  if (slave.Hreg(wifi_flag_edit_1))
  {
    number = 0;
    slave.Hreg(wifi_flag_edit_1, 0);
  }
  if (slave.Hreg(wifi_flag_edit_2))
  {
    number = 1;
    slave.Hreg(wifi_flag_edit_2, 0);
  }
  if (slave.Hreg(wifi_flag_edit_3))
  {
    number = 2;
    slave.Hreg(wifi_flag_edit_3, 0);
  }
  // if (slave.Hreg(wifi_flag_edit_7))
  // {
  //   number = 3;
  //   slave.Hreg(wifi_flag_edit_7, 0);
  // }
  if (number == 0xfff)
    return;

  int k = (int(wifi_HOLDING_REGS_SIZE) - int(wifi_flag_edit_1)) / 4;
  k *= number;
  arr_Tepl[number]->setSetPump(slave.Hreg(wifi_UstavkaPump_1 + k));
  flash.putUInt(String("SetPump" + String(arr_Tepl[number]->getId())).c_str(), slave.Hreg(wifi_UstavkaPump_1 + k));

  arr_Tepl[number]->setSetHeat(slave.Hreg(wifi_UstavkaHeat_1 + k));
  flash.putUInt(String("SetHeat" + String(arr_Tepl[number]->getId())).c_str(), slave.Hreg(wifi_UstavkaHeat_1 + k));

  arr_Tepl[number]->setSetWindow(slave.Hreg(wifi_UstavkaWin_1 + k));
  flash.putUInt(String("SetSetWindow" + String(arr_Tepl[number]->getId())).c_str(), slave.Hreg(wifi_UstavkaWin_1 + k));

  arr_Tepl[number]->setMode(slave.Hreg(wifi_mode_1 + k));

  arr_Tepl[number]->setMode(slave.Hreg(wifi_mode_1 + k));
  if (arr_Tepl[number]->getMode() == Teplica::MANUAL)
  {
    arr_Tepl[number]->setPump(slave.Hreg(wifi_pump_1 + k));
    arr_Tepl[number]->setHeat(slave.Hreg(wifi_heat_1 + k));
    arr_Tepl[number]->setWindowlevel(slave.Hreg(wifi_setWindow_1 + k));
  }
  if (arr_Tepl[number]->getMode() == Teplica::AIR)
    arr_Tepl[number]->air(AIRTIME, 0);

  if (arr_Tepl[number]->getMode() == Teplica::DECREASE_IN_HUMIDITY)
    arr_Tepl[number]->decrease_in_humidity(AIRTIME, 0);

  arr_Tepl[number]->setHysteresis(slave.Hreg(wifi_hysteresis_1 + k));
  flash.putUInt(String("Hyster" + String(arr_Tepl[number]->getId())).c_str(), slave.Hreg(wifi_hysteresis_1 + k));

  arr_Tepl[number]->setOpenTimeWindow(slave.Hreg(wifi_time_open_windows_1 + k));
  flash.putUInt(String("Opentwin" + String(arr_Tepl[number]->getId())).c_str(), slave.Hreg(wifi_time_open_windows_1 + k));
}

#ifdef USE_WEB_SERIAL
void webSerialdisplay(void *pvParameters)
{
  for (;;)
  {
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

void recvMsg(uint8_t *data, size_t len)
{
  WebSerial.println("Received Data...");
  String d = "";
  for (int i = 0; i < len; i++)
  {
    d += char(data[i]);
  }
  WebSerial.println(d);
}
#endif

String calculateTimeWork()
{
  String str = "\nDuration of work: ";
  unsigned long currentMillis = millis();
  unsigned long seconds = currentMillis / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  unsigned long days = hours / 24;
  currentMillis %= 1000;
  seconds %= 60;
  minutes %= 60;
  hours %= 24;

  if (days > 0)
  {
    str += String(days) + " d ";
    str += String(hours) + " h ";
    str += String(minutes) + " min ";
  }
  else if (hours > 0)
  {
    str += String(hours) + " h ";
    str += String(minutes) + " min ";
  }
  else if (minutes > 0)
    str += String(minutes) + " min ";
  str += String(seconds) + " sec";
  return str;
}

// регулировка окон
void updateGreenHouse(void *pvParameters)
{
  for (;;)
  {
    for (Teplica *t : arr_Tepl)
    {
      int level_window_tmp = t->getLevel();

      if (t->getSensorStatus() == Teplica::NO_ERROR && t->getThereAreWindows())
      {
        t->regulationWindow(t->getTemperature());
      }
      if (level_window_tmp != t->getLevel())
      {
        flash.putUInt(String("LevelWindow" + String(t->getId())).c_str(), t->getLevel()); // запись уровня открытия окна при его изменении.
        Serial.printf("\nТеплица %d : %d", t->getId(), t->getLevel());
      }
    }
    vTaskDelay(TIME_UPDATE_GREENOOUSE * 60 * 1000 / portTICK_PERIOD_MS);
  }
}

// вывод данных теплицы 1
void indiTepl1()
{
  if (!mb1108a.getErrorMB1108A())
  { // ввывод температуры и ошибок датчика температуры
    if (1 == Tepl1.getSensorStatus())
    {
      displNext("p0.t0.font", 5);
      displNext("p0.t0.txt", String(Tepl1.getTemperature() / 100.0, 1));
      if (Tepl1.getTemperature() < Tepl1.getSetHeat())
        displNext("p0.t0.pco", BLUE);
      else if (Tepl1.getTemperature() > Tepl1.getSetPump() + 300)
        displNext("p0.t0.pco", RED);
      else
        displNext("p0.t0.pco", GREEN);
    }
    else
    {
      displNext("p0.t0.pco", LIGHT);
      displNext("p0.t0.font", 1);
      displNext("p0.t0.txt", "Er " + String(Tepl1.getSensorStatus(), HEX));
    }
  }
  else
  {
    displNext("p0.t0.pco", RED);
    displNext("p0.t0.font", 1);
    displNext("p0.t0.txt", "Er 108");
  }
  // вывод уставки насос
  displNext("p0.x1.val", Tepl1.getSetPump() / 10);
  // вывод уставки дополнительный обогреватель
  displNext("p0.x2.val", Tepl1.getSetHeat() / 10);
  // вывод уставки окно
  displNext("p0.x3.val", Tepl1.getSetWindow() / 10);
  // вывод режима работы
  if (Tepl1.getMode() == Teplica::MANUAL)
    displNext("p0.t3.txt", "M");
  if (Tepl1.getMode() == Teplica::AUTO)
    displNext("p0.t3.txt", "A");
  if (Tepl1.getMode() == Teplica::AIR)
    displNext("p0.t3.txt", "W");
  if (Tepl1.getMode() == Teplica::DECREASE_IN_HUMIDITY)
    displNext("p0.t3.txt", "H");
  // идикация состояния насоса
  displNext("p0.p1.pic", Tepl1.getPump() ? 12 : 11);
  // идикация состояния дополнительного обогревателя (задвижка)
  displNext("p0.p2.pic", Tepl1.getHeat() && heat.getSatusHeat() ? 12 : 11);
  // вывод уровня открытия окон
  displNext("p0.h0.val", Tepl1.getLevel());
}

// вывод данных теплицы 2
void indiTepl2()
{
  if (!mb1108a.getErrorMB1108A())
  {
    // ввывод температуры и ошибок датчика температуры
    if (1 == Tepl2.getSensorStatus())
    {
      displNext("p0.t7.font", 5);
      displNext("p0.t7.txt", String(Tepl2.getTemperature() / 100.0, 1));
      if (Tepl2.getTemperature() < Tepl2.getSetHeat())
        displNext("p0.t7.pco", BLUE);
      else if (Tepl2.getTemperature() > Tepl2.getSetPump() + 300)
        displNext("p0.t7.pco", RED);
      else
        displNext("p0.t7.pco", GREEN);
    }
    else
    {
      displNext("p0.t7.pco", LIGHT);
      displNext("p0.t7.font", 1);
      displNext("p0.t7.txt", "Er " + String(Tepl2.getSensorStatus(), HEX));
    }
  }
  else
  {
    displNext("p0.t7.pco", RED);
    displNext("p0.t7.font", 1);
    displNext("p0.t7.txt", "Er 108");
  }
  // ввывод уставки насос
  displNext("p0.x4.val", Tepl2.getSetPump() / 10);
  // ввывод уставки дополнительный обогреватель
  displNext("p0.x6.val", Tepl2.getSetHeat() / 10);
  // вывод уставки окно
  displNext("p0.x7.val", Tepl2.getSetWindow() / 10);
  // ввывод режима работы
  if (Tepl2.getMode() == Teplica::MANUAL)
    displNext("p0.t4.txt", "M");
  if (Tepl2.getMode() == Teplica::AUTO)
    displNext("p0.t4.txt", "A");
  if (Tepl2.getMode() == Teplica::AIR)
    displNext("p0.t4.txt", "W");
  if (Tepl2.getMode() == Teplica::DECREASE_IN_HUMIDITY)
    displNext("p0.t4.txt", "H");
  // идикация состояния насоса
  displNext("p0.p3.pic", Tepl2.getPump() ? 12 : 11);
  // идикация состояния дополнительного обогревателя (задвижка)
  displNext("p0.p4.pic", Tepl2.getHeat() && heat.getSatusHeat() ? 12 : 11);
  // вывод уровня открытия окон
  displNext("p0.h1.val", Tepl2.getLevel());
}

// вывод данных теплицы 3
void indiTepl3()
{
  if (!mb1108a.getErrorMB1108A())
  {
    // ввывод температуры и ошибок датчика температуры
    if (1 == Tepl3.getSensorStatus())
    {
      displNext("p0.t8.font", 5);
      displNext("p0.t8.txt", String(Tepl3.getTemperature() / 100.0, 1));
      if (Tepl3.getTemperature() < Tepl3.getSetHeat())
        displNext("p0.t8.pco", BLUE);
      else if (Tepl3.getTemperature() > Tepl3.getSetPump() + 300)
        displNext("p0.t8.pco", RED);
      else
        displNext("p0.t8.pco", GREEN);
    }
    else
    {
      displNext("p0.t8.pco", LIGHT);
      displNext("p0.t8.font", 1);
      displNext("p0.t8.txt", "Er " + String(Tepl3.getSensorStatus(), HEX));
    }
  }
  else
  {
    displNext("p0.t8.pco", RED);
    displNext("p0.t8.font", 1);
    displNext("p0.t8.txt", "Er 108");
  }
  // ввывод уставки насос
  displNext("p0.x8.val", Tepl3.getSetPump() / 10);
  // ввывод уставки дополнительный обогреватель
  displNext("p0.x10.val", Tepl3.getSetHeat() / 10);
  // вывод уставки окно
  displNext("p0.x11.val", Tepl3.getSetWindow() / 10);
  // ввывод режима работы
  if (Tepl3.getMode() == Teplica::MANUAL)
    displNext("p0.t5.txt", "M");
  if (Tepl3.getMode() == Teplica::AUTO)
    displNext("p0.t5.txt", "A");
  if (Tepl3.getMode() == Teplica::AIR)
    displNext("p0.t5.txt", "W");
  if (Tepl3.getMode() == Teplica::DECREASE_IN_HUMIDITY)
    displNext("p0.t5.txt", "H");
  // индикация состояния насоса
  // Tepl3.getPump() ? displNext("p0.p5.pic", 12) : displNext("p0.p5.pic", 11);
  displNext("p0.p5.pic", Tepl3.getPump() ? 12 : 11);
  // индикация состояния дополнительного обогревателя (задвижка)
  // Tepl3.getHeat() && heat.getSatusHeat() ? displNext("p0.p6.pic", 12) : displNext("p0.p6.pic", 11);
  displNext("p0.p6.pic", Tepl3.getHeat() && heat.getSatusHeat() ? 12 : 11);
  // вывод уровня открытия окон
  displNext("p0.h2.val", Tepl3.getLevel());
}

// вывод данных о работе дизельного обогревателя
void indiGas()
{
  // идикация состояния компрессора
  displNext("p0.gm0.en", heat.getSatusHeat() ? 1 : 0);
  // идикация состояния задвижек
  displNext("p0.p8.pic", Tepl1.getHeat() ? 12 : 11);
  displNext("p0.p9.pic", Tepl2.getHeat() ? 12 : 11);
  displNext("p0.p10.pic", Tepl3.getHeat() ? 12 : 11);
}

// окно установок теплиц
void pageNextion_p1(int i)
{
  if (coun1 < 2)
  {
    String err = "";
    displNext("p1.x0.val", arr_Tepl[i]->getSetPump() / 10);
    displNext("p1.x4.val", arr_Tepl[i]->getSetHeat() / 10);
    displNext("p1.x1.val", arr_Tepl[i]->getHysteresis() / 10);
    displNext("p1.x2.val", arr_Tepl[i]->getOpenTimeWindow());
    displNext("p1.x3.val", arr_Tepl[i]->getSetWindow() / 10);
    displNext("p1.h0.val", arr_Tepl[i]->getLevel());
    displNext("p1.h1.val", arr_Tepl[i]->getHysteresis());
    displNext("p1.h2.val", arr_Tepl[i]->getOpenTimeWindow());
    displNext("p1.n0.val", arr_Tepl[i]->getLevel());
    if (mb11016p.getError())
      err = "MB16R: " + String(mb11016p.getError());
    if (mbsl8di8ro.getError())
      err += " MB16R_heat: " + String(mbsl8di8ro.getError());
    if (mb1108a.getErrorMB1108A())
      err += "  MB08A: " + String(mb1108a.getErrorMB1108A());
    if (err.length())
      displNext("g0.txt", err);
    else
      displNext("g0.txt", "Mb adress: " + String(IDSLAVE) + " | " + "\nRSSI: " + String(WiFi.RSSI()) + " | " + WiFi.localIP().toString());
    coun1++;
  }
  displNext("b3.picc", arr_Tepl[i]->getPump() ? 2 : 1);

  switch (arr_Tepl[i]->getMode())
  {
  case Teplica::MANUAL:
    displNext("b0.picc", 2);
    displNext("b1.picc", 1);
    displNext("b2.picc", 1);
    break;
  case Teplica::AUTO:
    displNext("b0.picc", 1);
    displNext("b1.picc", 1);
    displNext("b2.picc", 1);
    break;
  case Teplica::AIR:
    displNext("b0.picc", 1);
    displNext("b1.picc", 2);
    displNext("b2.picc", 1);
    break;
  case Teplica::DECREASE_IN_HUMIDITY:
    displNext("b0.picc", 1);
    displNext("b1.picc", 1);
    displNext("b2.picc", 2);
    break;
  default:
    break;
  }
}

void pageNextion_p2()
{
  if (coun1 < 3)
  {
    String incStr = flash.getString("adr", "");
    pars_str_adr(incStr);

    switch (flash.getInt("sspeed", 2400))
    {
    case 2400:
      displNext("r4.val=1");
      break;
    case 9600:
      displNext("r3.val=1");
      break;
    case 19200:
      displNext("r5.val=1");
      break;
    }
    switch (flash.getInt("mspeed", 2400))
    {
    case 2400:
      displNext("r0.val=1");
      break;
    case 9600:
      displNext("r1.val=1");
      break;
    case 19200:
      displNext("r2.val=1");
      break;
    }
    coun1++;
  }
}

// вывод данных о работе дизельного обогревателя
void pageNextion_p3()
{
  if (coun1 < 3)
  {
    displNext("p3.n0.val", flash.getInt("heat_adr", 102));
    coun1++;
  }
  // идикация состояния компрессора
  displNext("gm0.en", heat.getSatusHeat() ? 1 : 0);
  // идикация состояния задвижек
  displNext("p8.pic", Tepl1.getHeat() ? 12 : 11);
  displNext("p9.pic", Tepl2.getHeat() ? 12 : 11);
  displNext("p10.pic", Tepl3.getHeat() ? 12 : 11);
}
#include <FS.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include "ArduinoJson.h"
#include <WiFiManager.h>
#include <ArduinoOTA.h>

#define   LED_OFF_ON      0
#define   LATCH           12
#define   OUTPUT_ENABLE   13
#define   DATA            14
#define   SECONDS_BLINK   15
#define   CLOCK           16
#define   SLEEP_BEGIN     22
#define   SLEEP_END       6
#define   BLANKS_DIGITS   85

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
char NIXIE_CLOCK[12] = "WiFi_CLOCK";
//----------------VARIABLES------------------------
int hour_value;
int minute_value;
int second_value;
int minute_before = 0;
String formattedDate;
String string_hour;
String string_minute;
String string_second;
bool shouldSaveConfig = false;
bool sleep_flag = false;
int HMS[] = {0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 128, 136, 132, 140, 130, 138, 134, 142, 129, 137, 64, 72, 68, 76, 66, 74, 70, 78, 65, 73, 192, 200, 196, 204, 194, 202, 198, 206, 193, 201, 32, 40, 36, 44, 34, 42, 38, 46, 33, 41,
             160, 168, 164, 172, 162, 170, 166, 174, 161, 169, 96, 104, 100, 108, 98, 106, 102, 110, 97, 105, 224, 232, 228, 236, 226, 234, 230, 238, 225, 233, 16, 24, 20, 28, 18, 26, 22, 30, 17, 25, 144, 152, 148,
             156, 146, 154, 150, 158, 145, 153
            };
//-------------------------------------------------

void saveConfigCallback () {
  //Serial.println("Should save config");
  shouldSaveConfig = true;
}


void setup() {
  //---------------------------------------------------
  pinMode(OUTPUT_ENABLE, OUTPUT);//OUTPUT output enable
  pinMode(CLOCK, OUTPUT);//OUTPUT clock
  pinMode(LED_OFF_ON, OUTPUT);//OUTPUT led_off_on
  pinMode(DATA, OUTPUT);//OUTPUT data
  pinMode(LATCH, OUTPUT);//OUTPUT latch
  pinMode(2, OUTPUT);//OUTPUT
  //digitalWrite(CLOCK, 1);
  //digitalWrite(LATCH, 1);
  //digitalWrite(DATA, 1);
  digitalWrite(OUTPUT_ENABLE, 1); //enable txs0104 3.3V ----> 5V
  //sleep_nixie();
  //---------getting time------------------------------
  // put your setup code here, to run once:
  Serial.begin(115200);
  if (SPIFFS.begin()) {
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          //Serial.println("\nparsed json");
          //recupera variables almacenadas
          strcpy(NIXIE_CLOCK, json["NIXIE_CLOCK"]);
          //Serial.println("nombre dispositivo");
          //Serial.println(NIXIE_CLOCK);
        } else {
          //operacion
        }
        configFile.close();
      }
    }
  } else {
    //operacion
  }
  WiFiManagerParameter custom_NIXIE_CLOCK("NIXIE_CLOCK", "NIXIE_CLOCKNAME", NIXIE_CLOCK, 12);
  // Connect to WiFi network
  WiFiManager wifiManager;
  //set config
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  //agregar nuevo parametro
  wifiManager.addParameter(&custom_NIXIE_CLOCK);
  //***************RESET WIFIMANAGER***********************
  //wifiManager.resetSettings();
  //*******************************************************
  wifiManager.autoConnect("NIXIE_CLOCK");//coment serial.begin()
  strcpy(NIXIE_CLOCK, custom_NIXIE_CLOCK.getValue());
  if (shouldSaveConfig) {
    //Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["NIXIE_CLOCK"] = NIXIE_CLOCK;
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      //Serial.println("failed to open config file for writing");
    }
    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
  }
  //------------------------------arduino OTA-----------------------------------
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  //----------------------------------------------------------------------------
  timeClient.begin();
  timeClient.setTimeOffset(-10800);
  digitalWrite(2, 1);
  digitalWrite(LED_OFF_ON, 1);
}

void loop() {
  // put your main code here, to run repeatedly:

  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  formattedDate = timeClient.getFormattedTime();
  //Serial.println(formattedDate);
  string_hour = timeClient.getHours();
  hour_value = (string_hour.toInt());
  //Serial.println(hour_value);
  string_minute = timeClient.getMinutes();
  minute_value = (string_minute.toInt());
  //Serial.println(string_minute);
  string_second = timeClient.getSeconds();
  second_value = (string_second.toInt());
  //Serial.println(string_second);

  //-------------------------------------if hour between 2200hs and 0600 hours-------------------------------------------//
  if ((hour_value >= SLEEP_BEGIN))
  {
    //condicion sleep apaga leds azules background
    //setea bandera sleep
    if (!sleep_flag)
    {
      sleep_nixie();
      digitalWrite(LED_OFF_ON, 0);
      sleep_flag = true;
    }
  }
  else
  {
    //si termina horario sleep cambia bandera sleep
    //enciende led azules background
    if ((hour_value >= SLEEP_END))
    {
      sleep_flag = false;
      digitalWrite(LED_OFF_ON, 1); //blue leds ON
      if (((minute_value == 9) || (minute_value == 19) || (minute_value == 29) || (minute_value == 39)||(minute_value == 49)||(minute_value == 59)) && (second_value == 56)) //cada 15min inicia secuencia slot para evitar deterioro nixie
      {
        slot_machine_effect();
      }
      else
      {
        if (minute_value != minute_before) //cada 1 minuto actualiza clock
        {
          minute_before = minute_value;//update minute_before with actual minute
          shift_74hc595(hour_value, minute_value); //change nixie clock minute
        }
      }
    }//hour_value > SLEEP_END
  }//end ELSE
  ArduinoOTA.handle();
}

void shift_74hc595(int hour_value, int minute_value) {
  digitalWrite(LATCH, 0);
  shiftOut(DATA, CLOCK, LSBFIRST, HMS[minute_value]); //shiftOUT minute first to second 74HC595
  shiftOut(DATA, CLOCK, LSBFIRST, HMS[hour_value]); //shiftOut hour to first 74hc595
  digitalWrite(LATCH, 1);
}

void slot_machine_effect() {
  for (int j = 99; j >= 0; j -= 11)
  {
    digitalWrite(LATCH, 0);
    shiftOut(DATA, CLOCK, LSBFIRST, HMS[j]); //shiftOut minute first to second 74HC595
    shiftOut(DATA, CLOCK, LSBFIRST, HMS[j]); //shiftOut hour to first 74hc595
    digitalWrite(LATCH, 1);
    digitalWrite(2, 0);
    delay(200);
    digitalWrite(2, 1);
    delay(200);
    //Serial.println(j);
  }
}

void sleep_nixie() {
  digitalWrite(LATCH, 0);
  shiftOut(DATA, CLOCK, LSBFIRST, BLANKS_DIGITS); //shiftOUT minute first to second 74HC595
  shiftOut(DATA, CLOCK, LSBFIRST, BLANKS_DIGITS); //shiftout hour to first 74hc595
  digitalWrite(LATCH, 1);
}

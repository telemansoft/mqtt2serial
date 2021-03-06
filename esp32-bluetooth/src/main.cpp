//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial

#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

void setupBT() {
  Serial.begin(115200);
  SerialBT.begin("ESP32test"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
}

void loopBT() {
  if (Serial.available()) {
    SerialBT.write(Serial.read());
  }
  if (SerialBT.available()) {
    Serial.write(SerialBT.read());
  }
  delay(20);
}



#include <Arduino.h>

#include "proto.h"

//_______________________________________________________________________________________________________________
//

//_______________________________________________________________________________________________________________
//
class LedBlinker : public ProtoThread {
  uint32_t _pin, _delay;

public:
  LedBlinker(uint32_t pin, uint32_t delay) {
    _pin = pin;
    _delay = delay;
  }
  void setup() {
    LOG("LedBlinker started.");
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, 1);
  }
  void loop() {
    PT_BEGIN();
    while (true) {
      timeout(_delay);
      digitalWrite(_pin, 0);
      PT_YIELD_UNTIL(timeout());
      timeout(_delay);
      digitalWrite(_pin, 1);
      PT_YIELD_UNTIL(timeout());
    }
    PT_END();
  }
  void delay(uint32_t d) { _delay = d; }
};
//_______________________________________________________________________________________________________________
//
// LedBlinker ledBlinkerRed(LED_RED_PIN,100);
// LedBlinker ledBlinkerGreen(LED_GREEN_PIN,300);
//_______________________________________________________________________________________________________________
//
class Publisher : public ProtoThread {
  String _systemPrefix;
  MqttSerial &_mqtt;
  LedBlinker &_ledBlinker;

public:
  Publisher(MqttSerial &mqtt, LedBlinker &ledBlinker)
      : _mqtt(mqtt), _ledBlinker(ledBlinker){};
  void setup() {
    LOG("Publisher started");
    _systemPrefix = "src/" + Sys::hostname + "/system/";
  }
  void loop() {
    PT_BEGIN();
    while (true) {
      if (_mqtt.isConnected()) {
        _mqtt.publish(_systemPrefix + "upTime", String(millis()));
        _mqtt.publish(_systemPrefix + "build", Sys::build);
        _mqtt.publish(_systemPrefix + "cpu", Sys::cpu);
  //      _mqtt.publish(_systemPrefix + "heap", String(freeMemory()));
        _ledBlinker.delay(1000);
      } else
        _ledBlinker.delay(100);
      timeout(1000);
      PT_YIELD_UNTIL(timeout());
    }
    PT_END();
  }
};
//
//_____________________________________ protothreads running _____
//
#define PIN_LED 2

MqttSerial mqtt(SerialBT);
LedBlinker ledBlinker(PIN_LED, 100);
Publisher publisher(mqtt, ledBlinker);

void mqttCallback(String topic, String message) {
  Serial.println(" RXD " + topic + "=" + message);
}

void setup() {
  Serial.begin(115200);
  setupBT();
  LOG("===== Starting ProtoThreads  build " __DATE__ " " __TIME__);
  Sys::hostname = "esp32-bluetooth";
  Sys::cpu = "esp32";
  mqtt.onMqttPublish(mqttCallback);
  ProtoThread::setupAll();
}

void loop() { ProtoThread::loopAll(); }

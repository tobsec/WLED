#pragma once

#include <Arduino.h>   // WLEDMM: make sure that I2C drivers have the "right" Wire Object
#include <Wire.h>

#include "src/dependencies/time/DS1307RTC.h"
#include "wled.h"

//Connect DS1307 to standard I2C pins (ESP32: GPIO 21 (SDA)/GPIO 22 (SCL))

class RTCUsermod : public Usermod {
  private:
    unsigned long lastTime = 0;
    bool disabled = false;
  public:

    void setup() {
      PinManagerPinType pins[2] = { { i2c_scl, true }, { i2c_sda, true } };
#if defined(ARDUINO_ARCH_ESP32) && defined(HW_PIN_SDA) && defined(HW_PIN_SCL)
      //if (pins[0].pin < 0) pins[0].pin = HW_PIN_SCL; //WLEDMM 
      //if (pins[1].pin < 0) pins[1].pin = HW_PIN_SDA; //WLEDMM 
#endif
      if (pins[1].pin < 0 || pins[0].pin < 0)  { disabled=true; return; }  //WLEDMM bugfix - ensure that "final" GPIO are valid and no "-1" sneaks trough
      if (!pinManager.allocateMultiplePins(pins, 2, PinOwner::HW_I2C)) { disabled = true; return; }
#if defined(ARDUINO_ARCH_ESP32)
      Wire.begin(pins[1].pin, pins[0].pin);  // WLEDMM this might silently fail, which is OK as it just means that I2C bus is already running.
#else
      Wire.begin();  // WLEDMM - i2c pins on 8266 are fixed.
#endif

      RTC.begin();
      time_t rtcTime = RTC.get();
      if (rtcTime) {
        toki.setTime(rtcTime,TOKI_NO_MS_ACCURACY,TOKI_TS_RTC);
        updateLocalTime();
      } else {
        if (!RTC.chipPresent()) disabled = true; //don't waste time if H/W error
      }
    }

    void loop() {
      if (strip.isUpdating()) return;
      if (!disabled && toki.isTick()) {
        time_t t = toki.second();
        if (abs(t - RTC.get())> 2) RTC.set(t); //set RTC to NTP/UI-provided value - WLEDMM allow up to 3 sec deviation
      }
    }

    /*
     * addToConfig() can be used to add custom persistent settings to the cfg.json file in the "um" (usermod) object.
     * It will be called by WLED when settings are actually saved (for example, LED settings are saved)
     * I highly recommend checking out the basics of ArduinoJson serialization and deserialization in order to use custom settings!
     */
//    void addToConfig(JsonObject& root)
//    {
//      JsonObject top = root.createNestedObject("RTC");
//      JsonArray pins = top.createNestedArray("pin");
//      pins.add(i2c_scl);
//      pins.add(i2c_sda);
//    }

    uint16_t getId()
    {
      return USERMOD_ID_RTC;
    }
};
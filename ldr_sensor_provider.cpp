#include "wled.h"
#include "sensor_bus.h"

/*
 * LDR (photoresistor) light level sensor provider.
 *
 * Reads a simple analog LDR voltage divider (e.g. a KY-018 module) on a
 * single ADC-capable pin and pushes a raw 0-100% light level into the
 * Sensor Hub (see ../sensor-hub/usermod_sensor_hub.cpp and
 * ../sensor-hub/sensor_bus.h) as "<prefix>_light". This is an uncalibrated
 * percentage of the ADC's raw range, not a calibrated lux value - use the
 * BH1750 provider in this repo if you need actual lux. This usermod never
 * talks to MQTT, the JSON API or the Info tab itself - the hub takes care
 * of all of that once a sensor is registered here.
 *
 * The pin is not a shared WLED global - it is configured (and reserved via
 * WLED's PinManager, to avoid clashing with LEDs/relays/other usermods)
 * right here in this usermod's own settings. Note ESP8266 only exposes a
 * single fixed ADC pin (A0/GPIO17).
 */
class LDRSensorUsermod : public Usermod {
  private:
    SensorHub* hub = nullptr;
    uint8_t lightHandle = SENSOR_HANDLE_INVALID;

    bool enabled = true;
    bool initDone = false;

    unsigned long lastRead = 0;

    // config
    int8_t pin = -1;                // ADC pin, unset by default
    bool invert = false;            // flip the percentage for wiring where a higher raw reading means darker
    uint16_t checkIntervalMs = 500; // how often the pin is read
    String namePrefix = "ldr";      // sensor name becomes "<prefix>_light"
    uint8_t priority = 100;         // getValue() selection priority - lower wins among sensors of the same SensorType (see sensor_bus.h)

    static const char _name[];
    static const char _enabled[];
    static const char _pin[];
    static const char _invert[];
    static const char _checkInterval[];
    static const char _namePrefix[];
    static const char _priority[];

#ifdef ESP8266
    static const uint16_t ADC_MAX = 1023;
#else
    static const uint16_t ADC_MAX = 4095;
#endif

    void registerSensors() {
      if (!hub || lightHandle != SENSOR_HANDLE_INVALID) return; // already registered
      lightHandle = hub->registerSensor((namePrefix + "_light").c_str(), SensorType::Generic, "%", nullptr, 0, priority);
    }

  public:
    void setup() override {
      // Neither branch touches 'enabled' (the user's own on/off switch,
      // persisted to config) - initDone (left false here) is what actually
      // gates loop(), so a later pin fix takes effect on the next boot
      // instead of staying stuck disabled.
      if (pin < 0) return;
      if (!PinManager::allocatePin(pin, false, PinOwner::UM_Unspecified)) {
        pin = -1; // conflicts with another pin owner - force reconfiguration
        return;
      }
      pinMode(pin, INPUT);
      initDone = true;
    }

    void loop() override {
      if (!enabled || !initDone) return;

      if (!hub) hub = getSensorHub(); // Sensor Hub usermod may finish init after us
      if (hub) registerSensors();

      unsigned long now = millis();
      if (now - lastRead < (unsigned long)checkIntervalMs) return;
      lastRead = now;

      uint16_t raw = analogRead(pin);
      float pct = (raw * 100.0f) / (float)ADC_MAX;
      if (invert) pct = 100.0f - pct;

      if (hub && lightHandle != SENSOR_HANDLE_INVALID) {
        hub->setSensorAvailable(lightHandle, true);
        hub->updateSensor(lightHandle, pct);
      }
    }

    void addToConfig(JsonObject& root) override {
      JsonObject top = root.createNestedObject(FPSTR(_name));
      top[FPSTR(_enabled)] = enabled;
      top[FPSTR(_pin)] = pin;
      top[FPSTR(_invert)] = invert;
      top[FPSTR(_checkInterval)] = checkIntervalMs;
      top[FPSTR(_namePrefix)] = namePrefix;
      top[FPSTR(_priority)] = priority;
    }

    bool readFromConfig(JsonObject& root) override {
      int8_t oldPin = pin;

      JsonObject top = root[FPSTR(_name)];
      bool configComplete = !top.isNull();
      configComplete &= getJsonValue(top[FPSTR(_enabled)], enabled);
      configComplete &= getJsonValue(top[FPSTR(_pin)], pin);
      configComplete &= getJsonValue(top[FPSTR(_invert)], invert);
      configComplete &= getJsonValue(top[FPSTR(_checkInterval)], checkIntervalMs);
      configComplete &= getJsonValue(top[FPSTR(_namePrefix)], namePrefix);
      configComplete &= getJsonValue(top[FPSTR(_priority)], priority);

      if (initDone && pin != oldPin) {
        // pin changed at runtime via the Settings UI - release the old one and re-init on the new one
        if (oldPin >= 0) PinManager::deallocatePin(oldPin, PinOwner::UM_Unspecified);
        initDone = false;
        setup();
      }
      return configComplete;
    }

    void appendConfigData(Print& settingsScript) override {
      settingsScript.print(F("addInfo('LDRSensor:pin',1,'ADC pin');"));
      settingsScript.print(F("addInfo('LDRSensor:invert',1,'flip % if a higher raw reading means darker');"));
      settingsScript.print(F("addInfo('LDRSensor:checkInterval',1,'milliseconds between pin reads');"));
      settingsScript.print(F("addInfo('LDRSensor:namePrefix',1,'sensor name becomes &lt;prefix&gt;_light - must be unique across all sensor providers');"));
      settingsScript.print(F("addInfo('LDRSensor:priority',1,'getValue() selection priority - lower wins if another provider also registers a Generic sensor');"));
    }
};

const char LDRSensorUsermod::_name[]          PROGMEM = "LDRSensor";
const char LDRSensorUsermod::_enabled[]       PROGMEM = "enabled";
const char LDRSensorUsermod::_pin[]           PROGMEM = "pin";
const char LDRSensorUsermod::_invert[]        PROGMEM = "invert";
const char LDRSensorUsermod::_checkInterval[] PROGMEM = "checkInterval";
const char LDRSensorUsermod::_namePrefix[]    PROGMEM = "namePrefix";
const char LDRSensorUsermod::_priority[]      PROGMEM = "priority";

static LDRSensorUsermod ldr_sensor;
REGISTER_USERMOD(ldr_sensor);

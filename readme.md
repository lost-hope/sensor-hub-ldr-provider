# LDR Sensor Provider

A [Sensor Hub](../sensor-hub/readme.md) provider usermod for a simple
analog LDR/photoresistor voltage divider (e.g. a KY-018 module) - registers
a single `ldr_light` sensor (0-100%, uncalibrated) with the hub by default,
which then handles MQTT, Home Assistant discovery, the JSON API and the
Info tab. For a calibrated lux reading, use this repo's
[BH1750 provider](../sensor-hub-bh1750-provider/readme.md) instead.

## Hardware

Set the **Pin** in this usermod's own Settings page - it is reserved
through WLED's PinManager so it won't silently clash with LEDs, relays or
other usermods. Plain `analogRead()`, no library required. ESP8266 only
exposes a single fixed ADC pin (A0/GPIO17). If your wiring reads high when
dark, enable **Invert**.

## Usage

Add `sensor-hub-ldr-provider` to `custom_usermods` next to the
[Sensor Hub](../sensor-hub/readme.md) itself.

## Usermod Settings

| Setting | Default | Description |
|---|---|---|
| Enabled | on | Master on/off switch (also auto-disabled until a pin is set) |
| Pin | unset | ADC pin the LDR divider is wired to |
| Invert | off | Flip the percentage if a higher raw reading means darker |
| Check interval | 500 ms | How often the pin is read |
| Name prefix | `ldr` | Sensor name becomes `<prefix>_light` - must be unique across every provider registered with the hub |

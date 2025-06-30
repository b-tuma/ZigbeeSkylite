# Arduino-ESP32 Zigbee Sky Lite 2.0 Mod

More information on [brunotuma.com/blog/skylite-mod](https://brunotuma.com/blog/skylite-mod).

## Configure the Project

**Requires the [arduino-esp32](https://github.com/espressif/arduino-esp32) core and [OneButton](https://github.com/mathertel/OneButton) library!**

#### Using Arduino IDE

* Before Compile/Verify, select the correct board: `Tools -> Board`.
* Select the End device Zigbee mode: `Tools -> Zigbee mode: Zigbee ED (end device)`
* Select Partition Scheme for Zigbee: `Tools -> Partition Scheme: Zigbee 4MB with spiffs`
* Select the COM port: `Tools -> Port: xxx` where the `xxx` is the detected COM port.
* Optional: Set debug level to verbose to see all logs from Zigbee stack: `Tools -> Core Debug Level: Verbose`.

## Resources

* Arduino-ESP32 Official Repository: [espressif/arduino-esp32](https://github.com/espressif/arduino-esp32)
* ESP32-H2 Datasheet: [Link to datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-h2_datasheet_en.pdf)
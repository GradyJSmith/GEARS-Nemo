# GEARS Nemo Software

This software is intended for use on the ESP32-WROOM-32 board in the GEARS, Inc. Nemo float. Currently, **all rights are reserved** by Grady Smith (GradyJSmith).

---

## Hardware List

*As far as I know, this is a complete hardware list minus the acrylic tube, 3D-printed parts, and syringe.*
*3d printed parts are coming to printables soon.*

- [Linear Actuator](https://www.amazon.com/dp/B0CQV3WSFB) — 1x  
- [ESP32 and Breakout Board](https://www.amazon.com/AITRIP-ESP-WROOM-32-Bluetooth-ESP32-DevKitC-32-Development/dp/B0DPS44HCQ/) — 1x  
- [Relay Modules](https://www.amazon.com/dp/B07XGZSYJV/) — 2x  
- [Actuator Control Module](https://www.amazon.com/Forward-Reverse-Module-Actuator-Reversing/dp/B0879GGVPZ/) — 1x  
- [Power Supply](https://www.amazon.com/BINZET-Converter-Regulator-Regulated-Transformer/dp/B00J3MHRNO/) — 1x  
- [Battery Pack](https://www.amazon.com/AIMPGSTL-Battery-Holder-Storage-Plastic/dp/B0BH59RQX1/) — 1x  
- [NiMH Batteries](https://www.amazon.com/WENJOOP-Rechargeable-Batteries-Household-Controllers/dp/B0DMW7268Z/) — 1x  
- [Pressure Sensor](https://thepihut.com/products/gravity-industrial-stainless-steel-submersible-pressure-level-sensor-0-5m) — 1x  

---

## Wiring Diagrams

Full wiring diagrams are located in the `Wiring Diagrams` folder.

---

## Setup

### CLion
1. Install [CLion by Jetbrains](https://www.jetbrains.com/clion/).
1. Make sure the PlatformIO plugin is installed.
2. Clone the GitHub repo.
3. Open the project in CLion.
4. If you want the latest version just click build. If you want one of the other scripts use:
```bash
pio run -e SCRIPTNAME
```
5. Plug your ESP32 into your computer.
6. Click run. This will flash mainV2 onto the ESP32.

### Arduino IDE
1. Clone Repo
2. Remove platformio.ini file.
3. Rename each script with the .ino extension instead of .cpp.
4. Remove the #include <Arduino.h> line at the top of each script.
5. Download and install [Arduino IDE](https://www.arduino.cc/en/software/).  
6. Install the ESP32 board package by Espressif Systems using the Board Manager.  
7. Plug your ESP32 into your computer.  
8. Open Arduino IDE and select **ESP32 Dev Kit** from the board selection dropdown.  
9. Flash the code to the board.  

---

## Usage

After flashing the initial build, usage is straightforward:

- View data through the **Serial Monitor** when connected via USB, or  
- Access the ESP32 web server in your browser at:  
  `https://192.168.4.1`  
  (while connected to the ESP32’s Wi-Fi network)

---

## Extras

Other scripts included in this repository:

- `mainV1` — older version of the main code  
- `ACTUATOR_TEST` — actuator troubleshooting script  
- `PRESSURE_SENSOR` — pressure sensor testing script  

---

## Troubleshooting

- **ESP32 not connecting to your computer:**  
  Ensure the ESP32 board package is installed in Arduino IDE.

- **Webpage not loading:**  
  Confirm you are connected to `GEARS-Nemo_Data` and try opening the page in Firefox.

- **Nemo not returning after ~46 seconds:**  
  Possible causes:
  - Water ingress (manual recovery required), or  
  - Failure of the syringe buoyancy mechanism  

- **Need help?**  
  For any software or hardware-related issues, feel free to reach out for assistance.

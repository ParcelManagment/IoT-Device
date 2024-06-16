# Parcel Managemnet System

## Description

This project is a GPS and GPRS based railway package tracking system for monitoring and tracking packages in real-time. The system includes an OLED display for status updates. and a RFID for give the IDs to packages.

## Features

- Real-time GPS tracking
- GPRS communication for data transmission
- OLED display for package status
- PlatformIO project for ESP32 DevKit V1

## Hardware Requirements

- ESP32 DevKit V1
- SIM808 Module GPS & GPRS
- OLED display
- Necessary connecting wires and breadboard
- 18650 batteries for battery backup system
- Tacktile Push buttons

## Software Requirements

- PlatformIO
- Arduino framework
- VScode

## Setup Instructions

1. Clone the repository:
   ```sh
   git clone https://github.com/ParcelManagment/IoT-Device.git
   ```
2. Open the project in PlatformIO (You should setup the Platformio for ESP32 programming).
3. Build and upload the code to your ESP32 board.
4. Ensure all hardware components are properly connected.
5. Monitor the serial output for debug information.

## Usage

- Once deployed, the system will track packages in real-time.
- The OLED display will show status updates.

## Contributing

Please read `CONTRIBUTING.md` for details on our code of conduct, and the process for submitting pull requests to us.

## License

This project is licensed under the MIT License - see the `LICENSE` file for details.

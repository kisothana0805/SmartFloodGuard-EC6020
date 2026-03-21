# 🌊 SmartFloodGuard – EC6020

**IoT-Enabled Indoor Flood Entry Monitoring And Alerting System For Residential Safety**

---

## 📖 Introduction

SmartFloodGuard is an IoT-enabled embedded system developed to detect indoor flood entry at an early stage and provide real-time alerts to homeowners. Indoor flooding can occur unexpectedly, especially during heavy rainfall, often causing severe damage to household items and electrical systems.

This project integrates environmental sensors, microcontroller-based processing, RF wireless communication, and WiFi connectivity to ensure early detection and immediate notification. The system helps reduce property damage, improve safety, and enable remote monitoring through a web or mobile interface.

---

## 🔌 Circuit Design

The system is designed using a **two-node architecture** consisting of a Sensor Node and a Gateway Node.

### 🔹 Sensor Node (Transmitter Unit)

* Water Level Sensor – Detects presence of water
* Ultrasonic Sensor (HC-SR04) – Measures water level height
* ATmega328 Microcontroller – Processes sensor data
* RF Transmitter (433 MHz) – Sends alert signal
* 16 MHz Crystal Oscillator – Provides clock signal
* AMS1117 Voltage Regulator – Ensures stable power supply

### 🔹 Gateway Node (Receiver Unit)

* RF Receiver (433 MHz) – Receives transmitted signal
* ATmega328 Microcontroller – Validates received data
* ESP-01 WiFi Module (ESP8266) – Sends alert to cloud/web app
* Buzzer – Provides local alert
* LED – Visual indication of flood detection

📌 Complete circuit diagrams are available in the **hardware/** directory.

---

## 💻 Technologies Used

### 🔧 Hardware

* ATmega328 Microcontroller
* Water Level Sensor Module
* Ultrasonic Sensor (HC-SR04)
* RF Transmitter & Receiver Modules (433 MHz)
* ESP-01 WiFi Module (ESP8266)
* AMS1117 Voltage Regulator
* 16 MHz Crystal Oscillator
* Buzzer and LED
* Lithium-Ion Battery

### 💻 Software

* Embedded C / AVR Programming
* Arduino IDE
* ESP8266 WiFi Libraries
* HTML, CSS, JavaScript (Web Interface)
* GitHub Version Control

---

## 📚 References

* ATmega328 Datasheet – https://www.microchip.com
* ESP8266 Documentation – https://www.espressif.com
* Arduino Official Documentation – https://www.arduino.cc
* HC-SR04 Ultrasonic Sensor Datasheet
* IoT Web Development Resources

---

## 👥 Team & Mentors

### 👨‍💻 Group 05 Members

* 2022/E/102 – Dangshan N.
* 2022/E/104 – Kisothana P.
* 2021/E/117 – James P.S.V.
* 2021/E/148 – Thilookshan S.
* 2021/E/192 – Ajanthan T.
* 2018/E/107 – Sarugesh R.

### 🎓 Mentor / Supervisor

* Thanuja Uruththirakodeeswaran

---

## 📌 Course Information

EC6020 – Embedded Systems Design

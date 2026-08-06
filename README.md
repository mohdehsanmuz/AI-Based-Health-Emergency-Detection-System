#  AI-Based Health Emergency Detection System
 An AI-powered IoT healthcare monitoring system that continuously monitors a person's vital signs, detects health emergencies, and sends real-time alerts with GPS location using ESP32 and Blynk IoT.
## Overview

Health emergencies such as sudden cardiac events, low oxygen levels, or accidental falls require immediate attention. This project provides an AI and IoT-based solution that continuously monitors a user's health, predicts their health condition using Machine Learning, and automatically sends emergency alerts along with GPS coordinates to caregivers.

The system combines biomedical sensors, an ESP32 microcontroller, GPS tracking, and the Blynk IoT platform to enable real-time health monitoring from anywhere.

##  Features

- ❤️ Real-Time Heart Rate Monitoring
- 🫁 Blood Oxygen (SpO₂) Measurement
- 🌡 Body Temperature Monitoring
- 🚶 Fall Detection using Accelerometer
- 📍 Live GPS Location Tracking
- 🤖 AI-Based Health Condition Prediction
- 📱 Real-Time Dashboard on Blynk IoT
- 🚨 Automatic Emergency Alerts
- ☁ Wireless IoT Monitoring
- 🔋 Low-Cost Embedded Healthcare Solution


##  Hardware Components

| Component | Quantity |
|------------|----------|
| ESP32 DevKit V1 | 1 |
| MAX30102 Pulse Oximeter Sensor | 1 |
| DHT11 Temperature Sensor | 1 |
| ADXL345 Accelerometer | 1 |
| NEO-6M GPS Module | 1 |
| Breadboard | 1 |
| Jumper Wires | Several |
| USB Power Supply | 1 |

## Software & Tools
- Arduino IDE
- Python
- Scikit-Learn
- Joblib
- Blynk IoT
- Google Maps
- Git & GitHub

---

##  Machine Learning Model

The AI model predicts the user's health condition based on vital signs collected from sensors.

### Input Parameters

- Heart Rate (BPM)
- Blood Oxygen (SpO₂)
- Body Temperature (°C)
- Fall detction
### Predicted Output

-  Healthy
-  Moderate
-  Serious

The trained model helps identify abnormal health conditions before they become critical.

---

##  System Architecture

```text
          Health Sensors
                 │
                 ▼
      ESP32 Microcontroller
                 │
        Collect Sensor Data
                 │
                 ▼
      Machine Learning Model
                 │
      Predict Health Status
                 │
      ┌──────────┴──────────┐
      │                     │
 Healthy              Emergency
      │                     │
      ▼                     ▼
Update Dashboard     GPS Location
                           │
                           ▼
                 Send Alert via Blynk
                           │
                           ▼
                  Caregiver Notification
```

## Workflow

1. Read health data from sensors.
2. Process sensor readings using ESP32.
3. Predict health condition using the AI model.
4. Detect abnormal conditions or falls.
5. Obtain live GPS coordinates.
6. Send emergency alerts through Blynk IoT.
7. Display real-time data on the mobile dashboard.

##  Blynk Dashboard

The Blynk IoT dashboard displays:

- Heart Rate
- SpO₂
- Body Temperature
- GPS Location
- Emergency Alerts

##  Applications

- Elderly Healthcare Monitoring
- Home Healthcare
- Remote Patient Monitoring
- Hospitals
- Smart Healthcare Systems
- Emergency Response Systems

##  Future Enhancements

- ECG Sensor Integration
- Cloud Database Storage
- Mobile Application
- Doctor Dashboard
- Wearable Device Integration
- Voice Assistant Support
- AI-Based Disease Prediction
- SMS & Email Notifications

---


Add images of:


```markdown
![Prototype](Images/prototype.jpg)

![Circuit Diagram](Images/circuit.png)

![Blynk Dashboard](Images/blynk_dashboard.png)
```

---


##  Results

- Successfully monitored vital signs in real time.
- AI model accurately classified health conditions.
- Emergency alerts were transmitted instantly through Blynk.
- GPS location was successfully shared during emergencies.
- Designed as a low-cost and scalable healthcare solution.



## Author

**Mohd Ehsan Muzammil**


This project is licensed under the MIT---

⭐ If you found this project useful, please consider giving it a **Star** on GitHub!

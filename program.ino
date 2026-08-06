/* ESP32 + Blynk + MAX3010 + ADXL345 + DHT11 + NEO-6M GPS + rolling avg HR (12 samples)
   Requirements: TinyGPSPlus, ArduinoJson, Blynk, MAX30100_PulseOximeter, Adafruit libs, DHT
*/

#define BLYNK_TEMPLATE_ID "TMPL38LVRstde"
#define BLYNK_TEMPLATE_NAME "AI BASED HEALTH EMERGENCY DETECTION USING IOT"
#define BLYNK_AUTH   "vGUb1F4cLZTB18tIQoL6nyhmnXf_aghW"

#include <Wire.h>
#include <BlynkSimpleEsp32.h>
#include "MAX30100_PulseOximeter.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include "DHT.h"
#include <Ticker.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TinyGPS++.h>
#include <math.h>

#define WIFI_SSID    "Abc123"
#define WIFI_PASS    "0987654321"

#define SDA_PIN 21
#define SCL_PIN 22

#define DHTPIN 18
#define DHTTYPE DHT11
#define BUZZER_PIN 15

#define GPS_RX_PIN 16  // GPS TX -> ESP32 RX2
#define GPS_TX_PIN 17  // GPS RX -> ESP32 TX2
#define GPS_BAUD    9600

// Virtual pins
#define VP_HR          V0
#define VP_TEMP        V1
#define VP_SPO2        V3
#define VP_ACCEL       V4
#define VP_BEAT        V5
#define VP_HR_ALERT    V10
#define VP_SPO2_ALERT  V11
#define VP_FALL_ALERT  V12
#define VP_LAT         V13
#define VP_LON         V14
#define VP_AVG_HR      V20

#define FALL_THRESHOLD_G 3.0f
#define BUZZER_DURATION_MS 2500UL
#define SEND_INTERVAL_MS 1000UL
#define POX_UPDATE_MS 50UL

#define HR_SAMPLES 12
#define HR_SAMPLE_INTERVAL_MS 5000UL

PulseOximeter pox;
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
DHT dht(DHTPIN, DHTTYPE);
Ticker poxTicker;
TinyGPSPlus gps;
HardwareSerial GPSserial(2);
BlynkTimer timer;

volatile bool beatDetectedFlag = false;
bool fallDetected = false;
unsigned long fallDetectedAt = 0;
unsigned long buzzerEndAt = 0;
unsigned long lastSend = 0;

double lastLat = 0.0;
double lastLon = 0.0;
bool haveGpsFix = false;

float hrBuffer[HR_SAMPLES];
uint8_t hrIndex = 0;
uint8_t hrCount = 0;

// avg HR variable (updated by hrSampleTask)
float avgHR = 0.0f;
// previous HR alert state to avoid repeated events
String prevHrState = "Normal";

float ms2_to_g(float accel_ms2) { return accel_ms2 / 9.80665f; }

bool getIPLocation(float &outLat, float &outLon) {
  if (WiFi.status() != WL_CONNECTED) return false;
  HTTPClient http;
  http.begin("http://ip-api.com/json");
  int httpCode = http.GET();
  if (httpCode != 200) { http.end(); return false; }
  String payload = http.getString();
  http.end();

  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, payload)) return false;
  if (!doc.containsKey("status")) return false;
  if (String((const char*)doc["status"]) != "success") return false;
  outLat = doc["lat"].as<float>();
  outLon = doc["lon"].as<float>();
  return true;
}

void onBeatDetected() { beatDetectedFlag = true; }
void poxUpdateTick() { pox.update(); }

void hrSampleTask() {
  // get instantaneous HR and push into circular buffer
  float hr = pox.getHeartRate();
  if (isnan(hr) || hr < 0.0f) hr = 0.0f;

  hrBuffer[hrIndex] = hr;
  hrIndex = (hrIndex + 1) % HR_SAMPLES;
  if (hrCount < HR_SAMPLES) hrCount++;

  // compute average of non-zero samples
  float sum = 0.0f; uint8_t valid = 0;
  for (uint8_t i = 0; i < hrCount; ++i) {
    float v = hrBuffer[i];
    if (v > 0.0f) { sum += v; valid++; }
  }
  if (valid > 0) avgHR = sum / (float)valid;
  else avgHR = 0.0f;

  // publish avg to Blynk
  Blynk.virtualWrite(VP_AVG_HR, avgHR);

  Serial.print("AVG HR ("); Serial.print(valid); Serial.print("/"); Serial.print(hrCount); Serial.print(") = ");
  Serial.println(avgHR);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  pinMode(BUZZER_PIN, OUTPUT); digitalWrite(BUZZER_PIN, LOW);
  Wire.begin(SDA_PIN, SCL_PIN);

  GPSserial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  Blynk.begin(BLYNK_AUTH, WIFI_SSID, WIFI_PASS);

  Serial.println("Init MAX30100...");
  if (!pox.begin()) Serial.println("POX init failed");
  else { pox.setOnBeatDetectedCallback(onBeatDetected); }

  Serial.println("Init ADXL345...");
  if (!accel.begin()) Serial.println("ADXL345 init failed");
  else accel.setRange(ADXL345_RANGE_16_G);

  dht.begin();

  poxTicker.attach_ms(POX_UPDATE_MS, poxUpdateTick);
  timer.setInterval(HR_SAMPLE_INTERVAL_MS, hrSampleTask);

  for (uint8_t i = 0; i < HR_SAMPLES; ++i) hrBuffer[i] = 0.0f;

  lastSend = millis();
  Serial.println("Setup complete.");
}

void loop() {
  Blynk.run();
  timer.run();

  while (GPSserial.available()) {
    char c = (char)GPSserial.read();
    gps.encode(c);
  }
  if (gps.location.isValid()) {
    lastLat = gps.location.lat();
    lastLon = gps.location.lng();
    haveGpsFix = true;
  }

  if (beatDetectedFlag) {
    beatDetectedFlag = false;
    Blynk.virtualWrite(VP_BEAT, 1);
    delay(40);
    Blynk.virtualWrite(VP_BEAT, 0);
    Serial.println("Beat!");
  }

  if (buzzerEndAt && millis() > buzzerEndAt) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerEndAt = 0;
  }

  if (millis() - lastSend >= SEND_INTERVAL_MS) {
    lastSend = millis();
    sendSensorData();
  }
  delay(2);
}

void sendSensorData() {
  // read instantaneous sensors
  float heartRate = pox.getHeartRate();
  float spO2 = pox.getSpO2();
  if (isnan(heartRate) || heartRate < 0.0f) heartRate = 0.0f;
  if (isnan(spO2) || spO2 < 0.0f) spO2 = 0.0f;

  Serial.print("HR:"); Serial.print(heartRate); Serial.print(" SpO2:"); Serial.println(spO2);

  float tempC = dht.readTemperature();
  if (isnan(tempC)) { tempC = 0.0f; Serial.println("DHT failed"); }

  sensors_event_t event;
  float accelMagG = 0.0f;
  if (accel.getEvent(&event)) {
    float ax = ms2_to_g(event.acceleration.x);
    float ay = ms2_to_g(event.acceleration.y);
    float az = ms2_to_g(event.acceleration.z);
    accelMagG = sqrtf(ax*ax + ay*ay + az*az);
  }

  bool justDetectedFall = false;
  if (!fallDetected && accelMagG > FALL_THRESHOLD_G) {
    fallDetected = true;
    fallDetectedAt = millis();
    justDetectedFall = true;
    Serial.println("FALL DETECTED!");
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerEndAt = millis() + BUZZER_DURATION_MS;
    Blynk.virtualWrite(VP_FALL_ALERT, "FALL DETECTED!");
    // wait to call Blynk.logEvent until we have a map or fallback
  }

  if (fallDetected && millis() - fallDetectedAt > 10000UL) {
    fallDetected = false;
    Blynk.virtualWrite(VP_FALL_ALERT, "Normal");
  }

  // publish numeric sensor values
  Blynk.virtualWrite(VP_HR, heartRate);
  Blynk.virtualWrite(VP_SPO2, spO2);
  Blynk.virtualWrite(VP_TEMP, tempC);
  Blynk.virtualWrite(VP_ACCEL, accelMagG);

  // ---- USE avgHR for alerts ----
  String hrStatus = "Normal";
  if (avgHR > 120.0f) hrStatus = "HIGH AVERAGE HR!";
  else if (avgHR > 0.0f && avgHR < 50.0f) hrStatus = "LOW AVERAGE HR!";

  Blynk.virtualWrite(VP_HR_ALERT, hrStatus);

  // log event only when state changes and it's not Normal
  if (hrStatus != prevHrState) {
    if (hrStatus == "HIGH AVERAGE HR!") {
      String msg = "Avg HR high: " + String(avgHR,1);
      Blynk.logEvent("heart_high", msg);
    } else if (hrStatus == "LOW AVERAGE HR!") {
      String msg = "Avg HR low: " + String(avgHR,1);
      Blynk.logEvent("heart_low", msg);
    }
    prevHrState = hrStatus;
  }

  // SPO2 alerts (unchanged)
  String spo2Status = "Normal";
  if (spO2 > 0.0f && spO2 < 93.0f) {
    spo2Status = "LOW SPO2!";
    if (spo2Status != String("LOW SPO2!")) Blynk.logEvent("spo2_low", "SpO2 below 93%");
  }
  Blynk.virtualWrite(VP_SPO2_ALERT, spo2Status);

  // FALL + location handling (try GPS first, then short wait, then IP fallback)
  if (justDetectedFall) {
    bool sentLocation = false;
    String mapLink;

    // 1) if we already have a GPS fix
    if (haveGpsFix && gps.location.isValid()) {
      mapLink = "https://www.google.com/maps?q=" + String(lastLat, 6) + "," + String(lastLon, 6);
      Blynk.virtualWrite(VP_LAT, String(lastLat, 6));
      Blynk.virtualWrite(VP_LON, String(lastLon, 6));
      Blynk.virtualWrite(VP_FALL_ALERT, mapLink);
      Serial.println("GPS LOCATION SENT: " + mapLink);
      sentLocation = true;
      Blynk.logEvent("fall_alert", mapLink);
    }

    // 2) try wait short while for GPS fix (up to 10s)
    if (!sentLocation) {
      unsigned long start = millis();
      while (millis() - start < 10000UL) { // 10 seconds
        while (GPSserial.available()) gps.encode(GPSserial.read());
        Serial.print("GPS valid? "); Serial.print(gps.location.isValid());
        Serial.print(" lat: "); Serial.print(gps.location.lat(),6);
        Serial.print(" lon: "); Serial.println(gps.location.lng(),6);
        if (gps.location.isValid()) {
          lastLat = gps.location.lat();
          lastLon = gps.location.lng();
          haveGpsFix = true;
          mapLink = "https://www.google.com/maps?q=" + String(lastLat, 6) + "," + String(lastLon, 6);
          Blynk.virtualWrite(VP_LAT, String(lastLat, 6));
          Blynk.virtualWrite(VP_LON, String(lastLon, 6));
          Blynk.virtualWrite(VP_FALL_ALERT, mapLink);
          Serial.println("GPS LOCATION (after wait) SENT: " + mapLink);
          sentLocation = true;
          Blynk.logEvent("fall_alert", mapLink);
          break;
        }
        delay(250);
      }
    }

    // 3) IP geolocation fallback
    if (!sentLocation) {
      float latf = 0.0f, lonf = 0.0f;
      if (getIPLocation(latf, lonf)) {
        String ipMap = "https://www.google.com/maps?q=" + String(latf,6) + "," + String(lonf,6);
        Blynk.virtualWrite(VP_LAT, String(latf,6));
        Blynk.virtualWrite(VP_LON, String(lonf,6));
        Blynk.virtualWrite(VP_FALL_ALERT, ipMap);
        Serial.println("IP GEOLOCATION SENT: " + ipMap);
        Blynk.logEvent("fall_alert", ipMap);
        sentLocation = true;
      } else {
        String txt = "Location unavailable";
        Blynk.virtualWrite(VP_FALL_ALERT, "FALL detected! " + txt);
        Serial.println("LOCATION UNAVAILABLE");
        Blynk.logEvent("fall_alert", txt);
      }
    }
  } // end justDetectedFall
}

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "secrets.h"

// ========================================================
// 1. SENSOR AND TIMING CONFIGURATION
// ========================================================

const uint8_t LDR_PIN = 36;

// Serial updates every 100 ms
const unsigned long SERIAL_INTERVAL_MS = 100;

// Cloud updates every 15 seconds
const unsigned long CLOUD_INTERVAL_MS = 15000;

// Device-specific Firebase path
const String DEVICE_PATH = String("devices/") + USER_UID;

// ========================================================
// 2. FIREBASE OBJECTS
// ========================================================

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ========================================================
// 3. SHARED SENSOR DATA
// ========================================================

float ldrVoltage = 0.0f;
int adcValue = 0;
bool hasSensorReading = false;

// Protect shared sensor data across FreeRTOS tasks
portMUX_TYPE sensorMux = portMUX_INITIALIZER_UNLOCKED;

// ========================================================
// 4. TIMERS AND STATUS
// ========================================================

unsigned long lastSerialReadMs = 0;
unsigned long lastWiFiLogMs = 0;

bool wifiMessagePrinted = false;

// ========================================================
// 5. FIREBASE TASK
// ========================================================

void firebaseTask(void *parameter) {
  Serial.println("[FirebaseTask] Waiting for Wi-Fi...");

  // Wait for Wi-Fi without blocking the sensor loop
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(500));
  }

  Serial.println("[FirebaseTask] Wi-Fi connected.");

  // Configure Firebase authentication
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  config.token_status_callback = tokenStatusCallback;

  // Limit socket and server-response waiting time
  config.timeout.socketConnection = 10000;
  config.timeout.serverResponse = 10000;

  Serial.println("[FirebaseTask] Starting Firebase.begin()...");

  // This call may take time, but it runs separately from
  // the sensor-reading loop.
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("[FirebaseTask] Firebase.begin() returned.");

  unsigned long lastCloudWriteMs = 0;
  int count = 0;

  // Keep handling Firebase authentication and cloud writes
  for (;;) {
    unsigned long now = millis();

    // Firebase.ready() handles ongoing authentication tasks
    if (Firebase.ready() &&
        (lastCloudWriteMs == 0 ||
         now - lastCloudWriteMs >= CLOUD_INTERVAL_MS)) {

      bool readingAvailable;
      float voltageToSend;

      // Read a consistent snapshot of the latest sensor data
      portENTER_CRITICAL(&sensorMux);
      readingAvailable = hasSensorReading;
      voltageToSend = ldrVoltage;
      portEXIT_CRITICAL(&sensorMux);

      if (readingAvailable) {
        lastCloudWriteMs = now;

        int countToSend = count++;

        // Write count to test/int
        bool intAccepted = Firebase.RTDB.setIntAsync(
          &fbdo,
          DEVICE_PATH + "/test/int",
          countToSend
        );

        // Write voltage to test/float
        bool floatAccepted = Firebase.RTDB.setFloatAsync(
          &fbdo,
          DEVICE_PATH + "/test/float",
          voltageToSend
        );

        Serial.printf(
          "[CLOUD] int=%s | float=%s | voltage=%.3f V\n",
          intAccepted ? "ACCEPTED" : "FAILED",
          floatAccepted ? "ACCEPTED" : "FAILED",
          voltageToSend
        );
      }
    }

    // Allow the task to yield CPU time
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

/* ========================================================
   FIREBASE TASK NOTES
   - Firebase authentication runs separately from loop().
   - A stalled Firebase initialization should not stop the
     Arduino loop from printing sensor readings.
   - ACCEPTED means the async request was accepted by the
     library, not that the database confirmed the write.
   ======================================================== */

// ========================================================
// 6. SETUP
// ========================================================

void setup() {
  Serial.begin(115200);

  // Configure ADC
  pinMode(LDR_PIN, INPUT);
  analogReadResolution(12);

  Serial.println();
  Serial.println("================================");
  Serial.println(" ESP32 LDR Monitor + Firebase");
  Serial.println("================================");

  Serial.printf("LDR GPIO: %u\n", LDR_PIN);
  Serial.printf("Serial interval: %lu ms\n", SERIAL_INTERVAL_MS);
  Serial.printf("Cloud interval: %lu ms\n", CLOUD_INTERVAL_MS);

  // Start Wi-Fi connection without waiting for success
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("[WiFi] Connection started.");

  // Run Firebase on a separate FreeRTOS task pinned to core 0.
  // The Arduino loop can continue sampling and printing on core 1.
  BaseType_t taskCreated = xTaskCreatePinnedToCore(
    firebaseTask,
    "FirebaseTask",
    12288,
    nullptr,
    1,
    nullptr,
    0
  );

  if (taskCreated == pdPASS) {
    Serial.println("[SYSTEM] Firebase task created.");
  } else {
    Serial.println("[ERROR] Failed to create Firebase task.");
  }

  Serial.println("[SYSTEM] Setup completed.");
}

/* ========================================================
   SETUP NOTES
   - No Firebase.begin() call in setup().
   - setup() returns quickly so loop() can start.
   - Firebase task creation failure is reported on Serial.
   ======================================================== */

// ========================================================
// 7. MAIN LOOP: FAST SENSOR MONITORING
// ========================================================

void loop() {
  unsigned long now = millis();

  // ------------------------------------------------------
  // A. READ SENSOR AND PRINT EVERY 100 ms
  // ------------------------------------------------------

  if (now - lastSerialReadMs >= SERIAL_INTERVAL_MS) {
    lastSerialReadMs = now;

    // Read calibrated ADC voltage in millivolts
    uint32_t voltageMilliVolts = analogReadMilliVolts(LDR_PIN);
    float voltage = voltageMilliVolts / 1000.0f;

    // Publish the latest sensor values to the Firebase task
    portENTER_CRITICAL(&sensorMux);
    ldrVoltage = voltage;
    hasSensorReading = true;
    portEXIT_CRITICAL(&sensorMux);

    // Print independently of Firebase state
    Serial.printf(
      "[LDR] Voltage: %.3f V\n",
      voltage
    );
  }

  // ------------------------------------------------------
  // B. REPORT WI-FI STATUS WITHOUT BLOCKING
  // ------------------------------------------------------

  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiMessagePrinted) {
      wifiMessagePrinted = true;
      Serial.print("[WiFi] Connected. IP: ");
      Serial.println(WiFi.localIP());
    }
  } else if (now - lastWiFiLogMs >= 3000) {
    lastWiFiLogMs = now;
    Serial.println("[WiFi] Still connecting...");
  }

  // No delay(): keep the sensor loop responsive.
}

/* ==========================================================================
   SECTION 1: Nạp các thư viện cần thiết
   ========================================================================== */
#include <Arduino.h>
#include <Firebase_ESP_Client.h>

#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

/* ==========================================================================
   SECTION 2: Nạp các helper addons
   ========================================================================== */
// Cung cấp thông tin quá trình tạo token.
#include "addons/TokenHelper.h"

// Cung cấp hàm hỗ trợ in dữ liệu RTDB.
#include "addons/RTDBHelper.h"

#include "secrets.h"

/* ==========================================================================
   SECTION 3: Khai báo thông tin mạng Wi-Fi
   ========================================================================== */
// WIFI_SSID, WIFI_PASSWORD được khai báo trong secrets.h.

/* ==========================================================================
   SECTION 4: Khai báo Firebase API Key
   ========================================================================== */
// API_KEY được khai báo trong secrets.h.

/* ==========================================================================
   SECTION 5: Khai báo Firebase Realtime Database URL
   ========================================================================== */
// DATABASE_URL được khai báo trong secrets.h.

/* ==========================================================================
   SECTION 6: Khởi tạo Firebase và các biến điều khiển
   ========================================================================== */
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
const unsigned long READ_INTERVAL = 15000;

bool signupOK = false;

int intValue;
float floatValue;

// GPIO điều khiển LED.
const int LED_PIN = 2;

// Lưu trạng thái LED đọc từ Firebase.
bool ledState = false;

/* ==========================================================================
   SECTION 7: Khởi tạo Serial, GPIO và kết nối Wi-Fi
   ========================================================================== */
void setup() {
  Serial.begin(115200);

  // Khởi tạo GPIO2 làm ngõ ra, mặc định tắt LED.
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* ========================================================================
     SECTION 8: Cấu hình Firebase API Key và RTDB URL
     ======================================================================== */
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  /* ========================================================================
     SECTION 9: Cấu hình xác thực Email/Password
     ======================================================================== */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Cho phép loop xử lý khi Firebase sẵn sàng.
  // Firebase.begin() sẽ thực hiện quá trình xác thực.
  signupOK = true;

  /* ========================================================================
     SECTION 10: Callback Token và khởi động Firebase
     ======================================================================== */
  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

/* ==========================================================================
   SECTION 11: Đọc dữ liệu và điều khiển LED
   ========================================================================== */
void loop() {
  if (Firebase.ready() && signupOK &&
      (millis() - sendDataPrevMillis >= READ_INTERVAL ||
       sendDataPrevMillis == 0)) {

    // Cập nhật mốc thời gian đọc dữ liệu.
    sendDataPrevMillis = millis();

    /* ======================================================================
       SECTION 12: Đọc số nguyên từ test/int
       ====================================================================== */
    if (Firebase.RTDB.getInt(&fbdo, "/test/int")) {
      if (fbdo.dataType() == "int") {
        intValue = fbdo.intData();

        Serial.printf("[RTDB] test/int = %d\n", intValue);
      }
    } else {
      Serial.printf("[RTDB] Lỗi đọc test/int: %s\n",
                    fbdo.errorReason().c_str());
    }

    /* ======================================================================
       SECTION 13: Đọc số thực từ test/float
       ====================================================================== */
    if (Firebase.RTDB.getFloat(&fbdo, "/test/float")) {
      if (fbdo.dataType() == "float") {
        floatValue = fbdo.floatData();

        Serial.printf("[RTDB] test/float = %.2f\n", floatValue);
      }
    } else {
      Serial.printf("[RTDB] Lỗi đọc test/float: %s\n",
                    fbdo.errorReason().c_str());
    }

    /* ======================================================================
       SECTION 14: Đọc trạng thái LED từ test/ledState
       ====================================================================== */
    if (Firebase.RTDB.getBool(&fbdo, "/test/ledState")) {
      if (fbdo.dataType() == "boolean") {
        ledState = fbdo.boolData();

        Serial.printf("[RTDB] test/ledState = %s\n",
                      ledState ? "true" : "false");

        /* ------------------------------------------------------------------
           FIX: Điều khiển LED theo giá trị boolean từ Firebase.
           true  -> GPIO2 HIGH -> bật LED.
           false -> GPIO2 LOW  -> tắt LED.
           ------------------------------------------------------------------ */
        digitalWrite(LED_PIN, ledState ? HIGH : LOW);

        Serial.printf("[GPIO%d] LED %s\n",
                      LED_PIN,
                      ledState ? "ON" : "OFF");
      } else {
        Serial.printf("[RTDB] Sai kiểu dữ liệu test/ledState: %s\n",
                      fbdo.dataType().c_str());
      }
    } else {
      Serial.printf("[RTDB] Lỗi đọc test/ledState: %s\n",
                    fbdo.errorReason().c_str());
    }
  }
}

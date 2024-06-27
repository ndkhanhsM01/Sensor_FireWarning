#include <WiFi.h>
#include <HTTPClient.h>

const char* fireWarningSignal = "FIRE";
const char* gasWarningSignal = "GAS";

// Thông tin WiFi
const char* ssid = "Đình Khánh"; // Thay bằng SSID WiFi của bạn
const char* password = "13578642"; // Thay bằng mật khẩu WiFi của bạn

// Thông tin HTTP Request
const char* httpsRequest = "https://warningfire-notice.onrender.com/notify";
const char* requestFireWarning = "{\"message\":\"FIRE\"}";
const char* requestGasWarning = "{\"message\":\"GAS\"}";

// Chân LED WiFi
const int ledWifi = 32;

// Semaphore để đồng bộ hóa giữa các nhiệm vụ
SemaphoreHandle_t xSemaphore = NULL;

// Trạng thái yêu cầu HTTP
volatile bool requested = false;

void setup() {
  delay(1000);

  Serial.begin(9600); // Khởi động Serial giao tiếp với Arduino
  Serial.println("ESP32 ready");

  RegistIO();
  digitalWrite(ledWifi, LOW); 

  // Tạo semaphore
  xSemaphore = xSemaphoreCreateMutex();

  // Tạo các nhiệm vụ RTOS
  RegistTasks();
}

void RegistIO(){
  pinMode(ledWifi, OUTPUT); 
}

void RegistTasks(){
  xTaskCreatePinnedToCore(
    handleConnectWifi, /* Nhiệm vụ quản lý WiFi */
    "Connect WiFi Task", 
    4096, 
    NULL, 
    1, 
    NULL, 
    1); 

  xTaskCreatePinnedToCore(
    handleFireWarning, /* Nhiệm vụ xử lý thông báo từ Arduino */
    "Handle Fire Task", 
    8192, 
    NULL, 
    1, 
    NULL, 
    1); 
}

void handleConnectWifi(void * parameter) {
  // Kết nối WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    digitalWrite(ledWifi, LOW); 
  }
  Serial.println("Connected to WiFi");
  pingpongLedWifi();

  while (true) {
    // Kiểm tra trạng thái WiFi
    if (WiFi.status() != WL_CONNECTED) {
      digitalWrite(ledWifi, LOW); 
    } else {
      digitalWrite(ledWifi, HIGH); 
    }
    delay(500);
  }
}

void handleFireWarning(void * parameter) {
  while (true) {
    if (Serial.available() && !requested) {
      String message = Serial.readStringUntil('\n');
      message.trim(); // Loại bỏ các khoảng trắng ở đầu và cuối chuỗi
      Serial.println("Message from Arduino: " + message);

      if (message == fireWarningSignal || message == gasWarningSignal) {
        xSemaphoreTake(xSemaphore, portMAX_DELAY);
        requested = true;
        xSemaphoreGive(xSemaphore);

        // Gửi yêu cầu HTTP
        if (WiFi.status() == WL_CONNECTED) {
          HTTPClient http;
          http.begin(httpsRequest); // Thay bằng URL của bạn

          http.addHeader("Content-Type", "application/json");

          const char* reqBody = message == fireWarningSignal ? requestFireWarning : requestGasWarning;
          int httpResponseCode = http.POST(reqBody);

          Serial.println("Code: " + String(httpResponseCode));
          if (httpResponseCode == 200) {
            String response = http.getString();
            Serial.println("HTTP Response code: " + String(httpResponseCode));
            Serial.println("Response: " + response);
          } else {
            Serial.println("Error on HTTP request");
          }

          http.end();
          xSemaphoreTake(xSemaphore, portMAX_DELAY);
          requested = false;
          xSemaphoreGive(xSemaphore);
        } else {
          Serial.println("WiFi not connected");
        }
      }
    }
    delay(500);
  }
}

void pingpongLedWifi() {
  digitalWrite(ledWifi, HIGH); 
  delay(500);
  digitalWrite(ledWifi, LOW); 
  delay(500);
  digitalWrite(ledWifi, HIGH); 
  delay(500);
  digitalWrite(ledWifi, LOW); 
}

void loop() {
  // Không cần sử dụng vòng lặp chính vì RTOS sẽ quản lý các nhiệm vụ
}

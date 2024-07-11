#include <WiFi.h>
#include <HTTPClient.h>

const char* fireWarningSignal = "FIRE";

// Thông tin WiFi
const char* ssid = "MI 8 SE"; 
const char* password = "12345678923";

// Thông tin HTTP Request
const char* httpsRequest = "https://warningfire-notice.onrender.com/notify";
const char* requestFireWarning = "{\"type\": \"WARNING\",\"message\":\"FIRE\"}";

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
  requested = false;
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

      if (message == fireWarningSignal) {
        xSemaphoreTake(xSemaphore, portMAX_DELAY);
        requested = true;
        xSemaphoreGive(xSemaphore);

        // Gửi yêu cầu HTTP
        if (WiFi.status() == WL_CONNECTED) {
          SendRequestWarning();
          xSemaphoreTake(xSemaphore, portMAX_DELAY);
          requested = false;
          xSemaphoreGive(xSemaphore);
        } else {
          Serial.println("WiFi not connected");
        }
      }
    }
    else{
      //Serial.println("Something wrong");
    }
    //delay(500);
  }
}

void SendRequestWarning(){
  HTTPClient http;
  http.begin(httpsRequest); 

  http.addHeader("Content-Type", "application/json");

  const char* reqBody = requestFireWarning ;
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

#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <LiquidCrystal_I2C.h> 
#include <Wire.h>
// input
const byte firePin = A0; // cảm biến lửa

// output
const byte alertPin = 8; // Chân kết nối với Buzzer hoặc LED cảnh báo
LiquidCrystal_I2C lcd(0x27,  16, 2);
// config values
const int fireThreshold = 200; // Ngưỡng để phát hiện lửa (điều chỉnh tùy theo cảm biến)
const char* fireWarning = "FIRE";

// Semaphore để đồng bộ hóa giữa các nhiệm vụ
SemaphoreHandle_t xSemaphore = NULL;
// Biến để lưu giá trị cảm biến
volatile int fireValue = 0;
volatile bool isFireDetected = false; 

TaskHandle_t taskReadSensor;
TaskHandle_t taskHandleFireData;
TaskHandle_t taskDisplayData;
TaskHandle_t taskHandleWarning;
void setup() {
  RegistIO();
  Serial.begin(9600); // Khởi động Serial giao tiếp với ESP32
  
  xSemaphore = xSemaphoreCreateMutex();
  isFireDetected = false;
  RegistTasks();
}

void loop(){
  // do not anything
}

void RegistIO(){
  lcd.init();        
  lcd.backlight();
  lcd.print("Fire: ");
  lcd.setCursor(3, 1);
  lcd.print("--NORMAL--");

  pinMode(firePin, INPUT); // Cấu hình chân firePin làm đầu vào
  pinMode(alertPin, OUTPUT); // Cấu hình chân alertPin làm đầu ra
  digitalWrite(alertPin, LOW); // Tắt Buzzer hoặc LED cảnh báo ban đầu
}

void RegistTasks(){
  // Tạo tác vụ đọc cảm biến
  xTaskCreate(HandleReadSensor, "Read Sensor", 128, NULL, 2, &taskReadSensor);

  // Tạo tác vụ hiển thị dữ liệu
  xTaskCreate(HandleDisplayData, "Display Data", 128, NULL, 2, &taskDisplayData);

  xTaskCreate(HandleWarning, "Handle Warning", 128, NULL, 2, &taskHandleWarning);
}

// Tác vụ đọc cảm biến
void HandleReadSensor(void *pvParameters) {
  (void) pvParameters;

  while (true) {
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    fireValue = analogRead(firePin);
    xSemaphoreGive(xSemaphore);
    // show fire value
    Serial.print("Sensor Value: ");
    Serial.println(fireValue); // Hiển thị giá trị đọc được từ cảm biến trên Serial Monitor
    Serial.println("--------");

    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    if (!isFireDetected && fireValue < fireThreshold) { // Kiểm tra xem giá trị có vượt ngưỡng hay không
      Serial.println("ok");
      isFireDetected = true;
    }
    xSemaphoreGive(xSemaphore);
  }
}

// Tác vụ canh bao
void HandleWarning(void *pvParameters) {
  (void) pvParameters;

  while(true){
    if(isFireDetected){
      isFireDetected = false;
      xSemaphoreTake(xSemaphore, portMAX_DELAY);
      vTaskSuspend(taskReadSensor);
      Serial.println("");
      Serial.println(fireWarning); // Gửi tín hiệu cảnh báo lửa đến ESP32
      digitalWrite(alertPin, HIGH); // Bật Buzzer hoặc LED cảnh báo

      lcd.setCursor(3, 1);
      lcd.print("--FIRE--");
      
      vTaskDelay(3000 / portTICK_PERIOD_MS); // Cảnh báo trong 3 giây

      // end
      isFireDetected = false;
      digitalWrite(alertPin, LOW); // Tắt Buzzer hoặc LED cảnh báo

      lcd.setCursor(3, 1);
      lcd.print("--NORMAL--");
      vTaskResume(taskReadSensor);
      xSemaphoreGive(xSemaphore);
    }
  }
}

void HandleDisplayData(void *pvParameters) {
  (void) pvParameters;

  while (true) {
    //xSemaphoreTake(xSemaphore, portMAX_DELAY);
    lcd.setCursor(6, 0);
    lcd.print(fireValue);
    lcd.print("  "); // Xóa các ký tự thừa nếu có

    // status
    lcd.setCursor(0, 1);
    /*if (isFireDetected) {
      lcd.print("--FIRE--");
    } else {
      lcd.print("--NORMAL--");
    }*/
    //xSemaphoreGive(xSemaphore);

    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}


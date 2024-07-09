#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <LiquidCrystal_I2C.h> 
#include <Wire.h>
// input
const int firePin = A0; // cảm biến lửa
const int gasPin = A1; // cảm biến khí gas

// output
const int alertPin = 8; // Chân kết nối với Buzzer hoặc LED cảnh báo
LiquidCrystal_I2C lcd(0x27,  16, 2);
// config values
const int intervalWarning = 10000;
const int fireThreshold = 100; // Ngưỡng để phát hiện lửa (điều chỉnh tùy theo cảm biến)
const int gasThreshold = 500; 
const int gasStep = 150;
const char* fireWarning = "FIRE";
const char* gasWarning = "GAS";

// Semaphore để đồng bộ hóa giữa các nhiệm vụ
SemaphoreHandle_t xSemaphore = NULL;
// Biến để lưu giá trị cảm biến
volatile int fireValue = 0;
volatile int gasValue = 0;

TaskHandle_t taskReadSensor;
TaskHandle_t taskHandleFireData;
TaskHandle_t taskDisplayData;
void setup() {
  RegistIO();
  Serial.begin(9600); // Khởi động Serial giao tiếp với ESP32
  
  xSemaphore = xSemaphoreCreateMutex();
  RegistTasks();
}

void loop(){
  // do not anything
}

void RegistIO(){
  lcd.init();        
  lcd.backlight();
  lcd.print("Fire: ");
  lcd.setCursor(0, 1);
  lcd.print("Gas: ");

  pinMode(firePin, INPUT); // Cấu hình chân firePin làm đầu vào
  pinMode(gasPin, INPUT); // 
  pinMode(alertPin, OUTPUT); // Cấu hình chân alertPin làm đầu ra
  digitalWrite(alertPin, LOW); // Tắt Buzzer hoặc LED cảnh báo ban đầu
}

void RegistTasks(){
  // Tạo tác vụ đọc cảm biến
  xTaskCreate(
    HandleReadSensor, // Hàm thực hiện tác vụ
    "Read Sensor", // Tên tác vụ
    128, // Kích thước ngăn xếp (đơn vị là từ)
    NULL, // Tham số truyền vào tác vụ
    2, // Độ ưu tiên tác vụ
    &taskReadSensor // Con trỏ xử lý tác vụ
  );

  // Tạo tác vụ gửi dữ liệu
  xTaskCreate(
    HandleFireData, // Hàm thực hiện tác vụ
    "Handle Fire Data", // Tên tác vụ
    128, // Kích thước ngăn xếp (đơn vị là từ)
    NULL, // Tham số truyền vào tác vụ
    1, // Độ ưu tiên tác vụ
    &taskHandleFireData // Con trỏ xử lý tác vụ
  );

  xTaskCreate(
    HandleDisplayData, // Hàm thực hiện tác vụ
    "Display Data", // Tên tác vụ
    128, // Kích thước ngăn xếp (đơn vị là từ)
    NULL, // Tham số truyền vào tác vụ
    1, // Độ ưu tiên tác vụ
    &taskDisplayData // Con trỏ xử lý tác vụ
  );
}

// Tác vụ đọc cảm biến
void HandleReadSensor(void *pvParameters) {
  (void) pvParameters;

  while (true) {
    fireValue = analogRead(firePin); // Đọc giá trị từ cảm biến hồng ngoại
    gasValue = analogRead(gasPin);

    Serial.println("--------");
    vTaskDelay(500 / portTICK_PERIOD_MS); // Đợi 500ms trước khi đọc lại
  }
}

// Tác vụ gửi dữ liệu
void HandleFireData(void *pvParameters) {
  (void) pvParameters;
  while(true) {
    // show fire value
    Serial.print("Fire Sensor Value: ");
    Serial.println(fireValue); // Hiển thị giá trị đọc được từ cảm biến trên Serial Monitor
    
    Serial.print("Gas Sensor Value: ");
    Serial.println(gasValue);

    if (fireValue < fireThreshold) { // Kiểm tra xem giá trị có vượt ngưỡng hay không
      Serial.println(fireWarning); // Gửi tín hiệu cảnh báo lửa đến ESP32
      digitalWrite(alertPin, HIGH); // Bật Buzzer hoặc LED cảnh báo
      vTaskDelay(intervalWarning / portTICK_PERIOD_MS);
    }
    /*else if(gasValue > gasThreshold){
      Serial.println(gasWarning); // Gửi tín hiệu cảnh báo lửa đến ESP32
      digitalWrite(alertPin, HIGH); // Bật Buzzer hoặc LED cảnh báo
      vTaskDelay(intervalWarning / portTICK_PERIOD_MS);
    }*/
    else {
      digitalWrite(alertPin, LOW); // Tắt Buzzer hoặc LED cảnh báo
      vTaskDelay(1500 / portTICK_PERIOD_MS);
    }
  }
}

void HandleDisplayData(void *pvParameters) {
  (void) pvParameters;

  while (true) {
    xSemaphoreTake(xSemaphore, portMAX_DELAY);

    lcd.setCursor(6, 0);
    lcd.print(fireValue);
    lcd.print("  "); // Xóa các ký tự thừa nếu có

    lcd.setCursor(5, 1);
    lcd.print(gasValue);
    lcd.print("  "); // Xóa các ký tự thừa nếu có

    xSemaphoreGive(xSemaphore);

    vTaskDelay(1000 / portTICK_PERIOD_MS); // Đợi 1000ms trước khi cập nhật lại
  }
}


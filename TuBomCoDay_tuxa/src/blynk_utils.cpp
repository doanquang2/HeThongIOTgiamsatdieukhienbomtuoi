#include "blynk_utils.h"
#include "cau_hinh.h"
#include "chuc_nang.h" // Để dùng sendRS485Raw, waitForAck
#include <BlynkSimpleEsp32.h>

// --- BIẾN NỘI BỘ ---
static char auth[] = BLYNK_AUTH_TOKEN;
static int lastButtonState = HIGH;
static unsigned long lastDebounceTime = 0;
static const unsigned long debounceDelay = 50;
static int lastMode = -1; 
static bool lastMasterPumpStatus = false;
static bool lastPumpStatus[2][4] = {{0}};
static int manualRunTime = 0; // Thời gian chạy thủ công (giây), 0 = chạy mãi mãi

// --- XỬ LÝ TÍN HIỆU TỪ BLYNK APP ---
BLYNK_WRITE(VPIN_AUTO) {
  int val = param.asInt();
  if (val != mode) {
    mode = val;
    
    // Lưu cấu hình
    preferences.begin("config", false);
    preferences.putInt("mode", mode);
    preferences.end();
    
    // Xử lý logic khi tắt Auto
    if (mode == 0) {
      digitalWrite(MASTER_PUMP_PIN, LOW);
      masterPumpStatus = false;
      autoState = AUTO_IDLE;
    }
    Serial.println(mode ? "Blynk: Auto ON" : "Blynk: Auto OFF");
  }
}

// --- ĐIỀU KHIỂN BƠM TỔNG ---
BLYNK_WRITE(VPIN_MASTER_PUMP) {
  int state = param.asInt();
  if (mode == 1) { // Nếu đang Auto thì không cho điều khiển
    Blynk.virtualWrite(VPIN_MASTER_PUMP, !state); // Trả về trạng thái cũ trên App
    return;
  }
  
  digitalWrite(MASTER_PUMP_PIN, state ? HIGH : LOW);
  masterPumpStatus = (state == 1);
  
  preferences.begin("config", false);
  preferences.putBool("mps", masterPumpStatus);
  preferences.end();
}

// --- CÀI ĐẶT THỜI GIAN (AUTO & MANUAL) ---
BLYNK_WRITE(VPIN_MANUAL_TIME) {
  // Dùng Text Input: Nhập số giây (ví dụ: "60" hoặc "120")
  manualRunTime = param.asInt(); 
  Serial.printf("Blynk: Set Manual Time = %d s\n", manualRunTime);
}

BLYNK_WRITE(VPIN_AUTO_TIME_M) { // Text Input cho buổi sáng: "06:00"
  String timeStr = param.asStr();
  int sep = timeStr.indexOf(':');
  if (sep > 0) {
    startHourMorning = timeStr.substring(0, sep).toInt();
    startMinMorning = timeStr.substring(sep + 1).toInt();
    
    preferences.begin("config", false);
    preferences.putInt("mh", startHourMorning);
    preferences.putInt("mm", startMinMorning);
    preferences.end();
    
    Serial.printf("Blynk: Set Auto Morning %02d:%02d\n", startHourMorning, startMinMorning);
  }
}

BLYNK_WRITE(VPIN_AUTO_TIME_E) { // Text Input cho buổi chiều: "17:30"
  String timeStr = param.asStr();
  int sep = timeStr.indexOf(':');
  if (sep > 0) {
    startHourEvening = timeStr.substring(0, sep).toInt();
    startMinEvening = timeStr.substring(sep + 1).toInt();
    
    preferences.begin("config", false);
    preferences.putInt("eh", startHourEvening);
    preferences.putInt("em", startMinEvening);
    preferences.end();
    
    Serial.printf("Blynk: Set Auto Evening %02d:%02d\n", startHourEvening, startMinEvening);
  }
}

// --- ĐỒNG BỘ THỜI GIAN TỪ BLYNK ---
BLYNK_WRITE(VPIN_SYNC_TIME) {
  if (param.asInt() == 1) {
    Serial.println("Blynk: Dang yeu cau dong bo thoi gian...");
    Blynk.sendInternal("rtc", "sync"); // Gửi yêu cầu lấy thời gian từ Server
  }
}

// Hàm callback hệ thống: Được gọi khi Blynk Server trả về thời gian
BLYNK_WRITE(InternalPinRTC) {
  long t = param.asLong(); // Thời gian Unix Timestamp
  if (t > 0) {
    Serial.printf("Blynk: Nhan duoc thoi gian: %ld\n", t);
    rtc.adjust(DateTime(t)); // Cập nhật cho DS3231
    rtcFound = true;
    
    // Cập nhật giờ hệ thống ESP32 (để hàm getLocalTime hoạt động đúng)
    struct timeval tv = { .tv_sec = (time_t)t, .tv_usec = 0 };
    settimeofday(&tv, NULL);
    
    Serial.println("-> Da cap nhat RTC DS3231 & System Time thanh cong!");
  }
}

// Khi kết nối lại, đồng bộ trạng thái
BLYNK_CONNECTED() {
  Blynk.syncVirtual(VPIN_AUTO, VPIN_MASTER_PUMP, VPIN_MANUAL_TIME);
  Blynk.sendInternal("rtc", "sync"); // Tự động lấy giờ khi vừa kết nối
}

// --- HÀM HỖ TRỢ ĐIỀU KHIỂN VAN ---
void controlValveBlynk(int slave, int pump, int state, int vPin) {
  if (mode == 1) {
    Blynk.virtualWrite(vPin, !state); // Revert UI
    return;
  }

  uint8_t cmd = state ? CMD_ON : CMD_OFF;
  // Nếu bật (state=1), gửi kèm thời gian chạy (manualRunTime). Nếu tắt, gửi 0.
  uint16_t timeVal = (state == 1) ? manualRunTime : 0;
  sendRS485Raw(slave, cmd, pump, timeVal);

  // Chờ phản hồi (Blocking nhẹ, chấp nhận được khi nhấn nút)
  if (waitForAck(slave, cmd)) {
    pumpStatus[slave-1][pump-1] = (state == 1);
    
    preferences.begin("config", false);
    char key[10]; sprintf(key, "ps%d%d", slave-1, pump-1);
    preferences.putBool(key, pumpStatus[slave-1][pump-1]);
    preferences.end();
  } else {
    // Nếu lỗi, trả về trạng thái cũ trên App
    Blynk.virtualWrite(vPin, !state);
  }
}

// --- CÁC NÚT NHẤN VAN (SLAVE 1 & 2) ---
// Slave 1
BLYNK_WRITE(VPIN_S1_P1) { controlValveBlynk(1, 1, param.asInt(), VPIN_S1_P1); }
BLYNK_WRITE(VPIN_S1_P2) { controlValveBlynk(1, 2, param.asInt(), VPIN_S1_P2); }
BLYNK_WRITE(VPIN_S1_P3) { controlValveBlynk(1, 3, param.asInt(), VPIN_S1_P3); }
BLYNK_WRITE(VPIN_S1_P4) { controlValveBlynk(1, 4, param.asInt(), VPIN_S1_P4); }
// Slave 2
BLYNK_WRITE(VPIN_S2_P1) { controlValveBlynk(2, 1, param.asInt(), VPIN_S2_P1); }
BLYNK_WRITE(VPIN_S2_P2) { controlValveBlynk(2, 2, param.asInt(), VPIN_S2_P2); }
BLYNK_WRITE(VPIN_S2_P3) { controlValveBlynk(2, 3, param.asInt(), VPIN_S2_P3); }
BLYNK_WRITE(VPIN_S2_P4) { controlValveBlynk(2, 4, param.asInt(), VPIN_S2_P4); }

// --- XỬ LÝ NÚT NHẤN VẬT LÝ ---
void handlePhysicalButton() {
  int reading = digitalRead(BUTTON_AUTO_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    static int stableState = HIGH;
    if (reading != stableState) {
      stableState = reading;
      
      // Logic cho nút nhấn giữ (Latching Switch): LOW=ON, HIGH=OFF
      int newMode = (stableState == LOW) ? 1 : 0;
      if (mode != newMode) {
        mode = newMode;
        
        preferences.begin("config", false);
        preferences.putInt("mode", mode);
        preferences.end();
        
        if (mode == 0) {
          digitalWrite(MASTER_PUMP_PIN, LOW);
          masterPumpStatus = false;
          autoState = AUTO_IDLE;
        }
        Serial.println(mode ? "Button: Auto ON" : "Button: Auto OFF");
      }
    }
  }
  lastButtonState = reading;
}

// --- CÁC HÀM PUBLIC ---
void setupBlynk() {
  pinMode(BUTTON_AUTO_PIN, INPUT_PULLUP);
  Blynk.config(auth);
}

void syncTimeToBlynk() {
  // Format string HH:MM cho Text Input
  char timeStrM[6];
  sprintf(timeStrM, "%02d:%02d", startHourMorning, startMinMorning);
  Blynk.virtualWrite(VPIN_AUTO_TIME_M, timeStrM);

  char timeStrE[6];
  sprintf(timeStrE, "%02d:%02d", startHourEvening, startMinEvening);
  Blynk.virtualWrite(VPIN_AUTO_TIME_E, timeStrE);
  
  Serial.println("Da dong bo thoi gian tu Web len Blynk (Text Input)");
}

void loopBlynk() {
  Blynk.run();
  handlePhysicalButton();

  // Đồng bộ trạng thái Mode lên Blynk nếu có thay đổi (từ Web hoặc Button)
  if (mode != lastMode) {
    Blynk.virtualWrite(VPIN_AUTO, mode);
    Blynk.virtualWrite(VPIN_AUTO_LED, mode ? 255 : 0); // LED sáng (255) hoặc tắt (0)
    lastMode = mode;
  }

  // Đồng bộ trạng thái Bơm tổng
  if (masterPumpStatus != lastMasterPumpStatus) {
    Blynk.virtualWrite(VPIN_MASTER_PUMP, masterPumpStatus ? 1 : 0);
    lastMasterPumpStatus = masterPumpStatus;
  }

  // Đồng bộ trạng thái các Van (8 van)
  for(int s=0; s<2; s++) {
    for(int p=0; p<4; p++) {
      if (pumpStatus[s][p] != lastPumpStatus[s][p]) {
        // Tính toán VPIN dựa trên slave và pump
        int vPin = (s == 0) ? (VPIN_S1_P1 + p) : (VPIN_S2_P1 + p);
        Blynk.virtualWrite(vPin, pumpStatus[s][p] ? 1 : 0);
        lastPumpStatus[s][p] = pumpStatus[s][p];
      }
    }
  }
}
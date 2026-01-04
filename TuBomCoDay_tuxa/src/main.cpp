#include "cau_hinh.h"
#include "chuc_nang.h"
#include "blynk_utils.h"
#include <WebServer.h>
#include <ArduinoJson.h>
#include "giaodien.h"

// --- ĐỊNH NGHĨA BIẾN TOÀN CỤC ---
HardwareSerial RS485Serial(2);
WebServer server(80);
RTC_DS3231 rtc;
Preferences preferences;
WiFiClientSecure secured_client;
UniversalTelegramBot bot(TELEGRAM_BOT_TOKEN, secured_client);

bool rtcFound = false;
bool ntpSynced = false;
int mode = 0;
int startHourMorning = 7;
int startMinMorning = 0;
int startHourEvening = 17;
int startMinEvening = 0;

AutoState autoState = AUTO_IDLE;
int currentSlave = 1;
int currentPump = 1;
unsigned long autoTimer = 0;
int masterPumpMinutes = 30; // Mặc định 30 phút
bool pumpStatus[2][4] = {{0}}; // Mặc định tắt hết
bool masterPumpStatus = false;
int retryCount = 0;

// --- API HANDLERS CHO REACT FRONTEND ---
void enableCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS, PUT, DELETE");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleOptions() {
  enableCORS();
  server.send(200);
}

void handleAPIStatus() {
  enableCORS();
  JsonDocument doc;
  
  doc["rtcFound"] = rtcFound;
  doc["ntpSynced"] = ntpSynced;
  doc["mode"] = mode;
  doc["masterPumpStatus"] = masterPumpStatus;
  doc["masterPumpMinutes"] = masterPumpMinutes;
  
  // Trạng thái các bơm Slave
  JsonArray pumps = doc["pumps"].to<JsonArray>();
  for(int s=0; s<2; s++) {
    JsonArray slave = pumps.add<JsonArray>();
    for(int p=0; p<4; p++) {
      slave.add(pumpStatus[s][p]);
    }
  }
  
  // Thời gian cài đặt
  doc["mh"] = startHourMorning;
  doc["mm"] = startMinMorning;
  doc["eh"] = startHourEvening;
  doc["em"] = startMinEvening;
  
  // Gửi thời gian thực của hệ thống (để hiển thị giây trên Web)
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    doc["ch"] = timeinfo.tm_hour;
    doc["cm"] = timeinfo.tm_min;
    doc["cs"] = timeinfo.tm_sec;
  }

  doc["rssi"] = WiFi.RSSI();

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleAPIControl() {
  enableCORS();
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"Body missing\"}");
    return;
  }
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  // Cập nhật Mode
  if (doc.containsKey("mode")) mode = doc["mode"];
  
  // Cập nhật Bơm Tổng
  if (doc.containsKey("mps")) {
      masterPumpStatus = doc["mps"];
      digitalWrite(MASTER_PUMP_PIN, masterPumpStatus ? HIGH : LOW);
  }

  // Cập nhật Bơm Slave (Ví dụ JSON: {"s":0, "p":1, "state":true})
  if (doc.containsKey("s") && doc.containsKey("p") && doc.containsKey("state")) {
      int s = doc["s"];
      int p = doc["p"];
      bool state = doc["state"];
      if (s >= 0 && s < 2 && p >= 0 && p < 4) {
          pumpStatus[s][p] = state;
          // Gửi lệnh RS485 ngay nếu đang ở chế độ Manual
          if (mode == 0) {
             sendRS485Raw(s + 1, state ? CMD_ON : CMD_OFF, p + 1, 0);
          }
      }
  }

  // Cập nhật Cấu hình Thời gian (từ giao diện mới)
  bool configChanged = false;
  if (doc.containsKey("mh")) { startHourMorning = doc["mh"]; configChanged = true; }
  if (doc.containsKey("mm")) { startMinMorning = doc["mm"]; configChanged = true; }
  if (doc.containsKey("eh")) { startHourEvening = doc["eh"]; configChanged = true; }
  if (doc.containsKey("em")) { startMinEvening = doc["em"]; configChanged = true; }
  if (doc.containsKey("mt")) { masterPumpMinutes = doc["mt"]; configChanged = true; }

  if (configChanged) {
      preferences.begin("config", false);
      if (doc.containsKey("mh")) preferences.putInt("mh", startHourMorning);
      if (doc.containsKey("mm")) preferences.putInt("mm", startMinMorning);
      if (doc.containsKey("eh")) preferences.putInt("eh", startHourEvening);
      if (doc.containsKey("em")) preferences.putInt("em", startMinEvening);
      if (doc.containsKey("mt")) preferences.putInt("mt", masterPumpMinutes);
      preferences.end();
  }

  server.send(200, "application/json", "{\"success\":true}");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Doan Quang IOT Master Starting...");

  // Init Pins
  pinMode(MASTER_PUMP_PIN, OUTPUT);
  digitalWrite(MASTER_PUMP_PIN, LOW);

  // Init RS485
  RS485Serial.begin(RS485_BAUD, SERIAL_8N1, RS485_RX, RS485_TX);

  // Init RTC
  Wire.begin();
  if (!rtc.begin()) {
    Serial.println("Loi: Khong tim thay RTC! Kiem tra day SDA(21) va SCL(22)");
    rtcFound = false;
  } else {
    rtcFound = true;
    if (rtc.lostPower()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  }

  // Load Config
  preferences.begin("config", true);
  mode = preferences.getInt("mode", 0);
  startHourMorning = preferences.getInt("mh", 7);
  startMinMorning = preferences.getInt("mm", 0);
  startHourEvening = preferences.getInt("eh", 17);
  startMinEvening = preferences.getInt("em", 0);
  masterPumpMinutes = preferences.getInt("mt", 30);
  
  // Khôi phục trạng thái bơm nếu đang ở chế độ Manual (mode 0)
  if (mode == 0) {
    masterPumpStatus = preferences.getBool("mps", false);
    digitalWrite(MASTER_PUMP_PIN, masterPumpStatus ? HIGH : LOW);

    for(int s=0; s<2; s++) {
      for(int p=0; p<4; p++) {
        char key[10]; sprintf(key, "ps%d%d", s, p);
        pumpStatus[s][p] = preferences.getBool(key, false);
        
        // Nếu trạng thái là ON, gửi lệnh bật lại cho Slave
        if (pumpStatus[s][p]) {
          sendRS485Raw(s + 1, CMD_ON, p + 1, 0);
          delay(100); // Delay nhỏ để tránh nghẽn đường truyền
        }
      }
    }
  }
  preferences.end();

  // Init WiFi
  // Cấu hình IP tĩnh: 192.168.1.31
  // Lưu ý: Gateway (192.168.1.1) phải trùng với IP Router nhà bạn
  IPAddress local_IP(192, 168, 1, 31);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress primaryDNS(8, 8, 8, 8);   // Google DNS

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
    Serial.println("Loi cau hinh IP tinh!");
  }

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected: " + WiFi.localIP().toString());

  // Init Blynk & Button
  setupBlynk();

  // Init OTA
  setupOTA();

  // Init Telegram Security
  secured_client.setInsecure(); // Bỏ qua kiểm tra chứng chỉ SSL để chạy nhanh hơn

  // Init NTP (Internet Time)
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET, NTP_SERVER);

  // Nếu không có RTC, cố gắng chờ đồng bộ NTP ngay lập tức (trong 10s)
  if (!rtcFound) {
    Serial.print("RTC Error! Dang cho dong bo NTP...");
    int retry = 0;
    while (retry < 20) { // Thử 20 lần x 500ms = 10 giây
      struct tm timeinfo;
      if (getLocalTime(&timeinfo, 0)) {
        if (timeinfo.tm_year + 1900 >= 2024) {
          ntpSynced = true;
          Serial.println(" OK! Da co gio Internet.");
          break;
        }
      }
      delay(500);
      Serial.print(".");
      retry++;
    }
    if (!ntpSynced) Serial.println(" That bai (se thu lai ngam).");
  }

  // Init WebServer API
  server.on("/", HTTP_GET, []() { server.send(200, "text/html", INDEX_HTML); });
  server.on("/api/status", HTTP_GET, handleAPIStatus);
  server.on("/api/control", HTTP_POST, handleAPIControl);
  server.on("/api/control", HTTP_OPTIONS, handleOptions); // Hỗ trợ CORS preflight
  server.on("/api/sync_time", handleSyncTime); // Đăng ký API đồng bộ giờ
  server.begin();
  Serial.println("HTTP Server started");
}

void loop() {
  loopBlynk();
  server.handleClient();

  ArduinoOTA.handle(); // Lắng nghe yêu cầu nạp code
  
  // --- KIỂM TRA KẾT NỐI RTC ĐỊNH KỲ (Mỗi 5 giây) ---
  // Giúp tránh lỗi "Wire.cpp" spam liên tục khi lỏng dây
  static unsigned long lastRTCCheck = 0;
  if (millis() - lastRTCCheck > 5000) {
    lastRTCCheck = millis();
    Wire.beginTransmission(0x68); // Địa chỉ I2C của DS3231
    if (Wire.endTransmission() == 0) {
      rtcFound = true;
    } else {
      rtcFound = false;
    }
  }

  // Chỉ chạy logic tự động nếu có RTC hoặc đã có giờ NTP
  // Lưu ý: Hàm runAutoLogic() cần được sửa để dùng getLocalTime() khi !rtcFound
  if (rtcFound || ntpSynced) {
    runAutoLogic();
  }

  // --- TỰ ĐỘNG LƯU GIỜ TỪ INTERNET VÀO DS3231 (Mỗi 60 phút) ---
  static unsigned long lastNTPSync = 0;
  if (WiFi.status() == WL_CONNECTED && (millis() - lastNTPSync > 3600000UL || lastNTPSync == 0)) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10)) { // Timeout 10ms
      if ((timeinfo.tm_year + 1900) >= 2024) {
        ntpSynced = true; // Đánh dấu đã có giờ chuẩn
        if (rtcFound) {
           rtc.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
           Serial.println("-> Da dong bo thoi gian tu NTP vao RTC DS3231");
           lastNTPSync = millis();
        }
      }
    }
  }

  delay(10); // Nhường CPU nhẹ
}
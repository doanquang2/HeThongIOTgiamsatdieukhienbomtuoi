#include "chuc_nang.h"
#include "giao_dien.h"
#include "blynk_utils.h"

// --- HÀM RS485 ---
void sendRS485Raw(uint8_t slaveId, uint8_t cmd, uint8_t pump, uint16_t timeVal) {
  uint8_t frame[8];
  frame[0] = STX;
  frame[1] = slaveId;
  frame[2] = cmd;
  frame[3] = pump;
  frame[4] = (timeVal >> 8) & 0xFF;
  frame[5] = timeVal & 0xFF;
  
  uint8_t sum = 0;
  for(int i=1; i<=5; i++) sum += frame[i];
  frame[6] = sum;
  frame[7] = ETX;

  RS485Serial.write(frame, 8);
  RS485Serial.flush();
  Serial.printf("Da gui lenh: ID=%d, CMD=%d, PUMP=%d, Time=%d\n", slaveId, cmd, pump, timeVal);
}

bool waitForAck(uint8_t slaveId, uint8_t originalCmd) {
  unsigned long start = millis();
  uint8_t rxBuf[8];
  int rxIdx = 0;

  while (millis() - start < 500) { // Chờ 500ms
    if (RS485Serial.available()) {
      uint8_t b = RS485Serial.read();
      if (rxIdx == 0 && b != STX) continue;
      rxBuf[rxIdx++] = b;

      if (rxIdx == 8) {
        if (rxBuf[7] == ETX) {
          uint8_t sum = 0;
          for(int i=1; i<=5; i++) sum += rxBuf[i];
          if (sum == rxBuf[6] && rxBuf[1] == slaveId && rxBuf[2] == CMD_ACK && rxBuf[3] == originalCmd) {
            return true;
          }
        }
        rxIdx = 0;
      }
    }
  }
  return false;
}

// --- HÀM LẤY THỜI GIAN (RTC hoặc NTP) ---
DateTime getCurrentTime() {
  if (rtcFound) {
    return rtc.now();
  } else {
    struct tm timeinfo;
    // Thử lấy giờ hệ thống (đã sync NTP), timeout 10ms để không chặn loop
    if (getLocalTime(&timeinfo, 10)) { 
      return DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }
  }
  return DateTime(2000, 1, 1, 0, 0, 0); // Trả về năm 2000 nếu lỗi/chưa sync
}

// --- HÀM GỬI TELEGRAM ---
void sendTelegramMessage(String msg) {
  bot.sendMessage(TELEGRAM_CHAT_ID, msg, "");
}

// --- LOGIC TỰ ĐỘNG ---
void runAutoLogic() {
  if (mode != 1) { // Nếu không phải AUTO
    autoState = AUTO_IDLE;
    return;
  }

  DateTime now = getCurrentTime();
  if (now.year() < 2024) return; // Chưa có thời gian hợp lệ (RTC lỗi hoặc chưa sync NTP)

  switch (autoState) {
    case AUTO_IDLE:
      if ((now.hour() == startHourMorning && now.minute() == startMinMorning && now.second() == 0) ||
          (now.hour() == startHourEvening && now.minute() == startMinEvening && now.second() == 0)) {
        Serial.println(">>> BAT DAU CHU TRINH AUTO");
        sendTelegramMessage("Bắt đầu chu trình tưới tự động!");
        currentSlave = 1;
        currentPump = 1;
        autoState = AUTO_SEND_SLAVE;
        retryCount = 0;
      }
      break;

    case AUTO_SEND_SLAVE:
      Serial.printf(">>> Auto: Kich hoat Slave %d - Pump %d (20s)\n", currentSlave, currentPump);
      sendRS485Raw(currentSlave, CMD_ON, currentPump, 20);
      
      if (waitForAck(currentSlave, CMD_ON)) {
        Serial.println("-> ACK OK. Cho Slave chay 20s...");
        retryCount = 0; // Reset biến đếm
        pumpStatus[currentSlave-1][currentPump-1] = true; // Cập nhật trạng thái ON
        autoTimer = millis();
        autoState = AUTO_WAIT_SLAVE_RUN;
      } else {
        retryCount++;
        Serial.printf("-> ACK Fail. Thu lai lan %d...\n", retryCount);
        if (retryCount >= 3) {
          String msg = "CẢNH BÁO: Slave " + String(currentSlave) + " Van " + String(currentPump) + " không phản hồi! Đã bỏ qua.";
          sendTelegramMessage(msg);
          retryCount = 0;
          autoState = AUTO_NEXT_STEP; // Bỏ qua bước chạy bơm Master, chuyển sang van kế tiếp
        } else {
          delay(1000);
        }
      }
      break;

    case AUTO_WAIT_SLAVE_RUN:
      if (millis() - autoTimer >= TIME_SLAVE_RUN_MS + 2000) {
        Serial.println("-> Slave da chay xong. Bat Master Pump (30 phut)...");
        pumpStatus[currentSlave-1][currentPump-1] = false; // Slave tự tắt sau 20s -> Cập nhật OFF
        digitalWrite(MASTER_PUMP_PIN, HIGH);
        masterPumpStatus = true; // Master ON
        autoTimer = millis();
        autoState = AUTO_RUN_MASTER_PUMP;
      }
      break;

    case AUTO_RUN_MASTER_PUMP:
      if (millis() - autoTimer >= (unsigned long)masterPumpMinutes * 60000UL) {
        Serial.println("-> Master Pump xong. Tat Master Pump.");
        digitalWrite(MASTER_PUMP_PIN, LOW);
        masterPumpStatus = false; // Master OFF
        autoState = AUTO_NEXT_STEP;
      }
      break;

    case AUTO_NEXT_STEP:
      currentPump++;
      if (currentPump > 4) {
        currentPump = 1;
        currentSlave++;
      }
      if (currentSlave > 2) {
        Serial.println(">>> KET THUC CHU TRINH AUTO");
        sendTelegramMessage("Đã hoàn thành chu trình tưới tự động!");
        autoState = AUTO_FINISH;
      } else {
        autoState = AUTO_SEND_SLAVE;
        retryCount = 0;
      }
      break;

    case AUTO_FINISH:
      if (now.second() > 5) { 
        autoState = AUTO_IDLE;
      }
      break;
  }
}

// --- WEB HANDLERS ---
void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleStatus() {
  JsonDocument doc;
  char timeBuf[32];
  char dateBuf[16];
  DateTime now = getCurrentTime();
  
  if (now.year() >= 2024) {
    sprintf(timeBuf, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    sprintf(dateBuf, "%02d/%02d/%04d", now.day(), now.month(), now.year());
  } else {
    if (!rtcFound) {
      sprintf(timeBuf, "Lỗi RTC! Đang sync...");
    } else {
      sprintf(timeBuf, "Đang đồng bộ...");
    }
    sprintf(dateBuf, "--/--/----");
  }
  
  doc["time"] = timeBuf;
  doc["date"] = dateBuf;
  doc["mode"] = mode;
  doc["mh"] = startHourMorning;
  doc["mm"] = startMinMorning;
  doc["eh"] = startHourEvening;
  doc["em"] = startMinEvening;
  doc["mt"] = masterPumpMinutes;
  if (rtcFound) {
    doc["temp"] = rtc.getTemperature();
  }
  
  // Gửi trạng thái các bơm
  JsonArray slaves = doc["slaves"].to<JsonArray>();
  for(int s=0; s<2; s++) {
    JsonArray p = slaves.add<JsonArray>();
    for(int i=0; i<4; i++) p.add(pumpStatus[s][i] ? 1 : 0);
  }
  doc["master"] = masterPumpStatus ? 1 : 0;

  long remainSec = 0;
  if (mode == 1) {
    if (autoState == AUTO_IDLE) doc["status"] = "Auto: Đang chờ đến giờ...";
    else if (autoState == AUTO_SEND_SLAVE) doc["status"] = "Auto: Đang gửi lệnh Slave...";
    else if (autoState == AUTO_WAIT_SLAVE_RUN) {
      doc["status"] = "Auto: Slave đang chạy...";
      long elapsed = millis() - autoTimer;
      if (elapsed < TIME_SLAVE_RUN_MS) remainSec = (TIME_SLAVE_RUN_MS - elapsed) / 1000;
    }
    else if (autoState == AUTO_RUN_MASTER_PUMP) {
      doc["status"] = "Auto: Master Pump đang chạy...";
      unsigned long totalMs = (unsigned long)masterPumpMinutes * 60000UL;
      long elapsed = millis() - autoTimer;
      if (elapsed < totalMs) remainSec = (totalMs - elapsed) / 1000;
    }
    else doc["status"] = "Auto: Đang xử lý...";
  } else {
    doc["status"] = "Chế độ: Thủ công";
  }
  doc["remain"] = remainSec;

  if (!rtcFound) {
    String s = doc["status"];
    doc["status"] = s + " [Lỗi RTC]";
  }

  String res;
  serializeJson(doc, res);
  server.send(200, "application/json", res);
}

void handleSetMode() {
  if (server.hasArg("val")) {
    mode = server.arg("val").toInt();
    preferences.begin("config", false);
    preferences.putInt("mode", mode);
    preferences.end();
    
    if (mode == 0) {
      digitalWrite(MASTER_PUMP_PIN, LOW);
      autoState = AUTO_IDLE;
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleControl() {
  if (mode == 1) return server.send(400, "text/plain", "Dang o che do Auto");
  int s = server.arg("slave").toInt();
  int p = server.arg("pump").toInt();
  int st = server.arg("state").toInt();

  uint8_t cmd = st ? CMD_ON : CMD_OFF;
  sendRS485Raw(s, cmd, p, 0);

  // Kiểm tra phản hồi từ Slave (chờ tối đa 500ms)
  if (waitForAck(s, cmd)) {
    if (s >= 1 && s <= 2 && p >= 1 && p <= 4) {
      pumpStatus[s-1][p-1] = (st == 1);
      
      // Lưu trạng thái vào Flash
      preferences.begin("config", false);
      char key[10]; sprintf(key, "ps%d%d", s-1, p-1);
      preferences.putBool(key, pumpStatus[s-1][p-1]);
      preferences.end();
    }
    server.send(200, "text/plain", "OK");
  } else {
    // Gửi cảnh báo Telegram nếu không nhận được phản hồi
    String msg = "CẢNH BÁO: Mất kết nối với Slave " + String(s) + " khi điều khiển thủ công!";
    sendTelegramMessage(msg);
    server.send(504, "text/plain", "Slave Timeout");
  }
}

void handleControlMaster() {
  if (mode == 1) return server.send(400, "text/plain", "Dang o che do Auto");
  int st = server.arg("state").toInt();
  digitalWrite(MASTER_PUMP_PIN, st ? HIGH : LOW);
  masterPumpStatus = (st == 1);
  
  // Lưu trạng thái vào Flash
  preferences.begin("config", false);
  preferences.putBool("mps", masterPumpStatus);
  preferences.end();
  
  server.send(200, "text/plain", "OK");
}

void handleConfig() {
  startHourMorning = server.arg("mh").toInt();
  startMinMorning = server.arg("mm").toInt();
  startHourEvening = server.arg("eh").toInt();
  startMinEvening = server.arg("em").toInt();

  preferences.begin("config", false);
  preferences.putInt("mh", startHourMorning);
  preferences.putInt("mm", startMinMorning);
  preferences.putInt("eh", startHourEvening);
  preferences.putInt("em", startMinEvening);
  if (server.hasArg("mt")) {
    masterPumpMinutes = server.arg("mt").toInt();
    preferences.putInt("mt", masterPumpMinutes);
  }
  preferences.end();

  // Đồng bộ thời gian mới cài đặt lên Blynk
  syncTimeToBlynk();

  server.send(200, "text/plain", "OK");
}

void handleSyncTime() {
  if (server.hasArg("y") && server.hasArg("m") && server.hasArg("d") && 
      server.hasArg("h") && server.hasArg("mi") && server.hasArg("s")) {
    
    int y = server.arg("y").toInt();
    int m = server.arg("m").toInt();
    int d = server.arg("d").toInt();
    int h = server.arg("h").toInt();
    int mi = server.arg("mi").toInt();
    int s = server.arg("s").toInt();

    if (rtcFound) {
      rtc.adjust(DateTime(y, m, d, h, mi, s));
      Serial.println("Da dong bo thoi gian tu Web");
    }
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing args");
  }
}

// --- OTA SETUP ---
void setupOTA() {
  ArduinoOTA.setHostname("DoanQuangIOT-Master"); // Tên thiết bị hiển thị trên cổng nạp
  // ArduinoOTA.setPassword("admin"); // Bỏ comment dòng này nếu muốn đặt mật khẩu nạp

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}
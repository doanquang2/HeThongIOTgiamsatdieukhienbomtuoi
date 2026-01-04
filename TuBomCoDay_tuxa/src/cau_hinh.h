#ifndef CAU_HINH_H
#define CAU_HINH_H

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <RTClib.h>
#include <Preferences.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>

// --- CẤU HÌNH PHẦN CỨNG ---
#define RS485_TX      17
#define RS485_RX      16
#define RS485_BAUD    9600
#define MASTER_PUMP_PIN 13
#define BUTTON_AUTO_PIN 15

// --- WIFI CONFIG ---
#define WIFI_SSID     "THU UYEN"
#define WIFI_PASS     "03032002"

// --- TELEGRAM CONFIG ---
#define TELEGRAM_BOT_TOKEN "8550336380:AAGX_s3503irBRprb4-8Kd8h0wnm-RcHAfg"  // Thay Token của bạn vào đây
#define TELEGRAM_CHAT_ID   "-5214458184"    // Thay Chat ID của bạn vào đây

// --- BLYNK CONFIG ---
#define BLYNK_TEMPLATE_ID "TMPL6HkRI5krl"
#define BLYNK_TEMPLATE_NAME "TramTuoiCayTuXaCoDay"
#define BLYNK_AUTH_TOKEN "1Glm_UcFXTCd1KM_O4WsD-assB3U3dMk"
#define VPIN_AUTO        V1
#define VPIN_AUTO_LED    V2   // Đèn LED trạng thái Auto
#define VPIN_MASTER_PUMP V3   // Nút điều khiển Bơm tổng
#define VPIN_AUTO_TIME_M V5   // Time Input: Giờ Auto Sáng
#define VPIN_AUTO_TIME_E V6   // Time Input: Giờ Auto Chiều
#define VPIN_MANUAL_TIME V10  // Text Input: Thời gian chạy thủ công (giây)
#define VPIN_SYNC_TIME   V7   // Nút nhấn: Đồng bộ thời gian

// Slave 1 Valves
#define VPIN_S1_P1       V11
#define VPIN_S1_P2       V12
#define VPIN_S1_P3       V13
#define VPIN_S1_P4       V14
// Slave 2 Valves
#define VPIN_S2_P1       V21
#define VPIN_S2_P2       V22
#define VPIN_S2_P3       V23
#define VPIN_S2_P4       V24

// --- GIAO THỨC ---
#define STX           0xAA
#define ETX           0x55
#define CMD_ON        0x01
#define CMD_OFF       0x00
#define CMD_ACK       0xFF

// --- THỜI GIAN ---
#define TIME_SLAVE_RUN_MS  20000UL        // 20 giây
#define NTP_SERVER         "pool.ntp.org"
#define GMT_OFFSET_SEC     25200          // UTC+7 (7 * 3600)
#define DAYLIGHT_OFFSET    0

// --- ENUM TRẠNG THÁI AUTO ---
enum AutoState {
  AUTO_IDLE,
  AUTO_SEND_SLAVE,
  AUTO_WAIT_SLAVE_RUN,
  AUTO_RUN_MASTER_PUMP,
  AUTO_NEXT_STEP,
  AUTO_FINISH
};

// --- KHAI BÁO BIẾN TOÀN CỤC (EXTERN) ---
// (Các biến này được định nghĩa thực sự bên main.cpp)
extern HardwareSerial RS485Serial;
extern WebServer server;
extern RTC_DS3231 rtc;
extern Preferences preferences;
extern UniversalTelegramBot bot;

extern bool rtcFound;
extern int mode;
extern int startHourMorning;
extern int startMinMorning;
extern int startHourEvening;
extern int startMinEvening;

extern AutoState autoState;
extern int currentSlave;
extern int currentPump;
extern unsigned long autoTimer;
extern int masterPumpMinutes; // Thời gian chạy bơm Master (phút)
extern bool pumpStatus[2][4]; // Trạng thái 8 van (True=ON, False=OFF)
extern bool masterPumpStatus; // Trạng thái bơm Master
extern int retryCount;        // Biến đếm số lần thử lại khi mất kết nối

#endif
#ifndef CHUC_NANG_H
#define CHUC_NANG_H

#include "cau_hinh.h"

void sendRS485Raw(uint8_t slaveId, uint8_t cmd, uint8_t pump, uint16_t timeVal);
bool waitForAck(uint8_t slaveId, uint8_t originalCmd);
void runAutoLogic();
void handleRoot();
void handleStatus();
void handleSetMode();
void handleControl();
void handleControlMaster();
void handleConfig();
void handleSyncTime();
void sendTelegramMessage(String msg);
void setupOTA();

#endif
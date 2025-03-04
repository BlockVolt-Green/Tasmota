/*
  xdrv_129_rddl_network.ino - RDDL Network protocol integration

  Copyright (C) 2023  Jürgen Eckel

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#define USE_DRV_RDDL_NETWORK
#ifdef USE_DRV_RDDL_NETWORK
/*********************************************************************************************\
 * Settings to participate in the RDDL Network
 *
 * To test this file:
 * - ****************
 * - *********** in  platform_override.ini
\*********************************************************************************************/

#include "esp_random.h"

#define XDRV_129 129

#define DRV_DEMO_MAX_DRV_TEXT 16
#define RDDL_NETWORK_NOTARIZATION_TIMER_ONE_HOUR_IN_SECONDS (60 * 60)

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <algorithm>

#ifdef ESP32
#ifdef USE_SDCARD
#include <SD.h>
#endif // USE_SDCARD
#include "FFat.h"
#include "FS.h"
#endif // ESP32

#include "rddlSDKAPI.h"
#include "rddlSDKUtils.h"
#include "rddlSDKAbst.h"
#include "include/meter_modbus.h"
#include <ArduinoJson.h>

uint32_t counted_seconds = 0;

// Demo command line commands
const char kDrvDemoCommands[] PROGMEM = "Drv|" // Prefix
                                        "Text";

void (*const DrvDemoCommand[])(void) PROGMEM = {
    &CmndDrvText};

// Global structure containing driver saved variables
struct
{
  uint32_t crc32;   // To detect file changes
  uint32_t version; // To detect driver function changes
  char drv_text[DRV_DEMO_MAX_DRV_TEXT - 1][10];
} DrvDemoSettings;

// Global structure containing driver non-saved variables
struct
{
  uint32_t any_value;
} DrvDemoGlobal;

void CmndDrvText(void)
{
  if ((XdrvMailbox.index > 0) && (XdrvMailbox.index <= DRV_DEMO_MAX_DRV_TEXT))
  {
    if (!XdrvMailbox.usridx)
    {
      // Command DrvText
      for (uint32_t i = 0; i < DRV_DEMO_MAX_DRV_TEXT; i++)
      {
        AddLog(LOG_LEVEL_DEBUG, PSTR("DRV: DrvText%02d %s"), i, DrvDemoSettings.drv_text[i]);
      }
      ResponseCmndDone();
    }
    else
    {
      // Command DrvText<index> <text>
      uint32_t index = XdrvMailbox.index - 1;
      if (XdrvMailbox.data_len > 0)
      {
        snprintf_P(DrvDemoSettings.drv_text[index], sizeof(DrvDemoSettings.drv_text[index]), XdrvMailbox.data);
      }
      ResponseCmndIdxChar(DrvDemoSettings.drv_text[index]);
    }
  }
}

/*********************************************************************************************\
 * Driver Settings load and save
\*********************************************************************************************/

void RDDLNetworkSettingsLoad(bool erase)
{
  // Called from FUNC_PRE_INIT (erase = 0) once at restart
  // Called from FUNC_RESET_SETTINGS (erase = 1) after command reset 4, 5, or 6
  sdkSetSetting(SET_DEVICENAME, "RDDL-Tasmota");
}

void RDDLNetworkSettingsSave(void)
{
  // Called from FUNC_SAhttp://tasmota/VE_SETTINGS every SaveData second and at restart
}

void getNotarizationMessage()
{
  MqttShowSensor(false);
}

#define PUB_KEY_SIZE 33
#define ADDRESS_HASH_SIZE 20
#define ADDRESS_TAIL 20

bool g_mutex_running_notarization = false;

bool claimNotarizationMutex()
{
  if (!g_mutex_running_notarization)
  {
    g_mutex_running_notarization = true;
    return true;
  }
  else
    return false;
}
bool executePoP = false;
void SetExecutePoP()
{
  executePoP = true;
}

void releaseNotarizationMutex()
{
  g_mutex_running_notarization = false;
}

String getNotarizationMessagePhase3()
{

  init_meter_modbus();

  float readings[20] = {0.000};
  fetch_meter_readings(readings);

  JsonDocument doc;

  doc["Time"] = GetDateAndTime(DT_UTC);

  JsonObject voltage = doc["voltage"].to<JsonObject>();
  voltage["voltage_phase_1"] = readings[0];
  voltage["voltage_phase_2"] = readings[1];
  voltage["voltage_phase_3"] = readings[2];
  voltage["voltage_phase_avg"] = readings[3];

  JsonObject current = doc["current"].to<JsonObject>();
  current["current_phase_1"] = readings[4];
  current["current_phase_2"] = readings[5];
  current["current_phase_3"] = readings[6];
  current["current_phase_avg"] = readings[7];

  JsonObject power_factor = doc["power_factor"].to<JsonObject>();
  power_factor["power_factor_phase_1"] = readings[8];
  power_factor["power_factor_phase_2"] = readings[9];
  power_factor["power_factor_phase_3"] = readings[10];
  power_factor["power_factor_phase_avg"] = readings[11];

  JsonObject active_power = doc["active_power"].to<JsonObject>();
  active_power["active_power_phase_1"] = readings[12];
  active_power["active_power_phase_2"] = readings[13];
  active_power["active_power_phase_3"] = readings[14];
  active_power["active_power_phase_avg"] = readings[15];

  JsonObject apparent_power = doc["apparent_power"].to<JsonObject>();
  apparent_power["apparent_power_phase_1"] = readings[16];
  apparent_power["apparent_power_phase_2"] = readings[17];
  apparent_power["apparent_power_phase_3"] = readings[18];
  apparent_power["apparent_power_phase_avg"] = readings[19];

  String output;

  doc.shrinkToFit(); // optional

  serializeJson(doc, output);

  return output;
}

void RDDLNotarize()
{

  if (claimNotarizationMutex())
  {

    int not_perodicity = std::atoi(SettingsText(SET_NOTARIZTATION_PERIODICITY));

    // prevent notarization stalling
    if (not_perodicity < 60)
    {
      SettingsUpdateText(SET_NOTARIZTATION_PERIODICITY, "60");
    }

    char *meter = SettingsText(SET_METER);
    Serial.println(meter);
    if (strcmp(meter, "null") == 0 || strcmp(meter, "") == 0)
      return;
    Serial.println("Meter type passed");

    // create notarization message

    const char *data_str = nullptr;
    size_t data_length = 0;
    String output = "";

    if (strcmp(meter, "default") == 0)
    {
      int start_position = ResponseLength();
      getNotarizationMessage();
      int current_position = ResponseLength();
      data_length = (size_t)(current_position - start_position);
      data_str = TasmotaGlobal.mqtt_data.c_str() + start_position;
    }

    else if (strcmp(meter, "elite") == 0)
    {
      output = getNotarizationMessagePhase3();
      data_str = output.c_str();
      data_length = strlen(data_str);
    }

    else
      return;

    Serial.println("Data Str: ");
    Serial.println(data_str);
    Serial.println("Data Str Len: ");
    Serial.println(data_length);

    if (data_str == nullptr || data_length == 0)
      return;
    Serial.println("Data str null check pass");

    runRDDLSDKNotarizationWorkflow(data_str, data_length);

    HTTPClientLight http;
    http.begin(SettingsText(SET_NEXUS_API));
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", SettingsText(SET_NEXUS_AUTH_TOKEN));

    // String js = "{\"data\":\"" + String(data_str) + "\",\"address\":\"" + String(sdkGetRDDLAddress()) + "\"}";

    JsonDocument doc;
    doc["data"] = data_str;
    doc["address"] = sdkGetRDDLAddress();
    doc["meter"] = meter;

    String js;

    serializeJson(doc, js);

    Serial.println(js);
    AddLog(2, data_str);

    int httpResponseCode = http.POST(js);

    http.end();

    releaseNotarizationMutex();
  }
}

void RemoveFiles()
{
  deleteOldestFiles(500);
}

extern fs::FS *TfsGlobalFileSysHandle();
extern fs::FS *TfsFlashFileSysHandle();
extern fs::FS *TfsDownloadFileSysHandle();

#define MAX_FILES 10000

void deleteOldestFiles(int count)
{
  FS *filesystem = TfsGlobalFileSysHandle();
  if (filesystem)
    deleteOldestFilesFromFS(filesystem, count);

  filesystem = TfsFlashFileSysHandle();
  if (filesystem)
    deleteOldestFilesFromFS(filesystem, count);

  filesystem = TfsDownloadFileSysHandle();
  if (filesystem)
    deleteOldestFilesFromFS(filesystem, count);
}

void deleteOldestFilesFromFS(FS *filesystem, int count)
{
  if (!filesystem)
    Serial.println("Failed to mount file system");

  File dir = filesystem->open("/");
  std::vector<String> files;
  String nextFile = dir.getNextFileName();
  while (nextFile.length() > 0)
  {
    // unsigned long timeStamp = extractTimestamp(fileName);
    // files.push_back(std::make_pair(fileName, timeStamp));
    if (nextFile.length() > 20)
    {
      files.push_back(nextFile);
    }
    nextFile = dir.getNextFileName();
  }

  // // Sort files based on timestamp
  // std::sort(files.begin(), files.end(), [](const auto &a, const auto &b) {
  //     return a.second < b.second;
  // });

  // Delete the oldest 'count' files
  for (int i = 0; i < count && i < files.size(); ++i)
  {
    // AddLog(LOG_LEVEL_INFO, PSTR("Removing %s") ,files[i]);
#ifdef USE_UFILESYS
    if (TfsDeleteFile(files[i].c_str()))
    {
      // if (LittleFS.remove(files[i])) {
      Serial.println("Deleted file: " + files[i]);
    }
    else
    {

      Serial.println("Failed to delete file: " + files[i]);
    }
#else
    Serial.println("Failed to delete file: " + files[i]);
#endif
  }
}

void InitializePoPWorkflow()
{
  bool IamChallenger = amIChallenger();
  if (IamChallenger)
  {
    AddLog(2, "PoP: initialize PoP challenge");

    // select CID
    char *cid = getCIDofChallengee();
    AddLog(2, "PoP: CID to be challenged %s", cid);
    bool result = ChallengeChallengee(cid, NULL);
    if (result)
      AddLog(2, "{ \"PoP\": challenge broadcasted }");
    else
      AddLog(2, "{ \"PoP\": challenge initialization failed }");
  }
  else
    AddLog(2, "PoP: not the challenger");
}

void RDDLNetworkScheduler()
{
  ++counted_seconds;
  if (counted_seconds >= (uint32_t)atoi(sdkGetSetting(SET_NOTARIZTATION_PERIODICITY)))
  {
    counted_seconds = 0;
    RDDLNotarize();
  }
  else if (counted_seconds % 2 == 0 && executePoP)
  {
    executePoP = false;
    InitializePoPWorkflow();
  }
}

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

bool Xdrv129(uint32_t function)
{
  bool result = false;
  switch (function)
  {
  case FUNC_RESET_SETTINGS:
    RDDLNetworkSettingsLoad(1);
    break;
  case FUNC_SAVE_SETTINGS:
    RDDLNetworkSettingsSave();
    break;
  case FUNC_COMMAND:
    break;
  case FUNC_PRE_INIT:
    RDDLNetworkSettingsLoad(0);
    break;
  case FUNC_SAVE_BEFORE_RESTART:
    // !!! DO NOT USE AS IT'S FUNCTION IS BETTER HANDLED BY FUNC_SAVE_SETTINGS !!!
    break;
  }
  return result;
}

#endif // USE_DRV_FILE_DEMO

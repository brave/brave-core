/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <any>
#include <ctime>

#include "../include/json_bat_helper.h"

namespace settings {

const char enabled[] = "ads.enabled";
const char locale[] = "ads.locale";
const char perDay[] = "ads.amount.day";
const char perHour[] = "ads.amount.hour";
const char place[] = "ads.place";
const char operatingMode[] = "ads.operating-mode";

}  // namespace settings

namespace ads_bat_client {

struct USER_MODEL_STATE_ST {
  USER_MODEL_STATE_ST();
  USER_MODEL_STATE_ST(const USER_MODEL_STATE_ST& state);
  ~USER_MODEL_STATE_ST();

  bool LoadFromJson(const std::string& json);

  std::vector<std::time_t> adsShownHistory;
  std::string adUUID;
  std::map<std::string, bool> adsUUIDSeen;
  bool available;
  bool allowed;
  // std::map<std::string, std::any> catalog;
  bool configured;
  std::string currentSSID;
  bool expired;
  std::time_t finalContactTimestamp;
  std::time_t firstContactTimestamp;
  std::time_t lastSearchTime;
  std::time_t lastShopTime;
  std::time_t lastUserActivity;
  std::time_t lastUserIdleStopTime;
  std::string locale;
  std::vector<std::string> locales;
  std::vector<double> pageScoreHistory;
  std::map<std::string, std::string> places;
  double score;
  bool searchActivity;
  std::string searchUrl;
  bool shopActivity;
  std::string shopUrl;
  std::string status;
};

}  // namespace ads_bat_client

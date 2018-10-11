/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../include/state_settings.h"

namespace ads_bat_client {

SETTINGS_STATE_ST::SETTINGS_STATE_ST() :
    ads_enabled(false),
    ads_amount_day(""),
    ads_amount_hour("") {}

SETTINGS_STATE_ST::SETTINGS_STATE_ST(const SETTINGS_STATE_ST& state) {
  ads_enabled = state.ads_enabled;
  ads_amount_day = state.ads_amount_day;
  ads_amount_hour = state.ads_amount_hour;
}

SETTINGS_STATE_ST::~SETTINGS_STATE_ST() = default;

bool SETTINGS_STATE_ST::LoadFromJson(const std::string& json) {
  rapidjson::Document document;
  document.Parse(json.c_str());

  if (document.HasParseError()) {
    return false;
  }

  bool success =
    document.HasMember("ads.enabled") &&
      document["ads.enabled"].IsBool() &&
    document.HasMember("ads.amount.day") &&
      document["ads.amount.day"].IsString() &&
    document.HasMember("ads.amount.hour") &&
      document["ads.amount.hour"].IsString();

  if (success) {
    ads_enabled = document["ads.enabled"].GetBool();
    ads_amount_day = document["ads.amount.day"].GetString();
    ads_amount_hour = document["ads.amount.hour"].GetString();
  }

  return success;
}

}  // namespace ads_bat_client

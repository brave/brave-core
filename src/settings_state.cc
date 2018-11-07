/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <map>

#include "settings_state.h"
#include "json_helper.h"
#include "static_values.h"

namespace ads {

SETTINGS_STATE::SETTINGS_STATE() :
    ads_enabled(false),
    ads_locale("en"),
    ads_per_hour(""),
    ads_per_day("") {}

SETTINGS_STATE::SETTINGS_STATE(const SETTINGS_STATE& state) {
  ads_enabled = state.ads_enabled;
  ads_locale = state.ads_locale;
  ads_per_hour = state.ads_per_hour;
  ads_per_day = state.ads_per_day;
}

SETTINGS_STATE::~SETTINGS_STATE() = default;

bool SETTINGS_STATE::LoadFromJson(const std::string& json) {
  rapidjson::Document settings;
  settings.Parse(json.c_str());

  if (settings.HasParseError()) {
    return false;
  }

  if (settings.HasMember("ads.enabled")) {
    ads_enabled = settings["ads.enabled"].GetBool();
  }

  if (settings.HasMember("ads.locale")) {
    ads_locale = settings["ads.locale"].GetString();
  }

  if (settings.HasMember("ads.amount.hour")) {
    ads_per_hour = settings["ads.amount.hour"].GetString();
  } else {
    ads_per_hour = kDefaultAdsPerHour;
  }

  if (settings.HasMember("ads.amount.day")) {
    ads_per_day = settings["ads.amount.day"].GetString();
  } else {
    ads_per_day = kDefaultAdsPerDay;
  }

  return true;
}

}  // namespace ads

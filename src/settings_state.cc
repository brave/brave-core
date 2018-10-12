/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <map>

#include "../include/settings_state.h"
#include "../include/json_helper.h"
#include "../include/static_values.h"

namespace state {

SETTINGS_STATE::SETTINGS_STATE() :
    ads_enabled(false),
    ads_locale("en"),
    ads_amount_day(""),
    ads_amount_hour("") {}

SETTINGS_STATE::SETTINGS_STATE(const SETTINGS_STATE& state) {
  ads_enabled = state.ads_enabled;
  ads_locale = state.ads_locale;
  ads_amount_day = state.ads_amount_day;
  ads_amount_hour = state.ads_amount_hour;
}

SETTINGS_STATE::~SETTINGS_STATE() = default;

bool SETTINGS_STATE::LoadFromJson(const std::string& json) {
  rapidjson::Document settings;
  settings.Parse(json.c_str());

  if (settings.HasParseError()) {
    LOG(ERROR) << "Failed to parse Settings JSON" << std::endl;

    return false;
  }

  const std::map<std::string, std::string> members = {
    {"ads.enabled", "Bool"},
    {"ads.locale", "String"},
    {"ads.amount.day", "String"},
    {"ads.amount.hour", "String"}
  };

  // TODO(Terry Mancey): Refactor validateJson by moving to json_helper class
  for (auto& member : settings.GetObject()) {
    std::string member_name = member.name.GetString();
    std::string member_type = _rapidjson_member_types[member.value.GetType()];

    if (members.find(member_name) == members.end()) {
      LOG(WARNING) "JSON " << member_name << " member not used" << std::endl;
      continue;
    }

    std::string type = members.at(member_name);
    if (type.compare(member_type) != 0) {
      LOG(WARNING) << "Invalid type for JSON member "
        << member_name << std::endl;
      return false;
    }
  }

  if (settings.HasMember("ads.enabled")) {
    ads_enabled = settings["ads.enabled"].GetBool();
  }

  if (settings.HasMember("ads.locale")) {
    ads_locale = settings["ads.locale"].GetString();
  }

  if (settings.HasMember("ads.amount.day")) {
    ads_amount_day = settings["ads.amount.day"].GetString();
  }

  if (settings.HasMember("ads.amount.hour")) {
    ads_amount_hour = settings["ads.amount.hour"].GetString();
  }

  return true;
}

}  // namespace state

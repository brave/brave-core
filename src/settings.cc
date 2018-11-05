/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "settings.h"
#include "json_helper.h"

namespace ads {

Settings::Settings(AdsClient* ads_client) :
    ads_client_(ads_client),
    settings_state_(new SETTINGS_STATE()) {
}

Settings::~Settings() = default;

bool Settings::LoadJson(const std::string& json) {
  SETTINGS_STATE state;
  if (!LoadFromJson(state, json.c_str())) {
    ads_client_->DebugLog(LogLevel::ERROR, "Failed to parse settings JSON");
    return false;
  }

  settings_state_.reset(new SETTINGS_STATE(state));

  return true;
}

bool Settings::IsAdsEnabled() const {
  return settings_state_->ads_enabled;
}

std::string Settings::GetAdsLocale() const {
  return settings_state_->ads_locale;
}

uint64_t Settings::GetAdsPerHour() const {
  return std::atoll(settings_state_->ads_per_hour.c_str());
}

uint64_t Settings::GetAdsPerDay() const {
  return std::atoll(settings_state_->ads_per_day.c_str());
}

}  // namespace ads

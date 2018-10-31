/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <memory>

#include "ads_impl.h"
#include "ads_client.h"
#include "settings_state.h"

namespace rewards_ads {
class AdsImpl;
}  // namespace rewards_ads

namespace state {

class Settings {
 public:
  explicit Settings(ads::AdsClient* ads_client);
  ~Settings();

  bool LoadJson(const std::string& json);  // Deserialize

  bool IsAdsEnabled() const;
  std::string GetAdsLocale() const;
  uint64_t GetAdsPerHour() const;
  uint64_t GetAdsPerDay() const;

 private:
  ads::AdsClient* ads_client_;  // NOT OWNED

  std::unique_ptr<SETTINGS_STATE> settings_state_;
};

}  // namespace state

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <memory>

#include "../include/ads_impl.h"
#include "../include/settings_state.h"

namespace rewards_ads {
class AdsImpl;
}  // namespace rewards_ads

namespace state {

class Settings: public ads::CallbackHandler {
 public:
  Settings(rewards_ads::AdsImpl* ads, ads::AdsClient* ads_client);
  ~Settings();

  bool LoadState(const std::string& json);

  bool IsAdsEnabled() const;
  std::string GetAdsLocale() const;
  uint64_t GetAdsAmountPerHour() const;
  uint64_t GetAdsAmountPerDay() const;

 private:
  rewards_ads::AdsImpl* ads_;  // NOT OWNED
  ads::AdsClient* ads_client_;  // NOT OWNED

  std::unique_ptr<SETTINGS_STATE> settings_state_;
};

}  // namespace state

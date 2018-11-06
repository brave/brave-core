/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <memory>

#include "ads_impl.h"
#include "bat/ads/ads_client.h"
#include "settings_state.h"

namespace ads {
class AdsImpl;
}  // namespace ads

namespace ads {

class Settings {
 public:
  explicit Settings();
  ~Settings();

  bool FromJson(const std::string& json);  // Deserialize

  bool IsAdsEnabled() const;
  std::string GetAdsLocale() const;
  uint64_t GetAdsPerHour() const;
  uint64_t GetAdsPerDay() const;

 private:
  std::unique_ptr<SETTINGS_STATE> settings_state_;
};

}  // namespace ads

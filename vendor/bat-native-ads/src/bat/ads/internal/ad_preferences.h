/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_PREFERENCES_H_
#define BAT_ADS_INTERNAL_AD_PREFERENCES_H_

#include <string>
#include <vector>

#include "bat/ads/result.h"

namespace ads {

struct FilteredAd;
struct FilteredCategory;
struct FlaggedAd;
struct SavedAd;

struct AdPreferences {
  AdPreferences();
  AdPreferences(
      const AdPreferences& prefs);
  ~AdPreferences();

  const std::string ToJson() const;
  Result FromJson(
      const std::string& json,
      std::string* error_description = nullptr);

  std::vector<FilteredAd> filtered_ads;
  std::vector<FilteredCategory> filtered_categories;
  std::vector<SavedAd> saved_ads;
  std::vector<FlaggedAd> flagged_ads;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_PREFERENCES_H_

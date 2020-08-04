/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CLIENT_PREFERENCES_AD_PREFERENCES_H_
#define BAT_ADS_INTERNAL_CLIENT_PREFERENCES_AD_PREFERENCES_H_

#include <string>

#include "bat/ads/internal/client/preferences/filtered_ad.h"
#include "bat/ads/internal/client/preferences/filtered_category.h"
#include "bat/ads/internal/client/preferences/flagged_ad.h"
#include "bat/ads/internal/client/preferences/saved_ad.h"
#include "bat/ads/result.h"

namespace ads {

struct AdPreferences {
  AdPreferences();
  AdPreferences(
      const AdPreferences& prefs);
  ~AdPreferences();

  std::string ToJson() const;
  Result FromJson(
      const std::string& json);

  FilteredAdsList filtered_ads;
  FilteredCategoriesList filtered_categories;
  SavedAdsList saved_ads;
  FlaggedAdsList flagged_ads;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CLIENT_PREFERENCES_AD_PREFERENCES_H_

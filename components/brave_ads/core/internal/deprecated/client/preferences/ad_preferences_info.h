/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_CLIENT_PREFERENCES_AD_PREFERENCES_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_CLIENT_PREFERENCES_AD_PREFERENCES_INFO_H_

#include <string>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/preferences/filtered_advertiser_info.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/preferences/filtered_category_info.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/preferences/flagged_ad_info.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/preferences/saved_ad_info.h"

namespace brave_ads {

struct AdPreferencesInfo final {
  AdPreferencesInfo();

  AdPreferencesInfo(const AdPreferencesInfo&);
  AdPreferencesInfo& operator=(const AdPreferencesInfo&);

  AdPreferencesInfo(AdPreferencesInfo&&) noexcept;
  AdPreferencesInfo& operator=(AdPreferencesInfo&&) noexcept;

  ~AdPreferencesInfo();

  base::Value::Dict ToValue() const;
  void FromValue(const base::Value::Dict& dict);

  std::string ToJson() const;
  [[nodiscard]] bool FromJson(const std::string& json);

  FilteredAdvertiserList filtered_advertisers;
  FilteredCategoryList filtered_categories;
  SavedAdList saved_ads;
  FlaggedAdList flagged_ads;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_CLIENT_PREFERENCES_AD_PREFERENCES_INFO_H_

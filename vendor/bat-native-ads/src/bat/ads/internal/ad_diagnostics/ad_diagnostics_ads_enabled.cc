/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_ads_enabled.h"
#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_util.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/pref_names.h"

namespace ads {

AdDiagnosticsAdsEnabled::AdDiagnosticsAdsEnabled() = default;

AdDiagnosticsAdsEnabled::~AdDiagnosticsAdsEnabled() = default;

AdDiagnosticsEntryType AdDiagnosticsAdsEnabled::GetEntryType() const {
  return AdDiagnosticsEntryType::kAdsEnable;
}

std::string AdDiagnosticsAdsEnabled::GetKey() const {
  return "Ads enabled";
}

std::string AdDiagnosticsAdsEnabled::GetValue() const {
  const bool ads_enabled =
      AdsClientHelper::Get()->GetBooleanPref(prefs::kEnabled);
  return ConvertToString(ads_enabled);
}

}  // namespace ads

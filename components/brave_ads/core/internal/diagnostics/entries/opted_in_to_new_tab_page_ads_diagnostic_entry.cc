/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/opted_in_to_new_tab_page_ads_diagnostic_entry.h"

#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/strings/string_conversions_util.h"
#include "brave/components/ntp_background_images/common/pref_names.h"

namespace brave_ads {

namespace {
constexpr char kName[] = "Opted-in to new tab page ads";
}  // namespace

DiagnosticEntryType OptedInToNewTabPageAdsDiagnosticEntry::GetType() const {
  return DiagnosticEntryType::kOptedInToNewTabPageAds;
}

std::string OptedInToNewTabPageAdsDiagnosticEntry::GetName() const {
  return kName;
}

std::string OptedInToNewTabPageAdsDiagnosticEntry::GetValue() const {
  const bool is_opted_in =
      AdsClientHelper::GetInstance()->GetBooleanPref(
          ntp_background_images::prefs::kNewTabPageShowBackgroundImage) &&
      AdsClientHelper::GetInstance()->GetBooleanPref(
          ntp_background_images::prefs::
              kNewTabPageShowSponsoredImagesBackgroundImage);
  return BoolToString(is_opted_in);
}

}  // namespace brave_ads

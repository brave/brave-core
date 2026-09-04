/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/sponsored_ads_enabled_diagnostic_entry.h"

#include <string_view>

#include "brave/components/brave_ads/core/internal/common/strings/string_conversions_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"

namespace brave_ads {

namespace {
constexpr std::string_view kName = "Sponsored ads enabled";
}  // namespace

DiagnosticEntryType SponsoredAdsEnabledDiagnosticEntry::GetType() const {
  return DiagnosticEntryType::kSponsoredAdsEnabled;
}

std::string SponsoredAdsEnabledDiagnosticEntry::GetName() const {
  return std::string(kName);
}

std::string SponsoredAdsEnabledDiagnosticEntry::GetValue() const {
  return BoolToString(IsSponsoredAdsEnabled());
}

}  // namespace brave_ads

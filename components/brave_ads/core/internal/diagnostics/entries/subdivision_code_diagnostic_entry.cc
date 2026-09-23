/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/subdivision_code_diagnostic_entry.h"

#include "brave/components/brave_ads/core/internal/ad_units/ad_handler.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"

namespace brave_ads {

namespace {
constexpr char kName[] = "Subdivision code";
constexpr char kNotApplicable[] = "N/A";
}  // namespace

DiagnosticEntryType SubdivisionCodeDiagnosticEntry::GetType() const {
  return DiagnosticEntryType::kSubdivisionCode;
}

std::string SubdivisionCodeDiagnosticEntry::GetName() const {
  return kName;
}

std::string SubdivisionCodeDiagnosticEntry::GetValue() const {
  const SubdivisionTargeting& subdivision_targeting =
      GetAdHandler().GetSubdivisionTargeting();
  if (subdivision_targeting.IsDisabled()) {
    return kNotApplicable;
  }

  const std::string& subdivision = subdivision_targeting.GetSubdivision();
  return subdivision.empty() ? kNotApplicable : subdivision;
}

}  // namespace brave_ads

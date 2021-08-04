/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_last_unidle_timestamp.h"
#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_util.h"

namespace ads {

AdDiagnosticsLastUnIdleTimestamp::AdDiagnosticsLastUnIdleTimestamp() = default;

AdDiagnosticsLastUnIdleTimestamp::~AdDiagnosticsLastUnIdleTimestamp() = default;

AdDiagnosticsEntryType AdDiagnosticsLastUnIdleTimestamp::GetEntryType() const {
  return AdDiagnosticsEntryType::kLastUnIdleTimestamp;
}

void AdDiagnosticsLastUnIdleTimestamp::SetLastUnIdleTimestamp(
    const base::Time& time) {
  last_unidle_timestamp_ = time;
}

std::string AdDiagnosticsLastUnIdleTimestamp::GetKey() const {
  return "Last unidle timestamp";
}

std::string AdDiagnosticsLastUnIdleTimestamp::GetValue() const {
  return ConvertToString(last_unidle_timestamp_);
}

}  // namespace ads

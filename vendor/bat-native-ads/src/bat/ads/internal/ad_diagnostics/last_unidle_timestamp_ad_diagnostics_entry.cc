/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_diagnostics/last_unidle_timestamp_ad_diagnostics_entry.h"

#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_util.h"

namespace ads {

LastUnIdleTimestampAdDiagnosticsEntry::LastUnIdleTimestampAdDiagnosticsEntry() =
    default;

LastUnIdleTimestampAdDiagnosticsEntry::
    ~LastUnIdleTimestampAdDiagnosticsEntry() = default;

AdDiagnosticsEntryType LastUnIdleTimestampAdDiagnosticsEntry::GetEntryType()
    const {
  return AdDiagnosticsEntryType::kLastUnIdleTimestamp;
}

void LastUnIdleTimestampAdDiagnosticsEntry::SetLastUnIdleTimestamp(
    const base::Time& time) {
  last_unidle_timestamp_ = time;
}

std::string LastUnIdleTimestampAdDiagnosticsEntry::GetKey() const {
  return "Last unidle timestamp";
}

std::string LastUnIdleTimestampAdDiagnosticsEntry::GetValue() const {
  return ConvertToString(last_unidle_timestamp_);
}

}  // namespace ads

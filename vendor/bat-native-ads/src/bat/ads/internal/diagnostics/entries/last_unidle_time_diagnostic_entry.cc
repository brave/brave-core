/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/diagnostics/entries/last_unidle_time_diagnostic_entry.h"

#include "bat/ads/internal/common/time/time_formatting_util.h"

namespace ads {

namespace {

constexpr char kName[] = "Last unidle time";
constexpr char kNever[] = "Never";

}  // namespace

void LastUnIdleTimeDiagnosticEntry::SetLastUnIdleTime(const base::Time time) {
  last_unidle_time_ = time;
}

DiagnosticEntryType LastUnIdleTimeDiagnosticEntry::GetType() const {
  return DiagnosticEntryType::kLastUnIdleTime;
}

std::string LastUnIdleTimeDiagnosticEntry::GetName() const {
  return kName;
}

std::string LastUnIdleTimeDiagnosticEntry::GetValue() const {
  if (last_unidle_time_.is_null()) {
    return kNever;
  }

  return LongFriendlyDateAndTime(last_unidle_time_,
                                 /*use_sentence_style*/ false);
}

}  // namespace ads

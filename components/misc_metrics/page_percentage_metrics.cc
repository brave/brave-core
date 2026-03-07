/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/page_percentage_metrics.h"

#include "base/time/time.h"
#include "brave/components/p3a_utils/bucket.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace misc_metrics {

namespace {

constexpr base::TimeDelta kReportInterval = base::Hours(24);

}  // namespace

PagePercentageMetrics::PagePercentageMetrics(
    PrefService* local_state,
    std::string_view counts_pref_key,
    std::string_view frame_start_pref_key)
    : local_state_(local_state),
      counts_pref_key_(counts_pref_key),
      frame_start_pref_key_(frame_start_pref_key) {
  base::Time frame_start = local_state_->GetTime(frame_start_pref_key_);
  if (frame_start.is_null()) {
    local_state_->SetTime(frame_start_pref_key_, base::Time::Now());
  }
}

void PagePercentageMetrics::IncrementDictCount(std::string_view key) {
  ScopedDictPrefUpdate update(local_state_, counts_pref_key_);
  int current = update->FindInt(key).value_or(0);
  update->Set(key, current + 1);
}

void PagePercentageMetrics::RecordPercentageHistogram(
    const base::DictValue& counts,
    int total,
    std::string_view count_key,
    const char* histogram_name) {
  if (total == 0) {
    return;
  }
  int count = counts.FindInt(count_key).value_or(0);
  if (count == 0) {
    return;
  }
  int percentage = std::max(1, (count * 100) / total);
  p3a_utils::RecordToHistogramBucket(histogram_name, kPercentageBuckets,
                                     percentage);
}

bool PagePercentageMetrics::HasReportIntervalElapsed() const {
  base::Time frame_start = local_state_->GetTime(frame_start_pref_key_);
  base::Time now = base::Time::Now();
  return now - frame_start >= kReportInterval;
}

void PagePercentageMetrics::ResetCounts() {
  local_state_->SetTime(frame_start_pref_key_, base::Time::Now());
  local_state_->SetDict(counts_pref_key_, {});
}

}  // namespace misc_metrics

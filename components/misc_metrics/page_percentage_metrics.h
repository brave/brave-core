/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_PAGE_PERCENTAGE_METRICS_H_
#define BRAVE_COMPONENTS_MISC_METRICS_PAGE_PERCENTAGE_METRICS_H_

#include <string_view>

#include "base/memory/raw_ptr.h"
#include "base/values.h"

class PrefService;

namespace misc_metrics {

inline constexpr int kPercentageBuckets[] = {0, 5, 20, 80, 95};

// Base class for tracking percentage-based metrics over a reporting interval.
class PagePercentageMetrics {
 protected:
  PagePercentageMetrics(PrefService* local_state,
                        std::string_view counts_pref_key,
                        std::string_view frame_start_pref_key);

  void IncrementDictCount(std::string_view key);
  void RecordPercentageHistogram(const base::DictValue& counts,
                                 int total,
                                 std::string_view count_key,
                                 const char* histogram_name);
  bool HasReportIntervalElapsed() const;
  void ResetCounts();

  raw_ptr<PrefService> local_state_ = nullptr;

 private:
  std::string_view counts_pref_key_;
  std::string_view frame_start_pref_key_;
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_PAGE_PERCENTAGE_METRICS_H_

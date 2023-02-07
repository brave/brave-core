// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_P3A_UTILS_COUNT_REPORT_LIMITER_H_
#define BRAVE_COMPONENTS_P3A_UTILS_COUNT_REPORT_LIMITER_H_

#include "base/functional/callback.h"
#include "base/time/time.h"
#include "base/timer/timer.h"

namespace p3a_utils {

// Utility that consumes counted events, and reports totals to the
// report_callback, for a given report_period. If the events surpass the
// max_rate for the report_period, then reporting will be paused until the rate
// is <= the max_rate for a given period. The first use case of this utility is
// for pausing News card view counts if the user is scrolling excessively.
class CountReportLimiter {
 public:
  CountReportLimiter(uint64_t max_rate,
                     base::TimeDelta report_period,
                     base::RepeatingCallback<void(uint64_t)> report_callback);
  ~CountReportLimiter();

  CountReportLimiter(const CountReportLimiter&) = delete;
  CountReportLimiter& operator=(const CountReportLimiter&) = delete;

  void Add(uint64_t count);

 private:
  void OnReportInterval();

  uint64_t frame_event_count_;

  uint64_t max_rate_;
  base::TimeDelta report_period_;
  base::RepeatingCallback<void(uint64_t)> report_callback_;

  base::OneShotTimer update_timer_;
};

}  // namespace p3a_utils

#endif  // BRAVE_COMPONENTS_P3A_UTILS_COUNT_REPORT_LIMITER_H_

// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a_utils/count_report_limiter.h"

#include "base/logging.h"

namespace p3a_utils {

CountReportLimiter::CountReportLimiter(
    uint64_t max_rate,
    base::TimeDelta report_period,
    base::RepeatingCallback<void(uint64_t)> report_callback)
    : frame_event_count_(0),
      max_rate_(max_rate),
      report_period_(report_period),
      report_callback_(report_callback) {}

CountReportLimiter::~CountReportLimiter() = default;

void CountReportLimiter::Add(uint64_t count) {
  frame_event_count_ += count;
  if (!update_timer_.IsRunning()) {
    update_timer_.Start(FROM_HERE, report_period_, this,
                        &CountReportLimiter::OnReportInterval);
  }
}

void CountReportLimiter::OnReportInterval() {
  if (frame_event_count_ <= max_rate_) {
    report_callback_.Run(frame_event_count_);
    VLOG(2) << "CountReportLimiter: frame event count <= max rate, reporting";
  } else {
    VLOG(2)
        << "CountReportLimiter: frame event count > max rate, skipping report";
  }
  frame_event_count_ = 0;
}

}  // namespace p3a_utils

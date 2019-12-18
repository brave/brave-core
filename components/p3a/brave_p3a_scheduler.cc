/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_scheduler.h"

#include "base/rand_util.h"

namespace brave {

namespace {

// ----------------------------------------------
// Cargoculted from |metrics_upload_scheduler.cc|
// ----------------------------------------------

// When uploading metrics to the server fails, we progressively wait longer and
// longer before sending the next log. This backoff process helps reduce load
// on a server that is having issues.
// The following is the multiplier we use to expand that inter-log duration.
constexpr double kBackoffMultiplier = 2;

// The maximum backoff interval in hours.
constexpr int64_t kMaxBackoffIntervalHours = 1;

constexpr int64_t kInitialBackoffIntervalSeconds = 5;

// Increases the upload interval each time it's called, to handle the case
// where the server is having issues.
base::TimeDelta BackOffUploadInterval(base::TimeDelta interval) {
  DCHECK_GT(kBackoffMultiplier, 1.0);
  interval = base::TimeDelta::FromMicroseconds(
      static_cast<int64_t>(kBackoffMultiplier * interval.InMicroseconds()));

  base::TimeDelta max_interval =
      base::TimeDelta::FromHours(kMaxBackoffIntervalHours);
  if (interval > max_interval || interval.InSeconds() < 0) {
    interval = max_interval;
  }
  return interval;
}

}  // namespace

BraveP3AScheduler::BraveP3AScheduler(
    const base::Closure& upload_callback,
    const base::Callback<base::TimeDelta(void)>& get_interval_callback)
    : metrics::MetricsScheduler(upload_callback,
                                false /* fast_startup_for_testing */),
      get_interval_callback_(get_interval_callback),
      initial_backoff_interval_(
          base::TimeDelta::FromSeconds(kInitialBackoffIntervalSeconds)),
      backoff_interval_(
          base::TimeDelta::FromSeconds(kInitialBackoffIntervalSeconds)) {}

BraveP3AScheduler::~BraveP3AScheduler() {}

void BraveP3AScheduler::UploadFinished(bool ok) {
  if (!ok) {
    TaskDone(backoff_interval_);
    backoff_interval_ = BackOffUploadInterval(backoff_interval_);
  } else {
    backoff_interval_ = initial_backoff_interval_;
    TaskDone(get_interval_callback_.Run());
  }
}

}  // namespace brave

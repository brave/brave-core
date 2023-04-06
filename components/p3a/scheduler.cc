/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/scheduler.h"

#include "base/rand_util.h"
#include "brave/vendor/brave_base/random.h"

namespace p3a {

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
  interval = base::Microseconds(
      static_cast<int64_t>(kBackoffMultiplier * interval.InMicroseconds()));

  base::TimeDelta max_interval = base::Hours(kMaxBackoffIntervalHours);
  if (interval > max_interval || interval.InSeconds() < 0) {
    interval = max_interval;
  }
  return interval;
}

base::TimeDelta GetRandomizedUploadInterval(
    base::TimeDelta average_upload_interval) {
  const auto delta = base::Seconds(
      brave_base::random::Geometric(average_upload_interval.InSecondsF()));
  return delta;
}

}  // namespace

Scheduler::Scheduler(const base::RepeatingClosure& upload_callback,
                     bool randomize_upload_interval,
                     base::TimeDelta average_upload_interval)
    : metrics::MetricsScheduler(upload_callback,
                                false /* fast_startup_for_testing */),
      initial_backoff_interval_(base::Seconds(kInitialBackoffIntervalSeconds)),
      backoff_interval_(base::Seconds(kInitialBackoffIntervalSeconds)),
      randomize_upload_interval_(randomize_upload_interval),
      average_upload_interval_(average_upload_interval) {}

Scheduler::~Scheduler() = default;

void Scheduler::UploadFinished(bool ok) {
  if (!ok) {
    TaskDone(backoff_interval_);
    backoff_interval_ = BackOffUploadInterval(backoff_interval_);
  } else {
    backoff_interval_ = initial_backoff_interval_;
    if (randomize_upload_interval_) {
      TaskDone(GetRandomizedUploadInterval(average_upload_interval_));
    } else {
      TaskDone(average_upload_interval_);
    }
  }
}

}  // namespace p3a

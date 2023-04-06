/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_SCHEDULER_H_
#define BRAVE_COMPONENTS_P3A_SCHEDULER_H_

#include "base/functional/callback_forward.h"
#include "components/metrics/metrics_scheduler.h"

namespace p3a {

// Handles scheduling of metric uploads/Constellation metric preparation.
// Will callback to MessageManager on a given interval.
class Scheduler : public metrics::MetricsScheduler {
 public:
  explicit Scheduler(const base::RepeatingClosure& upload_callback,
                     bool randomize_upload_interval,
                     base::TimeDelta average_upload_interval);
  Scheduler(const Scheduler&) = delete;
  Scheduler& operator=(const Scheduler&) = delete;
  ~Scheduler() override;

  void UploadFinished(bool ok);

 private:
  // Provides us with the interval between successful uploads.
  base::RepeatingCallback<base::TimeDelta(void)> get_interval_callback_;

  // Initial time to wait between upload retry attempts.
  const base::TimeDelta initial_backoff_interval_;

  // Time to wait for the next upload attempt if the next one fails.
  base::TimeDelta backoff_interval_;

  bool randomize_upload_interval_;

  base::TimeDelta average_upload_interval_;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_SCHEDULER_H_

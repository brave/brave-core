/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_SCHEDULER_H_
#define BRAVE_COMPONENTS_P3A_SCHEDULER_H_

#include "base/functional/callback_forward.h"
#include "base/time/time.h"
#include "components/metrics/metrics_scheduler.h"

namespace p3a {

// Handles scheduling of metric uploads/Constellation metric preparation.
// Will callback to MessageManager on a given interval.
class Scheduler : public metrics::MetricsScheduler {
 public:
  Scheduler(const base::RepeatingClosure& upload_callback,
            bool randomize_upload_interval,
            base::TimeDelta average_upload_interval,
            base::TimeDelta average_priority_interval);
  Scheduler(const Scheduler&) = delete;
  Scheduler& operator=(const Scheduler&) = delete;
  ~Scheduler() override;

  // |priority_messages_pending| schedules the first task at the accelerated
  // priority interval instead of the standard one.
  void Start(bool priority_messages_pending);
  void Start() = delete;

  // |priority_messages_pending| schedules the next task at the accelerated
  // priority interval instead of the standard one.
  void UploadSucceeded(bool priority_messages_pending);

  // Schedules the next task at the current backoff interval, expanding the
  // backoff window for subsequent failures.
  void UploadFailed();

  // Shortens the wait before the next task to the priority interval. No-op if
  // a task is in flight, or if the next task is already expedited.
  void ExpediteForPriority();

 private:
  base::TimeDelta GenerateInterval() const;

  // Initial time to wait between upload retry attempts.
  const base::TimeDelta initial_backoff_interval_;

  // Time to wait for the next upload attempt if the next one fails.
  base::TimeDelta backoff_interval_;

  bool randomize_upload_interval_;

  base::TimeDelta average_upload_interval_;

  base::TimeDelta average_priority_interval_;

  // True if the currently scheduled task uses the priority interval.
  bool is_priority_scheduled_ = false;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_SCHEDULER_H_

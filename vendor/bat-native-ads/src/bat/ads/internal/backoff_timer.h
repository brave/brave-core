/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BACKOFF_TIMER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BACKOFF_TIMER_H_

#include <cstdint>
#include <memory>

#include "base/callback_forward.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "bat/ads/internal/timer.h"

namespace ads {

class BackoffTimer {
 public:
  BackoffTimer();

  ~BackoffTimer();

  // Set a mock implementation of base::OneShotTimer which requires |Fire()| to
  // be explicitly called. Prefer using TaskEnvironment::MOCK_TIME +
  // FastForward*() to this when possible
  void set_timer_for_testing(std::unique_ptr<base::OneShotTimer> timer);

  // Start a timer to run at the given |delay| from now backing off
  // exponentially for each call. If the timer is already running, it will be
  // replaced to call the given |user_task|. Returns the time the delayed task
  // will be fired
  base::Time Start(const base::TimeDelta& delay, base::OnceClosure user_task);

  // Start a timer to run at a geometrically distributed number of seconds
  // |~delay| from now backing off exponentially for each call. If the timer is
  // already running, it will be replaced to call the given |user_task|. Returns
  // the time the delayed task will be fired
  base::Time StartWithPrivacy(const base::TimeDelta& delay,
                              base::OnceClosure user_task);

  // Returns true if the timer is running (i.e., not stopped)
  bool IsRunning() const;

  // Run the scheduled task immediately, and stop the timer. The timer needs to
  // be running
  void FireNow();

  // Call this method to stop the timer if running and to reset the exponential
  // backoff delay. Returns |true| if the timer was stopped, otherwise returns
  // |false|
  bool Stop();

  // Optionally call this method to set the maximum backoff delay to
  // |max_delay|. Default maximum backoff delay is 1 hour
  void set_max_backoff_delay(const base::TimeDelta& max_delay);

 private:
  base::TimeDelta CalculateDelay(const base::TimeDelta& delay);

  Timer timer_;

  uint64_t backoff_count_ = 0;
  base::TimeDelta max_backoff_delay_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BACKOFF_TIMER_H_

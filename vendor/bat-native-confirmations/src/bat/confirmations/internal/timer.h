/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_TIMER_H_
#define BAT_CONFIRMATIONS_INTERNAL_TIMER_H_

#include <stdint.h>

#include <memory>

#include "base/bind.h"
#include "base/time/time.h"
#include "base/timer/timer.h"

namespace confirmations {

class Timer {
 public:
  Timer();

  ~Timer();

  // Set a mock implementation of base::OneShotTimer which requires |Fire()| to
  // be explicitly called. Prefer using TaskEnvironment::MOCK_TIME +
  // FastForward*() to this when possible
  void set_timer_for_testing(
      std::unique_ptr<base::OneShotTimer> timer);

  // Start a timer to run at the given |delay| from now. If the timer is already
  // running, it will be replaced to call the given |user_task|. Returns the
  // time the delayed task will be fired
  base::Time Start(
      const uint64_t delay,
      base::OnceClosure user_task);

  // Start a timer to run at a geometrically distributed number of seconds
  // |~delay| from now for privacy-focused events. If the timer is already
  // running, it will be replaced to call the given |user_task|. Returns the
  // time the delayed task will be fired
  base::Time StartWithPrivacy(
      const uint64_t delay,
      base::OnceClosure user_task);

  // Returns true if the timer is running (i.e., not stopped)
  bool IsRunning() const;

  // Call this method to stop the timer. It is a no-op if the timer is not
  // running
  void Stop();

 private:
  std::unique_ptr<base::OneShotTimer> timer_;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_TIMER_H_

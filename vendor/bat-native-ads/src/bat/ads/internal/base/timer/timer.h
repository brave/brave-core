/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_TIMER_TIMER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_TIMER_TIMER_H_

#include <memory>

#include "base/callback.h"

namespace base {
class Location;
class Time;
class TimeDelta;
class WallClockTimer;
}  // namespace base

namespace ads {

class Timer final {
 public:
  Timer();
  ~Timer();
  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;

  // |location| provides basic info where the timer was posted from. Start a
  // timer to run at the given |delay| from now. If the timer is already
  // running, it will be replaced to call the given |user_task|. Returns the
  // time the delayed task will be fired.
  base::Time Start(const base::Location& location,
                   const base::TimeDelta delay,
                   base::OnceClosure user_task);

  // |location| provides basic info where the timer was posted from. Returns the
  // time the delayed task will be fired. Start a timer to run at a
  // geometrically distributed number of seconds |~delay| from now. If the timer
  // is already running, it will be replaced to call the given |user_task|.
  base::Time StartWithPrivacy(const base::Location& location,
                              const base::TimeDelta delay,
                              base::OnceClosure user_task);

  // Returns true if the timer is running (i.e., not stopped).
  bool IsRunning() const;

  // Call this method to stop the timer. It is a no-op if the timer is not
  // running. Returns |true| if the timer was stopped, otherwise returns
  // |false|.
  bool Stop();

 private:
  std::unique_ptr<base::WallClockTimer> timer_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_TIMER_TIMER_H_

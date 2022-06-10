/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_BRAVE_P3A_ROTATION_SCHEDULER_H_
#define BRAVE_COMPONENTS_P3A_BRAVE_P3A_ROTATION_SCHEDULER_H_

#include "base/callback.h"
#include "base/time/time.h"
#include "base/timer/wall_clock_timer.h"

class PrefService;
class PrefRegistrySimple;

namespace brave {

class BraveP3ARotationScheduler {
  using RotationCallback = base::RepeatingCallback<void()>;

 public:
  BraveP3ARotationScheduler(PrefService* local_state,
                            base::TimeDelta json_rotation_interval,
                            RotationCallback json_rotation_callback,
                            RotationCallback star_rotation_callback);

  ~BraveP3ARotationScheduler();

  static void RegisterPrefs(PrefRegistrySimple* registry);

 private:
  void InitJsonTimer();
  void InitStarTimer();

  void UpdateJsonTimer();
  void UpdateStarTimer();

  void HandleJsonTimerTrigger();
  void HandleStarTimerTrigger();

  base::WallClockTimer json_rotation_timer_;
  base::WallClockTimer star_rotation_timer_;

  RotationCallback json_rotation_callback_;
  RotationCallback star_rotation_callback_;

  PrefService* local_state_;

  base::TimeDelta json_rotation_interval_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_BRAVE_P3A_ROTATION_SCHEDULER_H_

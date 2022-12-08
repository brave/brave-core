/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_BRAVE_P3A_ROTATION_SCHEDULER_H_
#define BRAVE_COMPONENTS_P3A_BRAVE_P3A_ROTATION_SCHEDULER_H_

#include <memory>

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/time/time.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/p3a/brave_p3a_config.h"
#include "brave/components/p3a/metric_log_type.h"

class PrefService;
class PrefRegistrySimple;

namespace brave {

class BraveP3ARotationScheduler {
  using JsonRotationCallback =
      base::RepeatingCallback<void(MetricLogType log_type)>;
  using StarRotationCallback = base::RepeatingCallback<void()>;

 public:
  BraveP3ARotationScheduler(PrefService* local_state,
                            BraveP3AConfig* config,
                            JsonRotationCallback json_rotation_callback,
                            StarRotationCallback star_rotation_callback);

  ~BraveP3ARotationScheduler();

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void InitStarTimer(base::Time next_epoch_time);

  base::Time GetLastJsonRotationTime(MetricLogType log_type);
  base::Time GetLastStarRotationTime();

 private:
  void InitJsonTimer(MetricLogType log_type);
  void UpdateJsonTimer(MetricLogType log_type);

  void HandleJsonTimerTrigger(MetricLogType log_type);
  void HandleStarTimerTrigger();

  base::flat_map<MetricLogType, std::unique_ptr<base::WallClockTimer>>
      json_rotation_timers_;
  base::WallClockTimer star_rotation_timer_;

  JsonRotationCallback json_rotation_callback_;
  StarRotationCallback star_rotation_callback_;

  base::flat_map<MetricLogType, base::Time> last_json_rotation_times_;
  base::Time last_star_rotation_time_;

  PrefService* local_state_;
  BraveP3AConfig* config_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_BRAVE_P3A_ROTATION_SCHEDULER_H_

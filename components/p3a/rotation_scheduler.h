/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_ROTATION_SCHEDULER_H_
#define BRAVE_COMPONENTS_P3A_ROTATION_SCHEDULER_H_

#include <memory>

#include "base/containers/flat_map.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/time/time.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/p3a_config.h"

class PrefService;
class PrefRegistrySimple;

namespace p3a {

// Schedules reporting period rotation (i.e. monthly, daily, or weekly) and
// calls back to the MessageManager on a given interval.
class RotationScheduler {
  using RotationCallback =
      base::RepeatingCallback<void(MetricLogType log_type)>;

 public:
  RotationScheduler(PrefService& local_state,
                    const P3AConfig* config,
                    RotationCallback json_rotation_callback,
                    RotationCallback constellation_rotation_callback);

  ~RotationScheduler();

  RotationScheduler(const RotationScheduler&) = delete;
  RotationScheduler& operator=(const RotationScheduler&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void InitConstellationTimer(MetricLogType log_type,
                              base::Time next_epoch_time);

  base::Time GetLastJsonRotationTime(MetricLogType log_type);
  base::Time GetLastConstellationRotationTime(MetricLogType log_type);

 private:
  void InitJsonTimer(MetricLogType log_type);
  void UpdateJsonTimer(MetricLogType log_type);

  void HandleJsonTimerTrigger(MetricLogType log_type);
  void HandleConstellationTimerTrigger(MetricLogType log_type);

  base::flat_map<MetricLogType, std::unique_ptr<base::WallClockTimer>>
      json_rotation_timers_;
  base::flat_map<MetricLogType, std::unique_ptr<base::WallClockTimer>>
      constellation_rotation_timers_;

  RotationCallback json_rotation_callback_;
  RotationCallback constellation_rotation_callback_;

  base::flat_map<MetricLogType, base::Time> last_json_rotation_times_;
  base::flat_map<MetricLogType, base::Time> last_constellation_rotation_times_;

  const raw_ref<PrefService, DanglingUntriaged> local_state_;
  const raw_ptr<const P3AConfig> config_;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_ROTATION_SCHEDULER_H_

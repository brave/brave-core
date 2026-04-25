/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/rotation_scheduler.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/p3a/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace p3a {

namespace {

const char* GetConstellationRotationTimestampPref(MetricLogType log_type) {
  switch (log_type) {
    case MetricLogType::kSlow:
      return kLastSlowConstellationRotationTimeStampPref;
    case MetricLogType::kTypical:
      return kLastTypicalConstellationRotationTimeStampPref;
    case MetricLogType::kExpress:
      return kLastExpressConstellationRotationTimeStampPref;
  }
  NOTREACHED();
}

}  // namespace

RotationScheduler::RotationScheduler(
    PrefService& local_state,
    const P3AConfig* config,
    RotationCallback constellation_rotation_callback)
    : constellation_rotation_callback_(constellation_rotation_callback),
      local_state_(local_state),
      config_(config) {
  for (MetricLogType log_type : kAllMetricLogTypes) {
    constellation_rotation_timers_[log_type] =
        std::make_unique<base::WallClockTimer>();
    last_constellation_rotation_times_[log_type] =
        local_state_->GetTime(GetConstellationRotationTimestampPref(log_type));
  }
}

RotationScheduler::~RotationScheduler() = default;

void RotationScheduler::RegisterPrefs(PrefRegistrySimple* registry) {
  // Using "year ago" as default value to fix macOS test crashes
  const base::Time year_ago = base::Time::Now() - base::Days(365);
  registry->RegisterTimePref(kLastTypicalConstellationRotationTimeStampPref,
                             year_ago);
  registry->RegisterTimePref(kLastSlowConstellationRotationTimeStampPref,
                             year_ago);
  registry->RegisterTimePref(kLastExpressConstellationRotationTimeStampPref,
                             year_ago);
}

void RotationScheduler::RegisterLocalStatePrefsForMigration(
    PrefRegistrySimple* registry) {
  // Added 06/2025
  registry->RegisterTimePref(kLastSlowJsonRotationTimeStampPref, {});
  registry->RegisterTimePref(kLastTypicalJsonRotationTimeStampPref, {});
  registry->RegisterTimePref(kLastExpressJsonRotationTimeStampPref, {});
}

void RotationScheduler::MigrateObsoleteLocalStatePrefs(
    PrefService* local_state) {
  local_state->ClearPref(kLastSlowJsonRotationTimeStampPref);
  local_state->ClearPref(kLastTypicalJsonRotationTimeStampPref);
  local_state->ClearPref(kLastExpressJsonRotationTimeStampPref);
}

void RotationScheduler::InitConstellationTimer(MetricLogType log_type,
                                               base::Time next_epoch_time) {
  constellation_rotation_timers_[log_type]->Start(
      FROM_HERE, next_epoch_time + base::Seconds(5),
      base::BindOnce(&RotationScheduler::HandleConstellationTimerTrigger,
                     base::Unretained(this), log_type));
}

base::Time RotationScheduler::GetLastConstellationRotationTime(
    MetricLogType log_type) {
  return last_constellation_rotation_times_[log_type];
}

void RotationScheduler::HandleConstellationTimerTrigger(
    MetricLogType log_type) {
  last_constellation_rotation_times_[log_type] = base::Time::Now();
  local_state_->SetTime(GetConstellationRotationTimestampPref(log_type),
                        last_constellation_rotation_times_[log_type]);
  constellation_rotation_callback_.Run(log_type);
}

}  // namespace p3a

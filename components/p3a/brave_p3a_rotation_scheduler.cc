/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_rotation_scheduler.h"

#include "base/logging.h"
#include "base/timer/wall_clock_timer.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave {

namespace {

constexpr char kLastTypicalJsonRotationTimeStampPref[] =
    "p3a.last_rotation_timestamp";
constexpr char kLastExpressJsonRotationTimeStampPref[] =
    "p3a.last_express_rotation_timestamp";
constexpr char kLastStarRotationTimeStampPref[] =
    "p3a.last_star_rotation_timestamp";

base::Time NextMonday(base::Time time) {
  base::Time::Exploded exploded;
  time.LocalMidnight().LocalExplode(&exploded);
  // 1 stands for Monday, 0 for Sunday
  int days_till_monday = 0;
  if (exploded.day_of_week >= 1) {
    days_till_monday = 8 - exploded.day_of_week;
  } else {
    days_till_monday = 1;
  }

  // Adding few hours of padding to prevent potential problems with DST.
  base::Time result =
      (time.LocalMidnight() + base::Days(days_till_monday) + base::Hours(4))
          .LocalMidnight();
  return result;
}

base::Time NextDay(base::Time time) {
  return (time.LocalMidnight() + base::Days(1) + base::Hours(4))
      .LocalMidnight();
}

base::Time GetNextJsonRotationTime(MetricLogType log_type,
                                   base::Time last_rotation) {
  switch (log_type) {
    case MetricLogType::kTypical:
      return NextMonday(last_rotation);
    case MetricLogType::kExpress:
      return NextDay(last_rotation);
  }
  NOTREACHED();
}

const char* GetJsonRotationTimestampPref(MetricLogType log_type) {
  switch (log_type) {
    case MetricLogType::kTypical:
      return kLastTypicalJsonRotationTimeStampPref;
    case MetricLogType::kExpress:
      return kLastExpressJsonRotationTimeStampPref;
  }
  NOTREACHED();
  return nullptr;
}

}  // namespace

BraveP3ARotationScheduler::BraveP3ARotationScheduler(
    PrefService* local_state,
    BraveP3AConfig* config,
    JsonRotationCallback json_rotation_callback,
    StarRotationCallback star_rotation_callback)
    : json_rotation_callback_(json_rotation_callback),
      star_rotation_callback_(star_rotation_callback),
      local_state_(local_state),
      config_(config) {
  for (MetricLogType log_type : kAllMetricLogTypes) {
    json_rotation_timers_[log_type] = std::make_unique<base::WallClockTimer>();
    InitJsonTimer(log_type);
  }
}

void BraveP3ARotationScheduler::RegisterPrefs(PrefRegistrySimple* registry) {
  // Using "year ago" as default value to fix macOS test crashes
  const base::Time year_ago = base::Time::Now() - base::Days(365);
  registry->RegisterTimePref(kLastTypicalJsonRotationTimeStampPref, year_ago);
  registry->RegisterTimePref(kLastExpressJsonRotationTimeStampPref, year_ago);
  registry->RegisterTimePref(kLastStarRotationTimeStampPref, year_ago);
}

BraveP3ARotationScheduler::~BraveP3ARotationScheduler() {}

void BraveP3ARotationScheduler::InitJsonTimer(MetricLogType log_type) {
  // Do rotation if needed.
  const base::Time last_rotation =
      local_state_->GetTime(GetJsonRotationTimestampPref(log_type));
  last_json_rotation_times_[log_type] = last_rotation;
  base::Time next_rotation_time =
      GetNextJsonRotationTime(log_type, last_rotation);
  if (last_rotation.is_null()) {
    HandleJsonTimerTrigger(log_type);
    return;
  } else {
    if (!config_->json_rotation_intervals[log_type].is_zero()) {
      if (base::Time::Now() - last_rotation >
          config_->json_rotation_intervals[log_type]) {
        HandleJsonTimerTrigger(log_type);
        return;
      }
    }
    if (base::Time::Now() > next_rotation_time) {
      HandleJsonTimerTrigger(log_type);
      return;
    }
  }
  UpdateJsonTimer(log_type);
}

void BraveP3ARotationScheduler::InitStarTimer(base::Time next_epoch_time) {
  star_rotation_timer_.Start(
      FROM_HERE, next_epoch_time + base::Seconds(5), this,
      &BraveP3ARotationScheduler::HandleStarTimerTrigger);
}

void BraveP3ARotationScheduler::UpdateJsonTimer(MetricLogType log_type) {
  base::Time now = base::Time::Now();
  base::Time next_rotation =
      config_->json_rotation_intervals[log_type].is_zero()
          ? GetNextJsonRotationTime(log_type, now)
          : now + config_->json_rotation_intervals[log_type];
  if (now >= next_rotation) {
    // Should never happen, but let's stay on the safe side.
    NOTREACHED();
    return;
  }
  json_rotation_timers_[log_type]->Start(
      FROM_HERE, next_rotation,
      base::BindOnce(&BraveP3ARotationScheduler::HandleJsonTimerTrigger,
                     base::Unretained(this), log_type));

  VLOG(2) << "BraveP3ARotationScheduler new rotation timer will fire at "
          << next_rotation << " after " << next_rotation - now;
}

base::Time BraveP3ARotationScheduler::GetLastJsonRotationTime(
    MetricLogType log_type) {
  return last_json_rotation_times_[log_type];
}

base::Time BraveP3ARotationScheduler::GetLastStarRotationTime() {
  return last_star_rotation_time_;
}

void BraveP3ARotationScheduler::HandleJsonTimerTrigger(MetricLogType log_type) {
  last_json_rotation_times_[log_type] = base::Time::Now();
  local_state_->SetTime(GetJsonRotationTimestampPref(log_type),
                        last_json_rotation_times_[log_type]);
  UpdateJsonTimer(log_type);
  json_rotation_callback_.Run(log_type);
}

void BraveP3ARotationScheduler::HandleStarTimerTrigger() {
  last_star_rotation_time_ = base::Time::Now();
  local_state_->SetTime(kLastStarRotationTimeStampPref,
                        last_star_rotation_time_);
  star_rotation_callback_.Run();
}

}  // namespace brave

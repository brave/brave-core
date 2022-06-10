/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_rotation_scheduler.h"

#include "base/logging.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave {

namespace {

constexpr char kLastJsonRotationTimeStampPref[] = "p3a.last_rotation_timestamp";
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

}  // namespace

BraveP3ARotationScheduler::BraveP3ARotationScheduler(
    PrefService* local_state,
    base::TimeDelta json_rotation_interval,
    RotationCallback json_rotation_callback,
    RotationCallback star_rotation_callback)
    : json_rotation_callback_(json_rotation_callback),
      star_rotation_callback_(star_rotation_callback),
      local_state_(local_state),
      json_rotation_interval_(json_rotation_interval) {
  InitStarTimer();
  InitJsonTimer();
}

void BraveP3ARotationScheduler::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterTimePref(kLastJsonRotationTimeStampPref, {});
  registry->RegisterTimePref(kLastStarRotationTimeStampPref, {});
}

BraveP3ARotationScheduler::~BraveP3ARotationScheduler() {}

void BraveP3ARotationScheduler::InitJsonTimer() {
  // Do rotation if needed.
  const base::Time last_rotation =
      local_state_->GetTime(kLastJsonRotationTimeStampPref);
  if (last_rotation.is_null()) {
    star_rotation_callback_.Run();
  } else {
    if (!json_rotation_interval_.is_zero()) {
      if (base::Time::Now() - last_rotation > json_rotation_interval_) {
        star_rotation_callback_.Run();
      }
    }
    if (base::Time::Now() > NextMonday(last_rotation)) {
      star_rotation_callback_.Run();
    }
  }
  UpdateJsonTimer();
}

void BraveP3ARotationScheduler::InitStarTimer() {
  // TODO(djandries): add rotation check logic here

  UpdateStarTimer();
}

void BraveP3ARotationScheduler::UpdateJsonTimer() {
  base::Time now = base::Time::Now();
  base::Time next_rotation = json_rotation_interval_.is_zero()
                                 ? NextMonday(now)
                                 : now + json_rotation_interval_;
  if (now >= next_rotation) {
    // Should never happen, but let's stay on the safe side.
    NOTREACHED();
    return;
  }
  json_rotation_timer_.Start(
      FROM_HERE, next_rotation, this,
      &BraveP3ARotationScheduler::HandleJsonTimerTrigger);

  VLOG(2) << "BraveP3ARotationScheduler new rotation timer will fire at "
          << next_rotation << " after " << next_rotation - now;
}

void BraveP3ARotationScheduler::UpdateStarTimer() {
  // TODO(djandries): add rotation timer update logic here
}

void BraveP3ARotationScheduler::HandleJsonTimerTrigger() {
  local_state_->SetTime(kLastJsonRotationTimeStampPref, base::Time::Now());
  UpdateJsonTimer();
  json_rotation_callback_.Run();
}

void BraveP3ARotationScheduler::HandleStarTimerTrigger() {
  local_state_->SetTime(kLastStarRotationTimeStampPref, base::Time::Now());
  UpdateStarTimer();
  star_rotation_callback_.Run();
}

}  // namespace brave

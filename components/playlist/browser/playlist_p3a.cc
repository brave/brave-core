/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_p3a.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/location.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "brave/components/playlist/browser/pref_names.h"
#include "components/prefs/pref_service.h"

namespace playlist {

constexpr base::TimeDelta kUpdateInterval = base::Days(1);

namespace {

constexpr int kFirstTimeOffsetBuckets[] = {0, 6, 13, 20, 27};

}  // namespace

PlaylistP3A::PlaylistP3A(PrefService* local_state,
                         base::Time browser_first_run_time)
    : local_state_(local_state),
      browser_first_run_time_(browser_first_run_time) {
  CHECK(local_state);
  SetUpTimer();
  Update(false);
}

PlaylistP3A::~PlaylistP3A() = default;

void PlaylistP3A::ReportNewUsage() {
  if (local_state_->GetTime(kPlaylistFirstUsageTime).is_null()) {
    int days_since_install =
        (base::Time::Now() - browser_first_run_time_).InDaysFloored();
    p3a_utils::RecordToHistogramBucket(kFirstTimeOffsetHistogramName,
                                       kFirstTimeOffsetBuckets,
                                       days_since_install);
  }
  p3a_utils::RecordFeatureUsage(local_state_, kPlaylistFirstUsageTime,
                                kPlaylistLastUsageTime);
  Update(true);
}

void PlaylistP3A::SetUpTimer() {
  update_timer_.Start(
      FROM_HERE, base::Time::Now() + kUpdateInterval,
      base::BindOnce(&PlaylistP3A::Update, base::Unretained(this), false));
}

void PlaylistP3A::Update(bool new_usage) {
  p3a_utils::RecordFeatureDaysInWeekUsed(local_state_, new_usage,
                                         kPlaylistUsageWeeklyStorage,
                                         kUsageDaysInWeekHistogramName);
  p3a_utils::RecordFeatureLastUsageTimeMetric(
      local_state_, kPlaylistLastUsageTime, kLastUsageTimeHistogramName);
  p3a_utils::RecordFeatureNewUserReturning(
      local_state_, kPlaylistFirstUsageTime, kPlaylistLastUsageTime,
      kPlaylistUsedSecondDay, kNewUserReturningHistogramName,
      /*write_to_histogram*/ true,
      /*active_users_only*/ true);
  SetUpTimer();
}

}  // namespace playlist

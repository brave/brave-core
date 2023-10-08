/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/p3a/p3a_core_metrics.h"

#include <utility>

#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "brave/components/p3a_utils/bucket.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave {

namespace {

BraveWindowTracker* g_brave_windows_tracker_instance = nullptr;

constexpr char kLastTimeIncognitoUsed[] =
    "core_p3a_metrics.incognito_used_timestamp";
constexpr char kTorUsed[] = "core_p3a_metrics.tor_used";

constexpr size_t kWindowUsageP3AIntervalMinutes = 10;

// Note: append-only enumeration! Never remove any existing values, as this enum
// is used to bucket a UMA histogram, and removing values breaks that.
// Each subsequent "bucket" doesn't include previous bucket (i.e. if the window
// was used 5 days ago, the proper bucket is |kUsedInLastWeek|, not
// |kUsedInLast28Days|.
enum class WindowUsageStats {
  kUsedIn24h,
  kUsedInLastWeek,
  kUsedInLast28Days,
  kEverUsed,
  kNeverUsed,
  kSize,
};

const char* GetPrefNameForProfile(Profile* profile) {
  if (profile->IsIncognitoProfile() &&
      !profile->IsTor()) {
    return kLastTimeIncognitoUsed;
  }
  return nullptr;
}

BraveUptimeTracker* g_brave_uptime_tracker_instance = nullptr;

constexpr base::TimeDelta kUsageTimeQueryInterval = base::Minutes(1);
constexpr base::TimeDelta kUsageTimeReportInterval = base::Days(1);
constexpr char kDailyUptimesListPrefName[] = "daily_uptimes";  // DEPRECATED
constexpr char kDailyUptimeSumPrefName[] = "brave.misc_metrics.uptime_sum";
constexpr char kDailyUptimeFrameStartTimePrefName[] =
    "brave.misc_metrics.uptime_frame_start_time";

constexpr char kBrowserOpenTimeHistogramName[] = "Brave.Uptime.BrowserOpenTime";

constexpr int kBrowserOpenTimeBuckets[] = {30, 60, 120, 180, 300, 420, 600};

}  // namespace

BraveUptimeTracker::BraveUptimeTracker(PrefService* local_state)
    : local_state_(local_state),
      report_frame_start_time_(
          local_state->GetTime(kDailyUptimeFrameStartTimePrefName)),
      report_frame_time_sum_(
          local_state_->GetTimeDelta(kDailyUptimeSumPrefName)) {
  if (report_frame_start_time_.is_null()) {
    // If today is the first time monitoring uptime, set the frame start time
    // to now.
    ResetReportFrame();
  }
  RecordP3A();
  timer_.Start(FROM_HERE, kUsageTimeQueryInterval,
               base::BindRepeating(&BraveUptimeTracker::RecordUsage,
                                   base::Unretained(this)));
}

void BraveUptimeTracker::RecordUsage() {
  const base::TimeDelta new_total = usage_clock_.GetTotalUsageTime();
  const base::TimeDelta total_diff = new_total - current_total_usage_;
  if (total_diff > base::TimeDelta()) {
    report_frame_time_sum_ += total_diff;
    current_total_usage_ = new_total;
    local_state_->SetTimeDelta(kDailyUptimeSumPrefName, report_frame_time_sum_);

    RecordP3A();
  }
}

void BraveUptimeTracker::RecordP3A() {
  if ((base::Time::Now() - report_frame_start_time_) <
      kUsageTimeReportInterval) {
    // Do not report, since 1 day has not passed.
    return;
  }
  p3a_utils::RecordToHistogramBucket(kBrowserOpenTimeHistogramName,
                                     kBrowserOpenTimeBuckets,
                                     report_frame_time_sum_.InMinutes());
  ResetReportFrame();
}

void BraveUptimeTracker::ResetReportFrame() {
  report_frame_time_sum_ = base::TimeDelta();
  report_frame_start_time_ = base::Time::Now();
  local_state_->SetTimeDelta(kDailyUptimeSumPrefName, report_frame_time_sum_);
  local_state_->SetTime(kDailyUptimeFrameStartTimePrefName,
                        report_frame_start_time_);
}

BraveUptimeTracker::~BraveUptimeTracker() = default;

void BraveUptimeTracker::CreateInstance(PrefService* local_state) {
  g_brave_uptime_tracker_instance = new BraveUptimeTracker(local_state);
}

void BraveUptimeTracker::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterTimeDeltaPref(kDailyUptimeSumPrefName, base::TimeDelta());
  registry->RegisterTimePref(kDailyUptimeFrameStartTimePrefName, base::Time());
}

void BraveUptimeTracker::RegisterPrefsForMigration(
    PrefRegistrySimple* registry) {
  // Added 10/2023
  registry->RegisterListPref(kDailyUptimesListPrefName);
}

void BraveUptimeTracker::MigrateObsoletePrefs(PrefService* local_state) {
  // Added 10/2023
  local_state->ClearPref(kDailyUptimesListPrefName);
}

BraveWindowTracker::BraveWindowTracker(PrefService* local_state)
    : local_state_(local_state) {
  if (!local_state) {
    // Can happen in tests.
    return;
  }
  BrowserList::AddObserver(this);
  timer_.Start(FROM_HERE, base::Minutes(kWindowUsageP3AIntervalMinutes),
               base::BindRepeating(&BraveWindowTracker::UpdateP3AValues,
                                   base::Unretained(this)));
  UpdateP3AValues();
}

BraveWindowTracker::~BraveWindowTracker() {
  BrowserList::RemoveObserver(this);
}

void BraveWindowTracker::CreateInstance(PrefService* local_state) {
  g_brave_windows_tracker_instance = new BraveWindowTracker(local_state);
}

void BraveWindowTracker::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterTimePref(kLastTimeIncognitoUsed, {});
  registry->RegisterBooleanPref(kTorUsed, false);
}

void BraveWindowTracker::OnBrowserAdded(Browser* browser) {
  if (browser->profile()->IsTor()) {
    local_state_->SetBoolean(kTorUsed, true);
    return;
  }
  const char* pref = GetPrefNameForProfile(browser->profile());
  if (pref) {
    local_state_->SetTime(pref, base::Time::Now());
  }
}

void BraveWindowTracker::OnBrowserSetLastActive(Browser* browser) {
  const char* pref = GetPrefNameForProfile(browser->profile());
  if (pref) {
    local_state_->SetTime(pref, base::Time::Now());
  }
}

void BraveWindowTracker::UpdateP3AValues() const {
  // Deal with the incognito window.
  WindowUsageStats bucket;
  const base::Time time = local_state_->GetTime(kLastTimeIncognitoUsed);
  const base::Time now = base::Time::Now();
  if (time.is_null()) {
    bucket = WindowUsageStats::kNeverUsed;
  } else if (now - time < base::Hours(24)) {
    bucket = WindowUsageStats::kUsedIn24h;
  } else if (now - time < base::Days(7)) {
    bucket = WindowUsageStats::kUsedInLastWeek;
  } else if (now - time < base::Days(28)) {
    bucket = WindowUsageStats::kUsedInLast28Days;
  } else {
    bucket = WindowUsageStats::kEverUsed;
  }

  UMA_HISTOGRAM_ENUMERATION("Brave.Core.LastTimeIncognitoUsed", bucket,
                            WindowUsageStats::kSize);

  // Record if the TOR window was ever used.
  // 0 -> Yes; 1 -> No.
  const int tor_used = !local_state_->GetBoolean(kTorUsed);
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Core.TorEverUsed", tor_used, 1);
}

}  // namespace brave

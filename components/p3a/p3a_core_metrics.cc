/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/p3a_core_metrics.h"

#include <numeric>

#include "base/metrics/histogram_macros.h"
#include "base/metrics/statistics_recorder.h"
#include "base/time/time.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave {

namespace {
BraveWindowsTracker* g_brave_windows_tracker_instance = nullptr;

constexpr char kLastTimeIncognitoUsed[] =
    "core_p3a_metrics.incognito_used_timestamp";
constexpr char kLastTimeTorUsed[] = "core_p3a_metrics.tor_used_timestamp";

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
  if (profile->IsTorProfile()) {
    return kLastTimeTorUsed;
  } else if (profile->GetProfileType() == Profile::INCOGNITO_PROFILE) {
    return kLastTimeIncognitoUsed;
  }
  return nullptr;
}

// Please keep this list sorted and synced with |DoHistogramBravezation|.
constexpr const char* kBravezationHistograms[] = {
    "Bookmarks.Count.OnProfileLoad",
    "Extensions.LoadExtension",
    "Tabs.TabCount",
    "Tabs.WindowCount",
};

// Records the given sample using the proper Brave way.
void DoHistogramBravezation(base::StringPiece histogram_name,
                            base::HistogramBase::Sample sample) {
  if ("Bookmarks.Count.OnProfileLoad" == histogram_name) {
    int answer = 0;
    if (0 <= sample && sample < 5)
      answer = 0;
    if (5 <= sample && sample < 20)
      answer = 1;
    if (20 <= sample && sample < 100)
      answer = 2;
    if (sample >= 100)
      answer = 3;

    UMA_HISTOGRAM_EXACT_LINEAR("Brave.Core.BookmarksCountOnProfileLoad", answer,
                               3);
    return;
  }

  if ("Extensions.LoadExtension" == histogram_name) {
    int answer = 0;
    if (sample == 1)
      answer = 1;
    else if (2 <= sample && sample <= 4)
      answer = 2;
    else if (sample >= 5)
      answer = 3;

    UMA_HISTOGRAM_EXACT_LINEAR("Brave.Core.NumberOfExtensions", answer, 3);
    return;
  }

  if ("Tabs.TabCount" == histogram_name) {
    int answer = 0;
    if (0 <= sample && sample <= 1) {
      answer = 0;
    } else if (2 <= sample && sample <= 5) {
      answer = 1;
    } else if (6 <= sample && sample <= 10) {
      answer = 2;
    } else if (11 <= sample && sample <= 50) {
      answer = 3;
    } else
      answer = 4;

    UMA_HISTOGRAM_EXACT_LINEAR("Brave.Core.TabCount", answer, 4);
    return;
  }

  if ("Tabs.WindowCount" == histogram_name) {
    int answer = 0;
    if (0 <= sample && sample <= 1) {
      answer = 0;
    } else if (2 <= sample && sample <= 5) {
      answer = 1;
    } else {
      answer = 2;
    }

    UMA_HISTOGRAM_EXACT_LINEAR("Brave.Core.WindowCount", answer, 2);
    return;
  }
}

BraveUptimeTracker* g_brave_uptime_tracker_instance = nullptr;

constexpr size_t kUsageTimeQueryIntervalMinutes = 1;
constexpr size_t kNumOfSavedDailyUptimes = 7;
constexpr char kDailyUptimesListPrefName[] = "daily_uptimes";

}  // namespace

UsagePermanentState::UsagePermanentState(PrefService* local_state)
    : local_state_(local_state) {
  if (local_state) {
    LoadUptimes();
  }
}

UsagePermanentState::~UsagePermanentState() = default;

void UsagePermanentState::AddInterval(base::TimeDelta delta) {
  base::Time now_midnight = base::Time::Now().LocalMidnight();
  base::Time last_saved_midnight;

  if (!daily_uptimes_.empty()) {
    last_saved_midnight = daily_uptimes_.front().day;
  }

  if (now_midnight - last_saved_midnight > base::TimeDelta()) {
    // Day changed. Since we consider only small incoming intervals, lets just
    // save it with a new timestamp.
    daily_uptimes_.push_front({now_midnight, delta});
    if (daily_uptimes_.size() > kNumOfSavedDailyUptimes) {
      daily_uptimes_.pop_back();
    }
  } else {
    daily_uptimes_.front().uptime += delta;
  }

  RecordP3A();
  SaveUptimes();
}

base::TimeDelta UsagePermanentState::GetTotalUsage() const {
  // We record only uptime for last N days.
  const base::Time n_days_ago =
      base::Time::Now() - base::TimeDelta::FromDays(kNumOfSavedDailyUptimes);
  return std::accumulate(daily_uptimes_.begin(), daily_uptimes_.end(),
                         DailyUptime(),
                         [n_days_ago](const auto& u1, const auto& u2) {
                           base::TimeDelta add;
                           // Check only last continious days.
                           if (u2.day > n_days_ago) {
                             add = u2.uptime;
                           }
                           return DailyUptime{{}, u1.uptime + add};
                         })
      .uptime;
}

void UsagePermanentState::LoadUptimes() {
  DCHECK(daily_uptimes_.empty());
  const base::ListValue* list =
      local_state_->GetList(kDailyUptimesListPrefName);
  if (!list) {
    return;
  }
  for (auto it = list->begin(); it != list->end(); ++it) {
    const base::Value* day = it->FindKey("day");
    const base::Value* uptime = it->FindKey("uptime");
    if (!day || !uptime || !day->is_double() || !uptime->is_double()) {
      continue;
    }
    if (daily_uptimes_.size() == kNumOfSavedDailyUptimes) {
      break;
    }
    daily_uptimes_.push_back(
        {base::Time::FromDoubleT(day->GetDouble()),
         base::TimeDelta::FromSecondsD(uptime->GetDouble())});
  }
}

void UsagePermanentState::SaveUptimes() {
  DCHECK(!daily_uptimes_.empty());
  DCHECK_LE(daily_uptimes_.size(), kNumOfSavedDailyUptimes);

  ListPrefUpdate update(local_state_, kDailyUptimesListPrefName);
  base::ListValue* list = update.Get();
  // TODO(iefremov): Optimize if needed.
  list->Clear();
  for (const auto& u : daily_uptimes_) {
    base::DictionaryValue value;
    value.SetKey("day", base::Value(u.day.ToDoubleT()));
    value.SetKey("uptime", base::Value(u.uptime.InSecondsF()));
    list->GetList().push_back(std::move(value));
  }
}

void UsagePermanentState::RecordP3A() {
  int answer = 0;
  if (daily_uptimes_.size() == kNumOfSavedDailyUptimes) {
    base::TimeDelta total = GetTotalUsage();
    const int minutes = total.InMinutes();
    DCHECK(minutes >= 0);
    if (0 <= minutes && minutes < 30) {
      answer = 1;
    } else if (30 <= minutes && minutes < 5 * 60) {
      answer = 2;
    } else {
      answer = 3;
    }
  }
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Uptime.BrowserOpenMinutes", answer, 3);
}

BraveUptimeTracker::BraveUptimeTracker(PrefService* local_state)
    : state_(local_state) {
  timer_.Start(
      FROM_HERE, base::TimeDelta::FromMinutes(kUsageTimeQueryIntervalMinutes),
      base::Bind(&BraveUptimeTracker::RecordUsage, base::Unretained(this)));
}

void BraveUptimeTracker::RecordUsage() {
  const base::TimeDelta new_total = usage_clock_.GetTotalUsageTime();
  const base::TimeDelta interval = new_total - current_total_usage_;
  if (interval > base::TimeDelta()) {
    state_.AddInterval(interval);
    current_total_usage_ = new_total;
  }
}

BraveUptimeTracker::~BraveUptimeTracker() = default;

void BraveUptimeTracker::CreateInstance(PrefService* local_state) {
  g_brave_uptime_tracker_instance = new BraveUptimeTracker(local_state);
}

void BraveUptimeTracker::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kDailyUptimesListPrefName);
}

BraveWindowsTracker::BraveWindowsTracker(PrefService* local_state)
    : local_state_(local_state) {
  if (!local_state) {
    // Can happen in tests.
    return;
  }
  BrowserList::AddObserver(this);
  timer_.Start(FROM_HERE,
               base::TimeDelta::FromMinutes(kWindowUsageP3AIntervalMinutes),
               base::Bind(&BraveWindowsTracker::UpdateP3AValues,
                          base::Unretained(this)));
  UpdateP3AValues();
}

BraveWindowsTracker::~BraveWindowsTracker() {
  BrowserList::RemoveObserver(this);
}

void BraveWindowsTracker::CreateInstance(PrefService* local_state) {
  g_brave_windows_tracker_instance = new BraveWindowsTracker(local_state);
}

void BraveWindowsTracker::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterTimePref(kLastTimeTorUsed, {});
  registry->RegisterTimePref(kLastTimeIncognitoUsed, {});
}

void BraveWindowsTracker::OnBrowserAdded(Browser* browser) {
  const char* pref = GetPrefNameForProfile(browser->profile());
  if (pref) {
    local_state_->SetTime(pref, base::Time::Now());
  }
}

void BraveWindowsTracker::OnBrowserSetLastActive(Browser* browser) {
  const char* pref = GetPrefNameForProfile(browser->profile());
  if (pref) {
    local_state_->SetTime(pref, base::Time::Now());
  }
}

void BraveWindowsTracker::UpdateP3AValues() const {
  auto get_bucket = [this](const char pref[]) {
    const base::Time time = local_state_->GetTime(pref);
    const base::Time now = base::Time::Now();
    if (time.is_null()) {
      return WindowUsageStats::kNeverUsed;
    } else if (now - time < base::TimeDelta::FromHours(24)) {
      return WindowUsageStats::kUsedIn24h;
    } else if (now - time < base::TimeDelta::FromDays(7)) {
      return WindowUsageStats::kUsedInLastWeek;
    } else if (now - time < base::TimeDelta::FromDays(28)) {
      return WindowUsageStats::kUsedInLast28Days;
    } else {
      return WindowUsageStats::kEverUsed;
    }
    NOTREACHED();
    return WindowUsageStats::kNeverUsed;
  };

  UMA_HISTOGRAM_ENUMERATION("Brave.Core.LastTimeTorUsed",
                            get_bucket(kLastTimeTorUsed),
                            WindowUsageStats::kSize);

  UMA_HISTOGRAM_ENUMERATION("Brave.Core.LastTimeIncognitoUsed",
                            get_bucket(kLastTimeIncognitoUsed),
                            WindowUsageStats::kSize);

  // 0 -> Yes; 1 -> No.
  const int tor_used = local_state_->GetTime(kLastTimeTorUsed).is_null();
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Core.TorEverUsed", tor_used, 1);
}

void SetupHistogramsBraveization() {
  for (const char* histogram_name : kBravezationHistograms) {
    base::StatisticsRecorder::SetCallback(
        histogram_name,
        base::BindRepeating(&DoHistogramBravezation, histogram_name));
  }
}

}  // namespace brave

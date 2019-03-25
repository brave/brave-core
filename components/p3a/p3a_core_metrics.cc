/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/p3a_core_metrics.h"

#include "base/metrics/histogram_macros.h"
#include "base/metrics/statistics_recorder.h"
#include "base/time/time.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

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
    if (0 <= sample && sample <= 1)
      answer = 0;
    if (2 <= sample && sample <= 5)
      answer = 1;
    if (6 <= sample && sample <= 10)
      answer = 2;
    if (11 <= sample && sample <= 30)
      answer = 3;
    if (30 <= sample && sample <= 100)
      answer = 4;
    if (101 <= sample)
      answer = 5;

    UMA_HISTOGRAM_EXACT_LINEAR("Brave.Core.TabCount", answer, 5);
    return;
  }

  if ("Tabs.WindowCount" == histogram_name) {
    int answer = 0;
    if (0 <= sample && sample <= 1)
      answer = 0;
    if (2 <= sample && sample <= 5)
      answer = 1;
    if (6 <= sample && sample <= 10)
      answer = 2;
    if (11 <= sample)
      answer = 3;

    UMA_HISTOGRAM_EXACT_LINEAR("Brave.Core.WindowCount", answer, 3);
    return;
  }
}

}  // namespace

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
}

void SetupHistogramsBraveization() {
  for (const char* histogram_name : kBravezationHistograms) {
    base::StatisticsRecorder::SetCallback(
        histogram_name,
        base::BindRepeating(&DoHistogramBravezation, histogram_name));
  }
}

}  // namespace brave

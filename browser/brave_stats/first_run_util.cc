/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_stats/first_run_util.h"

#include "base/threading/thread_restrictions.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/first_run/first_run.h"
#include "components/prefs/pref_service.h"

namespace brave_stats {

#if BUILDFLAG(IS_ANDROID)
namespace {

bool g_is_first_run = false;

base::Time GetAndroidFirstRunTimePrefValue(PrefService* local_state) {
  return local_state->GetTime(kReferralAndroidFirstRunTimestamp);
}

void InitAndroidFirstRunTime(PrefService* local_state) {
  if (GetAndroidFirstRunTimePrefValue(local_state).is_null()) {
    base::Time now = base::Time::Now();
    local_state->SetTime(kReferralAndroidFirstRunTimestamp, now);
    g_is_first_run = true;
  }
}

}  // namespace

void ResetAndroidFirstRunStateForTesting() {
  g_is_first_run = false;
}

#endif  // #BUILDFLAG(IS_ANDROID)

base::Time GetFirstRunTime([[maybe_unused]] PrefService* local_state) {
#if BUILDFLAG(IS_ANDROID)
  CHECK(local_state);
  // Android doesn't use a sentinel to track first run, so we use a
  // preference instead. kReferralAndroidFirstRunTimestamp is used because
  // previously only referrals needed to know the first run value.
  InitAndroidFirstRunTime(local_state);
  return GetAndroidFirstRunTimePrefValue(local_state);
#else
  // CreateSentinelIfNeeded() is called in chrome_browser_main.cc, making this a
  // non-blocking read of the cached sentinel value when running from production
  // code. However tests will never create the sentinel file due to being run
  // with the switches:kNoFirstRun flag, so we need to allow blocking for that.
  base::ScopedAllowBlockingForTesting allow_blocking;
  return first_run::GetFirstRunSentinelCreationTime();
#endif  // #BUILDFLAG(IS_ANDROID)
}

bool IsFirstRun([[maybe_unused]] PrefService* local_state) {
#if BUILDFLAG(IS_ANDROID)
  if (g_is_first_run) {
    return true;
  }
  InitAndroidFirstRunTime(local_state);
  return g_is_first_run;
#else
  return first_run::IsChromeFirstRun();
#endif  // #BUILDFLAG(IS_ANDROID)
}

}  // namespace brave_stats

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

base::Time GetFirstRunTime(PrefService* local_state) {
#if BUILDFLAG(IS_ANDROID)
  // Android doesn't use a sentinel to track first run, so we use a
  // preference instead. kReferralAndroidFirstRunTimestamp is used because
  // previously only referrals needed to know the first run value.
  base::Time first_run_timestamp =
      local_state->GetTime(kReferralAndroidFirstRunTimestamp);
  if (first_run_timestamp.is_null()) {
    first_run_timestamp = base::Time::Now();
    local_state->SetTime(kReferralAndroidFirstRunTimestamp,
                         first_run_timestamp);
  }
  return first_run_timestamp;
#else
  (void)local_state;  // suppress unused warning

  // CreateSentinelIfNeeded() is called in chrome_browser_main.cc, making this a
  // non-blocking read of the cached sentinel value when running from production
  // code. However tests will never create the sentinel file due to being run
  // with the switches:kNoFirstRun flag, so we need to allow blocking for that.
  base::ScopedAllowBlockingForTesting allow_blocking;
  return first_run::GetFirstRunSentinelCreationTime();
#endif  // #BUILDFLAG(IS_ANDROID)
}

}  // namespace brave_stats

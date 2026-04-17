/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/brave_stats/brave_stats.h"

#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "brave/components/brave_stats/browser/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/webcompat_reporter/buildflags/buildflags.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

NSString* const kBraveStatsAPIKey = @BUILDFLAG(BRAVE_STATS_API_KEY);
NSString* const kWebcompatReportEndpoint =
    @BUILDFLAG(WEBCOMPAT_REPORT_ENDPOINT);

@implementation BraveStats {
  raw_ptr<PrefService> _localPrefs;
  raw_ptr<PrefService> _profilePrefs;
}

- (instancetype)initWithBrowserState:(ProfileIOS*)profile {
  if ((self = [super init])) {
    _profilePrefs = profile->GetPrefs();
    _localPrefs = GetApplicationContext()->GetLocalState();
  }
  return self;
}

- (BOOL)isStatsReportingManaged {
  return _localPrefs->IsManagedPreference(kStatsReportingEnabled);
}

- (BOOL)isStatsReportingEnabled {
  return _localPrefs->GetBoolean(kStatsReportingEnabled);
}

- (void)setStatsReportingEnabled:(BOOL)statsReportingEnabled {
  _localPrefs->SetBoolean(kStatsReportingEnabled, statsReportingEnabled);
}

- (BOOL)isNotificationAdsEnabled {
  return _profilePrefs->GetBoolean(brave_ads::prefs::kOptedInToNotificationAds);
}

- (nullable NSDate*)lastPingDate {
  const std::string ymd = _localPrefs->GetString(kLastCheckYMD);
  if (ymd.empty()) {
    return nil;
  }
  // Daily usage pings on iOS use UTC for the date boundary.
  return brave_stats::GetYMDAsDate(ymd, /*use_utc=*/true).ToNSDate();
}

- (void)setLastPingDate:(NSDate*)date {
  if (!date) {
    _localPrefs->SetString(kLastCheckYMD, std::string());
    return;
  }
  // Daily usage pings on iOS use UTC for the date boundary.
  _localPrefs->SetString(kLastCheckYMD,
                         brave_stats::GetDateAsYMD(base::Time::FromNSDate(date),
                                                   /*use_utc=*/true));
}

@end

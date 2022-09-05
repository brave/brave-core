/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/brave_stats/brave_stats.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/application_context/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation BraveStats {
  PrefService* _localPrefs;
  PrefService* _profilePrefs;
}

- (instancetype)initWithBrowserState:(ChromeBrowserState*)browserState {
  if ((self = [super init])) {
    _profilePrefs = browserState->GetPrefs();
    _localPrefs = GetApplicationContext()->GetLocalState();
  }
  return self;
}

- (NSDictionary<NSString*, NSString*>*)walletParams {
  auto wallet_last_unlocked =
      _profilePrefs->GetTime(kBraveWalletLastUnlockTime);
  auto last_reported_wallet_unlock =
      _localPrefs->GetTime(kBraveWalletPingReportedUnlockTime);
  uint8_t usage_bitset = 0;
  if (wallet_last_unlocked > last_reported_wallet_unlock) {
    usage_bitset = brave_stats::UsageBitfieldFromTimestamp(
        wallet_last_unlocked, last_reported_wallet_unlock);
  }
  return @{@"wallet2" : base::SysUTF8ToNSString(std::to_string(usage_bitset))};
}

- (void)notifyStatsPingSent {
  auto wallet_last_unlocked =
      _profilePrefs->GetTime(kBraveWalletLastUnlockTime);
  _localPrefs->SetTime(kBraveWalletPingReportedUnlockTime,
                       wallet_last_unlocked);
}

@end
